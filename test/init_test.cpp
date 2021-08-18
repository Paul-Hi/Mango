//! \file      init_test.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#include "mock_classes.hpp"
#include <core/context_impl.hpp>
#include <gtest/gtest.h>
#include <mango/mango.hpp>

//! \cond NO_DOC

namespace mango
{
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
        EXPECT_CALL(*m_application, create()).WillOnce(Return(true));
        auto mango_context = m_application->m_context;
        mango_context->set_application(m_application);

        ASSERT_NE(nullptr, mango_context);
        ASSERT_NE(nullptr, mango_context->get_application());
        ASSERT_NE(nullptr, mango_context->get_input());
        ASSERT_NE(nullptr, mango_context->get_internal_resources());
    }

    // TODO Paul: Extend when we have more mocks.
} // namespace mango
//! \endcond
