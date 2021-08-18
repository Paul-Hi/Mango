//! \file      fxaa_step.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#include <mango/imgui_helper.hpp>
#include <mango/profile.hpp>
#include <rendering/renderer_impl.hpp>
#include <rendering/steps/fxaa_step.hpp>
#include <resources/resources_impl.hpp>
#include <util/helpers.hpp>

using namespace mango;

fxaa_step::fxaa_step(const fxaa_settings& settings)
    : m_settings(settings)
{
    m_fxaa_data.subpixel_filter = m_settings.get_subpixel_filter();
    m_fxaa_data.quality_preset  = static_cast<uint8>(m_settings.get_quality_preset());
}

fxaa_step::~fxaa_step() {}

bool fxaa_step::create_step_resources()
{
    PROFILE_ZONE;
    auto& graphics_device = m_shared_context->get_graphics_device();
    // buffers
    buffer_create_info buffer_info;
    buffer_info.buffer_target = gfx_buffer_target::buffer_target_uniform;
    buffer_info.buffer_access = gfx_buffer_access::buffer_access_dynamic_storage;
    buffer_info.size          = sizeof(fxaa_data);

    m_fxaa_data_buffer = graphics_device->create_buffer(buffer_info);
    if (!check_creation(m_fxaa_data_buffer.get(), "fxaa data buffer"))
        return false;

    // sampler
    sampler_create_info sampler_info;
    sampler_info.sampler_min_filter      = gfx_sampler_filter::sampler_filter_linear;
    sampler_info.sampler_max_filter      = gfx_sampler_filter::sampler_filter_linear;
    sampler_info.enable_comparison_mode  = false;
    sampler_info.comparison_operator     = gfx_compare_operator::compare_operator_always;
    sampler_info.edge_value_wrap_u       = gfx_sampler_edge_wrap::sampler_edge_wrap_clamp_to_edge;
    sampler_info.edge_value_wrap_v       = gfx_sampler_edge_wrap::sampler_edge_wrap_clamp_to_edge;
    sampler_info.edge_value_wrap_w       = gfx_sampler_edge_wrap::sampler_edge_wrap_clamp_to_edge;
    sampler_info.border_color[0]         = 0;
    sampler_info.border_color[1]         = 0;
    sampler_info.border_color[2]         = 0;
    sampler_info.border_color[3]         = 0;
    sampler_info.enable_seamless_cubemap = false;

    m_sampler_input = graphics_device->create_sampler(sampler_info);

    // shader stages
    auto& internal_resources = m_shared_context->get_internal_resources();
    shader_stage_create_info shader_info;
    shader_resource_resource_description res_resource_desc;
    shader_source_description source_desc;

    // vertex stage
    {
        res_resource_desc.path        = "res/shader/v_screen_space_triangle.glsl";
        const shader_resource* source = internal_resources->acquire(res_resource_desc);

        source_desc.entry_point = "main";
        source_desc.source      = source->source.c_str();
        source_desc.size        = static_cast<int32>(source->source.size());

        shader_info.stage         = gfx_shader_stage_type::shader_stage_vertex;
        shader_info.shader_source = source_desc;

        shader_info.resource_count = 0;

        m_fxaa_pass_vertex = graphics_device->create_shader_stage(shader_info);
        if (!check_creation(m_fxaa_pass_vertex.get(), "fxaa pass vertex shader"))
            return false;

        res_resource_desc.defines.clear();
    }
    // fragment stage
    {
        res_resource_desc.path        = "res/shader/post/f_fxaa.glsl";
        const shader_resource* source = internal_resources->acquire(res_resource_desc);

        source_desc.entry_point = "main";
        source_desc.source      = source->source.c_str();
        source_desc.size        = static_cast<int32>(source->source.size());

        shader_info.stage         = gfx_shader_stage_type::shader_stage_fragment;
        shader_info.shader_source = source_desc;

        shader_info.resource_count = 3;

        shader_info.resources = { {
            { gfx_shader_stage_type::shader_stage_fragment, 0, "texture_input", gfx_shader_resource_type::shader_resource_input_attachment, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, 0, "sampler_input", gfx_shader_resource_type::shader_resource_sampler, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, 1, "fxaa_data", gfx_shader_resource_type::shader_resource_buffer_storage, 1 },
        } };

        m_fxaa_pass_fragment = graphics_device->create_shader_stage(shader_info);
        if (!check_creation(m_fxaa_pass_fragment.get(), "fxaa pass fragment shader"))
            return false;

        res_resource_desc.defines.clear();
    }
    // Pass Pipeline
    {
        graphics_pipeline_create_info fxaa_pass_info = graphics_device->provide_graphics_pipeline_create_info();
        auto fxaa_pass_pipeline_layout               = graphics_device->create_pipeline_resource_layout({
            { gfx_shader_stage_type::shader_stage_fragment, 0, gfx_shader_resource_type::shader_resource_input_attachment, gfx_shader_resource_access::shader_access_dynamic },
            { gfx_shader_stage_type::shader_stage_fragment, 0, gfx_shader_resource_type::shader_resource_sampler, gfx_shader_resource_access::shader_access_dynamic },

            { gfx_shader_stage_type::shader_stage_fragment, 1, gfx_shader_resource_type::shader_resource_buffer_storage, gfx_shader_resource_access::shader_access_dynamic },
        });

        fxaa_pass_info.pipeline_layout = fxaa_pass_pipeline_layout;

        fxaa_pass_info.shader_stage_descriptor.vertex_shader_stage   = m_fxaa_pass_vertex;
        fxaa_pass_info.shader_stage_descriptor.fragment_shader_stage = m_fxaa_pass_fragment;

        fxaa_pass_info.vertex_input_state.attribute_description_count = 0;
        fxaa_pass_info.vertex_input_state.binding_description_count   = 0;

        fxaa_pass_info.input_assembly_state.topology = gfx_primitive_topology::primitive_topology_triangle_list; // Not relevant.

        // viewport_descriptor is dynamic

        // rasterization_state -> keep default
        fxaa_pass_info.depth_stencil_state.enable_depth_test = false;
        // blend_state -> keep default

        fxaa_pass_info.dynamic_state.dynamic_states = gfx_dynamic_state_flag_bits::dynamic_state_viewport | gfx_dynamic_state_flag_bits::dynamic_state_scissor;

        m_fxaa_pass_pipeline = graphics_device->create_graphics_pipeline(fxaa_pass_info);
    }

    return true;
}

