//! \file      signal.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
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
        //! \brief Convenience typedef of the template function.
        using fun = std::function<void(Args...)>;

      public:
        //! \brief Connects a function to the signal that will be called on signal execution.
        //! \param[in] fn The function to callback.
        inline void connect(fun fn)
        {
            observers.push_back(fn);
        }

        //! \brief Executes the connected functions with given arguments.
        //! \param[in] args 0 to n arguments to call the functions with.
        void operator()(Args... args)
        {
            for (const auto& fn : observers)
            {
                fn(args...);
            }
        }

      private:
        //! \brief The connected functions.
        std::vector<fun> observers;
    };
} // namespace mango

#endif // MANGO_SIGNAL_HPP