//! \file      graphics_common_test.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include "mock_classes.hpp"
#include <gtest/gtest.h>
#include <graphics/graphics.hpp>

//! \cond NO_DOC

TEST(graphics_common_test, calculate_mip_count_calculates_correct)
{
    mango::int32 width = 512, height = 512;
    ASSERT_EQ(mango::calculate_mip_count(width, height), 10);

    width = height = 1;
    ASSERT_EQ(mango::calculate_mip_count(width, height), 1);

    width = height = 19;
    ASSERT_EQ(mango::calculate_mip_count(width, height), 5);

    width  = 256;
    height = 64;
    ASSERT_EQ(mango::calculate_mip_count(width, height), 9);
}

//! \endcond
