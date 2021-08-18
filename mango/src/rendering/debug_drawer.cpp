//! \file      debug_drawer.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#include <mango/profile.hpp>
#include <rendering/debug_drawer.hpp>
#include <resources/resources_impl.hpp>

using namespace mango;

debug_drawer::debug_drawer(const shared_ptr<context_impl>& context)
    : m_shared_context(context)
    , m_buffer_size(32 * 2 * sizeof(vec3)) // TODO Paul: Size?
    , m_vertex_count(0)
{
    create_pipeline_resources();
}

bool debug_drawer::create_pipeline_resources()
{
    PROFILE_ZONE;
    auto& graphics_device = m_shared_context->get_graphics_device();

    // buffers
    buffer_create_info buffer_info;
    buffer_info.buffer_target = gfx_buffer_target::buffer_target_vertex;
    buffer_info.buffer_access = gfx_buffer_access::buffer_access_dynamic_storage;

    buffer_info.size = m_buffer_size;
    m_vertex_buffer  = graphics_device->create_buffer(buffer_info);
    if (!check_creation(m_vertex_buffer.get(), "debug draw vertex buffer"))
        return false;

    // shader stages
    auto& internal_resources = m_shared_context->get_internal_resources();
    shader_stage_create_info shader_info;
    shader_resource_resource_description res_resource_desc;
    shader_source_description source_desc;

    // vertex stage
    {
        res_resource_desc.path        = "res/shader/post/v_debug_drawer.glsl";
        const shader_resource* source = internal_resources->acquire(res_resource_desc);

        source_desc.entry_point = "main";
        source_desc.source      = source->source.c_str();
        source_desc.size        = static_cast<int32>(source->source.size());

        shader_info.stage         = gfx_shader_stage_type::shader_stage_vertex;
        shader_info.shader_source = source_desc;

        shader_info.resource_count = 0;

        m_debug_draw_vertex = graphics_device->create_shader_stage(shader_info);
        if (!check_creation(m_debug_draw_vertex.get(), "debug drawing pass vertex shader"))
            return false;

        res_resource_desc.defines.clear();
    }
    // fragment stage
    {
        res_resource_desc.path        = "res/shader/post/f_debug_drawer.glsl";
        const shader_resource* source = internal_resources->acquire(res_resource_desc);

        source_desc.entry_point = "main";
        source_desc.source      = source->source.c_str();
        source_desc.size        = static_cast<int32>(source->source.size());

        shader_info.stage         = gfx_shader_stage_type::shader_stage_fragment;
        shader_info.shader_source = source_desc;

        shader_info.resource_count = 0;

        m_debug_draw_fragment = graphics_device->create_shader_stage(shader_info);
        if (!check_creation(m_debug_draw_fragment.get(), "debug drawing pass fragment shader"))
            return false;

        res_resource_desc.defines.clear();
    }

    graphics_pipeline_create_info debug_drawing_pass_info = graphics_device->provide_graphics_pipeline_create_info();
    auto debug_drawing_pass_pipeline_layout               = graphics_device->create_pipeline_resource_layout({});

    debug_drawing_pass_info.pipeline_layout = debug_drawing_pass_pipeline_layout;

    debug_drawing_pass_info.shader_stage_descriptor.vertex_shader_stage   = m_debug_draw_vertex;
    debug_drawing_pass_info.shader_stage_descriptor.fragment_shader_stage = m_debug_draw_fragment;

    debug_drawing_pass_info.vertex_input_state.attribute_description_count = 2;
    debug_drawing_pass_info.vertex_input_state.binding_description_count   = 2;

    vertex_input_binding_description binding_desc;
    vertex_input_attribute_description attrib_desc;

    // position
    binding_desc.binding    = 0;
    binding_desc.stride     = 6 * sizeof(float);
    binding_desc.input_rate = gfx_vertex_input_rate::per_vertex;

    attrib_desc.binding          = 0;
    attrib_desc.offset           = 0;
    attrib_desc.attribute_format = gfx_format::rgb32f;
    attrib_desc.location         = 0;

    debug_drawing_pass_info.vertex_input_state.binding_descriptions[0]   = binding_desc;
    debug_drawing_pass_info.vertex_input_state.attribute_descriptions[0] = attrib_desc;

    // color
    binding_desc.binding    = 1;
    binding_desc.stride     = 6 * sizeof(float);
    binding_desc.input_rate = gfx_vertex_input_rate::per_vertex;

    attrib_desc.binding          = 1;
    attrib_desc.offset           = 0;
    attrib_desc.attribute_format = gfx_format::rgb32f;
    attrib_desc.location         = 1;

    debug_drawing_pass_info.vertex_input_state.binding_descriptions[1]   = binding_desc;
    debug_drawing_pass_info.vertex_input_state.attribute_descriptions[1] = attrib_desc;

    debug_drawing_pass_info.input_assembly_state.topology = gfx_primitive_topology::primitive_topology_line_list;

    // viewport_descriptor is dynamic
    debug_drawing_pass_info.rasterization_state.polygon_mode = gfx_polygon_mode::polygon_mode_line;
    debug_drawing_pass_info.rasterization_state.cull_mode    = gfx_cull_mode_flag_bits::mode_none;
    debug_drawing_pass_info.rasterization_state.line_width   = 2.0f;
    // blend_state -> keep default

    debug_drawing_pass_info.dynamic_state.dynamic_states =
        gfx_dynamic_state_flag_bits::dynamic_state_viewport | gfx_dynamic_state_flag_bits::dynamic_state_scissor; // set by renderer // TODO Paul: Good?

    m_debug_draw_pipeline = graphics_device->create_graphics_pipeline(debug_drawing_pass_info);

    return true;
}

