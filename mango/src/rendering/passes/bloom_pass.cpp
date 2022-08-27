//! \file      bloom_pass.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2022
//! \copyright Apache License 2.0

#include <mango/imgui_helper.hpp>
#include <mango/profile.hpp>
#include <rendering/passes/bloom_pass.hpp>
#include <rendering/renderer_bindings.hpp>
#include <resources/resources_impl.hpp>

using namespace mango;

bloom_pass::bloom_pass(const bloom_settings& settings)
    : m_settings(settings)
{
    PROFILE_ZONE;

    // TODO: Do not hardcode.
    m_bloom_data.filter_radius = settings.get_filter_radius();
    m_bloom_data.power         = settings.get_power();
    m_bloom_data.current_mip   = 0; // private setting

    m_viewport  = gfx_viewport{ 0.0f, 0.0f, 2.0f, 2.0f };
    m_mip_count = 0;
}

void bloom_pass::attach(const shared_ptr<context_impl>& context)
{
    m_shared_context = context;

    create_pass_resources();
}

void bloom_pass::execute(graphics_device_context_handle& device_context)
{
    GL_NAMED_PROFILE_ZONE("Bloom Pass");
    NAMED_PROFILE_ZONE("Bloom Pass");

    device_context->bind_pipeline(m_downsample_pipeline);

    int32 w = (int32)m_viewport.width;
    int32 h = (int32)m_viewport.height;
    w >>= 1;
    h >>= 1;
    auto output_vp = m_viewport;

    for (int32 i = 0; i < m_mip_count; ++i)
    {
        GL_NAMED_PROFILE_ZONE("Downsample Pass");
        NAMED_PROFILE_ZONE("Downsample Pass");

        output_vp.width  = w >> i;
        output_vp.height = h >> i;
        device_context->set_viewport(0, 1, &output_vp);
        auto bloom_buffer_view = m_bloom_buffer_levels[i];
        device_context->set_render_targets(1, &bloom_buffer_view, nullptr);

        m_bloom_data.current_mip = max(i - 1, 0);

        device_context->set_buffer_data(m_bloom_data_buffer, 0, sizeof(m_bloom_data), &m_bloom_data);

        m_downsample_pipeline->get_resource_mapping()->set("bloom_data", m_bloom_data_buffer);
        m_downsample_pipeline->get_resource_mapping()->set("texture_input", i == 0 ? m_hdr_texture : m_bloom_buffer);
        m_downsample_pipeline->get_resource_mapping()->set("sampler_input", m_mipmapped_linear_sampler);

        device_context->submit_pipeline_state_resources();

        device_context->set_index_buffer(nullptr, gfx_format::invalid);
        device_context->set_vertex_buffers(0, nullptr, nullptr, nullptr);

        device_context->draw(3, 0, 1, 0, 0, 0); // Triangle gets created in vertex shader.
    }

    w <<= 1;
    h <<= 1;

    device_context->bind_pipeline(m_upsample_and_blur_pipeline);

    for (int32 i = m_mip_count - 1; i > 0; --i)
    {
        GL_NAMED_PROFILE_ZONE("Upsample Blur Pass");
        NAMED_PROFILE_ZONE("Upsample Blur Pass");

        output_vp.width  = w >> i;
        output_vp.height = h >> i;

        device_context->set_viewport(0, 1, &output_vp);
        auto bloom_buffer_view = m_bloom_buffer_levels[i - 1];
        device_context->set_render_targets(1, &bloom_buffer_view, nullptr);

        m_bloom_data.current_mip = i;

        device_context->set_buffer_data(m_bloom_data_buffer, 0, sizeof(m_bloom_data), &m_bloom_data);

        m_upsample_and_blur_pipeline->get_resource_mapping()->set("bloom_data", m_bloom_data_buffer);
        m_upsample_and_blur_pipeline->get_resource_mapping()->set("texture_input", m_bloom_buffer);
        m_upsample_and_blur_pipeline->get_resource_mapping()->set("sampler_input", m_mipmapped_linear_sampler);

        device_context->submit_pipeline_state_resources();

        device_context->set_index_buffer(nullptr, gfx_format::invalid);
        device_context->set_vertex_buffers(0, nullptr, nullptr, nullptr);

        device_context->draw(3, 0, 1, 0, 0, 0); // Triangle gets created in vertex shader.
    }

    device_context->bind_pipeline(m_upsample_and_blur_pipeline);

    {
        GL_NAMED_PROFILE_ZONE("Upsample Mix Pass");
        NAMED_PROFILE_ZONE("Upsample Mix Pass");

        device_context->set_viewport(0, 1, &m_viewport);
        device_context->set_render_targets(1, &m_hdr_texture, nullptr);

        m_bloom_data.current_mip = 0;

        device_context->set_buffer_data(m_bloom_data_buffer, 0, sizeof(m_bloom_data), &m_bloom_data);

        m_upsample_and_blur_pipeline->get_resource_mapping()->set("bloom_data", m_bloom_data_buffer);
        m_upsample_and_blur_pipeline->get_resource_mapping()->set("texture_input", m_bloom_buffer);
        m_upsample_and_blur_pipeline->get_resource_mapping()->set("sampler_input", m_mipmapped_linear_sampler);

        device_context->submit_pipeline_state_resources();

        device_context->set_index_buffer(nullptr, gfx_format::invalid);
        device_context->set_vertex_buffers(0, nullptr, nullptr, nullptr);

        device_context->draw(3, 0, 1, 0, 0, 0); // Triangle gets created in vertex shader.
    }
}

