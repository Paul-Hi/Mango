//! \file      linear_allocator.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <mango/assert.hpp>
#include <memory/linear_allocator.hpp>

using namespace mango;

linear_allocator::linear_allocator(const int64 size)
    : allocator(size)
    , m_start(nullptr)
{
}

linear_allocator::~linear_allocator()
{
    free(m_start);
    m_start = nullptr;
}

void linear_allocator::init()
{
    if (m_start)
        free(m_start);

    m_start  = malloc(m_total_size);
    m_offset = 0;
}

void* linear_allocator::allocate(const int64 size, const int64 alignment)
{
    int64 address = reinterpret_cast<int64>(m_start) + m_offset;
    int64 padding = 0;

    if(alignment > 0 && m_offset % alignment != 0)
    {
        padding = calculate_padding(address, alignment);
    }

    if(m_offset + padding + size > m_total_size)
    {
        MANGO_LOG_ERROR("Linear Allocator Out Of Memory!");
        return nullptr;
    }

    m_offset += (padding + size);
    address += padding;

    return reinterpret_cast<void*>(address);
}

void linear_allocator::free_memory(void*)
{
    MANGO_ASSERT(false, "Linear Allocator can not free single blocks, use reset() instead!");
}

void linear_allocator::reset()
{
    m_offset = 0;
}
