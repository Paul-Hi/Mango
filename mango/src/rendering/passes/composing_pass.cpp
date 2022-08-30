//! \file      composing_pass.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2022
//! \copyright Apache License 2.0

#include <mango/imgui_helper.hpp>
#include <mango/profile.hpp>
#include <rendering/passes/composing_pass.hpp>
#include <rendering/renderer_bindings.hpp>
#include <resources/resources_impl.hpp>

using namespace mango;

const render_pass_execution_info composing_pass::s_rpei{ 1, 3 };

composing_pass::composing_pass(const composing_settings& settings)
{
    m_composing_data.exposure_bias = settings.get_exposure_bias();
    m_composing_data.contrast      = settings.get_contrast();
    m_composing_data.tint          = settings.get_tint();
    m_composing_data.saturation    = settings.get_saturation();
    m_composing_data.lift          = settings.get_lift();
    m_composing_data.gamma         = settings.get_gamma();
    m_composing_data.gain          = settings.get_gain();
}

void composing_pass::attach(const shared_ptr<context_impl>& context)
{
    m_shared_context = context;

    create_pass_resources();
}

void composing_pass::execute(graphics_device_context_handle& device_context)
{
    GL_NAMED_PROFILE_ZONE("Composing Pass");
    NAMED_PROFILE_ZONE("Composing Pass");
    device_context->bind_pipeline(m_composing_pass_pipeline);

    device_context->set_viewport(0, 1, &m_viewport);

    device_context->set_render_targets(static_cast<int32>(m_render_targets.size()) - 1, m_render_targets.data(), m_render_targets.back());

    device_context->set_buffer_data(m_composing_data_buffer, 0, sizeof(m_composing_data), &m_composing_data);

    m_composing_pass_pipeline->get_resource_mapping()->set("camera_data", m_camera_data_buffer);
    m_composing_pass_pipeline->get_resource_mapping()->set("renderer_data", m_renderer_data_buffer);
    m_composing_pass_pipeline->get_resource_mapping()->set("composing_data", m_composing_data_buffer);
    m_composing_pass_pipeline->get_resource_mapping()->set("texture_hdr_input", m_hdr_input);
    m_composing_pass_pipeline->get_resource_mapping()->set("sampler_hdr_input", m_hdr_input_sampler);
    m_composing_pass_pipeline->get_resource_mapping()->set("texture_geometry_depth_input", m_depth_input);
    m_composing_pass_pipeline->get_resource_mapping()->set("sampler_geometry_depth_input", m_depth_input_sampler);

    device_context->submit_pipeline_state_resources();

    device_context->set_index_buffer(nullptr, gfx_format::invalid);
    device_context->set_vertex_buffers(0, nullptr, nullptr, nullptr);

    device_context->draw(3, 0, 1, 0, 0, 0); // Triangle gets created in vertex shader.
}

