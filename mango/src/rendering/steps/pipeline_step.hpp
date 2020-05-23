//! \file      pipeline_step.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_PIPELINE_STEP_HPP
#define MANGO_PIPELINE_STEP_HPP

#include <graphics/command_buffer.hpp>
#include <mango/system.hpp>
#include <mango/types.hpp>

namespace mango
{
    //! \brief Base class for all optional pipeline steps in render pipelines.
    class pipeline_step : public system
    {
      public:
        //! \brief Attaches the step to the current active pipeline.
        //! \details After creation this function should be called. Does all the setup.
        virtual void attach() = 0;

        //! \brief Executes the command by inserting commands into the given \a render \a command_buffer.
        //! \param[in] command_buffer The buffer given by the \a render to execute into.
        virtual void execute(command_buffer_ptr& command_buffer) = 0;

      protected:
        virtual bool create()         = 0;
        virtual void update(float dt) = 0;
        virtual void destroy()        = 0;
    };
} // namespace mango

#endif // MANGO_PIPELINE_STEP_HPP
