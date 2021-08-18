//! \file      context_impl.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_CONTEXT_IMPL_HPP
#define MANGO_CONTEXT_IMPL_HPP

#include <mango/context.hpp>
#include <util/helpers.hpp>

namespace mango
{
    class display_impl;
    class input_impl;
    class mango_display_event_handler;
    class resources_impl;
    class ui_impl;
    class scene_impl;
    class renderer_impl;
    class graphics_device;

    //! \brief The implementation of the public context.
    class context_impl : public context, public std::enable_shared_from_this<context_impl>
    {
        MANGO_DISABLE_COPY_AND_ASSIGNMENT(context_impl)
      public:
        context_impl();
        virtual ~context_impl();
        void set_application(const shared_ptr<application>& application) override;

        display_handle create_display(const display_configuration& config) override;
        void destroy_display(display_handle display_in) override;
        display_handle get_display() override;
        scene_handle create_scene(const string& name) override;
        void destroy_scene(scene_handle scene_in) override;
        scene_handle get_current_scene() override;
        ui_handle create_ui(const ui_configuration& config) override;
        void destroy_ui(ui_handle ui_in) override;
        ui_handle get_ui() override;
        renderer_handle create_renderer(const renderer_configuration& config) override;
        void destroy_renderer(renderer_handle renderer_in) override;
        renderer_handle get_renderer() override;

        input_handle get_input() override;
        resources_handle get_resources() override;

        //! \brief Creation function for the context.
        //! \details Creates and initializes various systems like \a window_system.
        //! This function is only callable by mango internally.
        //! \return True on creation success, else false.
        bool create();

        //! \brief Queries and returns a unique pointer reference to mangos main \a display.
        //! \details Enables the caller to use internal functionality.
        //! \return A unique pointer reference to mangos \a display.
        const unique_ptr<display_impl>& get_internal_display();

        //! \brief Queries and returns a unique pointer reference to mangos \a renderer.
        //! \details Enables the caller to use internal functionality.
        //! \return A unique pointer reference to mangos \a renderer.
        const unique_ptr<renderer_impl>& get_internal_renderer();

        //! \brief Queries and returns a unique pointer reference to mangos \a scene.
        //! \details Enables the caller to use internal functionality.
        //! \return A unique pointer reference to mangos \a scene.
        const unique_ptr<scene_impl>& get_internal_scene();

        //! \brief Queries and returns a unique pointer reference to mangos \a resources.
        //! \details Enables the caller to use internal functionality.
        //! \return A unique pointer reference to mangos \a resources.
        const unique_ptr<resources_impl>& get_internal_resources();

        //! \brief Queries and returns a unique pointer reference to mangos \a graphics_device.
        //! \return A unique pointer reference to mangos \a graphics_device.
        const unique_ptr<graphics_device>& get_graphics_device();

        //! \brief Queries and returns a shared pointer to the current \a application.
        //! \return A shared pointer to the current \a application.
        virtual shared_ptr<application> get_application();

        //! \brief Polls the events.
        //! \details The call is necessary to receive events from the operating system.
        void poll_events();

        //! \brief Determines if mango should shut down.
        //! \return True if mango should shut down, else false.
        bool should_shutdown();

        //! \brief Calls the update routine for all mango internals.
        //! \param[in] dt Past time since last call. Can be used for frametime independent motion.
        void update(float dt);

        //! \brief Calls the render routine for all mango internals.
        //! \param[in] dt Past time since last call.
        void render(float dt);

        //! \brief Destruction function for the context.
        //! \details Destroys various systems like \a window_system.
        //! This function is only callable by mango internally.
        void destroy();

      private:
        //! \brief A shared pointer to the current active application.
        shared_ptr<application> m_application;

        //! \brief Creates internals.
        //! \return True on success, else false.
        bool startup();

        //! \brief Destroys internals.
        void shutdown();

        //! \brief A unique pointer to the \a display of mango.
        unique_ptr<display_impl> m_display;
        //! \brief A unique pointer to the \a input of mango.
        unique_ptr<input_impl> m_input;
        //! \brief A shared pointer to the \a display_event_handler of mango.
        shared_ptr<mango_display_event_handler> m_event_handler;
        //! \brief A unique pointer to the \a resources of mango.
        unique_ptr<resources_impl> m_resources;
        //! \brief A unique pointer to the \a ui of mango.
        unique_ptr<ui_impl> m_ui;
        //! \brief A unique pointer to the current \a scene of mango.
        unique_ptr<scene_impl> m_current_scene;
        //! \brief A unique pointer to the \a renderer of mango.
        unique_ptr<renderer_impl> m_renderer;
        //! \brief A unique pointer to the \a graphics_device of mango.
        unique_ptr<graphics_device> m_graphics_device;
    };
} // namespace mango

#endif // MANGO_CONTEXT_IMPL_HPP
