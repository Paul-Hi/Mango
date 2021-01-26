//! \file      render_system_impl.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <mango/profile.hpp>
#include <rendering/pipelines/deferred_pbr_render_system.hpp>
#include <rendering/render_system_impl.hpp>

using namespace mango;

render_system_impl::render_system_impl(const shared_ptr<context_impl>& context)
    : m_shared_context(context)
    , m_light_stack()
{
}

render_system_impl::~render_system_impl() {}

bool render_system_impl::create()
{
    if (m_current_render_system)
        return m_current_render_system->create();

    return true;
}

void render_system_impl::configure(const render_configuration& configuration)
{
    PROFILE_ZONE;
    bool success                        = false;
    render_pipeline configured_pipeline = configuration.get_base_render_pipeline();
    if (!m_current_render_system || configured_pipeline != m_current_render_system->get_base_render_pipeline())
    {
        // Pipeline has changed and the current render system needs to be recreated.
        if (m_current_render_system)
            m_current_render_system->destroy();

        switch (configured_pipeline)
        {
        case deferred_pbr:
            m_current_render_system = std::make_shared<deferred_pbr_render_system>(m_shared_context);
            success                 = m_current_render_system->create();
            MANGO_ASSERT(success, "Creation of the deferred pbr render system did fail!");
            break;
        default:
            MANGO_LOG_ERROR("Render pipeline is unknown and the render system cannot be created!");
            break;
        }
    }

    if (m_current_render_system)
        m_current_render_system->configure(configuration);

    if (!success)
    {
        MANGO_LOG_ERROR("Render pipeline  failed to create and render system cannot be created!");
    }
}

void render_system_impl::setup_cubemap_step(const cubemap_step_configuration& configuration)
{
    MANGO_ASSERT(m_current_render_system, "Current render sytem not valid!");
    m_current_render_system->setup_cubemap_step(configuration);
}

void render_system_impl::setup_shadow_map_step(const shadow_step_configuration& configuration)
{
    MANGO_ASSERT(m_current_render_system, "Current render sytem not valid!");
    m_current_render_system->setup_shadow_map_step(configuration);
}

void render_system_impl::setup_fxaa_step(const fxaa_step_configuration& configuration)
{
    MANGO_ASSERT(m_current_render_system, "Current render sytem not valid!");
    m_current_render_system->setup_fxaa_step(configuration);
}

void render_system_impl::begin_render()
{
    MANGO_ASSERT(m_current_render_system, "Current render sytem not valid!");
    m_current_render_system->begin_render();
}

void render_system_impl::finish_render(float dt)
{
    MANGO_ASSERT(m_current_render_system, "Current render sytem not valid!");
    m_current_render_system->finish_render(dt);
}

void render_system_impl::set_viewport(int32 x, int32 y, int32 width, int32 height)
{
    MANGO_ASSERT(m_current_render_system, "Current render sytem not valid!");
    MANGO_ASSERT(x >= 0, "Viewport x position has to be positive!");
    MANGO_ASSERT(y >= 0, "Viewport y position has to be positive!");
    MANGO_ASSERT(width >= 0, "Viewport width has to be positive!");
    MANGO_ASSERT(height >= 0, "Viewport height has to be positive!");
    m_current_render_system->set_viewport(x, y, width, height);
}

void render_system_impl::begin_mesh(const glm::mat4& model_matrix, bool has_normals, bool has_tangents)
{
    MANGO_ASSERT(m_current_render_system, "Current render sytem not valid!");
    m_current_render_system->begin_mesh(model_matrix, has_normals, has_tangents);
}

void render_system_impl::end_mesh()
{
    MANGO_ASSERT(m_current_render_system, "Current render sytem not valid!");
    m_current_render_system->end_mesh();
}

void render_system_impl::use_material(const material_ptr& mat)
{
    MANGO_ASSERT(m_current_render_system, "Current render sytem not valid!");
    m_current_render_system->use_material(mat);
}

void render_system_impl::draw_mesh(const vertex_array_ptr& vertex_array, primitive_topology topology, int32 first, int32 count, index_type type, int32 instance_count)
{
    MANGO_ASSERT(m_current_render_system, "Current render sytem not valid!");
    MANGO_ASSERT(first >= 0, "The first index has to be greater than 0!");
    MANGO_ASSERT(count >= 0, "The index count has to be greater than 0!");
    MANGO_ASSERT(instance_count >= 0, "The instance count has to be greater than 0!");
    m_current_render_system->draw_mesh(vertex_array, topology, first, count, type, instance_count);
}

void render_system_impl::submit_light(light_id id, mango_light* light)
{
    MANGO_ASSERT(m_current_render_system, "Current render sytem not valid!");
    m_current_render_system->submit_light(id, light);
}

framebuffer_ptr render_system_impl::get_backbuffer()
{
    MANGO_ASSERT(m_current_render_system, "Current render sytem not valid!");
    return m_current_render_system->get_backbuffer();
}

void render_system_impl::on_ui_widget()
{
    MANGO_ASSERT(m_current_render_system, "Current render sytem not valid!");
    m_current_render_system->on_ui_widget();
}

void render_system_impl::update(float dt)
{
    MANGO_ASSERT(m_current_render_system, "Current render sytem not valid!");
    m_current_render_system->update(dt);
}

void render_system_impl::destroy()
{
    if (m_current_render_system)
        m_current_render_system->destroy();
}

render_pipeline render_system_impl::get_base_render_pipeline()
{
    MANGO_ASSERT(m_current_render_system, "Current render sytem not valid!");
    return m_current_render_system->get_base_render_pipeline();
}
