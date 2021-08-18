//! \file      linear_allocator.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_LINEAR_ALLOCATOR_HPP
#define MANGO_LINEAR_ALLOCATOR_HPP

#include <memory/allocator.hpp>

namespace mango
{
    //! \brief A linear allocator.
    //! \details Memory is allocated on init and returned memory is placed in a linear fashion. Freeing memory is not possible without reseting the allocator.
    class linear_allocator : public allocator
    {
      public:
        //! \brief Constructs the \a linear_allocator.
        //! \details Does not allocate any memory. To use the allocator init() has to be called.
        //! \param[in] size The size of the memory to manage.
        linear_allocator(const int64 size);
        ~linear_allocator();

        void reset() override;

      private:
        //! \brief The current offset from the memory start.
        int64 m_offset;

        virtual int64 allocate_unaligned(const int64 size) override;
        void free_memory_unaligned(void* mem) override;
    };
} // namespace mango

#endif // MANGO_LINEAR_ALLOCATOR_HPP
