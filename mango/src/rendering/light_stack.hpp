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
    //! \brief The lihght stack is responsible for building and binding the resources regarding lights.
    class light_stack
    {
      public:
        //! \brief Constructs a new \a light_stack.
        light_stack();
        ~light_stack();

        //! \brief Initializes the light stack.
        bool init();

        //! \brief An entry on the stack.
        struct light_entry
        {
            light_id id;        //!< The id of the entry.
            mango_light* light; //!< Pointer to the light of the entry.
            bool dirty;         //!< True if entry is dirty, else false.
        };

        //! \brief Pushes a light on the stack.
        //! \param[in] id The id of the light to push.
        //! \param[in] light Pointer to the light to push.
        void push(light_id id, mango_light* light);

        //! \brief Updates the stack.
        void update();

        //! \brief Binds the gpu light buffers.
        //! \param[in] global_binding_commands The command buffer to submit the binding commands to.
        //! \param[in] frame_uniform_buffer The buffer to store the data in.
        void bind_light_buffers(const command_buffer_ptr<min_key>& global_binding_commands, gpu_buffer_ptr frame_uniform_buffer);

        //! \brief Retrieves all lights casting shadows (atm only directional lights).
        //! \return A vector of lights that cast shadows.
        inline std::vector<directional_light*> get_shadow_casters()
        {
            return m_current_shadow_casters;
        }

        //! \brief Returns a texture_ptr to the active skylight irradiance map.
        //! \return A texture_ptr to the skylight irradiance map.
        inline texture_ptr get_skylight_irradiance_map()
        {
            auto entry = m_light_cache.find(m_global_skylight);
            if (entry != m_light_cache.end())
            {
                return static_cast<skylight_cache*>(entry->second.data)->irradiance_cubemap;
            }
            return nullptr;
        }

        //! \brief Returns a texture_ptr to the active skylight radiance map.
        //! \return A texture_ptr to the skylight radiance map.
        inline texture_ptr get_skylight_specular_prefilter_map()
        {
            auto entry = m_light_cache.find(m_global_skylight);
            if (entry != m_light_cache.end())
            {
                return static_cast<skylight_cache*>(entry->second.data)->specular_prefiltered_cubemap;
            }
            return nullptr;
        }

        //! \brief Returns a texture_ptr to the skylight brdf lookup.
        //! \return A texture_ptr to the skylight brdf lookup.
        inline texture_ptr get_skylight_brdf_lookup()
        {
            return m_brdf_integration_lut;
        }

        //! \brief Checks if lighting is dirty.
        //! \return True if lighting is dirty, else false.
        inline bool lighting_dirty()
        {
            return m_lighting_dirty;
        }

      private:
        //! \brief A light render data cache entry.
        struct cache_entry
        {
            int64 light_checksum; //!< Checksum of the light to check for changes.
            light_render_data* data = nullptr; //!< Pointer to render data.
            bool expired; //!< True if the cache entry is expired, else false.
        };

        //! \brief Creates the brdf lookup for skylights.
        void create_brdf_lookup();

        //! \brief Updates directional lights.
        void update_directional_lights();
        //! \brief Updates atmospherical lights.
        void update_atmosphere_lights();
        //! \brief Updates skylights.
        void update_skylights();

        //! \brief Calculates the checksum of some bytes.
        //! \param[in] bytes Pointer to the bytes to calculate checksum for.
        //! \param[in] size The size of the bytes array.
        //! \return The checksum value.
        int64 calculate_checksum(uint8* bytes, int64 size); // TODO Paul: Maybe should be moved elsewhere.

        //! \brief Directional light stack.
        std::vector<light_entry> m_directional_stack;
        //! \brief Atmospheric light stack.
        std::vector<light_entry> m_atmosphere_stack;
        //! \brief Skylight stack.
        std::vector<light_entry> m_skylight_stack;

        //! \brief The allocator for render data.
        free_list_allocator m_allocator;

        //! \brief The light cache mapping light_id to render data.
        std::unordered_map<light_id, cache_entry> m_light_cache;

        //! \brief The current global active skylight.
        light_id m_global_skylight = invalid_light_id;
        //! \brief The last global active skylight.
        light_id m_last_skylight   = invalid_light_id;
        //! \brief True if lighting is dirty, else false.
        bool m_lighting_dirty;

        //! \brief List of current shadows casters.
        std::vector<directional_light*> m_current_shadow_casters;

        //! \brief The brdf lookup texture for skylights.
        texture_ptr m_brdf_integration_lut;

        //! \brief The render data builder for skylights.
        skylight_builder m_skylight_builder;

        // atmosphere_builder m_atmosphere_builder;

        //! \brief Compute shader program building the look up brdf integration texture for \a skylights.
        shader_program_ptr m_build_integration_lut;

        //! \brief Size of the brdf lookup texture for skylights.
        const int32 brdf_lut_size = 256;

        //! \brief Light buffer data.
        struct light_buffer
        {
            //! \brief Directional lights data.
            struct directional_light_buffer
            {
                std140_vec3 direction;    //!< The direction to the light.
                std140_vec3 color;        //!< The light color.
                std140_float intensity;   //!< The intensity of the directional light in lumen.
                std140_bool cast_shadows; //!< True, if shadows can be casted.
                std140_bool valid;        //!< True, if buffer is valid.
            } directional_light;          //!< Data for the active directional light (max one atm)

            //! \brief Skylights data.
            struct skylight_buffer
            {
                std140_float intensity; //!< The intensity of the skylight in cd/m^2.
                std140_bool valid;      //!< True, if buffer is valid. Does also guarantee that textures are bound.
                // local, global and if local bounds for parallax correction ....
            } skylight;        //!< Data for the active skylight (max one atm)
            std140_float pad0; //!< Padding.
            std140_float pad1; //!< Padding.
            std140_float pad2; //!< Padding.
        } m_light_buffer; //!< Current light buffer data.
    };
} // namespace mango

#endif // MANGO_LIGHT_STACK_HPP
