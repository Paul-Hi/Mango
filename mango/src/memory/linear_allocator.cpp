//! \file      linear_allocator.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#include <mango/assert.hpp>
#include <memory/linear_allocator.hpp>

using namespace mango;

linear_allocator::linear_allocator(const int64 size)
    : allocator(size)
{
}

linear_allocator::~linear_allocator()
{
    free(m_start);
    m_start = nullptr;
}

int64 linear_allocator::allocate_unaligned(const int64 size)
{
    int64 address = reinterpret_cast<int64>(m_start) + m_offset;

    if (m_offset + size > m_total_size)
    {
        MANGO_LOG_ERROR("Linear Allocator Out Of Memory!");
        return -1;
    }

    m_offset += size;

    return address;
}

void linear_allocator::free_memory_unaligned(void*)
{
    MANGO_ASSERT(false, "Linear Allocator can not free single blocks, use reset() instead!");
}

void linear_allocator::reset()
{
    m_offset = 0;
}
