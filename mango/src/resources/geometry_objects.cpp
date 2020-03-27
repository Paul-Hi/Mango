//! \file      geometry_objects.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <glad/glad.h>
#include <mango/log.hpp>
#include <resources/geometry_objects.hpp>

using namespace mango;

buffer_attribute buffer_attribute::create(const char* name, gpu_resource_type type, uint32 component_count, uint32 size_in_bytes, bool normalized, uint32 attrib_divisor)
{
    buffer_attribute attribute;
    attribute.name            = name;
    attribute.type            = type;
    attribute.component_count = component_count;
    attribute.size_in_bytes   = size_in_bytes;
    attribute.normalized      = normalized;
    attribute.attrib_divisor  = attrib_divisor;
    attribute.offset          = 0;

    return attribute;
}

buffer_layout buffer_layout::create(const std::initializer_list<buffer_attribute>& attributes)
{
    buffer_layout layout;
    layout.attributes = attributes;
    uint32 offset     = 0;
    layout.stride     = 0;
    for (uint32 i = 0; i < layout.attributes.size(); ++i)
    {
        layout.attributes[i].offset = offset;
        offset += layout.attributes[i].size_in_bytes;
        layout.stride += layout.attributes[i].size_in_bytes;
    }

    return layout;
}

namespace mango
{
    uint32 create_vertex_array_object(const buffer_configuration& configuration)
    {
        uint32 vao, vbo, ibo;
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ibo);

        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        buffer_type vertex_buffer_type = configuration.vertex_buffer_type;
        float* vertices                = configuration.vertices;
        if (vertex_buffer_type == vertex_buffer_static)
            glBufferData(GL_ARRAY_BUFFER, configuration.vertex_buffer_size, vertices, GL_STATIC_DRAW);
        else if (vertex_buffer_type == vertex_buffer_dynamic)
            glBufferData(GL_ARRAY_BUFFER, configuration.vertex_buffer_size, vertices, GL_DYNAMIC_DRAW);
        else
        {
            MANGO_LOG_ERROR("Invalid vertex buffer type! Vertex Array Object is not valid!");
            return 0;
        }

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        buffer_type index_buffer_type = configuration.index_buffer_type;
        uint32* indices               = configuration.indices;
        if (index_buffer_type == index_buffer_static)
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, configuration.index_buffer_size, indices, GL_STATIC_DRAW);
        else if (index_buffer_type == index_buffer_dynamic)
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, configuration.index_buffer_size, indices, GL_DYNAMIC_DRAW);
        else
        {
            MANGO_LOG_ERROR("Invalid index buffer type! Vertex Array Object is not valid!");
            return 0;
        }

        auto layout     = configuration.vertex_buffer_layout;
        auto attributes = layout.attributes;
        uint32 index    = 0;
        for (uint32 i = 0; i < attributes.size(); ++i, ++index)
        {
            if (attributes.at(i).type == gpu_int || attributes.at(i).type == gpu_ivec2 || attributes.at(i).type == gpu_ivec3 || attributes.at(i).type == gpu_ivec4)
            {
                glVertexAttribIPointer(index, attributes.at(i).component_count, GL_INT, layout.stride, (void*)attributes.at(i).offset);
            }
            else // gpu_float, gpu_vec2, gpu_vec3, gpu_vec4, gpu_mat3, gpu_mat4
            {
                glVertexAttribPointer(index, attributes.at(i).component_count, GL_FLOAT, attributes.at(i).normalized ? GL_TRUE : GL_FALSE, layout.stride, (void*)attributes.at(i).offset);
            }
            glEnableVertexAttribArray(index);
            if (attributes.at(i).attrib_divisor > 0)
                glVertexAttribDivisor(index, attributes.at(i).attrib_divisor);
        }
        return vao;
    }

    void update_vertex_array_object(uint32 vertex_array_object, const buffer_configuration& configuration)
    {
        uint32 vao = vertex_array_object;
        glBindVertexArray(vao);
        int32 vbo, ibo;
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &vbo);
        glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &ibo);

        buffer_type vertex_buffer_type = configuration.vertex_buffer_type;
        float* vertices                = configuration.vertices;
        if (vertex_buffer_type == vertex_buffer_static)
            glBufferData(GL_ARRAY_BUFFER, configuration.vertex_buffer_size, vertices, GL_STATIC_DRAW);
        else if (vertex_buffer_type == vertex_buffer_dynamic)
            glBufferData(GL_ARRAY_BUFFER, configuration.vertex_buffer_size, vertices, GL_DYNAMIC_DRAW);
        else
        {
            MANGO_LOG_ERROR("Invalid vertex buffer type! Vertex Array Object is not valid!");
            return;
        }

        buffer_type index_buffer_type = configuration.index_buffer_type;
        uint32* indices               = configuration.indices;
        if (index_buffer_type == index_buffer_static)
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, configuration.index_buffer_size, indices, GL_STATIC_DRAW);
        else if (index_buffer_type == index_buffer_dynamic)
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, configuration.index_buffer_size, indices, GL_DYNAMIC_DRAW);
        else
        {
            MANGO_LOG_ERROR("Invalid index buffer type! Vertex Array Object is not valid!");
            return;
        }

        auto layout     = configuration.vertex_buffer_layout;
        auto attributes = layout.attributes;
        uint32 index    = 0;
        for (uint32 i = 0; i < attributes.size(); ++i, ++index)
        {
            if (attributes.at(i).type == gpu_int || attributes.at(i).type == gpu_ivec2 || attributes.at(i).type == gpu_ivec3 || attributes.at(i).type == gpu_ivec4)
            {
                glVertexAttribIPointer(index, attributes.at(i).component_count, GL_INT, layout.stride, (void*)attributes.at(i).offset);
            }
            else // gpu_float, gpu_vec2, gpu_vec3, gpu_vec4, gpu_mat3, gpu_mat4
            {
                glVertexAttribPointer(index, attributes.at(i).component_count, GL_FLOAT, attributes.at(i).normalized ? GL_TRUE : GL_FALSE, layout.stride, (void*)attributes.at(i).offset);
            }
            glEnableVertexAttribArray(index);
            if (attributes.at(i).attrib_divisor > 0)
                glVertexAttribDivisor(index, attributes.at(i).attrib_divisor);
        }
    }
} // namespace mango
