//! \file      light_stack.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_LIGHT_STACK_HPP
#define MANGO_LIGHT_STACK_HPP

#include <memory/free_list_allocator.hpp>
#include <rendering/render_data_builder.hpp>
#include <rendering/renderer_impl.hpp>
#include <scene/scene_internals.hpp>
#include <unordered_map>

namespace mango
{
    //! \brief The light stack is responsible for building and binding the resources regarding lights.
    class light_stack
    {
      public:
        //! \brief Constructs a new \a light_stack.
        light_stack();
        ~light_stack();

        //! \brief Initializes the light stack.
        //! \param[in] context The internally shared context of mango.
        //! \return True on success, else false.
        bool init(const shared_ptr<context_impl>& context);

        //! \brief Pushes a light on the stack.
        //! \param[in] light A reference to the \a scene_light to push.
        void push(const scene_light& light);

        //! \brief Updates the stack.
        //! \param[in] scene A pointer to the current scene.
        void update(scene_impl* scene);

        //! \brief Retrieves the current \a light_data of the \a light_stack.
        //! \return The current \a light_data of the \a light_stack.
        inline light_data& get_light_data()
        {
            return m_current_light_data;
        }

        //! \brief Retrieves all lights casting shadows (atm only directional lights).
        //! \return A vector of lights that cast shadows.
        inline std::vector<directional_light> get_shadow_casters()
        {
            return m_current_shadow_casters;
        }

        //! \brief Returns a handle to the active skylight irradiance map.
        //! \return A \a gfx_handle of the skylight irradiance map.
        inline gfx_handle<const gfx_texture> get_skylight_irradiance_map()
        {
            auto entry = m_light_cache.find(m_global_skylight);
            if (entry != m_light_cache.end())
            {
                return static_cast<skylight_cache*>(entry->second.data)->irradiance_cubemap;
            }
            return nullptr;
        }

        //! \brief Returns a handle to the active skylight radiance map.
        //! \return A \a gfx_handle of the skylight radiance map.
        inline gfx_handle<const gfx_texture> get_skylight_specular_prefilter_map()
        {
            auto entry = m_light_cache.find(m_global_skylight);
            if (entry != m_light_cache.end())
            {
                return static_cast<skylight_cache*>(entry->second.data)->specular_prefiltered_cubemap;
            }
            return nullptr;
        }

        //! \brief Returns a handle to the skylight brdf lookup.
        //! \return A \a gfx_handle of the skylight brdf lookup texture.
        inline gfx_handle<const gfx_texture> get_skylight_brdf_lookup()
        {
            return m_skylight_builder.get_skylight_brdf_lookup();
        }

      private:
        //! \brief Mangos internal context for shared usage.
        shared_ptr<context_impl> m_shared_context;

        //! \brief A light render data cache entry.
        struct cache_entry
        {
            light_render_data* data = nullptr; //!< Pointer to render data.
            bool expired;                      //!< True if the cache entry is expired, else false.
        };

        //! \brief Updates directional lights.
        void update_directional_lights();
        //! \brief Updates atmospherical lights.
        void update_atmosphere_lights();
        //! \brief Updates skylights.
        //! \param[in] scene A pointer to the current scene.
        void update_skylights(scene_impl* scene);

        //! \brief Calculates the checksum of some bytes.
        //! \param[in] bytes Pointer to the bytes to calculate checksum for.
        //! \param[in] size The size of the bytes array.
        //! \return The checksum value.
        int64 calculate_checksum(uint8* bytes, int64 size); // TODO Paul: Maybe should be moved elsewhere.

        //! \brief Directional light stack.
        std::vector<directional_light> m_directional_stack;
        //! \brief Atmospheric light stack.
        std::vector<atmospheric_light> m_atmosphere_stack;
        //! \brief Skylight stack.
        std::vector<skylight> m_skylight_stack;

        //! \brief The allocator for render data.
        free_list_allocator m_allocator;

        //! \brief The light cache mapping checksum to render data.
        std::unordered_map<int64, cache_entry> m_light_cache;

        //! \brief The current \a light_data.
        light_data m_current_light_data;

        //! \brief The current global active skylight.
        int64 m_global_skylight;
        //! \brief The last global active skylight.
        int64 m_last_skylight;

        //! \brief List of current shadows casters.
        std::vector<directional_light> m_current_shadow_casters;

        //! \brief The render data builder for skylights.
        skylight_builder m_skylight_builder;

        // atmosphere_builder m_atmosphere_builder;
    };
} // namespace mango

#endif // MANGO_LIGHT_STACK_HPP
