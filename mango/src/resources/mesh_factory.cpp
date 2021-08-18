//! \file      mesh_factory.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#include <graphics/buffer.hpp>
#include <graphics/vertex_array.hpp>
#include <mango/scene_ecs.hpp>
#include <mango/mesh_factory.hpp>
#include <util/helpers.hpp>

using namespace mango;

shared_ptr<vertex_array> mesh_factory::create_vertex_array(int32& out_count)
{
    vertex_array_ptr geometry = vertex_array::create();

    std::vector<float> box_vertex_data;
    std::vector<uint32> box_index_data;

    append(box_vertex_data, box_index_data, false, false);

    // create vbo
    buffer_configuration b_config;
    b_config.access     = buffer_access::none;
    b_config.size       = sizeof(float) * box_vertex_data.size();
    b_config.target     = buffer_target::vertex_buffer;
    const void* vb_data = static_cast<const void*>(box_vertex_data.data());
    b_config.data       = vb_data;
    buffer_ptr vb       = buffer::create(b_config);
    int32 stride        = sizeof(float) * 3;
    if (create_texture_coordinates())
        stride += sizeof(float) * 2;
    if (create_normals())
        stride += sizeof(float) * 3;

    geometry->bind_vertex_buffer(0, vb, 0, stride);

    // enable vertex attributes
    geometry->set_vertex_attribute(0, 0, format::rgb32f, 0);
    if (create_normals())
        geometry->set_vertex_attribute(1, 0, format::rgb32f, sizeof(float) * 3);
    if (create_texture_coordinates())
        geometry->set_vertex_attribute(2, 0, format::rg32f, sizeof(float) * 6);

    // create ibo

    b_config.size       = sizeof(float) * box_index_data.size();
    b_config.target     = buffer_target::index_buffer;
    const void* ib_data = static_cast<const void*>(box_index_data.data());
    b_config.data       = ib_data;
    buffer_ptr ib       = buffer::create(b_config);

    geometry->bind_index_buffer(ib);

    out_count = static_cast<int32>(box_index_data.size()); // TODO Paul: Cast ...

    return geometry;
}

// plane
void plane_factory::create_mesh_primitive_component(mesh_primitive_component* component)
{
    component->vertex_array_object = create_vertex_array(component->count);
    component->first               = 0;
    component->instance_count      = 1;
    component->type_index          = index_type::uint;
    component->topology            = primitive_topology::triangle_strip;
    component->has_normals         = m_generate_normals;
    component->has_tangents        = false;
    component->tp                  = mesh_primitive_type::plane;
}

void plane_factory::append(std::vector<float>& vertex_data, std::vector<uint32>& index_data, bool restart, bool seal)
{
    vec3 diff_x = glm::cross(m_face_normal, GLOBAL_UP);
    if (glm::all(glm::equal(GLOBAL_UP, glm::abs(m_face_normal))))
        diff_x = glm::cross(m_face_normal, GLOBAL_FORWARD);
    vec3 diff_y = glm::cross(m_face_normal, diff_x);

    int32 x_count     = m_segments.x + 1;
    int32 y_count     = m_segments.y + 1;
    vec3 delta_x = 1.0f / m_segments.x * diff_x;
    vec3 delta_y = 1.0f / m_segments.y * diff_y;

    vec3 origin = -0.5f * diff_x - 0.5f * diff_y + m_offset * m_face_normal;

    bool reverse = false;
    if (glm::dot(glm::cross(diff_x, diff_y), m_face_normal) > 0)
        reverse = true;

    int32 stride = 3;
    if (m_generate_texcoords)
        stride += 2;
    if (m_generate_normals)
        stride += 3;

    int32 start_idx = static_cast<int32>(vertex_data.size()) / stride; // TODO Paul: Cast...

    for (int32 y = 0; y < y_count; ++y)
    {
        for (int32 x = 0; x < x_count; ++x)
        {
            // vertex data
            vec3 point = origin + (float)x * delta_x + (float)y * delta_y;
            vertex_data.push_back(point.x);
            vertex_data.push_back(point.y);
            vertex_data.push_back(point.z);
            if (m_generate_normals)
            {
                vertex_data.push_back(m_face_normal.x);
                vertex_data.push_back(m_face_normal.y);
                vertex_data.push_back(m_face_normal.z);
            }
            if (m_generate_texcoords)
            {
                vertex_data.push_back(m_uv_tiling.x * (1.0f - ((float)x / m_segments.x)));
                vertex_data.push_back(m_uv_tiling.y * ((float)y / m_segments.y));
            }

            // index data
            if (y < m_segments.y)
            {
                if (reverse)
                {
                    index_data.push_back(x + (y + 1) * x_count + start_idx);
                    if (restart && x == 0 && y == 0)
                        index_data.push_back(x + (y + 1) * x_count + start_idx);
                    if (y > 0 && x == 0)
                        index_data.push_back(x + (y + 1) * x_count + start_idx);
                    index_data.push_back(x + y * x_count + start_idx);
                    if (y < m_segments.y - 1 && x == m_segments.x)
                        index_data.push_back(x + y * x_count + start_idx);
                }
                else
                {
                    index_data.push_back(x + y * x_count + start_idx);
                    if (restart && x == 0 && y == 0)
                        index_data.push_back(x + y * x_count + start_idx);
                    if (y > 0 && x == 0)
                        index_data.push_back(x + y * x_count + start_idx);
                    index_data.push_back(x + (y + 1) * x_count + start_idx);
                    if (y < m_segments.y - 1 && x == m_segments.x)
                        index_data.push_back(x + (y + 1) * x_count + start_idx);
                }
            }
        }
    }

    if (seal)
        index_data.push_back(index_data.back());
}

