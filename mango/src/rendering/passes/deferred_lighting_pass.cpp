//! \file      deferred_lighting_pass.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2022
//! \copyright Apache License 2.0

#include <mango/profile.hpp>
#include <rendering/passes/deferred_lighting_pass.hpp>
#include <rendering/renderer_bindings.hpp>
#include <resources/resources_impl.hpp>

using namespace mango;

const render_pass_execution_info deferred_lighting_pass::s_rpei{ 1, 3 };

void deferred_lighting_pass::attach(const shared_ptr<context_impl>& context)
{
    m_shared_context = context;

    create_pass_resources();
}

void deferred_lighting_pass::execute(graphics_device_context_handle& device_context)
{
    GL_NAMED_PROFILE_ZONE("Deferred Lighting Pass");
    NAMED_PROFILE_ZONE("Deferred Lighting Pass");

    device_context->bind_pipeline(m_lighting_pass_pipeline);

    device_context->set_viewport(0, 1, &m_viewport);

    device_context->set_render_targets(static_cast<int32>(m_render_targets.size()) - 1, m_render_targets.data(), m_render_targets.back());

    if (m_camera_data_buffer)
        m_lighting_pass_pipeline->get_resource_mapping()->set("camera_data", m_camera_data_buffer);
    if (m_renderer_data_buffer)
        m_lighting_pass_pipeline->get_resource_mapping()->set("renderer_data", m_renderer_data_buffer);
    if (m_light_data_buffer)
        m_lighting_pass_pipeline->get_resource_mapping()->set("light_data", m_light_data_buffer);
    if (m_shadow_data_buffer)
        m_lighting_pass_pipeline->get_resource_mapping()->set("shadow_data", m_shadow_data_buffer);

    m_lighting_pass_pipeline->get_resource_mapping()->set("texture_gbuffer_c0", m_gbuffer[0]);
    m_lighting_pass_pipeline->get_resource_mapping()->set("sampler_gbuffer_c0", m_gbuffer_sampler);
    m_lighting_pass_pipeline->get_resource_mapping()->set("texture_gbuffer_c1", m_gbuffer[1]);
    m_lighting_pass_pipeline->get_resource_mapping()->set("sampler_gbuffer_c1", m_gbuffer_sampler);
    m_lighting_pass_pipeline->get_resource_mapping()->set("texture_gbuffer_c2", m_gbuffer[2]);
    m_lighting_pass_pipeline->get_resource_mapping()->set("sampler_gbuffer_c2", m_gbuffer_sampler);
    m_lighting_pass_pipeline->get_resource_mapping()->set("texture_gbuffer_c3", m_gbuffer[3]);
    m_lighting_pass_pipeline->get_resource_mapping()->set("sampler_gbuffer_c3", m_gbuffer_sampler);
    m_lighting_pass_pipeline->get_resource_mapping()->set("texture_gbuffer_depth", m_gbuffer[4]);
    m_lighting_pass_pipeline->get_resource_mapping()->set("sampler_gbuffer_depth", m_gbuffer_sampler);

    m_lighting_pass_pipeline->get_resource_mapping()->set("texture_irradiance_map", m_irradiance_map);
    m_lighting_pass_pipeline->get_resource_mapping()->set("sampler_irradiance_map", m_irradiance_map_sampler);
    m_lighting_pass_pipeline->get_resource_mapping()->set("texture_radiance_map", m_radiance_map);
    m_lighting_pass_pipeline->get_resource_mapping()->set("sampler_radiance_map", m_radiance_map_sampler);
    m_lighting_pass_pipeline->get_resource_mapping()->set("texture_brdf_integration_lut", m_brdf_integration_lut);
    m_lighting_pass_pipeline->get_resource_mapping()->set("sampler_brdf_integration_lut", m_brdf_integration_lut_sampler);

    m_lighting_pass_pipeline->get_resource_mapping()->set("texture_shadow_map_comp", m_shadow_map);
    m_lighting_pass_pipeline->get_resource_mapping()->set("texture_shadow_map", m_shadow_map);
    m_lighting_pass_pipeline->get_resource_mapping()->set("sampler_shadow_shadow_map", m_shadow_map_compare_sampler);
    m_lighting_pass_pipeline->get_resource_mapping()->set("sampler_shadow_map", m_shadow_map_sampler);

    device_context->submit_pipeline_state_resources();

    device_context->set_index_buffer(nullptr, gfx_format::invalid);
    device_context->set_vertex_buffers(0, nullptr, nullptr, nullptr);

    device_context->draw(3, 0, 1, 0, 0, 0); // Triangle gets created in vertex shader.
}

