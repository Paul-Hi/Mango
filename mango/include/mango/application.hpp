//! \file      application.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_APPLICATION_HPP
#define MANGO_APPLICATION_HPP

#include <mango/context.hpp>
#include <mango/types.hpp>

#ifdef WIN32

#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>

//! \brief A macro to define an application main.
//! \param class_name Name of the inheriting from mango::application to run
//! \return 0 on success, 1 else.
#define MANGO_DEFINE_APPLICATION_MAIN(class_name)                                                 \
    int WINAPI WinMain(HINSTANCE hIntance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow) \
    {                                                                                             \
        class_name application;                                                                   \
        return application.run();                                                                 \
    }

#else

//! \brief A macro to define an application main.
//! \param class_name Name of the inheriting from mango::application to run
//! \return 0 on success, 1 else.
#define MANGO_DEFINE_APPLICATION_MAIN(class_name) \
    int main(int argc, char** argv)               \
    {                                             \
        class_name application;                   \
        return application.run(argc, argv);       \
    }

#endif // WIN32

namespace mango
{
    //! \brief Application interface.
    //! \details The application is the base for all applications using the mango graphics engine.
    //! \details Every application needs to inherit from this.
    class application
    {
      public:
        application();
        ~application() = default;

        //! \brief Runs the application.
        //! \details This includes the application loop that runs until the termination.
        //! \param[in] t_argc Number of command line arguments.
        //! \param[in] t_argv Command line arguments.
        //! \return 0 on success, else 1.
        uint32 run(uint32 t_argc = 0, char** t_argv = nullptr);

        //! \brief Returns the current mango application context.
        //! \return A weak pointer to the context.
        inline weak_ptr<context> get_context()
        {
            return m_context;
        };

      private:
        //! \brief The context of the application.
        shared_ptr<context> m_context;
    };

} // namespace mango

#endif // MANGO_APPLICATION_HPP