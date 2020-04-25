//! \file      application.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <core/context_impl.hpp>
#include <core/window_system_impl.hpp>
#include <graphics/command_buffer.hpp>
#include <mango/application.hpp>
#include <mango/assert.hpp>
#include <mango/scene.hpp>
#include <rendering/render_system_impl.hpp>
#include <resources/resource_system.hpp>

using namespace mango;

application::application()
{
    m_context = std::make_shared<context_impl>();
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
        shared_ptr<scene> scene = m_context->get_current_scene();

        // poll events
        ws->poll_events();
        should_close = ws->should_close();

        // update
        update(0.0f);
        ws->update(0.0f);
        rs->update(0.0f);
        scene->update(0.0f);

        // render
        auto cmdb = rs->get_command_buffer();
        cmdb->set_depth_test(true);
        cmdb->set_cull_face(polygon_face::FACE_BACK);
        cmdb->clear_framebuffer(clear_buffer_mask::COLOR_AND_DEPTH, attachement_mask::ALL_DRAW_BUFFERS_AND_DEPTH, 1.0f, 0.8f, 0.133f, 1.0f);
        // cmdb->set_polygon_mode(polygon_face::FACE_FRONT_AND_BACK, polygon_mode::LINE);

        //
        scene->render();
        //

        rs->render();

        // swap buffers
        ws->swap_buffers();
    }

    return 0;
}

weak_ptr<context> application::get_context()
{
    return m_context;
}
