//! \file      context.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_CONTEXT_HPP
#define MANGO_CONTEXT_HPP

#include <mango/types.hpp>

namespace mango
{
    class application;
    class window_system;
    class render_system;

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

        //! \brief Queries and returns a weak pointer to mangos \a render_system.
        //! \details Can be used to configure the \a rendering_pipeline used by mango.
        //! \return A weak pointer to the mango \a render_system.
        virtual weak_ptr<render_system> get_render_system() = 0;
    };
} // namespace mango

#endif // MANGO_CONTEXT_HPP
