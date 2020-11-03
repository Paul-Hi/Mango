//! \file      context_impl.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <core/context_impl.hpp>
#if defined(WIN32)
#include <core/win32_input_system.hpp>
#include <core/win32_window_system.hpp>
#elif defined(LINUX)
#include <core/linux_input_system.hpp>
#include <core/linux_window_system.hpp>
#endif
#include <mango/application.hpp>
#include <mango/assert.hpp>
#include <mango/profile.hpp>
#include <mango/scene.hpp>
#include <rendering/render_system_impl.hpp>
#include <resources/resource_system.hpp>
#include <ui/ui_system_impl.hpp>

using namespace mango;

void context_impl::set_application(const shared_ptr<application>& application)
{
    PROFILE_ZONE;
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

weak_ptr<input_system> context_impl::get_input_system()
{
    return m_input_system;
}

weak_ptr<render_system> context_impl::get_render_system()
{
    return m_render_system;
}

weak_ptr<ui_system> context_impl::get_ui_system()
{
    return m_ui_system;
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

shared_ptr<application> context_impl::get_application()
{
    return m_application;
}

weak_ptr<window_system_impl> context_impl::get_window_system_internal()
{
    return m_window_system;
}

weak_ptr<input_system_impl> context_impl::get_input_system_internal()
{
    return m_input_system;
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

weak_ptr<ui_system_impl> context_impl::get_ui_system_internal()
{
    return m_ui_system;
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
    NAMED_PROFILE_ZONE("Context Creation");

    return create_systems();
}

void context_impl::make_current()
{
    m_window_system->make_window_context_current();
}

void context_impl::destroy()
{
    NAMED_PROFILE_ZONE("Context Destruction");
    destroy_systems();
}

bool context_impl::create_systems()
{
    NAMED_PROFILE_ZONE("System Creation");
    bool success = true;
#if defined(WIN32)
    m_window_system = std::make_shared<win32_window_system>(shared_from_this());
    m_input_system  = std::make_shared<win32_input_system>(shared_from_this());
#elif defined(LINUX)
    m_window_system = std::make_shared<linux_window_system>(shared_from_this());
    m_input_system  = std::make_shared<linux_input_system>(shared_from_this());
#endif
    success = success && m_window_system->create();
    success = success && m_input_system->create();

    m_render_system = std::make_shared<render_system_impl>(shared_from_this());
    success         = success && m_render_system->create();

    m_ui_system = std::make_shared<ui_system_impl>(shared_from_this());
    success     = success && m_ui_system->create();

    m_resource_system = std::make_shared<resource_system>(shared_from_this());
    success           = success && m_resource_system->create();
    return success;
}

void context_impl::destroy_systems()
{
    NAMED_PROFILE_ZONE("System Destruction");
    MANGO_ASSERT(m_resource_system, "Resource System is invalid!");
    m_resource_system->destroy();
    MANGO_ASSERT(m_ui_system, "UI System is invalid!");
    m_ui_system->destroy();
    MANGO_ASSERT(m_render_system, "Render System is invalid!");
    m_render_system->destroy();
    MANGO_ASSERT(m_input_system, "Input System is invalid!");
    m_input_system->destroy();
    MANGO_ASSERT(m_window_system, "Window System is invalid!");
    m_window_system->destroy();
}
