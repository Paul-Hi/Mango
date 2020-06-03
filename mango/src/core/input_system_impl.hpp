//! \file      input_system_impl.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_INPUT_SYSTEM_IMPL_HPP
#define MANGO_INPUT_SYSTEM_IMPL_HPP

#include <core/context_impl.hpp>
#include <mango/input_system.hpp>
#include <util/signal.hpp>

namespace mango
{
    struct platform_data;
    //! \brief The implementation of the \a input_system.
    class input_system_impl : public input_system
    {
      public:
        virtual bool create() = 0;

        //! \brief Sets the \a platform_data given by the \a window_system.
        //! \details This is required so that anything input related can work.
        //! \param[in] data A shared pointer to the \a platform_data given by the \a window_system.
        virtual void set_platform_data(const shared_ptr<platform_data>& data) = 0;

        virtual void update(float dt) = 0;
        virtual void destroy()        = 0;

        virtual input_action get_key(key_code key)                 = 0;
        virtual input_action get_mouse_button(mouse_button button) = 0;
        virtual modifier get_modifiers()                           = 0;
        virtual glm::vec2 get_mouse_position()                     = 0;
        virtual glm::vec2 get_mouse_scroll()                       = 0;

        virtual void set_key_callback(key_callback callback)                       = 0;
        virtual void set_mouse_button_callback(mouse_button_callback callback)     = 0;
        virtual void set_mouse_position_callback(mouse_position_callback callback) = 0;
        virtual void set_mouse_scroll_callback(mouse_scroll_callback callback)     = 0;
        virtual void set_drag_and_drop_callback(drag_n_drop_callback callback)     = 0;

      protected:
        //! \brief User data for glfw.
        struct input_user_data
        {
            //! \brief Mangos internal context for shared usage in all systems.
            shared_ptr<context_impl> shared_context;

            //! \brief Used \a signal for key changes.
            signal<key_code, input_action, modifier> key_change;
            //! \brief Used \a signal for mouse button changes.
            signal<mouse_button, input_action, modifier> mouse_button_change;
            //! \brief Used \a signal for mouse position changes.
            signal<float, float> mouse_position_change;
            //! \brief Used \a signal for mouse scroll changes.
            signal<float, float> mouse_scroll_change;
            //! \brief Used \a signal for drag'n'drop changes.
            signal<int, const char**> drag_n_drop_change;
        } m_input_user_data; //!< The user data instance used for callback and context access.
    };

} // namespace mango

#endif // MANGO_INPUT_SYSTEM_IMPL_HPP
