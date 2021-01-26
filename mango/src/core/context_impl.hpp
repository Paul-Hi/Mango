//! \file      context_impl.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_CONTEXT_IMPL_HPP
#define MANGO_CONTEXT_IMPL_HPP

#include <mango/context.hpp>

namespace mango
{
    class window_system_impl;
    class input_system_impl;
    class render_system_impl;
    class ui_system_impl;
    class shader_system;
    class resource_system;
    //! \brief The implementation of the public context.
    class context_impl : public context, public std::enable_shared_from_this<context_impl>
    {
      public:
        context_impl() = default;
        virtual ~context_impl() = default;
        void set_application(const shared_ptr<application>& application) override;
        weak_ptr<window_system> get_window_system() override;
        weak_ptr<input_system> get_input_system() override;
        weak_ptr<render_system> get_render_system() override;
        weak_ptr<ui_system> get_ui_system() override;
        void register_scene(shared_ptr<scene>& scene) override;
        void make_scene_current(shared_ptr<scene>& scene) override;
        shared_ptr<scene>& get_current_scene() override;

        //! \brief Creation function for the context.
        //! \details Creates and initializes various systems like \a window_system.
        //! This function is only callable by mango internally.
        //! \return True on creation success, else false.
        bool create();

        //! \brief Queries and returns a shared pointer to the current \a application.
        //! \return A shared pointer to the current \a application.
        virtual shared_ptr<application> get_application();

        //! \brief Queries and returns a weak pointer to mangos \a window_system.
        //! \details This enables you using internal functionalities.
        //! \return A weak pointer to the internal \a window_system.
        virtual weak_ptr<window_system_impl> get_window_system_internal();

        //! \brief Queries and returns a weak pointer to mangos \a input_system.
        //! \details This enables you using internal functionalities.
        //! \return A weak pointer to the internal \a input_system.
        virtual weak_ptr<input_system_impl> get_input_system_internal();

        //! \brief Queries and returns a weak pointer to mangos \a render_system.
        //! \details This enables you using internal functionalities.
        //! \return A weak pointer to the internal \a render_system.
        virtual weak_ptr<render_system_impl> get_render_system_internal();

        //! \brief Queries and returns a weak pointer to mangos \a ui_system.
        //! \details This enables you using internal functionalities.
        //! \return A weak pointer to the internal \a ui_system.
        virtual weak_ptr<ui_system_impl> get_ui_system_internal();

        //! \brief Queries and returns a weak pointer to mangos \a shader_system.
        //! \details The \a shader_system is only available internally, but the function name was choosen to be consistent.
        //! \return A weak pointer to the internal \a shader_system.
        virtual weak_ptr<shader_system> get_shader_system_internal();

        //! \brief Queries and returns a weak pointer to mangos \a resource_system.
        //! \details The \a resource_system is only available internally, but the function name was choosen to be consistent.
        //! \return A weak pointer to the internal \a resource_system.
        virtual weak_ptr<resource_system> get_resource_system_internal();

        //! \brief Queries and returns a mangos loading procedure for opengl.
        //! \return Mangos loading procedure for opengl.
        const mango_gl_load_proc& get_gl_loading_procedure();

        //! \brief Sets mangos loading procedure for opengl.
        //! \details This is usually called by the \a window_system.
        //! \param[in] procedure Mangos loading procedure for opengl.
        void set_gl_loading_procedure(mango_gl_load_proc procedure);

        //! \brief Makes this context the current one.
        //! \details This should be called before making changes to the \a window_system and \a render_system.
        void make_current();

        //! \brief Destruction function for the context.
        //! \details Destroys various systems like \a window_system.
        //! This function is only callable by mango internally.
        void destroy();

      private:
        //! \brief A shared pointer to the current active application.
        shared_ptr<application> m_application;
        //! \brief A shared pointer to the \a window_system of mango.
        shared_ptr<window_system_impl> m_window_system;
        //! \brief A shared pointer to the \a input_system of mango.
        shared_ptr<input_system_impl> m_input_system;
        //! \brief A shared pointer to the \a render_system of mango.
        shared_ptr<render_system_impl> m_render_system;
        //! \brief A shared pointer to the \a shader_system of mango.
        shared_ptr<shader_system> m_shader_system;
        //! \brief A shared pointer to the \a resource_system of mango.
        shared_ptr<resource_system> m_resource_system;
        //! \brief A shared pointer to the \a ui_system of mango.
        shared_ptr<ui_system_impl> m_ui_system;
        //! \brief A shared pointer to the current \a scene of mango.
        shared_ptr<scene> m_current_scene;
        //! \brief The gl loading procedure of mango.
        //! \details This is usually set by the \a window_system on creation and can be used in the \a render_system to initialize opengl.
        mango_gl_load_proc m_procedure;

        //! \brief Creates all systems.
        //! \return True on success, else False.
        bool create_systems();

        //! \brief Destroys all systems.
        void destroy_systems();
    };
} // namespace mango

#endif // MANGO_CONTEXT_IMPL_HPP
