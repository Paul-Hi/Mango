//! \file      primitive_builder.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2022
//! \copyright Apache License 2.0

#include <graphics/graphics_resources.hpp>
#include <resources/primitive_builder.hpp>

using namespace mango;

primitive_builder& primitive_builder::unify()
{
    if (m_positions.empty())
    {
        MANGO_LOG_WARN("No positions! Can not do anything!");
        return *this;
    }
    if (!m_input_assembly_added)
    {
        MANGO_LOG_WARN("No input assembly added! Can not do anything!");
        return *this;
    }

    m_unified             = true;
    m_vertex_layout_added = true;
    // convert to triangles
    if (m_input_assembly.topology != gfx_primitive_topology::primitive_topology_triangle_list)
    {
        if (m_input_assembly.topology == gfx_primitive_topology::primitive_topology_triangle_strip)
        {
            MANGO_LOG_INFO("Converting triangle strip to triangle list!");
            MANGO_ASSERT(!m_positions.empty(), "Positions are empty");
            if (m_indices.empty())
            {
                std::vector<vec2> new_uvs;
                std::vector<vec3> new_positions;
                for (int32 i = 0; i < m_positions.size() - 2; i++)
                {
                    if (i % 2)
                    {
                        new_positions.push_back(m_positions[i]);
                        new_positions.push_back(m_positions[i + 1]);
                        new_positions.push_back(m_positions[i + 2]);
                        if (!m_uvs.empty())
                        {
                            new_uvs.push_back(m_uvs[i]);
                            new_uvs.push_back(m_uvs[i + 1]);
                            new_uvs.push_back(m_uvs[i + 2]);
                        }
                    }
                    else
                    {
                        new_positions.push_back(m_positions[i]);
                        new_positions.push_back(m_positions[i + 2]);
                        new_positions.push_back(m_positions[i + 1]);
                        if (!m_uvs.empty())
                        {
                            new_uvs.push_back(m_uvs[i]);
                            new_uvs.push_back(m_uvs[i + 2]);
                            new_uvs.push_back(m_uvs[i + 1]);
                        }
                    }
                }
                m_positions    = new_positions;
                m_uvs          = new_uvs;
                m_has_normals  = false;
                m_has_tangents = false;
            }
            else
            {
                // Does not remove degenerated triangles -.-
                std::vector<uint32> new_indices;
                for (int32 i = 0; i < m_indices.size() - 2; i++)
                {
                    if (i % 2)
                    {
                        new_indices.push_back(m_indices[i]);
                        new_indices.push_back(m_indices[i + 1]);
                        new_indices.push_back(m_indices[i + 2]);
                    }
                    else
                    {
                        new_indices.push_back(m_indices[i]);
                        new_indices.push_back(m_indices[i + 2]);
                        new_indices.push_back(m_indices[i + 1]);
                    }
                }
                m_indices      = new_indices;
                m_has_normals  = false;
                m_has_tangents = false;
            }
        }
        else if (m_input_assembly.topology == gfx_primitive_topology::primitive_topology_triangle_fan)
        {
            MANGO_LOG_INFO("Converting triangle fan to triangle list!");
            MANGO_ASSERT(!m_positions.empty(), "Positions are empty");
            if (m_indices.empty())
            {
                std::vector<vec2> new_uvs;
                std::vector<vec3> new_positions;
                for (int32 i = 0; i < m_positions.size() - 2; i++)
                {
                    new_positions.push_back(m_positions[i]);
                    new_positions.push_back(m_positions[i + 1]);
                    new_positions.push_back(m_positions[i + 2]);
                    if (!m_uvs.empty())
                    {
                        new_uvs.push_back(m_uvs[i]);
                        new_uvs.push_back(m_uvs[i + 1]);
                        new_uvs.push_back(m_uvs[i + 2]);
                    }
                }
                m_positions    = new_positions;
                m_uvs          = new_uvs;
                m_has_normals  = false;
                m_has_tangents = false;
            }
            else
            {
                // Does not remove degenerated triangles -.-
                std::vector<uint32> new_indices;
                for (int32 i = 0; i < m_indices.size() - 2; i++)
                {
                    new_indices.push_back(m_indices[i]);
                    new_indices.push_back(m_indices[i + 1]);
                    new_indices.push_back(m_indices[i + 2]);
                }
                m_indices      = new_indices;
                m_has_normals  = false;
                m_has_tangents = false;
            }
        }
        else
        {
            MANGO_ASSERT(false, "We do not support line or point data yet.");
        }
    }

    if (m_uvs.empty())
        m_uvs.resize(m_positions.size(), vec2(0.0f, 0.0f));
    if (m_normals.empty())
        m_normals.resize(m_positions.size(), make_vec3(0.0f));
    if (m_tangents.empty())
        m_tangents.resize(m_positions.size(), make_vec4(0.0f));

    if (m_indices.empty())
    {
        for (int32 i = 0; i < m_positions.size() - 3; i += 3)
        {
            m_indices.push_back(i);
            m_indices.push_back(i + 1);
            m_indices.push_back(i + 2);
        }
    }

    m_vertex_layout  = graphics::UNIFIED_VERTEX_LAYOUT;
    m_input_assembly = graphics::UNIFIED_INPUT_ASSEMBLY;

    return *this;
}

