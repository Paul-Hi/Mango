//! \file      mock_classes.hpp
//! The classes in this file are all used to fake classes for testing.
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <core/context_impl.hpp>
#include <gmock/gmock.h>
#include <mango/mango.hpp>

using ::testing::_;
using ::testing::Return;

//! \brief A fake application.
class fake_application : public mango::application
{
  public:
    //! \cond NO_DOC
    MOCK_METHOD(bool, create, (), (override));
    MOCK_METHOD(void, update, (float dt), (override));
    MOCK_METHOD(void, destroy, (), (override));
    //! \endcond
};

//! \brief A fake context_impl.
class fake_context : public mango::context_impl
{
  public:
    //! \cond NO_DOC
    MOCK_METHOD(void, set_application, (const mango::shared_ptr<mango::application>& application));
    MOCK_METHOD(mango::weak_ptr<mango::window_system>, get_window_system, ());
    MOCK_METHOD(mango::weak_ptr<mango::render_system>, get_render_system, ());
    MOCK_METHOD(bool, create, ());
    MOCK_METHOD(mango::weak_ptr<mango::window_system_impl>, get_window_system_internal, ());
    MOCK_METHOD(mango::weak_ptr<mango::render_system_impl>, get_render_system_internal, ());
    MOCK_METHOD(const mango::mango_gl_load_proc&, get_gl_loading_procedure, ());
    MOCK_METHOD(void, set_gl_loading_procedure, (mango::mango_gl_load_proc procedure));
    MOCK_METHOD(void, destroy, ());
    //! \endcond
};
