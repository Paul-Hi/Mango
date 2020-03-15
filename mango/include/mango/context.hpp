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
    //! \brief Context interface.
    //! \details The context holds shared pointers to the various subsystems of mango.
    //! \details It can be used to read, create and modify data in mango.
    class context
    {
    };
} // namespace mango

#endif // MANGO_CONTEXT_HPP