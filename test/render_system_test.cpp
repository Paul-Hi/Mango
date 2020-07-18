//! \file      render_system_test.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#if 0

#include "mock_classes.hpp"
#include <gtest/gtest.h>
#include <mango/mango.hpp>
#include <rendering/render_system_impl.hpp>

//! \cond NO_DOC

class render_system_test : public ::testing::Test
{
  protected:
    render_system_test() {}

    ~render_system_test() override {}

    void SetUp() override {}

    void TearDown() override {}
    mango::shared_ptr<mango::render_system_impl> m_render_system;
};

TEST_F(render_system_test, render_system_no_failure_on_function_calls)
{
    auto c = std::make_shared<fake_context>();
    ASSERT_NO_FATAL_FAILURE(m_render_system = std::make_shared<mango::render_system_impl>(c));
    ASSERT_NE(nullptr, m_render_system);
    // The following can not be tested at the moment, because of the dependencies with window and context and the glfw proc adress is missing.
    // ASSERT_TRUE(m_render_system->create());
    // mango::render_configuration render_config(mango::render_pipeline::deferred_pbr, true);
    // ASSERT_NO_FATAL_FAILURE(m_render_system->configure(render_config));
    // ASSERT_NO_FATAL_FAILURE(m_render_system->update(0.0f));
    // ASSERT_EQ(m_render_system->get_base_render_pipeline(), mango::render_pipeline::deferred_pbr);
    // ASSERT_NO_FATAL_FAILURE(m_render_system->begin_render());
    // ASSERT_NO_FATAL_FAILURE(m_render_system->finish_render());
    // ASSERT_NO_FATAL_FAILURE(m_render_system->destroy());
}

//! \endcond

#endif
