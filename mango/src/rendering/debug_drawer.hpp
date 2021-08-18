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
    //! \brief Batches and draws lines for debug purposes.
    class debug_drawer
    {
      public:
        //! \brief Constructs a new \a debug_drawer.
        //! \param[in] context The internally shared context of mango.
        debug_drawer(const shared_ptr<context_impl>& context);
        ~debug_drawer() = default;

        //! \brief Sets the color for the following lines.
        //! \param[in] color The color to set.
        void set_color(const color_rgb& color);

        //! \brief Clears the list of points.
        void clear();

        //! \brief Adds two points / a line.
        //! \param[in] point0 The first point of the line.
        //! \param[in] point1 The second point of the line.
        void add(const vec3& point0, const vec3& point1);

        //! \brief Updates the internal \a gfx_buffer with the current list of points and colors.
        void update_buffer();

        //! \brief Draws the lines.
        void execute();

        //! \brief Retrieves the number of vertices currently added.
        //! \return The number of currently added vertices.
        inline int32 vertex_count()
        {
            return m_vertex_count;
        }

      private:
        //! \brief Creates pipeline resources required for drawing the debug lines.
        //! \return True on success, else false.
        bool create_pipeline_resources();

        //! \brief Mangos internal context for shared usage.
        shared_ptr<context_impl> m_shared_context;

        //! \brief The currently set color for new lines.
        color_rgb m_color;

        //! \brief List of vertices.
        //! \details position0, color0, position1, color1 ...
        std::vector<vec3> m_vertices;

        //! \brief The vertex buffer used to render the lines.
        gfx_handle<const gfx_buffer> m_vertex_buffer;

        //! \brief Current size of the \a gfx_buffer.
        int32 m_buffer_size;

        //! \brief Current number of vertices in the \a gfx_buffer.
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
