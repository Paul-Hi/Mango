//! \file      texture.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <graphics/texture.hpp>
#include <graphics/impl/texture_impl.hpp>
#include <mango/profile.hpp>

using namespace mango;

texture_ptr texture::create(const texture_configuration& configuration)
{
    PROFILE_ZONE;
    return std::static_pointer_cast<texture>(std::make_shared<texture_impl>(configuration));
}
