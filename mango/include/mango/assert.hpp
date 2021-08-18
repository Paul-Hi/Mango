//! \file      assert.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_ASSERT_HPP
#define MANGO_ASSERT_HPP

#include <iostream>
#include <mango/log.hpp>

#if defined(MANGO_DEBUG) || defined(MANGO_DOCUMENTATION)

#ifdef MANGO_TEST

//! \brief Macro for assertions.
//! \details Checks and prints out the expression on fail.
//! An additional message can be added.
//! If the asserted expression is true, nothing happens.
//! Assertions are only enabled in debug mode.
#define MANGO_ASSERT(expression, ...)                                                                                                                                                            \
    ((void)(!(expression) && (MANGO_LOG_CRITICAL("\nAssertion '{0}' failed in function {1}, file {2}, line {3}.\nMessage: '{4}'", #expression, __func__, __FILE__, __LINE__, __VA_ARGS__), 1) && \
            (std::abort(), 1)))
#else

//! \brief Macro for assertions.
//! \details Checks and prints out the expression on fail.
//! An additional message can be added.
//! If the asserted expression is true, nothing happens.
//! Assertions are only enabled in debug mode.
#define MANGO_ASSERT(expression, ...)                                                                                                                                                   \
    ((void)(!(expression) &&                                                                                                                                                            \
            (MANGO_LOG_CRITICAL("\nAssertion '{0}' failed in function {1}, file {2}, line {3}.\nMessage: '{4}. Pause.'", #expression, __func__, __FILE__, __LINE__, __VA_ARGS__), 1) && \
            (std::cin.get(), 1) && (std::abort(), 1)))

#endif // MANGO_TEST

#else

//! \brief Macro for assertions.
//! \details Checks and prints out the expression on fail.
//! An additional message can be added.
//! If the asserted expression is true, nothing happens.
//! Assertions are only enabled in debug mode.
#define MANGO_ASSERT(expression, ...) \
    do                                \
    {                                 \
        (void)sizeof(expression);     \
    } while (0)

#endif // MANGO_DEBUG || MANGO_DOCUMENTATION

#endif // MANGO_ASSERT_HPP
