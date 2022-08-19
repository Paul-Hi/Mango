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
    //! \brief A \a render_pass calculating deferred lighting given a gbuffer.
    class deferred_lighting_pass : public render_pass
    {
      public:
        deferred_lighting_pass()  = default;
        ~deferred_lighting_pass() = default;

        void attach(const shared_ptr<context_impl>& context) override;
        void execute(graphics_device_context_handle& device_context) override;

        void on_ui_widget() override{};

        inline render_pass_execution_info get_info() override
        {
            return s_rpei;
        }

        //! \brief Set the camera data buffer.
        //! \param[in] camera_data_buffer The camera data buffer.
        inline void set_camera_data_buffer(const gfx_handle<const gfx_buffer>& camera_data_buffer)
        {
            m_camera_data_buffer = camera_data_buffer;
        }

        //! \brief Set the renderer data buffer.
        //! \param[in] renderer_data_buffer The renderer data buffer.
        inline void set_renderer_data_buffer(const gfx_handle<const gfx_buffer>& renderer_data_buffer)
        {
            m_renderer_data_buffer = renderer_data_buffer;
        }

        //! \brief Set the light data buffer.
        //! \param[in] light_data_buffer The light data buffer.
        inline void set_light_data_buffer(const gfx_handle<const gfx_buffer>& light_data_buffer)
        {
            m_light_data_buffer = light_data_buffer;
        }

        //! \brief Set the shadow data buffer.
        //! \param[in] shadow_data_buffer The shadow data buffer.
        inline void set_shadow_data_buffer(const gfx_handle<const gfx_buffer>& shadow_data_buffer)
        {
            m_shadow_data_buffer = shadow_data_buffer;
        }

        //! \brief Set the viewport.
        //! \param[in] viewport The viewport to use.
        inline void set_viewport(const gfx_viewport& viewport)
        {
            m_viewport = viewport;
        }

        //! \brief Set the irradiance map.
        //! \param[in] irradiance_map The irradiance map to sample.
        inline void set_irradiance_map(const gfx_handle<const gfx_texture>& irradiance_map)
        {
            m_irradiance_map = irradiance_map;
        }

        //! \brief Set the sampler for the irradiance map.
        //! \param[in] irradiance_map_sampler The sampler to use.
        inline void set_irradiance_map_sampler(const gfx_handle<const gfx_sampler>& irradiance_map_sampler)
        {
            m_irradiance_map_sampler = irradiance_map_sampler;
        }

        //! \brief Set the radiance map.
        //! \param[in] radiance_map The radiance map to sample.
        inline void set_radiance_map(const gfx_handle<const gfx_texture>& radiance_map)
        {
            m_radiance_map = radiance_map;
        }

        //! \brief Set the sampler for the radiance map.
        //! \param[in] radiance_map_sampler The sampler to use.
        inline void set_radiance_map_sampler(const gfx_handle<const gfx_sampler>& radiance_map_sampler)
        {
            m_radiance_map_sampler = radiance_map_sampler;
        }

        //! \brief Set the brdf integration lookup texture.
        //! \param[in] brdf_integration_lut The brdf integration lookup.
        inline void set_brdf_integration_lut(const gfx_handle<const gfx_texture>& brdf_integration_lut)
        {
            m_brdf_integration_lut = brdf_integration_lut;
        }

        //! \brief Set the sampler for the brdf integration lookup texture.
        //! \param[in] brdf_integration_lut_sampler The sampler to use for the brdf integration lookup.
        inline void set_brdf_integration_lut_sampler(const gfx_handle<const gfx_sampler>& brdf_integration_lut_sampler)
        {
            m_brdf_integration_lut_sampler = brdf_integration_lut_sampler;
        }

        //! \brief Set the shadow map to use.
        //! \param[in] shadow_map The shadow map texture.
        inline void set_shadow_map(const gfx_handle<const gfx_texture>& shadow_map)
        {
            m_shadow_map = shadow_map;
        }

        //! \brief Set the shadow map basic sampler.
        //! \param[in] shadow_map_sampler The sampler for basic samples.
        inline void set_shadow_map_sampler(const gfx_handle<const gfx_sampler>& shadow_map_sampler)
        {
            m_shadow_map_sampler = shadow_map_sampler;
        }

        //! \brief Set the shadow map compare sampler.
        //! \param[in] shadow_map_compare_sampler The sampler for comparison samles.
        inline void set_shadow_map_compare_sampler(const gfx_handle<const gfx_sampler>& shadow_map_compare_sampler)
        {
            m_shadow_map_compare_sampler = shadow_map_compare_sampler;
        }

        //! \brief Set the gbuffer to sample from.
        //! \param[in] gbuffer List of \a gfx_textures used as gbuffer.
        //! \param[in] gbuffer_sampler The \a gfx_sampler to use for sampling the gbuffer.
        inline void set_gbuffer(const std::vector<gfx_handle<const gfx_texture>>& gbuffer, const gfx_handle<const gfx_sampler>& gbuffer_sampler)
        {
            m_gbuffer         = gbuffer;
            m_gbuffer_sampler = gbuffer_sampler;
        }

        //! \brief Set the render targets.
        //! \param[in] render_targets List of render targets, last one is depth (-stencil).
        inline void set_render_targets(const std::vector<gfx_handle<const gfx_texture>>& render_targets)
        {
            m_render_targets = render_targets;
        }

      private:
        //! \brief Execution info of this pass.
        static const render_pass_execution_info s_rpei;

        bool create_pass_resources() override;

        //! \brief The vertex \a gfx_shader_stage producing a screen space triangle.
        gfx_handle<const gfx_shader_stage> m_screen_space_triangle_vertex;
        //! \brief The fragment \a gfx_shader_stage for the deferred lighting pass.
        gfx_handle<const gfx_shader_stage> m_lighting_pass_fragment;
        //! \brief Graphics \a gfx_pipeline calculating deferred lighting.
        gfx_handle<const gfx_pipeline> m_lighting_pass_pipeline;

        //! \brief The \a gfx_viewport to render to.
        gfx_viewport m_viewport;

        //! \brief The render targets to render to.
        std::vector<gfx_handle<const gfx_texture>> m_render_targets;

        //! \brief The gbuffer to sample.
        std::vector<gfx_handle<const gfx_texture>> m_gbuffer;
        //! \brief The gbuffers \a gfx_sampler.
        gfx_handle<const gfx_sampler> m_gbuffer_sampler;

        //! \brief The camera data \a gfx_buffer.
        gfx_handle<const gfx_buffer> m_camera_data_buffer;
        //! \brief The renderer data \a gfx_buffer.
        gfx_handle<const gfx_buffer> m_renderer_data_buffer;
        //! \brief The light data \a gfx_buffer.
        gfx_handle<const gfx_buffer> m_light_data_buffer;
        //! \brief The shadow data \a gfx_buffer.
        gfx_handle<const gfx_buffer> m_shadow_data_buffer;

        //! \brief The irradiance map \a gfx_texture.
        gfx_handle<const gfx_texture> m_irradiance_map;
        //! \brief The irradiance map \a gfx_sampler.
        gfx_handle<const gfx_sampler> m_irradiance_map_sampler;
        //! \brief The radiance map \a gfx_texture.
        gfx_handle<const gfx_texture> m_radiance_map;
        //! \brief The radiance map \a gfx_sampler.
        gfx_handle<const gfx_sampler> m_radiance_map_sampler;
        //! \brief The brdf integration lookup \a gfx_texture.
        gfx_handle<const gfx_texture> m_brdf_integration_lut;
        //! \brief The brdf integration lookup \a gfx_sampler.
        gfx_handle<const gfx_sampler> m_brdf_integration_lut_sampler;

        //! \brief The shadow map \a gfx_texture.
        gfx_handle<const gfx_texture> m_shadow_map;
        //! \brief The shadow map basic \a gfx_sampler.
        gfx_handle<const gfx_sampler> m_shadow_map_sampler;
        //! \brief The shadow map comparison \a gfx_sampler.
        gfx_handle<const gfx_sampler> m_shadow_map_compare_sampler;
    };
} // namespace mango

#endif // MANGO_DEFERRED_LIGHTING_PASS_HPP
