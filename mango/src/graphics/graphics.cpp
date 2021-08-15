//! \file      graphics.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#include <graphics/graphics.hpp>
#include <graphics/opengl/gl_graphics_device.hpp>

namespace mango
{
    namespace graphics
    {
        graphics_device_handle create_graphics_device(display_impl::native_window_handle display_window_handle)
        {
            return mango::make_unique<gl_graphics_device>(std::forward<display_impl::native_window_handle>(display_window_handle));
        }
    } // namespace graphics
} // namespace mango
