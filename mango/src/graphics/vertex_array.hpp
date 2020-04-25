//! \file      vertex_array.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_VERTEX_ARRAY_HPP
#define MANGO_VERTEX_ARRAY_HPP

#include <graphics/graphics_object.hpp>

namespace mango
{
    //! \brief A vertex array graphics object.
    class vertex_array : public graphics_object
    {
      public:
        //! \brief Creates a new \a vertex_array and returns it as a shared_ptr.
        //! \return The shared_ptr to the created \a vertex_array.
        static vertex_array_ptr create();

        //! \brief Binds a \a buffer to the \a vertex_array used as an vertex buffer.
        //! \param[in] index The vertex buffer binding index the buffer should be bound to.
        //! \param[in] buffer A shared_ptr to to the vertex buffer which should be bound.
        //! \param[in] offset The offset of the first element in \a buffer.
        //! \param[in] stride The stride of each element in \a buffer.
        virtual void bind_vertex_buffer(g_uint index, buffer_ptr buffer, g_intptr offset, g_sizei stride) = 0;

        //! \brief Binds a \a buffer to the \a vertex_array used as an index buffer (element array buffer).
        //! \param[in] buffer A shared_ptr to to the index buffer which should be bound.
        virtual void bind_index_buffer(buffer_ptr buffer) = 0;

        //! \brief Sets one vertex attribute in a vertex buffer.
        //! \details Enables the vertex attribute and sets all necessary format specific data.
        //! \param[in] index The index of the vertex attribute.
        //! \param[in] buffer_index The index of the vertex buffer the vertex attribute should be enabled in.
        //! \param[in] attribute_format The \a format of the vertex attribute. Has to be \a R8, \a R16, \a R16F, \a R32F, \a R8I, \a R16I, \a R32I, \a R8UI, \a R16UI, \a R32UI, \a RG8, \a RG16, \a
        //! RG16F, \a RG32F, \a RG8I, \a RG16I, \a RG32I, \a RG8UI, \a RG16UI, \a RG32UI, \a RGB32F, \a RGB32I, \a RGB32UI, \a RGBA8, \a RGBA16, \a RGBA16F, \a RGBA32F, \a RGBA8I, \a RGBA16I, \a
        //! RGBA32I, \a RGBA8UI, \a RGBA16UI or \a RGBA32UI.
        //! \param[in] relative_offset The offset relative to the whole vertex (if existent).
        virtual void set_vertex_attribute(g_uint index, g_uint buffer_index, format attribute_format, g_uint relative_offset) = 0;

      protected:
        vertex_array()  = default;
        ~vertex_array() = default;
    };
} // namespace mango

#endif // MANGO_VERTEX_ARRAY_HPP
