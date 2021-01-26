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
    //! \brief A block used for the \a free_list_allocator internal linked list.
    struct free_list_memory_block
    {
        int64 size; //!< Size of the block.
        free_list_memory_block* next; //!< Pointer to the next block.
        uintptr data[1]; //!< Pointer to the block data.
    };
    //! \brief An allocator with an internal linked list.
    //! \details Allocates memory block on init and manages it with a linked list.
    class free_list_allocator : public allocator
    {
      public:
        //! \brief Constructs the \a free_list_allocator.
        //! \details Does not allocate any memory. To use the allocator init() has to be called.
        //! \param[in] size The size of the memory to manage.
        free_list_allocator(const int64 size);
        ~free_list_allocator();

        void reset() override;

      private:
        //! \brief Pointer to the head of the internal linked list.
        free_list_memory_block* m_head;

        virtual int64 allocate_unaligned(const int64 size) override;
        void free_memory_unaligned(void* mem) override;

        //! \brief Does a search on th internal linked list and returns the first fitting block.
        //! \param[in] size The required size to search for.
        //! \return The first fitting \a free_list_memory_block or nullptr if out of memory.
        free_list_memory_block* first_fit(int64 size);

        //! \brief Splits blocks into two after finding a fitting block beeing to large.
        //! \param[in] wanted The required searched size.
        //! \param[in] got The found size of the fitting block.
        //! \param[in] last The last block in relation to the current one.
        //! \param[in] current The current block with the fitting size.
        //! \param[in] next The next block in relation to the current one.
        void split(int64 wanted, int64 got, free_list_memory_block* last, free_list_memory_block* current, free_list_memory_block* next);

        //! \brief Merges blocks after freeing a block.
        //! \param[in] last The last block in relation to the current one.
        //! \param[in] current The current block that was freed.
        //! \param[in] next The next block in relation to the current one.
        void coalesce(free_list_memory_block* last, free_list_memory_block* current, free_list_memory_block* next);

        //! \brief Creates the head of the linked list.
        //! \return The \a free_list_memory_block to be used as head.
        free_list_memory_block* create_start_block();
    };
} // namespace mango

#endif // MANGO_FREE_LIST_ALLOCATOR_HPP
