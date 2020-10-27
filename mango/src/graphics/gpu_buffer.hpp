//! \file      gpu_buffer.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_GPU_BUFFER
#define MANGO_GPU_BUFFER

#include <graphics/buffer.hpp>
#include <graphics/command_buffer.hpp>

namespace mango
{
// These buffer bindings points are reserved

//! \brief Slot for the renderers uniform buffer.
#define UB_SLOT_RENDERER_FRAME 0
//! \brief Slot for the lighting data uniform buffer.
#define UB_SLOT_LIGHTING_PASS_DATA 3

//! \brief Slot for the model data uniform buffer.
#define UB_SLOT_MODEL_DATA 1
//! \brief Slot for the material uniform buffer.
#define UB_SLOT_MATERIAL_DATA 2
//! \brief Slot for the ibl step uniform buffer.
#define UB_SLOT_IBL_DATA 5

// Shared buffer binding points

//! \brief Slot for the shadow step uniform buffer.
#define UB_SLOT_SHADOW_DATA 4
//! \brief Slot for the shader storage buffer used for automatic exposure calculation.
#define SSB_SLOT_EXPOSURE 4

    //! \brief Structure describing various buffering techniques.
    enum class buffer_technique : uint8
    {
        single_buffering,
        double_buffering,
        triple_buffering,
        count,
    };

    //! \brief Buffer class mapping gpu buffers persistent and managing the memory per frame.
    class gpu_buffer
    {
      public:
        //! \brief Creates a new \a gpu_buffer.
        //! \return A shared_ptr to the create gpu_buffer.
        static gpu_buffer_ptr create()
        {
            return std::make_shared<gpu_buffer>();
        };

        //! \brief Constructs a \a gpu_buffer.
        //! \details Normally not called directly.
        gpu_buffer();
        ~gpu_buffer();

        //! \brief Initializes the \a  gpu_buffer.
        //! \param[in] frame_size Size of one frame.
        //! \param[in] technique The \a buffer_technique to use.
        //! \return True if init was successful, else False.
        bool init(int64 frame_size, buffer_technique technique);

        //! \brief Returns pointer to the g_sync value that needs to be unlocked for the next frame.
        //! \details This should be called, after finishing the current frame to prepare the next one.
        //! \return Pointer to the sync object to wait for.
        g_sync* prepare();

        //! \brief Returns pointer to the g_sync value that needs to be locked after the current frame.
        //! \details This should be called, after finishing the current frame.
        //! \return Pointer to the sync object to lock.
        g_sync* end_frame();

        //! \brief Gives the \a gpu_buffer data to write into memory.
        //! \param[in] size The size of data in bytes.
        //! \param[in] data Pointer to the data to write into the buffer memory.
        //! \return The offset in the buffer the data is written to.
        int64 write_data(int64 size, void* data);

        //! \brief Returns the gl name of the internally used buffer.
        //! \return The gl name of the internal buffer.
        inline g_uint buffer_name()
        {
            return m_gpu_buffer->get_name();
        }

        //! \brief Returns the buffer occupancy. Can be used for debugging.
        //! \details This is the occupancy per frame!
        //! \return The buffer occupancy in percent.
        inline float get_occupancy()
        {
            return 100.0f * static_cast<float>(m_last_offset) / static_cast<float>(m_frame_size);
        }

      private:
        //! \brief The managed size.
        int64 m_gpu_buffer_size;
        //! \brief The used frame size.
        int64 m_frame_size;
        //! \brief The used buffering technique.
        buffer_technique m_technique;

        //! \brief The id of the part of the buffer currently in use.
        int32 m_current_buffer_part;
        //! \brief Offset to the part of the buffer currently in use.
        int64 m_current_buffer_start;
        //! \brief The current global offset.
        int64 m_global_offset;
        //! \brief The current local offset (frame offset).
        int64 m_local_offset;
        //! \brief The last local offset (last frame offset).
        int64 m_last_offset;
        //! \brief Uniform buffer alignment. Queried from OpenGl.
        g_int m_uniform_buffer_alignment;

        //! \brief The internal buffer.
        buffer_ptr m_gpu_buffer;
        //! \brief The persistent mapping of the internal buffer.
        void* m_mapping;

        //! \brief List of sync objects.
        g_sync m_buffer_sync_objects[3];
    };
} // namespace mango

#endif // MANGO_GPU_BUFFER