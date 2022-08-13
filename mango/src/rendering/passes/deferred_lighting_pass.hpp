//! \file      deferred_lighting_pass.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2022
//! \copyright Apache License 2.0

#ifndef MANGO_DEFERRED_LIGHTING_PASS_HPP
#define MANGO_DEFERRED_LIGHTING_PASS_HPP

#include <graphics/graphics.hpp>
#include <rendering/passes/render_pass.hpp>

namespace mango
{
    class deferred_lighting_pass : public render_pass
    {
      public:
        deferred_lighting_pass()  = default;
        ~deferred_lighting_pass() = default;

        void attach(const shared_ptr<context_impl>& context) override;
        void execute(graphics_device_context_handle& device_context) override;

        inline render_pass_execution_info get_info() override
        {
            return s_rpei;
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

        inline void set_viewport(const gfx_viewport& viewport)
        {
            m_viewport = viewport;
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

        inline void set_gbuffer(const std::vector<gfx_handle<const gfx_texture>>& gbuffer, const gfx_handle<const gfx_sampler>& gbuffer_sampler)
        {
            m_gbuffer         = gbuffer;
            m_gbuffer_sampler = gbuffer_sampler;
        }

        inline void set_render_targets(const std::vector<gfx_handle<const gfx_texture>>& render_targets)
        {
            m_render_targets = render_targets;
        }

      private:
        //! \brief Executioin info of this pass. Has to be defined in cpp.
        static const render_pass_execution_info s_rpei;

        bool create_pass_resources() override;

        //! \brief The vertex \a gfx_shader_stage producing a screen space triangle.
        gfx_handle<const gfx_shader_stage> m_screen_space_triangle_vertex;
        //! \brief The fragment \a gfx_shader_stage for the deferred lighting pass.
        gfx_handle<const gfx_shader_stage> m_lighting_pass_fragment;
        //! \brief Graphics \a gfx_pipeline calculating deferred lighting.
        gfx_handle<const gfx_pipeline> m_lighting_pass_pipeline;

        gfx_viewport m_viewport;

        std::vector<gfx_handle<const gfx_texture>> m_render_targets;

        std::vector<gfx_handle<const gfx_texture>> m_gbuffer;
        gfx_handle<const gfx_sampler> m_gbuffer_sampler;

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
    };
} // namespace mango

#endif // MANGO_DEFERRED_LIGHTING_PASS_HPP
