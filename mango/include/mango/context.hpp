//! \file      context.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_CONTEXT_HPP
#define MANGO_CONTEXT_HPP

#include <mango/display.hpp>
#include <mango/input.hpp>
#include <mango/renderer.hpp>
#include <mango/resources.hpp>
#include <mango/scene.hpp>
#include <mango/types.hpp>
#include <mango/ui.hpp>

namespace mango
{
    class application;

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

        //! \brief Creates a \a display to use for graphics.
        //! \details The retrieved handle should be destroyed with destroy_display(...) when the \a display is not longer required.
        //! \param[in] config A configuration to set some basic values for the \a display.
        //! \return A handle to the created \a display or nullptr on failed creation.
        virtual display_handle create_display(const display_configuration& config) = 0;

        //! \brief Destroys a \a display.
        //! \param[in] display_in The handle to the \a display to destroy.
        virtual void destroy_display(display_handle display_in) = 0;

        //! \brief Queries and returns a handle reference to mangos main \a display.
        //! \details At the moment only one \a display is supported anyway.
        //! \return A handle reference to mangos \a display or nullptr when \a display is not accessible.
        virtual display_handle get_display() = 0;

        //! \brief Queries and returns a handle reference to mangos \a input.
        //! \return A handle reference to mangos \a input or nullptr when \a input is not accessible.
        virtual input_handle get_input() = 0;

        //! \brief Queries and returns a handle reference to mangos \a resources.
        //! \return A handle reference to mangos \a resources or nullptr when \a resources is not accessible.
        virtual resources_handle get_resources() = 0;

        //! \brief Creates a \a ui to use for graphics.
        //! \details The retrieved handle should be destroyed with destroy_ui(...) when the \a ui is not longer required.
        //! \param[in] config A configuration to set some basic values for the \a ui.
        //! \return A handle to the created \a ui or nullptr on failed creation.
        virtual ui_handle create_ui(const ui_configuration& config) = 0;

        //! \brief Destroys a \a ui.
        //! \param[in] ui_in The handle to the \a ui to destroy.
        virtual void destroy_ui(ui_handle ui_in) = 0;

        //! \brief Queries and returns a handle reference to mangos \a ui.
        //! \details At the moment only one \a ui is supported anyway.
        //! \return A handle reference to mangos \a ui or nullptr when \a ui is not accessible.
        virtual ui_handle get_ui() = 0;

        //! \brief Creates a \a renderer to use for graphics.
        //! \details The retrieved handle should be destroyed with destroy_renderer(...) when the \a renderer is not longer required.
        //! \param[in] config A configuration to set some basic values for the \a renderer.
        //! \return A handle to the created \a renderer or nullptr on failed creation.
        virtual renderer_handle create_renderer(const renderer_configuration& config) = 0;

        //! \brief Destroys a \a renderer.
        //! \param[in] renderer_in The handle to the \a renderer to destroy.
        virtual void destroy_renderer(renderer_handle renderer_in) = 0;

        //! \brief Queries and returns a handle reference to mangos \a renderer.
        //! \details At the moment only one \a renderer is supported anyway.
        //! \return A handle reference to mangos \a renderer or nullptr when \a renderer is not accessible.
        virtual renderer_handle get_renderer() = 0;

        //! \brief Creates a \a scene.
        //! \details The retrieved handle should be destroyed with destroy_scene(...) when the \a scene is not longer required.
        //! \param[in] name The name for the new \a scene.
        //! \return A handle to the created \a scene or nullptr when creation failed.
        virtual scene_handle create_scene(const string& name) = 0;

        //! \brief Destroys a \a scene
        //! \param[in] scene_in The handle to the \a scene to destroy.
        virtual void destroy_scene(scene_handle scene_in) = 0;

        //! \brief Queries and returns a handle reference to mangos current \a scene.
        //! \details At the moment only one scene is supported anyway.
        //! \return A handle reference to mangos \a scene or nullptr when \a scene is not accessible.
        virtual scene_handle get_current_scene() = 0;
    };
} // namespace mango

#endif // MANGO_CONTEXT_HPP
