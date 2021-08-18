//! \file      shadow_map_step.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_SHADOW_MAP_STEP_HPP
#define MANGO_SHADOW_MAP_STEP_HPP

#include <core/context_impl.hpp>
#include <graphics/graphics.hpp>
#include <rendering/steps/render_step.hpp>
#include <util/intersect.hpp>

namespace mango
{
    //! \brief A pipeline step adding shadow mapping.
    class shadow_map_step : public render_step
    {
      public:
        //! \brief The maximum number of cascades.
        static const int32 max_shadow_mapping_cascades = 4; // TODO Paul: We should move this.
        //! \brief Uniform buffer struct for shadow data.
        struct shadow_data
        {
            std140_mat4 view_projection_matrices[max_shadow_mapping_cascades]; //!< The view projection matrices.
            // TODO Paul: We really should not use arrays with that padding -.-
            std140_float_array split_depth[max_shadow_mapping_cascades]; //!< The calculated split depths.
            std140_vec4 far_planes;                                      //!< The far planes of the shadow views.
            std140_int resolution                    = 2048;             //!< The shadow map resolution.
            std140_int cascade_count                 = 3;                //!< The number of cascades.
            std140_float cascade_interpolation_range = 0.5f;   //!< The range to use for interpolating the cascades. Larger values mean smoother transition, but less quality and performance impact.
            std140_int sample_count                  = 16;     //!< The sample count. Larger values can look more natural, but may cause artefacts and performance drops.
            std140_float slope_bias                  = 0.005f; //!< The slope bias.
            std140_float normal_bias                 = 0.01f;  //!< The bias along the normal.
            std140_int filter_mode                   = 0;      //!< shadow_filtering parameter.
            std140_float shadow_width                = 1.0f;   //!< Width of the PCF shadow.
            std140_float shadow_light_size           = 4.0f;   //!< Size of the light used for PCSS shadow.
            std140_int cascade                       = 0;      //!< The currently rendered cascade. Only used in the rendering process, not required in th lookup while lighting is calculated.
            std140_float pad1;                                 //!< Padding.
            std140_float pad2;                                 //!< Padding.
        };

        //! \brief Constructs a the \a shadow_map_step.
        //! \param[in] settings The \a shadow_settings to use.
        shadow_map_step(const shadow_settings& settings);
        ~shadow_map_step();

        void attach(const shared_ptr<context_impl>& context) override;
        void execute() override;
        void on_ui_widget() override;

        //! \brief Returns the shadow depth texture with multiple layers.
        //! \return A \a gfx_texture containing all shadows.
        inline graphics_pipeline_create_info get_shadow_pass_pipeline_base()
        {
            return m_shadow_pass_pipeline_create_info_base;
        }

        //! \brief Returns the shadow data buffer for binding.
        //! \return A \a gfx_buffer to upload the \a shadow_data.
        inline const gfx_handle<const gfx_buffer>& get_shadow_data_buffer()
        {
            return m_shadow_data_buffer;
        }

        //! \brief Returns the current shadow data.
        //! \return The current \a shadow_data.
        inline shadow_data& get_shadow_data()
        {
            return m_shadow_data;
        }

        //! \brief Returns the shadow depth texture with multiple layers.
        //! \return A \a gfx_texture containing all shadows.
        inline gfx_handle<const gfx_texture> get_shadow_maps_texture()
        {
            return m_shadow_map;
        }

        //! \brief Returns the shadow depth sampler to bind as sampler2DArrayShadow.
        //! \return A \a gfx_sampler.
        inline gfx_handle<const gfx_sampler> get_shadow_maps_shadow_sampler()
        {
            return m_shadow_map_shadow_sampler;
        }

        //! \brief Returns the shadow depth sampler to bind as sampler2DArray.
        //! \return A \a gfx_sampler.
        inline gfx_handle<const gfx_sampler> get_shadow_maps_sampler()
        {
            return m_shadow_map_sampler;
        }

        //! \brief Returns resolution of the quadratic shadow map.
        //! \return The resolution of the quadratic shadow map.
        inline int32 resolution()
        {
            return m_shadow_data.resolution;
        }

        //! \brief Returns a \a bounding_frustum of a shadow map cascade.
        //! \param[in] cascade_idx The index of the cascade to query the \a bounding_frustum for.
        //! \return The \a bounding_frustum of the shadow map cascade.
        inline const bounding_frustum& get_cascade_frustum(int32 cascade_idx)
        {
            return m_cascade_data.frusta[cascade_idx];
        }

        //! \brief Updates the cascades for CSM.
        //! \details Calculates the camera frustum, the cascade split depths and the view projection matrices for the directional light.
        //! \param[in] dt Time since last call.
        //! \param[in] camera_near The cameras near plane depth.
        //! \param[in] camera_far The cameras far plane depth.
        //! \param[in] camera_view_projection The cameras view projection matrix.
        //! \param[in] directional_direction The direction to the light.
        void update_cascades(float dt, float camera_near, float camera_far, const mat4& camera_view_projection, const vec3& directional_direction);

      private:
        bool create_step_resources() override;

        //! \brief Creates the shadow map.
        //! \return True on success, else false.
        bool create_shadow_map();

        //! \brief The \a shadow_settings for the step.
        shadow_settings m_settings;

        //! \brief The \a gfx_texture storing all shadow maps.
        gfx_handle<const gfx_texture> m_shadow_map;
        //! \brief The \a gfx_sampler for shadow sampling with samplerShadow.
        gfx_handle<const gfx_sampler> m_shadow_map_shadow_sampler;
        //! \brief The \a gfx_sampler for shadow sampling.
        gfx_handle<const gfx_sampler> m_shadow_map_sampler;
        //! \brief The vertex \a shader_stage for the shadow map pass.
        gfx_handle<const gfx_shader_stage> m_shadow_pass_vertex;
        //! \brief The geometry \a shader_stage for the shadow map pass.
        gfx_handle<const gfx_shader_stage> m_shadow_pass_geometry;
        //! \brief The fragment \a shader_stage for the shadow map pass.
        gfx_handle<const gfx_shader_stage> m_shadow_pass_fragment;

        //! \brief The \a graphics_pipeline_create_info to use as a base for \a gfx_pipelines for shadow map rendering.
        graphics_pipeline_create_info m_shadow_pass_pipeline_create_info_base;

        //! \brief The offset for the projection.
        float m_shadow_map_offset = 0.0f; // TODO Paul: This can probably be done better.

        //! \brief The shadow data buffer.
        gfx_handle<const gfx_buffer> m_shadow_data_buffer;

        //! \brief Current shadow_data.
        shadow_data m_shadow_data;

        struct
        {
            float camera_near;                                    //!< The cameras near plane depth.
            float camera_far;                                     //!< The cameras far plane depth.
            vec3 directional_direction;                           //!< The direction to the light.
            float lambda;                                         //!< Lambda used to calculate split depths uniform <-> log.
            bounding_frustum frusta[max_shadow_mapping_cascades]; //!< List of current frusta.
        } m_cascade_data;                                         //!< Data required to calculate shadow cascades.
    };
} // namespace mango

#endif // MANGO_SHADOW_MAP_STEP_HPP
