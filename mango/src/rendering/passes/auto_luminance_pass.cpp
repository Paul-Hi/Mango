//! \file      auto_luminance_pass.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2022
//! \copyright Apache License 2.0

#include <mango/profile.hpp>
#include <rendering/passes/auto_luminance_pass.hpp>
#include <rendering/renderer_bindings.hpp>
#include <resources/resources_impl.hpp>

using namespace mango;

const render_pass_execution_info auto_luminance_pass::s_rpei{ 0, 0 };

void auto_luminance_pass::attach(const shared_ptr<context_impl>& context)
{
    m_shared_context = context;

    create_pass_resources();
}

void auto_luminance_pass::execute(graphics_device_context_handle& device_context)
{
    GL_NAMED_PROFILE_ZONE("Auto Exposure Calculation");
    NAMED_PROFILE_ZONE("Auto Exposure Calculation");

    auto& graphics_device    = m_shared_context->get_graphics_device();

    device_context->bind_pipeline(m_luminance_construction_pipeline);

    device_context->calculate_mipmaps(m_hdr_input);

    int32 mip_level = 0;
    int32 hr_width  = m_input_width;
    int32 hr_height = m_input_height;
    while (hr_width >> mip_level > 512 && hr_height >> mip_level > 512) // we can make it smaller, when we have some better focussing.
    {
        ++mip_level;
    }
    hr_width >>= mip_level;
    hr_height >>= mip_level;

    barrier_description bd;
    bd.barrier_bit = gfx_barrier_bit::shader_image_access_barrier_bit;
    device_context->barrier(bd);

    auto hdr_view = graphics_device->create_image_texture_view(m_hdr_input, mip_level);

    // time coefficient with tau = 1.1;
    float tau                        = 1.1f;
    float time_coefficient           = 1.0f - expf(-m_dt * tau);
    m_luminance_data_mapping->params = vec4(-8.0f, 1.0f / 31.0f, time_coefficient, static_cast<float>(hr_width * hr_height)); // min -8.0, max +23.0

    m_luminance_construction_pipeline->get_resource_mapping()->set("image_hdr_color", hdr_view);
    m_luminance_construction_pipeline->get_resource_mapping()->set("luminance_data", m_luminance_data_buffer);
    device_context->submit_pipeline_state_resources();

    device_context->dispatch(hr_width / 16, hr_height / 16, 1);

    bd.barrier_bit = gfx_barrier_bit::shader_storage_barrier_bit;
    device_context->barrier(bd);

    device_context->bind_pipeline(m_luminance_reduction_pipeline);

    m_luminance_reduction_pipeline->get_resource_mapping()->set("luminance_data", m_luminance_data_buffer);
    device_context->submit_pipeline_state_resources();

    device_context->dispatch(1, 1, 1);

    bd.barrier_bit = gfx_barrier_bit::buffer_update_barrier_bit;
    device_context->barrier(bd);
}