bool deferred_lighting_pass::create_pass_resources()
{
    PROFILE_ZONE;
    auto& graphics_device    = m_shared_context->get_graphics_device();
    auto& internal_resources = m_shared_context->get_internal_resources();

    shader_stage_create_info shader_info;
    shader_resource_resource_description res_resource_desc;
    shader_source_description source_desc;

    // Screen Space Quad Vertex Shader Stage
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
    // Deferred Lighting Fragment Shader Stage
    {
        res_resource_desc.path        = "res/shader/deferred/f_deferred_lighting.glsl";
        const shader_resource* source = internal_resources->acquire(res_resource_desc);

        source_desc.entry_point = "main";
        source_desc.source      = source->source.c_str();
        source_desc.size        = static_cast<int32>(source->source.size());

        shader_info.stage         = gfx_shader_stage_type::shader_stage_fragment;
        shader_info.shader_source = source_desc;

        shader_info.resource_count = 24;

        shader_info.resources = { {
            { gfx_shader_stage_type::shader_stage_fragment, CAMERA_DATA_BUFFER_BINDING_POINT, "camera_data", gfx_shader_resource_type::shader_resource_constant_buffer, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, RENDERER_DATA_BUFFER_BINDING_POINT, "renderer_data", gfx_shader_resource_type::shader_resource_constant_buffer, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, LIGHT_DATA_BUFFER_BINDING_POINT, "light_data", gfx_shader_resource_type::shader_resource_constant_buffer, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, SHADOW_DATA_BUFFER_BINDING_POINT, "shadow_data", gfx_shader_resource_type::shader_resource_constant_buffer, 1 },

            { gfx_shader_stage_type::shader_stage_fragment, GBUFFER_TEXTURE_SAMPLER_TARGET0, "texture_gbuffer_c0", gfx_shader_resource_type::shader_resource_input_attachment, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, GBUFFER_TEXTURE_SAMPLER_TARGET0, "sampler_gbuffer_c0", gfx_shader_resource_type::shader_resource_sampler, 1 },

            { gfx_shader_stage_type::shader_stage_fragment, GBUFFER_TEXTURE_SAMPLER_TARGET1, "texture_gbuffer_c1", gfx_shader_resource_type::shader_resource_input_attachment, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, GBUFFER_TEXTURE_SAMPLER_TARGET1, "sampler_gbuffer_c1", gfx_shader_resource_type::shader_resource_sampler, 1 },

            { gfx_shader_stage_type::shader_stage_fragment, GBUFFER_TEXTURE_SAMPLER_TARGET2, "texture_gbuffer_c2", gfx_shader_resource_type::shader_resource_input_attachment, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, GBUFFER_TEXTURE_SAMPLER_TARGET2, "sampler_gbuffer_c2", gfx_shader_resource_type::shader_resource_sampler, 1 },

            { gfx_shader_stage_type::shader_stage_fragment, GBUFFER_TEXTURE_SAMPLER_TARGET3, "texture_gbuffer_c3", gfx_shader_resource_type::shader_resource_input_attachment, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, GBUFFER_TEXTURE_SAMPLER_TARGET3, "sampler_gbuffer_c3", gfx_shader_resource_type::shader_resource_sampler, 1 },

            { gfx_shader_stage_type::shader_stage_fragment, GBUFFER_TEXTURE_SAMPLER_DEPTH, "texture_gbuffer_depth", gfx_shader_resource_type::shader_resource_input_attachment, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, GBUFFER_TEXTURE_SAMPLER_DEPTH, "sampler_gbuffer_depth", gfx_shader_resource_type::shader_resource_sampler, 1 },

            { gfx_shader_stage_type::shader_stage_fragment, IBL_SAMPLER_IRRADIANCE_MAP, "texture_irradiance_map", gfx_shader_resource_type::shader_resource_input_attachment, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, IBL_SAMPLER_IRRADIANCE_MAP, "sampler_irradiance_map", gfx_shader_resource_type::shader_resource_sampler, 1 },

            { gfx_shader_stage_type::shader_stage_fragment, IBL_SAMPLER_RADIANCE_MAP, "texture_radiance_map", gfx_shader_resource_type::shader_resource_input_attachment, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, IBL_SAMPLER_RADIANCE_MAP, "sampler_radiance_map", gfx_shader_resource_type::shader_resource_sampler, 1 },

            { gfx_shader_stage_type::shader_stage_fragment, IBL_SAMPLER_LOOKUP, "texture_brdf_integration_lut", gfx_shader_resource_type::shader_resource_input_attachment, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, IBL_SAMPLER_LOOKUP, "sampler_brdf_integration_lut", gfx_shader_resource_type::shader_resource_sampler, 1 },

            { gfx_shader_stage_type::shader_stage_fragment, SAMPLER_SHADOW_SHADOW_MAP, "texture_shadow_map_comp", gfx_shader_resource_type::shader_resource_input_attachment, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, SAMPLER_SHADOW_SHADOW_MAP, "sampler_shadow_shadow_map", gfx_shader_resource_type::shader_resource_sampler, 1 },

            { gfx_shader_stage_type::shader_stage_fragment, SAMPLER_SHADOW_MAP, "texture_shadow_map", gfx_shader_resource_type::shader_resource_input_attachment, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, SAMPLER_SHADOW_MAP, "sampler_shadow_map", gfx_shader_resource_type::shader_resource_sampler, 1 },
        } };

        m_lighting_pass_fragment = graphics_device->create_shader_stage(shader_info);
        if (!check_creation(m_lighting_pass_fragment.get(), "lighting pass fragment shader"))
            return false;

        res_resource_desc.defines.clear();
    }

    graphics_pipeline_create_info lighting_pass_info = graphics_device->provide_graphics_pipeline_create_info();
    auto lighting_pass_pipeline_layout               = graphics_device->create_pipeline_resource_layout({
                      { gfx_shader_stage_type::shader_stage_fragment, CAMERA_DATA_BUFFER_BINDING_POINT, gfx_shader_resource_type::shader_resource_constant_buffer,
                        gfx_shader_resource_access::shader_access_dynamic },
                      { gfx_shader_stage_type::shader_stage_fragment, RENDERER_DATA_BUFFER_BINDING_POINT, gfx_shader_resource_type::shader_resource_constant_buffer,
                        gfx_shader_resource_access::shader_access_dynamic },
                      { gfx_shader_stage_type::shader_stage_fragment, LIGHT_DATA_BUFFER_BINDING_POINT, gfx_shader_resource_type::shader_resource_constant_buffer, gfx_shader_resource_access::shader_access_dynamic },
                      { gfx_shader_stage_type::shader_stage_fragment, SHADOW_DATA_BUFFER_BINDING_POINT, gfx_shader_resource_type::shader_resource_constant_buffer,
                        gfx_shader_resource_access::shader_access_dynamic },

                      { gfx_shader_stage_type::shader_stage_fragment, GBUFFER_TEXTURE_SAMPLER_TARGET0, gfx_shader_resource_type::shader_resource_input_attachment,
                        gfx_shader_resource_access::shader_access_dynamic },
                      { gfx_shader_stage_type::shader_stage_fragment, GBUFFER_TEXTURE_SAMPLER_TARGET0, gfx_shader_resource_type::shader_resource_sampler, gfx_shader_resource_access::shader_access_dynamic },

                      { gfx_shader_stage_type::shader_stage_fragment, GBUFFER_TEXTURE_SAMPLER_TARGET1, gfx_shader_resource_type::shader_resource_input_attachment,
                        gfx_shader_resource_access::shader_access_dynamic },
                      { gfx_shader_stage_type::shader_stage_fragment, GBUFFER_TEXTURE_SAMPLER_TARGET1, gfx_shader_resource_type::shader_resource_sampler, gfx_shader_resource_access::shader_access_dynamic },

                      { gfx_shader_stage_type::shader_stage_fragment, GBUFFER_TEXTURE_SAMPLER_TARGET2, gfx_shader_resource_type::shader_resource_input_attachment,
                        gfx_shader_resource_access::shader_access_dynamic },
                      { gfx_shader_stage_type::shader_stage_fragment, GBUFFER_TEXTURE_SAMPLER_TARGET2, gfx_shader_resource_type::shader_resource_sampler, gfx_shader_resource_access::shader_access_dynamic },

                      { gfx_shader_stage_type::shader_stage_fragment, GBUFFER_TEXTURE_SAMPLER_TARGET3, gfx_shader_resource_type::shader_resource_input_attachment,
                        gfx_shader_resource_access::shader_access_dynamic },
                      { gfx_shader_stage_type::shader_stage_fragment, GBUFFER_TEXTURE_SAMPLER_TARGET3, gfx_shader_resource_type::shader_resource_sampler, gfx_shader_resource_access::shader_access_dynamic },

                      { gfx_shader_stage_type::shader_stage_fragment, GBUFFER_TEXTURE_SAMPLER_DEPTH, gfx_shader_resource_type::shader_resource_input_attachment, gfx_shader_resource_access::shader_access_dynamic },
                      { gfx_shader_stage_type::shader_stage_fragment, GBUFFER_TEXTURE_SAMPLER_DEPTH, gfx_shader_resource_type::shader_resource_sampler, gfx_shader_resource_access::shader_access_dynamic },

                      { gfx_shader_stage_type::shader_stage_fragment, IBL_SAMPLER_IRRADIANCE_MAP, gfx_shader_resource_type::shader_resource_input_attachment, gfx_shader_resource_access::shader_access_dynamic },
                      { gfx_shader_stage_type::shader_stage_fragment, IBL_SAMPLER_IRRADIANCE_MAP, gfx_shader_resource_type::shader_resource_sampler, gfx_shader_resource_access::shader_access_dynamic },

                      { gfx_shader_stage_type::shader_stage_fragment, IBL_SAMPLER_RADIANCE_MAP, gfx_shader_resource_type::shader_resource_input_attachment, gfx_shader_resource_access::shader_access_dynamic },
                      { gfx_shader_stage_type::shader_stage_fragment, IBL_SAMPLER_RADIANCE_MAP, gfx_shader_resource_type::shader_resource_sampler, gfx_shader_resource_access::shader_access_dynamic },

                      { gfx_shader_stage_type::shader_stage_fragment, IBL_SAMPLER_LOOKUP, gfx_shader_resource_type::shader_resource_input_attachment, gfx_shader_resource_access::shader_access_dynamic },
                      { gfx_shader_stage_type::shader_stage_fragment, IBL_SAMPLER_LOOKUP, gfx_shader_resource_type::shader_resource_sampler, gfx_shader_resource_access::shader_access_dynamic },

                      { gfx_shader_stage_type::shader_stage_fragment, SAMPLER_SHADOW_SHADOW_MAP, gfx_shader_resource_type::shader_resource_input_attachment, gfx_shader_resource_access::shader_access_dynamic },
                      { gfx_shader_stage_type::shader_stage_fragment, SAMPLER_SHADOW_SHADOW_MAP, gfx_shader_resource_type::shader_resource_sampler, gfx_shader_resource_access::shader_access_dynamic },

                      { gfx_shader_stage_type::shader_stage_fragment, SAMPLER_SHADOW_MAP, gfx_shader_resource_type::shader_resource_input_attachment, gfx_shader_resource_access::shader_access_dynamic },
                      { gfx_shader_stage_type::shader_stage_fragment, SAMPLER_SHADOW_MAP, gfx_shader_resource_type::shader_resource_sampler, gfx_shader_resource_access::shader_access_dynamic },
    });

    lighting_pass_info.pipeline_layout = lighting_pass_pipeline_layout;

    lighting_pass_info.shader_stage_descriptor.vertex_shader_stage   = m_screen_space_triangle_vertex;
    lighting_pass_info.shader_stage_descriptor.fragment_shader_stage = m_lighting_pass_fragment;

    lighting_pass_info.vertex_input_state.attribute_description_count = 0;
    lighting_pass_info.vertex_input_state.binding_description_count   = 0;

    lighting_pass_info.input_assembly_state.topology = gfx_primitive_topology::primitive_topology_triangle_list; // Not relevant.

    // viewport_descriptor is dynamic
    // rasterization_state -> keep default
    // depth_stencil_state -> keep default
    // blend_state -> keep default

    lighting_pass_info.dynamic_state.dynamic_states = gfx_dynamic_state_flag_bits::dynamic_state_viewport | gfx_dynamic_state_flag_bits::dynamic_state_scissor;

    m_lighting_pass_pipeline = graphics_device->create_graphics_pipeline(lighting_pass_info);

    return true;
}
