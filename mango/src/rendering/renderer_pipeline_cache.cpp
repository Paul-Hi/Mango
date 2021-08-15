//! \file      renderer_pipeline_cache.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#include <graphics/graphics.hpp>
#include <rendering/renderer_pipeline_cache.hpp>

using namespace mango;

gfx_handle<const gfx_pipeline> renderer_pipeline_cache::get_opaque(const vertex_input_descriptor& geo_vid, const input_assembly_descriptor& geo_iad, bool wireframe)
{
    pipeline_key key;
    key.vid       = geo_vid;
    key.iad       = geo_iad;
    key.wireframe = wireframe;
    auto it       = m_opaque_cache.find(key);
    if (it != m_opaque_cache.end())
        return it->second;

    auto create_info = m_opaque_create_info;

    create_info.vertex_input_state   = geo_vid;
    create_info.input_assembly_state = geo_iad;
    if (wireframe)
        create_info.rasterization_state.polygon_mode = gfx_polygon_mode::polygon_mode_line;

    auto& graphics_device = m_shared_context->get_graphics_device();

    gfx_handle<const gfx_pipeline> created_pipeline = graphics_device->create_graphics_pipeline(create_info);

    m_opaque_cache.insert({ key, created_pipeline });

    return created_pipeline;
}

gfx_handle<const gfx_pipeline> renderer_pipeline_cache::get_transparent(const vertex_input_descriptor& geo_vid, const input_assembly_descriptor& geo_iad, bool wireframe)
{
    pipeline_key key;
    key.vid       = geo_vid;
    key.iad       = geo_iad;
    key.wireframe = wireframe;
    auto it       = m_transparent_cache.find(key);
    if (it != m_transparent_cache.end())
        return it->second;

    auto create_info = m_transparent_create_info;

    create_info.vertex_input_state   = geo_vid;
    create_info.input_assembly_state = geo_iad;
    if (wireframe)
        create_info.rasterization_state.polygon_mode = gfx_polygon_mode::polygon_mode_line;

    auto& graphics_device = m_shared_context->get_graphics_device();

    gfx_handle<const gfx_pipeline> created_pipeline = graphics_device->create_graphics_pipeline(create_info);

    m_transparent_cache.insert({ key, created_pipeline });

    return created_pipeline;
}

gfx_handle<const gfx_pipeline> renderer_pipeline_cache::get_shadow(const vertex_input_descriptor& geo_vid, const input_assembly_descriptor& geo_iad)
{
    pipeline_key key;
    key.vid       = geo_vid;
    key.iad       = geo_iad;
    key.wireframe = false;
    auto it       = m_shadow_cache.find(key);
    if (it != m_shadow_cache.end())
        return it->second;

    auto create_info = m_shadow_create_info;

    create_info.vertex_input_state   = geo_vid;
    create_info.input_assembly_state = geo_iad;

    auto& graphics_device = m_shared_context->get_graphics_device();

    gfx_handle<const gfx_pipeline> created_pipeline = graphics_device->create_graphics_pipeline(create_info);

    m_shadow_cache.insert({ key, created_pipeline });

    return created_pipeline;
}