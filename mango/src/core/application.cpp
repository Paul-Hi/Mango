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
    return 0;
}
