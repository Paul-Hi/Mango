//! \file      render_system.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_RENDER_SYSTEM_HPP
#define MANGO_RENDER_SYSTEM_HPP

#include <mango/assert.hpp>
#include <mango/system.hpp>

namespace mango
{
    //! \brief This can be used to specify the base pipeline for the \a render_system.
    //! \details A \a render_pipeline has to be specified as base pipeline in the \a render_configuration.
    //! The information is then used to pre build all necessary data on cpu and gpu for that specific pipeline.
    //! Some \a render_steps may not be available on a certain \a render_pipeline.
    enum render_pipeline
    {
        deferred_pbr,
        // forward_pbr,
        default_pbr = deferred_pbr
    };

    //! \brief An additional step extending the base \a render_pipeline of the \a render_system.
    //! \details A \a render_step has to be specified in the \a render_configuration.
    //! The information is then used to enable or disable certain passes in the \a render_pipeline.
    //! Some steps may not be available on certain \a render_systems and \a render_pipelines.
    enum render_step
    {
        cubemap,
        shadow_map,
        fxaa,
        // ssao,
        // voxel_gi,
        // dof,
        // bloom,
        number_of_step_types
    };

    //! \brief The configuration for the \a render_system.
    //! \details Should be used to configure the \a render_system in the \a application create() method.
    class render_configuration
    {
      public:
        //! \brief Default constructor to set some default values before the user application configures the \a render_system.
        render_configuration()
            : m_base_pipeline(render_pipeline::default_pbr)
            , m_vsync(true)
        {
            std::memset(m_render_steps, 0, render_step::number_of_step_types * sizeof(bool));
        }

        //! \brief Constructs a \a render_configuration with specific values.
        //! \param[in] base_render_pipeline The configurated base \a render_pipeline of the \a render_system to configure.
        //! \param[in] vsync The configurated setting for the \a render_system. Spezifies if vertical synchronization should be enabled or disabled.
        render_configuration(render_pipeline base_render_pipeline, bool vsync)
            : m_base_pipeline(base_render_pipeline)
            , m_vsync(vsync)
        {
            std::memset(m_render_steps, 0, render_step::number_of_step_types * sizeof(bool));
        }

        //! \brief Sets or changes the base \a render_pipeline of the \a render_system in the \a render_configuration.
        //! \param[in] base_render_pipeline The configurated base \a render_pipeline of the \a render_system to configure.
        //! \return A reference to the modified \a render_configuration.
        inline render_configuration& set_base_render_pipeline(render_pipeline base_render_pipeline)
        {
            m_base_pipeline = base_render_pipeline;
            return *this;
        }

        //! \brief Enables an additional \a render_step in the \a render_configuration.
        //! \details This is then used to add the \a render_step in to base \a render_pipeline of the \a render_system.
        //! \param[in] additional_render_step The configurated \a render_step to enable.
        //! \return A reference to the modified \a render_configuration.
        inline render_configuration& enable_render_step(render_step additional_render_step)
        {
            MANGO_ASSERT(additional_render_step < render_step::number_of_step_types, "Additional render step can not be added, it is out of bounds!");
            m_render_steps[additional_render_step] = true;
            return *this;
        }

        //! \brief Sets or changes the setting for vertical synchronization in the \a render_configuration.
        //! \param[in] vsync The configurated setting for the \a render_system. Spezifies if vertical synchronization should be enabled or disabled.
        //! \return A reference to the modified \a render_configuration.
        inline render_configuration& set_vsync(bool vsync)
        {
            m_vsync = vsync;
            return *this;
        }

        //! \brief Retrieves and returns the setting for vertical synchronization of the \a render_configuration.
        //! \return The current configurated vertical synchronization setting.
        inline bool is_vsync_enabled() const
        {
            return m_vsync;
        }

        //! \brief Retrieves and returns the base \a render_pipeline set in the \a render_configuration.
        //! \return The current configurated base \a render_pipeline of the \a render_system.
        inline render_pipeline get_base_render_pipeline() const
        {
            return m_base_pipeline;
        }

        //! \brief Retrieves and returns the array of possible \a render_steps set in the \a render_configuration.
        //! \details The returned array has the size \a render_step::number_of_step_types.
        //! On a specific position in the array there is the value 'true' if the \a render_step is enabled, else 'false'.
        //! \return The current configurated array of possible \a render_steps of the \a render_system.
        inline const bool* get_render_steps() const
        {
            return m_render_steps;
        }

