//! \file      log.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_LOG_HPP
#define MANGO_LOG_HPP

#include <spdlog/spdlog.h>

namespace mango
{
    //! \namespace mango::log A namespace used to enable logging capabilities.
    namespace log
    {
        //! \brief The log level that can be specified for logging.
        //! \details \a info and \a critical are enabled anytime,
        //! \a trace, \a debug, \a warn and \a error are only enabled in debug.
        enum level
        {
            info,
            trace,
            debug,
            warn,
            error,
            critical
        };

        //! \brief The core of the logging system.
        //! \details This function can be used to log to the console with different log levels.
        //! The message can contain arguments "{0}" to "{n}"" which are specified in args.
        //! \param[in] level The log level in { info, debug, trace, warn, error, critical }.
        //! \param[in] message The message string to display containing optional arguments.
        //! \param[in] args 0 to n optional arguments that will be displayed in \a message.
        template <typename... Args>
        inline void message(level level, const char* message, const Args&... args)
        {
#ifdef MANGO_DEBUG
            spdlog::set_level(spdlog::level::trace);
#endif
            spdlog::set_pattern("%^ [%l] [%X]  %v %$");

            switch (level)
            {
            case info:
                spdlog::info(message, args...);
                break;
            case trace:
                spdlog::trace(message, args...);
                break;
            case debug:
                spdlog::debug(message, args...);
                break;
            case warn:
                spdlog::warn(message, args...);
                break;
            case error:
                spdlog::error(message, args...);
                break;
            case critical:
                spdlog::critical(message, args...);
                break;
            default:
                spdlog::error("Unkown Log Level!");
            }
        }
    } // namespace log
} // namespace mango

//! \brief Log macro with info level.
#define MANGO_LOG_INFO(...) log::message(log::level::info, __VA_ARGS__)
//! \brief Log macro with error level.
#define MANGO_LOG_ERROR(...) log::message(log::level::error, __VA_ARGS__)
//! \brief Log macro with critical level.
#define MANGO_LOG_CRITICAL(...) log::message(log::level::critical, __VA_ARGS__)

#if defined(MANGO_DEBUG) || defined(MANGO_DOCUMENTATION)
//! \brief Log macro with tracing level.
#define MANGO_LOG_TRACE(...) log::message(log::level::trace, __VA_ARGS__)
//! \brief Log macro with debug level.
#define MANGO_LOG_DEBUG(...) log::message(log::level::debug, __VA_ARGS__)
//! \brief Log macro with warn level.
#define MANGO_LOG_WARN(...) log::message(log::level::warn, __VA_ARGS__)
#else
//! \brief Log macro with tracing level.
#define MANGO_LOG_TRACE(...)
//! \brief Log macro with debug level.
#define MANGO_LOG_DEBUG(...)
//! \brief Log macro with warn level.
#define MANGO_LOG_WARN(...)
#endif // MANGO_DEBUG || MANGO_DOCUMENTATION


#endif // MANGO_LOG_HPP
