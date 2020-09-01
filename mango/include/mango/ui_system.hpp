//! \file      ui_system.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_UI_SYSTEM_HPP
#define MANGO_UI_SYSTEM_HPP

#include <mango/assert.hpp>
#include <mango/system.hpp>

namespace mango
{
    //! \brief Available widgets for the ui.
    enum ui_widget
    {
        render_view,                //!< Widget displaying the rendered scene.
        hardware_info,              //!< Widget giving some hardware info.
        scene_inspector,            //!< Widget displaying the scene hierachy.
        material_inspector,         //!< Widget displaying material properties for the selected entity.
        entity_component_inspector, //! Widget displaying component properties for the selected entity.
        render_system_ui,           //! Widget displaying render system related settings and debugging information.
        number_of_ui_widgets        //!< Number of widgets.
    };

    //! \brief The custom ui data.
    struct custom_ui_data
    {
        string window_name;                          //!< The name of the window used for menu generation.
        std::function<void(bool& enabled)> function; //!< The custom function with the imgui code.
        bool always_open;                            //!< True if window should always be open, else false.
    };

    //! \brief The configuration for the \a ui_system.
    //! \details Should be used to configure the \a ui_system in the \a application create() method.
    class ui_configuration
    {
      public:
        //! \brief Default constructor to set some default values before the user application configures the \a ui_system.
        ui_configuration()
            : m_docking(true)
        {
            std::memset(m_ui_widgets, 0, ui_widget::number_of_ui_widgets * sizeof(bool));
        }

        //! \brief Constructs a \a ui_configuration with specific values.
        //! \param[in] docking True if the \a ui_system should provide a dock space, else false.
        ui_configuration(bool docking)
            : m_docking(docking)
        {
            std::memset(m_ui_widgets, 0, ui_widget::number_of_ui_widgets * sizeof(bool));
        }

        //! \brief Enables or disables the docking functionality in the \a ui_system.
        //! \param[in] docking True if the \a ui_system should provide a dock space, else false.
        //! \return A reference to the modified \a ui_configuration.
        inline ui_configuration& enable_dock_space(bool docking)
        {
            m_docking = docking;
            return *this;
        }

        //! \brief Makes the \a ui_system show an \a ui_widget.
        //! \param[in] some_ui_widget The configurated \a ui_widget to show.
        //! \return A reference to the modified \a ui_configuration.
        inline ui_configuration& show_widget(ui_widget some_ui_widget)
        {
            MANGO_ASSERT(some_ui_widget < ui_widget::number_of_ui_widgets, "ui widget can not be added, it is out of bounds!");
            m_ui_widgets[some_ui_widget] = true;
            return *this;
        }

        //! \brief Submits any custom ui function to the \a ui_system.
        //! \details  Has to be only one window at the moment.
        //! \param[in] window_name The name of the top window in the custom function.
        //! \param[in] custom_ui_function The function with custom ui to show.
        //! \param[in] always_open Specifies if window can be closed.
        //! \return A reference to the modified \a ui_configuration.
        inline ui_configuration& submit_custom(string window_name, std::function<void(bool& enabled)> custom_ui_function, bool always_open = false)
        {
            MANGO_ASSERT(custom_ui_function, "Function does not exist!");
            m_custom_ui_data.window_name = window_name;
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
        //! \return The current configurated custom ui data of the \a ui_system.
        inline custom_ui_data get_custom_ui_data() const
        {
            return m_custom_ui_data;
        }

        //! \brief Retrieves and returns the array of possible \a ui_widgets set in the \a ui_configuration.
        //! \details The returned array has the size \a ui_widget::number_of_ui_widgets.
        //! On a specific position in the array there is the value 'true' if the \a ui_widget should be shown, else 'false'.
        //! \return The current configurated array of possible \a ui_widgets of the \a ui_system.
        inline const bool* get_ui_widgets() const
        {
            return m_ui_widgets;
        }

      private:
        //! \brief The configurated setting of the \a ui_configuration to enable or disable docking in the \a ui_system.
        bool m_docking;
        //! \brief The configurated \a ui_widgets in the \a ui_configuration to show in the \a ui_system.
        bool m_ui_widgets[ui_widget::number_of_ui_widgets];
        //! \brief The configurated custom ui data.
        custom_ui_data m_custom_ui_data;
    };

    //! \brief A system for user interface drawing.
    class ui_system : public system
    {
      public:
        //! \brief Does the configuration of the \a ui_system.
        //! \details After creation this function should be called.
        //! Changes the configuration in the \a ui_system to \a configuration.
        //! \param[in] configuration The \a ui_configuration to use.
        virtual void configure(const ui_configuration& configuration) = 0;

      protected:
        virtual bool create()         = 0;
        virtual void update(float dt) = 0;
        virtual void destroy()        = 0;
    };

} // namespace mango

#endif // MANGO_UI_SYSTEM_HPP
