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

        //! \brief Configures the \a shadow_map_step.
        //! \param[in] configuration The \a shadow_step_configuration to use.
        void configure(const shadow_step_configuration& configuration);
        void execute(gpu_buffer_ptr frame_uniform_buffer) override;

        void destroy() override;

        //! \brief Returns the shadow framebuffer with the shadow map attached as depth attachment with multiple layers.
        //! \return A shared_ptr to the shadow \a framebuffer.
        inline framebuffer_ptr get_shadow_buffer()
        {
            return m_shadow_buffer;
        }

        //! \brief Returns a shared_ptr to the \a command_buffer of the shadow step.
        //! \details The returned \a command_buffer can be used to render geometry that should cast shadows. It gets executed by the rendering system.
        //! \return A shared_ptr to the shadow step \a command_buffer.
        inline command_buffer_ptr<max_key> get_shadow_commands()
        {
            return m_shadow_command_buffer;
        }

        //! \brief Updates the cascades for CSM.
        //! \details Calculates the camera frustum, the cascade split depths and the view projection matrices for the directional light.
        //! \param[in] dt Time since last call.
        //! \param[in] camera_near The cameras near plane depth.
        //! \param[in] camera_far The cameras far plane depth.
        //! \param[in] camera_view_projection The cameras view projection matrix.
        //! \param[in] directional_direction The direction to the light.
        //! reduce quality.
        void update_cascades(float dt, float camera_near, float camera_far, const glm::mat4& camera_view_projection, const glm::vec3& directional_direction);

        //! \brief The maximum number of cascades.
        static const int32 max_shadow_mapping_cascades = 4; // TODO Paul: We should move this.

        void on_ui_widget() override;

      private:
        bool setup_shader_programs() override;
        bool setup_buffers() override;

        //! \brief The \a command_buffer storing all shadow step related commands.
        command_buffer_ptr<max_key> m_shadow_command_buffer;
        //! \brief The framebuffer storing all shadow maps.
        framebuffer_ptr m_shadow_buffer;
        //! \brief Program to execute the shadow mapping pass.
        shader_program_ptr m_shadow_pass;

        //! \brief The offset for the projection.
        float m_shadow_map_offset = 0.0f; // TODO Paul: This can probably be done better.

        //! \brief Dirty bit for cascade count update.
        bool m_dirty_cascades;

        //! \brief Uniform buffer struct for shadow data.
        struct shadow_data
        {
            std140_mat4 view_projection_matrices[max_shadow_mapping_cascades]; //!< The view projection matrices.
            // TODO Paul: We really should not use arrays with that padding -.-
            std140_float_array split_depth[max_shadow_mapping_cascades + 1]; //!< The calculated split depths. Times two because of better alignment
            std140_vec4 far_planes;                                          //!< The far planes of the shadow views.
            std140_int resolution                    = 2048;                 //!< The shadow map resolution.
            std140_int cascade_count                 = 3;                    //!< The number of cascades.
            std140_float cascade_interpolation_range = 0.5f;   //!< The range to use for interpolating the cascades. Larger values mean smoother transition, but less quality and performance impact.
            std140_int sample_count                  = 16;     //!< The sample count. Larger values can look more natural, but may cause artefacts and performance drops.
            std140_float slope_bias                  = 0.005f; //!< The slope bias.
            std140_float normal_bias                 = 0.01f;  //!< The bias along the normal.
            std140_int filter_mode                   = 0;      //!< shadow_filtering parameter.
            std140_float light_size                  = 4.0f;   //!< Size of the light for PCSS.
        } m_shadow_data;                                       //!< Current shadow_data.

        struct
        {
            float camera_near;               //!< The cameras near plane depth.
            float camera_far;                //!< The cameras far plane depth.
            glm::vec3 directional_direction; //!< The direction to the light.
            float lambda;                    //!< Lambda used to calculate split depths uniform <-> log.

        } m_cascade_data; //!< Data required to calculate shadow cascades.
    };
} // namespace mango

#endif // MANGO_SHADOW_MAP_STEP_HPP
