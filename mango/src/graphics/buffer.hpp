//! \file      buffer.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_BUFFER_HPP
#define MANGO_BUFFER_HPP

#include <graphics/graphics_object.hpp>

namespace mango
{
    //! \brief A configuration for \a buffers.
    class buffer_configuration : public graphics_configuration
    {
      public:
        //! \brief Constructs a new \a buffer_configuration with default values.
        buffer_configuration()
            : graphics_configuration()
        {
        }

        //! \brief Constructs a new \a buffer_configuration.
        //! \param[in] size The size for the \a buffer to create.
        //! \param[in] target The \ buffer_target hint for the \a buffer to create.
        //! \param[in] access The \a buffer_access for the \a buffer to create.
        //! \param[in] data Optional data for the \a buffer to create.
        buffer_configuration(ptr_size size, buffer_target target, buffer_access access, void* data = nullptr)
            : graphics_configuration()
            , m_size(size)
            , m_target(target)
            , m_access(access)
        {
          MANGO_UNUSED(data);
        }

        //! \brief Size information.
        ptr_size m_size = 0;
        //! \brief Target information.
        buffer_target m_target = buffer_target::NONE;
        //! \brief Access information.
        buffer_access m_access = buffer_access::NONE;
        //! \brief Data.
        const void* m_data = nullptr;

        bool is_valid() const
        {
            return m_size && (m_access != buffer_access::NONE || m_data); // Some buffers do not need a target specified.
        }
    };

    //! \brief Memory object.
    //! \details Used to share data between cpu and gpu devices.
    //! Can be mapped, bound etc.
    class buffer : public graphics_object
    {
      public:
        //! \brief Creates a new \a buffer and returns a pointer to it.
        //! \param[in] configuration The \a buffer_configuration for the new \a buffer.
        //! \return A pointer to the new \a buffer.
        static buffer_ptr create(const buffer_configuration& configuration);

        //! \brief Returns the size of the \a buffer in bytes.
        //! \return Size of the \a buffer in bytes.
        virtual ptr_size byte_length() = 0;

        //! \brief Sets the data of the \a buffer.
        //! \details On creation the flag \a buffer_access::DYNAMIC_STORAGE has to be specified.
        //! \param[in] internal_format The internal \a buffer \a format to use. Has to be \a R8, \a R16, \a R16F, \a R32F, \a R8I, \a R16I, \a R32I, \a R8UI, \a R16UI, \a R32UI, \a RG8, \a RG16, \a
        //! RG16F, \a RG32F, \a RG8I, \a RG16I, \a RG32I, \a RG8UI, \a RG16UI, \a RG32UI, \a RGB32F, \a RGB32I, \a RGB32UI, \a RGBA8, \a RGBA16, \a RGBA16F, \a RGBA32F, \a RGBA8I, \a RGBA16I, \a
        //! RGBA32I, \a RGBA8UI, \a RGBA16UI or \a RGBA32UI.
        //! \param[in] offset The offset in the \a buffer where the data should start.
        //! \param[in] size The size of the data to put into the \a buffer.
        //! \param[in] pixel_format The pixel \a format. Do not question the naming. It's OpenGL. Has to be \a DEPTH_COMPONENT, \a STENCIL_INDEX, \a DEPTH_STENCIL, \a RED, \a GREEN, \a BLUE, \a RG, \a
        //! RGB, \a BGR, \a RGBA, \a BGRA, \a RED_INTEGER, \a GREEN_INTEGER, \a BLUE_INTEGER, \a RG_INTEGER, \a RGB_INTEGER, \a BGR_INTEGER, \a RGBA_INTEGER or \a BGRA_INTEGER
        //! \param[in] type The type of the data. Has to be \a \a UNSIGNED_BYTE, \a BYTE, \a UNSIGNED_SHORT, \a SHORT, \a UNSIGNED_INT, \a INT, \a FLOAT, \a UNSIGNED_BYTE_3_3_2,
        //! \a UNSIGNED_BYTE_2_3_3_REV, \a UNSIGNED_SHORT_5_6_5, \a UNSIGNED_SHORT_5_6_5_REV, \a UNSIGNED_SHORT_4_4_4_4, \a UNSIGNED_SHORT_4_4_4_4_REV, \a UNSIGNED_SHORT_5_5_5_1,
        //! \a UNSIGNED_SHORT_1_5_5_5_REV, \a UNSIGNED_INT_8_8_8_8, \a UNSIGNED_INT_8_8_8_8_REV, \a UNSIGNED_INT_10_10_10_2 or \a UNSIGNED_INT_2_10_10_10_REV.
        //! \param[in] data The data to set the memory specified before to. ATTENTION: This is only one value, that gets replicated.
        virtual void set_data(format internal_format, g_intptr offset, g_sizeiptr size, format pixel_format, format type, const void* data) = 0;

        //! \brief Binds the \a buffer to a specific target.
        //! \param[in] target The \a buffer_target to bind the \a buffer to.
        //! \param[in] index The buffer index to bind the \a buffer to.
        //! \param[in] offset The offset in the \a buffer to start the binding from.
        //! \param[in] size The size to bind. Leave empty if the \a buffer should be bound from offset to end.
        virtual void bind(buffer_target target, g_uint index, g_intptr offset = 0, g_sizeiptr size = MAX_G_SIZE_PTR_SIZE) = 0;

        //! \brief Maps part of the \a buffer and returns it.
        //! \details On creation a flag with \a buffer_access::MAPPED_ACCESS_* has to be specified.
        //! We do always map persistent if we map.
        //! \param[in] offset The offset in the \a buffer to start the binding from.
        //! \param[in] length The length to map.
        //! \param[in] access The \a buffer_access- This has to be the same as specified on creation.
        //! \return A mapping of the specified \a buffer part. Is persistent.
        virtual void* map(g_intptr offset, g_sizeiptr length, buffer_access access) = 0;

        //! \brief Unmaps the \a buffer. Not used at the moment, because we map persistent.
        virtual void unmap() = 0;

        //! \brief Locks the \a buffer. This places a fence that will be removed when commands are finished on gpu.
        virtual void lock() = 0;

        //! \brief Waits for the \a buffer until it is not longer used by gpu.
        virtual void request_wait() = 0;

      protected:
        buffer()  = default;
        ~buffer() = default;
    };
} // namespace mango

#endif // MANGO_BUFFER_HPP
