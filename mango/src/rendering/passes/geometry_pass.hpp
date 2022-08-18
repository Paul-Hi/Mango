//! \file      geometry_pass.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2022
//! \copyright Apache License 2.0

#ifndef MANGO_GEOMETRY_PASS_HPP
#define MANGO_GEOMETRY_PASS_HPP

#include <graphics/graphics.hpp>
#include <rendering/debug_drawer.hpp>
#include <rendering/passes/render_pass.hpp>
#include <rendering/renderer_pipeline_cache.hpp>

namespace mango
{
    class geometry_pass : public render_pass
    {
      public:
        geometry_pass() = default;
        ~geometry_pass() = default;

        void setup(const shared_ptr<renderer_pipeline_cache>& pipeline_cache, const shared_ptr<debug_drawer>& dbg_drawer);

        void attach(const shared_ptr<context_impl>& context) override;
        void execute(graphics_device_context_handle& device_context) override;

        void on_ui_widget() override{};

        inline render_pass_execution_info get_info() override
        {
            return m_rpei;
        }

        inline void set_camera_data_buffer(const gfx_handle<const gfx_buffer>& camera_data_buffer)
        {
            m_camera_data_buffer = camera_data_buffer;
        }

        inline void set_viewport(const gfx_viewport& viewport)
        {
            m_viewport = viewport;
        }

        inline void set_scene_pointer(scene_impl* scene)
        {
            m_scene = scene;
        }

        inline void set_render_targets(const std::vector<gfx_handle<const gfx_texture>>& render_targets)
        {
            m_render_targets = render_targets;
        }

        inline void set_frustum_culling(bool frustum_culling)
        {
            m_frustum_culling = frustum_culling;
        }

        inline void set_debug_bounds(bool debug_bounds)
        {
            m_debug_bounds = debug_bounds;
        }

        inline void set_wireframe(bool wireframe)
        {
            m_wireframe = wireframe;
        }

        inline void set_default_texture_2D(const gfx_handle<const gfx_texture>& default_texture_2D)
        {
            m_default_texture_2D = default_texture_2D;
        }

        inline void set_camera_frustum(const bounding_frustum& camera_frustum)
        {
            m_camera_frustum = camera_frustum;
        }

        inline void set_opaque_count(int32 opaque_count)
        {
            m_opaque_count = opaque_count;
        }

        inline void set_draws(const shared_ptr<std::vector<draw_key>>& draws)
        {
            m_draws = draws;
        }

      private:
        //! \brief Execution info of this pass.
        render_pass_execution_info m_rpei;

        bool create_pass_resources() override;

        //! \brief The vertex \a gfx_shader_stage for the deferred geometry pass.
        gfx_handle<const gfx_shader_stage> m_geometry_pass_vertex;
        //! \brief The fragment \a gfx_shader_stage for the deferred geometry pass.
        gfx_handle<const gfx_shader_stage> m_geometry_pass_fragment;

        //! \brief The \a renderer_pipeline_cache to create and cache \a gfx_pipelines for the geometry.
        shared_ptr<renderer_pipeline_cache> m_pipeline_cache;

        //! \brief The shared \a debug_drawer to debug draw.
        shared_ptr<debug_drawer> m_debug_drawer;

        scene_impl* m_scene;

        gfx_viewport m_viewport;

        bounding_frustum m_camera_frustum;

        std::vector<gfx_handle<const gfx_texture>> m_render_targets;

        gfx_handle<const gfx_buffer> m_camera_data_buffer;

        bool m_frustum_culling;
        bool m_debug_bounds;
        bool m_wireframe;

        int32 m_opaque_count;

        gfx_handle<const gfx_texture> m_default_texture_2D;

        shared_ptr<std::vector<draw_key>> m_draws;
    };
} // namespace mango

#endif // MANGO_GEOMETRY_PASS_HPP
