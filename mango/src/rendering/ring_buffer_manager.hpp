//! \file      ring_buffer_manager.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2022
//! \copyright Apache License 2.0

#ifndef MANGO_RING_BUFFER_MANAGER_HPP
#define MANGO_RING_BUFFER_MANAGER_HPP

#include <graphics/graphics.hpp>
#include <mango/types.hpp>

namespace mango
{
    class ring_buffer_manager
    {
      public:
        inline void create(int32 byte_size)
        {
            m_byte_size      = byte_size;
            m_current_offset = 0;
        }

        inline void lock_range(int32 start, int32 end, int32 next_offset, const graphics_device_context_handle& context)
        {
            range_block block;
            block.start     = start;
            block.end       = end;
            block.semaphore = context->fence(semaphore_create_info());
            m_blocks.push_back(block);

            m_current_offset = end + next_offset;

            // align current offset to at least 64
            // TODO Paul: This has to be done better!s
            int32 mask         = 64 - 2;
            int32 misalignment = m_current_offset & mask;
            m_current_offset =  m_current_offset + (64 - misalignment);
        }

        inline int32 wait_for_range(int32 byte_size, const graphics_device_context_handle& context)
        {
            MANGO_ASSERT(byte_size <= m_byte_size, "Waiting for range bigger then buffer!");
            int32 start = m_current_offset;
            if (m_current_offset + byte_size > m_byte_size)
            {
                start = 0;
            }
            int end = start + byte_size;
            for (auto it = m_blocks.begin(); it != m_blocks.end();)
            {
                auto& block = *it;
                if (block.start <= end && block.end >= start)
                {
                    context->client_wait(block.semaphore);
                    it = m_blocks.erase(it);
                }
                else
                {
                    it++;
                }
            }
            return start;
        }

      private:
        struct range_block
        {
            int32 start;
            int32 end;
            gfx_handle<const gfx_semaphore> semaphore;
        };

        std::vector<range_block> m_blocks;
        int32 m_byte_size;

        int32 m_current_offset;
    };
} // namespace mango

#endif // MANGO_RING_BUFFER_MANAGER_HPP
