//! \file      buffer_impl.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <glad/glad.h>
#include <graphics/impl/buffer_impl.hpp>

using namespace mango;

buffer_impl::buffer_impl(const buffer_configuration& configuration)
    : m_persistent_data(nullptr)
    , m_size(configuration.size)
    , m_target(GL_NONE)
    , m_access_flags(GL_NONE)
{
    m_target = GL_ARRAY_BUFFER; // Use this as default.

    if (configuration.target == buffer_target::index_buffer)
    {
        m_target = GL_ELEMENT_ARRAY_BUFFER;
    }
    else if (configuration.target == buffer_target::uniform_buffer)
    {
        m_target = GL_UNIFORM_BUFFER;
    }
    else if (configuration.target == buffer_target::shader_storage_buffer)
    {
        m_target = GL_SHADER_STORAGE_BUFFER;
    }
    else if (configuration.target == buffer_target::texture_buffer)
    {
        m_target = GL_TEXTURE_BUFFER;
    }

    bool persistent = false;

    if ((configuration.access & buffer_access::dynamic_storage) != buffer_access::none) // Used if glBufferSubData() etc. should be possible.
    {
        m_access_flags |= GL_DYNAMIC_STORAGE_BIT;
    }
    if ((configuration.access & buffer_access::mapped_access_read) != buffer_access::none) // Used for glMapNamedBufferRange().
    {
        m_access_flags |= GL_MAP_READ_BIT;
        m_access_flags |= GL_MAP_PERSISTENT_BIT;
        m_access_flags |= GL_MAP_COHERENT_BIT;
        persistent = true;
    }
    if ((configuration.access & buffer_access::mapped_access_write) != buffer_access::none) // Used for glMapNamedBufferRange().
    {
        m_access_flags |= GL_MAP_WRITE_BIT;
        m_access_flags |= GL_MAP_PERSISTENT_BIT;
        m_access_flags |= GL_MAP_COHERENT_BIT;
        persistent = true;
    }

    glCreateBuffers(1, &m_name);
    glNamedBufferStorage(m_name, static_cast<g_sizeiptr>(m_size), configuration.data, m_access_flags);

    if (persistent)
    {
        g_bitfield all_map = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
        m_persistent_data  = static_cast<void*>(glMapNamedBufferRange(m_name, 0, static_cast<g_sizeiptr>(m_size), all_map & m_access_flags));
        MANGO_ASSERT(nullptr != m_persistent_data, "Failed mapping buffer {0}!", m_name);
    }
}

buffer_impl::~buffer_impl()
{
    MANGO_ASSERT(is_created(), "Buffer not created!");
    if (m_persistent_data)
    {
        MANGO_ASSERT(glUnmapNamedBuffer(m_name), "Unmapping of persistent mapped buffer failed!");
    }
    glDeleteBuffers(1, &m_name);
}

void buffer_impl::set_data(format internal_format, int64 offset, int64 size, format pixel_format, format type, const void* data)
{
    size = (size == MAX_INT64) ? m_size - offset : size;
    MANGO_ASSERT(size > 0, "Negative size is not possible!");
    MANGO_ASSERT(is_created(), "Buffer not created!");
    MANGO_ASSERT((m_access_flags & GL_DYNAMIC_STORAGE_BIT), "Can not set the data! Buffer is not created with dynamic storage!");
    MANGO_ASSERT(offset < m_size, "Can not set data outside the buffer!");
    MANGO_ASSERT(offset >= 0, "Can not set data outside the buffer! Negative offset!");
    MANGO_ASSERT(offset + size <= m_size, "Can not set data outside the buffer!");
    int64 multiple = number_of_basic_machine_units(internal_format);
    MANGO_ASSERT(offset % multiple == 0 && size % multiple == 0, "Alignment is not valid!");
    MANGO_ASSERT(nullptr != data, "Data is null!");

    g_enum gl_internal_f = static_cast<g_enum>(internal_format);
    g_enum gl_pixel_f    = static_cast<g_enum>(pixel_format);
    g_enum gl_type       = static_cast<g_enum>(type);

    glClearNamedBufferSubData(m_name, gl_internal_f, static_cast<g_intptr>(offset), static_cast<g_sizeiptr>(size), gl_pixel_f, gl_type, data);
}

void* buffer_impl::map(int64 offset, int64 length, buffer_access)
{
    MANGO_ASSERT(is_created(), "Buffer not created!");
    MANGO_ASSERT(offset < m_size, "Can not map data outside the buffer!");
    MANGO_ASSERT(offset >= 0, "Can not map data outside the buffer! Negative offset!");
    MANGO_ASSERT(offset + length <= m_size, "Can not map data outside the buffer!");
    MANGO_ASSERT(m_persistent_data, "Can not map the buffer, maybe the wrong access flags where set!");

    int8* mapped_data = static_cast<int8*>(m_persistent_data);
    mapped_data += offset;

    return static_cast<void*>(mapped_data);
}

void buffer_impl::unmap() {}
