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

        //! \brief Configures the \a ibl_step.
        //! \param[in] configuration The \a ibl_step_configuration to use.
        void configure(const ibl_step_configuration& configuration);
        void execute(gpu_buffer_ptr frame_uniform_buffer) override;

        void destroy() override;

        //! \brief Loads and creates an image based light step from a hdr texture.
        //! \details Creates irradiance map and prefiltered specular map.
        //! \param[in] hdr_texture The texture with the hdr image data.
        void load_from_hdr(const texture_ptr& hdr_texture);

        //! \brief Returns a shared_ptr to the \a command_buffer of the ibl step.
        //! \details The returned \a command_buffer gets executed by the rendering system.
        //! \return A shared_ptr to the ibl step \a command_buffer.
        inline command_buffer_ptr<min_key> get_ibl_commands()
        {
            return m_ibl_command_buffer;
        }

        //! \brief Returns the preconvoluted irradiance map of the currently loaded hdr image.
        //! \return A shared_ptr to the preconvoluted irradiance \a texture.
        texture_ptr get_irradiance_map();
        //! \brief Returns the prefiltered specular map of the currently loaded hdr image.
        //! \return A shared_ptr to the prefiltered specular \a texture.
        texture_ptr get_prefiltered_specular();
        //! \brief Returns the precalculated brdf lookup table of the currently loaded hdr image.
        //! \return A shared_ptr to the precalculated brdf lookup table in the form of a \a texture.
        texture_ptr get_brdf_lookup();

        void on_ui_widget() override;

      private:
        //! \brief The \a command_buffer storing all ibl step related commands.
        command_buffer_ptr<min_key> m_ibl_command_buffer;
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

        //! \brief Uniform buffer struct for ibl data.
        struct ibl_data
        {
            std140_mat3 current_rotation_scale; //!< Rotation and scale for the cubemap (currently unused).
            std140_float render_level;          //!< The miplevel to render the cubemap with.
            std140_float p0;                    //!< Padding.
            std140_float p1;                    //!< Padding.
            std140_float p2;                    //!< Padding.
        } m_ibl_data; //!< Current ibl_data.
    };
} // namespace mango

#endif // MANGO_IBL_STEP_HPP
