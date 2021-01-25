//! \file      cubemap_step.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_cubemap_step_HPP
#define MANGO_cubemap_step_HPP

#include <rendering/steps/pipeline_step.hpp>

namespace mango
{
    //! \brief A pipeline step adding image based lighting.
    class cubemap_step : public pipeline_step
    {
      public:
        bool create() override;
        void update(float dt) override;

        void attach() override;

        //! \brief Configures the \a cubemap_step.
        //! \param[in] configuration The \a cubemap_step_configuration to use.
        void configure(const cubemap_step_configuration& configuration);
        void execute(gpu_buffer_ptr frame_uniform_buffer) override;

        void destroy() override;

        //! \brief Sets the active cubemap to render.
        //! \param[in] cubemap Pointer to the cubemap texture to set.
        //! \param[in] model_matrix A model matrix for the cubemap.
        void set_cubemap(const texture_ptr& cubemap = nullptr, const glm::mat4& model_matrix = glm::mat4(1.0f))
        {
            m_current_cubemap           = cubemap;
            m_cubemap_data.model_matrix = model_matrix;
        }

        //! \brief Returns a shared_ptr to the \a command_buffer of the cubemap step.
        //! \details The returned \a command_buffer gets executed by the rendering system.
        //! \return A shared_ptr to the cubemap step \a command_buffer.
        inline command_buffer_ptr<min_key> get_cubemap_commands()
        {
            return m_cubemap_command_buffer;
        }

        void on_ui_widget() override;

      private:
        bool setup_shader_programs() override;
        bool setup_buffers() override;

        //! \brief The \a command_buffer storing all cubemap step related commands.
        command_buffer_ptr<min_key> m_cubemap_command_buffer;

        //! \brief The cube geometry used for rendering the skybox.
        vertex_array_ptr m_cube_geometry;

        //! \brief The current cubemap to render.
        texture_ptr m_current_cubemap;

        //! \brief Shader program to render a cubemap.
        shader_program_ptr m_draw_environment;

        //! \brief Uniform buffer struct for cubemap data.
        struct cubemap_data
        {
            std140_mat4 model_matrix;  //!< Rotation and scale for the cubemap (currently unused).
            std140_float render_level; //!< The miplevel to render the cubemap with.
            std140_float p0;           //!< Padding.
            std140_float p1;           //!< Padding.
            std140_float p2;           //!< Padding.
        } m_cubemap_data;              //!< Current cubemap_data.
    };
} // namespace mango

#endif // MANGO_cubemap_step_HPP
