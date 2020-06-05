//! \file      win32_input_system.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_WIN32_INPUT_SYSTEM_HPP
#define MANGO_WIN32_INPUT_SYSTEM_HPP

#include <core/input_system_impl.hpp>

namespace mango
{
    //! \brief The \a input_system for the microsoft windows platform.
    class win32_input_system : public input_system_impl
    {
      public:
        //! \brief Constructs the \a win32_input_system.
        //! \param[in] context The internally shared context of mango.
        win32_input_system(const shared_ptr<context_impl>& context);

        ~win32_input_system();
        bool create() override;
        virtual void set_platform_data(const shared_ptr<platform_data>& data) override;

        virtual void update(float dt) override;
        virtual void destroy() override;

        virtual input_action get_key(key_code key) override;
        virtual input_action get_mouse_button(mouse_button button) override;
        virtual modifier get_modifiers() override;
        virtual glm::vec2 get_mouse_position() override;
        virtual glm::vec2 get_mouse_scroll() override;

        virtual void set_key_callback(key_callback callback) override;
        virtual void set_mouse_button_callback(mouse_button_callback callback) override;
        virtual void set_mouse_position_callback(mouse_position_callback callback) override;
        virtual void set_mouse_scroll_callback(mouse_scroll_callback callback) override;
        virtual void set_drag_and_drop_callback(drag_n_drop_callback callback) override;

        virtual void hide_cursor(bool hide) override;

      private:
        //! \brief The platform data holds the window handle that is needed to identify the window after creation.
        //! \details This is important, because without it destruction, update and input polling would fail.
        shared_ptr<platform_data> m_platform_data;

        //! \brief Last scroll offset. Stored for access without callback.
        glm::vec2 m_last_scroll_offset;
        //! \brief Last modifier bits. Stored for access without callback.
        modifier m_last_mods;
    };

} // namespace mango

#endif // MANGO_WIN32_INPUT_SYSTEM_HPP
