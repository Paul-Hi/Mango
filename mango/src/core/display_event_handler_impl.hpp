//! \file      display_event_handler_impl.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_DISPLAY_EVENT_HANDLER_IMP_HPP
#define MANGO_DISPLAY_EVENT_HANDLER_IMP_HPP

#include <core/input_impl.hpp>
#include <mango/display_event_handler.hpp>

namespace mango
{
    //! \brief A \a display_event_handler forwarding \a display events to mango.
    class mango_display_event_handler : public display_event_handler
    {
      public:
        //! \brief Constructs a \a display_event_handler forwarding events to mangos input.
        //! \param[in] forward_input A pointer to mangos internal input.
        mango_display_event_handler(input_impl* forward_input)
            : m_forward_input(forward_input)
        {
        }
        ~mango_display_event_handler() = default;

        void on_window_position(int32 x_position, int32 y_position) override
        {
            m_forward_input->on_window_position(x_position, y_position);
        }

        void on_window_resize(int32 width, int32 height) override
        {
            m_forward_input->on_window_resize(width, height);
        }

        void on_window_close() override
        {
            m_forward_input->on_window_close();
        }

        void on_window_refresh() override
        {
            m_forward_input->on_window_refresh();
        }

        void on_window_focus(bool focused) override
        {
            m_forward_input->on_window_focus(focused);
        }

        void on_window_iconify(bool iconified) override
        {
            m_forward_input->on_window_iconify(iconified);
        }

        void on_window_maximize(bool maximized) override
        {
            m_forward_input->on_window_maximize(maximized);
        }

        void on_window_framebuffer_resize(int32 width, int32 height) override
        {
            m_forward_input->on_window_framebuffer_resize(width, height);
        }

        void on_window_content_scale(float x_scale, float y_scale) override
        {
            m_forward_input->on_window_content_scale(x_scale, y_scale);
        }

        void on_input_mouse_button(mouse_button button, input_action action, modifier mods) override
        {
            m_forward_input->on_input_mouse_button(button, action, mods);
        }

        void on_input_cursor_position(double x_position, double y_position) override
        {
            m_forward_input->on_input_cursor_position(x_position, y_position);
        }

        void on_input_cursor_enter(bool entered) override
        {
            m_forward_input->on_input_cursor_enter(entered);
        }

        void on_input_scroll(double x_offset, double y_offset) override
        {
            m_forward_input->on_input_scroll(x_offset, y_offset);
        }

        void on_input_key(key_code key, input_action action, modifier mods) override
        {
            m_forward_input->on_input_key(key, action, mods);
        }

        void on_input_drop(int32 path_count, const char** paths) override
        {
            m_forward_input->on_input_drop(path_count, paths);
        }

      private:
        //! \brief The pointer to forward everything to mangos input.
        input_impl* m_forward_input;
    };
}; // namespace mango

#endif // MANGO_DISPLAY_EVENT_HANDLER_IMP_HPP