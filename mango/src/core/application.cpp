//! \file      application.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <core/context_impl.hpp>
#include <core/window_system_impl.hpp>
#include <mango/application.hpp>
#include <mango/assert.hpp>
#include <rendering/render_system_impl.hpp>

using namespace mango;

application::application()
{
    m_context = make_shared<context_impl>();
    m_context->create();
}

application::~application()
{
    MANGO_ASSERT(m_context, "Context is not valid!");
    m_context->destroy();
}

uint32 application::run(uint32 t_argc, char** t_argv)
{
    MANGO_UNUSED(t_argc);
    MANGO_UNUSED(t_argv);

    bool should_close = false;

    while (!should_close)
    {
        shared_ptr<window_system_impl> ws = m_context->get_window_system_internal().lock();
        MANGO_ASSERT(ws, "Window System is expired!");
        shared_ptr<render_system_impl> rs = m_context->get_render_system_internal().lock();
        MANGO_ASSERT(rs, "Render System is expired!");

        // update
        ws->update(0.0f);
        rs->update(0.0f);

        // render
        rs->start_frame();

        rs->finish_frame();

        rs->render();

        // swap buffers
        ws->swap_buffers();

        // poll events
        ws->poll_events();
        should_close = ws->should_close();
    }

    return 0;
}

weak_ptr<context> application::get_context()
{
    return m_context;
}
