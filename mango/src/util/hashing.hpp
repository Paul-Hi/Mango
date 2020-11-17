//! \file      hashing.hpp
//! This file provides hashing functionality for user structures.
//! There is also an implementation of the fnv1a hash function.
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_HASHING_HPP
#define MANGO_HASHING_HPP

#include <mango/types.hpp>
#include <type_traits>

namespace mango
{
    class djb2_string_hash
    {
      public:
        static uint64 hash(const char* str)
        {
            uint64 hash = 5381;
            int64 c;
            while (c = *str++)
                hash = ((hash << 5) + hash) + c; // hash * 33 + c
            return hash;
        }
    };
} // namespace mango

#endif // MANGO_HASHING_HPP