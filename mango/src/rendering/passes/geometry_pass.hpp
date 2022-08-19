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
    //! \brief A \a render_pass drawing opaque geometry to the gbuffer.
    class geometry_pass : public render_pass
    {
      public:
        geometry_pass() = default;
        ~geometry_pass() = default;

        //! \brief Additional setup function - needs to be called before attach() is called.
        //! \param[in] pipeline_cache Shared \a renderer_pipeline_cache to add and retrieve pipelines.
        //! \param[in] dbg_drawer Shared \a debug_drawer to draw debug lines.
        void setup(const shared_ptr<renderer_pipeline_cache>& pipeline_cache, const shared_ptr<debug_drawer>& dbg_drawer);

        void attach(const shared_ptr<context_impl>& context) override;
        void execute(graphics_device_context_handle& device_context) override;

        void on_ui_widget() override{};

        inline render_pass_execution_info get_info() override
        {
            return m_rpei;
        }

        //! \brief Set the camera data buffer.
        //! \param[in] camera_data_buffer The camera data buffer.
        inline void set_camera_data_buffer(const gfx_handle<const gfx_buffer>& camera_data_buffer)
        {
            m_camera_data_buffer = camera_data_buffer;
        }

        //! \brief Set the viewport.
        //! \param[in] viewport The viewport to use.
        inline void set_viewport(const gfx_viewport& viewport)
        {
            m_viewport = viewport;
        }

        //! \brief Set \a scene_impl pointer.
        //! \param[in] scene Pointer to the \a scene_impl to use for retrieving geometry data.
        inline void set_scene_pointer(scene_impl* scene)
        {
            m_scene = scene;
        }

        //! \brief Set the render targets.
        //! \param[in] render_targets List of render targets, last one is depth (-stencil).
        inline void set_render_targets(const std::vector<gfx_handle<const gfx_texture>>& render_targets)
        {
            m_render_targets = render_targets;
        }

        //! \brief Set frustum culling.
        //! \param[in] frustum_culling True if frustum culling is enabled, else false.
        inline void set_frustum_culling(bool frustum_culling)
        {
            m_frustum_culling = frustum_culling;
        }

        //! \brief Set debug bounds drawing.
        //! \param[in] debug_bounds True if drawing debug bounds is enabled, else false.
        inline void set_debug_bounds(bool debug_bounds)
        {
            m_debug_bounds = debug_bounds;
        }

        //! \brief Set wireframe drawing.
        //! \param[in] wireframe True if wireframe drawing is enabled, else false.
        inline void set_wireframe(bool wireframe)
        {
            m_wireframe = wireframe;
        }

        //! \brief Set a default 2d texture.
        //! \param[in] default_texture_2D The default 2d texture.
        inline void set_default_texture_2D(const gfx_handle<const gfx_texture>& default_texture_2D)
        {
            m_default_texture_2D = default_texture_2D;
        }

        //! \brief Set the camera frustum.
        //! \param[in] camera_frustum The cameras \a bounding_frustum.
        inline void set_camera_frustum(const bounding_frustum& camera_frustum)
        {
            m_camera_frustum = camera_frustum;
        }

        //! \brief Set the number of opaque draw calls in draws.
        //! \param[in] opaque_count The number of opaque draw calls in draws.
        inline void set_opaque_count(int32 opaque_count)
        {
            m_opaque_count = opaque_count;
        }

        //! \brief Set draws.
        //! \param[in] draws The list of \a draw_keys.
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

        //! \brief Pointer to the \a scene_impl to query data for rendering.
        scene_impl* m_scene;

        //! \brief The \a gfx_viewport to render to.
        gfx_viewport m_viewport;

        //! \brief The \a bounding_frustum of the camera.
        bounding_frustum m_camera_frustum;

        //! \brief The render targets to render to.
        std::vector<gfx_handle<const gfx_texture>> m_render_targets;

        //! \brief The camera data \a gfx_buffer.
        gfx_handle<const gfx_buffer> m_camera_data_buffer;

        //! \brief True if frustum culling is enabled, else false.
        bool m_frustum_culling;
        //! \brief True if drawing debug bounds is enabled, else false.
        bool m_debug_bounds;
        //! \brief True if wireframe drawing is enabled, else false.
        bool m_wireframe;

        //! \brief The number of opaque draws to draw.
        int32 m_opaque_count;

        //! \brief The default 2d \a gfx_texture.
        gfx_handle<const gfx_texture> m_default_texture_2D;

        //! \brief The list of \a draw_keys.
        shared_ptr<std::vector<draw_key>> m_draws;
    };
} // namespace mango

#endif // MANGO_GEOMETRY_PASS_HPP
