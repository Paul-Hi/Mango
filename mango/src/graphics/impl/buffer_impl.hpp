//! \file      buffer_impl.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_BUFFER_IMPL_HPP
#define MANGO_BUFFER_IMPL_HPP

#include <graphics/buffer.hpp>

namespace mango
{
    //! \brief The implementation of the \a buffer.
    class buffer_impl : public buffer
    {
      public:
        //! \brief Constructs the \a buffer_impl.
        //! \param[in] configuration The \a buffer_configuration.
        buffer_impl(const buffer_configuration& configuration);
        ~buffer_impl();

        inline ptr_size byte_length() override
        {
            return m_size;
        }

        void set_data(format internal_format, g_intptr offset, g_sizeiptr size, format pixel_format, format type, const void* data) override;
        void bind(buffer_target target, g_uint index, g_intptr offset, g_sizeiptr size) override;
        void* map(g_intptr offset, g_sizeiptr length, buffer_access access) override;
        void unmap() override;

      private:
        //! \brief The persistent data, if persistent mapping is requested.
        void* m_persistent_data;

        //! \brief The size of the memory.
        ptr_size m_size;

        //! \brief The target of the \a buffer. Used as a hint and fallback.
        g_enum m_target;

        //! \brief The access flags specified for the \a buffer.
        g_bitfield m_access_flags;
    };
} // namespace mango

#endif // MANGO_BUFFER_IMPL_HPP
