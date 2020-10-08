//! \file      uniform_buffer.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <mango/profile.hpp>
#include <graphics/uniform_buffer.hpp>
#include <util/helpers.hpp>

using namespace mango;

uniform_buffer::uniform_buffer() {}

bool uniform_buffer::init(int64 frame_size, buffer_technique technique)
{
    PROFILE_ZONE;
    m_frame_size = frame_size;
    m_technique  = technique;
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &m_uniform_buffer_alignment);
    int32 off = m_frame_size % m_uniform_buffer_alignment;
    m_frame_size += off;
    MANGO_LOG_DEBUG("Frame Size: {0} Byte!", m_frame_size);

    m_uniform_buffer_size  = m_frame_size + static_cast<int8>(m_technique) * m_frame_size;
    m_global_offset        = 0;
    m_local_offset         = 0;
    m_last_offset          = 0;
    m_current_buffer_start = 0;
    m_current_buffer_part  = 0;

    buffer_configuration uniform_buffer_config(m_uniform_buffer_size, buffer_target::UNIFORM_BUFFER, buffer_access::MAPPED_ACCESS_WRITE);
    m_uniform_buffer = buffer::create(uniform_buffer_config);

    if (!check_creation(m_uniform_buffer.get(), "Uniform buffer", "Render System"))
        return false;

    m_mapping = m_uniform_buffer->map(0, m_uniform_buffer->byte_length(), buffer_access::MAPPED_ACCESS_WRITE);

    if (!check_mapping(m_uniform_buffer.get(), "Uniform Mapping", "Render System"))
        return false;

    return true;
}

uniform_buffer::~uniform_buffer() {}

void uniform_buffer::begin_frame(command_buffer_ptr& command_buffer)
{
    PROFILE_ZONE;
    command_buffer->client_wait_sync(m_buffer_sync_objects[m_current_buffer_part]);
}

void uniform_buffer::end_frame(command_buffer_ptr& command_buffer)
{
    PROFILE_ZONE;
    command_buffer->fence_sync(m_buffer_sync_objects[m_current_buffer_part]);
    m_current_buffer_part++;
    m_current_buffer_part %= static_cast<int8>(m_technique);
    m_current_buffer_start = m_current_buffer_part * m_frame_size;
    m_global_offset        = m_current_buffer_start;
    m_last_offset          = m_local_offset;
    m_local_offset         = 0;
}

bind_uniform_buffer_cmd uniform_buffer::bind_uniform_buffer(int32 slot, int64 size, void* data)
{
    PROFILE_ZONE;
    int64 to_add = m_uniform_buffer_alignment;
    while (to_add < static_cast<int64>(size))
        to_add += m_uniform_buffer_alignment;

    MANGO_ASSERT(m_local_offset < m_frame_size - to_add, "Frame size is too small.");
    memcpy(static_cast<g_byte*>(m_mapping) + m_global_offset, data, size);

    bind_uniform_buffer_cmd cmd(m_uniform_buffer, m_global_offset, size, slot);

    m_local_offset += to_add;
    m_global_offset += to_add;
    return cmd;
}
