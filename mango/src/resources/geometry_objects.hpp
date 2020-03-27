//! \file      geometry_objects.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_GEOMETRY_OBJECTS_HPP
#define MANGO_GEOMETRY_OBJECTS_HPP

#include <mango/types.hpp>
#include <vector>

namespace mango
{
    //! \brief The type of any buffer.
    //! \details There are two versions of each buffer. The '_static' and '_dynamic' is used as a hint, wether the buffer will be updated later or not.
    enum buffer_type
    {
        vertex_buffer_static,  //!< A vertex buffer that will not get updated later on.
        vertex_buffer_dynamic, //!< A vertex buffer that could get updated later on.
        index_buffer_static,   //!< An index buffer that will not get updated later on.
        index_buffer_dynamic   //!< An index buffer that could get updated later on.
    };

    //! \brief An attribute in a \a buffer_layout.
    struct buffer_attribute
    {
        const char* name;       //!< Name of the attribute. Unused at the moment.
        gpu_resource_type type; //!< The type of the attribute.
        uint32 component_count; //!< The number of components in that attribute.
        uint32 size_in_bytes;   //!< The size of the attribute in bytes.
        bool normalized;        //!< True, if the attribute is normalized, else false.
        uint32 attrib_divisor;  //!< The attribute divisor used for instanced rendering. This is only > 0, if there is a buffer that is not updated per vertex, but per instance.
        uint32 offset;          //!< The offset of the attribute in the vertex. Calculated by the layout.

        //! \brief Creates a \a buffer_attribute.
        //! \param[in] name The name of the attribute.
        //! \param[in] type The attributes type.
        //! \param[in] component_count The number of components in the attribute.
        //! \param[in] size_in_bytes The size of the attribute in bytes.
        //! \param[in] normalized True, if the attribute is normalized, else false.
        //! \param[in] attrib_divisor he attribute divisor used for instanced rendering.
        //! \return The created \a buffer_attribute.
        static buffer_attribute create(const char* name, gpu_resource_type type, uint32 component_count, uint32 size_in_bytes, bool normalized, uint32 attrib_divisor = 0);

      private:
        buffer_attribute() = default;
    };

    //! \brief The data layout for vertex buffers.
    //! \details Layout is used to tell the gpu where to find certain attributes in the buffer.
    struct buffer_layout
    {
        std::vector<buffer_attribute> attributes; //!< The list of attributes in the buffer and in the layout.
        uint32 stride;                            //!< The stride of one vertex.

        //! \brief Creates a \a buffer_layout.
        //! \details This does also calculate the offset of all attributes as well as the stride of on vertex in the buffer.
        //! \param[in] attributes The initializer list of attributes for the layout.
        //! \return The created \a buffer_layout.
        static buffer_layout create(const std::initializer_list<buffer_attribute>& attributes);

      private:
        buffer_layout() = default;
    };

    //! \brief The configuration for all buffers of a vertex array object.
    //! \details This includes the specification for vertex and index buffers.
    struct buffer_configuration
    {
        buffer_type vertex_buffer_type; //!< The type of the vertex buffer of the vertex array object.
        buffer_type index_buffer_type;  //!< The type of the index buffer of the vertex array object.
        float* vertices;//!< The vertices of the vertex buffer of the vertex array object.
        //! \brief Size of \a vertices.
        uint32 vertex_buffer_size;
        buffer_layout vertex_buffer_layout;//!< The layout of the vertex buffer of the vertex array object.
        uint32* indices;//!< The indices of the vertex buffer of the vertex array object.
        //! \brief Size of \a indices.
        uint32 index_buffer_size;
    };

    //! \brief Creates a vertex array object.
    //! \details This generates and initializes all necessary gpu buffers for rendering with a vertex array object.
    //! \param[in] configuration The \a buffer_configuration for creating the vertex array object.
    //! \return The handle for the created vertex array object.
    uint32 create_vertex_array_object(const buffer_configuration& configuration);
    //! \brief Updates a vertex array object.
    //! \details This updates ALL gpu buffers of the vertex array object. This may kill performance.
    //! \param[in] vertex_array_object The vertex_array_object to update.
    //! \param[in] configuration The \a buffer_configuration for updating the vertex array object.
    void update_vertex_array_object(uint32 vertex_array_object, const buffer_configuration& configuration);
} // namespace mango

#endif // MANGO_GEOMETRY_OBJECTS_HPP