      private:
        //! \brief The configurated base \a render_pipeline of the \a render_system to configure.
        render_pipeline m_base_pipeline;
        //! \brief The configurated setting of the \a render_configuration to enable or disable vertical synchronization.
        bool m_vsync;
        //! \brief The configurated additional \ render_steps of the \a render_configuration to enable or disable vertical synchronization.
        bool m_render_steps[render_step::number_of_step_types];
    };

    //! \brief Filter specification for shadow map samples.
    enum class shadow_filtering : uint8
    {
        hard_shadows   = 0, //!< Shadows point filtered, hard edge.
        softer_shadows = 1, //!< Shadows bilinear filtered, smooth edges.
        soft_shadows   = 2, //!< Shadows bilinear filtered, smooth edges. Bigger filter then soft.
        pcss           = 3, //!< Shadows bilinear filtered, smooth edges. Filter width depends on distance from occluder and light size.
        count          = 4
    };

    //! \brief The configuration for the \a shadow_map_step.
    class shadow_step_configuration
    {
      public:
        //! \brief Default constructor to set some default values.
        shadow_step_configuration()
            : m_resolution(2048)
            , m_sample_count(3)
            , m_light_size(4.0f)
            , m_offset(0.0f)
            , m_cascade_count(3)
            , m_lambda(0.65f)
            , m_slope_bias(0.005f)
            , m_normal_bias(0.01f)
            , m_interpolation_range(0.5f)
            , m_filter_mode(shadow_filtering::softer_shadows)
        {
        }

        //! \brief Constructs a \a shadow_step_configuration with specific values.
        //! \param[in] resolution The configurated resolution for each cascade shadow map.
        //! \param[in] sample_count The configurated pcf sample count.
        //! \param[in] offset The configurated offset for the orthographic cameras so that every bit of geometry can potentially cast shadows.
        //! \param[in] cascade_count The configurated number of cascades for the shadow mapping.
        //! \param[in] lambda The configurated lambda for the split calculation. 0 means complete uniform, 1 complete logarithmic.
        //! \param[in] slope_bias The configurated slope bias.
        //! \param[in] normal_bias The configurated normal bias.
        //! \param[in] interpolation_range The configurated interpolation range.
        //! \param[in] filter_mode The configurated shadow filter mode.
        shadow_step_configuration(int32 resolution, int32 sample_count, float offset, int32 cascade_count, float lambda, float slope_bias, float normal_bias, float interpolation_range, shadow_filtering filter_mode)
            : m_resolution(resolution)
            , m_sample_count(sample_count)
            , m_offset(offset)
            , m_cascade_count(cascade_count)
            , m_lambda(lambda)
            , m_slope_bias(slope_bias)
            , m_normal_bias(normal_bias)
            , m_interpolation_range(interpolation_range)
            , m_filter_mode(filter_mode)
        {
        }

        //! \brief Sets the shadow map resolution.
        //! \param[in] resolution The resolution to set the shadow maps to.
        //! \return A reference to the modified \a shadow_step_configuration.
        inline shadow_step_configuration& set_resolution(int32 resolution)
        {
            m_resolution = resolution;
            return *this;
        }

        //! \brief Sets the sample count.
        //! \param[in] sample_count The number of pcf samples.
        //! \return A reference to the modified \a shadow_step_configuration.
        inline shadow_step_configuration& set_sample_count(int32 sample_count)
        {
            m_sample_count = sample_count;
            return *this;
        }

        //! \brief Sets the light size.
        //! \param[in] light_size The size of the virtual pcss light.
        //! \return A reference to the modified \a shadow_step_configuration.
        inline shadow_step_configuration& set_light_size(float light_size)
        {
            m_light_size = light_size;
            return *this;
        }

        //! \brief Sets the shadow map camera offset.
        //! \param[in] offset The offset for the orthographic cameras so that every bit of geometry can potentially cast shadows.
        //! \return A reference to the modified \a shadow_step_configuration.
        inline shadow_step_configuration& set_offset(float offset)
        {
            m_offset = offset;
            return *this;
        }

