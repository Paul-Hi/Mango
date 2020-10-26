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
            , m_start(nullptr)
        {
        }

        ~allocator()
        {
            m_total_size = 0;
        }

        virtual void init()
        {
            if (m_start)
            {
                free(m_start);
                m_start = nullptr;
            }
            m_start = malloc(m_total_size);

            reset();
        }

        virtual void reset() = 0;

        virtual void* allocate(const int64 size)
        {
            int64 unaligned_address = allocate_unaligned(size);
            return reinterpret_cast<void*>(unaligned_address);
        }

        virtual void* allocate_aligned(const int64 size, const int64 alignment = 2)
        {
            MANGO_ASSERT(alignment >= 2, "Alignment has to be bigger or equal 2!");
            MANGO_ASSERT(alignment <= 128, "Alignment has to be smaller or equal 128!");
            MANGO_ASSERT(alignment % 2 == 0, "Alignment has to be power of two!");
            int64 expanded_size = size + alignment;

            int64 unaligned_address = allocate_unaligned(expanded_size);
            if (unaligned_address < 0)
                return nullptr;

            int64 adjustment = 0;

            adjustment = calculate_adjustment(unaligned_address, alignment);

            int64 aligned_address = unaligned_address + adjustment;

            MANGO_ASSERT(adjustment < 256, "Adjustment is wrong!");

            // store adjustment in last byte (needed for free)
            uint8* aligned_memory = reinterpret_cast<uint8*>(aligned_address);
            aligned_memory[-1]    = static_cast<uint8>(adjustment);

            return reinterpret_cast<void*>(aligned_memory);
        }

        virtual void free_memory(void* mem)
        {
            free_memory_unaligned(mem);
        }

        virtual void free_memory_aligned(void* mem)
        {
            uint8* aligned_memory   = reinterpret_cast<uint8*>(mem);
            int64 aligned_address   = reinterpret_cast<int64>(mem);
            int64 adjustment        = static_cast<int64>(aligned_memory[-1]);
            int64 unaligned_address = aligned_address - adjustment;
            free_memory_unaligned(reinterpret_cast<void*>(unaligned_address));
        }

      protected:
        void* m_start;
        int64 m_total_size;

        virtual int64 allocate_unaligned(const int64 size) = 0;

        virtual void free_memory_unaligned(void* mem) = 0;

        const int64 calculate_adjustment(int64 unaligned_address, const int64 alignment)
        {
            int64 mask         = alignment - 2;
            int64 misalignment = unaligned_address & mask;
            return alignment - misalignment;
        }
    };
} // namespace mango

#endif // MANGO_ALLOCATOR_HPP
