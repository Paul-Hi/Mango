//! \file      packed_freelist_test.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <gtest/gtest.h>
#include <mango/packed_freelist.hpp>

//! \cond NO_DOC

class packed_freelist_test : public ::testing::Test
{
  protected:
    packed_freelist_test() {}

    ~packed_freelist_test() override {}

    void SetUp() override {}

    void TearDown() override {}
};

using namespace mango;

template <>
uint32 packed_freelist_id::to()
{
    return lookup_id;
}

template <>
void packed_freelist_id::from(uint32& in)
{
    lookup_id = in;
}

TEST_F(packed_freelist_test, can_insert_access_erase)
{
    packed_freelist<string, 32> string_list;
    ASSERT_EQ(string_list.array_capacity(), 32);
    ASSERT_EQ(string_list.size(), 0);

    string test_string = "Hello World";

    uint32 id = string_list.insert(test_string).to<uint32>();

    ASSERT_EQ(string_list.size(), 1);

    packed_freelist_id pf_id;
    pf_id.from<uint32>(id);
    ASSERT_TRUE(string_list.contains(pf_id));

    string& accessed = string_list.at(pf_id);

    ASSERT_EQ(test_string.compare(accessed), 0);

    string_list.erase(pf_id);
    ASSERT_EQ(string_list.size(), 0);
    ASSERT_FALSE(string_list.contains(pf_id));
}

//! \endcond
