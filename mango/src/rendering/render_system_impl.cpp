//! \file      render_system_impl.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <rendering/pipelines/deferred_pbr_render_system.hpp>
#include <rendering/render_system_impl.hpp>

using namespace mango;

render_system_impl::render_system_impl(const shared_ptr<context_impl>& context)
    : m_shared_context(context)
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
            MANGO_ASSERT(m_current_render_system->create(), "Creation of the deferred pbr render system did fail!");
            break;

        default:
            MANGO_LOG_ERROR("Render pipeline is unknown and and the render system cannot be created!");
            break;
        }
    }

    m_current_render_system->configure(configuration);
}

void render_system_impl::start_frame()
{
    MANGO_ASSERT(m_current_render_system, "Current render sytem not valid!");
    m_current_render_system->start_frame();
}

void render_system_impl::submit(const render_command& command)
{
    MANGO_ASSERT(m_current_render_system, "Current render sytem not valid!");
    m_current_render_system->submit(command);
}

void render_system_impl::finish_frame()
{
    MANGO_ASSERT(m_current_render_system, "Current render sytem not valid!");
    m_current_render_system->finish_frame();
}

void render_system_impl::render()
{
    MANGO_ASSERT(m_current_render_system, "Current render sytem not valid!");
    m_current_render_system->render();
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

void render_system_impl::updateState(const render_state& state)
{
    m_render_state = state;
}
