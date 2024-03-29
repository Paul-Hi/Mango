//! \file      slotmap_test.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2022
//! \copyright Apache License 2.0

#include <gtest/gtest.h>
#include <mango/slotmap.hpp>

//! \cond NO_DOC

namespace mango
{
    class slotmap_test : public ::testing::Test
    {
      protected:
        slotmap_test() {}

        ~slotmap_test() override {}

        void SetUp() override {}

        void TearDown() override {}
    };

    using namespace mango;

    TEST_F(slotmap_test, can_insert_access_erase)
    {
        slotmap<string> string_list;
        ASSERT_EQ(string_list.size(), 0);

        string test_string = "Hello World";

        key test_key = string_list.insert(test_string);

        ASSERT_EQ(string_list.size(), 1);

        ASSERT_TRUE(string_list.valid(test_key));

        string& accessed = string_list[test_key];

        ASSERT_EQ(test_string.compare(accessed), 0);

        string_list.erase(test_key);
        ASSERT_EQ(string_list.size(), 0);
        ASSERT_FALSE(string_list.valid(test_key));
    }
} // namespace mango

//! \endcond
