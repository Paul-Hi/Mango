//! \file      context.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_CONTEXT_HPP
#define MANGO_CONTEXT_HPP

#include <mango/display.hpp>
#include <mango/input.hpp>
#include <mango/types.hpp>

namespace mango
{
    class application;
    class render_system;
    class ui_system;
    class scene;

    // TODO Paul: Remove!
    class window_system;
    class input_system;

    //! \brief Context interface.
    //! \details The context holds shared pointers to the various subsystems of mango.
    //! It can be used to read, create and modify data in mango.
    class context
    {
      public:
        //! \brief Sets the \a application and creates it internally.
        //! \details Internally the create() function of the \a application is called after attaching it to the context.
        //! This function is called by the \a MANGO_DEFINE_APPLICATION_MAIN main function and should not be called elsewhere.
        //! \param[in] application The application to set in the context.
        virtual void set_application(const shared_ptr<application>& application) = 0;

        //! \brief Queries and returns a weak pointer to mangos \a window_system.
        //! \details Can be used to configure the window shown by mango.
        //! \return A weak pointer to the mango \a window_system.
        virtual weak_ptr<window_system> get_window_system() = 0;

        // TODO Paul: The new graphics stuff has to be done here.

        //! \brief Creates a \a display to use for graphics.
        //! \details The retrieved handle should be destroyed with destroy_display(...) when the \a display is not longer required.
        //! \param[in] config A configuration to set some basic values for the \a display.
        //! \param[out] display_out A reference to the created \a display will be stored here.
        //! \return True if \a display creation was successful, else false.
        virtual bool create_display(const display_configuration& config, display_handle& display_out) = 0;

        //! \brief Destroys a \a display
        //! \param[in] display_in The handle to the \a display to destroy.
        virtual void destroy_display(display_handle& display_in) = 0;

        //! \brief Queries and returns a handle reference to mangos \a input.
        //! \return A handle reference to mangos \a input.
        virtual const input_handle& get_input() = 0;

        //! \brief Queries and returns a weak pointer to mangos \a input_system.
        //! \details Mangos input.
        //! \return A weak pointer to the mango \a input_system.
        virtual weak_ptr<input_system> get_input_system() = 0;

        //! \brief Queries and returns a weak pointer to mangos \a render_system.
        //! \details Can be used to configure the \a rendering_pipeline used by mango.
        //! \return A weak pointer to the mango \a render_system.
        virtual weak_ptr<render_system> get_render_system() = 0;

        //! \brief Queries and returns a weak pointer to mangos \a ui_system.
        //! \details Mangos ui.
        //! \return A weak pointer to the mango \a ui_system.
        virtual weak_ptr<ui_system> get_ui_system() = 0;

        //! \brief Registers a \a scene to mango.
        //! \details This is used to inject the internal \a context into the scene.
        //! \param[in] scene The scene to register.
        virtual void register_scene(shared_ptr<scene>& scene) = 0;

        //! \brief Makes \a scene the current scene in mango.
        //! \details By making a scene current the old scene will be destroyed if no reference is hold by the user.
        //! \param[in] scene The scene to make the current one.
        virtual void make_scene_current(shared_ptr<scene>& scene) = 0;

        //! \brief Retireves the currently active \a scene from mango.
        //! \details By making a scene current the old scene will be destroyed if no reference is hold by the user.
        //! \return The current scene.
        virtual shared_ptr<scene>& get_current_scene() = 0;
    };
} // namespace mango

#endif // MANGO_CONTEXT_HPP
