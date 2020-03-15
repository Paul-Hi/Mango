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
    //! \brief The implementation of the public context.
    class context_impl : public context
    {
      public:
        context_impl();
        ~context_impl();
    };
} // namespace mango

#endif // MANGO_CONTEXT_IMPL_HPP