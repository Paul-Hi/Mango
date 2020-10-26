//! \file      free_list_allocator.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_FREE_LIST_ALLOCATOR_HPP
#define MANGO_FREE_LIST_ALLOCATOR_HPP

#include <memory/allocator.hpp>

namespace mango
{
    struct free_list_memory_block
    {
        int64 size;
        free_list_memory_block* next;
        uintptr data[1];
    };
    class free_list_allocator : public allocator
    {
      public:
        free_list_allocator(const int64 size);
        ~free_list_allocator();

        void reset() override;

      private:
        free_list_memory_block* m_head;

        virtual int64 allocate_unaligned(const int64 size) override;
        void free_memory_unaligned(void* mem) override;

        free_list_memory_block* first_fit(int64 size);
        void split(int64 wanted, int64 got, free_list_memory_block* current, free_list_memory_block* last, free_list_memory_block* next);
        void coalesce(free_list_memory_block* last, free_list_memory_block* current, free_list_memory_block* next);

        free_list_memory_block* create_start_block();
    };
} // namespace mango

#endif // MANGO_FREE_LIST_ALLOCATOR_HPP
