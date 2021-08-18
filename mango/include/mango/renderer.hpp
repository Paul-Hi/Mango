//! \file      renderer.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_RENDERER_HPP
#define MANGO_RENDERER_HPP

#include <mango/assert.hpp>
#include <mango/types.hpp>

namespace mango
{
    //! \brief This can be used to specify the base pipeline for the \a renderer.
    //! \details A \a render_pipeline has to be specified as base pipeline in the \a renderer_configuration.
    //! The information is then used to pre build all necessary data on cpu and gpu for that specific pipeline.
    //! Some \a render_pipeline_steps may not be available on a certain \a render_pipeline.
    enum render_pipeline
    {
        deferred_pbr,
        // forward_pbr,
        default_pbr = deferred_pbr
    };

    //! \brief An additional step extending the base \a render_pipeline of the \a renderer.
    //! \details A \a render_pipeline_step has to be specified in the \a renderer_configuration.
    //! The information is then used to enable or disable certain passes in the \a render_pipeline.
    //! Some steps may not be available on certain \a renderers and \a render_pipelines.
    enum render_pipeline_step
    {
        environment_display,
        shadow_map,
        fxaa,
        // ssao,
        // voxel_gi,
        // dof,
        // bloom,
        number_of_steps
    };

    //! \brief Filter specification for shadow map samples.
    enum class shadow_filtering : uint8
    {
        hard_shadows = 0, //!< Shadows point filtered, hard edge.
        soft_shadows = 1, //!< Shadows bilinear filtered, smooth edges.
        pcss_shadows = 2, //!< Realistic shadows bilinear filtered, hard edges near the occluder, smooth edges far from it.
        count        = 3
    };

    //! \brief Presets for fxaa quality.
    enum class fxaa_quality_preset : uint8
    {
        medium_quality  = 0,
        high_quality    = 1,
        extreme_quality = 2,
        count           = 3
    };

    //! \brief The settings for the \a shadow_map_step.
    class shadow_settings
    {
      public:
        //! \brief Default constructor to set some default values.
        shadow_settings()
            : m_resolution(1024)
            , m_sample_count(8)
            , m_shadow_width(1.0f)
            , m_offset(0.0f)
            , m_cascade_count(3)
            , m_lambda(0.65f)
            , m_slope_bias(0.005f)
            , m_normal_bias(0.01f)
            , m_interpolation_range(0.5f)
            , m_filter_mode(shadow_filtering::soft_shadows)
            , m_light_size(4.0f)
        {
        }

        //! \brief Constructs a \a shadow_settings with specific values.
        //! \param[in] resolution The resolution for each cascade shadow map.
        //! \param[in] sample_count The pcf sample count.
        //! \param[in] offset The offset for the orthographic cameras so that every bit of geometry can potentially cast shadows.
        //! \param[in] cascade_count The number of cascades for the shadow mapping.
        //! \param[in] lambda The lambda for the split calculation. 0 means complete uniform, 1 complete logarithmic.
        //! \param[in] slope_bias The slope bias.
        //! \param[in] normal_bias The normal bias.
        //! \param[in] interpolation_range The interpolation range.
        //! \param[in] filter_mode The shadow filter mode.
        //! \param[in] light_size The light size for pcss.
        shadow_settings(int32 resolution, int32 sample_count, float offset, int32 cascade_count, float lambda, float slope_bias, float normal_bias, float interpolation_range,
                        shadow_filtering filter_mode, float light_size)
            : m_resolution(resolution)
            , m_sample_count(sample_count)
            , m_offset(offset)
            , m_cascade_count(cascade_count)
            , m_lambda(lambda)
            , m_slope_bias(slope_bias)
            , m_normal_bias(normal_bias)
            , m_interpolation_range(interpolation_range)
            , m_filter_mode(filter_mode)
            , m_light_size(light_size)
        {
        }

