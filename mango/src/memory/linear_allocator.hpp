//! \file      linear_allocator.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_LINEAR_ALLOCATOR_HPP
#define MANGO_LINEAR_ALLOCATOR_HPP

#include <memory/allocator.hpp>

namespace mango
{
    class linear_allocator : allocator
    {
      public:
        linear_allocator(const int64 size);
        ~linear_allocator();

        void init() override;

        void* allocate(const int64 size, const int64 alignment) override;

        void free_memory(void *) override;

        void reset();

      private:
        void* m_start;
        int64 m_offset;
    };
} // namespace mango

#endif // MANGO_LINEAR_ALLOCATOR_HPP