bool auto_luminance_pass::create_pass_resources()
{
    PROFILE_ZONE;
    auto& graphics_device    = m_shared_context->get_graphics_device();
    auto& internal_resources = m_shared_context->get_internal_resources();

    // buffers
    buffer_create_info buffer_info;
    buffer_info.buffer_target = gfx_buffer_target::buffer_target_shader_storage;
    buffer_info.buffer_access = gfx_buffer_access::buffer_access_mapped_access_read_write;
    buffer_info.size          = sizeof(luminance_data);
    m_luminance_data_buffer   = graphics_device->create_buffer(buffer_info);
    if (!check_creation(m_luminance_data_buffer.get(), "luminance data buffer"))
        return false;
    m_luminance_data_mapping                      = nullptr;
    graphics_device_context_handle device_context = graphics_device->create_graphics_device_context();
    device_context->begin();
    m_luminance_data_mapping = static_cast<luminance_data*>(device_context->map_buffer_data(m_luminance_data_buffer, 0, sizeof(luminance_data)));
    device_context->end();
    device_context->submit();
    if (!check_mapping(m_luminance_data_mapping, "luminance data buffer"))
        return false;

    memset(m_luminance_data_mapping, 0, sizeof(luminance_data));
    m_luminance_data_mapping->luminance = 1.0f;

    shader_stage_create_info shader_info;
    shader_resource_resource_description res_resource_desc;
    shader_source_description source_desc;

    // Luminance Construction Compute Stage
    {
        res_resource_desc.path = "res/shader/luminance_compute/c_construct_luminance_buffer.glsl";
        res_resource_desc.defines.push_back({ "COMPUTE", "" });
        const shader_resource* source = internal_resources->acquire(res_resource_desc);

        source_desc.entry_point = "main";
        source_desc.source      = source->source.c_str();
        source_desc.size        = static_cast<int32>(source->source.size());

        shader_info.stage         = gfx_shader_stage_type::shader_stage_compute;
        shader_info.shader_source = source_desc;

        shader_info.resource_count = 2;

        shader_info.resources = { {
            { gfx_shader_stage_type::shader_stage_compute, LUMINANCE_DATA_BUFFER_BINDING_POINT, "luminance_data", gfx_shader_resource_type::shader_resource_buffer_storage, 1 },
            { gfx_shader_stage_type::shader_stage_compute, HDR_IMAGE_LUMINANCE_COMPUTE, "image_hdr_color", gfx_shader_resource_type::shader_resource_image_storage, 1 },
        } };

        m_luminance_construction_compute = graphics_device->create_shader_stage(shader_info);
        if (!check_creation(m_luminance_construction_compute.get(), "luminance construction compute shader"))
            return false;

        res_resource_desc.defines.clear();
    }
    // Luminance Reduction Compute Stage
    {
        res_resource_desc.path = "res/shader/luminance_compute/c_luminance_buffer_reduction.glsl";
        res_resource_desc.defines.push_back({ "COMPUTE", "" });
        const shader_resource* source = internal_resources->acquire(res_resource_desc);

        source_desc.entry_point = "main";
        source_desc.source      = source->source.c_str();
        source_desc.size        = static_cast<int32>(source->source.size());

        shader_info.stage         = gfx_shader_stage_type::shader_stage_compute;
        shader_info.shader_source = source_desc;

        shader_info.resource_count = 1;

        shader_info.resources = { {
            { gfx_shader_stage_type::shader_stage_compute, LUMINANCE_DATA_BUFFER_BINDING_POINT, "luminance_data", gfx_shader_resource_type::shader_resource_buffer_storage, 1 },
        } };

        m_luminance_reduction_compute = graphics_device->create_shader_stage(shader_info);
        if (!check_creation(m_luminance_reduction_compute.get(), "luminance reduction compute shader"))
            return false;

        res_resource_desc.defines.clear();
    }

    // Luminance Construction Pipeline
    {
        compute_pipeline_create_info construction_pass_info = graphics_device->provide_compute_pipeline_create_info();
        auto construction_pass_pipeline_layout              = graphics_device->create_pipeline_resource_layout({
                         { gfx_shader_stage_type::shader_stage_compute, LUMINANCE_DATA_BUFFER_BINDING_POINT, gfx_shader_resource_type::shader_resource_buffer_storage,
                           gfx_shader_resource_access::shader_access_dynamic },

                         { gfx_shader_stage_type::shader_stage_compute, HDR_IMAGE_LUMINANCE_COMPUTE, gfx_shader_resource_type::shader_resource_image_storage, gfx_shader_resource_access::shader_access_dynamic },
        });

        construction_pass_info.pipeline_layout = construction_pass_pipeline_layout;

        construction_pass_info.shader_stage_descriptor.compute_shader_stage = m_luminance_construction_compute;

        m_luminance_construction_pipeline = graphics_device->create_compute_pipeline(construction_pass_info);
    }
    // Luminance Reduction Pipeline
    {
        compute_pipeline_create_info reduction_pass_info = graphics_device->provide_compute_pipeline_create_info();
        auto reduction_pass_pipeline_layout              = graphics_device->create_pipeline_resource_layout({
                         { gfx_shader_stage_type::shader_stage_compute, LUMINANCE_DATA_BUFFER_BINDING_POINT, gfx_shader_resource_type::shader_resource_buffer_storage,
                           gfx_shader_resource_access::shader_access_dynamic },
        });

        reduction_pass_info.pipeline_layout = reduction_pass_pipeline_layout;

        reduction_pass_info.shader_stage_descriptor.compute_shader_stage = m_luminance_reduction_compute;

        m_luminance_reduction_pipeline = graphics_device->create_compute_pipeline(reduction_pass_info);
    }

    return true;
}
