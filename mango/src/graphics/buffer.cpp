//! \file      buffer.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <graphics/buffer.hpp>
#include <graphics/impl/buffer_impl.hpp>
#include <mango/profile.hpp>

using namespace mango;

buffer_ptr buffer::create(const buffer_configuration& configuration)
{
    PROFILE_ZONE;
    return std::static_pointer_cast<buffer>(std::make_shared<buffer_impl>(configuration));
}