void fxaa_step::attach(const shared_ptr<context_impl>& context)
{
    m_shared_context = context;

    create_step_resources();
}

void fxaa_step::execute()
{
    PROFILE_ZONE;
    if (!m_texture_input || !m_output_target || !m_output_target_depth_stencil)
        return;

    auto& graphics_device = m_shared_context->get_graphics_device();

    auto step_context = graphics_device->create_graphics_device_context();

    step_context->begin();

    step_context->bind_pipeline(m_fxaa_pass_pipeline);

    step_context->set_render_targets(1, &m_output_target, m_output_target_depth_stencil);

    m_fxaa_data.inverse_screen_size = 1.0f / m_output_target->get_size();
    step_context->set_buffer_data(m_fxaa_data_buffer, 0, sizeof(m_fxaa_data), &m_fxaa_data);

    m_fxaa_pass_pipeline->get_resource_mapping()->set("fxaa_data", m_fxaa_data_buffer);
    m_fxaa_pass_pipeline->get_resource_mapping()->set("texture_input", m_texture_input);
    m_fxaa_pass_pipeline->get_resource_mapping()->set("sampler_input", m_sampler_input);

    step_context->submit_pipeline_state_resources();

    step_context->draw(3, 0, 1, 0, 0, 0); // Triangle gets created in geometry shader.

    step_context->end();
    step_context->submit();

    // #ifdef MANGO_DEBUG
    //     bsp                      = m_fxaa_command_buffer->create<bind_shader_program_command>(command_keys::no_sort);
    //     bsp->shader_program_name = 0;
    // #endif // MANGO_DEBUG
}

void fxaa_step::on_ui_widget()
{
    ImGui::PushID("fxaa_step");

    // Quality Preset
    const char* presets[3] = { "Medium Quality", "High Quality", "Extreme Quality" };
    bool changed           = combo("FXAA Mode", presets, 3, m_fxaa_data.quality_preset, 1);

    float default_value = 0.0f;
    float spf           = m_fxaa_data.subpixel_filter;
    changed |= slider_float_n("Subpixel Filter", &spf, 1, &default_value, 0.0f, 1.0f);
    m_fxaa_data.subpixel_filter = spf;

    ImGui::PopID();
}
