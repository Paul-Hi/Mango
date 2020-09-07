//! \file      shadow_map_step.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_SHADOW_MAP_STEP_HPP
#define MANGO_SHADOW_MAP_STEP_HPP

#include <graphics/framebuffer.hpp>
#include <rendering/steps/pipeline_step.hpp>

namespace mango
{
    //! \brief A pipeline step adding shadow mapping.
    class shadow_map_step : public pipeline_step
    {
      public:
        bool create() override;
        void update(float dt) override;

        void attach() override;
        void execute(command_buffer_ptr& command_buffer) override;

        void destroy() override;

        command_buffer_ptr get_caster_queue()
        {
            return m_caster_queue;
        }

        void update_cascades(float camera_near, float camera_far, const glm::mat4& camera_view_projection, const glm::vec3& directional_direction, float shadow_map_offset);

        static const int32 num_cascade_splits = 4;

        void bind_shadow_maps_and_get_shadow_data(command_buffer_ptr& command_buffer, glm::mat4 (&out_view_projections)[num_cascade_splits], glm::vec3& cascade_splits);

      private:
        command_buffer_ptr m_caster_queue;
        framebuffer_ptr m_shadow_buffer;
        shader_program_ptr m_shadow_pass;

        int32 m_width  = 4096;
        int32 m_height = 4096; // hardcoded

        float m_shadowmap_offset = 0.0f; // TODO Paul: This can probably be done better.

        struct
        {
            float camera_near;
            float camera_far;
            float camera_vfov;
            glm::vec3 directional_direction;
            float split_depth[num_cascade_splits + 1];
            float lambda;
            glm::mat4 view_projection_matrices[num_cascade_splits];
        } m_cascade_data;
    };
} // namespace mango

#endif // MANGO_SHADOW_MAP_STEP_HPP
