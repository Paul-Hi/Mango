//! \file      init_test.cpp
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

TEST_F(init_test, init_does_not_fail_on_context_creation)
{
    ASSERT_NO_FATAL_FAILURE(m_application = std::make_shared<fake_application>());
    auto mango_context = m_application->get_context().lock();
    ASSERT_NE(nullptr, mango_context);
    ASSERT_NE(nullptr, mango_context->get_window_system().lock());
}

TEST_F(init_test, no_crash_on_public_function_calls)
{
    ASSERT_NO_FATAL_FAILURE(m_application = std::make_shared<fake_application>());
    auto mango_context = m_application->get_context().lock();
    auto mango_ws = mango_context->get_window_system().lock();
    mango::window_configuration window_config(100, 100, "Test");
    ASSERT_NO_FATAL_FAILURE(mango_ws->configure(window_config));
    ASSERT_NO_FATAL_FAILURE(mango_ws->update(0.0f));
    ASSERT_NO_FATAL_FAILURE(mango_ws->destroy());
}

//! \endcond
