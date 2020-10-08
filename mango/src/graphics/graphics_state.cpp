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
#include <mango/profile.hpp>

using namespace mango;

graphics_state::graphics_state()
{
    PROFILE_ZONE;
    m_internal_state.shader_program        = nullptr;
    m_internal_state.framebuffer           = nullptr;
    m_internal_state.vertex_array          = nullptr;
    m_internal_state.viewport.x            = 0;
    m_internal_state.viewport.y            = 0;
    m_internal_state.viewport.width        = 0;
    m_internal_state.viewport.height       = 0;
    m_internal_state.poly_mode.face        = polygon_face::face_front_and_back;
    m_internal_state.poly_mode.mode        = polygon_mode::fill;
    m_internal_state.depth_test.enabled    = false;
    m_internal_state.depth_test.depth_func = compare_operation::less;
    m_internal_state.face_culling.enabled  = false;
    m_internal_state.face_culling.face     = polygon_face::face_back;
    m_internal_state.blending.enabled      = false;
    m_internal_state.blending.src          = blend_factor::one;
    m_internal_state.blending.dest         = blend_factor::zero;
}

bool graphics_state::set_viewport(int32 x, int32 y, int32 width, int32 height)
{
    PROFILE_ZONE;
    MANGO_ASSERT(x >= 0, "Viewport x position has to be positive!");
    MANGO_ASSERT(y >= 0, "Viewport y position has to be positive!");
    MANGO_ASSERT(width >= 0, "Viewport width has to be positive!");
    MANGO_ASSERT(height >= 0, "Viewport height has to be positive!");

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
    PROFILE_ZONE;
    if (m_internal_state.depth_test.enabled != enabled)
    {
        m_internal_state.depth_test.enabled = enabled;
        return true;
    }
    return false;
}

bool graphics_state::set_depth_func(compare_operation op)
{
    PROFILE_ZONE;
    if (m_internal_state.depth_test.depth_func != op)
    {
        m_internal_state.depth_test.depth_func = op;
        return true;
    }
    return false;
}

bool graphics_state::set_polygon_mode(polygon_face face, polygon_mode mode)
{
    PROFILE_ZONE;
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
    PROFILE_ZONE;
    if (m_internal_state.vertex_array != vertex_array)
    {
        m_internal_state.vertex_array = vertex_array;
        return true;
    }
    return false;
}

bool graphics_state::bind_shader_program(shader_program_ptr shader_program)
{
    PROFILE_ZONE;
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
    PROFILE_ZONE;
    // TODO Paul
    return true;
}

bool graphics_state::bind_buffer(g_uint index, buffer_ptr buffer, buffer_target target, int64 offset, int64 size)
{
    PROFILE_ZONE;
    MANGO_UNUSED(index);
    MANGO_UNUSED(buffer);
    MANGO_UNUSED(target);
    MANGO_UNUSED(offset);
    MANGO_UNUSED(size);
    // TODO Paul
    return true;
}

bool graphics_state::bind_texture(int32 binding, g_uint name)
{
    PROFILE_ZONE;
    MANGO_ASSERT(binding >= 0, "Texture binding has to be greater than 0!");
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
    PROFILE_ZONE;
    if (m_internal_state.framebuffer != framebuffer)
    {
        m_internal_state.framebuffer = framebuffer;
        return true;
    }
    return false;
}

bool graphics_state::set_face_culling(bool enabled)
{
    PROFILE_ZONE;
    if (m_internal_state.face_culling.enabled != enabled)
    {
        m_internal_state.face_culling.enabled = enabled;
        return true;
    }
    return false;
}

bool graphics_state::set_cull_face(polygon_face face)
{
    PROFILE_ZONE;
    if (m_internal_state.face_culling.face != face)
    {
        m_internal_state.face_culling.face = face;
        return true;
    }
    return false;
}

bool graphics_state::set_blending(bool enabled)
{
    PROFILE_ZONE;
    if (m_internal_state.blending.enabled != enabled)
    {
        m_internal_state.blending.enabled = enabled;
        return true;
    }
    return false;
}

bool graphics_state::set_blend_factors(blend_factor source, blend_factor destination)
{
    PROFILE_ZONE;
    if (m_internal_state.blending.src != source || m_internal_state.blending.dest != destination)
    {
        m_internal_state.blending.src  = source;
        m_internal_state.blending.dest = destination;
        return true;
    }
    return false;
}

bool graphics_state::set_polygon_offset(float factor, float units)
{
    PROFILE_ZONE;
    // TODO Paul
    MANGO_UNUSED(factor);
    MANGO_UNUSED(units);
    return true;
}
