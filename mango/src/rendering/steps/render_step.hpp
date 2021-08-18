//! \file      render_step.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_RENDER_STEP_HPP
#define MANGO_RENDER_STEP_HPP

#include <mango/types.hpp>

namespace mango
{
    //! \brief Base class for all optional render steps in \a renderers.
    class render_step
    {
      public:
        //! \brief Attaches the step to the current active \a renderer.
        //! \details After creation this function has to be called. Does all the setup.
        //! \param[in] context The internally shared context of mango.
        virtual void attach(const shared_ptr<context_impl>& context) = 0;

        //! \brief Executes the \a render_step.
        virtual void execute() = 0;

        //! \brief Custom UI function.
        //! \details This can be called by any \a ui_widget and displays settings for the active \a render_step.
        //! This does not draw any window, so it needs one surrounding it.
        virtual void on_ui_widget() = 0;

      protected:
        //! \brief Does create step resources for the \a render_step.
        //! \return True on success, else false.
        virtual bool create_step_resources() = 0;

        //! \brief Mangos shared context.
        shared_ptr<context_impl> m_shared_context;
    };
} // namespace mango

#endif // MANGO_RENDER_STEP_HPP
