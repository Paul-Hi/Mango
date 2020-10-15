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
#define SSB_SLOT_EXPOSURE 4

    enum class buffer_technique : uint8
    {
        single_buffering,
        double_buffering,
        triple_buffering,
        count,
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

        g_sync* prepare();
        g_sync* end_frame();

        int64 write_data(int64 size, void* data);

        inline g_uint buffer_name()
        {
            return m_uniform_buffer->get_name();
        }

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