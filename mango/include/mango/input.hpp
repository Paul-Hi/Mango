//! \file      input.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_INPUT_HPP
#define MANGO_INPUT_HPP

#include <mango/input_codes.hpp>
#include <mango/types.hpp>

namespace mango
{
    //! \brief The interface for mangos input.
    //! \details Can be used to poll event states and to submit callbacks.
    class input
    {
      public:
        virtual ~input() = default;

        //! \brief Tries to undo the last actions.
        //! \return True on sucess, else false.
        virtual bool undo_action(int32 steps = 1) = 0;

        //! \brief Tries to redo undone actions.
        //! \return True on sucess, else false.
        virtual bool redo_action(int32 steps = 1) = 0;

        //! \brief Retrieves the current state of a specific key.
        //! \param[in] key The \a key_code of the key to query the state for.
        //! \return The \a input_action representing the state of the key.
        virtual input_action get_key(key_code key) = 0;

        //! \brief Retrieves the current state of a specific mouse button.
        //! \param[in] button The \a mouse_button to query the state for.
        //! \return The \a input_action representing the state of the mouse button.
        virtual input_action get_mouse_button(mouse_button button) = 0;

        //! \brief Retrieves the currently activated modifiers.
        //! \details These are special keys that are relevant if pressed at the same time with other actions.
        //! \return The \a modifier including all active modifiers.
        virtual modifier get_modifiers() = 0;

        //! \brief Retrieves the current cursor position.
        //! \return The current position of the cursor.
        virtual dvec2 get_cursor_position() = 0;

        //! \brief Retrieves the current scroll offsets.
        //! \return The scroll offset in x and y direction.
        virtual dvec2 get_scroll_offset() = 0;

        //
        // Callback connection.
        //

        //! \brief Registers a callback function getting called on \a display position change.
        //! \param[in] callback Function to callback.
        virtual void register_display_position_callback(display_position_callback callback) = 0;

        //! \brief Registers a callback function getting called on \a display resize.
        //! \param[in] callback Function to callback.
        virtual void register_display_resize_callback(display_resize_callback callback) = 0;

        //! \brief Registers a callback function getting called on \a display close.
        //! \param[in] callback Function to callback.
        virtual void register_display_close_callback(display_close_callback callback) = 0;

        //! \brief Registers a callback function getting called on \a display refresh.
        //! \param[in] callback Function to callback.
        virtual void register_display_refresh_callback(display_refresh_callback callback) = 0;

        //! \brief Registers a callback function getting called on \a display focus change.
        //! \param[in] callback Function to callback.
        virtual void register_display_focus_callback(display_focus_callback callback) = 0;

        //! \brief Registers a callback function getting called on \a display iconification change.
        //! \param[in] callback Function to callback.
        virtual void register_display_iconify_callback(display_iconify_callback callback) = 0;

        //! \brief Registers a callback function getting called on \a display maximization change.
        //! \param[in] callback Function to callback.
        virtual void register_display_maximize_callback(display_maximize_callback callback) = 0;

        //! \brief Registers a callback function getting called on \a display framebuffer resize.
        //! \param[in] callback Function to callback.
        virtual void register_display_framebuffer_resize_callback(display_framebuffer_resize_callback callback) = 0;

        //! \brief Registers a callback function getting called on \a display content scale change.
        //! \param[in] callback Function to callback.
        virtual void register_display_content_scale_callback(display_content_scale_callback callback) = 0;

        //! \brief Registers a callback function getting called on mouse button events.
        //! \param[in] callback Function to callback.
        virtual void register_mouse_button_callback(mouse_button_callback callback) = 0;

        //! \brief Registers a callback function getting called on cursor position change.
        //! \param[in] callback Function to callback.
        virtual void register_cursor_position_callback(cursor_position_callback callback) = 0;

        //! \brief Registers a callback function getting called on cursor entering or exiting the \a display.
        //! \param[in] callback Function to callback.
        virtual void register_cursor_enter_callback(cursor_enter_callback callback) = 0;

        //! \brief Registers a callback function getting called on scroll events.
        //! \param[in] callback Function to callback.
        virtual void register_scroll_callback(scroll_callback callback) = 0;

        //! \brief Registers a callback function getting called on key events.
        //! \param[in] callback Function to callback.
        virtual void register_key_callback(key_callback callback) = 0;

        //! \brief Registers a callback function getting called on drop events.
        //! \param[in] callback Function to callback.
        virtual void register_drop_callback(drop_callback callback) = 0;
    };

    //! \brief A unique pointer holding the \a input.
    using input_ptr    = std::unique_ptr<input>;

    //! \brief A pointer pointing to the \a input.
    using input_handle = input*;

} // namespace mango

#endif // MANGO_INPUT_HPP
