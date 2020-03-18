//! \file      context_impl.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <core/context_impl.hpp>
#include <mango/application.hpp>
#include <mango/assert.hpp>
#if defined(WIN32)
#include <core/win32_window_system.hpp>
#elif defined(LINUX)
#include <core/linux_window_system.hpp>
#endif

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
    MANGO_ASSERT(m_application->create(), "Creation of application '{0}' failed!", m_application->get_name());
}

weak_ptr<window_system> context_impl::get_window_system()
{
    return m_window_system;
}

weak_ptr<window_system_impl> context_impl::get_window_system_internal()
{
    return m_window_system;
}

bool context_impl::create()
{
#if defined(WIN32)
    m_window_system = make_shared<win32_window_system>();
#elif defined(LINUX)
    m_window_system = make_shared<linux_window_system>();
#endif
    m_window_system->create();
    return true;
}
