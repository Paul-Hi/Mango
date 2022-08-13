//! \file      intersect_test.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2022
//! \copyright Apache License 2.0

#include "mock_classes.hpp"
#include <gtest/gtest.h>
#include <mango/intersect.hpp>

//! \cond NO_DOC

namespace mango
{
    TEST(intersect_test, sphere_sphere_intersection_works)
    {
        bounding_sphere s1, s2;

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

    TEST(intersect_test, aabb_aabb_intersection_works)
    {
        axis_aligned_bounding_box a1, a2;

        a1.center  = make_vec3(0.0f);
        a1.extents = make_vec3(1.0f);

        a2.center  = vec3(1.0f, 0.0f, 0.0f);
        a2.extents = make_vec3(1.0f);

        ASSERT_TRUE(a1.intersects(a2));

        a1.center  = vec3(0.0f, -1.0f, 0.0f);
        a1.extents = make_vec3(1.0f);

        a2.center  = vec3(0.0f, 1.0f, 0.0f);
        a2.extents = make_vec3(1.0f);

        ASSERT_TRUE(a1.intersects(a2));

        a1.center  = vec3(0.0f, -1.0f, 0.0f);
        a1.extents = vec3(0.5f, 1.0f, 0.5f);

        a2.center  = vec3(0.0f, 1.0f, 0.0f);
        a2.extents = vec3(0.5f, 1.5f, 0.5f);

        ASSERT_TRUE(a1.intersects(a2));

        a1.center  = vec3(0.0f, 0.0f, 2.0f);
        a1.extents = make_vec3(1.0f);

        a2.center  = vec3(0.0f, 0.0f, -2.0f);
        a2.extents = make_vec3(1.0f);

        ASSERT_FALSE(a1.intersects(a2));
    }

    TEST(intersect_test, frustum_sphere_intersection_works)
    {
        // lookAt(vec3(0.0f), vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, 1.0f, 0.0f))
        mat4 p = mango::perspective(mango::deg_to_rad(45.0f), 16.0f / 9.0f, 0.1f, 10.0f);
        bounding_frustum f(mat4::Identity(), p);

        bounding_sphere s;

        s.center = make_vec3(0.0f);
        s.radius = 1.0f;

        ASSERT_TRUE(f.intersects(s));
        ASSERT_TRUE(s.intersects(f));

        s.center = vec3(0.0f, 1.0f, -5.0f);
        s.radius = 2.0f;

        ASSERT_TRUE(f.intersects(s));
        ASSERT_TRUE(s.intersects(f));

        s.center = vec3(-1.0f, 1.0f, -8.0f);
        s.radius = 6.0f;

        ASSERT_TRUE(f.intersects(s));
        ASSERT_TRUE(s.intersects(f));

        s.center = vec3(0.0f, 0.0f, 2.0f);
        s.radius = 1.0f;

        ASSERT_FALSE(f.intersects(s));
        ASSERT_FALSE(s.intersects(f));

        s.center = vec3(0.0f, 0.0f, -16.0f);
        s.radius = 1.0f;

        ASSERT_FALSE(f.intersects(s));
        ASSERT_FALSE(s.intersects(f));

        s.center = vec3(10.0f, 0.0f, -1.0f);
        s.radius = 2.0f;

        ASSERT_FALSE(f.intersects(s));
        ASSERT_FALSE(s.intersects(f));
    }

    TEST(intersect_test, frustum_aabb_intersection_works)
    {
        bounding_frustum f(mango::lookAt(make_vec3(0.0f), vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, 1.0f, 0.0f)), mango::perspective(mango::deg_to_rad(45.0f), 16.0f / 9.0f, 0.1f, 10.0f));

        axis_aligned_bounding_box a;

        a.center  = make_vec3(0.0f);
        a.extents = make_vec3(1.0f);

        ASSERT_TRUE(f.intersects(a));
        ASSERT_TRUE(a.intersects(f));

        a.center  = vec3(0.0f, 1.0f, -5.0f);
        a.extents = vec3(0.5f, 2.0f, 2.0f);

        ASSERT_TRUE(f.intersects(a));
        ASSERT_TRUE(a.intersects(f));

        a.center  = vec3(-1.0f, 1.0f, -8.0f);
        a.extents = vec3(1.0f, 6.0f, 2.0f);

        ASSERT_TRUE(f.intersects(a));
        ASSERT_TRUE(a.intersects(f));

        a.center  = vec3(0.0f, 0.0f, 2.0f);
        a.extents = make_vec3(1.0f);

        ASSERT_FALSE(f.intersects(a));
        ASSERT_FALSE(a.intersects(f));

        a.center  = vec3(0.0f, 0.0f, -16.0f);
        a.extents = vec3(3.0f, 7.0f, 1.0f);

        ASSERT_FALSE(f.intersects(a));
        ASSERT_FALSE(a.intersects(f));

        a.center  = vec3(10.0f, 0.0f, -1.0f);
        a.extents = vec3(1.0f, 3.0f, 1.0f);

        ASSERT_FALSE(f.intersects(a));
        ASSERT_FALSE(a.intersects(f));
    }
} // namespace mango

//! \endcond
