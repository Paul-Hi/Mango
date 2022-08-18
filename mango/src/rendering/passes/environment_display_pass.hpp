//! \file      environment_display_pass.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2022
//! \copyright Apache License 2.0

#ifndef MANGO_ENVIRONMENT_DISPLAY_PASS_HPP
#define MANGO_ENVIRONMENT_DISPLAY_PASS_HPP

#include <graphics/graphics.hpp>
#include <rendering/passes/render_pass.hpp>

namespace mango
{
    //! \brief A pipeline pass adding image based lighting.
    class environment_display_pass : public render_pass
    {
      public:
        //! \brief Constructs a the \a environment_display_pass.
        //! \param[in] settings The \a environment_display_settings to use.
        environment_display_pass(const environment_display_settings& settings);
        ~environment_display_pass();

        void attach(const shared_ptr<context_impl>& context) override;
        void execute(graphics_device_context_handle& device_context) override;
        void on_ui_widget() override;

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


        //! \brief Sets the active cubemap to render.
        //! \param[in] environment_cubemap Pointer to the environment cubemap texture to set.
        //! \param[in] model_matrix A model matrix for the cubemap.
        void set_cubemap(const gfx_handle<const gfx_texture> environment_cubemap, const mat4& model_matrix = mat4::Identity())
        {
            m_current_cubemap           = environment_cubemap;
            m_cubemap_data.model_matrix = model_matrix;
        }

      private:
        //! \brief Execution info of this pass.
        static const render_pass_execution_info s_rpei;

        bool create_pass_resources() override;

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

        gfx_handle<const gfx_buffer> m_camera_data_buffer;
        gfx_handle<const gfx_buffer> m_renderer_data_buffer;

        //! \brief Current cubemap data.
        cubemap_data m_cubemap_data;
    };
} // namespace mango

#endif // MANGO_ENVIRONMENT_DISPLAY_PASS_HPP
