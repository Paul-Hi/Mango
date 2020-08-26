//! \file      application.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <core/context_impl.hpp>
#include <core/input_system_impl.hpp>
#include <core/timer.hpp>
#include <core/window_system_impl.hpp>
#include <graphics/command_buffer.hpp>
#include <mango/application.hpp>
#include <mango/assert.hpp>
#include <mango/scene.hpp>
#include <rendering/render_system_impl.hpp>
#include <ui/ui_system_impl.hpp>
#include <mango/profile.hpp>

using namespace mango;

application::application()
{
    NAMED_PROFILE_ZONE("Application Startup");
    m_context = std::make_shared<context_impl>();
    m_context->create();

    m_frame_timer = std::make_shared<timer>();
    m_frame_timer->start();
}

application::~application()
{
    NAMED_PROFILE_ZONE("Application Destruction");
    MANGO_ASSERT(m_context, "Context is not valid!");
    m_context->destroy();
}

int32 application::run(int32 t_argc, char** t_argv)
{
    MANGO_UNUSED(t_argc);
    MANGO_UNUSED(t_argv);

    m_should_close = false;

    while (!m_should_close)
    {
        shared_ptr<window_system_impl> ws = m_context->get_window_system_internal().lock();
        MANGO_ASSERT(ws, "Window System is expired!");
        shared_ptr<input_system_impl> is = m_context->get_input_system_internal().lock();
        MANGO_ASSERT(is, "Input System is expired!");
        shared_ptr<render_system_impl> rs = m_context->get_render_system_internal().lock();
        MANGO_ASSERT(rs, "Render System is expired!");
        shared_ptr<ui_system_impl> uis = m_context->get_ui_system_internal().lock();
        MANGO_ASSERT(uis, "UI System is expired!");
        shared_ptr<scene> scene = m_context->get_current_scene();

        // poll events
        ws->poll_events();
        m_should_close = m_should_close || ws->should_close();

        float frame_time = static_cast<float>(m_frame_timer->elapsedMicroseconds().count()) * 0.000001f; // We need the resolution. TODO Paul: We could wrap this with some kind of 'high res clock'.
        m_frame_timer->restart();

        // update
        update(frame_time);
        scene->update(frame_time);
        ws->update(frame_time);
        is->update(frame_time);
        rs->update(frame_time);
        uis->update(frame_time);

        // render
        rs->begin_render();
        //
        scene->render();
        //
        rs->finish_render(frame_time); // needs frame time for auto exposure.
        //
        uis->draw_ui();

        // swap buffers
        ws->swap_buffers();

        MARK_FRAME;
    }

    return 0;
}

weak_ptr<context> application::get_context()
{
    return m_context;
}
