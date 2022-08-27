//! \file      gtao_pass.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2022
//! \copyright Apache License 2.0

#include <mango/imgui_helper.hpp>
#include <mango/profile.hpp>
#include <rendering/passes/gtao_pass.hpp>
#include <rendering/renderer_bindings.hpp>
#include <resources/resources_impl.hpp>

using namespace mango;

const render_pass_execution_info gtao_pass::s_rpei{ 3, 9 };

gtao_pass::gtao_pass(const gtao_settings& settings)
    : m_settings(settings)
{
    PROFILE_ZONE;

    m_gtao_data.ao_radius                  = settings.get_ao_radius();
    m_gtao_data.thin_occluder_compensation = settings.get_thin_occluder_compensation();
    m_gtao_data.slices                     = settings.get_slices();
    m_gtao_data.direction_samples          = settings.get_direction_samples();
    m_gtao_data.multi_bounce               = settings.get_multi_bounce();
    m_gtao_data.power                      = settings.get_power();

    m_viewport = gfx_viewport{ 0.0f, 0.0f, 2.0f, 2.0f };
}

void gtao_pass::attach(const shared_ptr<context_impl>& context)
{
    m_shared_context = context;

    create_pass_resources();
}

void gtao_pass::execute(graphics_device_context_handle& device_context)
{
    GL_NAMED_PROFILE_ZONE("GTAO Pass");
    NAMED_PROFILE_ZONE("GTAO Pass");
    device_context->bind_pipeline(m_gtao_pass_pipeline);

    auto real_vp   = m_viewport;
    real_vp.width  = (int32)m_viewport.width >> 1;
    real_vp.height = (int32)m_viewport.height >> 1;
    device_context->set_viewport(0, 1, &real_vp);

    device_context->set_render_targets(1, &m_gtao_texture0, nullptr);

    m_gtao_data.depth_mip_count = m_mip_count;

    device_context->set_buffer_data(m_gtao_data_buffer, 0, sizeof(m_gtao_data), &m_gtao_data);

    m_gtao_pass_pipeline->get_resource_mapping()->set("camera_data", m_camera_data_buffer);
    m_gtao_pass_pipeline->get_resource_mapping()->set("gtao_data", m_gtao_data_buffer);
    m_gtao_pass_pipeline->get_resource_mapping()->set("texture_hierarchical_depth", m_hierarchical_depth_texture);
    m_gtao_pass_pipeline->get_resource_mapping()->set("sampler_hierarchical_depth", m_nearest_sampler);
    m_gtao_pass_pipeline->get_resource_mapping()->set("texture_normal", m_normal_texture);
    m_gtao_pass_pipeline->get_resource_mapping()->set("sampler_normal", m_nearest_sampler);

    device_context->submit_pipeline_state_resources();

    device_context->set_index_buffer(nullptr, gfx_format::invalid);
    device_context->set_vertex_buffers(0, nullptr, nullptr, nullptr);

    device_context->draw(3, 0, 1, 0, 0, 0); // Triangle gets created in vertex shader.

    GL_NAMED_PROFILE_ZONE("GTAO Spatial Denoise Pass");
    NAMED_PROFILE_ZONE("GTAO Spatial Denoise Pass");
    device_context->bind_pipeline(m_spatial_denoise_pipeline);

    device_context->set_viewport(0, 1, &real_vp);

    device_context->set_render_targets(1, &m_gtao_texture1, nullptr);

    m_spatial_denoise_pipeline->get_resource_mapping()->set("texture_gtao", m_gtao_texture0);
    m_spatial_denoise_pipeline->get_resource_mapping()->set("sampler_gtao", m_nearest_sampler);
    m_spatial_denoise_pipeline->get_resource_mapping()->set("texture_depth", m_depth_texture);
    m_spatial_denoise_pipeline->get_resource_mapping()->set("sampler_depth", m_nearest_sampler);

    device_context->submit_pipeline_state_resources();

    device_context->set_index_buffer(nullptr, gfx_format::invalid);
    device_context->set_vertex_buffers(0, nullptr, nullptr, nullptr);

    device_context->draw(3, 0, 1, 0, 0, 0); // Triangle gets created in vertex shader.

    device_context->set_render_targets(1, &m_gtao_texture0, nullptr);

    m_spatial_denoise_pipeline->get_resource_mapping()->set("texture_gtao", m_gtao_texture1);
    m_spatial_denoise_pipeline->get_resource_mapping()->set("sampler_gtao", m_nearest_sampler);
    m_spatial_denoise_pipeline->get_resource_mapping()->set("texture_depth", m_depth_texture);
    m_spatial_denoise_pipeline->get_resource_mapping()->set("sampler_depth", m_nearest_sampler);

    device_context->submit_pipeline_state_resources();

    device_context->set_index_buffer(nullptr, gfx_format::invalid);
    device_context->set_vertex_buffers(0, nullptr, nullptr, nullptr);

    device_context->draw(3, 0, 1, 0, 0, 0); // Triangle gets created in vertex shader.

    GL_NAMED_PROFILE_ZONE("GTAO Upsample Pass");
    NAMED_PROFILE_ZONE("GTAO Upsample Pass");
    device_context->bind_pipeline(m_upsample_pipeline);

    device_context->set_viewport(0, 1, &m_viewport);

    device_context->set_render_targets(1, &m_orm_texture, nullptr);

    m_upsample_pipeline->get_resource_mapping()->set("texture_gtao_linear", m_gtao_texture0);
    m_upsample_pipeline->get_resource_mapping()->set("sampler_gtao_linear", m_linear_sampler);
    m_upsample_pipeline->get_resource_mapping()->set("texture_gtao_nearest", m_gtao_texture0);
    m_upsample_pipeline->get_resource_mapping()->set("sampler_gtao_nearest", m_nearest_sampler);
    m_upsample_pipeline->get_resource_mapping()->set("texture_depth", m_hierarchical_depth_texture);
    m_upsample_pipeline->get_resource_mapping()->set("sampler_depth", m_nearest_sampler);

    device_context->submit_pipeline_state_resources();

    device_context->set_index_buffer(nullptr, gfx_format::invalid);
    device_context->set_vertex_buffers(0, nullptr, nullptr, nullptr);

    device_context->draw(3, 0, 1, 0, 0, 0); // Triangle gets created in vertex shader.
}

