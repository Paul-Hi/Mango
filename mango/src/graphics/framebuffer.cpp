//! \file      framebuffer.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <graphics/framebuffer.hpp>
#include <graphics/impl/framebuffer_impl.hpp>
#include <mango/profile.hpp>

using namespace mango;

framebuffer_ptr framebuffer::create(const framebuffer_configuration& configuration)
{
    PROFILE_ZONE;
    return std::static_pointer_cast<framebuffer>(std::make_shared<framebuffer_impl>(configuration));
}
