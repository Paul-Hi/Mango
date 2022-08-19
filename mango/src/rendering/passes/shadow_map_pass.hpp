//! \file      shadow_map_pass.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2022
//! \copyright Apache License 2.0

#ifndef MANGO_SHADOW_MAP_PASS_HPP
#define MANGO_SHADOW_MAP_PASS_HPP

#include <graphics/graphics.hpp>
#include <rendering/debug_drawer.hpp>
#include <rendering/passes/render_pass.hpp>
#include <rendering/renderer_pipeline_cache.hpp>
#include <mango/intersect.hpp>

namespace mango
{
    //! \brief A pipeline pass adding shadow mapping.
    class shadow_map_pass : public render_pass
    {
      public:
        //! \brief The maximum number of cascades.
        static const int32 max_shadow_mapping_cascades = 4; // TODO Paul: We should move this.

        //! \brief Constructs a the \a shadow_map_pass.
        //! \param[in] settings The \a shadow_settings to use.
        shadow_map_pass(const shadow_settings& settings);
        ~shadow_map_pass();

        //! \brief Additional setup function - needs to be called before attach() is called.
        //! \param[in] pipeline_cache Shared \a renderer_pipeline_cache to add and retrieve pipelines.
        //! \param[in] dbg_drawer Shared \a debug_drawer to draw debug lines.
        void setup(const shared_ptr<renderer_pipeline_cache>& pipeline_cache, const shared_ptr<debug_drawer>& dbg_drawer);

        void attach(const shared_ptr<context_impl>& context) override;
        void execute(graphics_device_context_handle& device_context) override;
        void on_ui_widget() override;

        inline render_pass_execution_info get_info() override
        {
            return m_rpei;
        }

        //! \brief Returns the shadow depth texture with multiple layers.
        //! \return A \a gfx_texture containing all shadows.
        inline gfx_handle<const gfx_texture> get_shadow_maps_texture()
        {
            return m_shadow_map;
        }

        //! \brief Returns the shadow data buffer for binding.
        //! \return A \a gfx_buffer to upload the \a shadow_data.
        inline const gfx_handle<const gfx_buffer>& get_shadow_data_buffer()
        {
            return m_shadow_data_buffer;
        }

        //! \brief Set the camera data buffer.
        //! \param[in] camera_data_buffer The camera data buffer.
        inline void set_camera_data_buffer(const gfx_handle<const gfx_buffer>& camera_data_buffer)
        {
            m_camera_data_buffer = camera_data_buffer;
        }

        //! \brief Set \a scene_impl pointer.
        //! \param[in] scene Pointer to the \a scene_impl to use for retrieving geometry data.
        inline void set_scene_pointer(scene_impl* scene)
        {
            m_scene = scene;
        }

        //! \brief Set frustum culling.
        //! \param[in] frustum_culling True if frustum culling is enabled, else false.
        inline void set_frustum_culling(bool frustum_culling)
        {
            m_frustum_culling = frustum_culling;
        }

        //! \brief Set debug bounds drawing.
        //! \param[in] debug_bounds True if drawing debug bounds is enabled, else false.
        inline void set_debug_bounds(bool debug_bounds)
        {
            m_debug_bounds = debug_bounds;
        }

        //! \brief Set wireframe drawing.
        //! \param[in] wireframe True if wireframe drawing is enabled, else false.
        inline void set_wireframe(bool wireframe)
        {
            m_wireframe = wireframe;
        }

        //! \brief Set debug view status.
        //! \param[in] debug_view_enabled True if debug view is enabled, else false.
        inline void set_debug_view_enabled(bool debug_view_enabled)
        {
            m_debug_view_enabled = debug_view_enabled;
        }

        //! \brief Set the delta time.
        //! \param[in] dt The delta time.
        inline void set_delta_time(float dt)
        {
            m_dt = dt;
        }

        //! \brief Set the camera near plane depth.
        //! \param[in] camera_near The camera near plane depth.
        inline void set_camera_near(float camera_near)
        {
            m_camera_near = camera_near;
        }

        //! \brief Set the camera far plane depth.
        //! \param[in] camera_far The camera far plane depth.
        inline void set_camera_far(float camera_far)
        {
            m_camera_far = camera_far;
        }

