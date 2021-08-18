//! \file      helpers.hpp
//! This file provides helpers functionality for user structures.
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
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

    //! \brief Checks if the given pointer is created (not null) and gives proper output, in case it is not.
    //! \param[in] ptr The const pointer to check.
    //! \param[in] what The name of the checked object. Used for output.
    bool check_creation(const void* ptr, const string& what);
    //! \brief Checks if the given pointer is mapped (not null) and gives proper output, in case it is not.
    //! \param[in] ptr The const pointer to check.
    //! \param[in] what The name of the checked object. Used for output.
    bool check_mapping(const void* ptr, const string& what);
    //! \brief Checks if the given pointer is acquired (not null) and gives proper output, in case it is not.
    //! \param[in] ptr The const pointer to check.
    //! \param[in] what The name of the checked object. Used for output.
    bool check_acquisition(const void* ptr, const string& what);

    //! \brief Macro used to disable copy and assignment for a class or structure.
#ifndef MANGO_DISABLE_COPY_AND_ASSIGNMENT
#define MANGO_DISABLE_COPY_AND_ASSIGNMENT(classname) \
  private:                                           \
    classname(const classname&);                     \
    classname& operator=(const classname&);
#endif

} // namespace mango

#endif // MANGO_HELPERS_HPP