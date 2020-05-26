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
    //! \brief Base class for all optional pipeline steps in render pipelines.
    class ibl_step : public pipeline_step
    {
      public:
        bool create() override;
        void update(float dt) override;

        void attach() override;
        void execute(command_buffer_ptr& command_buffer) override;

        void destroy() override;

        void load_from_hdr(const texture_ptr& hdr_texture);

        inline void set_render_level(float render_level)
        {
            m_render_level = render_level;
        }

        void bind_image_based_light_maps(command_buffer_ptr& command_buffer);

      private:
        texture_ptr m_cubemap;
        texture_ptr m_irradiance_map;
        texture_ptr m_prefiltered_specular;
        texture_ptr m_brdf_integration_lut;
        vertex_array_ptr m_cube_geometry;
        shader_program_ptr m_equi_to_cubemap;
        shader_program_ptr m_build_irradiance_map;
        shader_program_ptr m_build_specular_prefiltered_map;
        shader_program_ptr m_build_integration_lut;
        shader_program_ptr m_draw_environment;
        glm::mat3 m_current_rotation_scale;
        float m_render_level;

        const uint32 m_cube_width = 1024;
        const uint32 m_cube_height = 1024;
        const uint32 m_irradiance_width = 32;
        const uint32 m_irradiance_height = 32;
        const uint32 m_prefiltered_base_width = 1024;
        const uint32 m_prefiltered_base_height = 1024;
        const uint32 m_integration_lut_width = 256;
        const uint32 m_integration_lut_height = 256;
    };
} // namespace mango

#endif // MANGO_IBL_STEP_HPP
