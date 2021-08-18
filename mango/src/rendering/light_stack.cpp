//! \file      light_stack.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#include <mango/profile.hpp>
#include <rendering/light_stack.hpp>
#include <resources/resources_impl.hpp>
#include <util/helpers.hpp>

using namespace mango;

light_stack::light_stack()
    : m_allocator(524288) // 0.5 MiB
{
    m_current_light_data.directional_light.direction    = vec3(0.5f, 0.5f, 0.5f);
    m_current_light_data.directional_light.color        = vec3(1.0f);
    m_current_light_data.directional_light.intensity    = default_directional_intensity;
    m_current_light_data.directional_light.cast_shadows = false;
    m_current_light_data.directional_light.valid        = false;

    m_current_light_data.skylight.intensity = default_skylight_intensity;
    m_current_light_data.skylight.valid     = false;
}

light_stack::~light_stack() {}

bool light_stack::init(const shared_ptr<context_impl>& context)
{
    m_allocator.init();
    m_shared_context = context;

    if (!m_skylight_builder.init(m_shared_context))
        return false;

    // success &= m_atmosphere_builder.init();

    return true;
}

void light_stack::push(const scene_light& light)
{
    // push to correct stack
    switch (light.type)
    {
    case light_type::directional:
        m_directional_stack.emplace_back(light.public_data_as_directional_light.value());
        break;
    case light_type::atmospheric:
        m_atmosphere_stack.emplace_back(light.public_data_as_atmospheric_light.value());
        break;
    case light_type::skylight:
        m_skylight_stack.emplace_back(light.public_data_as_skylight.value());
        break;
    default:
        break;
    }
}

void light_stack::update(scene_impl* scene)
{
    PROFILE_ZONE;
    GL_NAMED_PROFILE_ZONE("Light Stack Update");
    for (auto& c : m_light_cache)
        c.second.expired = true;

    m_current_shadow_casters.clear();
    m_last_skylight = m_global_skylight;
    m_global_skylight = 0;

    // order is important!
    update_directional_lights();
    // update_atmosphere_lights();
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
    for (auto& d : m_directional_stack)
    {
        int64 checksum = calculate_checksum(reinterpret_cast<uint8*>(&d.cast_shadows), sizeof(bool));
        checksum += calculate_checksum(reinterpret_cast<uint8*>(&d.color), sizeof(color_rgb));
        checksum += calculate_checksum(reinterpret_cast<uint8*>(&d.contribute_to_atmosphere), sizeof(bool));
        checksum += calculate_checksum(reinterpret_cast<uint8*>(&d.direction), sizeof(vec3));
        checksum += calculate_checksum(reinterpret_cast<uint8*>(&d.intensity), sizeof(float));

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
            m_light_cache.insert({ checksum, new_entry });
        }
    }

    if (m_directional_stack.empty())
        return;
    // atm there is only one directional light bound :D
    const auto& light                                   = m_directional_stack.back();
    m_current_light_data.directional_light.valid        = true;
    m_current_light_data.directional_light.direction    = light.direction;
    m_current_light_data.directional_light.color        = light.color.values;
    m_current_light_data.directional_light.intensity    = light.intensity;
    m_current_light_data.directional_light.cast_shadows = light.cast_shadows;
    if (light.cast_shadows)
        m_current_shadow_casters.push_back(light);
}

void light_stack::update_atmosphere_lights()
{
    // TODO Paul: TBD!
    for (auto& a : m_atmosphere_stack)
    {
        int64 checksum = 0; // calculate_checksum(...)

        // find in light cache
        auto entry = m_light_cache.find(checksum);
        bool found = entry != m_light_cache.end();
        if (found)
            m_light_cache.at(checksum).expired = false;

        MANGO_UNUSED(a);

        // check dependencies
        // (dependencies are only directional lights for now)
        // Cases:
        // Dependency added -> a.dirty |= d.dirty
        // Dependency still here -> a.dirty |= d.dirty
        // Dependency removed -> new_dependency size smaller -> a.dirty = true
        // Dependency removed but other added -> new_dependency size the same -> a.dirty |= d.dirty (= true, because light is new)

        /*
                std::vector<entity> new_dependencies;
                for (auto& d : directional_stack)
                {
                    auto data = static_cast<directional_light_component*>(d.light_pointer);
                    if (data->atmospherical)
                    {
                        new_dependencies.push_back(d.light_entity);
                        a.dirty |= d.dirty;
                    }
                }

        if (found && new_dependencies.size() != entry->second.dependencies.size())
            a.dirty = true;


        // create light cache entry if non existent or update it on change
        if (a.dirty)
        {
            // recreate atmosphere LUTs
            // auto light                    = static_cast<atmosphere_light*>(a.light);
            atmosphere_cache* cached_data = static_cast<atmosphere_cache*>(m_allocator.allocate(sizeof(atmosphere_cache)));
            MANGO_ASSERT(cached_data, "Light Stack Out Of Memory!");
            // (cached_data, 0, sizeof(atmosphere_cache));
            // m_atmosphere_builder.build(light, cached_data);

            if (found)
            {
                m_light_cache.at(a.id).light_checksum = checksum;
                m_allocator.free_memory(m_light_cache.at(a.id).data);
                m_light_cache.at(a.id).data = cached_data;
            }
            else
            {
                cache_entry new_entry;
                new_entry.light_checksum = checksum;
                new_entry.data           = cached_data;
                new_entry.expired        = false;
                m_light_cache.insert({ a.id, new_entry });
            }
        }
        */
    }
}

void light_stack::update_skylights(scene_impl* scene)
{
    for (auto& s : m_skylight_stack)
    {
        int64 checksum = calculate_checksum(reinterpret_cast<uint8*>(&s.dynamic), sizeof(bool));
        checksum += calculate_checksum(reinterpret_cast<uint8*>(&s.hdr_texture), sizeof(sid));
        checksum += calculate_checksum(reinterpret_cast<uint8*>(&s.intensity), sizeof(float));
        checksum += calculate_checksum(reinterpret_cast<uint8*>(&s.local), sizeof(bool));
        checksum += calculate_checksum(reinterpret_cast<uint8*>(&s.use_texture), sizeof(bool));

        if(m_global_skylight == 0)
            m_global_skylight = checksum;

        // find in light cache
        auto entry = m_light_cache.find(checksum);
        bool found = entry != m_light_cache.end();
        if (found)
            m_light_cache.at(checksum).expired = false;

        // if skylight captures (does not depend on hdr image) check dependencies
        // (dependencies are only atmosphere lights for now)

        // if (!s.use_texture)
        // {
        //     for (auto& a : m_atmosphere_stack)
        //     {
        //         // m_skylight_builder.add_atmosphere_influence(static_cast<atmosphere_light*>(a.light));
        //         s.dirty |= a.dirty;
        //     }
        //     s.dirty |= m_skylight_builder.needs_rebuild();
        // }

        // create light cache entry if non existent or update it on change
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
            m_light_cache.insert({ checksum, new_entry });
        }

        // atm there is only one skylight bound and ist has to be the global one :D
        if (!s.local)
        {
            m_current_light_data.skylight.valid     = true;
            m_current_light_data.skylight.intensity = s.intensity;
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
