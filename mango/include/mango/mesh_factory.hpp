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
    // fwd;
    class vertex_array;
    struct mesh_primitive_component;
    class plane_factory;
    class box_factory;
    class sphere_factory;

    //! \brief Base class for factories creating geometry primitive data.
    class mesh_factory
    {
      public:
        //! \brief Get a plane factory.
        //! \return A shared_ptr to a \a plane_factory.
        static shared_ptr<plane_factory> get_plane_factory()
        {
            return std::make_shared<plane_factory>();
        }

        //! \brief Get a box factory.
        //! \return A shared_ptr to a \a box_factory.
        static shared_ptr<box_factory> get_box_factory()
        {
            return std::make_shared<box_factory>();
        }

        //! \brief Get a sphere factory.
        //! \return A shared_ptr to a \a sphere_factory.
        static shared_ptr<sphere_factory> get_sphere_factory()
        {
            return std::make_shared<sphere_factory>();
        }

        //! \brief Creates a vertex array from specific data.
        //! \param[out] out_count The number of indeices in the index buffer.
        //! \return A shared_ptr to the \a vertex_array filled with vertex and index buffers.
        shared_ptr<vertex_array> create_vertex_array(int32& out_count);

        //! \brief Creates a \a mesh_primitive_component.
        //! \param[out] component Pointer to the \a mesh_primitive_component to store the generated data in.
        virtual void create_mesh_primitive_component(mesh_primitive_component* component) = 0;

        //! \brief Appends the specific geometry data to existung vertex and index data.
        //! \param[in,out] vertex_data The vertex data to append to.
        //! \param[in,out] index_data The index data to append to.
        //! \param[in] restart True, if index buffer has to be restarted (degenerate triangles).
        //! \param[in] seal True, if index buffer has to be sealed (degenerate triangles).
        virtual void append(std::vector<float>& vertex_data, std::vector<uint32>& index_data, bool restart, bool seal) = 0;

        //! \brief Sets the generation of texture coordinates.
        //! \param[in] enabled True if texture coordinates should be generated, else false.
        //! \return A reference to the \a mesh_factory.
        virtual mesh_factory& set_texture_coordinates(bool enabled) = 0;

        //! \brief Sets the generation of normals.
        //! \param[in] enabled True if normals should be generated, else false.
        //! \return A reference to the \a mesh_factory.
        virtual mesh_factory& set_normals(bool enabled) = 0;

        // virtual void set_tangents(bool enabled) = 0;
      protected:
        mesh_factory() = default;

        //! \brief Retrieve the base class texture coordinates setting in derived ones.
        //! \return True if texture coordinates should be generated, else false.
        virtual bool create_texture_coordinates() = 0;

        //! \brief Retrieve the base class normal setting in derived ones.
        //! \return True if normals should be generated, else false.
        virtual bool create_normals() = 0;
    };

    //! \brief Factory creating a plane geometry.
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

        //! \brief Sets the face normal.
        //! \param[in] face_normal The face normal to set for generation.
        //! \return A reference to the \a plane_factory.
        plane_factory& set_face_normal(const glm::vec3& face_normal)
        {
            m_face_normal = face_normal;
            return *this;
        }

        //! \brief Sets the offset to move the plane from the origin along the face normal.
        //! \param[in] offset The offset to use for generation.
        //! \return A reference to the \a plane_factory.
        plane_factory& set_offset_along_face_normal(float offset)
        {
            m_offset = offset;
            return *this;
        }

        //! \brief Sets the number of segments in x and y direction to generate.
        //! \param[in] segments The number of segments in x and y direction.
        //! \return A reference to the \a plane_factory.
        plane_factory& set_segments(const glm::ivec2& segments)
        {
            m_segments = segments;
            return *this;
        }

        //! \brief Sets the uv tiling in x and y direction for texture coordinate generation.
        //! \param[in] tiling The uv tiling in x and y direction.
        //! \return A reference to the \a plane_factory.
        plane_factory& set_uv_tiling(const glm::vec2& tiling)
        {
            m_uv_tiling = tiling;
            return *this;
        }

      private:
        //! \brief True if texture coordinates should be generated, else false.
        bool m_generate_texcoords = true;
        //! \brief True if normals should be generated, else false.
        bool m_generate_normals = false;
        //! \brief The face normal to use for generation.
        glm::vec3 m_face_normal = GLOBAL_UP;
        //! \brief The face normal offset to use for generation.
        float m_offset = 0.0f;
        //! \brief The number of segments to generate.
        glm::ivec2 m_segments = glm::ivec2(1);
        //! \brief The uv tiling to use for generation.
        glm::vec2 m_uv_tiling = glm::vec2(1);

        bool create_texture_coordinates() override
        {
            return m_generate_texcoords;
        };
        bool create_normals() override
        {
            return m_generate_normals;
        };
    };

    //! \brief Factory creating a box geometry.
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

        //! \brief Sets the number of segments in x and y direction to generate for vertical faces.
        //! \param[in] segments The number of segments in x and y direction.
        //! \return A reference to the \a plane_factory.
        box_factory& set_segments_vertical(const glm::ivec2& segments)
        {
            m_segments.r = segments.r;
            m_segments.g = segments.g;
            return *this;
        }

        //! \brief Sets the number of segments in x and y direction to generate for horizontal faces.
        //! \param[in] segments The number of segments in x and y direction.
        //! \return A reference to the \a plane_factory.
        box_factory& set_segments_horizontal(const glm::ivec2& segments)
        {
            m_segments.b = segments.r;
            m_segments.a = segments.g;
            return *this;
        }

        //! \brief Sets the uv tiling in x and y direction for texture coordinate generation for vertical faces.
        //! \param[in] tiling The uv tiling in x and y direction.
        //! \return A reference to the \a plane_factory.
        box_factory& set_uv_tiling_vertical(const glm::vec2& tiling)
        {
            m_uv_tiling.r = tiling.r;
            m_uv_tiling.g = tiling.g;
            return *this;
        }

        //! \brief Sets the uv tiling in x and y direction for texture coordinate generation for horizontal faces.
        //! \param[in] tiling The uv tiling in x and y direction.
        //! \return A reference to the \a plane_factory.
        box_factory& set_uv_tiling_horizontal(const glm::vec2& tiling)
        {
            m_uv_tiling.b = tiling.r;
            m_uv_tiling.a = tiling.g;
            return *this;
        }

      private:
        //! \brief True if texture coordinates should be generated, else false.
        bool m_generate_texcoords = true;
        //! \brief True if normals should be generated, else false.
        bool m_generate_normals = false;
        //! \brief The number of segments to generate.
        glm::ivec4 m_segments = glm::ivec4(1);
        //! \brief The uv tiling to use for generation.
        glm::vec4 m_uv_tiling = glm::vec4(1);

        bool create_texture_coordinates() override
        {
            return m_generate_texcoords;
        };
        bool create_normals() override
        {
            return m_generate_normals;
        };
    };

    //! \brief Factory creating a sphere geometry.
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

        //! \brief Sets the number of segments in x direction to generate.
        //! \param[in] segments The number of segments x direction.
        //! \return A reference to the \a plane_factory.
        sphere_factory& set_segments(int32 segments)
        {
            m_segments.x = segments;
            return *this;
        }

        //! \brief Sets the number of rings in y direction to generate.
        //! \param[in] rings The number of rings in y direction.
        //! \return A reference to the \a plane_factory.
        sphere_factory& set_rings(int32 rings)
        {
            m_segments.y = rings;
            return *this;
        }

        //! \brief Sets the uv tiling in x and y direction for texture coordinate generation.
        //! \param[in] tiling The uv tiling in x and y direction.
        //! \return A reference to the \a plane_factory.
        sphere_factory& set_uv_tiling(const glm::vec2& tiling)
        {
            m_uv_tiling = tiling;
            return *this;
        }

      private:
        //! \brief True if texture coordinates should be generated, else false.
        bool m_generate_texcoords = true;
        //! \brief True if normals should be generated, else false.
        bool m_generate_normals = false;
        //! \brief The number of segments to generate.
        glm::ivec2 m_segments = glm::ivec2(44, 22);
        //! \brief The uv tiling to use for generation.
        glm::vec2 m_uv_tiling = glm::vec2(1);

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
