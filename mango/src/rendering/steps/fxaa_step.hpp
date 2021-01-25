//! \file      fxaa_step.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_FXAA_STEP_HPP
#define MANGO_FXAA_STEP_HPP

#include <rendering/steps/pipeline_step.hpp>

namespace mango
{
    //! \brief A pipeline step adding Fast Approximate Anti Aliasing.
    class fxaa_step : public pipeline_step
    {
      public:
        bool create() override;
        void update(float dt) override;

        void attach() override;

        //! \brief Configures the \a fxaa_step.
        //! \param[in] configuration The \a fxaa_step_configuration to use.
        void configure(const fxaa_step_configuration& configuration);
        void execute(gpu_buffer_ptr frame_uniform_buffer) override;

        void destroy() override;

        //! \brief Sets the input texture for the \a fxaa_step.
        //! \param[in] input_texture The texture to use.
        inline void set_input_texture(const texture_ptr& input_texture)
        {
            if (m_input_texture != input_texture)
                m_fxaa_command_buffer->invalidate();
            m_input_texture = input_texture;
        }

        //! \brief Sets the output framebuffer for the \a fxaa_step.
        //! \param[in] output_buffer The framebuffer to use.
        inline void set_output_framebuffer(const framebuffer_ptr& output_buffer)
        {
            if (m_output_buffer != output_buffer)
                m_fxaa_command_buffer->invalidate();
            m_output_buffer = output_buffer;
        }

        //! \brief Returns a shared_ptr to the \a command_buffer of the fxaa step.
        //! \details The returned \a command_buffer gets executed by the rendering system.
        //! \return A shared_ptr to the fxaa step \a command_buffer.
        inline command_buffer_ptr<min_key> get_fxaa_commands()
        {
            return m_fxaa_command_buffer;
        }

        void on_ui_widget() override;

      private:
        bool setup_shader_programs() override;
        bool setup_buffers() override;

        //! \brief The \a command_buffer storing all fxaa step related commands.
        command_buffer_ptr<min_key> m_fxaa_command_buffer;
        //! \brief Input texture.
        texture_ptr m_input_texture;
        //! \brief Output framebuffer.
        framebuffer_ptr m_output_buffer;
        //! \brief Shader program to anti aliase an input with fxaa.
        shader_program_ptr m_fxaa_pass;

        //!\brief The \a fxaa_quality_preset.
        fxaa_quality_preset m_quality_preset = fxaa_quality_preset::medium_quality;
        //!\brief The filter value for subpixels.
        float m_subpixel_filter = 0.0f;
    };
} // namespace mango

#endif // MANGO_FXAA_STEP_HPP