        //! \brief Sets the shadow map resolution.
        //! \param[in] resolution The resolution to set the shadow maps to.
        //! \return A reference to the modified \a shadow_settings.
        inline shadow_settings& set_resolution(int32 resolution)
        {
            m_resolution = resolution;
            return *this;
        }

        //! \brief Sets the sample count.
        //! \param[in] sample_count The number of pcf samples.
        //! \return A reference to the modified \a shadow_settings.
        inline shadow_settings& set_sample_count(int32 sample_count)
        {
            m_sample_count = sample_count;
            return *this;
        }

        //! \brief Sets the light size.
        //! \param[in] shadow_width The size of the virtual pcss light.
        //! \return A reference to the modified \a shadow_settings.
        inline shadow_settings& set_shadow_width(float shadow_width)
        {
            m_shadow_width = shadow_width;
            return *this;
        }

        //! \brief Sets the shadow map camera offset.
        //! \param[in] offset The offset for the orthographic cameras so that every bit of geometry can potentially cast shadows.
        //! \return A reference to the modified \a shadow_settings.
        inline shadow_settings& set_offset(float offset)
        {
            m_offset = offset;
            return *this;
        }

        //! \brief Sets the shadow map slope bias.
        //! \param[in] slope_bias The slope bias to prevent shadow acne.
        //! \return A reference to the modified \a shadow_settings.
        inline shadow_settings& set_slope_bias(float slope_bias)
        {
            m_slope_bias = slope_bias;
            return *this;
        }

        //! \brief Sets the shadow map normal bias.
        //! \param[in] normal_bias The normal bias to prevent shadow acne.
        //! \return A reference to the modified \a shadow_settings.
        inline shadow_settings& set_normal_bias(float normal_bias)
        {
            m_normal_bias = normal_bias;
            return *this;
        }

        //! \brief Sets the number of shadow cascades.
        //! \param[in] cascade_count The number of shadow mapping cascades.
        //! \return A reference to the modified \a shadow_settings.
        inline shadow_settings& set_cascade_count(int32 cascade_count)
        {
            m_cascade_count = cascade_count;
            return *this;
        }

        //! \brief Sets the lambda to calculate the cascade splits with.
        //! \param[in] lambda The lambda to use.
        //! \details  0 means complete uniform, 1 complete logarithmic.
        //! \return A reference to the modified \a shadow_settings.
        inline shadow_settings& set_split_lambda(float lambda)
        {
            m_lambda = lambda;
            return *this;
        }

        //! \brief Sets range, the cascades get interpolated between.
        //! \param[in] interpolation_range The interpolation range to use.
        //! \details  Should be between 0 and 10. Bigger values need more pc power.
        //! \return A reference to the modified \a shadow_settings.
        inline shadow_settings& set_cascade_interpolation_range(float interpolation_range)
        {
            m_interpolation_range = interpolation_range;
            return *this;
        }

        //! \brief Sets the \a shadow_filtering mode.
        //! \param[in] filter_mode The filter mode to use.
        //! \return A reference to the modified \a shadow_settings.
        inline shadow_settings& set_filter_mode(shadow_filtering filter_mode)
        {
            m_filter_mode = filter_mode;
            return *this;
        }

        //! \brief Sets the light size for pcss.
        //! \param[in] light_size The light size to use.
        //! \return A reference to the modified \a shadow_settings.
        inline shadow_settings& set_light_size(float light_size)
        {
            m_light_size = light_size;
            return *this;
        }

        //! \brief Retrieves and returns the shadow map resolution.
        //! \return The  shadow map resolution.
        inline int32 get_resolution() const
        {
            return m_resolution;
        }

        //! \brief Retrieves and returns the sample count.
        //! \return The  sample count.
        inline int32 get_sample_count() const
        {
            return m_sample_count;
        }

        //! \brief Retrieves and returns the shadow width.
        //! \return The shadow width.
        inline float get_shadow_width() const
        {
            return m_shadow_width;
        }

