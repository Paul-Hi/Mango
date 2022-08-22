//! \file      hi_z_pass.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2022
//! \copyright Apache License 2.0

#include <mango/profile.hpp>
#include <rendering/passes/hi_z_pass.hpp>
#include <rendering/renderer_bindings.hpp>
#include <resources/resources_impl.hpp>

using namespace mango;

const render_pass_execution_info hi_z_pass::s_rpei{ 0, 0 };

void hi_z_pass::attach(const shared_ptr<context_impl>& context)
{
    m_shared_context = context;

    create_pass_resources();
}

void hi_z_pass::execute(graphics_device_context_handle& device_context)
{
    GL_NAMED_PROFILE_ZONE("Hi-Z Pass");
    NAMED_PROFILE_ZONE("Hi-Z Pass");
    auto& graphics_device = m_shared_context->get_graphics_device();

    device_context->bind_pipeline(m_hi_z_construction_pipeline);

    MANGO_ASSERT(m_hi_z_texture, "Hi-Z texture does not exist!");

    uint32 number_mips = graphics::calculate_mip_count(m_depth_width, m_depth_height);

    m_hi_z_construction_pipeline->get_resource_mapping()->set("hi_z_data", m_hi_z_data_buffer);
    m_hi_z_construction_pipeline->get_resource_mapping()->set("sampler_depth_input", m_nearest_sampler);

    ivec2 out_size = ivec2(m_depth_width, m_depth_height);
    m_hi_z_data.params = vec4(float(out_size.x()), float(out_size.y()), 1.0f / float(out_size.x()), 1.0f / float(out_size.y()));
    for (uint32 i = 0; i < number_mips; ++i)
    {
        auto depth_mip_view = graphics_device->create_image_texture_view(m_hi_z_texture, i);

        m_hi_z_data.pass = i;
        device_context->set_buffer_data(m_hi_z_data_buffer, 0, sizeof(hi_z_data), &m_hi_z_data);

        m_hi_z_construction_pipeline->get_resource_mapping()->set("image_hi_z_output", depth_mip_view);
        m_hi_z_construction_pipeline->get_resource_mapping()->set("texure_depth_input", i == 0 ? m_depth_texture : m_hi_z_texture);
        device_context->submit_pipeline_state_resources();

        device_context->dispatch(max(1, ceil(float(out_size.x()) / 16)), max(1, ceil(float(out_size.y()) / 16)), 1);

        m_hi_z_data.params.z() = out_size.x();
        m_hi_z_data.params.w() = out_size.y();
        out_size.x() >>= 1;
        out_size.y() >>= 1;
        m_hi_z_data.params.x() = max(1, out_size.x());
        m_hi_z_data.params.y() = max(1, out_size.y());
    }
}

bool hi_z_pass::create_pass_resources()
{
    PROFILE_ZONE;
    auto& graphics_device    = m_shared_context->get_graphics_device();
    auto& internal_resources = m_shared_context->get_internal_resources();

    shader_stage_create_info shader_info;
    shader_resource_resource_description res_resource_desc;
    shader_source_description source_desc;

    // texture
    if (!recreate_hi_z_texture())
        return false;

    // buffer
    buffer_create_info buffer_info;
    buffer_info.buffer_target = gfx_buffer_target::buffer_target_uniform;
    buffer_info.buffer_access = gfx_buffer_access::buffer_access_dynamic_storage;
    buffer_info.size          = sizeof(luminance_data);
    m_hi_z_data_buffer        = graphics_device->create_buffer(buffer_info);
    if (!check_creation(m_hi_z_data_buffer.get(), "hi-z data buffer"))
        return false;

    // Hi-Z Construction Compute Stage
    {
        res_resource_desc.path = "res/shader/hi_z_compute/c_hi_z_construction.glsl";
        res_resource_desc.defines.push_back({ "COMPUTE", "" });
        const shader_resource* source = internal_resources->acquire(res_resource_desc);

        source_desc.entry_point = "main";
        source_desc.source      = source->source.c_str();
        source_desc.size        = static_cast<int32>(source->source.size());

        shader_info.stage         = gfx_shader_stage_type::shader_stage_compute;
        shader_info.shader_source = source_desc;

        shader_info.resource_count = 4;

        shader_info.resources = { {
            { gfx_shader_stage_type::shader_stage_compute, HI_Z_DATA_BUFFER_BINDING_POINT, "hi_z_data", gfx_shader_resource_type::shader_resource_constant_buffer, 1 },
            { gfx_shader_stage_type::shader_stage_compute, HI_Z_DEPTH_SAMPLER, "texure_depth_input", gfx_shader_resource_type::shader_resource_texture, 1 },
            { gfx_shader_stage_type::shader_stage_compute, HI_Z_DEPTH_SAMPLER, "sampler_depth_input", gfx_shader_resource_type::shader_resource_sampler, 1 },
            { gfx_shader_stage_type::shader_stage_compute, HI_Z_IMAGE_COMPUTE, "image_hi_z_output", gfx_shader_resource_type::shader_resource_image_storage, 1 },
        } };

        m_hi_z_compute = graphics_device->create_shader_stage(shader_info);
        if (!check_creation(m_hi_z_compute.get(), "hi-z construction compute shader"))
            return false;

        res_resource_desc.defines.clear();
    }

    compute_pipeline_create_info construction_pass_info = graphics_device->provide_compute_pipeline_create_info();
    auto construction_pass_pipeline_layout              = graphics_device->create_pipeline_resource_layout({
                     { gfx_shader_stage_type::shader_stage_compute, HI_Z_DATA_BUFFER_BINDING_POINT, gfx_shader_resource_type::shader_resource_constant_buffer, gfx_shader_resource_access::shader_access_dynamic },
                     { gfx_shader_stage_type::shader_stage_compute, HI_Z_DEPTH_SAMPLER, gfx_shader_resource_type::shader_resource_texture, gfx_shader_resource_access::shader_access_dynamic },
                     { gfx_shader_stage_type::shader_stage_compute, HI_Z_DEPTH_SAMPLER, gfx_shader_resource_type::shader_resource_sampler, gfx_shader_resource_access::shader_access_dynamic },
                     { gfx_shader_stage_type::shader_stage_compute, HI_Z_IMAGE_COMPUTE, gfx_shader_resource_type::shader_resource_image_storage, gfx_shader_resource_access::shader_access_dynamic },
    });

    construction_pass_info.pipeline_layout = construction_pass_pipeline_layout;

    construction_pass_info.shader_stage_descriptor.compute_shader_stage = m_hi_z_compute;

    m_hi_z_construction_pipeline = graphics_device->create_compute_pipeline(construction_pass_info);

    return true;
}

bool hi_z_pass::recreate_hi_z_texture()
{
    auto& graphics_device = m_shared_context->get_graphics_device();

    const int32& w = m_depth_width;
    const int32& h = m_depth_height;

    texture_create_info attachment_info;
    attachment_info.texture_type   = gfx_texture_type::texture_type_2d;
    attachment_info.width          = w;
    attachment_info.height         = h;
    attachment_info.miplevels      = graphics::calculate_mip_count(w, h);
    attachment_info.array_layers   = 1;
    attachment_info.texture_format = gfx_format::rg32f;
    m_hi_z_texture                 = graphics_device->create_texture(attachment_info);
    if (!check_creation(m_hi_z_texture.get(), "hi-z texture"))
        return false;

    graphics_device_context_handle device_context = graphics_device->create_graphics_device_context();
    device_context->begin();
    device_context->calculate_mipmaps(m_hi_z_texture);
    device_context->end();
    device_context->submit();

    return true;
}