//! \file      input_impl.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_INPUT_IMPL_HPP
#define MANGO_INPUT_IMPL_HPP

#include <array>
#include <mango/input.hpp>
#include <util/helpers.hpp>
#include <util/signal.hpp>

namespace mango
{
    //! \brief The internal input.
    class input_impl : public input
    {
        MANGO_DISABLE_COPY_AND_ASSIGNMENT(input_impl)
      public:
        input_impl();
        ~input_impl();
        bool undo_action(int32 steps = 1) override;
        bool redo_action(int32 steps = 1) override;
        input_action get_key(key_code key) override;
        input_action get_mouse_button(mouse_button button) override;
        modifier get_modifiers() override;
        dvec2 get_cursor_position() override;
        dvec2 get_scroll_offset() override;
        void register_display_position_callback(display_position_callback callback) override;
        void register_display_resize_callback(display_resize_callback callback) override;
        void register_display_close_callback(display_close_callback callback) override;
        void register_display_refresh_callback(display_refresh_callback callback) override;
        void register_display_focus_callback(display_focus_callback callback) override;
        void register_display_iconify_callback(display_iconify_callback callback) override;
        void register_display_maximize_callback(display_maximize_callback callback) override;
        void register_display_framebuffer_resize_callback(display_framebuffer_resize_callback callback) override;
        void register_display_content_scale_callback(display_content_scale_callback callback) override;
        void register_mouse_button_callback(mouse_button_callback callback) override;
        void register_cursor_position_callback(cursor_position_callback callback) override;
        void register_cursor_enter_callback(cursor_enter_callback callback) override;
        void register_scroll_callback(scroll_callback callback) override;
        void register_key_callback(key_callback callback) override;
        void register_drop_callback(drop_callback callback) override;

        //! \brief Signals window position change.
        //! \param[in] x_position The new upper-left corner x position in screen coordinates.
        //! \param[in] y_position he new upper-left corner y position in screen coordinates.
        void on_window_position(int32 x_position, int32 y_position);

        //! \brief Signals window resize event.
        //! \param[in] width The new width of the window in screen coordinates.
        //! \param[in] height The new height of the window in screen coordinates.
        void on_window_resize(int32 width, int32 height);

        //! \brief Signals window close event.
        void on_window_close();

        //! \brief Signals window refresh event.
        void on_window_refresh();

        //! \brief Signals window focus change.
        //! \param[in] focused True if the window was given input focus, or false if it lost it.
        void on_window_focus(bool focused);

        //! \brief Signals window iconify event.
        //! \param[in] iconified True if the window was iconified, or false if it was restored.
        void on_window_iconify(bool iconified);

        //! \brief Signals window maximize event.
        //! \param[in] maximized True if the window was maximized, or false if it was restored.
        void on_window_maximize(bool maximized);

        //! \brief Signals window framebuffer resize event.
        //! \param[in] width The new width of the window framebuffer in pixels.
        //! \param[in] height The new height of the window framebuffer in pixels.
        void on_window_framebuffer_resize(int32 width, int32 height);

        //! \brief Signals window content scale change.
        //! \param[in] x_scale The new x-axis content scale of the window.
        //! \param[in] y_scale The new y-axis content scale of the window.
        void on_window_content_scale(float x_scale, float y_scale);

        //! \brief Signals mouse button events.
        //! \param[in] button The \a mouse_button that was pressed or released.
        //! \param[in] action Can be input_action::press or input_action::release.
        //! \param[in] mods Bit field describing which modifier keys were held down.
        void on_input_mouse_button(mouse_button button, input_action action, modifier mods);

        //! \brief Signals cursor position changes.
        //! \param[in] x_position The new cursor x-coordinate, relative to the left edge of the content area.
        //! \param[in] y_position The new cursor y-coordinate, relative to the top edge of the content area.
        void on_input_cursor_position(double x_position, double y_position);

        //! \brief Signals cursor enter events.
        //! \param[in] entered True if the cursor entered the window's content area, false if it left it.
        void on_input_cursor_enter(bool entered);

        //! \brief Signals scroll events.
        //! \param[in] x_offset The scroll offset along the x-axis.
        //! \param[in] y_offset The scroll offset along the y-axis.
        void on_input_scroll(double x_offset, double y_offset);

        //! \brief Signals key events.
        //! \param[in] key The \a key_code that was pressed or released.
        //! \param[in] action Can be input_action::press or input_action::release.
        //! \param[in] mods Bit field describing which modifier keys were held down.
        void on_input_key(key_code key, input_action action, modifier mods);

        //! \brief Signals drop events.
        //! \param[in] path_count The number of paths.
        //! \param[in] paths The file and/or directory path names.
        void on_input_drop(int32 path_count, const char** paths);

      private:
        //! \brief Structure containing the input state that can be polled directly.
        struct input_state
        {
            //! \brief Mouse button map.
            std::array<input_action, static_cast<size_t>(mouse_button::count)> mouse_buttons;
            //! \brief Keymap.
            std::array<input_action, static_cast<size_t>(key_code::count)> keys;
            //! \brief Active modifiers.
            modifier modifier_field;
            //! \brief Current cursor position.
            dvec2 cursor_position;
            //! \brief Current scroll offset.
            dvec2 scroll_offset;
        };

        //! \brief Current input state.
        input_state m_current_input_state;

        struct
        {
            //! \brief Used \a signal for window position changes.
            signal<int32, int32> window_position;
            //! \brief Used \a signal for window resize.
            signal<int32, int32> window_resize;
            //! \brief Used \a signal for window close.
            signal<> window_close;
            //! \brief Used \a signal for window refresh.
            signal<> window_refresh;
            //! \brief Used \a signal for window focus changes.
            signal<bool> window_focus;
            //! \brief Used \a signal for window iconification.
            signal<bool> window_iconify;
            //! \brief Used \a signal for window maximization.
            signal<bool> window_maximize;
            //! \brief Used \a signal for window framebuffer resize.
            signal<int32, int32> window_framebuffer_resize;
            //! \brief Used \a signal for window content scale.
            signal<float, float> window_content_scale;
            //! \brief Used \a signal for mouse button input.
            signal<mouse_button, input_action, modifier> input_mouse_button;
            //! \brief Used \a signal for cursor movement.
            signal<double, double> input_cursor_position;
            //! \brief Used \a signal for curso enter status changes.
            signal<bool> input_cursor_enter;
            //! \brief Used \a signal for scrolling.
            signal<double, double> input_scroll;
            //! \brief Used \a signal for key input.
            signal<key_code, input_action, modifier> input_key;
            //! \brief Used \a signal for drop events.
            signal<int32, const char**> input_drop;
        } m_signals; //!< Signals used for connecting functions and calling them on events.
    };

} // namespace mango

#endif // MANGO_INPUT_IMPL_HPP
