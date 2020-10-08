//! \file      uniform_buffer.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_UNIFORM_BUFFER
#define MANGO_UNIFORM_BUFFER

#include <graphics/buffer.hpp>
#include <graphics/command_buffer.hpp>

namespace mango
{
// These buffer bindings points are reserved
#define UB_SLOT_RENDERER_FRAME 0
#define UB_SLOT_LIGHTING_PASS_DATA 3

#define UB_SLOT_MODEL_DATA 1
#define UB_SLOT_MATERIAL_DATA 2

// Shared buffer binding points
#define UB_SLOT_SHADOW_DATA 4
#define UB_SLOT_IBL_DATA 4

    enum class buffer_technique : uint8
    {
        single_buffering,
        double_buffering,
        triple_buffering,
        count,
    };

    class bind_uniform_buffer_cmd : public command
    {
      public:
        buffer_ptr m_buffer;
        int64 m_offset;
        int64 m_size;
        int32 m_slot;
        bind_uniform_buffer_cmd(buffer_ptr buffer, int64 offset, int64 size, int32 slot)
            : m_buffer(buffer)
            , m_offset(offset)
            , m_size(size)
            , m_slot(slot)
        {
        }

        bind_uniform_buffer_cmd(const bind_uniform_buffer_cmd& other)
        {
            if (this != &other)
            {
                m_buffer = other.m_buffer;
                m_offset = other.m_offset;
                m_size   = other.m_size;
                m_slot   = other.m_slot;
            }
        }

        void execute(graphics_state&) override
        {
            GL_NAMED_PROFILE_ZONE("Bind Uniform Buffer");
            MANGO_ASSERT(m_buffer, "Uniform Buffer does not exist anymore.");
            m_buffer->bind(buffer_target::uniform_buffer, m_slot, m_offset, m_size);
        }
    };

    class uniform_buffer
    {
      public:
        static uniform_buffer_ptr create()
        {
            return std::make_shared<uniform_buffer>();
        };

        uniform_buffer();
        ~uniform_buffer();

        bool init(int64 frame_size, buffer_technique technique);

        void begin_frame(command_buffer_ptr& command_buffer);
        void end_frame(command_buffer_ptr& command_buffer);

        bind_uniform_buffer_cmd bind_uniform_buffer(int32 slot, int64 size, void* data);

        inline float get_occupancy()
        {
            return 100.0f * static_cast<float>(m_last_offset) / static_cast<float>(m_frame_size);
        }

      private:
        int64 m_uniform_buffer_size;
        int64 m_frame_size;
        buffer_technique m_technique;

        int32 m_current_buffer_part;
        int64 m_current_buffer_start;
        int64 m_global_offset;
        int64 m_local_offset;
        int64 m_last_offset;
        g_int m_uniform_buffer_alignment;

        buffer_ptr m_uniform_buffer;
        void* m_mapping;

        g_sync m_buffer_sync_objects[3];
    };
} // namespace mango

#endif // MANGO_UNIFORM_BUFFER