//! \file      free_list_allocator.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <mango/assert.hpp>
#include <memory/free_list_allocator.hpp>

using namespace mango;

free_list_allocator::free_list_allocator(const int64 size)
    : allocator(size)
{
}

free_list_allocator::~free_list_allocator()
{
    free(m_start);
    m_start = nullptr;
}

int64 free_list_allocator::allocate_unaligned(const int64 size)
{
    free_list_memory_block* fitting_block = first_fit(size);
    if (!fitting_block)
    {
        MANGO_LOG_ERROR("Free List Allocator Out Of Memory!");
        return -1;
    }
    return reinterpret_cast<int64>(fitting_block->data);
}

void free_list_allocator::free_memory_unaligned(void* mem)
{
    free_list_memory_block* free_block = reinterpret_cast<free_list_memory_block*>(reinterpret_cast<int64>(mem) - sizeof(free_list_memory_block) + sizeof(free_list_memory_block::data));
    free_list_memory_block* current    = m_head;
    free_list_memory_block* last       = nullptr;
    int64 free_pos                     = reinterpret_cast<int64>(free_block);
    while (reinterpret_cast<int64>(current) < free_pos)
    {
        last    = current;
        current = current->next;
        if (!current)
            break;
    }

    if (last)
        last->next = free_block;

    if (current)
        free_block->next = current;

    coalesce(last, free_block, current);
}

void free_list_allocator::reset()
{
    m_head = create_start_block();
}

free_list_memory_block* free_list_allocator::create_start_block()
{
    free_list_memory_block* block = static_cast<free_list_memory_block*>(m_start);
    block->size                   = m_total_size - sizeof(free_list_memory_block) + sizeof(free_list_memory_block::data);
    block->next                   = nullptr;
    return block;
}

free_list_memory_block* free_list_allocator::first_fit(int64 size)
{
    free_list_memory_block* current = m_head;
    free_list_memory_block* last    = nullptr;
    while (current->size < size)
    {
        last    = current;
        current = current->next;
        if (!current)
            return nullptr;
    }

    free_list_memory_block* next = current->next;

    if (current->size - size > static_cast<int64>(sizeof(free_list_memory_block))) // Smaller does not really make sense.
    {
        split(size, current->size, current, last, next);
        return current;
    }

    if (last)
        last->next = next;

    current->next = nullptr;
    return current;
}

void free_list_allocator::split(int64 wanted, int64 got, free_list_memory_block* current, free_list_memory_block* last, free_list_memory_block* next)
{
    free_list_memory_block* new_block = reinterpret_cast<free_list_memory_block*>(reinterpret_cast<int64>(current) + wanted + sizeof(free_list_memory_block) - sizeof(free_list_memory_block::data));
    new_block->size                   = got - wanted - sizeof(free_list_memory_block) + sizeof(free_list_memory_block::data);
    if (last)
        last->next = new_block;
    new_block->next = next;
}

void free_list_allocator::coalesce(free_list_memory_block* last, free_list_memory_block* current, free_list_memory_block* next)
{
    int64 current_pos = reinterpret_cast<int64>(current);

    if (last)
    {
        int64 last_pos = reinterpret_cast<int64>(last);
        int64 diff     = current_pos - (last_pos + sizeof(free_list_memory_block) - sizeof(free_list_memory_block::data) + last->size);

        if (diff == 0)
        {
            last->size += sizeof(free_list_memory_block) - sizeof(free_list_memory_block::data) + current->size;
            last->next  = next;
            current_pos = last_pos;
            current     = last;
        }
    }
    if (next)
    {
        int64 next_pos = reinterpret_cast<int64>(next);
        int64 diff     = next_pos - (current_pos + sizeof(free_list_memory_block) - sizeof(free_list_memory_block::data) + current->size);

        if (diff == 0)
        {
            current->size += sizeof(free_list_memory_block) - sizeof(free_list_memory_block::data) + next->size;
            current->next = next->next;
        }
    }
}
