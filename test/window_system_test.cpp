//! \file      window_system_test.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#if 0

#include "mock_classes.hpp"
#include <gtest/gtest.h>
#include <mango/mango.hpp>
#if defined(WIN32)
#include <core/win32_window_system.hpp>
#define platform_window_system_impl mango::win32_window_system
#elif defined(LINUX)
#include <core/linux_window_system.hpp>
#define platform_window_system_impl mango::linux_window_system
#endif

//! \cond NO_DOC

class window_system_test : public ::testing::Test
{
  protected:
    window_system_test()
    {
        m_fake_context = std::make_shared<fake_context>();
        m_fake_input   = std::make_shared<fake_input>();
    }

    ~window_system_test() override {}

    void SetUp() override {}

    void TearDown() override {}
    mango::shared_ptr<platform_window_system_impl> m_window_system;
    mango::shared_ptr<fake_context> m_fake_context;
    mango::shared_ptr<fake_input> m_fake_input;
};

TEST_F(window_system_test, platform_window_system_no_failure_on_function_calls)
{
    ASSERT_NO_FATAL_FAILURE(m_window_system = std::make_shared<platform_window_system_impl>(m_fake_context));
    ASSERT_NE(nullptr, m_window_system);
    ASSERT_TRUE(m_window_system->create());
    mango::window_configuration window_config(100, 100, "Test");
    EXPECT_CALL(*m_fake_context, get_input_system_internal()).Times(1).WillOnce(Return(m_fake_input));
    ASSERT_NO_FATAL_FAILURE(m_window_system->configure(window_config));
    ASSERT_NO_FATAL_FAILURE(m_window_system->update(0.0f));
    ASSERT_NO_FATAL_FAILURE(m_window_system->swap_buffers());
    ASSERT_NO_FATAL_FAILURE(m_window_system->poll_events());
    ASSERT_FALSE(m_window_system->should_close());
    ASSERT_NO_FATAL_FAILURE(m_window_system->destroy());
}

//! \endcond

#endif
