//! \file      input_impl.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#include <core/input_impl.hpp>

using namespace mango;

input_impl::input_impl()
{
    m_current_input_state.keys.fill(input_action::release);
    m_current_input_state.mouse_buttons.fill(input_action::release);
    m_current_input_state.modifier_field  = modifier::none;
    m_current_input_state.cursor_position = dvec3(0.0);
    m_current_input_state.scroll_offset   = dvec3(0.0);

    m_signals.input_key.connect([this](key_code key, input_action action, modifier mods) {
        m_current_input_state.keys[static_cast<int32>(key)] = action;
        m_current_input_state.modifier_field &= mods;
    });

    m_signals.input_mouse_button.connect([this](mouse_button button, input_action action, modifier mods) {
        m_current_input_state.mouse_buttons[static_cast<int32>(button)] = action;
        m_current_input_state.modifier_field &= mods;
    });

    m_signals.input_cursor_position.connect([this](double x_position, double y_position) {
        m_current_input_state.cursor_position.x = x_position;
        m_current_input_state.cursor_position.y = y_position;
    });

    m_signals.input_cursor_position.connect([this](double x_offset, double y_offset) {
        m_current_input_state.scroll_offset.x = x_offset;
        m_current_input_state.scroll_offset.y = y_offset;
    });
}

input_impl::~input_impl() {}

bool input_impl::undo_action(int32 steps)
{
    MANGO_UNUSED(steps);
    MANGO_LOG_INFO("Undo is currently not supported!");
    return false;
}

bool input_impl::redo_action(int32 steps)
{
    MANGO_UNUSED(steps);
    MANGO_LOG_INFO("Redo is currently not supported!");
    return false;
}

input_action input_impl::get_key(key_code key)
{
    return m_current_input_state.keys[static_cast<int32>(key)];
}

input_action input_impl::get_mouse_button(mouse_button button)
{
    return m_current_input_state.mouse_buttons[static_cast<int32>(button)];
}

modifier input_impl::get_modifiers()
{
    return m_current_input_state.modifier_field;
}

dvec2 input_impl::get_cursor_position()
{
    return m_current_input_state.cursor_position;
}

dvec2 input_impl::get_scroll_offset()
{
    return m_current_input_state.scroll_offset;
}

void input_impl::register_display_position_callback(display_position_callback callback)
{
    m_signals.window_position.connect(callback);
}

void input_impl::register_display_resize_callback(display_resize_callback callback)
{
    m_signals.window_resize.connect(callback);
}

void input_impl::register_display_close_callback(display_close_callback callback)
{
    m_signals.window_close.connect(callback);
}

void input_impl::register_display_refresh_callback(display_refresh_callback callback)
{
    m_signals.window_refresh.connect(callback);
}

void input_impl::register_display_focus_callback(display_focus_callback callback)
{
    m_signals.window_focus.connect(callback);
}

void input_impl::register_display_iconify_callback(display_iconify_callback callback)
{
    m_signals.window_iconify.connect(callback);
}

void input_impl::register_display_maximize_callback(display_maximize_callback callback)
{
    m_signals.window_maximize.connect(callback);
}

void input_impl::register_display_framebuffer_resize_callback(display_framebuffer_resize_callback callback)
{
    m_signals.window_framebuffer_resize.connect(callback);
}

void input_impl::register_display_content_scale_callback(display_content_scale_callback callback)
{
    m_signals.window_content_scale.connect(callback);
}

void input_impl::register_mouse_button_callback(mouse_button_callback callback)
{
    m_signals.input_mouse_button.connect(callback);
}

void input_impl::register_cursor_position_callback(cursor_position_callback callback)
{
    m_signals.input_cursor_position.connect(callback);
}

void input_impl::register_cursor_enter_callback(cursor_enter_callback callback)
{
    m_signals.input_cursor_enter.connect(callback);
}

void input_impl::register_scroll_callback(scroll_callback callback)
{
    m_signals.input_scroll.connect(callback);
}

void input_impl::register_key_callback(key_callback callback)
{
    m_signals.input_key.connect(callback);
}

void input_impl::register_drop_callback(drop_callback callback)
{
    m_signals.input_drop.connect(callback);
}

void input_impl::on_window_position(int32 x_position, int32 y_position)
{
    m_signals.window_position(x_position, y_position);
}

void input_impl::on_window_resize(int32 width, int32 height)
{
    m_signals.window_resize(width, height);
}

void input_impl::on_window_close()
{
    m_signals.window_close();
}

void input_impl::on_window_refresh()
{
    m_signals.window_refresh();
}

void input_impl::on_window_focus(bool focused)
{
    m_signals.window_focus(focused);
}

void input_impl::on_window_iconify(bool iconified)
{
    m_signals.window_iconify(iconified);
}

void input_impl::on_window_maximize(bool maximized)
{
    m_signals.window_maximize(maximized);
}

void input_impl::on_window_framebuffer_resize(int32 width, int32 height)
{
    m_signals.window_framebuffer_resize(width, height);
}

void input_impl::on_window_content_scale(float x_scale, float y_scale)
{
    m_signals.window_content_scale(x_scale, y_scale);
}

void input_impl::on_input_mouse_button(mouse_button button, input_action action, modifier mods)
{
    m_signals.input_mouse_button(button, action, mods);
}

void input_impl::on_input_cursor_position(double x_position, double y_position)
{
    m_signals.input_cursor_position(x_position, y_position);
}

void input_impl::on_input_cursor_enter(bool entered)
{
    m_signals.input_cursor_enter(entered);
}

void input_impl::on_input_scroll(double x_offset, double y_offset)
{
    m_signals.input_scroll(x_offset, y_offset);
}

void input_impl::on_input_key(key_code key, input_action action, modifier mods)
{
    m_signals.input_key(key, action, mods);
}

void input_impl::on_input_drop(int32 path_count, const char** paths)
{
    m_signals.input_drop(path_count, paths);
}
