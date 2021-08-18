//! \file      environment_display_step.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#include <mango/imgui_helper.hpp>
#include <mango/profile.hpp>
#include <rendering/renderer_impl.hpp>
#include <rendering/steps/environment_display_step.hpp>
#include <resources/resources_impl.hpp>
#include <util/helpers.hpp>

using namespace mango;

static const float cubemap_vertices[36] = { -1.0f, 1.0f, 1.0f,  1.0f, 1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f,
                                            -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f, -1.0f, 1.0f };

static const uint8 cubemap_indices[18] = { 8, 9, 0, 2, 1, 3, 3, 2, 5, 4, 7, 6, 6, 0, 7, 1, 10, 11 };

environment_display_step::environment_display_step(const environment_display_settings& settings)
    : m_settings(settings)
{
    m_cubemap_data.render_level = m_settings.get_render_level();
    MANGO_ASSERT(m_cubemap_data.render_level >= 0.0f && m_cubemap_data.render_level < 10.1f, "Cubemap render level has to be between 0.0 and 10.0f!");
}

environment_display_step::~environment_display_step() {}

bool environment_display_step::create_step_resources()
{
    PROFILE_ZONE;
    auto& graphics_device = m_shared_context->get_graphics_device();

    // buffers
    buffer_create_info buffer_info;
    buffer_info.buffer_target = gfx_buffer_target::buffer_target_vertex;
    buffer_info.buffer_access = gfx_buffer_access::buffer_access_dynamic_storage;

    buffer_info.size = sizeof(cubemap_vertices);
    m_cube_vertices  = graphics_device->create_buffer(buffer_info);
    if (!check_creation(m_cube_vertices.get(), "cubemap vertex buffer"))
        return false;

    buffer_info.buffer_target = gfx_buffer_target::buffer_target_index;
    buffer_info.size          = sizeof(cubemap_indices);
    m_cube_indices            = graphics_device->create_buffer(buffer_info);
    if (!check_creation(m_cube_indices.get(), "cubemap index buffer"))
        return false;

    auto device_context = graphics_device->create_graphics_device_context();
    device_context->begin();
    device_context->set_buffer_data(m_cube_vertices, 0, sizeof(cubemap_vertices), const_cast<void*>((void*)cubemap_vertices));
    device_context->set_buffer_data(m_cube_indices, 0, sizeof(cubemap_indices), const_cast<void*>((void*)cubemap_indices));
    device_context->end();
    device_context->submit();

    buffer_info.buffer_target = gfx_buffer_target::buffer_target_uniform;
    buffer_info.size          = sizeof(cubemap_data);
    m_cubemap_data_buffer     = graphics_device->create_buffer(buffer_info);
    if (!check_creation(m_cubemap_data_buffer.get(), "cubemap data buffer"))
        return false;

    // sampler
    sampler_create_info sampler_info;
    sampler_info.sampler_min_filter      = gfx_sampler_filter::sampler_filter_linear_mipmap_linear;
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
    sampler_info.enable_seamless_cubemap = true;

    m_cubemap_sampler = graphics_device->create_sampler(sampler_info);

    // shader stages
    auto& internal_resources = m_shared_context->get_internal_resources();
    shader_stage_create_info shader_info;
    shader_resource_resource_description res_resource_desc;
    shader_source_description source_desc;

    // vertex stage
    {
        res_resource_desc.path        = "res/shader/post/v_cubemap.glsl";
        const shader_resource* source = internal_resources->acquire(res_resource_desc);

        source_desc.entry_point = "main";
        source_desc.source      = source->source.c_str();
        source_desc.size        = static_cast<int32>(source->source.size());

        shader_info.stage         = gfx_shader_stage_type::shader_stage_vertex;
        shader_info.shader_source = source_desc;

        shader_info.resource_count = 3;

        shader_info.resources = { {
            { gfx_shader_stage_type::shader_stage_vertex, CAMERA_DATA_BUFFER_BINDING_POINT, "camera_data", gfx_shader_resource_type::shader_resource_buffer_storage, 1 },
            { gfx_shader_stage_type::shader_stage_vertex, RENDERER_DATA_BUFFER_BINDING_POINT, "renderer_data", gfx_shader_resource_type::shader_resource_buffer_storage, 1 },
            { gfx_shader_stage_type::shader_stage_vertex, 3, "cubemap_data", gfx_shader_resource_type::shader_resource_buffer_storage, 1 },
        } };

        m_cubemap_pass_vertex = graphics_device->create_shader_stage(shader_info);
        if (!check_creation(m_cubemap_pass_vertex.get(), "cubemap pass vertex shader"))
            return false;

        res_resource_desc.defines.clear();
    }
    // fragment stage
    {
        res_resource_desc.path        = "res/shader/post/f_cubemap.glsl";
        const shader_resource* source = internal_resources->acquire(res_resource_desc);

        source_desc.entry_point = "main";
        source_desc.source      = source->source.c_str();
        source_desc.size        = static_cast<int32>(source->source.size());

        shader_info.stage         = gfx_shader_stage_type::shader_stage_fragment;
        shader_info.shader_source = source_desc;

        shader_info.resource_count = 6;

        shader_info.resources = { {
            { gfx_shader_stage_type::shader_stage_fragment, CAMERA_DATA_BUFFER_BINDING_POINT, "camera_data", gfx_shader_resource_type::shader_resource_buffer_storage, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, RENDERER_DATA_BUFFER_BINDING_POINT, "renderer_data", gfx_shader_resource_type::shader_resource_buffer_storage, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, LIGHT_DATA_BUFFER_BINDING_POINT, "light_data", gfx_shader_resource_type::shader_resource_buffer_storage, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, 0, "texture_environment_cubemap", gfx_shader_resource_type::shader_resource_texture, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, 0, "sampler_environment_cubemap", gfx_shader_resource_type::shader_resource_sampler, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, 3, "cubemap_data", gfx_shader_resource_type::shader_resource_buffer_storage, 1 },
        } };

        m_cubemap_pass_fragment = graphics_device->create_shader_stage(shader_info);
        if (!check_creation(m_cubemap_pass_fragment.get(), "cubemap pass fragment shader"))
            return false;

        res_resource_desc.defines.clear();
    }

    graphics_pipeline_create_info cubemap_pass_info = graphics_device->provide_graphics_pipeline_create_info();
    auto cubemap_pass_pipeline_layout               = graphics_device->create_pipeline_resource_layout({
        { gfx_shader_stage_type::shader_stage_vertex, CAMERA_DATA_BUFFER_BINDING_POINT, gfx_shader_resource_type::shader_resource_buffer_storage, gfx_shader_resource_access::shader_access_dynamic },
        { gfx_shader_stage_type::shader_stage_vertex, RENDERER_DATA_BUFFER_BINDING_POINT, gfx_shader_resource_type::shader_resource_buffer_storage, gfx_shader_resource_access::shader_access_dynamic },
        { gfx_shader_stage_type::shader_stage_vertex, 3, gfx_shader_resource_type::shader_resource_buffer_storage, gfx_shader_resource_access::shader_access_dynamic },

        { gfx_shader_stage_type::shader_stage_fragment, CAMERA_DATA_BUFFER_BINDING_POINT, gfx_shader_resource_type::shader_resource_buffer_storage, gfx_shader_resource_access::shader_access_dynamic },
        { gfx_shader_stage_type::shader_stage_fragment, RENDERER_DATA_BUFFER_BINDING_POINT, gfx_shader_resource_type::shader_resource_buffer_storage,
          gfx_shader_resource_access::shader_access_dynamic },
        { gfx_shader_stage_type::shader_stage_fragment, LIGHT_DATA_BUFFER_BINDING_POINT, gfx_shader_resource_type::shader_resource_buffer_storage, gfx_shader_resource_access::shader_access_dynamic },
        { gfx_shader_stage_type::shader_stage_fragment, 3, gfx_shader_resource_type::shader_resource_buffer_storage, gfx_shader_resource_access::shader_access_dynamic },

        { gfx_shader_stage_type::shader_stage_fragment, 0, gfx_shader_resource_type::shader_resource_texture, gfx_shader_resource_access::shader_access_dynamic },
        { gfx_shader_stage_type::shader_stage_fragment, 0, gfx_shader_resource_type::shader_resource_sampler, gfx_shader_resource_access::shader_access_dynamic },
    });

    cubemap_pass_info.pipeline_layout = cubemap_pass_pipeline_layout;

    cubemap_pass_info.shader_stage_descriptor.vertex_shader_stage   = m_cubemap_pass_vertex;
    cubemap_pass_info.shader_stage_descriptor.fragment_shader_stage = m_cubemap_pass_fragment;

    cubemap_pass_info.vertex_input_state.attribute_description_count = 1;
    cubemap_pass_info.vertex_input_state.binding_description_count   = 1;

    vertex_input_binding_description binding_desc;
    vertex_input_attribute_description attrib_desc;
    binding_desc.binding    = 0;
    binding_desc.stride     = 3 * sizeof(float);
    binding_desc.input_rate = gfx_vertex_input_rate::per_vertex;

    attrib_desc.binding          = 0;
    attrib_desc.offset           = 0;
    attrib_desc.attribute_format = gfx_format::rgb32f;
    attrib_desc.location         = 0;

    cubemap_pass_info.vertex_input_state.binding_descriptions[0]   = binding_desc;
    cubemap_pass_info.vertex_input_state.attribute_descriptions[0] = attrib_desc;

    cubemap_pass_info.input_assembly_state.topology = gfx_primitive_topology::primitive_topology_triangle_strip;

    // viewport_descriptor is dynamic

    cubemap_pass_info.rasterization_state.cull_mode              = gfx_cull_mode_flag_bits::mode_front;
    cubemap_pass_info.depth_stencil_state.depth_compare_operator = gfx_compare_operator::compare_operator_less_equal;
    // blend_state -> keep default

    cubemap_pass_info.dynamic_state.dynamic_states = gfx_dynamic_state_flag_bits::dynamic_state_viewport | gfx_dynamic_state_flag_bits::dynamic_state_scissor; // set by renderer // TODO Paul: Good?

    m_cubemap_pass_pipeline = graphics_device->create_graphics_pipeline(cubemap_pass_info);

    return true;
}

