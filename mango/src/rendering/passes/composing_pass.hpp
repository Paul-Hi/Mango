//! \file      composing_pass.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2022
//! \copyright Apache License 2.0

#ifndef MANGO_COMPOSING_PASS_HPP
#define MANGO_COMPOSING_PASS_HPP

#include <graphics/graphics.hpp>
#include <rendering/passes/render_pass.hpp>

namespace mango
{
    //! \brief A \a render_pass for composition (does tonemapping and more).
    class composing_pass : public render_pass
    {
      public:
        composing_pass(const composing_settings& settings);
        ~composing_pass() = default;

        void attach(const shared_ptr<context_impl>& context) override;
        void execute(graphics_device_context_handle& device_context) override;

        void on_ui_widget() override;

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

        //! \brief Set the viewport.
        //! \param[in] viewport The viewport to use.
        inline void set_viewport(const gfx_viewport& viewport)
        {
            m_viewport = viewport;
        }

        //! \brief Set the hdr input texture.
        //! \param[in] hdr_input The hdr input texture to set.
        inline void set_hdr_input(const gfx_handle<const gfx_texture>& hdr_input)
        {
            m_hdr_input = hdr_input;
        }

        //! \brief Set the sampler for the hdr input texture.
        //! \param[in] hdr_input_sampler The sampler to use.
        inline void set_hdr_input_sampler(const gfx_handle<const gfx_sampler>& hdr_input_sampler)
        {
            m_hdr_input_sampler = hdr_input_sampler;
        }

        //! \brief Set the depth input texture.
        //! \param[in] depth_input The depth input texture to set.
        inline void set_depth_input(const gfx_handle<const gfx_texture>& depth_input)
        {
            m_depth_input = depth_input;
        }

        //! \brief Set the sampler for the depth input texture.
        //! \param[in] depth_input_sampler The sampler to use.
        inline void set_depth_input_sampler(const gfx_handle<const gfx_sampler>& depth_input_sampler)
        {
            m_depth_input_sampler = depth_input_sampler;
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
        //! \brief The fragment \a gfx_shader_stage for the composing pass.
        gfx_handle<const gfx_shader_stage> m_composing_pass_fragment;
        //! \brief Graphics \a gfx_pipeline for composing
        gfx_handle<const gfx_pipeline> m_composing_pass_pipeline;

        //! \brief The \a gfx_viewport to render to.
        gfx_viewport m_viewport;

        //! \brief The render targets to render to.
        std::vector<gfx_handle<const gfx_texture>> m_render_targets;

        //! \brief The camera data \a gfx_buffer.
        gfx_handle<const gfx_buffer> m_camera_data_buffer;
        //! \brief The renderer data \a gfx_buffer.
        gfx_handle<const gfx_buffer> m_renderer_data_buffer;

        //! \brief The hdr input \a gfx_texture.
        gfx_handle<const gfx_texture> m_hdr_input;
        //! \brief The hdr input \a gfx_sampler.
        gfx_handle<const gfx_sampler> m_hdr_input_sampler;
        //! \brief The depth input \a gfx_texture.
        gfx_handle<const gfx_texture> m_depth_input;
        //! \brief The depth input \a gfx_sampler.
        gfx_handle<const gfx_sampler> m_depth_input_sampler;

        //! \brief The fxaa data buffer.
        gfx_handle<const gfx_buffer> m_composing_data_buffer;

        // TODO NEXT: Create settings in code: not everyone uses editor...
        //! \brief The current \a composing_data.
        composing_data m_composing_data;
    };
} // namespace mango

#endif // MANGO_COMPOSING_PASS_HPP