        //! \brief Retrieves and returns the shadow map orthographic camera offset.
        //! \return The  shadow map orthographic camera offset.
        inline float get_offset() const
        {
            return m_offset;
        }

        //! \brief Retrieves and returns the shadow map slope bias.
        //! \return The shadow map slope bias.
        inline float get_slope_bias() const
        {
            return m_slope_bias;
        }

        //! \brief Retrieves and returns the shadow map normal bias.
        //! \return The shadow map normal bias.
        inline float get_normal_bias() const
        {
            return m_normal_bias;
        }

        //! \brief Retrieves and returns the number of cascades.
        //! \return The  number of cascades.
        inline int32 get_cascade_count() const
        {
            return m_cascade_count;
        }

        //! \brief Retrieves and returns the lambda used for split calculation.
        //! \return The  lambda used for split calculation.
        inline float get_split_lambda() const
        {
            return m_lambda;
        }

        //! \brief Retrieves and returns the interpolation range for cascade interpolation.
        //! \return The  interpolation range for cascade interpolation.
        inline float get_cascade_interpolation_range() const
        {
            return m_interpolation_range;
        }

        //! \brief Retrieves and returns the \a shadow_filtering mode.
        //! \return The \a shadow_filtering mode.
        inline shadow_filtering get_filter_mode() const
        {
            return m_filter_mode;
        }
        //! \brief Retrieves and returns the light size for pcss.
        //! \return The light size.
        inline float get_light_size() const
        {
            return m_light_size;
        }

      private:
        //! \brief The configured shadow map resolution.
        int32 m_resolution;
        //! \brief The configured sample count.
        int32 m_sample_count;
        //! \brief The configured shadow width.
        float m_shadow_width;
        //! \brief The configured offset for the shadow map orthographic cameras.
        float m_offset;
        //! \brief The configured number of cascades.
        int32 m_cascade_count;
        //! \brief The configured splitting lambda
        float m_lambda;
        //! \brief The  m_slope bias.
        float m_slope_bias;
        //! \brief The  m_normal bias.
        float m_normal_bias;
        //! \brief The m_interpolation_range.
        float m_interpolation_range;
        //! \brief The filter mode. (Hard, soft or pcss shadows).
        shadow_filtering m_filter_mode;
        //! \brief The configured size of the light for pcss.
        float m_light_size;
    };

    //! \brief The settings for the \a environment_display_step.
    class environment_display_settings
    {
      public:
        //! \brief Default constructor to set some default values.
        environment_display_settings()
            : m_render_level(0.0f)
        {
        }

        //! \brief Constructs a \a environment_display_settings with specific values.
        //! \param[in] render_level The  render_level to render the cubemap with.
        environment_display_settings(float render_level)
            : m_render_level(render_level)
        {
        }

        //! \brief Sets the render level to render the cubemap with.
        //! \param[in] render_level The render_level to render the cubemap with.
        //! \return A reference to the modified \a environment_display_settings.
        inline environment_display_settings& set_render_level(float render_level)
        {
            m_render_level = render_level;
            return *this;
        }

        //! \brief Retrieves and returns the render level to render the cubemap with.
        //! \return The  render_level to render the cubemap with.
        inline float get_render_level() const
        {
            return m_render_level;
        }

      private:
        //! \brief The render level.
        float m_render_level;
    };

    //! \brief The settings for the \a fxaa_step.
    class fxaa_settings
    {
      public:
        //! \brief Default constructor to set some default values.
        fxaa_settings()
            : m_quality(fxaa_quality_preset::medium_quality)
            , m_subpixel_filter(0.0f)
        {
        }

        //! \brief Constructs a \a fxaa_settings with specific values.
        //! \param[in] quality The  \a fxaa_quality_preset to render the fxaa with.
        //! \param[in] subpixel_filter The  subpixel filter value to render the fxaa with.
        fxaa_settings(fxaa_quality_preset quality, float subpixel_filter)
            : m_quality(quality)
            , m_subpixel_filter(subpixel_filter)
        {
        }

