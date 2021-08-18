//! \file      application.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#include <core/context_impl.hpp>
#include <core/timer.hpp>
#include <mango/application.hpp>
#include <mango/assert.hpp>
#include <mango/profile.hpp>
#include <mango/scene.hpp>
#include <ui/ui_impl.hpp>
// #include <core/input_system_impl.hpp>
// #include <core/window_system_impl.hpp>
// #include <rendering/renderer_impl.hpp>
// #include <resources/resources.hpp>

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
        m_context->poll_events();
        m_should_close = m_should_close || m_context->should_shutdown();

        m_frametime = static_cast<float>(m_frame_timer->elapsedMicroseconds().count()) * 0.000001f; // We need the resolution. //TODO Paul: We could wrap this with some kind of 'high res clock'.
// Debug every second
#ifdef MANGO_DEBUG
        static float fps_lock = 0.0f;
        fps_lock += m_frametime;
        if (fps_lock >= 1.0f)
        {
            fps_lock -= 1.0f;
            MANGO_LOG_DEBUG("Frame Time: {0} ms", m_frametime * 1000.0f);
            MANGO_LOG_DEBUG("Framerate: {0} fps", 1.0f / m_frametime);
        }
#endif // MANGO_DEBUG
        m_frame_timer->restart();

        // update
        update(m_frametime);
        m_context->update(m_frametime);

        // render
        m_context->render(m_frametime);

        MARK_FRAME;
    }

    return 0;
}

weak_ptr<context> application::get_context()
{
    return m_context;
}
