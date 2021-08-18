//! \file      graphics_test.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#include "mock_classes.hpp"
#include <graphics/graphics.hpp>
#include <gtest/gtest.h>

//! \cond NO_DOC

namespace mango
{
    TEST(graphics_test, calculate_mip_count_calculates_correct_mipcount)
    {
        int32 width = 512, height = 512;
        ASSERT_EQ(graphics::calculate_mip_count(width, height), 10);

        width = height = 1;
        ASSERT_EQ(graphics::calculate_mip_count(width, height), 1);

        width = height = 19;
        ASSERT_EQ(graphics::calculate_mip_count(width, height), 5);

        width  = 256;
        height = 64;
        ASSERT_EQ(graphics::calculate_mip_count(width, height), 9);
    }

    TEST(graphics_test, get_formats_for_image_provides_correct_formats)
    {
        int32 components, bits;
        bool srgb, hdr;

        gfx_format internal, pixel, component_type;

        components = 3;
        bits       = 8;
        srgb       = true;
        hdr        = false;

        graphics::get_formats_for_image(components, bits, srgb, hdr, internal, pixel, component_type);

        ASSERT_EQ(internal, gfx_format::srgb8);
        ASSERT_EQ(pixel, gfx_format::rgb);
        ASSERT_EQ(component_type, gfx_format::t_unsigned_byte);

        components = 4;
        bits       = 32;
        srgb       = false;
        hdr        = true;

        graphics::get_formats_for_image(components, bits, srgb, hdr, internal, pixel, component_type);

        ASSERT_EQ(internal, gfx_format::rgba32f);
        ASSERT_EQ(pixel, gfx_format::rgba);
        ASSERT_EQ(component_type, gfx_format::t_float);

        components = 2;
        bits       = 16;
        srgb       = false;
        hdr        = false;

        graphics::get_formats_for_image(components, bits, srgb, hdr, internal, pixel, component_type);

        ASSERT_EQ(internal, gfx_format::rg16);
        ASSERT_EQ(pixel, gfx_format::rg);
        ASSERT_EQ(component_type, gfx_format::t_unsigned_short);
    }

    TEST(graphics_test, get_attribute_format_for_component_info_provides_correct_format)
    {
        int32 component_count       = 3;
        gfx_format component_format = gfx_format::t_unsigned_byte;

        ASSERT_EQ(graphics::get_attribute_format_for_component_info(component_format, component_count), gfx_format::rgb8ui);

        component_count  = 1;
        component_format = gfx_format::t_short;

        ASSERT_EQ(graphics::get_attribute_format_for_component_info(component_format, component_count), gfx_format::r16i);

        component_count  = 4;
        component_format = gfx_format::t_half_float;

        ASSERT_EQ(graphics::get_attribute_format_for_component_info(component_format, component_count), gfx_format::rgba16f);

        component_count  = 2;
        component_format = gfx_format::t_int;

        ASSERT_EQ(graphics::get_attribute_format_for_component_info(component_format, component_count), gfx_format::rg32i);
    }
} // namespace mango

//! \endcond
