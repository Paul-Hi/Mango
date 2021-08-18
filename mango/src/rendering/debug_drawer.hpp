//! \file      debug_drawer.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_DEBUG_DRAWER_HPP
#define MANGO_DEBUG_DRAWER_HPP

#include <core/context_impl.hpp>
#include <graphics/graphics.hpp>

namespace mango
{
    class debug_drawer
    {
      public:
        debug_drawer(const shared_ptr<context_impl>& context);
        ~debug_drawer() = default;

        void set_color(const color_rgb& color);

        void clear();

        void add(const vec3& point0, const vec3& point1);

        void update_buffer();

        void execute();

        inline int32 vertex_count()
        {
            return m_vertex_count;
        }

      private:
        bool create_pipeline_resources();

        shared_ptr<context_impl> m_shared_context;

        color_rgb m_color;

        std::vector<vec3> m_vertices; // position, color, position, color ...

        //! \brief The cube vertices used for rendering the skybox.
        gfx_handle<const gfx_buffer> m_vertex_buffer;

        int32 m_buffer_size;

        int32 m_vertex_count;

        //! \brief The vertex \a shader_stage for the debug draw pass.
        gfx_handle<const gfx_shader_stage> m_debug_draw_vertex;
        //! \brief The fragment \a shader_stage for the debug draw pass.
        gfx_handle<const gfx_shader_stage> m_debug_draw_fragment;
        //! \brief Graphics pipeline to render the debug draw.
        gfx_handle<const gfx_pipeline> m_debug_draw_pipeline;
    };
} // namespace mango

#endif // MANGO_DEBUG_DRAWER_HPP
