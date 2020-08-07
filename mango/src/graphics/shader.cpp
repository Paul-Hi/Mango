//! \file      shader.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <graphics/shader.hpp>
#include <graphics/impl/shader_impl.hpp>
#include <mango/profile.hpp>

using namespace mango;

shader_ptr shader::create(const shader_configuration& configuration)
{
    PROFILE_ZONE;
    return std::static_pointer_cast<shader>(std::make_shared<shader_impl>(configuration));
}
