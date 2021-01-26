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
    //! \brief Base class for memory managing classes.
    class allocator
    {
      public:
        //! \brief Constructs the \a allocator.
        //! \details Does not allocate any memory. To use the allocator init() has to be called.
        //! \param[in] size The size of the memory to manage.
        allocator(const int64 size)
            : m_total_size(size)
            , m_start(nullptr)
        {
        }

        ~allocator()
        {
            m_total_size = 0;
            free(m_start);
        }

        //! \brief Initializes the \a allocator.
        //! \details Before using the allocator this has to be called.
        virtual void init()
        {
            if (m_start)
            {
                free(m_start);
                m_start = nullptr;
            }
            m_start = malloc(m_total_size);
            if(!m_start)
                MANGO_LOG_ERROR("Malloc failed! Allocator broken!");

            reset();
        }

        //! \brief Resets the \a allocator. All memory allocated is invalid after that.
        virtual void reset() = 0;

        //! \brief Allocates memory of a specific size.
        //! \param[in] size The size in bytes to allocate.
        //! \return A void* to the allocated memory.
        virtual void* allocate(const int64 size)
        {
            int64 unaligned_address = allocate_unaligned(size);
            return reinterpret_cast<void*>(unaligned_address);
        }

        //! \brief Allocates memory of a specific size with alignment.
        //! \param[in] size The size in bytes to allocate.
        //! \param[in] alignment The alignment in byte. Has to be a multiple of 2. Basis value is 2.
        //! \return A void* to the allocated memory.
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

        //! \brief Frees memory.
        //! \param[in] mem The memory to free.
        virtual void free_memory(void* mem)
        {
            free_memory_unaligned(mem);
        }

        //! \brief Frees aligned memory.
        //! \details This should be called, when the memory got allocated with allocate_aligned().
        //! \param[in] mem The memory to free.
        virtual void free_memory_aligned(void* mem)
        {
            uint8* aligned_memory   = reinterpret_cast<uint8*>(mem);
            int64 aligned_address   = reinterpret_cast<int64>(mem);
            int64 adjustment        = static_cast<int64>(aligned_memory[-1]);
            int64 unaligned_address = aligned_address - adjustment;
            free_memory_unaligned(reinterpret_cast<void*>(unaligned_address));
        }

      protected:
        //! \brief Total size of memory managed by the \a allocator in bytes.
        int64 m_total_size;
        //! \brief Pointer to the start of the preallocated memory block.
        void* m_start;

        //! \brief Allocates memory unaligned of a specific size.
        //! \details Internal function overwritten by the derived allocators.
        //! \param[in] size The size in bytes to allocate.
        //! \return A memory address where the allocated memory starts.
        virtual int64 allocate_unaligned(const int64 size) = 0;

        //! \brief Frees memory unaligned.
        //! \details Internal function overwritten by the derived allocators.
        //! \param[in] mem The memory to free.
        virtual void free_memory_unaligned(void* mem) = 0;

        //! \brief Calculates the adjustment needed to align a given adress for a specific alignment.
        //! \param[in] unaligned_address The adress to calculate the alignment adjustment for.
        //! \param[in] alignment The alignment to calculate the adjustment from.
        //! \return The adjustment needed to align unaligned_adress with alignment.
        int64 calculate_adjustment(int64 unaligned_address, const int64 alignment)
        {
            int64 mask         = alignment - 2;
            int64 misalignment = unaligned_address & mask;
            return alignment - misalignment;
        }
    };
} // namespace mango

#endif // MANGO_ALLOCATOR_HPP
