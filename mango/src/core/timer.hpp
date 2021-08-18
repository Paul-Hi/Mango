//! \file      timer.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_TIMER_HPP
#define MANGO_TIMER_HPP

#include <chrono>

namespace mango
{
    //! \cond NO_DOC
    using clock_type = typename std::conditional<std::chrono::high_resolution_clock::is_steady, std::chrono::high_resolution_clock, std::chrono::steady_clock>::type;
    //! \endcond
    //! \brief Typedef of a timestep.
    using timestep = std::chrono::time_point<clock_type>;

    //! \brief Some small timer class.
    class timer
    {
      public:
        ~timer() = default;

        //! \brief Returns the elapsed time since the last start() call.
        //! \return The elapsed time since the last start() call in microseconds.
        inline std::chrono::microseconds elapsedMicroseconds() const
        {
            return std::chrono::duration_cast<std::chrono::microseconds>(clock_type::now() - m_start_time);
        }

        //! \brief Returns the elapsed time since the last start() call.
        //! \return The elapsed time since the last start() call in milliseconds.
        inline std::chrono::milliseconds elapsedMilliseconds() const
        {
            return std::chrono::duration_cast<std::chrono::milliseconds>(clock_type::now() - m_start_time);
        }

        //! \brief Returns the elapsed time since the last start() call.
        //! \return The elapsed time since the last start() call in seconds.
        inline std::chrono::seconds elapsedSeconds() const
        {
            return std::chrono::duration_cast<std::chrono::seconds>(clock_type::now() - m_start_time);
        }

        //! \brief Starts the \a timer.
        inline void start()
        {
            m_start_time = clock_type::now();
        }

        //! \brief Restarts the \a timer.
        inline void restart()
        {
            m_start_time = clock_type::now();
        }

      private:
        //! \brief The time the Timer started.
        timestep m_start_time;
    };
} // namespace mango

#endif // MANGO_TIMER_HPP
