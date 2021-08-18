//! \file      ui_impl.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_UI_IMPL_HPP
#define MANGO_UI_IMPL_HPP

#include <core/context_impl.hpp>
#include <mango/ui.hpp>
#include <util/helpers.hpp>

namespace mango
{
    //! \brief The implementation of the \a ui.
    class ui_impl : public ui
    {
        MANGO_DISABLE_COPY_AND_ASSIGNMENT(ui_impl)
      public:
        //! \brief Constructs the \a ui_impl.
        //! \param[in] configuration The \a ui_configuration to use.
        //! \param[in] context The internally shared context of mango.
        ui_impl(const ui_configuration& configuration, const shared_ptr<context_impl>& context);
        ~ui_impl();

        //! \brief Updates the \a ui.
        //! \param[in] dt Past time since last call.
        void update(float dt);

        //! \brief Does all the setup and draws the user interface.
        void draw_ui();

        inline bool is_dock_space_enabled() const override
        {
            return m_configuration.is_dock_space_enabled();
        }

        inline const ivec2& get_content_size() const override
        {
            return content_size;
        }

      private:
        //! \brief Setup for the \a ui.
        //! \details Called on construction.
        void setup();

        //! \brief The current size of the rectangular content space.
        ivec2 content_size;

        //! \brief The custom ui function.
        std::function<void()> m_custom_ui_function;

        //! \brief The \a ui_configuration of the \a ui.
        ui_configuration m_configuration;

        //! \brief Mangos internal context for shared usage in all \a uis.
        shared_ptr<context_impl> m_shared_context;

        //! \brief Specifies if \a ui is in cinema mode.
        bool m_cinema_view;
        //! \brief Specifies which \a ui_widgets are shown if available.
        bool m_enabled_ui_widgets[mango::ui_widget::number_of_ui_widgets + 1];
    };

} // namespace mango

#endif // #define MANGO_UI_IMPL_HPP
