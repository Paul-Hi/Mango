//! \file      system.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_SYSTEM_HPP
#define MANGO_SYSTEM_HPP

#include <mango/context.hpp>
#include <mango/types.hpp>

namespace mango
{
    //! \brief The base class for all systems in mango.
    //! \details A \a system is a part of mango doing specific actions on creation, update and on destruction of mango.
    //! They are designed and abstracted to be modular so they may have platform or api specific implementations.
    class system
    {
      protected:
        virtual ~system() = default;
        //! \brief Creation function for every system.
        //! \details All the necessary system specific setup should be done in here and not in the constructor.
        //! \return True on creation success, else false.
        virtual bool create() = 0;

        //! \brief Calls the system specific update routine.
        //! \details All the necessary system specific updates are done in here.
        //! \param[in] dt Past time since last call.
        virtual void update(float dt) = 0;

        //! \brief Destroys the system.
        //! \details All the necessary application specific cleanup should be done in here and not in the destructor.
        virtual void destroy() = 0;

        //! \brief Custom UI function.
        //! \details This can be called by any \a ui_widget and displays information for the \a system.
        //! This does not draw any window, so it needs one surrounding it.
        virtual void on_ui_widget() {}
    };

} // namespace mango

#endif // MANGO_SYSTEM_HPP
