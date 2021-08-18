//! \file      render_data_builder.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_RENDER_DATA_BUILDER_HPP
#define MANGO_RENDER_DATA_BUILDER_HPP

#include <rendering/renderer_impl.hpp>
#include <scene/scene_internals.hpp>

namespace mango
{
    //! \brief Base class of render data for lights.
    struct light_render_data
    {
    };

    //! \brief Render data for directional lights.
    struct directional_cache : light_render_data
    {
    };

    //! \brief Render data for atmospherical lights.
    struct atmosphere_cache : light_render_data
    {
    };

    //! \brief Render data for skylights.
    struct skylight_cache : light_render_data
    {
        gfx_handle<const gfx_texture> cubemap;                      //!< The cubemap.
        gfx_handle<const gfx_texture> irradiance_cubemap;           //!< The irradiance convolution cubemap.
        gfx_handle<const gfx_texture> specular_prefiltered_cubemap; //!< The specular radiance convolution cubemap.
    };

    //! \brief A builder base class for creating render data.
    template <typename T, typename data>
    class render_data_builder
    {
      public:
        //! \brief Initializes the \a render_data_builder.
        //! \param[in] context The internally shared context of mango.
        //! \return True on success, else false.
        virtual bool init(const shared_ptr<context_impl>& context) = 0;
        //! \brief Queries, if the data needs to be rebuild
        //! \return True data needs to be rebuild, else false.
        virtual bool needs_rebuild() = 0;
        //! \brief Builds the render data for a specific type.
        //! \param[in] scene A pointer to the current scene.
        //! \param[in] type The input data of type T.
        //! \param[out] render_data The output render data.
        virtual void build(scene_impl* scene, const T& type, data* render_data) = 0;

      protected:
        //! \brief Mangos internal context for shared usage.
        shared_ptr<context_impl> m_shared_context;
    };

    //! \brief A builder class for skylight render data.
    class skylight_builder : render_data_builder<skylight, skylight_cache>
    {
      public:
        bool init(const shared_ptr<context_impl>& context) override;
        bool needs_rebuild() override;
        void build(scene_impl* scene, const skylight& light, skylight_cache* render_data) override;

        // void add_atmosphere_influence(atmospheric_light* light);

        //! \brief Returns a handle to the skylight brdf lookup.
        //! \return A \a gfx_handle of the skylight brdf lookup texture.
        inline gfx_handle<const gfx_texture> get_skylight_brdf_lookup()
        {
            return m_brdf_integration_lut;
        }

      private:
        //! \brief Compute \a shader_stage converting a equirectangular hdr to a cubemap.
        gfx_handle<const gfx_shader_stage> m_equi_to_cubemap;
        //! \brief Compute \a shader_stage creating a cubemap with atmospheric scattering.
        gfx_handle<const gfx_shader_stage> m_atmospheric_cubemap;
        //! \brief Compute \a shader_stage for the generation of a irradiance map from a cubemap.
        gfx_handle<const gfx_shader_stage> m_build_irradiance_map;
        //! \brief Compute \a shader_stage for the generation of the prefiltered specular map from a cubemap.
        gfx_handle<const gfx_shader_stage> m_build_specular_prefiltered_map;
        //! \brief Compute \a shader_stage converting a equirectangular hdr to a cubemap.

        //! \brief Compute pipeline converting a equirectangular hdr to a cubemap.
        gfx_handle<const gfx_pipeline> m_equi_to_cubemap_pipeline;
        //! \brief Compute pipeline creating a cubemap with atmospheric scattering.
        gfx_handle<const gfx_pipeline> m_generate_atmospheric_cubemap_pipeline;
        //! \brief Compute pipeline to generate a irradiance map from a cubemap.
        gfx_handle<const gfx_pipeline> m_build_irradiance_map_pipeline;
        //! \brief Compute pipeline to generate the prefiltered specular map from a cubemap.
        gfx_handle<const gfx_pipeline> m_build_specular_prefiltered_map_pipeline;

        //! \brief Old dependencies on atmospheric lights.
        std::vector<atmospheric_light*> old_dependencies;
        //! \brief New dependencies on atmospheric lights.
        std::vector<atmospheric_light*> new_dependencies;

        //! \brief Creates the brdf lookup for skylights.
        void create_brdf_lookup();

        //! \brief Loads render data members from a hdr image.
        //! \param[in] scene A pointer to the current scene.
        //! \param[in] light Reference to the light.
        //! \param[out] render_data Pointer to the render data.
        void load_from_hdr(scene_impl* scene, const skylight& light, skylight_cache* render_data);

        // void capture(const command_buffer_ptr<min_key>& compute_commands, skylight_cache* render_data);

        //! \brief Calculates the convolution maps.
        //! \param[in,out] render_data Pointer to the render data.
        void calculate_ibl_maps(skylight_cache* render_data);

        //! \brief Clears the data.
        //! \param[in,out] render_data Pointer to the render data to clear.
        void clear(skylight_cache* render_data);

        //! \brief The size of the base cubemap faces.
        const int32 global_cubemap_size = 1024;
        //! \brief The size of the irradiance cubemap faces.
        const int32 global_irradiance_map_size = 64;
        //! \brief The size of the radiance cubemap faces.
        const int32 global_specular_convolution_map_size = 1024;

        // const int32 local_cubemap_size = 1024;
        // const int32 local_irradiance_map_size = 32;
        // const int32 local_specular_convolution_map_size = 1024;

        //! \brief The brdf lookup texture for skylights.
        gfx_handle<const gfx_texture> m_brdf_integration_lut;

        //! \brief Compute pipeline building the look up brdf integration texture for \a skylights.
        gfx_handle<const gfx_pipeline> m_brdf_integration_lut_pipeline;

        //! \brief The compute \a shader_stage for the generation of the brdf integration lookup.
        gfx_handle<const gfx_shader_stage> m_brdf_lookup_generation_compute;

        //! \brief Size of the brdf lookup texture for skylights.
        const int32 brdf_lut_size = 256;

        //! \brief Uniform buffer struct for data used by the ibl generation shaders.
        //! \details Bound to binding point 3.
        struct ibl_generator_data
        {
            std140_vec2 out_size; //!< The size of the output image.
            std140_vec2 data;     //!< Data used for calculation - Specification in the shaders.
        };
        //! \brief The current \a ibl_generator_data.
        ibl_generator_data m_current_ibl_generator_data;
        //! \brief The graphics uniform buffer for uploading \a ibl_generator_data.
        gfx_handle<const gfx_buffer> m_ibl_generator_data_buffer;
    };

    /*
    class atmosphere_builder : render_data_builder<atmosphere_light, atmosphere_cache>
    {
      public:
        bool init() override;
        bool needs_rebuild() override;
        void build(atmosphere_light* light, atmosphere_cache* render_data) override;
    };
    */
} // namespace mango

#endif // MANGO_RENDER_DATA_BUILDER_HPP
