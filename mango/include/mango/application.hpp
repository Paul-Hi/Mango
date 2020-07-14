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
//! \param[in] class_name Name of the application to run inheriting from mango::application.
//! \return 0 on success, 1 else.
#define MANGO_DEFINE_APPLICATION_MAIN(class_name)                    \
    int WINAPI WinMain(HINSTANCE, HINSTANCE, PSTR, int)              \
    {                                                                \
        shared_ptr<class_name> app = std::make_shared<class_name>(); \
        weak_ptr<context> c        = app->get_context();             \
        if (auto sp = c.lock())                                      \
        {                                                            \
            sp->set_application(app);                                \
            return app->run();                                       \
        }                                                            \
        MANGO_LOG_CRITICAL("Context is expired");                    \
        std::cin.get();                                              \
        return 1;                                                    \
    }

#else

//! \brief A macro to define an application main.
//! \param[in] class_name Name of the application to run inheriting from mango::application.
//! \return 0 on success, 1 else.
#define MANGO_DEFINE_APPLICATION_MAIN(class_name)                    \
    int main(int, char**)                                            \
    {                                                                \
        shared_ptr<class_name> app = std::make_shared<class_name>(); \
        weak_ptr<context> c        = app->get_context();             \
        if (auto sp = c.lock())                                      \
        {                                                            \
            sp->set_application(app);                                \
            return app->run();                                       \
        }                                                            \
        MANGO_LOG_CRITICAL("Context is expired");                    \
        std::cin.get();                                              \
        return 1;                                                    \
    }

#endif // WIN32

namespace mango
{
    class context_impl;
    class timer;
    //! \brief Application interface.
    //! \details The application is the base for all applications using the mango graphics engine.
    //! Every application needs to inherit from this.
    class application
    {
      public:
        application();
        ~application();

        //! \brief Creation function for every application.
        //! \details This has to be overriden by the inheriting application.
        //! All the necessary application specific setup should be done in here and not in the constructor.
        //! The function gets called by mango and should not be called elsewhere.
        //! \return True on creation success, else false.
        virtual bool create() = 0;

        //! \brief Runs the application.
        //! \details This includes the application loop that runs until the termination.
        //! \param[in] argc Number of command line arguments \a argv.
        //! \param[in] argv Command line arguments.
        //! \return 0 on success, else 1.
        uint32 run(uint32 argc = 0, char** argv = nullptr);

        //! \brief Calls the application specific update routine.
        //! \details This has to be overriden by the inheriting application.
        //! All the necessary application specific updates can be done in here.
        //! The function gets called by mango and should not be called elsewhere.
        //! \param[in] dt Past time since last call. Can be used for frametime independent motion.
        virtual void update(float dt) = 0;

        //! \brief Destroys the application.
        //! \details This has to be overriden by the inheriting application.
        //! All the necessary application specific cleanup should be done in here and not in the destructor.
        //! The function gets called by mango and should not be called elsewhere.
        virtual void destroy() = 0;

        //! \brief Returns the name of the application.
        //! \details This can be overriden by the inheriting application.
        //! Returns a default name for the application if not set.
        //! \return The name of the application.
        virtual const char* get_name()
        {
            return "Mango Application";
        }

        //! \brief Returns the current mango application context.
        //! \return A weak pointer to the context.
        weak_ptr<context> get_context();

      private:
        //! \brief The context of the application.
        shared_ptr<context_impl> m_context;
        //! \brief The timer per frame of the application.
        shared_ptr<timer> m_frame_timer;
    };

} // namespace mango

#endif // MANGO_APPLICATION_HPP