        //! \brief Sets the \a fxaa_quality_preset to render the fxaa with.
        //! \param[in] quality The \a fxaa_quality_preset to render the fxaa with.
        //! \return A reference to the modified \a fxaa_settings.
        inline fxaa_settings& set_quality_preset(fxaa_quality_preset quality)
        {
            m_quality = quality;
            return *this;
        }

        //! \brief Retrieves and returns the \a fxaa_quality_preset to render the fxaa with.
        //! \return The  \a fxaa_quality_preset to render the fxaa with.
        inline fxaa_quality_preset get_quality_preset() const
        {
            return m_quality;
        }

        //! \brief Sets the  subpixel filter value to render the fxaa with.
        //! \param[in] subpixel_filter The  subpixel filter value to render the fxaa with.
        //! \return A reference to the modified \a fxaa_settings.
        inline fxaa_settings& set_subpixel_filter(float subpixel_filter)
        {
            m_subpixel_filter = subpixel_filter;
            return *this;
        }

        //! \brief Retrieves and returns the  subpixel filter value to render the fxaa with.
        //! \return The  subpixel filter value to render the fxaa with.
        inline float get_subpixel_filter() const
        {
            return m_subpixel_filter;
        }

      private:
        //! \brief The \a fxaa_quality_preset.
        fxaa_quality_preset m_quality;
        //!\brief The filter value for subpixels.
        float m_subpixel_filter;
    };

    //! \brief The configuration for the \a renderer.
    //! \details Has to be used to configure the \a renderer in the \a application create() method.
    class renderer_configuration
    {
      public:
        //! \brief Default constructor to set some default values before the user application configures the \a renderer.
        renderer_configuration()
            : m_base_pipeline(render_pipeline::default_pbr)
            , m_vsync(true)
            , m_wireframe(false)
            , m_frustum_culling(true)
            , m_debug_bounds(false)
        {
            std::memset(m_render_steps, 0, render_pipeline_step::number_of_steps * sizeof(bool));
        }

        //! \brief Constructs a \a renderer_configuration with specific values.
        //! \param[in] base_render_pipeline The  base \a render_pipeline of the \a renderer to configure.
        //! \param[in] vsync The setting for the \a renderer. Spezifies if vertical synchronization should be enabled or disabled.
        //! \param[in] wireframe The setting for the \a renderer. Spezifies if wireframe should be drawn or not.
        //! \param[in] frustum_culling The setting for the \a renderer. Spezifies if frustum culling should be enabled or disabled.
        //! \param[in] draw_debug_bounds The setting for the \a renderer. Spezifies if debug bounds should be drawn or not.
        renderer_configuration(render_pipeline base_render_pipeline, bool vsync, bool wireframe, bool frustum_culling, bool draw_debug_bounds)
            : m_base_pipeline(base_render_pipeline)
            , m_vsync(vsync)
            , m_wireframe(wireframe)
            , m_frustum_culling(frustum_culling)
            , m_debug_bounds(draw_debug_bounds)
        {
            std::memset(m_render_steps, 0, render_pipeline_step::number_of_steps * sizeof(bool));
        }

        //! \brief Sets or changes the base \a render_pipeline of the \a renderer in the \a renderer_configuration.
        //! \param[in] base_render_pipeline The base \a render_pipeline of the \a renderer to configure.
        //! \return A reference to the modified \a renderer_configuration.
        inline renderer_configuration& set_base_render_pipeline(render_pipeline base_render_pipeline)
        {
            m_base_pipeline = base_render_pipeline;
            return *this;
        }

        //! \brief Enables shadow map rendering in the \a renderer_configuration.
        //! \details This is then used to add the \a render_pipeline_step to the base \a render_pipeline of the \a renderer.
        //! \param[in] settings The \a shadow_settings to use for the \a shadow_map_step.
        //! \return A reference to the modified \a renderer_configuration.
        inline renderer_configuration& enable_shadow_maps(const shadow_settings& settings)
        {
            m_render_steps[render_pipeline_step::shadow_map] = true;
            m_shadow_settings                                = settings;
            return *this;
        }

