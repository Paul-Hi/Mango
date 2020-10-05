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
        ibl,
        shadow_map,
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

    //! \brief The configuration for the \a shadow_map_step.
    class shadow_step_configuration
    {
      public:
        //! \brief Default constructor to set some default values.
        shadow_step_configuration()
            : m_resolution(2048)
            , m_max_penumbra(3.0f)
            , m_offset(0.0f)
            , m_cascade_count(3)
            , m_lambda(0.65f)
        {
        }

        //! \brief Constructs a \a shadow_step_configuration with specific values.
        //! \param[in] resolution The configurated resolution for each cascade shadow map.
        //! \param[in] max_penumbra The configurated maximum diffusion value for soft shadows.
        //! \param[in] offset The configurated offset for the orthographic cameras so that every bit of geometry can potentially cast shadows.
        //! \param[in] cascade_count The configurated number of cascades for the shadow mapping.
        //! \param[in] lambda The configurated lambda for the split calculation. 0 means complete uniform, 1 complete logarithmic.
        shadow_step_configuration(int32 resolution, float max_penumbra, float offset, int32 cascade_count, float lambda)
            : m_resolution(resolution)
            , m_max_penumbra(max_penumbra)
            , m_offset(offset)
            , m_cascade_count(cascade_count)
            , m_lambda(lambda)
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

        //! \brief Sets the maximum penumbra.
        //! \param[in] max_penumbra The maximum diffusion value for soft shadows.
        //! \return A reference to the modified \a shadow_step_configuration.
        inline shadow_step_configuration& set_max_penumbra(float max_penumbra)
        {
            m_max_penumbra = max_penumbra;
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

        //! \brief Sets the number of shadow cascades.
        //! \param[in] cascade_count The number of shadow mapping cascades.
        //! \return A reference to the modified \a shadow_step_configuration.
        inline shadow_step_configuration& set_cascade_count(int32 cascade_count)
        {
            m_cascade_count = cascade_count;
            return *this;
        }

        //! \brief Sets the lambda to calculate th cascade splits with.
        //! \param[in] lambda The lambda to use.
        //! \details  0 means complete uniform, 1 complete logarithmic.
        //! \return A reference to the modified \a shadow_step_configuration.
        inline shadow_step_configuration& set_split_lambda(float lambda)
        {
            m_lambda = lambda;
            return *this;
        }

        //! \brief Retrieves and returns the shadow map resolution.
        //! \return The configurated shadow map resolution.
        inline int32 get_resolution() const
        {
            return m_resolution;
        }

        //! \brief Retrieves and returns the maximum penumbra.
        //! \return The configurated maximum penumbra.
        inline float get_max_penumbra() const
        {
            return m_max_penumbra;
        }

        //! \brief Retrieves and returns the shadow map orthographic camera offset.
        //! \return The configurated shadow map orthographic camera offset.
        inline float get_offset() const
        {
            return m_offset;
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

      private:
        //! \brief The configured shadow map resolution.
        int32 m_resolution;
        //! \brief The configured maximum penumbra.
        float m_max_penumbra;
        //! \brief The configured offset for the shadow map orthographic cameras.
        float m_offset;
        //! \brief The configured number of cascades.
        int32 m_cascade_count;
        //! \brief The configured splitting lambda
        float m_lambda;
    };

    //! \brief The configuration for the \a ibl_step.
    class ibl_step_configuration
    {
      public:
        //! \brief Default constructor to set some default values.
        ibl_step_configuration()
            : m_render_level(0.0f)
        {
        }

        //! \brief Constructs a \a ibl_step_configuration with specific values.
        //! \param[in] render_level The configurated render_level to render the ibl with.
        ibl_step_configuration(float render_level)
            : m_render_level(render_level)
        {
        }

        //! \brief Sets the render level to render the ibl with.
        //! \param[in] render_level The render_level to render the ibl with.
        //! \return A reference to the modified \a ibl_step_configuration.
        inline ibl_step_configuration& set_render_level(float render_level)
        {
            m_render_level = render_level;
            return *this;
        }

        //! \brief Retrieves and returns the render level to render the ibl with.
        //! \return The configurated render_level to render the ibl with.
        inline float get_render_level() const
        {
            return m_render_level;
        }

      private:
        //! \brief The render level.
        float m_render_level;
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

        //! \brief Does the setup of the \a ibl_step.
        //! \details After configuration this function should be called.
        //! \param[in] configuration The \a ibl_step_configuration to use.
        virtual void setup_ibl_step(const ibl_step_configuration& configuration) = 0;
        //! \brief Does the setup of the \a shadow_map_step.
        //! \details After configuration this function should be called.
        //! \param[in] configuration The \a shadow_step_configuration to use.
        virtual void setup_shadow_map_step(const shadow_step_configuration& configuration) = 0;

      protected:
        virtual bool create()         = 0;
        virtual void update(float dt) = 0;
        virtual void destroy()        = 0;
    };

} // namespace mango

#endif // MANGO_RENDER_SYSTEM_HPP