        //! \brief Set the camera inverse view projection matrix.
        //! \param[in] inverse_camera_view_projection The camera inverse view projection matrix.
        inline void set_camera_inverse_view_projection(const mat4& inverse_camera_view_projection)
        {
            m_inverse_camera_view_projection = inverse_camera_view_projection;
        }

        //! \brief Set a default 2d texture.
        //! \param[in] default_texture_2D The default 2d texture.
        inline void set_default_texture_2D(const gfx_handle<const gfx_texture>& default_texture_2D)
        {
            m_default_texture_2D = default_texture_2D;
        }

        //! \brief Set the camera frustum.
        //! \param[in] camera_frustum The cameras \a bounding_frustum.
        inline void set_camera_frustum(const bounding_frustum& camera_frustum)
        {
            m_camera_frustum = camera_frustum;
        }

        //! \brief Set the \a directional_lights that cast shadows.
        //! \param[in] shadow_casters The list of \a directional_lights that cast shadows.
        inline void set_shadow_casters(const std::vector<directional_light>& shadow_casters)
        {
            m_shadow_casters = shadow_casters;
        }

        //! \brief Set draws.
        //! \param[in] draws The list of \a draw_keys.
        inline void set_draws(const shared_ptr<std::vector<draw_key>>& draws)
        {
            m_draws = draws;
        }

      private:
        //! \brief Execution info of this pass.
        render_pass_execution_info m_rpei;

        bool create_pass_resources() override;

        //! \brief Creates the shadow map.
        //! \return True on success, else false.
        bool create_shadow_map();

        //! \brief Updates the cascades for CSM.
        //! \details Calculates the camera frustum, the cascade split depths and the view projection matrices for a given directional light.
        //! \param[in] directional_light_direction The direction to the light.
        void update_cascades(const vec3& directional_light_direction);

        //! \brief The \a shadow_settings for the pass.
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

        //! \brief The \a renderer_pipeline_cache to create and cache \a gfx_pipelines for the geometry.
        shared_ptr<renderer_pipeline_cache> m_pipeline_cache;

        //! \brief The shared \a debug_drawer to debug draw.
        shared_ptr<debug_drawer> m_debug_drawer;

        //! \brief Pointer to the \a scene_impl to query data for rendering.
        scene_impl* m_scene;

        //! \brief The \a bounding_frustum of the camera.
        bounding_frustum m_camera_frustum;

        //! \brief The camera data \a gfx_buffer.
        gfx_handle<const gfx_buffer> m_camera_data_buffer;
        //! \brief The shadow data \a gfx_buffer.
        gfx_handle<const gfx_buffer> m_shadow_data_buffer;

        //! \brief True if frustum culling is enabled, else false.
        bool m_frustum_culling;
        //! \brief True if drawing debug bounds is enabled, else false.
        bool m_debug_bounds;
        //! \brief True if wireframe drawing is enabled, else false.
        bool m_wireframe;
        //! \brief True if debug view  is enabled, else false.
        bool m_debug_view_enabled;

        //! \brief The delta time to use smoothing shadows.
        float m_dt;

        //! \brief The camera near plane depth.
        float m_camera_near;
        //! \brief The camera far plane depth.
        float m_camera_far;
        //! \brief The camera inverse view projection matrix.
        mat4 m_inverse_camera_view_projection;

        //! \brief List of \a directional_lights that cast shadows.
        std::vector<directional_light> m_shadow_casters;

        //! \brief The default 2d \a gfx_texture.
        gfx_handle<const gfx_texture> m_default_texture_2D;

        //! \brief The list of \a draw_keys.
        shared_ptr<std::vector<draw_key>> m_draws;

        //! \brief The offset for the projection.
        float m_shadow_map_offset = 0.0f; // TODO Paul: This can probably be done better.

        //! \brief Current shadow_data.
        shadow_data m_shadow_data;

        struct
        {
            float camera_near;                                    //!< The cameras near plane depth.
            float camera_far;                                     //!< The cameras far plane depth.
            vec3 directional_light_direction;                           //!< The direction to the light.
            float lambda;                                         //!< Lambda used to calculate split depths uniform <-> log.
            bounding_frustum frusta[max_shadow_mapping_cascades]; //!< List of current frusta.
        } m_cascade_data;                                         //!< Data required to calculate shadow cascades.
    };
} // namespace mango

#endif // MANGO_SHADOW_MAP_PASS_HPP
