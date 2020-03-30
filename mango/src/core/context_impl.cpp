//! \file      context_impl.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <core/context_impl.hpp>
#if defined(WIN32)
#include <core/win32_window_system.hpp>
#elif defined(LINUX)
#include <core/linux_window_system.hpp>
#endif
#include <mango/application.hpp>
#include <mango/assert.hpp>
#include <mango/scene.hpp>
#include <rendering/render_system_impl.hpp>
#include <resources/resource_system.hpp>
#include <resources/shader_system.hpp>

using namespace mango;

context_impl::context_impl() {}

context_impl::~context_impl() {}

void context_impl::set_application(const shared_ptr<application>& application)
{
    if (m_application)
    {
        m_application->destroy();
        MANGO_LOG_INFO("Destroying the current application '{0}'.", m_application->get_name());
    }
    m_application = application;
    MANGO_LOG_INFO("Setting the application to '{0}'.", m_application->get_name());
    bool success = m_application->create();
    MANGO_ASSERT(success, "Creation of application '{0}' failed!", m_application->get_name());
}

weak_ptr<window_system> context_impl::get_window_system()
{
    return m_window_system;
}

weak_ptr<render_system> context_impl::get_render_system()
{
    return m_render_system;
}

void context_impl::register_scene(shared_ptr<scene>& scene)
{
    scene->m_shared_context = shared_from_this();
}

void context_impl::make_scene_current(shared_ptr<scene>& scene)
{
    m_current_scene = scene;
}

shared_ptr<scene>& context_impl::get_current_scene()
{
    return m_current_scene;
}

weak_ptr<window_system_impl> context_impl::get_window_system_internal()
{
    return m_window_system;
}

weak_ptr<render_system_impl> context_impl::get_render_system_internal()
{
    return m_render_system;
}

weak_ptr<shader_system> context_impl::get_shader_system_internal()
{
    return m_shader_system;
}

weak_ptr<resource_system> context_impl::get_resource_system_internal()
{
    return m_resource_system;
}

const mango_gl_load_proc& context_impl::get_gl_loading_procedure()
{
    return m_procedure;
}

void context_impl::set_gl_loading_procedure(mango_gl_load_proc procedure)
{
    m_procedure = procedure;
}

bool context_impl::create()
{
    bool success = true;
#if defined(WIN32)
    m_window_system = std::make_shared<win32_window_system>(shared_from_this());
#elif defined(LINUX)
    m_window_system = std::make_shared<linux_window_system>(shared_from_this());
#endif
    success = success && m_window_system->create();

    m_render_system = std::make_shared<render_system_impl>(shared_from_this());
    success         = success && m_render_system->create();

    m_resource_system = std::make_shared<resource_system>(shared_from_this());
    success           = success && m_resource_system->create();

    m_shader_system = std::make_shared<shader_system>(shared_from_this());
    success         = success && m_shader_system->create();

    return success;
}

void context_impl::make_current()
{
    m_window_system->make_window_context_current();
}

void context_impl::destroy()
{
    MANGO_ASSERT(m_shader_system, "Shader System is invalid!");
    m_shader_system->destroy();
    MANGO_ASSERT(m_resource_system, "Resource System is invalid!");
    m_resource_system->destroy();
    MANGO_ASSERT(m_render_system, "Render System is invalid!");
    m_render_system->destroy();
    MANGO_ASSERT(m_window_system, "Window System is invalid!");
    m_window_system->destroy();
}
