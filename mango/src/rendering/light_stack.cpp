//! \file      light_stack.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2022
//! \copyright Apache License 2.0

#include <mango/profile.hpp>
#include <rendering/light_stack.hpp>
#include <resources/resources_impl.hpp>
#include <util/helpers.hpp>

using namespace mango;

light_stack::light_stack()
    : m_allocator(524288) // 0.5 MiB
{
    m_current_light_data.directional_light_direction    = vec3(0.5f, 0.5f, 0.5f);
    m_current_light_data.directional_light_color        = make_vec3(1.0f);
    m_current_light_data.directional_light_intensity    = default_directional_intensity;
    m_current_light_data.directional_light_cast_shadows = false;
    m_current_light_data.directional_light_valid        = false;

    m_current_light_data.skylight_intensity = default_skylight_intensity;
    m_current_light_data.skylight_valid     = false;
}

light_stack::~light_stack() {}

bool light_stack::init(const shared_ptr<context_impl>& context)
{
    m_allocator.init();
    m_shared_context = context;

    if (!m_skylight_builder.init(m_shared_context))
        return false;

    if (!m_atmosphere_builder.init(m_shared_context))
        return false;

    return true;
}

void light_stack::push(const directional_light& light)
{
    m_directional_stack.emplace_back(light);
}

void light_stack::push(const skylight& light)
{
    m_skylight_stack.emplace_back(light);
}

void light_stack::push(const atmospheric_light& light)
{
    m_atmosphere_stack.emplace_back(light);
}

void light_stack::update(scene_impl* scene, float dt)
{
    // TODO Paul: Think of a better way to limit and handle the updates...
    // Update only with 8 fps
    static float fps_lock = 0.0f;
    fps_lock += dt;
    if (fps_lock * 1000.0f < 1.0f / 8.0f)
    {
        m_directional_stack.clear();
        m_atmosphere_stack.clear();
        m_skylight_stack.clear();
        return;
    }
    fps_lock -= 1.0f / 8.0f;

    PROFILE_ZONE;
    GL_NAMED_PROFILE_ZONE("Light Stack Update");
    for (auto& c : m_light_cache)
        c.second.expired = true;

    m_current_shadow_casters.clear();
    m_global_skylight = 0;

    // order is important!
    update_directional_lights();
    update_atmosphere_lights(scene);
    update_skylights(scene);

    for (auto it = m_light_cache.begin(); it != m_light_cache.end();)
    {
        if (it->second.expired)
        {
            if (it->second.data)
                m_allocator.free_memory(it->second.data);
            it->second.data = nullptr;

            it = m_light_cache.erase(it);
        }
        else
            it++;
    }

    m_directional_stack.clear();
    m_atmosphere_stack.clear();
    m_skylight_stack.clear();
}

void light_stack::update_directional_lights()
{
    int32 i = 0;
    for (auto& d : m_directional_stack)
    {
        int64 checksum = calculate_checksum(reinterpret_cast<uint8*>(&d.cast_shadows), sizeof(bool));
        checksum += 2 * calculate_checksum(reinterpret_cast<uint8*>(&d.color), sizeof(color_rgb));
        checksum += 3 * calculate_checksum(reinterpret_cast<uint8*>(&d.contribute_to_atmosphere), sizeof(bool));
        checksum += 4 * calculate_checksum(reinterpret_cast<uint8*>(&d.direction), sizeof(vec3));
        checksum += 5 * calculate_checksum(reinterpret_cast<uint8*>(&d.intensity), sizeof(float));

        // find in light cache
        auto entry = m_light_cache.find(checksum);
        bool found = entry != m_light_cache.end();
        if (found)
            m_light_cache.at(checksum).expired = false;

        // create light cache entry if non existent or update it on change
        if (!found)
        {
            cache_entry new_entry;
            new_entry.expired = false;
            // No additional render data required for directional lights.
            m_light_cache[checksum] = new_entry;



            if (i == m_directional_stack.size() - 1)
            {
                m_atmosphere_builder.update_directional_influence(&d);
            }
        }
        ++i;
    }

    if (m_directional_stack.empty())
        return;
    // atm there is only one directional light bound :D
    const auto& light                                   = m_directional_stack.back();
    m_current_light_data.directional_light_valid        = true;
    m_current_light_data.directional_light_direction    = light.direction;
    m_current_light_data.directional_light_color        = light.color.as_vec3();
    m_current_light_data.directional_light_intensity    = light.intensity;
    m_current_light_data.directional_light_cast_shadows = light.cast_shadows;
    if (light.cast_shadows)
        m_current_shadow_casters.push_back(light);
}

