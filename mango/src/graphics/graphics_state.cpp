//! \file      graphics_state.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <graphics/graphics_state.hpp>
#include <graphics/shader_program.hpp>
#include <graphics/vertex_array.hpp>
#include <mango/assert.hpp>
#include <map>

using namespace mango;

graphics_state::graphics_state()
{
    m_internal_state.shader_program        = nullptr;
    m_internal_state.framebuffer           = nullptr;
    m_internal_state.vertex_array          = nullptr;
    m_internal_state.viewport.x            = 0;
    m_internal_state.viewport.y            = 0;
    m_internal_state.viewport.width        = 0;
    m_internal_state.viewport.height       = 0;
    m_internal_state.poly_mode.face        = polygon_face::FACE_FRONT_AND_BACK;
    m_internal_state.poly_mode.mode        = polygon_mode::FILL;
    m_internal_state.depth_test.enabled    = false;
    m_internal_state.depth_test.depth_func = compare_operation::LESS;
    m_internal_state.face_culling.enabled  = false;
    m_internal_state.face_culling.face     = polygon_face::FACE_BACK;
    m_internal_state.blending.enabled      = false;
    m_internal_state.blending.src          = blend_factor::ONE;
    m_internal_state.blending.dest         = blend_factor::ZERO;
}

bool graphics_state::set_viewport(uint32 x, uint32 y, uint32 width, uint32 height)
{
    if (x != m_internal_state.viewport.x || y != m_internal_state.viewport.y || width != m_internal_state.viewport.width || height != m_internal_state.viewport.height)
    {
        m_internal_state.viewport.x      = x;
        m_internal_state.viewport.y      = y;
        m_internal_state.viewport.width  = width;
        m_internal_state.viewport.height = height;
        return true;
    }
    return false;
}

bool graphics_state::set_depth_test(bool enabled)
{
    if (m_internal_state.depth_test.enabled != enabled)
    {
        m_internal_state.depth_test.enabled = enabled;
        return true;
    }
    return false;
}

bool graphics_state::set_depth_func(compare_operation op)
{
    if (m_internal_state.depth_test.depth_func != op)
    {
        m_internal_state.depth_test.depth_func = op;
        return true;
    }
    return false;
}

bool graphics_state::set_polygon_mode(polygon_face face, polygon_mode mode)
{
    if (m_internal_state.poly_mode.face != face || m_internal_state.poly_mode.mode != mode)
    {
        m_internal_state.poly_mode.face = face;
        m_internal_state.poly_mode.mode = mode;
        return true;
    }
    return false;
}

bool graphics_state::bind_vertex_array(vertex_array_ptr vertex_array)
{
    if (m_internal_state.vertex_array != vertex_array)
    {
        m_internal_state.vertex_array = vertex_array;
        return true;
    }
    return false;
}

bool graphics_state::bind_shader_program(shader_program_ptr shader_program)
{
    if (m_internal_state.shader_program != shader_program)
    {
        m_internal_state.shader_program = shader_program;
        m_internal_state.m_active_texture_bindings.fill(0);
        return true;
    }
    return false;
}

bool graphics_state::bind_single_uniform()
{
    // TODO Paul
    return true;
}

bool graphics_state::bind_uniform_buffer(g_uint index, buffer_ptr uniform_buffer)
{
    // TODO Paul
    return true;
}

bool graphics_state::bind_texture(uint32 binding, uint32 name)
{
    if (binding > max_texture_bindings)
        return true;
    auto& t_name = m_internal_state.m_active_texture_bindings.at(binding);
    if (t_name != name)
    {
        t_name = name;
        return true;
    }
    return false;
}

bool graphics_state::bind_framebuffer(framebuffer_ptr framebuffer)
{
    if (m_internal_state.framebuffer != framebuffer)
    {
        m_internal_state.framebuffer = framebuffer;
        return true;
    }
    return false;
}

bool graphics_state::set_face_culling(bool enabled)
{
    if (m_internal_state.face_culling.enabled != enabled)
    {
        m_internal_state.face_culling.enabled = enabled;
        return true;
    }
    return false;
}

bool graphics_state::set_cull_face(polygon_face face)
{
    if (m_internal_state.face_culling.face != face)
    {
        m_internal_state.face_culling.face = face;
        return true;
    }
    return false;
}

bool graphics_state::set_blending(bool enabled)
{
    if (m_internal_state.blending.enabled != enabled)
    {
        m_internal_state.blending.enabled = enabled;
        return true;
    }
    return false;
}

bool graphics_state::set_blend_factors(blend_factor source, blend_factor destination)
{
    if (m_internal_state.blending.src != source || m_internal_state.blending.dest != destination)
    {
        m_internal_state.blending.src  = source;
        m_internal_state.blending.dest = destination;
        return true;
    }
    return false;
}
