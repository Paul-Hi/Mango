//! \file      mock_classes.hpp
//! The classes in this file are all used to fake classes for testing.
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#include <core/context_impl.hpp>
#include <core/input_impl.hpp>
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
/*
//! \brief A fake context_impl.
class fake_context : public mango::context_impl
{
  public:
    //! \cond NO_DOC
    MOCK_METHOD(void, set_application, (const mango::shared_ptr<mango::application>& application));
    MOCK_METHOD(mango::weak_ptr<mango::window_system>, get_window_system, ());
    MOCK_METHOD(mango::weak_ptr<mango::input_system>, get_input_system, ());
    MOCK_METHOD(mango::weak_ptr<mango::renderer>, get_renderer, ());
    MOCK_METHOD(bool, create, ());
    MOCK_METHOD(mango::weak_ptr<mango::window_system_impl>, get_window_system_internal, (), (override));
    MOCK_METHOD(mango::weak_ptr<mango::input_system_impl>, get_input_system_internal, (), (override));
    MOCK_METHOD(mango::weak_ptr<mango::renderer_impl>, get_renderer_internal, (), (override));
    MOCK_METHOD(mango::weak_ptr<mango::shader_system>, get_shader_system_internal, (), (override));
    MOCK_METHOD(mango::weak_ptr<mango::resources>, get_resources_internal, (), (override));
    MOCK_METHOD(const mango::mango_gl_load_proc&, get_gl_loading_procedure, ());
    MOCK_METHOD(void, set_gl_loading_procedure, (mango::mango_gl_load_proc procedure));
    MOCK_METHOD(void, make_current, ());
    MOCK_METHOD(void, destroy, ());
    //! \endcond
};

//! \brief A fake input.
class fake_input : public mango::input_system_impl
{
  public:
    //! \cond NO_DOC
    MOCK_METHOD(bool, create, (), (override));
    MOCK_METHOD(void, set_platform_data, (const mango::shared_ptr<mango::platform_data>& data), (override));
    MOCK_METHOD(void, update, (float dt), (override));
    MOCK_METHOD(void, destroy, (), (override));
    MOCK_METHOD(mango::input_action, get_key, (mango::key_code key), (override));
    MOCK_METHOD(mango::input_action, get_mouse_button, (mango::mouse_button button), (override));
    MOCK_METHOD(mango::modifier, get_modifiers, (), (override));
    MOCK_METHOD(vec2, get_mouse_position, (), (override));
    MOCK_METHOD(vec2, get_mouse_scroll, (), (override));
    MOCK_METHOD(void, set_key_callback, (mango::key_callback callback), (override));
    MOCK_METHOD(void, set_mouse_button_callback, (mango::mouse_button_callback callback), (override));
    MOCK_METHOD(void, set_mouse_position_callback, (mango::mouse_position_callback callback), (override));
    MOCK_METHOD(void, set_mouse_scroll_callback, (mango::mouse_scroll_callback callback), (override));
    MOCK_METHOD(void, set_drag_and_drop_callback, (mango::drag_n_drop_callback callback), (override));
    MOCK_METHOD(void, hide_cursor, (bool hide), (override));
    //! \endcond
};
*/