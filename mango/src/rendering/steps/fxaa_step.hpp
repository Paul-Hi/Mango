//! \file      fxaa_step.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_FXAA_STEP_HPP
#define MANGO_FXAA_STEP_HPP

#include <rendering/steps/render_step.hpp>

namespace mango
{
    //! \brief A pipeline step adding Fast Approximate Anti Aliasing.
    class fxaa_step : public render_step
    {
      public:
        //! \brief Constructs a the \a fxaa_step.
        //! \param[in] settings The \a fxaa_settings to use.
        fxaa_step(const fxaa_settings& settings);
        ~fxaa_step();

        void attach(const shared_ptr<context_impl>& context) override;
        void execute() override;
        void on_ui_widget() override;

        //! \brief Sets the input texture for the \a fxaa_step.
        //! \param[in] input_texture The texture to use.
        inline void set_input_texture(const gfx_handle<const gfx_texture>& input_texture)
        {
            m_texture_input = input_texture;
        }

        //! \brief Sets the output render target for the \a fxaa_step.
        //! \param[in] output_target The \a gfx_texture to use as output color target.
        //! \param[in] output_depth_stencil_target The \a gfx_texture to use as output depth stencil target.
        inline void set_output_targets(const gfx_handle<const gfx_texture>& output_target, const gfx_handle<const gfx_texture>& output_depth_stencil_target)
        {
            m_output_target               = output_target;
            m_output_target_depth_stencil = output_depth_stencil_target;
        }

      private:
        bool create_step_resources() override;

        //! \brief Input texture.
        gfx_handle<const gfx_texture> m_texture_input;
        //! \brief Input sampler.
        gfx_handle<const gfx_sampler> m_sampler_input;

        //! \brief The \a gfx_texture to use as output color target.
        gfx_handle<const gfx_texture> m_output_target;
        //! \brief The \a gfx_texture to use as output depth stencil target.
        gfx_handle<const gfx_texture> m_output_target_depth_stencil;

        //! \brief The vertex \a shader_stage for the cubemap pass.
        gfx_handle<const gfx_shader_stage> m_fxaa_pass_vertex;
        //! \brief The fragment \a shader_stage for the cubemap pass.
        gfx_handle<const gfx_shader_stage> m_fxaa_pass_fragment;
        //! \brief Pipeline to anti alias an input with fxaa.
        gfx_handle<const gfx_pipeline> m_fxaa_pass_pipeline;

        //! \brief The fxaa data buffer.
        gfx_handle<const gfx_buffer> m_fxaa_data_buffer;

    //! \brief Uniform buffer struct for fxaa data.
    //! \details Bound to binding point 1.
        struct fxaa_data
        {
            std140_vec2 inverse_screen_size;     //!< The inverse screen size.
            std140_int quality_preset    = 0;    //!< The fxaa_quality_preset.
            std140_float subpixel_filter = 0.0f; //!< The filter value for subpixels.
        };
        //! \brief The current \a fxaa_data.
        fxaa_data m_fxaa_data;

        //! \brief The \a fxaa_settings for the step.
        fxaa_settings m_settings;
    };
} // namespace mango

#endif // MANGO_FXAA_STEP_HPP