bool bloom_pass::create_bloom_texture()
{
    auto& graphics_device = m_shared_context->get_graphics_device();

    texture_create_info bloom_texture_info;
    bloom_texture_info.texture_type   = gfx_texture_type::texture_type_2d;
    bloom_texture_info.width          = (int)m_viewport.width >> 1;
    bloom_texture_info.height         = (int)m_viewport.height >> 1;
    bloom_texture_info.miplevels      = min(3, graphics::calculate_mip_count(bloom_texture_info.width, bloom_texture_info.height));
    bloom_texture_info.array_layers   = 1;
    bloom_texture_info.texture_format = gfx_format::rgb32f;

    m_mip_count = bloom_texture_info.miplevels;

    m_rpei.draw_calls = m_mip_count * 2 + 2;
    m_rpei.vertices   = m_rpei.draw_calls * 3;

    m_bloom_buffer = graphics_device->create_texture(bloom_texture_info);
    if (!check_creation(m_bloom_buffer.get(), "bloom buffer"))
        return false;

    m_bloom_buffer_levels.clear();
    for (int32 i = 0; i < m_mip_count; ++i)
    {
        m_bloom_buffer_levels.push_back(graphics_device->create_image_texture_view(m_bloom_buffer, i));
    }

    return true;
}

bool bloom_pass::create_pass_resources()
{
    PROFILE_ZONE;
    auto& graphics_device = m_shared_context->get_graphics_device();

    // buffer
    buffer_create_info buffer_info;
    buffer_info.buffer_target = gfx_buffer_target::buffer_target_uniform;
    buffer_info.buffer_access = gfx_buffer_access::buffer_access_dynamic_storage;
    buffer_info.size          = sizeof(bloom_data);

    m_bloom_data_buffer = graphics_device->create_buffer(buffer_info);
    if (!check_creation(m_bloom_data_buffer.get(), "bloom data buffer"))
        return false;

    // texture
    if (!create_bloom_texture())
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
    // Bloom Downsample Fragment Stage
    {
        res_resource_desc.path        = "res/shader/post/f_downsample.glsl";
        const shader_resource* source = internal_resources->acquire(res_resource_desc);

        source_desc.entry_point = "main";
        source_desc.source      = source->source.c_str();
        source_desc.size        = static_cast<int32>(source->source.size());

        shader_info.stage         = gfx_shader_stage_type::shader_stage_fragment;
        shader_info.shader_source = source_desc;

        shader_info.resource_count = 3;

        shader_info.resources = { {
            { gfx_shader_stage_type::shader_stage_fragment, BLOOM_DATA_BUFFER_BINDING_POINT, "bloom_data", gfx_shader_resource_type::shader_resource_constant_buffer, 1 },

            { gfx_shader_stage_type::shader_stage_fragment, 0, "texture_input", gfx_shader_resource_type::shader_resource_input_attachment, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, 0, "sampler_input", gfx_shader_resource_type::shader_resource_sampler, 1 },
        } };

        m_downsample_fragment = graphics_device->create_shader_stage(shader_info);
        if (!check_creation(m_downsample_fragment.get(), "bloom downsample fragment shader"))
            return false;

        res_resource_desc.defines.clear();
    }
    // Bloom Upsample and Blur Fragment Stage
    {
        res_resource_desc.path        = "res/shader/post/f_upsample.glsl";
        const shader_resource* source = internal_resources->acquire(res_resource_desc);
        res_resource_desc.defines.push_back({ "BLUR", "" });

        source_desc.entry_point = "main";
        source_desc.source      = source->source.c_str();
        source_desc.size        = static_cast<int32>(source->source.size());

        shader_info.stage         = gfx_shader_stage_type::shader_stage_fragment;
        shader_info.shader_source = source_desc;

        shader_info.resource_count = 3;

        shader_info.resources = { {
            { gfx_shader_stage_type::shader_stage_fragment, BLOOM_DATA_BUFFER_BINDING_POINT, "bloom_data", gfx_shader_resource_type::shader_resource_constant_buffer, 1 },

            { gfx_shader_stage_type::shader_stage_fragment, 0, "texture_input", gfx_shader_resource_type::shader_resource_input_attachment, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, 0, "sampler_input", gfx_shader_resource_type::shader_resource_sampler, 1 },
        } };

        m_upsample_and_blur_fragment = graphics_device->create_shader_stage(shader_info);
        if (!check_creation(m_upsample_and_blur_fragment.get(), "bloom upsample and blur fragment shader"))
            return false;

        res_resource_desc.defines.clear();
    }

    // Downsample pipeline
    {
        graphics_pipeline_create_info downsample_pass_info = graphics_device->provide_graphics_pipeline_create_info();
        auto downsample_pass_pipeline_layout               = graphics_device->create_pipeline_resource_layout({
                          { gfx_shader_stage_type::shader_stage_fragment, BLOOM_DATA_BUFFER_BINDING_POINT, gfx_shader_resource_type::shader_resource_constant_buffer,
                            gfx_shader_resource_access::shader_access_dynamic },

                          { gfx_shader_stage_type::shader_stage_fragment, 0, gfx_shader_resource_type::shader_resource_input_attachment, gfx_shader_resource_access::shader_access_dynamic },
                          { gfx_shader_stage_type::shader_stage_fragment, 0, gfx_shader_resource_type::shader_resource_sampler, gfx_shader_resource_access::shader_access_dynamic },
        });

        downsample_pass_info.pipeline_layout = downsample_pass_pipeline_layout;

        downsample_pass_info.shader_stage_descriptor.vertex_shader_stage   = m_screen_space_triangle_vertex;
        downsample_pass_info.shader_stage_descriptor.fragment_shader_stage = m_downsample_fragment;

        downsample_pass_info.vertex_input_state.attribute_description_count = 0;
        downsample_pass_info.vertex_input_state.binding_description_count   = 0;

        downsample_pass_info.input_assembly_state.topology = gfx_primitive_topology::primitive_topology_triangle_list; // Not relevant.

        // viewport_descriptor is dynamic

        // rasterization_state -> keep default
        downsample_pass_info.depth_stencil_state.enable_depth_test = false;

        downsample_pass_info.dynamic_state.dynamic_states = gfx_dynamic_state_flag_bits::dynamic_state_viewport | gfx_dynamic_state_flag_bits::dynamic_state_scissor;

        m_downsample_pipeline = graphics_device->create_graphics_pipeline(downsample_pass_info);
    }

    // Upsample and Blur pipeline
    {
        graphics_pipeline_create_info upsample_info = graphics_device->provide_graphics_pipeline_create_info();
        auto upsample_pipeline_layout               = graphics_device->create_pipeline_resource_layout({
                          { gfx_shader_stage_type::shader_stage_fragment, BLOOM_DATA_BUFFER_BINDING_POINT, gfx_shader_resource_type::shader_resource_constant_buffer,
                            gfx_shader_resource_access::shader_access_dynamic },

                          { gfx_shader_stage_type::shader_stage_fragment, 0, gfx_shader_resource_type::shader_resource_input_attachment, gfx_shader_resource_access::shader_access_dynamic },
                          { gfx_shader_stage_type::shader_stage_fragment, 0, gfx_shader_resource_type::shader_resource_sampler, gfx_shader_resource_access::shader_access_dynamic },
        });

        upsample_info.pipeline_layout = upsample_pipeline_layout;

        upsample_info.shader_stage_descriptor.vertex_shader_stage   = m_screen_space_triangle_vertex;
        upsample_info.shader_stage_descriptor.fragment_shader_stage = m_upsample_and_blur_fragment;

        upsample_info.vertex_input_state.attribute_description_count = 0;
        upsample_info.vertex_input_state.binding_description_count   = 0;

        upsample_info.input_assembly_state.topology = gfx_primitive_topology::primitive_topology_triangle_list; // Not relevant.

        // viewport_descriptor is dynamic

        // rasterization_state -> keep default
        upsample_info.depth_stencil_state.enable_depth_test               = false;
        upsample_info.blend_state.blend_description.enable_blend          = true;
        upsample_info.blend_state.blend_description.color_blend_operation = gfx_blend_operation::blend_operation_add;
        upsample_info.blend_state.blend_description.alpha_blend_operation = gfx_blend_operation::blend_operation_add;

        upsample_info.blend_state.blend_description.src_color_blend_factor = gfx_blend_factor::blend_factor_one;
        upsample_info.blend_state.blend_description.dst_color_blend_factor = gfx_blend_factor::blend_factor_one;
        upsample_info.blend_state.blend_description.src_alpha_blend_factor = gfx_blend_factor::blend_factor_one;
        upsample_info.blend_state.blend_description.dst_alpha_blend_factor = gfx_blend_factor::blend_factor_one;

        upsample_info.dynamic_state.dynamic_states = gfx_dynamic_state_flag_bits::dynamic_state_viewport | gfx_dynamic_state_flag_bits::dynamic_state_scissor;

        m_upsample_and_blur_pipeline = graphics_device->create_graphics_pipeline(upsample_info);
    }

    return true;
}

void bloom_pass::on_ui_widget()
{
    ImGui::PushID("bloom_pass");

    int32 default_valuei[1] = { 2 };
    slider_int_n("Radius", &m_bloom_data.filter_radius, 1, default_valuei, 1, 6);
    float default_value[1] = { 0.5f };
    slider_float_n("Power", &m_bloom_data.power, 1, default_value, 0.25f, 1.25f);

    ImGui::PopID();
}