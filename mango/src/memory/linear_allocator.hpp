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
    class linear_allocator : public allocator
    {
      public:
        linear_allocator(const int64 size);
        ~linear_allocator();

        void reset() override;

      private:
        int64 m_offset;

        virtual int64 allocate_unaligned(const int64 size) override;
        void free_memory_unaligned(void* mem) override;
    };
} // namespace mango

#endif // MANGO_LINEAR_ALLOCATOR_HPP
