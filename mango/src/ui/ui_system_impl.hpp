//! \file      ui_system_impl.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_UI_SYSTEM_IMPL_HPP
#define MANGO_UI_SYSTEM_IMPL_HPP

#include <core/context_impl.hpp>
#include <mango/ui_system.hpp>

namespace mango
{
    //! \brief The implementation of the \a ui_system.
    class ui_system_impl : public ui_system
    {
      public:
        //! \brief Constructs the \a ui_system_impl.
        //! \param[in] context The internally shared context of mango.
        ui_system_impl(const shared_ptr<context_impl>& context);
        ~ui_system_impl();

        virtual bool create() override;
        virtual void configure(const ui_configuration& configuration) override;
        virtual void update(float dt) override;
        virtual void destroy() override;

        //! \brief Does all the setup and draws the user interface.
        virtual void draw_ui();

      private:
        //! \brief Specifies if docking is enabled or not.
        //! \details This does also enable rendering to a backbuffer so the rendered content remains visible.
        bool m_docking;
        //! \brief The \a ui_widgets that are shown.
        shared_ptr<ui_widget> m_ui_widgets[mango::ui_widget::number_of_ui_widgets];
        //! \brief The custom ui function.
        std::function<void()> m_custom_ui_function;

        //! \brief Mangos internal context for shared usage in all \a ui_systems.
        shared_ptr<context_impl> m_shared_context;
    };

} // namespace mango

#endif // #define MANGO_UI_SYSTEM_IMPL_HPP
