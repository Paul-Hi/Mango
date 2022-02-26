//! \file      primitive_builder.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2022
//! \copyright Apache License 2.0

#ifndef MANGO_PRIMITIVE_BUILDER_HPP
#define MANGO_PRIMITIVE_BUILDER_HPP

#include <mango/types.hpp>
#include <scene/scene_structures_internal.hpp>

namespace mango
{
    class primitive_builder
    {
      public:
        primitive_builder()
        {
            m_unified              = false;
            m_vertex_layout_added  = false;
            m_input_assembly_added = false;
            m_has_uvs              = false;
            m_has_normals          = false;
            m_has_tangents         = false;
        }

        inline primitive_builder& with_positions(std::vector<vec3>&& positions)
        {
            if (m_unified)
            {
                MANGO_LOG_WARN("Can not add more data, primitive is already unified!");
                return *this;
            }
            m_positions = std::move(positions);
            return *this;
        }
        inline primitive_builder& with_normals(std::vector<vec3>&& normals)
        {
            if (m_unified)
            {
                MANGO_LOG_WARN("Can not add more data, primitive is already unified!");
                return *this;
            }
            m_normals     = std::move(normals);
            m_has_normals = true;
            return *this;
        }
        inline primitive_builder& with_uvs(std::vector<vec2>&& uvs)
        {
            if (m_unified)
            {
                MANGO_LOG_WARN("Can not add more data, primitive is already unified!");
                return *this;
            }
            m_uvs     = std::move(uvs);
            m_has_uvs = true;
            return *this;
        }
        inline primitive_builder& with_tangents(std::vector<vec4>&& tangents)
        {
            if (m_unified)
            {
                MANGO_LOG_WARN("Can not add more data, primitive is already unified!");
                return *this;
            }
            m_tangents     = std::move(tangents);
            m_has_tangents = true;
            return *this;
        }
        inline primitive_builder& with_indices(std::vector<uint32>&& indices)
        {
            if (m_unified)
            {
                MANGO_LOG_WARN("Can not add more data, primitive is already unified!");
                return *this;
            }
            m_indices = std::move(indices);
            return *this;
        }

        inline primitive_builder& with_vertex_layout(vertex_input_descriptor&& vertex_layout)
        {
            if (m_unified)
            {
                MANGO_LOG_WARN("Can not add more data, primitive is already unified!");
                return *this;
            }
            m_vertex_layout_added = true;
            m_vertex_layout       = std::move(vertex_layout);
            return *this;
        }
        inline primitive_builder& with_input_assembly(input_assembly_descriptor&& input_assembly)
        {
            if (m_unified)
            {
                MANGO_LOG_WARN("Can not add more data, primitive is already unified!");
                return *this;
            }
            m_input_assembly_added = true;
            m_input_assembly       = std::move(input_assembly);
            return *this;
        }

        primitive_builder& unify();

        primitive_builder& remove_doubles();
        primitive_builder& calculate_face_normals();
        primitive_builder& calculate_vertex_normals();
        primitive_builder& calculate_tangents();
        primitive_builder& double_side(); // can be removed with remove doubles?

        void build();

        inline bool has_position_data()
        {
            return !m_positions.empty();
        }

        inline bool has_uv_data()
        {
            return m_has_uvs;
        }

        inline bool has_normal_data()
        {
            return m_has_normals;
        }

        inline bool has_tangent_data()
        {
            return m_has_tangents;
        }

        inline std::vector<vec3>& get_positions()
        {
            if (!m_unified)
            {
                MANGO_LOG_WARN("Data is not unified! Primitive might be broken!");
            }
            return m_positions;
        }

        inline std::vector<vec3>& get_normals()
        {
            if (!m_unified)
            {
                MANGO_LOG_WARN("Data is not unified! Primitive might be broken!");
            }
            return m_normals;
        }

        inline std::vector<vec2>& get_uvs()
        {
            if (!m_unified)
            {
                MANGO_LOG_WARN("Data is not unified! Primitive might be broken!");
            }
            return m_uvs;
        }

        inline std::vector<vec4>& get_tangents()
        {
            if (!m_unified)
            {
                MANGO_LOG_WARN("Data is not unified! Primitive might be broken!");
            }
            return m_tangents;
        }

        inline std::vector<uint32>& get_indices()
        {
            if (!m_unified)
            {
                MANGO_LOG_WARN("Data is not unified! Primitive might be broken!");
            }
            return m_indices;
        }

        inline input_assembly_descriptor& get_input_assembly()
        {
            if (!m_unified)
            {
                MANGO_LOG_WARN("Data is not unified! Primitive might be broken!");
            }
            return m_input_assembly;
        }

        inline vertex_input_descriptor& get_vertex_layout()
        {
            if (!m_unified)
            {
                MANGO_LOG_WARN("Data is not unified! Primitive might be broken!");
            }
            return m_vertex_layout;
        }

      private:
        std::vector<vec3> m_positions;
        std::vector<vec3> m_normals;
        std::vector<vec2> m_uvs;
        std::vector<vec4> m_tangents;
        std::vector<uint32> m_indices;
        vertex_input_descriptor m_vertex_layout;
        input_assembly_descriptor m_input_assembly;
        draw_call_description m_draw_call_desc;

        bool m_unified;
        bool m_vertex_layout_added;
        bool m_input_assembly_added;
        bool m_has_uvs;
        bool m_has_normals;
        bool m_has_tangents;
    };
} // namespace mango

#endif // MANGO_PRIMITIVE_BUILDER_HPP