        //! \brief Sets the shadow map slope bias.
        //! \param[in] slope_bias The slope bias to prevent shadow acne.
        //! \return A reference to the modified \a shadow_step_configuration.
        inline shadow_step_configuration& set_slope_bias(float slope_bias)
        {
            m_slope_bias = slope_bias;
            return *this;
        }

        //! \brief Sets the shadow map normal bias.
        //! \param[in] normal_bias The normal bias to prevent shadow acne.
        //! \return A reference to the modified \a shadow_step_configuration.
        inline shadow_step_configuration& set_normal_bias(float normal_bias)
        {
            m_normal_bias = normal_bias;
            return *this;
        }

        //! \brief Sets the number of shadow cascades.
        //! \param[in] cascade_count The number of shadow mapping cascades.
        //! \return A reference to the modified \a shadow_step_configuration.
        inline shadow_step_configuration& set_cascade_count(int32 cascade_count)
        {
            m_cascade_count = cascade_count;
            return *this;
        }

        //! \brief Sets the lambda to calculate the cascade splits with.
        //! \param[in] lambda The lambda to use.
        //! \details  0 means complete uniform, 1 complete logarithmic.
        //! \return A reference to the modified \a shadow_step_configuration.
        inline shadow_step_configuration& set_split_lambda(float lambda)
        {
            m_lambda = lambda;
            return *this;
        }

        //! \brief Sets range, the cascades get interpolated between.
        //! \param[in] interpolation_range The interpolation range to use.
        //! \details  Should be between 0 and 10. Bigger values need more pc power.
        //! \return A reference to the modified \a shadow_step_configuration.
        inline shadow_step_configuration& set_cascade_interpolation_range(float interpolation_range)
        {
            m_interpolation_range = interpolation_range;
            return *this;
        }

        //! \brief Sets the \a shadow_filtering mode.
        //! \param[in] filter_mode The filter mode to use.
        //! \return A reference to the modified \a shadow_step_configuration.
        inline shadow_step_configuration& set_filter_mode(shadow_filtering filter_mode)
        {
            m_filter_mode = filter_mode;
            return *this;
        }

        //! \brief Retrieves and returns the shadow map resolution.
        //! \return The configurated shadow map resolution.
        inline int32 get_resolution() const
        {
            return m_resolution;
        }

        //! \brief Retrieves and returns the sample count.
        //! \return The configurated sample count.
        inline int32 get_sample_count() const
        {
            return m_sample_count;
        }

        //! \brief Retrieves and returns the light size.
        //! \return The configurated light size.
        inline float get_light_size() const
        {
            return m_light_size;
        }

        //! \brief Retrieves and returns the shadow map orthographic camera offset.
        //! \return The configurated shadow map orthographic camera offset.
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
        //! \return The configurated number of cascades.
        inline int32 get_cascade_count() const
        {
            return m_cascade_count;
        }

        //! \brief Retrieves and returns the lambda used for split calculation.
        //! \return The configurated lambda used for split calculation.
        inline float get_split_lambda() const
        {
            return m_lambda;
        }

        //! \brief Retrieves and returns the interpolation range for cascade interpolation.
        //! \return The configurated interpolation range for cascade interpolation.
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

      private:
        //! \brief The configured shadow map resolution.
        int32 m_resolution;
        //! \brief The configured sample count.
        int32 m_sample_count;
        //! \brief The configured light size.
        float m_light_size;
        //! \brief The configured offset for the shadow map orthographic cameras.
        float m_offset;
        //! \brief The configured number of cascades.
        int32 m_cascade_count;
        //! \brief The configured splitting lambda
        float m_lambda;
        //! \brief The configurated m_slope bias.
        float m_slope_bias;
        //! \brief The configurated m_normal bias.
        float m_normal_bias;
        //! \brief The m_interpolation_range.
        float m_interpolation_range;
        //! \brief The filter mode. (Hard, softer, soft or pcss shadows).
        shadow_filtering m_filter_mode;
    };

    //! \brief The configuration for the \a cubemap_step.
    class cubemap_step_configuration
    {
      public:
        //! \brief Default constructor to set some default values.
        cubemap_step_configuration()
            : m_render_level(0.0f)
        {
        }

