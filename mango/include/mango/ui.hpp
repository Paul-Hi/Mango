//! \file      ui.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_UI_HPP
#define MANGO_UI_HPP

#include <mango/assert.hpp>

namespace mango
{
    //! \brief Prebuild widgets for the ui.
    enum ui_widget
    {
        render_view,                //!< Widget displaying the rendered scene.
        graphics_info,              //!< Widget giving some graphics info.
        scene_inspector,            //!< Widget displaying the scene hierachy.
        entity_component_inspector, //! Widget displaying component properties for the selected entity.
        primitive_material_inspector, //! Widget displaying primitive and material information for selected primitive.
        renderer_ui,                //! Widget displaying render system related settings and debugging information.
        number_of_ui_widgets        //!< Number of widgets.
    };

    //! \brief The custom ui data.
    struct custom_ui_data
    {
        string widget_name;                          //!< The name of the widget used for menu generation.
        std::function<void(bool& enabled)> function; //!< The custom function with the imgui code.
        bool always_open;                            //!< True if widget should always be open, else false.
    };

    //! \brief The configuration for the \a ui.
    //! \details Should be used to configure the \a ui in the \a application create() method.
    class ui_configuration
    {
      public:
        //! \brief Default constructor to set some default values before the user application configures the \a ui.
        ui_configuration()
            : m_docking(true)
        {
            std::memset(m_ui_widgets, 0, ui_widget::number_of_ui_widgets * sizeof(bool));
        }

        //! \brief Constructs a \a ui_configuration with specific values.
        //! \param[in] docking True if the \a ui should provide a dock space, else false.
        ui_configuration(bool docking)
            : m_docking(docking)
        {
            std::memset(m_ui_widgets, 0, ui_widget::number_of_ui_widgets * sizeof(bool));
        }

        //! \brief Enables or disables the docking functionality in the \a ui.
        //! \param[in] docking True if the \a ui should provide a dock space, else false.
        //! \return A reference to the modified \a ui_configuration.
        inline ui_configuration& enable_dock_space(bool docking)
        {
            m_docking = docking;
            return *this;
        }

        //! \brief Makes the \a ui show an \a ui_widget.
        //! \param[in] some_ui_widget The configurated \a ui_widget to show.
        //! \return A reference to the modified \a ui_configuration.
        inline ui_configuration& show_widget(ui_widget some_ui_widget)
        {
            MANGO_ASSERT(some_ui_widget < ui_widget::number_of_ui_widgets, "ui widget can not be added, it is out of bounds!");
            m_ui_widgets[some_ui_widget] = true;
            return *this;
        }

        //! \brief Submits any custom ui function to the \a ui.
        //! \details  Has to be only one widget at the moment.
        //! \param[in] widget_name The name of the top widget in the custom function.
        //! \param[in] custom_ui_function The function with custom ui to show. Without Begin() and End() since this is added internally.
        //! \param[in] always_open Specifies if widget can be closed.
        //! \return A reference to the modified \a ui_configuration.
        inline ui_configuration& submit_custom(string widget_name, std::function<void(bool& enabled)> custom_ui_function, bool always_open = false)
        {
            MANGO_ASSERT(custom_ui_function, "Function does not exist!");
            m_custom_ui_data.widget_name = widget_name;
            m_custom_ui_data.function    = custom_ui_function;
            m_custom_ui_data.always_open = always_open;
            return *this;
        }

        //! \brief Retrieves and returns the setting for the dock space in the \a ui_configuration.
        //! \return True if the dock space is enabled, else false.
        inline bool is_dock_space_enabled() const
        {
            return m_docking;
        }

        //! \brief Retrieves and returns the custom ui data set in the \a ui_configuration.
        //! \return The current configurated custom ui data of the \a ui.
        inline const custom_ui_data& get_custom_ui_data() const
        {
            return m_custom_ui_data;
        }

        //! \brief Retrieves and returns the array of possible \a ui_widgets set in the \a ui_configuration.
        //! \details The returned array has the size \a ui_widget::number_of_ui_widgets.
        //! On a specific position in the array there is the value 'true' if the \a ui_widget should be shown, else 'false'.
        //! \return The current configurated array of possible \a ui_widgets of the \a ui.
        inline const bool* get_ui_widgets() const
        {
            return m_ui_widgets;
        }

      private:
        //! \brief The configurated setting of the \a ui_configuration to enable or disable docking in the \a ui.
        bool m_docking;
        //! \brief The configurated \a ui_widgets in the \a ui_configuration to show in the \a ui.
        bool m_ui_widgets[ui_widget::number_of_ui_widgets];
        //! \brief The configurated custom ui data.
        custom_ui_data m_custom_ui_data;
    };

    //! \brief A system for user interface drawing.
    class ui
    {
      public:
        //! \brief Determines wether the dock space of the \a ui is enabled.
        //! \return True if the dock space is enabled, else false.
        virtual bool is_dock_space_enabled() const = 0;

        //! \brief Retrieves the current content size.
        //! \details This is used when the ui is providing a widget rendering the main content.
        //! \return The content size that can be rendered to.
        virtual const ivec2& get_content_size() const = 0;
    };

    //! \brief A unique pointer holding a \a ui.
    using ui_ptr    = std::unique_ptr<ui>;

    //! \brief A constant pointer pointing to a \a ui.
    using ui_handle = const ui*;
} // namespace mango

#endif // MANGO_UI_HPP
