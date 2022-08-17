//! \file      transparent_pass.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2022
//! \copyright Apache License 2.0

#ifndef MANGO_TRANSPARENT_PASS_HPP
#define MANGO_TRANSPARENT_PASS_HPP

#include <graphics/graphics.hpp>
#include <rendering/debug_drawer.hpp>
#include <rendering/passes/render_pass.hpp>
#include <rendering/renderer_pipeline_cache.hpp>

namespace mango
{
    class transparent_pass : public render_pass
    {
      public:
        transparent_pass() = default;
        ~transparent_pass() = default;

        void setup(const shared_ptr<renderer_pipeline_cache>& pipeline_cache, const shared_ptr<debug_drawer>& dbg_drawer);

        void attach(const shared_ptr<context_impl>& context) override;
        void execute(graphics_device_context_handle& device_context) override;

        inline render_pass_execution_info get_info() override
        {
            return m_rpei;
        }

        inline void set_viewport(const gfx_viewport& viewport)
        {
            m_viewport = viewport;
        }

        inline void set_scene_pointer(scene_impl* scene)
        {
            m_scene = scene;
        }

        inline void set_camera_data_buffer(const gfx_handle<const gfx_buffer>& camera_data_buffer)
        {
            m_camera_data_buffer = camera_data_buffer;
        }

        inline void set_renderer_data_buffer(const gfx_handle<const gfx_buffer>& renderer_data_buffer)
        {
            m_renderer_data_buffer = renderer_data_buffer;
        }

        inline void set_light_data_buffer(const gfx_handle<const gfx_buffer>& light_data_buffer)
        {
            m_light_data_buffer = light_data_buffer;
        }

        inline void set_shadow_data_buffer(const gfx_handle<const gfx_buffer>& shadow_data_buffer)
        {
            m_shadow_data_buffer = shadow_data_buffer;
        }

        inline void set_irradiance_map(const gfx_handle<const gfx_texture>& irradiance_map)
        {
            m_irradiance_map = irradiance_map;
        }

        inline void set_irradiance_map_sampler(const gfx_handle<const gfx_sampler>& irradiance_map_sampler)
        {
            m_irradiance_map_sampler = irradiance_map_sampler;
        }

        inline void set_radiance_map(const gfx_handle<const gfx_texture>& radiance_map)
        {
            m_radiance_map = radiance_map;
        }

        inline void set_radiance_map_sampler(const gfx_handle<const gfx_sampler>& radiance_map_sampler)
        {
            m_radiance_map_sampler = radiance_map_sampler;
        }

        inline void set_brdf_integration_lut(const gfx_handle<const gfx_texture>& brdf_integration_lut)
        {
            m_brdf_integration_lut = brdf_integration_lut;
        }

        inline void set_brdf_integration_lut_sampler(const gfx_handle<const gfx_sampler>& brdf_integration_lut_sampler)
        {
            m_brdf_integration_lut_sampler = brdf_integration_lut_sampler;
        }

        inline void set_shadow_map(const gfx_handle<const gfx_texture>& shadow_map)
        {
            m_shadow_map                 = shadow_map;
        }

        inline void set_shadow_map_sampler(const gfx_handle<const gfx_sampler>& shadow_map_sampler)
        {
            m_shadow_map_sampler         = shadow_map_sampler;
        }

        inline void set_shadow_map_compare_sampler(const gfx_handle<const gfx_sampler>& shadow_map_compare_sampler)
        {
            m_shadow_map_compare_sampler = shadow_map_compare_sampler;
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

        inline void set_transparent_start(int32 transparent_start)
        {
            m_transparent_start = transparent_start;
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
        gfx_handle<const gfx_shader_stage> m_transparent_pass_vertex;
        //! \brief The fragment \a gfx_shader_stage for the deferred geometry pass.
        gfx_handle<const gfx_shader_stage> m_transparent_pass_fragment;

        //! \brief The \a renderer_pipeline_cache to create and cache \a gfx_pipelines for the geometry.
        shared_ptr<renderer_pipeline_cache> m_pipeline_cache;

        //! \brief The shared \a debug_drawer to debug draw.
        shared_ptr<debug_drawer> m_debug_drawer;

        scene_impl* m_scene;

        gfx_viewport m_viewport;

        bounding_frustum m_camera_frustum;

        std::vector<gfx_handle<const gfx_texture>> m_render_targets;

        gfx_handle<const gfx_buffer> m_camera_data_buffer;
        gfx_handle<const gfx_buffer> m_renderer_data_buffer;
        gfx_handle<const gfx_buffer> m_light_data_buffer;
        gfx_handle<const gfx_buffer> m_shadow_data_buffer;

        gfx_handle<const gfx_texture> m_irradiance_map;
        gfx_handle<const gfx_sampler> m_irradiance_map_sampler;
        gfx_handle<const gfx_texture> m_radiance_map;
        gfx_handle<const gfx_sampler> m_radiance_map_sampler;
        gfx_handle<const gfx_texture> m_brdf_integration_lut;
        gfx_handle<const gfx_sampler> m_brdf_integration_lut_sampler;

        gfx_handle<const gfx_texture> m_shadow_map;
        gfx_handle<const gfx_sampler> m_shadow_map_sampler;
        gfx_handle<const gfx_sampler> m_shadow_map_compare_sampler;

        bool m_frustum_culling;
        bool m_debug_bounds;
        bool m_wireframe;

        int32 m_transparent_start;

        gfx_handle<const gfx_texture> m_default_texture_2D;

        shared_ptr<std::vector<draw_key>> m_draws;
    };
} // namespace mango

#endif // MANGO_TRANSPARENT_PASS_HPP