primitive_builder& primitive_builder::remove_doubles()
{
    if (!m_unified)
    {
        MANGO_LOG_WARN("Can not work on non unified data!");
        return *this;
    }
    // TODO Paul: We need an acceleration structure for this (KDTree).
    return *this;
}

primitive_builder& primitive_builder::calculate_face_normals()
{
    if (!m_unified)
    {
        MANGO_LOG_WARN("Can not work on non unified data!");
        return *this;
    }

    remove_doubles();

    auto flatten = [this](auto old)
    {
        decltype(old) flat_data;
        for (uint32 i = 0; i < m_indices.size() - 2; ++i)
        {
            flat_data.push_back(old[m_indices[i]]);
            flat_data.push_back(old[m_indices[i + 1]]);
            flat_data.push_back(old[m_indices[i + 2]]);
        }
        return flat_data;
    };

    m_positions = flatten(m_positions);
    m_normals   = flatten(m_normals);
    m_uvs       = flatten(m_uvs);
    m_tangents  = flatten(m_tangents);

    std::vector<uint32> new_indices;
    for (uint32 i = 0; i < m_indices.size() - 2; ++i)
    {
        if (i % 2)
        {
            new_indices[i]     = i * 3;
            new_indices[i + 1] = i * 3 + 1;
            new_indices[i + 2] = i * 3 + 2;
        }
        else
        {
            new_indices[i]     = i * 3;
            new_indices[i + 2] = i * 3 + 2;
            new_indices[i + 1] = i * 3 + 1;
        }
    }

    m_indices     = new_indices;
    m_has_normals = true;

    return *this;
}

primitive_builder& primitive_builder::calculate_vertex_normals()
{
    if (!m_unified)
    {
        MANGO_LOG_WARN("Can not work on non unified data!");
        return *this;
    }

    m_normals.resize(m_positions.size());
    std::fill(m_normals.begin(), m_normals.end(), make_vec3(0.0));

    for (int32 index = 0; index < m_indices.size(); index += 3)
    {
        vec3 normal = (m_positions[index + 1] - m_positions[index]).cross(m_positions[index + 2] - m_positions[index]);
        m_normals[index] += normal;
        m_normals[index + 1] += normal;
        m_normals[index + 2] += normal;
    }

    for (vec3& normal : m_normals)
    {
        normal.normalize();
    }
    m_has_normals = true;

    return *this;
}

primitive_builder& primitive_builder::calculate_tangents()
{
    if (!m_unified)
    {
        MANGO_LOG_WARN("Can not work on non unified data!");
        return *this;
    }

    m_tangents.clear();

    for (int32 i = 0; i < m_positions.size() - 3; i += 3)
    {
        const vec3& p0 = m_positions[i];
        const vec3& p1 = m_positions[i + 1];
        const vec3& p2 = m_positions[i + 2];

        // This algorithm does not work, if uvs are zeroed -.-
        const vec2& uv0 = m_uvs[i];
        const vec2& uv1 = m_uvs[i + 1];
        const vec2& uv2 = m_uvs[i + 2];

        const vec3 delta_p0  = p1 - p0;
        const vec3 delta_p1  = p2 - p0;
        const vec2 delta_uv0 = uv1 - uv0;
        const vec2 delta_uv1 = uv2 - uv0;

        float r            = 1.0f / (delta_uv0.x() * delta_uv1.y() - delta_uv0.y() * delta_uv1.x() + 1e-5);
        const vec4 tangent = ((delta_p0 * delta_uv1.y() - delta_p1 * delta_uv0.y()) * r).homogeneous(); // homogeneous works here, since it only appends a 1.0
        m_tangents.push_back(tangent);
        m_tangents.push_back(tangent);
        m_tangents.push_back(tangent);
    }
    m_has_tangents = true;

    return *this;
}

primitive_builder& primitive_builder::double_side()
{
    if (!m_unified)
    {
        MANGO_LOG_WARN("Can not work on non unified data!");
        return *this;
    }
    // TODO Paul: Implement this - Not sure how to do it properly.
    return *this;
}

void primitive_builder::build()
{
    if (!m_unified)
    {
        MANGO_LOG_WARN("Data is not unified! Primitive might be broken!");
    }
    if (!m_vertex_layout_added)
    {
        MANGO_LOG_WARN("No vertex_layout added! Primitive might be broken!");
    }
}
