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
        inline void create(int32 buffer_count)
        {
            m_buffer_count = buffer_count;
        }

        inline void lock_range(int32 start, int32 end, const graphics_device_context_handle& context)
        {
            range_block block;
            block.start     = start;
            block.end       = end;
            block.semaphore = context->fence(semaphore_create_info());
            m_blocks.push_back(block);
            m_current_offset = end + 1;
        }

        inline int32 wait_for_range(int32 count, const graphics_device_context_handle& context)
        {
            MANGO_ASSERT(count <= m_buffer_count, "Waiting for range bigger then buffer!");
            int32 start = m_current_offset;
            if (m_current_offset + count > m_buffer_count)
            {
                start = 0;
            }
            int end = start + count;
            for (auto it = m_blocks.begin(); it != m_blocks.end();)
            {
                auto& block = *it;
                if (block.start <= end && block.end >= start)
                {
                    context->client_wait(block.semaphore);
                    it = m_blocks.erase(it);
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
        int32 m_buffer_count;

        int32 m_current_offset;
    };
} // namespace mango

#endif // MANGO_RING_BUFFER_MANAGER_HPP