bool composing_pass::create_pass_resources()
{
    PROFILE_ZONE;
    auto& graphics_device    = m_shared_context->get_graphics_device();
    auto& internal_resources = m_shared_context->get_internal_resources();

    // buffer
    buffer_create_info buffer_info;
    buffer_info.buffer_target = gfx_buffer_target::buffer_target_uniform;
    buffer_info.buffer_access = gfx_buffer_access::buffer_access_dynamic_storage;
    buffer_info.size          = sizeof(composing_data);

    m_composing_data_buffer = graphics_device->create_buffer(buffer_info);
    if (!check_creation(m_composing_data_buffer.get(), "composing data buffer"))
        return false;

    shader_stage_create_info shader_info;
    shader_resource_resource_description res_resource_desc;
    shader_source_description source_desc;

    // Screen Space Quad for Compositing
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
    // Composing Pass Fragment Stage
    {
        res_resource_desc.path = "res/shader/post/f_composing.glsl";
        res_resource_desc.defines.push_back({ "COMPOSING", "" });
        const shader_resource* source = internal_resources->acquire(res_resource_desc);

        source_desc.entry_point = "main";
        source_desc.source      = source->source.c_str();
        source_desc.size        = static_cast<int32>(source->source.size());

        shader_info.stage         = gfx_shader_stage_type::shader_stage_fragment;
        shader_info.shader_source = source_desc;

        shader_info.resource_count = 7;

        shader_info.resources = { {
            { gfx_shader_stage_type::shader_stage_fragment, CAMERA_DATA_BUFFER_BINDING_POINT, "camera_data", gfx_shader_resource_type::shader_resource_constant_buffer, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, RENDERER_DATA_BUFFER_BINDING_POINT, "renderer_data", gfx_shader_resource_type::shader_resource_constant_buffer, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, COMPOSING_DATA_BUFFER_BINDING_POINT, "composing_data", gfx_shader_resource_type::shader_resource_constant_buffer, 1 },

            { gfx_shader_stage_type::shader_stage_fragment, COMPOSING_HDR_SAMPLER, "texture_hdr_input", gfx_shader_resource_type::shader_resource_input_attachment, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, COMPOSING_HDR_SAMPLER, "sampler_hdr_input", gfx_shader_resource_type::shader_resource_sampler, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, COMPOSING_DEPTH_SAMPLER, "texture_geometry_depth_input", gfx_shader_resource_type::shader_resource_input_attachment, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, COMPOSING_DEPTH_SAMPLER, "sampler_geometry_depth_input", gfx_shader_resource_type::shader_resource_sampler, 1 },
        } };

        m_composing_pass_fragment = graphics_device->create_shader_stage(shader_info);
        if (!check_creation(m_composing_pass_fragment.get(), "composing pass fragment shader"))
            return false;

        res_resource_desc.defines.clear();
    }

    graphics_pipeline_create_info composing_pass_info = graphics_device->provide_graphics_pipeline_create_info();
    auto composing_pass_pipeline_layout               = graphics_device->create_pipeline_resource_layout({
                      { gfx_shader_stage_type::shader_stage_fragment, CAMERA_DATA_BUFFER_BINDING_POINT, gfx_shader_resource_type::shader_resource_constant_buffer,
                        gfx_shader_resource_access::shader_access_dynamic },
                      { gfx_shader_stage_type::shader_stage_fragment, RENDERER_DATA_BUFFER_BINDING_POINT, gfx_shader_resource_type::shader_resource_constant_buffer,
                        gfx_shader_resource_access::shader_access_dynamic },
                      { gfx_shader_stage_type::shader_stage_fragment, COMPOSING_DATA_BUFFER_BINDING_POINT, gfx_shader_resource_type::shader_resource_constant_buffer,
                        gfx_shader_resource_access::shader_access_dynamic },

                      { gfx_shader_stage_type::shader_stage_fragment, COMPOSING_HDR_SAMPLER, gfx_shader_resource_type::shader_resource_input_attachment, gfx_shader_resource_access::shader_access_dynamic },
                      { gfx_shader_stage_type::shader_stage_fragment, COMPOSING_HDR_SAMPLER, gfx_shader_resource_type::shader_resource_sampler, gfx_shader_resource_access::shader_access_dynamic },
                      { gfx_shader_stage_type::shader_stage_fragment, COMPOSING_DEPTH_SAMPLER, gfx_shader_resource_type::shader_resource_input_attachment, gfx_shader_resource_access::shader_access_dynamic },
                      { gfx_shader_stage_type::shader_stage_fragment, COMPOSING_DEPTH_SAMPLER, gfx_shader_resource_type::shader_resource_sampler, gfx_shader_resource_access::shader_access_dynamic },
    });

    composing_pass_info.pipeline_layout = composing_pass_pipeline_layout;

    composing_pass_info.shader_stage_descriptor.vertex_shader_stage   = m_screen_space_triangle_vertex;
    composing_pass_info.shader_stage_descriptor.fragment_shader_stage = m_composing_pass_fragment;

    composing_pass_info.vertex_input_state.attribute_description_count = 0;
    composing_pass_info.vertex_input_state.binding_description_count   = 0;

    composing_pass_info.input_assembly_state.topology = gfx_primitive_topology::primitive_topology_triangle_list; // Not relevant.

    // viewport_descriptor is dynamic

    // rasterization_state -> keep default
    composing_pass_info.depth_stencil_state.depth_compare_operator = gfx_compare_operator::compare_operator_always; // Do not disable since it writes the depth in the fragment shader.
    // blend_state -> keep default

    composing_pass_info.dynamic_state.dynamic_states = gfx_dynamic_state_flag_bits::dynamic_state_viewport | gfx_dynamic_state_flag_bits::dynamic_state_scissor;

    m_composing_pass_pipeline = graphics_device->create_graphics_pipeline(composing_pass_info);

    return true;
}

void composing_pass::on_ui_widget()
{
    ImGui::PushID("composing_pass");

    const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_AllowItemOverlap;

    bool changed = false;
    bool open    = ImGui::CollapsingHeader("Composing", flags);
    if (open)
    {
        float default_fl3[3] = { 0.0f, 0.0f, 0.0f };
        changed |= drag_float_n("Exposure Bias", &m_composing_data.exposure_bias[0], 3, default_fl3, 0.001f, 0.0f, 1.0f, "%.3f", false);

        default_fl3[0] = 1.0f;
        default_fl3[1] = 1.0f;
        default_fl3[2] = 1.0f;
        changed |= drag_float_n("Contrast", &m_composing_data.contrast[0], 3, default_fl3, 0.08f, 0.0f, 2.0f, "%.2f", false);

        default_fl3[0] = 1.0f;
        default_fl3[1] = 0.0f;
        default_fl3[2] = 0.0f;
        changed |= drag_float_n("Color Tint", &m_composing_data.tint[0], 3, default_fl3, 0.08f, 0.0f, 2.0f, "%.2f", false);

        default_fl3[0] = 1.0f;
        default_fl3[1] = 1.0f;
        default_fl3[2] = 1.0f;
        changed |= drag_float_n("Saturation", &m_composing_data.saturation[0], 3, default_fl3, 0.08f, 0.0f, 2.0f, "%.2f", false);

        default_fl3[0] = 0.0f;
        default_fl3[1] = 0.0f;
        default_fl3[2] = 0.0f;
        changed |= drag_float_n("Lift", &m_composing_data.lift[0], 3, default_fl3, 0.08f, 0.0f, 2.0f, "%.2f", false);

        default_fl3[0] = 1.0f;
        default_fl3[1] = 1.0f;
        default_fl3[2] = 1.0f;
        changed |= drag_float_n("Gamma", &m_composing_data.gamma[0], 3, default_fl3, 0.08f, 0.0f, 2.0f, "%.2f", false);

        changed |= drag_float_n("Gain", &m_composing_data.gain[0], 3, default_fl3, 0.08f, 0.0f, 2.0f, "%.2f", false);
    }

    ImGui::PopID();
}