        //! \brief Constructs a \a cubemap_step_configuration with specific values.
        //! \param[in] render_level The configurated render_level to render the cubemap with.
        cubemap_step_configuration(float render_level)
            : m_render_level(render_level)
        {
        }

        //! \brief Sets the render level to render the cubemap with.
        //! \param[in] render_level The render_level to render the cubemap with.
        //! \return A reference to the modified \a cubemap_step_configuration.
        inline cubemap_step_configuration& set_render_level(float render_level)
        {
            m_render_level = render_level;
            return *this;
        }

        //! \brief Retrieves and returns the render level to render the cubemap with.
        //! \return The configurated render_level to render the cubemap with.
        inline float get_render_level() const
        {
            return m_render_level;
        }

      private:
        //! \brief The render level.
        float m_render_level;
    };

    //! \brief Presets for fxaa quality.
    enum class fxaa_quality_preset : uint8
    {
        medium_quality  = 0,
        high_quality    = 1,
        extreme_quality = 2,
        count           = 3
    };

    //! \brief The configuration for the \a fxaa_step.
    class fxaa_step_configuration
    {
      public:
        //! \brief Default constructor to set some default values.
        fxaa_step_configuration()
            : m_quality(fxaa_quality_preset::medium_quality)
            , m_subpixel_filter(0.0f)
        {
        }

        //! \brief Constructs a \a fxaa_step_configuration with specific values.
        //! \param[in] quality The configurated \a fxaa_quality_preset to render the fxaa with.
        //! \param[in] subpixel_filter The configurated subpixel filter value to render the fxaa with.
        fxaa_step_configuration(fxaa_quality_preset quality, float subpixel_filter)
            : m_quality(quality)
            , m_subpixel_filter(subpixel_filter)
        {
        }

        //! \brief Sets the \a fxaa_quality_preset to render the fxaa with.
        //! \param[in] quality The \a fxaa_quality_preset to render the fxaa with.
        //! \return A reference to the modified \a fxaa_step_configuration.
        inline fxaa_step_configuration& set_quality_preset(fxaa_quality_preset quality)
        {
            m_quality = quality;
            return *this;
        }

        //! \brief Retrieves and returns the \a fxaa_quality_preset to render the fxaa with.
        //! \return The configurated \a fxaa_quality_preset to render the fxaa with.
        inline fxaa_quality_preset get_quality_preset() const
        {
            return m_quality;
        }

        //! \brief Sets the  subpixel filter value to render the fxaa with.
        //! \param[in] subpixel_filter The  subpixel filter value to render the fxaa with.
        //! \return A reference to the modified \a fxaa_step_configuration.
        inline fxaa_step_configuration& set_subpixel_filter(float subpixel_filter)
        {
            m_subpixel_filter = subpixel_filter;
            return *this;
        }

        //! \brief Retrieves and returns the  subpixel filter value to render the fxaa with.
        //! \return The configurated subpixel filter value to render the fxaa with.
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

    //! \brief A system for window creation and handling.
    //! \details The \a render_system manages the handle of the window, swaps buffers after rendering and polls for input.
    class render_system : public system
    {
      public:
        //! \brief Does the configuration of the \a render_system.
        //! \details After creation this function should be called.
        //! Changes the configuration in the \a render_system to \a configuration.
        //! \param[in] configuration The \a render_configuration to use for the window.
        virtual void configure(const render_configuration& configuration) = 0;

        //! \brief Does the setup of the \a cubemap_step.
        //! \details After configuration this function should be called.
        //! \param[in] configuration The \a cubemap_step_configuration to use.
        virtual void setup_cubemap_step(const cubemap_step_configuration& configuration) = 0;
        //! \brief Does the setup of the \a shadow_map_step.
        //! \details After configuration this function should be called.
        //! \param[in] configuration The \a shadow_step_configuration to use.
        virtual void setup_shadow_map_step(const shadow_step_configuration& configuration) = 0;
        //! \brief Does the setup of the \a fxaa_step.
        //! \details After configuration this function should be called.
        //! \param[in] configuration The \a fxaa_step_configuration to use.
        virtual void setup_fxaa_step(const fxaa_step_configuration& configuration) = 0;

      protected:
        virtual bool create()         = 0;
        virtual void update(float dt) = 0;
        virtual void destroy()        = 0;
    };

} // namespace mango

#endif // MANGO_RENDER_SYSTEM_HPP