        //! \brief Enables cubemap rendering in for the environment in the \a renderer_configuration.
        //! \details This is then used to add the \a render_pipeline_step to the base \a render_pipeline of the \a renderer.
        //! \param[in] settings The \a environment_display_settings to use for the \a environment_display_step.
        //! \return A reference to the modified \a renderer_configuration.
        inline renderer_configuration& display_environment(const environment_display_settings& settings)
        {
            m_render_steps[render_pipeline_step::environment_display] = true;
            m_environment_display_settings                            = settings;
            return *this;
        }

        //! \brief Enables fxaa in the \a renderer_configuration.
        //! \details This is then used to add the \a render_pipeline_step to the base \a render_pipeline of the \a renderer.
        //! \param[in] settings The \a fxaa_settings to use for the \a fxaa_step.
        //! \return A reference to the modified \a renderer_configuration.
        inline renderer_configuration& enable_fxaa(const fxaa_settings& settings)
        {
            m_render_steps[render_pipeline_step::fxaa] = true;
            m_fxaa_settings                            = settings;
            return *this;
        }

        //! \brief Sets or changes the setting for vertical synchronization in the \a renderer_configuration.
        //! \param[in] vsync The setting for the \a renderer. Spezifies if vertical synchronization should be enabled or disabled.
        //! \return A reference to the modified \a renderer_configuration.
        inline renderer_configuration& set_vsync(bool vsync)
        {
            m_vsync = vsync;
            return *this;
        }

        //! \brief Sets or changes the setting for wireframe drawing in the \a renderer_configuration.
        //! \param[in] wireframe The setting for the \a renderer. Spezifies if wireframe should be drawn or not.
        //! \return A reference to the modified \a renderer_configuration.
        inline renderer_configuration& draw_wireframe(bool wireframe)
        {
            m_wireframe = wireframe;
            return *this;
        }

        //! \brief Sets or changes the setting for frustum culling in the \a renderer_configuration.
        //! \param[in] cull The setting for the \a renderer. Spezifies if frustum culling should be enabled or disabled.
        //! \return A reference to the modified \a renderer_configuration.
        inline renderer_configuration& set_frustum_culling(bool cull)
        {
            m_frustum_culling = cull;
            return *this;
        }

        //! \brief Sets or changes the setting for drawing debug bounds in the \a renderer_configuration.
        //! \param[in] draw The setting for the \a renderer. Spezifies if debug bounds should be drawn or not.
        //! \return A reference to the modified \a renderer_configuration.
        inline renderer_configuration& draw_debug_bounds(bool draw)
        {
            m_debug_bounds = draw;
            return *this;
        }

        //! \brief Retrieves and returns the setting for vertical synchronization of the \a renderer_configuration.
        //! \return The current vertical synchronization setting.
        inline bool is_vsync_enabled() const
        {
            return m_vsync;
        }

        //! \brief Retrieves and returns the setting for wireframe drawing of the \a renderer_configuration.
        //! \return The current wireframe drawing setting.
        inline bool should_draw_wireframe() const
        {
            return m_wireframe;
        }

        //! \brief Retrieves and returns the setting for frustum culling of the \a renderer_configuration.
        //! \return The current frustum culling setting.
        inline bool is_frustum_culling_enabled() const
        {
            return m_frustum_culling;
        }

        //! \brief Retrieves and returns the setting for drawing debug bounds of the \a renderer_configuration.
        //! \return The current setting for drawing debug bounds.
        inline bool should_draw_debug_bounds() const
        {
            return m_debug_bounds;
        }

        //! \brief Retrieves and returns the base \a render_pipeline set in the \a renderer_configuration.
        //! \return The current  base \a render_pipeline of the \a renderer.
        inline render_pipeline get_base_render_pipeline() const
        {
            return m_base_pipeline;
        }

