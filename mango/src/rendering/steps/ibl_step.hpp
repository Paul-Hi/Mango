//! \file      ibl_step.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_IBL_STEP_HPP
#define MANGO_IBL_STEP_HPP

#include <rendering/steps/pipeline_step.hpp>

namespace mango
{
    //! \brief A pipeline step adding image based lighting.
    class ibl_step : public pipeline_step
    {
      public:
        bool create() override;
        void update(float dt) override;

        void attach() override;
        void configure(const ibl_step_configuration& config);
        void execute(command_buffer_ptr& command_buffer) override;

        void destroy() override;

        //! \brief Loads and creates an image based light step from a hdr texture.
        //! \details Creates irradiance map and prefiltered specular map.
        //! \param[in] hdr_texture The texture with the hdr image data.
        void load_from_hdr(const texture_ptr& hdr_texture);

        //! \brief Sets the intensity of the environment light.
        //! \param[in] intensity The intensity in cd/m^2.
        inline void set_intensity(float intensity)
        {
            m_intensity = intensity;
        }

        //! \brief Submits the texture binding \a commands and additional uniform calls to the given \a command_buffer.
        //! \param[in] command_buffer The \a command_buffer to submit the \a commands to.
        void bind_image_based_light_maps(command_buffer_ptr& command_buffer);

        //! \brief Sets the view projection matrix used for rendering the environment as a skybox.
        //! \details This matrix should be given without the view translation.
        //! \param[in] view_projection The matrix to use.
        inline void set_view_projection_matrix(const glm::mat4& view_projection)
        {
            m_view_projection = view_projection;
        }

        void on_ui_widget() override;

      private:
        //! \brief Cubemap texture.
        texture_ptr m_cubemap;
        //! \brief Irradiance map.
        texture_ptr m_irradiance_map;
        //! \brief Prefiltered specular map.
        texture_ptr m_prefiltered_specular;
        //! \brief The integration look up for specular and diffuse brdf.
        texture_ptr m_brdf_integration_lut;
        //! \brief The cube geometry used for rendering the skybox.
        vertex_array_ptr m_cube_geometry;

        //! \brief Compute shader program converting a equirectangular hdr to a cubemap.
        shader_program_ptr m_equi_to_cubemap;
        //! \brief Compute shader program building a irradiance map from a cubemap.
        shader_program_ptr m_build_irradiance_map;
        //! \brief Compute shader program building the prefiltered specular map from a cubemap.
        shader_program_ptr m_build_specular_prefiltered_map;
        //! \brief Compute shader program building the look up brdf integration texture.
        shader_program_ptr m_build_integration_lut;
        //! \brief Shader program to render a cubemap.
        shader_program_ptr m_draw_environment;
        //! \brief Rotation and scale for the cubemap (currently unused).
        glm::mat3 m_current_rotation_scale;
        //! \brief The view projection matrix to render with.
        glm::mat4 m_view_projection;
        //! \brief The miplevel to render the cubemap with.
        float m_render_level;
        //! \brief The intensity of the environment light in cd/m^2.
        float m_intensity;

        //! \brief The width of one cubemap face.
        const int32 m_cube_width = 1024;
        //! \brief The height of one cubemap face.
        const int32 m_cube_height = 1024;
        //! \brief The width of one irradiance map face.
        const int32 m_irradiance_width = 32;
        //! \brief The height of one irradiance map face.
        const int32 m_irradiance_height = 32;
        //! \brief The width of one prefiltered specular map face.
        const int32 m_prefiltered_base_width = 1024;
        //! \brief The height of one prefiltered specular map face.
        const int32 m_prefiltered_base_height = 1024;
        //! \brief The width of the brdf integration look up texture.
        const int32 m_integration_lut_width = 256;
        //! \brief The height of the brdf integration look up texture.
        const int32 m_integration_lut_height = 256;
    };
} // namespace mango

#endif // MANGO_IBL_STEP_HPP
