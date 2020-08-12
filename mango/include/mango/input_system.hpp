//! \file      input_system.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_INPUT_SYSTEM_HPP
#define MANGO_INPUT_SYSTEM_HPP

#include <mango/input_codes.hpp>
#include <mango/system.hpp>

namespace mango
{
    //! \brief A system for input handling.
    //! \details The \a input_system manages input and calls input callbacks.
    class input_system : public system
    {
      public:
        //! \brief Retrieves the current state of a specific key.
        //! \param[in] key The \a key_code of the key to query the state for.
        //! \returns The \a input_action representing the state of the key.
        virtual input_action get_key(key_code key) = 0;
        //! \brief Retrieves the current state of a specific mouse button.
        //! \param[in] button The \a mouse_button to query the state for.
        //! \returns The \a input_action representing the state of the mouse button.
        virtual input_action get_mouse_button(mouse_button button) = 0;
        //! \brief Retrieves the currently activated modifiers.
        //! \details These are special keys that are relevant if pressed at the same time.
        //! \returns The \a modifier including all active modifiers.
        virtual modifier get_modifiers() = 0;
        //! \brief Retrieves the current mouse cursor position.
        //! \returns The current position of the mouse cursor.
        virtual glm::vec2 get_mouse_position() = 0;
        //! \brief Retrieves the current mouse scroll offsets.
        //! \returns The mouse scroll offset in x and y direction.
        virtual glm::vec2 get_mouse_scroll() = 0;

        //! \brief Sets the callback function for key events.
        //! \param[in] callback The callback to use for key events.
        virtual void set_key_callback(key_callback callback) = 0;
        //! \brief Sets the callback function for mouse button events.
        //! \param[in] callback The callback to use for mouse button events.
        virtual void set_mouse_button_callback(mouse_button_callback callback) = 0;
        //! \brief Sets the callback function for mouse position change events.
        //! \param[in] callback The callback to use for mouse position change events.
        virtual void set_mouse_position_callback(mouse_position_callback callback) = 0;
        //! \brief Sets the callback function for mouse scroll events.
        //! \param[in] callback The callback to use for mouse scroll events.
        virtual void set_mouse_scroll_callback(mouse_scroll_callback callback) = 0;
        //! \brief Sets the callback function for drag and drop.
        //! \param[in] callback The callback to use for things dragged onto the mango window.
        virtual void set_drag_and_drop_callback(drag_n_drop_callback callback) = 0;

        //! \brief Hides or unhides the mouse cursor in the \a window.
        //! \param[in] hide True if cursor should be hidden, else false.
        virtual void hide_cursor(bool hide) = 0;

      protected:
        virtual bool create()         = 0;
        virtual void update(float dt) = 0;
        virtual void destroy()        = 0;
    };

} // namespace mango

#endif // MANGO_INPUT_SYSTEM_HPP