        //! \brief Retrieves and returns the array of possible \a render_steps set in the \a renderer_configuration.
        //! \details The returned array has the size \a render_step::number_of_step_types.
        //! On a specific position in the array there is the value 'true' if the \a render_step is enabled, else 'false'.
        //! \return The current  array of possible \a render_steps of the \a renderer.
        inline const bool* get_render_steps() const
        {
            return m_render_steps;
        }

        //! \brief Retrieves and returns the base \a shadow_settings set in the \a renderer_configuration.
        //! \return The shadow_settings, when the \a shadow_map_step is enabled.
        inline const shadow_settings& get_shadow_settings() const
        {
            return m_shadow_settings;
        }

        //! \brief Retrieves and returns the base \a environment_display_settings set in the \a renderer_configuration.
        //! \return The environment_display_settings, when the environment display is enabled.
        inline const environment_display_settings& get_environment_display_settings() const
        {
            return m_environment_display_settings;
        }

        //! \brief Retrieves and returns the base \a fxaa_settings set in the \a renderer_configuration.
        //! \return The fxaa_settings, when the \a fxaa_step is enabled.
        inline const fxaa_settings& get_fxaa_settings() const
        {
            return m_fxaa_settings;
        }

      private:
        //! \brief The base \a render_pipeline of the \a renderer to configure.
        render_pipeline m_base_pipeline;
        //! \brief The setting of the \a renderer_configuration to enable or disable vertical synchronization.
        bool m_vsync;

        //! \brief TThe setting of the \a renderer_configuration to enable or disable wireframe.
        bool m_wireframe;

        //! \brief The setting of the \a renderer_configuration to enable or disable drawing of debug bounds.
        bool m_debug_bounds;

        //! \brief The setting of the \a renderer_configuration to enable or disable culling primitives against camera and shadow frusta.
        bool m_frustum_culling;

        //! \brief The additional \a render_pipeline_steps of the \a renderer_configuration to enable or disable vertical synchronization.
        bool m_render_steps[render_pipeline_step::number_of_steps];

        //! \brief The \a shadow_settings of the \a renderer to configure.
        shadow_settings m_shadow_settings;
        //! \brief The \a environment_display_settings of the \a renderer to configure.
        environment_display_settings m_environment_display_settings;
        //! \brief The \a fxaa_settings of the \a renderer to configure.
        fxaa_settings m_fxaa_settings;
    };

    //! \brief Information used and filled by the \a renderer.
    struct renderer_info
    {
        //! \brief The graphics API version used.
        string api_version;

        struct
        {
            int32 x;      //!< The x of the current render canvas.
            int32 y;      //!< The y of the current render canvas.
            int32 width;  //!< The width of the current render canvas.
            int32 height; //!< The height of the current render canvas.
        } canvas;         //!< Draw canvas information.

        struct
        {
            int32 draw_calls; //!< The number of draw calls.
            int32 meshes;     //!< The number of meshes.
            int32 primitives; //!< The number of primitives.
            int32 vertices;   //!< The number of vertices.
            int32 triangles;  //!< The number of triangles (approx.).
            int32 materials;  //!< The number of materials.
        } last_frame;         //!< Measured stats from the last rendered frame.
    };

    //! \brief A class for rendering stuff.
    class renderer
    {
      public:
        //! \brief Checks if vertical synchronization is enabled.
        //! \return True if vertical synchronization is enabled, else false.
        virtual bool is_vsync_enabled() const = 0;

        //! \brief Retrieves the current \a renderer_info.
        //! \return A constant reference to the current \a renderer_info.
        virtual const renderer_info& get_renderer_info() const = 0;
    };

    //! \brief A unique pointer holding a \a renderer.
    using renderer_ptr = std::unique_ptr<renderer>;

    //! \brief A constant pointer pointing to a \a renderer.
    using renderer_handle = const renderer*;

} // namespace mango

#endif // MANGO_RENDERER_HPP
