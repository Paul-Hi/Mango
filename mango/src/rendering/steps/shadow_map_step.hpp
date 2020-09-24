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

        //! \brief Returns the queue to submit commands used to render shadow casters.
        //! \return The casters queue.
        command_buffer_ptr get_caster_queue()
        {
            return m_caster_queue;
        }

        //! \brief Updates the cascades for CSM.
        //! \details Calculates the camera frustum, the cascade split depths and the view projection matrices for the directional light.
        //! \param[in] camera_near The cameras near plane depth.
        //! \param[in] camera_far The cameras far plane depth.
        //! \param[in] camera_view_projection The cameras view projection matrix.
        //! \param[in] directional_direction The direction to the light.
        //! \param[in] shadow_map_offset The offset to use for the orthogonal projection. This is needed at the moment to account for certain geometry not in the camera frustum. Higher value does
        //! reduce quality.
        void update_cascades(float camera_near, float camera_far, const glm::mat4& camera_view_projection, const glm::vec3& directional_direction, float shadow_map_offset);

        //! \brief The number if cascades.
        static const int32 shadow_mapping_cascades = 4; // TODO Paul: We should move this.

        //! \brief Binds the shadow maps and returns relevant lighting pass data.
        //!\param[in] command_buffer The \a command_buffer to add the binding commands to.
        //! \param[out] out_view_projections An array to store the view projection matrices of the cascades into.
        //! \param[out] far_planes An vec4 to store the far planes of the cascade views into.
        //! \param[out] cascade_info An four dimensional vector to store the splits depths into. The w component is the shadow map resolution.
        void bind_shadow_maps_and_get_shadow_data(command_buffer_ptr& command_buffer, glm::mat4 (&out_view_projections)[shadow_mapping_cascades], glm::vec4& far_planes, glm::vec4& cascade_info);

      private:
        //! \brief Queue to store caster render commands into.
        command_buffer_ptr m_caster_queue;
        //! \brief The framebuffer storing all shadow maps.
        framebuffer_ptr m_shadow_buffer;
        //! \brief Program to execute the shadow mapping pass.
        shader_program_ptr m_shadow_pass;

        //! \brief Shadow map resolution.
        int32 m_resolution = 2048; // TODO Paul: hardcoded. Read from config.

        //! \brief The offset for the projection.
        float m_shadowmap_offset = 0.0f; // TODO Paul: This can probably be done better.

        struct
        {
            float camera_near;                                           //!< The cameras near plane depth.
            float camera_far;                                            //!< The cameras far plane depth.
            glm::vec3 directional_direction;                             //!< The direction to the light.
            float split_depth[shadow_mapping_cascades + 1];              //!< The calculated split depths.
            float lambda;                                                //!< Lambda used to calculate split depths uniform <-> log.
            glm::mat4 view_projection_matrices[shadow_mapping_cascades]; //!< The view projection matrices.
            glm::vec4 far_planes;                                        //!< The far planes of the shadow views.
        } m_cascade_data;                                                //!< Data related to shadow cascades.
    };
} // namespace mango

#endif // MANGO_SHADOW_MAP_STEP_HPP
