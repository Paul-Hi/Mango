//! \file      hashing.hpp
//! This file provides hashing functionality for user structures.
//! There is also an implementation of the fnv1a hash function.
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_HASHING_HPP
#define MANGO_HASHING_HPP

#include <mango/types.hpp>
#include <type_traits>

namespace mango
{
    //! \brief djb2_string_hash
    class djb2_string_hash
    {
      public:
        //! \brief Calculate the string hash for a given string.
        //! \param[in] str The string to hash.
        //! \return The hash.
        static uint64 hash(const char* str)
        {
            uint64 hash = 5381;
            int64 c     = *str++;
            while (c)
            {
                hash = ((hash << 5) + hash) + c; // hash * 33 + c
                c    = *str++;
            }
            return hash;
        }
    };
} // namespace mango

#endif // MANGO_HASHING_HPP