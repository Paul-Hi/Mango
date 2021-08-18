//! \file      renderer_test.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#if 0

#include "mock_classes.hpp"
#include <gtest/gtest.h>
#include <mango/mango.hpp>
#include <rendering/renderer_impl.hpp>

//! \cond NO_DOC

class renderer_test : public ::testing::Test
{
  protected:
    renderer_test() {}

    ~renderer_test() override {}

    void SetUp() override {}

    void TearDown() override {}
    mango::shared_ptr<mango::renderer_impl> m_renderer;
};

TEST_F(renderer_test, renderer_no_failure_on_function_calls)
{
    auto c = std::make_shared<fake_context>();
    ASSERT_NO_FATAL_FAILURE(m_renderer = std::make_shared<mango::renderer_impl>(c));
    ASSERT_NE(nullptr, m_renderer);
    // The following can not be tested at the moment, because of the dependencies with window and context and the glfw proc adress is missing.
    // ASSERT_TRUE(m_renderer->create());
    // mango::renderer_configuration render_config(mango::render_pipeline::deferred_pbr, true);
    // ASSERT_NO_FATAL_FAILURE(m_renderer->configure(render_config));
    // ASSERT_NO_FATAL_FAILURE(m_renderer->update(0.0f));
    // ASSERT_EQ(m_renderer->get_base_render_pipeline(), mango::render_pipeline::deferred_pbr);
    // ASSERT_NO_FATAL_FAILURE(m_renderer->begin_render());
    // ASSERT_NO_FATAL_FAILURE(m_renderer->finish_render());
    // ASSERT_NO_FATAL_FAILURE(m_renderer->destroy());
}

//! \endcond

#endif
