//! \file      vertex_array_impl.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <glad/glad.h>
#include <graphics/buffer.hpp>
#include <graphics/impl/vertex_array_impl.hpp>

using namespace mango;

static const vertex_buffer_cache uninstantiated_vb_cache = { 0, {} };

vertex_array_impl::vertex_array_impl()
    : m_index_buffer(0)
{
    m_vertex_buffers.fill(uninstantiated_vb_cache);
    glCreateVertexArrays(1, &m_name);
}

vertex_array_impl::~vertex_array_impl()
{
    glDeleteVertexArrays(1, &m_name);
}

void vertex_array_impl::bind_vertex_buffer(g_uint index, buffer_ptr buffer, g_intptr offset, g_sizei stride)
{
    MANGO_ASSERT(is_created(), "Vertex array not created!");
    MANGO_ASSERT(buffer->is_created(), "Buffer not created!");
    auto& potential_buffer = m_vertex_buffers.at(index);
    g_uint name            = buffer->get_name();
    if (!potential_buffer.buf || potential_buffer.buf->get_name() != name)
    {
        potential_buffer.buf = buffer;
    }
    glVertexArrayVertexBuffer(m_name, index, name, offset, stride);
}

void vertex_array_impl::bind_index_buffer(buffer_ptr buffer)
{
    MANGO_ASSERT(is_created(), "Vertex array not created!");
    MANGO_ASSERT(buffer->is_created(), "Buffer not created!");
    g_uint name = buffer->get_name();
    if (!m_index_buffer || m_index_buffer->get_name() != name)
    {
        m_index_buffer = buffer;
        glVertexArrayElementBuffer(m_name, name);
    }
}

void vertex_array_impl::set_vertex_attribute(g_uint index, g_uint buffer_index, format attribute_format, g_uint relative_offset)
{
    MANGO_ASSERT(is_created(), "Vertex array not created!");
    auto& hopefully_bound_buffer = m_vertex_buffers.at(buffer_index);
    if (!hopefully_bound_buffer.buf || !hopefully_bound_buffer.buf->is_created())
    {
        MANGO_LOG_ERROR("Vertex buffer on index {0} is not bound, but trying to set vertex attribute!", buffer_index);
        return;
    }

    attr a;
    a.attribute_format = attribute_format;
    a.relative_offset  = relative_offset;

    auto it = hopefully_bound_buffer.enabled_attributes.find(index);
    if (it == hopefully_bound_buffer.enabled_attributes.end())
    {
        g_enum type;
        g_int size;
        g_bool normalized;
        type = get_gl_vertex_attribute_data(attribute_format, size, normalized);

        hopefully_bound_buffer.enabled_attributes.insert({ index, a });
        glEnableVertexArrayAttrib(m_name, index);
        glVertexArrayAttribFormat(m_name, index, size, type, normalized, relative_offset);
        glVertexArrayAttribBinding(m_name, index, buffer_index);
    }
    else if (a.attribute_format != it->second.attribute_format || a.relative_offset != it->second.relative_offset)
    {
        g_enum type;
        g_int size;
        g_bool normalized;
        type = get_gl_vertex_attribute_data(attribute_format, size, normalized);

        hopefully_bound_buffer.enabled_attributes.at(index) = a;
        glEnableVertexArrayAttrib(m_name, index);
        glVertexArrayAttribFormat(m_name, index, size, type, normalized, relative_offset);
        glVertexArrayAttribBinding(m_name, index, buffer_index);
    }
}