void debug_drawer::set_color(const color_rgb& color)
{
    m_color = color;
}

void debug_drawer::clear()
{
    m_vertices.clear();
}

void debug_drawer::add(const vec3& point0, const vec3& point1)
{
    m_vertices.push_back(point0);
    m_vertices.push_back(m_color);
    m_vertices.push_back(point1);
    m_vertices.push_back(m_color);
}

void debug_drawer::update_buffer()
{
    PROFILE_ZONE;
    auto& graphics_device = m_shared_context->get_graphics_device();

    while (static_cast<int32>(m_vertices.size()) * sizeof(vec3) > m_buffer_size)
    {
        m_buffer_size *= 2;
        buffer_create_info buffer_info;
        buffer_info.buffer_target = gfx_buffer_target::buffer_target_vertex;
        buffer_info.buffer_access = gfx_buffer_access::buffer_access_dynamic_storage;

        buffer_info.size = m_buffer_size;
        m_vertex_buffer  = graphics_device->create_buffer(buffer_info);
        check_creation(m_vertex_buffer.get(), "debug draw vertex buffer");
    }

    auto device_context = graphics_device->create_graphics_device_context();
    device_context->begin();
    device_context->set_buffer_data(m_vertex_buffer, 0, static_cast<int32>(m_vertices.size()) * sizeof(vec3), m_vertices.data());
    device_context->end();
    device_context->submit();
    m_vertex_count = static_cast<int32>(m_vertices.size()) / 2;
}

void debug_drawer::execute()
{
    PROFILE_ZONE;

    auto& graphics_device = m_shared_context->get_graphics_device();

    auto debug_draw_context = graphics_device->create_graphics_device_context();

    debug_draw_context->begin();

    debug_draw_context->bind_pipeline(m_debug_draw_pipeline);

    // TODO Paul: Other Uniform Buffers? Can we be sure, that they are set by renderer? -.-

    debug_draw_context->submit_pipeline_state_resources();

    int32 list0[2]                                 = { 0, 1 };
    int32 list1[2]                                 = { 0, 3 * sizeof(float) };
    gfx_handle<const gfx_buffer> vertex_buffers[2] = { m_vertex_buffer, m_vertex_buffer };
    debug_draw_context->set_vertex_buffers(2, vertex_buffers, list0, list1);

    debug_draw_context->draw(m_vertex_count, 0, 1, 0, 0, 0);

    debug_draw_context->end();
    debug_draw_context->submit();
}
