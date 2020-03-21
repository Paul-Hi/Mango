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
    class render_system_impl;
    //! \brief The implementation of the public context.
    class context_impl : public context
    {
      public:
        context_impl();
        ~context_impl();
        void set_application(const shared_ptr<application>& application) override;
        weak_ptr<window_system> get_window_system() override;
        weak_ptr<render_system> get_render_system() override;

        //! \brief Creation function for the context.
        //! \details Creates and initializes various systems like \a window_system.
        //! This function is only callable by mango internally.
        //! \return True on creation success, else false.
        bool create();

        //! \brief Queries and returns a weak pointer to mangos \a window_system.
        //! \details This enables you using internal functionalities.
        //! \return A weak pointer to the internal \a window_system.
        weak_ptr<window_system_impl> get_window_system_internal();

        //! \brief Queries and returns a weak pointer to mangos \a render_system.
        //! \details This enables you using internal functionalities.
        //! \return A weak pointer to the internal \a render_system.
        weak_ptr<render_system_impl> get_render_system_internal();

        //! \brief Destruction function for the context.
        //! \details Destriys various systems like \a window_system.
        //! This function is only callable by mango internally.
        virtual void destroy();

      private:
        //! \brief A shared pointer to the current active application.
        shared_ptr<application> m_application;
        //! \brief A shared pointer to the \a window_system of mango.
        shared_ptr<window_system_impl> m_window_system;
        //! \brief A shared pointer to the \a render_system of mango.
        shared_ptr<render_system_impl> m_render_system;
    };
} // namespace mango

#endif // MANGO_CONTEXT_IMPL_HPP