bool gtao_pass::create_ao_textures()
{
    auto& graphics_device = m_shared_context->get_graphics_device();

    texture_create_info ao_texture_info;
    ao_texture_info.texture_type   = gfx_texture_type::texture_type_2d;
    ao_texture_info.width          = (int)m_viewport.width >> 1;
    ao_texture_info.height         = (int)m_viewport.height >> 1;
    ao_texture_info.miplevels      = 1;
    ao_texture_info.array_layers   = 1;
    ao_texture_info.texture_format = gfx_format::r32f;

    m_gtao_texture0 = graphics_device->create_texture(ao_texture_info);
    if (!check_creation(m_gtao_texture0.get(), "gtao texture 0"))
        return false;

    m_gtao_texture1 = graphics_device->create_texture(ao_texture_info);
    if (!check_creation(m_gtao_texture1.get(), "gtao texture 1"))
        return false;

    return true;
}

bool gtao_pass::create_pass_resources()
{
    PROFILE_ZONE;
    auto& graphics_device = m_shared_context->get_graphics_device();

    // buffer
    buffer_create_info buffer_info;
    buffer_info.buffer_target = gfx_buffer_target::buffer_target_uniform;
    buffer_info.buffer_access = gfx_buffer_access::buffer_access_dynamic_storage;
    buffer_info.size          = sizeof(gtao_data);

    m_gtao_data_buffer = graphics_device->create_buffer(buffer_info);
    if (!check_creation(m_gtao_data_buffer.get(), "gtao data buffer"))
        return false;

    // texture
    if (!create_ao_textures())
        return false;

    auto& internal_resources = m_shared_context->get_internal_resources();

    shader_stage_create_info shader_info;
    shader_resource_resource_description res_resource_desc;
    shader_source_description source_desc;

    // Screen Space Quad
    {
        res_resource_desc.path        = "res/shader/v_screen_space_triangle.glsl";
        const shader_resource* source = internal_resources->acquire(res_resource_desc);

        source_desc.entry_point = "main";
        source_desc.source      = source->source.c_str();
        source_desc.size        = static_cast<int32>(source->source.size());

        shader_info.stage         = gfx_shader_stage_type::shader_stage_vertex;
        shader_info.shader_source = source_desc;

        shader_info.resource_count = 0;

        m_screen_space_triangle_vertex = graphics_device->create_shader_stage(shader_info);
        if (!check_creation(m_screen_space_triangle_vertex.get(), "screen space triangle vertex shader"))
            return false;

        res_resource_desc.defines.clear();
    }
    // GTAO Pass Fragment Stage
    {
        res_resource_desc.path        = "res/shader/post/f_gtao.glsl";
        const shader_resource* source = internal_resources->acquire(res_resource_desc);

        source_desc.entry_point = "main";
        source_desc.source      = source->source.c_str();
        source_desc.size        = static_cast<int32>(source->source.size());

        shader_info.stage         = gfx_shader_stage_type::shader_stage_fragment;
        shader_info.shader_source = source_desc;

        shader_info.resource_count = 6;

        shader_info.resources = { {
            { gfx_shader_stage_type::shader_stage_fragment, CAMERA_DATA_BUFFER_BINDING_POINT, "camera_data", gfx_shader_resource_type::shader_resource_constant_buffer, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, GTAO_DATA_BUFFER_BINDING_POINT, "gtao_data", gfx_shader_resource_type::shader_resource_constant_buffer, 1 },

            { gfx_shader_stage_type::shader_stage_fragment, 0, "texture_hierarchical_depth", gfx_shader_resource_type::shader_resource_input_attachment, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, 0, "sampler_hierarchical_depth", gfx_shader_resource_type::shader_resource_sampler, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, 1, "texture_normal", gfx_shader_resource_type::shader_resource_input_attachment, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, 1, "sampler_normal", gfx_shader_resource_type::shader_resource_sampler, 1 },
        } };

        m_gtao_fragment = graphics_device->create_shader_stage(shader_info);
        if (!check_creation(m_gtao_fragment.get(), "gtao pass fragment shader"))
            return false;

        res_resource_desc.defines.clear();
    }

    // GTAO Spatial Denoiser
    {
        res_resource_desc.path        = "res/shader/post/f_spatial_denoiser.glsl";
        const shader_resource* source = internal_resources->acquire(res_resource_desc);

        source_desc.entry_point = "main";
        source_desc.source      = source->source.c_str();
        source_desc.size        = static_cast<int32>(source->source.size());

        shader_info.stage         = gfx_shader_stage_type::shader_stage_fragment;
        shader_info.shader_source = source_desc;

        shader_info.resource_count = 4;

        shader_info.resources = { {
            { gfx_shader_stage_type::shader_stage_fragment, 0, "texture_gtao", gfx_shader_resource_type::shader_resource_input_attachment, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, 0, "sampler_gtao", gfx_shader_resource_type::shader_resource_sampler, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, 1, "texture_depth", gfx_shader_resource_type::shader_resource_input_attachment, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, 1, "sampler_depth", gfx_shader_resource_type::shader_resource_sampler, 1 },
        } };

        m_spatial_denoise_fragment = graphics_device->create_shader_stage(shader_info);
        if (!check_creation(m_spatial_denoise_fragment.get(), "gtao spatial denoiser fragment shader"))
            return false;

        res_resource_desc.defines.clear();
    }

    // GTAO Upsample
    {
        res_resource_desc.path        = "res/shader/post/f_nearest_depth_upsample.glsl";
        const shader_resource* source = internal_resources->acquire(res_resource_desc);

        source_desc.entry_point = "main";
        source_desc.source      = source->source.c_str();
        source_desc.size        = static_cast<int32>(source->source.size());

        shader_info.stage         = gfx_shader_stage_type::shader_stage_fragment;
        shader_info.shader_source = source_desc;

        shader_info.resource_count = 6;

        shader_info.resources = { {
            { gfx_shader_stage_type::shader_stage_fragment, 0, "texture_gtao_linear", gfx_shader_resource_type::shader_resource_input_attachment, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, 0, "sampler_gtao_linear", gfx_shader_resource_type::shader_resource_sampler, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, 1, "texture_gtao_nearest", gfx_shader_resource_type::shader_resource_input_attachment, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, 1, "sampler_gtao_nearest", gfx_shader_resource_type::shader_resource_sampler, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, 2, "texture_depth", gfx_shader_resource_type::shader_resource_input_attachment, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, 2, "sampler_depth", gfx_shader_resource_type::shader_resource_sampler, 1 },
        } };

        m_upsample_fragment = graphics_device->create_shader_stage(shader_info);
        if (!check_creation(m_upsample_fragment.get(), "gtao upsample fragment shader"))
            return false;

        res_resource_desc.defines.clear();
    }

    // gtao pipeline
    {
        graphics_pipeline_create_info gtao_pass_info = graphics_device->provide_graphics_pipeline_create_info();
        auto gtao_pass_pipeline_layout               = graphics_device->create_pipeline_resource_layout({
                          { gfx_shader_stage_type::shader_stage_fragment, CAMERA_DATA_BUFFER_BINDING_POINT, gfx_shader_resource_type::shader_resource_constant_buffer,
                            gfx_shader_resource_access::shader_access_dynamic },
                          { gfx_shader_stage_type::shader_stage_fragment, GTAO_DATA_BUFFER_BINDING_POINT, gfx_shader_resource_type::shader_resource_constant_buffer,
                            gfx_shader_resource_access::shader_access_dynamic },

                          { gfx_shader_stage_type::shader_stage_fragment, 0, gfx_shader_resource_type::shader_resource_input_attachment, gfx_shader_resource_access::shader_access_dynamic },
                          { gfx_shader_stage_type::shader_stage_fragment, 0, gfx_shader_resource_type::shader_resource_sampler, gfx_shader_resource_access::shader_access_dynamic },
                          { gfx_shader_stage_type::shader_stage_fragment, 1, gfx_shader_resource_type::shader_resource_input_attachment, gfx_shader_resource_access::shader_access_dynamic },
                          { gfx_shader_stage_type::shader_stage_fragment, 1, gfx_shader_resource_type::shader_resource_sampler, gfx_shader_resource_access::shader_access_dynamic },
        });

        gtao_pass_info.pipeline_layout = gtao_pass_pipeline_layout;

        gtao_pass_info.shader_stage_descriptor.vertex_shader_stage   = m_screen_space_triangle_vertex;
        gtao_pass_info.shader_stage_descriptor.fragment_shader_stage = m_gtao_fragment;

        gtao_pass_info.vertex_input_state.attribute_description_count = 0;
        gtao_pass_info.vertex_input_state.binding_description_count   = 0;

        gtao_pass_info.input_assembly_state.topology = gfx_primitive_topology::primitive_topology_triangle_list; // Not relevant.

        // viewport_descriptor is dynamic

        // rasterization_state -> keep default
        gtao_pass_info.depth_stencil_state.enable_depth_test = false;

        gtao_pass_info.dynamic_state.dynamic_states = gfx_dynamic_state_flag_bits::dynamic_state_viewport | gfx_dynamic_state_flag_bits::dynamic_state_scissor;

        m_gtao_pass_pipeline = graphics_device->create_graphics_pipeline(gtao_pass_info);
    }

    // spatial denoise pipeline
    {
        graphics_pipeline_create_info spatial_denoise_info = graphics_device->provide_graphics_pipeline_create_info();
        auto spatial_denoise_pipeline_layout               = graphics_device->create_pipeline_resource_layout({
                          { gfx_shader_stage_type::shader_stage_fragment, 0, gfx_shader_resource_type::shader_resource_input_attachment, gfx_shader_resource_access::shader_access_dynamic },
                          { gfx_shader_stage_type::shader_stage_fragment, 0, gfx_shader_resource_type::shader_resource_sampler, gfx_shader_resource_access::shader_access_dynamic },
                          { gfx_shader_stage_type::shader_stage_fragment, 1, gfx_shader_resource_type::shader_resource_input_attachment, gfx_shader_resource_access::shader_access_dynamic },
                          { gfx_shader_stage_type::shader_stage_fragment, 1, gfx_shader_resource_type::shader_resource_sampler, gfx_shader_resource_access::shader_access_dynamic },
        });

        spatial_denoise_info.pipeline_layout = spatial_denoise_pipeline_layout;

        spatial_denoise_info.shader_stage_descriptor.vertex_shader_stage   = m_screen_space_triangle_vertex;
        spatial_denoise_info.shader_stage_descriptor.fragment_shader_stage = m_spatial_denoise_fragment;

        spatial_denoise_info.vertex_input_state.attribute_description_count = 0;
        spatial_denoise_info.vertex_input_state.binding_description_count   = 0;

        spatial_denoise_info.input_assembly_state.topology = gfx_primitive_topology::primitive_topology_triangle_list; // Not relevant.

        // viewport_descriptor is dynamic

        // rasterization_state -> keep default
        spatial_denoise_info.depth_stencil_state.enable_depth_test = false;

        spatial_denoise_info.dynamic_state.dynamic_states = gfx_dynamic_state_flag_bits::dynamic_state_viewport | gfx_dynamic_state_flag_bits::dynamic_state_scissor;

        m_spatial_denoise_pipeline = graphics_device->create_graphics_pipeline(spatial_denoise_info);
    }

    // spatial upsample pipeline
    {
        graphics_pipeline_create_info upsample_info = graphics_device->provide_graphics_pipeline_create_info();
        auto upsample_pipeline_layout               = graphics_device->create_pipeline_resource_layout({
                          { gfx_shader_stage_type::shader_stage_fragment, 0, gfx_shader_resource_type::shader_resource_input_attachment, gfx_shader_resource_access::shader_access_dynamic },
                          { gfx_shader_stage_type::shader_stage_fragment, 0, gfx_shader_resource_type::shader_resource_sampler, gfx_shader_resource_access::shader_access_dynamic },
                          { gfx_shader_stage_type::shader_stage_fragment, 1, gfx_shader_resource_type::shader_resource_input_attachment, gfx_shader_resource_access::shader_access_dynamic },
                          { gfx_shader_stage_type::shader_stage_fragment, 1, gfx_shader_resource_type::shader_resource_sampler, gfx_shader_resource_access::shader_access_dynamic },
                          { gfx_shader_stage_type::shader_stage_fragment, 2, gfx_shader_resource_type::shader_resource_input_attachment, gfx_shader_resource_access::shader_access_dynamic },
                          { gfx_shader_stage_type::shader_stage_fragment, 2, gfx_shader_resource_type::shader_resource_sampler, gfx_shader_resource_access::shader_access_dynamic },
        });

        upsample_info.pipeline_layout = upsample_pipeline_layout;

        upsample_info.shader_stage_descriptor.vertex_shader_stage   = m_screen_space_triangle_vertex;
        upsample_info.shader_stage_descriptor.fragment_shader_stage = m_upsample_fragment;

        upsample_info.vertex_input_state.attribute_description_count = 0;
        upsample_info.vertex_input_state.binding_description_count   = 0;

        upsample_info.input_assembly_state.topology = gfx_primitive_topology::primitive_topology_triangle_list; // Not relevant.

        // viewport_descriptor is dynamic

        // rasterization_state -> keep default
        upsample_info.depth_stencil_state.enable_depth_test                = false;
        upsample_info.blend_state.blend_description.color_write_mask       = gfx_color_component_flag_bits::component_r;
        upsample_info.blend_state.blend_description.enable_blend           = true;
        upsample_info.blend_state.blend_description.src_color_blend_factor = gfx_blend_factor::blend_factor_one;
        upsample_info.blend_state.blend_description.dst_color_blend_factor = gfx_blend_factor::blend_factor_one;
        upsample_info.blend_state.blend_description.color_blend_operation  = gfx_blend_operation::blend_operation_take_min;

        upsample_info.dynamic_state.dynamic_states = gfx_dynamic_state_flag_bits::dynamic_state_viewport | gfx_dynamic_state_flag_bits::dynamic_state_scissor;

        m_upsample_pipeline = graphics_device->create_graphics_pipeline(upsample_info);
    }

    return true;
}

void gtao_pass::on_ui_widget()
{
    ImGui::PushID("gtao_pass");

    float default_value[1] = { 3.5f };

    slider_float_n("Radius", &m_gtao_data.ao_radius, 1, default_value, 0.5f, 10.0f);
    default_value[0] = 12.5f;
    slider_float_n("Power", &m_gtao_data.power, 1, default_value, 0.5f, 25.5f);

    int32 default_value_i[1] = { 3 };

    slider_int_n("Slices", &m_gtao_data.slices, 1, default_value_i, 2, 16);
    slider_int_n("Samples per Direction", &m_gtao_data.direction_samples, 1, default_value_i, 2, 8);

    default_value[0] = 0.8f;
    slider_float_n("Thin Occluder Compensation", &m_gtao_data.thin_occluder_compensation, 1, default_value, 0.0f, 1.0f);

    bool& mb = m_gtao_data.multi_bounce;
    checkbox("Multi Bounce (bw)", &mb, true);

    ImGui::PopID();
}