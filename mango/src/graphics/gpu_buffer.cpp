//! \file      gpu_buffer.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <graphics/gpu_buffer.hpp>
#include <mango/profile.hpp>
#include <util/helpers.hpp>

using namespace mango;

gpu_buffer::gpu_buffer() {}

bool gpu_buffer::init(int64 frame_size, buffer_technique technique)
{
    PROFILE_ZONE;
    m_frame_size = frame_size;
    m_technique  = technique;
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &m_uniform_buffer_alignment);
    int32 off = m_frame_size % m_uniform_buffer_alignment;
    m_frame_size += off;
    MANGO_LOG_DEBUG("Frame Size: {0} Byte!", m_frame_size);

    m_gpu_buffer_size      = m_frame_size + (static_cast<int8>(m_technique) + 1) * m_frame_size;
    m_global_offset        = 0;
    m_local_offset         = 0;
    m_last_offset          = 0;
    m_current_buffer_start = 0;
    m_current_buffer_part  = 0;

    // TODO Paul: This could be any buffer not just an uniform one.
    buffer_configuration gpu_buffer_config(m_gpu_buffer_size, buffer_target::uniform_buffer, buffer_access::mapped_access_write);
    m_gpu_buffer = buffer::create(gpu_buffer_config);

    if (!check_creation(m_gpu_buffer.get(), "GPU buffer", "Render System"))
        return false;

    m_mapping = m_gpu_buffer->map(0, m_gpu_buffer->byte_length(), buffer_access::mapped_access_write);

    if (!check_mapping(m_gpu_buffer.get(), "GPU buffer Mapping", "Render System"))
        return false;

    return true;
}

gpu_buffer::~gpu_buffer() {}

g_sync* gpu_buffer::prepare()
{
    PROFILE_ZONE;
    // command_buffer->client_wait_sync(m_buffer_sync_objects[m_current_buffer_part]);
    return &m_buffer_sync_objects[m_current_buffer_part];
}

g_sync* gpu_buffer::end_frame()
{
    PROFILE_ZONE;
    // command_buffer->fence_sync(m_buffer_sync_objects[m_current_buffer_part]);
    g_sync* sync_to_place = &m_buffer_sync_objects[m_current_buffer_part];
    m_current_buffer_part++;
    m_current_buffer_part %= (static_cast<int8>(m_technique) + 1);
    m_current_buffer_start = m_current_buffer_part * m_frame_size;
    m_global_offset        = m_current_buffer_start;
    m_last_offset          = m_local_offset;
    m_local_offset         = 0;
    return sync_to_place;
}

int64 gpu_buffer::write_data(int64 size, void* data)
{
    PROFILE_ZONE;
    int64 to_add = m_uniform_buffer_alignment;
    while (to_add < static_cast<int64>(size))
        to_add += m_uniform_buffer_alignment;

    MANGO_ASSERT(m_local_offset < m_frame_size - to_add, "Frame size is too small.");
    memcpy(static_cast<g_byte*>(m_mapping) + m_global_offset, data, size);

    m_local_offset += to_add;
    m_global_offset += to_add;

    return m_global_offset - to_add;
}
