//! \file      render_data_builder.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_RENDER_DATA_BUILDER_HPP
#define MANGO_RENDER_DATA_BUILDER_HPP

#include <graphics/command_buffer.hpp>
#include <graphics/shader_program.hpp>

namespace mango
{
    //! \brief Light id.
    using light_id = uint32;
    //! \brief An invalid id for lights.
    const light_id invalid_light_id = 0;

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
        shared_ptr<texture> cubemap;                      //!< The cubemap.
        shared_ptr<texture> irradiance_cubemap;           //!< The irradiance convolution cubemap.
        shared_ptr<texture> specular_prefiltered_cubemap; //!< The specular radiance convolution cubemap.
    };

    //! \brief A builder base class for creating render data.
    template <typename T, typename data>
    class render_data_builder
    {
      public:
        //! \brief Initializes the \a render_data_builder.
        //! \return True on success, else false.
        virtual bool init() = 0;
        //! \brief Queries, if the data needs to be rebuild
        //! \return True data needs to be rebuild, else false.
        virtual bool needs_rebuild() = 0;
        //! \brief Builds the render data for a specific type.
        //! \param[in] type The input data of type T.
        //! \param[out] render_data The output render data.
        virtual void build(T* type, data* render_data) = 0;
    };

    //! \brief A builder class for skylight render data.
    class skylight_builder : render_data_builder<skylight, skylight_cache>
    {
      public:
        bool init() override;
        bool needs_rebuild() override;
        void build(skylight* light, skylight_cache* render_data) override;

        /*
        inline void set_draw_commands(const command_buffer_ptr<max_key>& draw_commands)
        {
            m_draw_commands = draw_commands;
        }

        void add_atmosphere_influence(atmosphere_light* light);
        */

      private:
        //! \brief Compute shader program converting a equirectangular hdr to a cubemap.
        shader_program_ptr m_equi_to_cubemap;
        //! \brief Compute shader program createing a cubemap with atmospheric scattering.
        shader_program_ptr m_atmospheric_cubemap;
        //! \brief Compute shader program building a irradiance map from a cubemap.
        shader_program_ptr m_build_irradiance_map;
        //! \brief Compute shader program building the prefiltered specular map from a cubemap.
        shader_program_ptr m_build_specular_prefiltered_map;

        //! \brief Old dependencies on atmospheric lights.
        std::vector<atmosphere_light*> old_dependencies;
        //! \brief New dependencies on atmospheric lights.
        std::vector<atmosphere_light*> new_dependencies;

        //! \brief Loads render data members from a hdr image.
        //! \param[in,out] compute_commands The command buffer to submit commands to.
        //! \param[in] light Pointer to the light.
        //! \param[out] render_data Pointer to the render data.
        void load_from_hdr(const command_buffer_ptr<min_key>& compute_commands, skylight* light, skylight_cache* render_data);

        /*
        void capture(const command_buffer_ptr<min_key>& compute_commands, skylight_cache* render_data);
        */

        //! \brief Calculates the convolution maps.
        //! \param[in,out] compute_commands The command buffer to submit commands to.
        //! \param[in,out] render_data Pointer to the render data.
        void calculate_ibl_maps(const command_buffer_ptr<min_key>& compute_commands, skylight_cache* render_data);

        //! \brief Clears the data.
        //! \param[in,out] render_data Pointer to the render data to clear.
        void clear(skylight_cache* render_data);

        //! \brief The size of the base cubemap faces.
        const int32 global_cubemap_size = 1024;
        //! \brief The size of the irradiance cubemap faces.
        const int32 global_irradiance_map_size = 64;
        //! \brief The size of the radiance cubemap faces.
        const int32 global_specular_convolution_map_size = 1024;

        // command_buffer_ptr<max_key> m_draw_commands;

        // const int32 local_cubemap_size = 1024;
        // const int32 local_irradiance_map_size = 32;
        // const int32 local_specular_convolution_map_size = 1024;
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
