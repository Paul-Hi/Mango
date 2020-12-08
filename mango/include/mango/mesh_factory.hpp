//! \file      mesh_factory.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_MESH_FACTORY_HPP
#define MANGO_MESH_FACTORY_HPP

#include <mango/types.hpp>

namespace mango
{
    class vertex_array;
    struct mesh_primitive_component;

    class plane_factory;
    class box_factory;
    class sphere_factory;
    class mesh_factory
    {
      public:
        static shared_ptr<plane_factory> mesh_factory::get_plane_factory()
        {
            return std::make_shared<plane_factory>();
        }
        static shared_ptr<box_factory> mesh_factory::get_box_factory()
        {
            return std::make_shared<box_factory>();
        }
        static shared_ptr<sphere_factory> mesh_factory::get_sphere_factory()
        {
            return std::make_shared<sphere_factory>();
        }

        shared_ptr<vertex_array> create_vertex_array(int32& out_count);
        virtual void create_mesh_primitive_component(mesh_primitive_component* component)                              = 0;
        virtual void append(std::vector<float>& vertex_data, std::vector<uint32>& index_data, bool restart, bool seal) = 0;

        virtual mesh_factory& set_texture_coordinates(bool enabled) = 0;

        virtual mesh_factory& set_normals(bool enabled) = 0;

        // virtual void set_tangents(bool enabled) = 0;
      protected:
        mesh_factory()                            = default;
        virtual bool create_texture_coordinates() = 0;
        virtual bool create_normals()             = 0;
    };

    class plane_factory : public mesh_factory
    {
      public:
        plane_factory() = default;
        void create_mesh_primitive_component(mesh_primitive_component* component) override;
        void append(std::vector<float>& vertex_data, std::vector<uint32>& index_data, bool restart, bool seal) override;

        plane_factory& set_texture_coordinates(bool enabled) override
        {
            m_generate_texcoords = enabled;
            return *this;
        }

        plane_factory& set_normals(bool enabled) override
        {
            m_generate_normals = enabled;
            return *this;
        }

        plane_factory& set_face_normal(const glm::vec3& face_normal)
        {
            m_face_normal = face_normal;
            return *this;
        }

        plane_factory& set_offset_along_face_normal(float offset)
        {
            m_offset = offset;
            return *this;
        }

        plane_factory& set_segments(const glm::ivec2& segments)
        {
            m_segments = segments;
            return *this;
        }

        plane_factory& set_uv_tiling(const glm::vec2& tiling)
        {
            m_uv_tiling = tiling;
            return *this;
        }

      private:
        bool m_generate_texcoords = true;
        bool m_generate_normals   = false;
        glm::vec3 m_face_normal   = GLOBAL_UP;
        float m_offset            = 0.0f;
        glm::ivec2 m_segments     = glm::ivec2(1);
        glm::vec2 m_uv_tiling     = glm::vec2(1);

        bool create_texture_coordinates() override
        {
            return m_generate_texcoords;
        };
        bool create_normals() override
        {
            return m_generate_normals;
        };
    };

    class box_factory : public mesh_factory
    {
      public:
        box_factory() = default;
        void create_mesh_primitive_component(mesh_primitive_component* component) override;
        void append(std::vector<float>& vertex_data, std::vector<uint32>& index_data, bool restart, bool seal) override;

        box_factory& set_texture_coordinates(bool enabled) override
        {
            m_generate_texcoords = enabled;
            return *this;
        }

        box_factory& set_normals(bool enabled) override
        {
            m_generate_normals = enabled;
            return *this;
        }

        box_factory& set_segments_vertical(const glm::ivec2& segments)
        {
            m_segments.r = segments.r;
            m_segments.g = segments.g;
            return *this;
        }

        box_factory& set_segments_horizontal(const glm::ivec2& segments)
        {
            m_segments.b = segments.r;
            m_segments.a = segments.g;
            return *this;
        }

        box_factory& set_uv_tiling_vertical(const glm::vec2& tiling)
        {
            m_uv_tiling.r = tiling.r;
            m_uv_tiling.g = tiling.g;
            return *this;
        }

        box_factory& set_uv_tiling_horizontal(const glm::vec2& tiling)
        {
            m_uv_tiling.b = tiling.r;
            m_uv_tiling.a = tiling.g;
            return *this;
        }

      private:
        bool m_generate_texcoords = true;
        bool m_generate_normals   = false;
        glm::ivec4 m_segments     = glm::ivec4(1);
        glm::vec4 m_uv_tiling     = glm::vec4(1);

        bool create_texture_coordinates() override
        {
            return m_generate_texcoords;
        };
        bool create_normals() override
        {
            return m_generate_normals;
        };
    };

    class sphere_factory : public mesh_factory
    {
      public:
        sphere_factory() = default;
        void create_mesh_primitive_component(mesh_primitive_component* component) override;
        void append(std::vector<float>& vertex_data, std::vector<uint32>& index_data, bool restart, bool seal) override;

        sphere_factory& set_texture_coordinates(bool enabled) override
        {
            m_generate_texcoords = enabled;
            return *this;
        }

        sphere_factory& set_normals(bool enabled) override
        {
            m_generate_normals = enabled;
            return *this;
        }

        sphere_factory& set_segments(float segments)
        {
            m_segments.x = segments;
            return *this;
        }

        sphere_factory& set_rings(float rings)
        {
            m_segments.y = rings;
            return *this;
        }

        sphere_factory& set_uv_tiling(const glm::vec2& tiling)
        {
            m_uv_tiling = tiling;
            return *this;
        }

      private:
        bool m_generate_texcoords = true;
        bool m_generate_normals   = false;
        glm::ivec2 m_segments     = glm::ivec2(32, 16);
        glm::vec2 m_uv_tiling     = glm::vec2(1);

        bool create_texture_coordinates() override
        {
            return m_generate_texcoords;
        };
        bool create_normals() override
        {
            return m_generate_normals;
        };
    };

} // namespace mango

#endif // MANGO_MESH_FACTORY_HPP
