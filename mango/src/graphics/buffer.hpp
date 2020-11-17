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
        //! \param[in] size The size for the \a buffer to create. Has to be a positive value.
        //! \param[in] target The \a buffer_target hint for the \a buffer to create.
        //! \param[in] access The \a buffer_access for the \a buffer to create.
        //! \param[in] data Optional data for the \a buffer to create.
        buffer_configuration(int64 size, buffer_target target, buffer_access access, void* data = nullptr)
            : graphics_configuration()
            , size(size)
            , target(target)
            , access(access)
        {
          MANGO_UNUSED(data);
        }

        //! \brief Size information.
        int64 size = 0;
        //! \brief Target information.
        buffer_target target = buffer_target::none;
        //! \brief Access information.
        buffer_access access = buffer_access::none;
        //! \brief Data.
        const void* data = nullptr;

        bool is_valid() const
        {
            return (size > 0) && (access != buffer_access::none || data); // Some buffers do not need a target specified.
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
        virtual int64 byte_length() = 0;

        //! \brief Sets the data of the \a buffer.
        //! \details On creation the flag \a buffer_access::DYNAMIC_STORAGE has to be specified.
        //! \param[in] internal_format The internal \a buffer \a format to use. Has to be \a r8, \a r16, \a r16f, \a r32f, \a r8i, \a r16i, \a r32i, \a r8ui, \a r16ui, \a r32ui, \a rg8, \a rg16, \a
        //! rg16f, \a rg32f, \a rg8i, \a rg16i, \a rg32i, \a rg8ui, \a rg16ui, \a rg32ui, \a rgb32f, \a rgb32i, \a rgb32ui, \a rgba8, \a rgba16, \a rgba16f, \a rgba32f, \a rgba8i, \a rgba16i, \a
        //! rgba32i, \a rgba8ui, \a rgba16ui or \a rgba32ui.
        //! \param[in] offset The offset in the \a buffer where the data should start. Has to be a positive value.
        //! \param[in] size The size of the data to put into the \a buffer. Has to be a positive value.
        //! \param[in] pixel_format The pixel \a format. Do not question the naming. It's OpenGL. Has to be \a depth_component, \a stencil_index, \a depth_stencil, \a red, \a green, \a blue, \a rg, \a
        //! rgb, \a bgr, \a rgba, \a bgra, \a red_integer, \a green_integer, \a blue_integer, \a rg_integer, \a rgb_integer, \a bgr_integer, \a rgba_integer or \a bgra_integer
        //! \param[in] type the type of the data. has to be \a \a unsigned_byte, \a byte, \a unsigned_short, \a short, \a unsigned_int, \a int, \a float, \a unsigned_byte_3_3_2,
        //! \a unsigned_byte_2_3_3_rev, \a unsigned_short_5_6_5, \a unsigned_short_5_6_5_rev, \a unsigned_short_4_4_4_4, \a unsigned_short_4_4_4_4_rev, \a unsigned_short_5_5_5_1,
        //! \a unsigned_short_1_5_5_5_rev, \a unsigned_int_8_8_8_8, \a unsigned_int_8_8_8_8_rev, \a unsigned_int_10_10_10_2 or \a unsigned_int_2_10_10_10_rev.
        //! \param[in] data The data to set the memory specified before to. ATTENTION: This is only one value, that gets replicated.
        virtual void set_data(format internal_format, int64 offset, int64 size, format pixel_format, format type, const void* data) = 0;

        //! \brief Maps part of the \a buffer and returns it.
        //! \details On creation a flag with \a buffer_access::MAPPED_ACCESS_* has to be specified.
        //! We do always map persistent if we map.
        //! \param[in] offset The offset in the \a buffer to start the binding from. Has to be a positive value.
        //! \param[in] length The length to map. Has to be a positive value.
        //! \param[in] access The \a buffer_access- This has to be the same as specified on creation.
        //! \return A mapping of the specified \a buffer part. Is persistent.
        virtual void* map(int64 offset, int64 length, buffer_access access) = 0;

        //! \brief Unmaps the \a buffer. Not used at the moment, because we map persistent.
        virtual void unmap() = 0;

      protected:
        buffer()  = default;
        ~buffer() = default;
    };
} // namespace mango

#endif // MANGO_BUFFER_HPP