void light_stack::update_atmosphere_lights(scene_impl* scene)
{
    int32 i = 0;
    for (auto& a : m_atmosphere_stack)
    {
        int64 checksum = calculate_checksum(reinterpret_cast<uint8*>(&a.intensity_multiplier), sizeof(float));
        checksum += 2 * calculate_checksum(reinterpret_cast<uint8*>(&a.scatter_points), sizeof(int32));
        checksum += 3 * calculate_checksum(reinterpret_cast<uint8*>(&a.scatter_points_second_ray), sizeof(int32));
        checksum += 4 * calculate_checksum(reinterpret_cast<uint8*>(&a.rayleigh_scattering_coefficients), sizeof(vec3));
        checksum += 5 * calculate_checksum(reinterpret_cast<uint8*>(&a.mie_scattering_coefficient), sizeof(float));
        checksum += 6 * calculate_checksum(reinterpret_cast<uint8*>(&a.density_multiplier), sizeof(vec2));
        checksum += 7 * calculate_checksum(reinterpret_cast<uint8*>(&a.ground_radius), sizeof(float));
        checksum += 8 * calculate_checksum(reinterpret_cast<uint8*>(&a.atmosphere_radius), sizeof(float));
        checksum += 9 * calculate_checksum(reinterpret_cast<uint8*>(&a.view_height), sizeof(float));
        checksum += 10 * calculate_checksum(reinterpret_cast<uint8*>(&a.mie_preferred_scattering_dir), sizeof(float));
        checksum += 11 * calculate_checksum(reinterpret_cast<uint8*>(&a.draw_sun_disc), sizeof(bool));

        // find in light cache
        auto entry = m_light_cache.find(checksum);
        bool found = entry != m_light_cache.end();
        if (found)
            m_light_cache.at(checksum).expired = false;

        found &= !m_atmosphere_builder.needs_rebuild();

        if (!found)
        {
            // recreate atmosphere
            void* cached_data = m_allocator.allocate(sizeof(atmosphere_cache));
            MANGO_ASSERT(cached_data, "Light Stack Out Of Memory!");
            memset(cached_data, 0, sizeof(atmosphere_cache));
            m_atmosphere_builder.build(scene, a, static_cast<atmosphere_cache*>(cached_data));

            cache_entry new_entry;
            new_entry.data    = static_cast<atmosphere_cache*>(cached_data);
            new_entry.expired = false;
            m_light_cache[checksum] = new_entry;

            if (i == m_atmosphere_stack.size() - 1)
            {
                auto last = m_light_cache.at(checksum);
                m_skylight_builder.update_atmosphere_influence(static_cast<atmosphere_cache*>(last.data));
            }
        }
        ++i;
    }
}

void light_stack::update_skylights(scene_impl* scene)
{
    for (auto& s : m_skylight_stack)
    {
        int64 checksum = calculate_checksum(reinterpret_cast<uint8*>(&s.dynamic), sizeof(bool));
        checksum += 2 * calculate_checksum(reinterpret_cast<uint8*>(&s.hdr_texture), sizeof(key));
        // TODO: Check these things but atm we do not have to recreate all the textures, when the intensity is changing -.-
        // checksum += calculate_checksum(reinterpret_cast<uint8*>(&s.intensity), sizeof(float));
        checksum += 3 * calculate_checksum(reinterpret_cast<uint8*>(&s.local), sizeof(bool));
        checksum += 4 * calculate_checksum(reinterpret_cast<uint8*>(&s.use_texture), sizeof(bool));
        checksum += 5 * calculate_checksum(reinterpret_cast<uint8*>(&s.use_atmospheric), sizeof(bool));

        // find in light cache
        auto entry = m_light_cache.find(checksum);
        bool found = entry != m_light_cache.end();
        if (found)
            m_light_cache.at(checksum).expired = false;

        if (!s.use_texture && s.use_atmospheric)
        {
            found &= !m_skylight_builder.needs_rebuild();
        }

        if (!found)
        {
            // recreate skylight cubemaps
            void* cached_data = m_allocator.allocate(sizeof(skylight_cache));
            MANGO_ASSERT(cached_data, "Light Stack Out Of Memory!");
            memset(cached_data, 0, sizeof(skylight_cache));
            m_skylight_builder.build(scene, s, static_cast<skylight_cache*>(cached_data));

            cache_entry new_entry;
            new_entry.data    = static_cast<skylight_cache*>(cached_data);
            new_entry.expired = false;
            m_light_cache[checksum] = new_entry;
        }

        // atm there is only one skylight bound and ist has to be the global one :D
        if (m_global_skylight == 0)
            m_global_skylight = checksum;
        if (!s.local)
        {
            m_current_light_data.skylight_valid     = true;
            m_current_light_data.skylight_intensity = s.intensity;
        }
    }
}

int64 light_stack::calculate_checksum(uint8* bytes, int64 size)
{
    int64 sum = 0;
    for (int64 i = 0; i < size; ++i)
    {
        sum += bytes[i];
    }
    return sum;
}
