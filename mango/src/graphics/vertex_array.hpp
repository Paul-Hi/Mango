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
        //! \param[in] index The vertex buffer binding index the buffer should be bound to. Has to be a positive value.
        //! \param[in] buffer A shared_ptr to to the vertex buffer which should be bound.
        //! \param[in] offset The offset of the first element in \a buffer. Has to be a positive value.
        //! \param[in] stride The stride of each element in \a buffer. Has to be a positive value.
        virtual void bind_vertex_buffer(int32 index, buffer_ptr buffer, int64 offset, int64 stride) = 0;

        //! \brief Binds a \a buffer to the \a vertex_array used as an index buffer (element array buffer).
        //! \param[in] buffer A shared_ptr to to the index buffer which should be bound.
        virtual void bind_index_buffer(buffer_ptr buffer) = 0;

        //! \brief Sets one vertex attribute in a vertex buffer.
        //! \details Enables the vertex attribute and sets all necessary format specific data.
        //! \param[in] index The index of the vertex attribute. Has to be a positive value.
        //! \param[in] buffer_index The index of the vertex buffer the vertex attribute should be enabled in. Has to be a positive value.
        //! \param[in] attribute_format The \a format of the vertex attribute. Has to be \a r8, \a r16, \a r16f, \a r32f, \a r8i, \a r16i, \a r32i, \a r8ui, \a r16ui, \a r32ui, \a rg8, \a rg16, \a
        //! rg16f, \a rg32f, \a rg8i, \a rg16i, \a rg32i, \a rg8ui, \a rg16ui, \a rg32ui, \a rgb32f, \a rgb32i, \a rgb32ui, \a rgba8, \a rgba16, \a rgba16f, \a rgba32f, \a rgba8i, \a rgba16i, \a
        //! rgba32i, \a rgba8ui, \a rgba16ui or \a rgba32ui.
        //! \param[in] relative_offset The offset relative to the whole vertex (if existent). Has to be a positive value.
        virtual void set_vertex_attribute(int32 index, int32 buffer_index, format attribute_format, int64 relative_offset) = 0;

      protected:
        vertex_array()  = default;
        ~vertex_array() = default;
    };
} // namespace mango

#endif // MANGO_VERTEX_ARRAY_HPP
