//! \file      helpers.hpp
//! This file provides helpers functionality for user structures.
//! There is also an implementation of the fnv1a hash function.
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_HELPERS_HPP
#define MANGO_HELPERS_HPP

#include <mango/types.hpp>

namespace mango
{
    //! \brief Checks if the given pointer is created (not null) and gives proper output, in case it is not.
    //! \param[in] ptr The pointer to check.
    //! \param[in] what The name of the checked object. Used for output.
    bool check_creation(void* ptr, const string& what);
    //! \brief Checks if the given pointer is mapped (not null) and gives proper output, in case it is not.
    //! \param[in] ptr The pointer to check.
    //! \param[in] what The name of the checked object. Used for output.
    bool check_mapping(void* ptr, const string& what);
    //! \brief Checks if the given pointer is acquired (not null) and gives proper output, in case it is not.
    //! \param[in] ptr The pointer to check.
    //! \param[in] what The name of the checked object. Used for output.
    bool check_acquisition(void* ptr, const string& what);
} // namespace mango

#endif // MANGO_HELPERS_HPP