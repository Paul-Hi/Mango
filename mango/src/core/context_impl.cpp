//! \file      context_impl.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <core/context_impl.hpp>
#include <mango/application.hpp>
#include <mango/assert.hpp>

using namespace mango;

context_impl::context_impl() {}

context_impl::~context_impl() {}

void context_impl::set_application(const shared_ptr<application>& application)
{
    if(m_application)
    {
        m_application->destroy();
        MANGO_LOG_INFO("Destroying the current application '{0}'.", m_application->get_name());
    }
    m_application = application;
    MANGO_LOG_INFO("Setting the application to '{0}'.", m_application->get_name());
    MANGO_ASSERT(m_application->create(), "Creation of application '{0}' failed!", m_application->get_name());
}
