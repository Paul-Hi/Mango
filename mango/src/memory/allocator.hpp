//! \file      allocator.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_ALLOCATOR_HPP
#define MANGO_ALLOCATOR_HPP

#include <mango/types.hpp>

namespace mango
{
    class allocator
    {
      public:
        allocator(const int64 size)
            : m_total_size(size)
        {
        }

        ~allocator()
        {
            m_total_size = 0;
        }

        virtual void init() = 0;

        virtual void* allocate(const int64 size, const int64 alignment = 0) = 0;

        virtual void free_memory(void*) = 0;

      protected:
        int64 m_total_size;

        const int64 calculate_padding(int64 base_adress, const int64 alignment)
        {
            return (base_adress + alignment - 1) & ~(alignment - 1);
        }
    };
} // namespace mango

#endif // MANGO_ALLOCATOR_HPP
