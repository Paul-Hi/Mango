//! \file      graphics.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_GRAPHICS_HPP
#define MANGO_GRAPHICS_HPP

#include <core/display_impl.hpp>
#include <graphics/graphics_device.hpp>
#include <graphics/graphics_device_context.hpp>
#include <graphics/graphics_resources.hpp>
#include <graphics/graphics_types.hpp>

namespace mango
{
    namespace graphics
    {
        //! \brief Creates a \a graphics_device and returns the handle.
        //! \param[in] display_window_handle The handle of the platform window used to create the graphics api.
        //! \return The handle to the created \a graphics_device.
        graphics_device_handle create_graphics_device(display_impl::native_window_handle display_window_handle);

        //! \brief Retrieves the necessary image formats for given image data.
        //! \param[in] components The number of components in the image.
        //! \param[in] bits The number of bits per component.
        //! \param[in] srgb True if the image is in standard color space, else false.
        //! \param[in] is_hdr True if the image has high dynamic range, else false.
        //! \param[out] internal The \a gfx_format to use internally.
        //! \param[out] pixel_format The \a gfx_format describing the pixel format.
        //! \param[out] component_type The \a gfx_format for the component type.
        inline void get_formats_for_image(int32 components, int32 bits, bool srgb, bool is_hdr, gfx_format& internal, gfx_format& pixel_format, gfx_format& component_type)
        {
            // TODO Paul: Check for correctness.
            component_type = (bits == 32) ? gfx_format::t_float : gfx_format::t_half_float;

            if (is_hdr)
            {
                internal     = (bits == 32) ? gfx_format::rgb32f : gfx_format::rgb16f;
                pixel_format = gfx_format::rgb;

                if (components == 4)
                {
                    internal     = (bits == 32) ? gfx_format::rgba32f : gfx_format::rgba16f;
                    pixel_format = gfx_format::rgba;
                }
                return;
            }

            component_type = (bits == 32) ? gfx_format::t_unsigned_int : (bits == 16) ? gfx_format::t_unsigned_short : gfx_format::t_unsigned_byte;

            if (srgb)
            {
                internal     = (components == 3) ? gfx_format::srgb8 : gfx_format::srgb8_alpha8;
                pixel_format = (components == 3) ? gfx_format::rgb : gfx_format::rgba;
                return;
            }

            // TODO Paul: 32 Bit is only for integer textures ...

            if (components == 1)
            {
                internal     = (bits == 32) ? gfx_format::r32ui : (bits == 16) ? gfx_format::r16 : gfx_format::r8;
                pixel_format = gfx_format::red;
                return;
            }
            if (components == 2)
            {
                internal     = (bits == 32) ? gfx_format::rg32ui : (bits == 16) ? gfx_format::rg16 : gfx_format::rg8;
                pixel_format = gfx_format::rg;
                return;
            }
            if (components == 3)
            {
                internal     = (bits == 32) ? gfx_format::rgb32ui : (bits == 16) ? gfx_format::rgb16 : gfx_format::rgb8;
                pixel_format = gfx_format::rgb;
                return;
            }
            if (components == 4)
            {
                internal     = (bits == 32) ? gfx_format::rgba32ui : (bits == 16) ? gfx_format::rgba16 : gfx_format::rgba8;
                pixel_format = gfx_format::rgba;
                return;
            }
        }

        //! \brief Calculates the number of mipmap images for a given images size.
        //! \param[in] width The width of the image. Has to be a positive value.
        //! \param[in] height The height of the image. Has to be a positive value.
        //! \return The number of mipmaps that need to be generated.
        inline int32 calculate_mip_count(int32 width, int32 height)
        {
            int32 levels = 1;
            while ((width | height) >> levels)
            {
                ++levels;
            }
            return levels;
        }

        //! \brief Creates an attribute format from component type and count.
        //! \param[in] component_format The format for each component.
        //! \param[in] number_of_components The number of components.
        //! \return The fitting attribute format.
        inline gfx_format get_attribute_format_for_component_info(const gfx_format& component_format, int32 number_of_components)
        {
            switch (component_format)
            {
            case gfx_format::t_byte:
                if (number_of_components == 1)
                    return gfx_format::r8i;
                if (number_of_components == 2)
                    return gfx_format::rg8i;
                if (number_of_components == 3)
                    return gfx_format::rgb8i;
                if (number_of_components == 4)
                    return gfx_format::rgba8i;
                break;
            case gfx_format::t_unsigned_byte:
                if (number_of_components == 1)
                    return gfx_format::r8ui;
                if (number_of_components == 2)
                    return gfx_format::rg8ui;
                if (number_of_components == 3)
                    return gfx_format::rgb8ui;
                if (number_of_components == 4)
                    return gfx_format::rgba8ui;
                break;
            case gfx_format::t_short:
                if (number_of_components == 1)
                    return gfx_format::r16i;
                if (number_of_components == 2)
                    return gfx_format::rg16i;
                if (number_of_components == 3)
                    return gfx_format::rgb16i;
                if (number_of_components == 4)
                    return gfx_format::rgba16i;
                break;
            case gfx_format::t_unsigned_short:
                if (number_of_components == 1)
                    return gfx_format::r16ui;
                if (number_of_components == 2)
                    return gfx_format::rg16ui;
                if (number_of_components == 3)
                    return gfx_format::rgb16ui;
                if (number_of_components == 4)
                    return gfx_format::rgba16ui;
                break;
            case gfx_format::t_int:
                if (number_of_components == 1)
                    return gfx_format::r32i;
                if (number_of_components == 2)
                    return gfx_format::rg32i;
                if (number_of_components == 3)
                    return gfx_format::rgb32i;
                if (number_of_components == 4)
                    return gfx_format::rgba32i;
                break;
            case gfx_format::t_unsigned_int:
                if (number_of_components == 1)
                    return gfx_format::r32ui;
                if (number_of_components == 2)
                    return gfx_format::rg32ui;
                if (number_of_components == 3)
                    return gfx_format::rgb32ui;
                if (number_of_components == 4)
                    return gfx_format::rgba32ui;
                break;
            case gfx_format::t_half_float:
                if (number_of_components == 1)
                    return gfx_format::r16f;
                if (number_of_components == 2)
                    return gfx_format::rg16f;
                if (number_of_components == 3)
                    return gfx_format::rgb16f;
                if (number_of_components == 4)
                    return gfx_format::rgba16f;
                break;
            case gfx_format::t_float:
                if (number_of_components == 1)
                    return gfx_format::r32f;
                if (number_of_components == 2)
                    return gfx_format::rg32f;
                if (number_of_components == 3)
                    return gfx_format::rgb32f;
                if (number_of_components == 4)
                    return gfx_format::rgba32f;
                break;
            default:
                MANGO_ASSERT(false, "Invalid component format! Could also be, that I did not think of adding this here!");
                return gfx_format::invalid;
            }
            MANGO_ASSERT(false, "Invalid component count! Could also be, that I did not think of adding this here!");
            return gfx_format::invalid;
        }

    } // namespace graphics
} // namespace mango

#endif // MANGO_GRAPHICS_HPP
