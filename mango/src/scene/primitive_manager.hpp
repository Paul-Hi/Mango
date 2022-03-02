//! \file      primitive_manager.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2022
//! \copyright Apache License 2.0

#ifndef MANGO_PRIMITIVE_MANAGER_HPP
#define MANGO_PRIMITIVE_MANAGER_HPP

#include <mango/types.hpp>
#include <resources/primitive_builder.hpp>
#include <scene/scene_structures_internal.hpp>
#include <mango/packed_freelist.hpp>

namespace mango
{
    class primitive_manager
    {
      public:
        primitive_manager();

        primitive_gpu_data add_primitive(primitive_builder& builder);
        void remove_primitive(uid manager_id);

        void generate_buffers(const graphics_device_handle& graphics_device);

        void bind_buffers(const graphics_device_context_handle& frame_context, const gfx_handle<const gfx_buffer> indirect_buffer, int32 indirect_buffer_offset) const;

        inline int32 get_vertex_count() const
        {
            return m_vertices;
        }

        inline bool get_draw_parameters(uid manager_id, int64* out_index_offset, int32* out_index_count, int32* out_vertex_count, int32* out_base_vertex) const
        {
            if (!manager_id.is_valid() || out_index_offset == nullptr || out_index_count == nullptr || out_base_vertex == nullptr)
            {
                return false;
            }

            if (!m_internal_data.contains(manager_id))
            {
                MANGO_LOG_WARN("Can not retrieve draw parameters! Manager Id is not valid!", manager_id.get());
                return false;
            }

            const managed_data& data = m_internal_data.at(manager_id);

            *out_index_offset = data.draw_call_desc.index_offset;
            *out_index_count  = data.draw_call_desc.index_count;
            *out_vertex_count = data.draw_call_desc.vertex_count;
            *out_base_vertex  = data.draw_call_desc.base_vertex;

            return true;
        }

      private:
        struct managed_data
        {
            //! \brief The position buffer data.
            std::vector<vec3> position_data;
            //! \brief The normal buffer data.
            std::vector<vec3> normal_data;
            //! \brief The uv buffer data.
            std::vector<vec2> uv_data;
            //! \brief The tangent buffer data.
            std::vector<vec4> tangent_data;
            //! \brief The index buffer data. Always blown up to uint32.
            std::vector<uint32> index_data;
            //! \brief The \a draw_call_description providing information to schedule a draw call for this \a primitive_gpu_data.
            draw_call_description draw_call_desc;
        };

        packed_freelist<managed_data, 16384> m_internal_data;

        gfx_handle<const gfx_buffer> m_position_buffer;
        gfx_handle<const gfx_buffer> m_normal_buffer;
        gfx_handle<const gfx_buffer> m_uv_buffer;
        gfx_handle<const gfx_buffer> m_tangent_buffer;
        gfx_handle<const gfx_buffer> m_index_buffer;

        int32 m_vertices;
        int32 m_indices;
    };

} // namespace mango

#endif // MANGO_PRIMITIVE_MANAGER_HPP
