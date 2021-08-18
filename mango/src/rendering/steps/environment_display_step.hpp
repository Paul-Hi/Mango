//! \file      environment_display_step.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_ENVIRONMENT_DISPLAY_STEP_HPP
#define MANGO_ENVIRONMENT_DISPLAY_STEP_HPP

#include <core/context_impl.hpp>
#include <graphics/graphics.hpp>
#include <rendering/steps/render_step.hpp>

namespace mango
{
    //! \brief A pipeline step adding image based lighting.
    class environment_display_step : public render_step
    {
      public:
        //! \brief Constructs a the \a environment_display_step.
        //! \param[in] settings The \a environment_display_settings to use.
        environment_display_step(const environment_display_settings& settings);
        ~environment_display_step();

        void attach(const shared_ptr<context_impl>& context) override;
        void execute() override;
        void on_ui_widget() override;

        //! \brief Sets the active cubemap to render.
        //! \param[in] environment_cubemap Pointer to the environment cubemap texture to set.
        //! \param[in] model_matrix A model matrix for the cubemap.
        void set_cubemap(const gfx_handle<const gfx_texture> environment_cubemap, const mat4& model_matrix = mat4(1.0f))
        {
            m_current_cubemap           = environment_cubemap;
            m_cubemap_data.model_matrix = model_matrix;
        }

      private:
        bool create_step_resources() override;

        //! \brief The settings used for rendering the environment.
        environment_display_settings m_settings;

        //! \brief The cube vertices used for rendering the skybox.
        gfx_handle<const gfx_buffer> m_cube_vertices;
        //! \brief The cube indices used for rendering the skybox.
        gfx_handle<const gfx_buffer> m_cube_indices;

        //! \brief The current cubemap to render.
        gfx_handle<const gfx_texture> m_current_cubemap;

        //! \brief The cubemap sampler.
        gfx_handle<const gfx_sampler> m_cubemap_sampler;

        //! \brief The vertex \a shader_stage for the cubemap pass.
        gfx_handle<const gfx_shader_stage> m_cubemap_pass_vertex;
        //! \brief The fragment \a shader_stage for the cubemap pass.
        gfx_handle<const gfx_shader_stage> m_cubemap_pass_fragment;
        //! \brief Graphics pipeline to render the cubemap.
        gfx_handle<const gfx_pipeline> m_cubemap_pass_pipeline;

        //! \brief The cubemap data buffer.
        gfx_handle<const gfx_buffer> m_cubemap_data_buffer;

        //! \brief Uniform buffer struct for cubemap data.
        struct cubemap_data
        {
            std140_mat4 model_matrix;  //!< Rotation and scale for the cubemap.
            std140_float render_level; //!< The miplevel to render the cubemap with.
            std140_float p0;           //!< Padding.
            std140_float p1;           //!< Padding.
            std140_float p2;           //!< Padding.
        } m_cubemap_data;              //!< Current cubemap_data.
    };
} // namespace mango

#endif // MANGO_ENVIRONMENT_DISPLAY_STEP_HPP