void environment_display_step::attach(const shared_ptr<context_impl>& context)
{
    m_shared_context = context;

    create_step_resources();
}

void environment_display_step::execute()
{
    PROFILE_ZONE;
    if (!m_current_cubemap || m_cubemap_data.render_level < 0.0f)
        return;

    auto& graphics_device = m_shared_context->get_graphics_device();

    auto step_context = graphics_device->create_graphics_device_context();

    step_context->begin();

    step_context->bind_pipeline(m_cubemap_pass_pipeline);

    // TODO Paul: Other Uniform Buffers? Can we be sure, that they are set by renderer? -.-

    m_cubemap_data.model_matrix = mat4(1.0f); // TODO Paul!
    step_context->set_buffer_data(m_cubemap_data_buffer, 0, sizeof(m_cubemap_data), &m_cubemap_data);

    m_cubemap_pass_pipeline->get_resource_mapping()->set("cubemap_data", m_cubemap_data_buffer);
    m_cubemap_pass_pipeline->get_resource_mapping()->set("texture_environment_cubemap", m_current_cubemap);
    m_cubemap_pass_pipeline->get_resource_mapping()->set("sampler_environment_cubemap", m_cubemap_sampler);

    step_context->submit_pipeline_state_resources();

    step_context->set_index_buffer(m_cube_indices, gfx_format::t_unsigned_byte);
    int32 list[1] = { 0 };
    step_context->set_vertex_buffers(1, &m_cube_vertices, list, list);

    step_context->draw(0, 18, 1, 0, 0, 0);

    step_context->end();
    step_context->submit();

    // #ifdef MANGO_DEBUG
    //     bva                    = m_cubemap_command_buffer->create<bind_vertex_array_command>(command_keys::no_sort);
    //     bva->vertex_array_name = 0;
    //
    //     bsp                      = m_cubemap_command_buffer->create<bind_shader_program_command>(command_keys::no_sort);
    //     bsp->shader_program_name = 0;
    // #endif // MANGO_DEBUG
}

void environment_display_step::on_ui_widget()
{
    ImGui::PushID("environment_display_step");
    // Render Level 0.0 - 10.0
    static float tmp = 0.0f;

    m_cubemap_data.render_level = tmp;
    float& render_level         = m_cubemap_data.render_level;
    float default_value         = 0.0f;
    slider_float_n("Blur Level", &render_level, 1, &default_value, 0.0f, 10.0f);
    tmp = m_cubemap_data.render_level;

    ImGui::PopID();
}
