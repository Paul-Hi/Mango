//! \file      vertex_array_impl.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_VERTEX_ARRAY_IMPL_HPP
#define MANGO_VERTEX_ARRAY_IMPL_HPP

#include <array>
#include <graphics/vertex_array.hpp>
#include <map>

namespace mango
{
    //! \brief Helper structure to cache vertex attributes.
    struct attr
    {
        format attribute_format;  //!< Format.
        ptr_size relative_offset; //!< Relative offset.
    };

    //! \brief Helper structure to cache vertex buffers.
    struct vertex_buffer_cache
    {
        buffer_ptr buf;                                      //!< The buffer.
        std::map<uint32 /*index*/, attr> enabled_attributes; //!< List of enabled attributes.
    };

    //! \endcond

    //! \brief The implementation of the \a vertex_array.
    class vertex_array_impl : public vertex_array
    {
      public:
        vertex_array_impl();
        ~vertex_array_impl();

        void bind_vertex_buffer(g_uint index, buffer_ptr buffer, g_intptr offset, g_sizei stride) override;
        void bind_index_buffer(buffer_ptr buffer) override;
        void set_vertex_attribute(g_uint index, g_uint buffer_index, format attribute_format, g_uint relative_offset) override;

      private:
        //! \brief List of bound vertex \a buffers for caching purpose. Maps \a buffer index to vertex buffer.
        std::array<vertex_buffer_cache, max_vertex_buffers> m_vertex_buffers;

        //! \brief The bound index \a buffer for caching purpose.
        buffer_ptr m_index_buffer;
    };
} // namespace mango

#endif // MANGO_VERTEX_ARRAY_IMPL_HPP