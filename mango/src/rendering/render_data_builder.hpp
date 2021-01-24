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
    using light_id = uint32;
    const light_id invalid_light_id = 0;

    struct light_render_data
    {
    };
    struct directional_cache : light_render_data
    {
    };

    struct atmosphere_cache : light_render_data
    {
    };

    struct skylight_cache : light_render_data
    {
        shared_ptr<texture> cubemap;
        shared_ptr<texture> irradiance_cubemap;
        shared_ptr<texture> specular_prefiltered_cubemap;
    };

    template <typename T, typename data>
    class render_data_builder
    {
      public:
        virtual bool init()           = 0;
        virtual bool needs_rebuild()  = 0;
        virtual void build(T*, data*) = 0;
    };

    class skylight_builder : render_data_builder<skylight, skylight_cache>
    {
      public:
        bool init() override;
        bool needs_rebuild() override;
        void build(skylight* light, skylight_cache* render_data) override;

        inline void set_draw_commands(const command_buffer_ptr<max_key>& draw_commands)
        {
            m_draw_commands = draw_commands;
        }

        void add_atmosphere_influence(atmosphere_light* light);

      private:
        //! \brief Compute shader program converting a equirectangular hdr to a cubemap.
        shader_program_ptr m_equi_to_cubemap;
        //! \brief Compute shader program createing a cubemap with atmospheric scattering.
        shader_program_ptr m_atmospheric_cubemap;
        //! \brief Compute shader program building a irradiance map from a cubemap.
        shader_program_ptr m_build_irradiance_map;
        //! \brief Compute shader program building the prefiltered specular map from a cubemap.
        shader_program_ptr m_build_specular_prefiltered_map;

        std::vector<atmosphere_light*> old_dependencies;
        std::vector<atmosphere_light*> new_dependencies;

        void load_from_hdr(const command_buffer_ptr<min_key>& compute_commands, skylight* light, skylight_cache* render_data);
        void capture(const command_buffer_ptr<min_key>& compute_commands, skylight* light, skylight_cache* render_data);

        void calculate_ibl_maps(const command_buffer_ptr<min_key>& compute_commands, skylight* light, skylight_cache* render_data);

        void clear(skylight_cache* render_data);

        // some required constants
        const int32 global_cubemap_size                  = 1024;
        const int32 global_irradiance_map_size           = 64;
        const int32 global_specular_convolution_map_size = 1024;

        command_buffer_ptr<max_key> m_draw_commands;

        // const int32 local_cubemap_size = 1024;
        // const int32 local_irradiance_map_size = 32;
        // const int32 local_specular_convolution_map_size = 1024;
    };

    class atmosphere_builder : render_data_builder<atmosphere_light, atmosphere_cache>
    {
      public:
        bool init() override;
        bool needs_rebuild() override;
        void build(atmosphere_light* light, atmosphere_cache* render_data) override;
    };
} // namespace mango

#endif // MANGO_RENDER_DATA_BUILDER_HPP
