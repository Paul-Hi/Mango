//! \file      primitive_manager.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2022
//! \copyright Apache License 2.0

#include <mango/profile.hpp>
#include <scene/primitive_manager.hpp>

using namespace mango;

primitive_manager::primitive_manager()
    : m_vertices(0)
    , m_indices(0)
{
}

primitive_gpu_data primitive_manager::add_primitive(primitive_builder& builder)
{
    managed_data data;
    builder.build();
    data.position_data                 = builder.get_positions();
    data.normal_data                   = builder.get_normals();
    data.uv_data                       = builder.get_uvs();
    data.tangent_data                  = builder.get_tangents();
    data.index_data                    = builder.get_indices();
    data.draw_call_desc.vertex_count   = data.position_data.size();
    data.draw_call_desc.index_count    = data.index_data.size();
    data.draw_call_desc.instance_count = 1;

    primitive_gpu_data p_data;
    p_data.input_assembly = builder.get_input_assembly();
    p_data.vertex_layout  = builder.get_vertex_layout();
    p_data.manager_id     = m_internal_data.emplace(std::move(data));

    return p_data;
}

void primitive_manager::remove_primitive(uid manager_id)
{
    if (!manager_id.is_valid())
        return;

    if (!m_internal_data.contains(manager_id))
    {
        MANGO_LOG_WARN("Can not remove primitive! Manager Id is not valid!", manager_id.get());
        return;
    }

    m_internal_data.erase(manager_id);
}

void primitive_manager::generate_buffers(const graphics_device_handle& graphics_device)
{
    m_vertices = 0;
    m_indices  = 0;

    for (auto data_id : m_internal_data)
    {
        const managed_data& data = m_internal_data.at(data_id);
        m_vertices += data.position_data.size();
        m_indices += data.index_data.size();
    }

    buffer_create_info buffer_info;
    buffer_info.buffer_target = gfx_buffer_target::buffer_target_vertex;
    buffer_info.buffer_access = gfx_buffer_access::buffer_access_mapped_access_write | gfx_buffer_access::buffer_access_dynamic_storage;
    buffer_info.size          = sizeof(vec3) * m_vertices;

    m_position_buffer = graphics_device->create_buffer(buffer_info);
    if (!check_creation(m_position_buffer.get(), "position buffer"))
        return;
    m_normal_buffer = graphics_device->create_buffer(buffer_info);
    if (!check_creation(m_normal_buffer.get(), "normal buffer"))
        return;

    buffer_info.size = sizeof(vec2) * m_vertices;
    m_uv_buffer      = graphics_device->create_buffer(buffer_info);
    if (!check_creation(m_uv_buffer.get(), "uv buffer"))
        return;

    buffer_info.size = sizeof(vec4) * m_vertices;
    m_tangent_buffer = graphics_device->create_buffer(buffer_info);
    if (!check_creation(m_tangent_buffer.get(), "tangent buffer"))
        return;

    buffer_info.buffer_target = gfx_buffer_target::buffer_target_index;
    buffer_info.size          = sizeof(uint32) * m_indices;
    m_index_buffer            = graphics_device->create_buffer(buffer_info);
    if (!check_creation(m_index_buffer.get(), "index buffer"))
        return;

    graphics_device_context_handle device_context = graphics_device->create_graphics_device_context();
    vec3* m_position_buffer_mapping               = nullptr;
    vec3* m_normal_buffer_mapping                 = nullptr;
    vec2* m_uv_buffer_mapping                     = nullptr;
    vec4* m_tangent_buffer_mapping                = nullptr;
    uint32* m_index_buffer_mapping                = nullptr;
    device_context->begin();
    m_position_buffer_mapping = static_cast<vec3*>(device_context->map_buffer_data(m_position_buffer, 0, sizeof(vec3) * m_vertices));
    m_normal_buffer_mapping   = static_cast<vec3*>(device_context->map_buffer_data(m_normal_buffer, 0, sizeof(vec3) * m_vertices));
    m_uv_buffer_mapping       = static_cast<vec2*>(device_context->map_buffer_data(m_uv_buffer, 0, sizeof(vec2) * m_vertices));
    m_tangent_buffer_mapping  = static_cast<vec4*>(device_context->map_buffer_data(m_tangent_buffer, 0, sizeof(vec4) * m_vertices));
    m_index_buffer_mapping    = static_cast<uint32*>(device_context->map_buffer_data(m_index_buffer, 0, sizeof(uint32) * m_indices));
    device_context->end();
    device_context->submit();

    {
        int32 global_vertex_count = 0;
        int32 global_index_count  = 0;
        NAMED_PROFILE_ZONE("Megabuffer Creation");
        for (auto data_id : m_internal_data)
        {
            managed_data& data = m_internal_data.at(data_id);
            data.draw_call_desc.index_count  = data.index_data.size();
            data.draw_call_desc.index_offset = global_index_count;
            data.draw_call_desc.base_vertex  = global_vertex_count;

            std::copy(data.index_data.begin(), data.index_data.end(), m_index_buffer_mapping + global_index_count);

            std::copy(data.position_data.begin(), data.position_data.end(), m_position_buffer_mapping + global_vertex_count);
            std::copy(data.normal_data.begin(), data.normal_data.end(), m_normal_buffer_mapping + global_vertex_count);
            std::copy(data.uv_data.begin(), data.uv_data.end(), m_uv_buffer_mapping + global_vertex_count);
            std::copy(data.tangent_data.begin(), data.tangent_data.end(), m_tangent_buffer_mapping + global_vertex_count);

            global_index_count += data.index_data.size();
            global_vertex_count += data.position_data.size();
        }
    }

    device_context->begin();
    bool sucess = device_context->unmap_buffer_data(m_position_buffer);
    sucess &= device_context->unmap_buffer_data(m_normal_buffer);
    sucess &= device_context->unmap_buffer_data(m_uv_buffer);
    sucess &= device_context->unmap_buffer_data(m_tangent_buffer);
    sucess &= device_context->unmap_buffer_data(m_index_buffer);
    device_context->end();
    device_context->submit();

    if(!sucess)
    {
        MANGO_LOG_ERROR("Unmapping failed. Rendering might not work!");
    }
}

void primitive_manager::bind_buffers(const graphics_device_context_handle& frame_context, const gfx_handle<const gfx_buffer> id_buffer, int32 id_offset) const
{
    frame_context->set_index_buffer(m_index_buffer, gfx_format::t_unsigned_int); // unsigned int is unified

    // vertex buffers and layout are unified
    gfx_handle<const gfx_buffer> vbs[5] = { m_position_buffer, m_normal_buffer, m_uv_buffer, m_tangent_buffer, id_buffer };
    int32 bindings[5]                   = { 0, 1, 2, 3, 4 };
    int32 offsets[5]                    = { 0, 0, 0, 0, id_offset * sizeof(ivec2) };

    frame_context->set_vertex_buffers(5, vbs, bindings, offsets);
}