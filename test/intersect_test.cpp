//! \file      intersect_test.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#include "mock_classes.hpp"
#include <gtest/gtest.h>
#include <util/intersect.hpp>

//! \cond NO_DOC

namespace mango
{
    TEST(intersect_test, sphere_sphere_intersection_works)
    {
        mango::bounding_sphere s1, s2;

        s1.center = vec3(0.0f, -1.0f, 0.0f);
        s1.radius = 2.0f;

        s2.center = vec3(0.0f, 1.0f, 0.0f);
        s2.radius = 2.0f;

        ASSERT_TRUE(s1.intersects(s2));

        s1.center = vec3(0.0f, -1.0f, 0.0f);
        s1.radius = 1.0f;

        s2.center = vec3(0.0f, 1.0f, 0.0f);
        s2.radius = 1.0f;

        ASSERT_TRUE(s1.intersects(s2));

        s1.center = vec3(0.0f, 0.0f, 2.0f);
        s1.radius = 1.0f;

        s2.center = vec3(0.0f, 0.0f, -2.0f);
        s2.radius = 1.0f;

        ASSERT_FALSE(s1.intersects(s2));
    }

    TEST(intersect_test, sphere_sphere_containment_works)
    {
        mango::bounding_sphere s1, s2;

        s1.center = vec3(0.0f);
        s1.radius = 2.0f;

        s2.center = vec3(0.0f);
        s2.radius = 1.0f;

        ASSERT_EQ(s1.contains(s2), containment_result::contain);

        s1.center = vec3(1.0f, 0.0f, 0.0f);
        s1.radius = 2.0f;

        s2.center = vec3(0.0f);
        s2.radius = 1.0f;

        ASSERT_EQ(s1.contains(s2), containment_result::contain);

        s1.center = vec3(0.0f, -1.0f, 0.0f);
        s1.radius = 2.0f;

        s2.center = vec3(0.0f, 1.0f, 0.0f);
        s2.radius = 2.0f;

        ASSERT_EQ(s1.contains(s2), containment_result::intersect);

        s1.center = vec3(1.0f, 0.0f, 0.0f);
        s1.radius = 1.0f;

        s2.center = vec3(-2.0f, 0.0f, 0.0f);
        s2.radius = 1.0f;

        ASSERT_EQ(s1.contains(s2), containment_result::disjoint);
    }
} // namespace mango

//! \endcond
