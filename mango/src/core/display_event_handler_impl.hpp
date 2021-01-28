//! \file      display_event_handler_impl.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_DISPLAY_EVENT_HANDLER_IMP_HPP
#define MANGO_DISPLAY_EVENT_HANDLER_IMP_HPP

#include <mango/display_event_handler.hpp>
#include <core/input_impl.hpp>

namespace mango
{
    //! \brief A \a display_event_handler forwarding \a display events to mango.
    class mango_display_event_handler : public display_event_handler
    {
      public:
        //! \brief Constructs a \a display_event_handler forwarding events to mangos input.
        //! \param[in] forward_input A pointer to mangos internal input.
        mango_display_event_handler(input_impl* forward_input);
        ~mango_display_event_handler();

        void on_window_position(int32 x_position, int32 y_position) override;
        void on_window_resize(int32 width, int32 height) override;
        void on_window_close() override;
        void on_window_refresh() override;
        void on_window_focus(bool focused) override;
        void on_window_iconify(bool iconified) override;
        void on_window_maximize(bool maximized) override;
        void on_window_framebuffer_resize(int32 width, int32 height) override;
        void on_window_content_scale(float x_scale, float y_scale) override;
        void on_input_mouse_button(mouse_button button, input_action action, modifier mods) override;
        void on_input_cursor_position(double x_position, double y_position) override;
        void on_input_cursor_enter(bool entered) override;
        void on_input_scroll(double x_offset, double y_offset) override;
        void on_input_key(key_code key, input_action action, modifier mods) override;
        void on_input_drop(int32 path_count, const char** paths) override;

      private:
        //! \brief The pointer to forward everything to mangos input.
        input_impl* m_forward_input;
    };
}; // namespace mango

#endif // MANGO_DISPLAY_EVENT_HANDLER_IMP_HPP