// box

void box_factory::create_mesh_primitive_component(mesh_primitive_component* component)
{
    component->vertex_array_object = create_vertex_array(component->count);
    component->first               = 0;
    component->instance_count      = 1;
    component->type_index          = index_type::uint;
    component->topology            = primitive_topology::triangle_strip;
    component->has_normals         = m_generate_normals;
    component->has_tangents        = false;
    component->tp                  = mesh_primitive_type::box;
}

void box_factory::append(std::vector<float>& vertex_data, std::vector<uint32>& index_data, bool restart, bool seal)
{
    auto pf = mesh_factory::get_plane_factory();
    pf->set_texture_coordinates(m_generate_texcoords).set_normals(m_generate_normals).set_offset_along_face_normal(0.5f);

    vec3 face_normal = GLOBAL_UP;
    pf->set_face_normal(face_normal);
    pf->set_segments({ m_segments.b, m_segments.a }).set_uv_tiling({ m_uv_tiling.b, m_uv_tiling.a });
    pf->append(vertex_data, index_data, restart, true);

    face_normal = -GLOBAL_UP;
    pf->set_face_normal(face_normal);
    pf->set_segments({ m_segments.b, m_segments.a }).set_uv_tiling({ m_uv_tiling.b, m_uv_tiling.a });
    pf->append(vertex_data, index_data, true, true);

    face_normal = GLOBAL_FORWARD;
    pf->set_face_normal(face_normal);
    pf->set_segments({ m_segments.r, m_segments.g }).set_uv_tiling({ m_uv_tiling.r, m_uv_tiling.g });
    pf->append(vertex_data, index_data, true, true);

    face_normal = -GLOBAL_FORWARD;
    pf->set_face_normal(face_normal);
    pf->set_segments({ m_segments.r, m_segments.g }).set_uv_tiling({ m_uv_tiling.r, m_uv_tiling.g });
    pf->append(vertex_data, index_data, true, true);

    face_normal = GLOBAL_RIGHT;
    pf->set_face_normal(face_normal);
    pf->set_segments({ m_segments.r, m_segments.g }).set_uv_tiling({ m_uv_tiling.r, m_uv_tiling.g });
    pf->append(vertex_data, index_data, true, true);

    face_normal = -GLOBAL_RIGHT;
    pf->set_face_normal(face_normal);
    pf->set_segments({ m_segments.r, m_segments.g }).set_uv_tiling({ m_uv_tiling.r, m_uv_tiling.g });
    pf->append(vertex_data, index_data, true, seal);
}

// sphere

void sphere_factory::create_mesh_primitive_component(mesh_primitive_component* component)
{
    component->vertex_array_object = create_vertex_array(component->count);
    component->first               = 0;
    component->instance_count      = 1;
    component->type_index          = index_type::uint;
    component->topology            = primitive_topology::triangle_strip;
    component->has_normals         = m_generate_normals;
    component->has_tangents        = false;
    component->tp                  = mesh_primitive_type::sphere;
}

void sphere_factory::append(std::vector<float>& vertex_data, std::vector<uint32>& index_data, bool restart, bool seal)
{
    int32 seg_count  = m_segments.x + 1;
    int32 ring_count = m_segments.y + 1;

    float ring_delta = PI / m_segments.y;
    float seg_delta  = TWO_PI / m_segments.x;

    int32 stride = 3;
    if (m_generate_texcoords)
        stride += 2;
    if (m_generate_normals)
        stride += 3;

    int32 start_idx = static_cast<int32>(vertex_data.size()) / stride; // TODO Paul: Cast...

    for (int32 ring = 0; ring < ring_count; ++ring)
    {
        float r0 = glm::sin(ring * ring_delta);
        float y0 = glm::cos(ring * ring_delta);
        for (int32 seg = 0; seg < seg_count; ++seg)
        {
            float x0 = r0 * glm::sin(seg * seg_delta);
            float z0 = r0 * glm::cos(seg * seg_delta);

            vec3 point = vec3(x0, y0, z0);
            vertex_data.push_back(point.x);
            vertex_data.push_back(point.y);
            vertex_data.push_back(point.z);
            vec3 normal = glm::normalize(point);
            if (m_generate_normals)
            {
                vertex_data.push_back(normal.x);
                vertex_data.push_back(normal.y);
                vertex_data.push_back(normal.z);
            }
            if (m_generate_texcoords)
            {
                vertex_data.push_back(m_uv_tiling.x * ((float)seg / m_segments.x));
                vertex_data.push_back(m_uv_tiling.y * ((float)ring / m_segments.y));
            }

            // index data
            if (ring < m_segments.y)
            {
                index_data.push_back(seg + ring * seg_count + start_idx);
                if (restart && seg == 0 && ring == 0)
                    index_data.push_back(seg + ring * seg_count + start_idx);
                if (ring > 0 && seg == 0)
                    index_data.push_back(seg + ring * seg_count + start_idx);
                index_data.push_back(seg + (ring + 1) * seg_count + start_idx);
                if (ring < m_segments.y - 1 && seg == m_segments.x)
                    index_data.push_back(seg + (ring + 1) * seg_count + start_idx);
            }
        }
    }

    if (seal)
        index_data.push_back(index_data.back());
}
