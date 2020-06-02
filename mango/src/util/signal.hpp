//! \file      signal.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_SIGNAL_HPP
#define MANGO_SIGNAL_HPP

#include <mango/types.hpp>
#include <vector>

namespace mango
{
    //! \brief Implementation of a simple class providing signal slot functionality .
    //! \details Can be used to call multiple function callbacks.
    template <typename... Args>
    class signal
    {
        using fun = std::function<void(Args...)>;

      public:
        inline void connect(fun fn)
        {
            observers.push_back(fn);
        }

        void operator()(Args... a)
        {
            for (const auto& fn : observers)
            {
                fn(a...);
            }
        }

      private:
        std::vector<fun> observers;
    };
} // namespace mango

#endif // MANGO_SIGNAL_HPP