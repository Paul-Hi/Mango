//! \file      display_event_handler.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_DISPLAY_EVENT_HANDLER_HPP
#define MANGO_DISPLAY_EVENT_HANDLER_HPP

#include <mango/input_codes.hpp>
#include <mango/types.hpp>

namespace mango
{
    //! \brief Interface for all handlers forwarding display events somewhere else.
    class display_event_handler
    {
      public:
        virtual ~display_event_handler() = default;
        //
        // Window callbacks.
        //

        //! \brief Forwards window position change.
        //! \param[in] x_position The new upper-left corner x position in screen coordinates.
        //! \param[in] y_position he new upper-left corner y position in screen coordinates.
        virtual void on_window_position(int32 x_position, int32 y_position) = 0;

        //! \brief Forwards window resize event.
        //! \param[in] width The new width of the window in screen coordinates.
        //! \param[in] height The new height of the window in screen coordinates.
        virtual void on_window_resize(int32 width, int32 height) = 0;

        //! \brief Forwards window close event.
        virtual void on_window_close() = 0;

        //! \brief Forwards window refresh event.
        virtual void on_window_refresh() = 0;

        //! \brief Forwards window focus change.
        //! \param[in] focused True if the window was given input focus, or false if it lost it.
        virtual void on_window_focus(bool focused) = 0;

        //! \brief Forwards window iconify event.
        //! \param[in] iconified True if the window was iconified, or false if it was restored.
        virtual void on_window_iconify(bool iconified) = 0;

        //! \brief Forwards window maximize event.
        //! \param[in] maximized True if the window was maximized, or false if it was restored.
        virtual void on_window_maximize(bool maximized) = 0;

        //! \brief Forwards window framebuffer resize event.
        //! \param[in] width The new width of the window framebuffer in pixels.
        //! \param[in] height The new height of the window framebuffer in pixels.
        virtual void on_window_framebuffer_resize(int32 width, int32 height) = 0;

        //! \brief Forwards window content scale change.
        //! \param[in] x_scale The new x-axis content scale of the window.
        //! \param[in] y_scale The new y-axis content scale of the window.
        virtual void on_window_content_scale(float x_scale, float y_scale) = 0;

        //
        // Input callbacks.
        //
        //! \brief Forwards mouse button events.
        //! \param[in] button The \a mouse_button that was pressed or released.
        //! \param[in] action Can be input_action::press or input_action::release.
        //! \param[in] mods Bit field describing which modifier keys were held down.
        virtual void on_input_mouse_button(mouse_button button, input_action action, modifier mods) = 0;

        //! \brief Forwards cursor position changes.
        //! \param[in] x_position The new cursor x-coordinate, relative to the left edge of the content area.
        //! \param[in] y_position The new cursor y-coordinate, relative to the top edge of the content area.
        virtual void on_input_cursor_position(double x_position, double y_position) = 0;

        //! \brief Forwards cursor enter events.
        //! \param[in] entered True if the cursor entered the window's content area, false if it left it.
        virtual void on_input_cursor_enter(bool entered) = 0;

        //! \brief Forwards scroll events.
        //! \param[in] x_offset The scroll offset along the x-axis.
        //! \param[in] y_offset The scroll offset along the y-axis.
        virtual void on_input_scroll(double x_offset, double y_offset) = 0;

        //! \brief Forwards key input events.
        //! \param[in] key The \a key_code that was pressed or released.
        //! \param[in] action Can be input_action::press or input_action::release.
        //! \param[in] mods Bit field describing which modifier keys were held down.
        virtual void on_input_key(key_code key, input_action action, modifier mods) = 0;

        //! \brief Forwards drop events.
        //! \param[in] path_count The number of paths.
        //! \param[in] paths The file and/or directory path names.
        virtual void on_input_drop(int32 path_count, const char** paths) = 0;

        // Missing
        // Monitor dis-/connection
        // Joystick dis-/connection
    };

    //! \brief A shared pointer holding a \a display_event_handler.
    using display_event_handler_ptr = std::shared_ptr<display_event_handler>;
}; // namespace mango

#endif // MANGO_DISPLAY_EVENT_HANDLER_HPP