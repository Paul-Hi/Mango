//! \file      light_stack.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_LIGHT_STACK_HPP
#define MANGO_LIGHT_STACK_HPP

#include <graphics/texture.hpp>
#include <memory/free_list_allocator.hpp>
#include <rendering/render_data_builder.hpp>
#include <unordered_map>

namespace mango
{
    class light_stack
    {
      public:
        //! \brief Constructs a new \a light_stack.
        light_stack();
        ~light_stack();

        bool init();

        struct light_entry
        {
            light_id id;
            mango_light* light;
            bool dirty;
        };

        void push(light_id id, mango_light* light);

        void update();

        void bind_light_buffers(const command_buffer_ptr<min_key>& global_binding_commands, gpu_buffer_ptr frame_uniform_buffer);

        inline std::vector<directional_light*> get_shadow_casters()
        {
            return m_current_shadow_casters;
        }

        inline texture_ptr get_skylight_irradiance_map()
        {
            auto entry = m_light_cache.find(m_global_skylight);
            if (entry != m_light_cache.end())
            {
                return static_cast<skylight_cache*>(entry->second.data)->irradiance_cubemap;
            }
            return nullptr;
        }

        inline texture_ptr get_skylight_specular_prefilter_map()
        {
            auto entry = m_light_cache.find(m_global_skylight);
            if (entry != m_light_cache.end())
            {
                return static_cast<skylight_cache*>(entry->second.data)->specular_prefiltered_cubemap;
            }
            return nullptr;
        }

        inline texture_ptr get_skylight_brdf_lookup()
        {
            return m_brdf_integration_lut;
        }

      private:
        struct cache_entry
        {
            int64 light_checksum;
            light_render_data* data;
            bool expired;
        };

        void create_brdf_lookup();

        void update_directional_lights();
        void update_atmosphere_lights();
        void update_skylights();

        int64 calculate_checksum(uint8* bytes, int64 size);

        std::vector<light_entry> m_directional_stack;
        std::vector<light_entry> m_atmosphere_stack;
        std::vector<light_entry> m_skylight_stack;

        free_list_allocator m_allocator;

        std::unordered_map<light_id, cache_entry> m_light_cache;

        light_id m_global_skylight;

        std::vector<directional_light*> m_current_shadow_casters;

        // global light render data.
        texture_ptr m_brdf_integration_lut;

        // data builder
        skylight_builder m_skylight_builder;
        atmosphere_builder m_atmosphere_builder;

        //! \brief Compute shader program building the look up brdf integration texture for \a skylights.
        shader_program_ptr m_build_integration_lut;

        // required constants
        const int32 brdf_lut_size = 256;

        struct light_buffer
        {
            struct directional_light_buffer
            {
                std140_vec3 direction;    //!< The direction to the light.
                std140_vec3 color;        //!< The light color.
                std140_float intensity;   //!< The intensity of the directional light in lumen.
                std140_bool cast_shadows; //!< True, if shadows can be casted.
                std140_bool valid;        //!< True, if buffer is valid.
            } directional_light;          //!< Data for the active directional light (max one atm)

            struct skylight_buffer
            {
                std140_float intensity; //!< The intensity of the skylight in cd/m^2.
                std140_bool valid;      //!< True, if buffer is valid. Does also guarantee that textures are bound.
                // local, global and if local bounds for parallax correction ....
            } skylight;        //!< Data for the active skylight (max one atm)
            std140_float pad0; //!< Padding.
            std140_float pad1; //!< Padding.
            std140_float pad2; //!< Padding.
        } m_light_buffer;
    };
} // namespace mango

#endif // MANGO_LIGHT_STACK_HPP
