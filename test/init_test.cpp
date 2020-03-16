//! \file      init_test.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <gtest/gtest.h>
#include <mango/mango.hpp>
#include "mock_classes.hpp"

//! \cond NO_DOC

class init_test : public ::testing::Test
{
  protected:
    init_test() {}

    ~init_test() override {}

    void SetUp() override {}

    void TearDown() override {}
    mango::shared_ptr<fake_application> m_application;
};

TEST_F(init_test, initializationDoesNotCrashAnContextIsCreated)
{
    ASSERT_NO_FATAL_FAILURE(m_application = mango::make_shared<fake_application>());
    ASSERT_NE(nullptr, m_application->get_context().lock());
}

//! \endcond
