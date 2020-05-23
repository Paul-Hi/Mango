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

      private:
        texture_ptr m_cubemap;
        texture_ptr m_irradiance_map;
        texture_ptr m_brdf_integration_lut;
        vertex_array_ptr m_cube_geometry;
        shader_program_ptr m_equi_to_cubemap;
        shader_program_ptr m_prefilter_ibl;
        shader_program_ptr m_draw_environment;
        glm::mat3 m_current_rotation_scale;
    };
} // namespace mango

#endif // MANGO_IBL_STEP_HPP
