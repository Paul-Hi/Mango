//! \file      application.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <core/context_impl.hpp>
#include <mango/application.hpp>

using namespace mango;

application::application()
{
    m_context = make_shared<context_impl>();
}

uint32 application::run(uint32 t_argc, char** t_argv)
{
    MANGO_UNUSED(t_argc);
    MANGO_UNUSED(t_argv);

    return 0;
}

weak_ptr<context> application::get_context()
{
    return std::static_pointer_cast<context>(m_context);
}
