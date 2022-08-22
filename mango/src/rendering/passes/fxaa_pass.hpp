//! \file      fxaa_pass.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2022
//! \copyright Apache License 2.0

#ifndef MANGO_FXAA_PASS_HPP
#define MANGO_FXAA_PASS_HPP

#include <rendering/passes/render_pass.hpp>
#include <graphics/graphics.hpp>

namespace mango
{
    //! \brief A pipeline pass adding Fast Approximate Anti Aliasing.
    class fxaa_pass : public render_pass
    {
      public:
        //! \brief Constructs a the \a fxaa_pass.
        //! \param[in] settings The \a fxaa_settings to use.
        fxaa_pass(const fxaa_settings& settings);
        ~fxaa_pass();

        void attach(const shared_ptr<context_impl>& context) override;
        void execute(graphics_device_context_handle& device_context) override;
        void on_ui_widget() override;

        inline render_pass_execution_info get_info() override
        {
            return s_rpei;
        }

        //! \brief Sets the input texture for the \a fxaa_pass.
        //! \param[in] input_texture The texture to use.
        inline void set_input_texture(const gfx_handle<const gfx_texture>& input_texture)
        {
            m_texture_input = input_texture;
        }

        //! \brief Sets the output render target for the \a fxaa_pass.
        //! \param[in] output_target The \a gfx_texture to use as output color target.
        //! \param[in] output_depth_stencil_target The \a gfx_texture to use as output depth stencil target.
        inline void set_output_targets(const gfx_handle<const gfx_texture>& output_target, const gfx_handle<const gfx_texture>& output_depth_stencil_target)
        {
            m_output_target               = output_target;
            m_output_target_depth_stencil = output_depth_stencil_target;
        }

      private:
        //! \brief Execution info of this pass.
        static const render_pass_execution_info s_rpei;

        bool create_pass_resources() override;

        //! \brief Input texture.
        gfx_handle<const gfx_texture> m_texture_input;
        //! \brief Input sampler.
        gfx_handle<const gfx_sampler> m_sampler_input;

        //! \brief The \a gfx_texture to use as output color target.
        gfx_handle<const gfx_texture> m_output_target;
        //! \brief The \a gfx_texture to use as output depth stencil target.
        gfx_handle<const gfx_texture> m_output_target_depth_stencil;

        //! \brief The vertex \a gfx_shader_stage for the cubemap pass.
        gfx_handle<const gfx_shader_stage> m_fxaa_pass_vertex;
        //! \brief The fragment \a gfx_shader_stage for the cubemap pass.
        gfx_handle<const gfx_shader_stage> m_fxaa_pass_fragment;
        //! \brief Pipeline to anti alias an input with fxaa.
        gfx_handle<const gfx_pipeline> m_fxaa_pass_pipeline;

        //! \brief The fxaa data buffer.
        gfx_handle<const gfx_buffer> m_fxaa_data_buffer;

        //! \brief The current \a fxaa_data.
        fxaa_data m_fxaa_data;

        //! \brief The \a fxaa_settings for the pass.
        fxaa_settings m_settings;
    };
} // namespace mango

#endif // MANGO_FXAA_PASS_HPP
