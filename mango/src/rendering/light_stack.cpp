//! \file      light_stack.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <graphics/command_buffer.hpp>
#include <graphics/gpu_buffer.hpp>
#include <graphics/shader.hpp>
#include <graphics/shader_program.hpp>
#include <graphics/texture.hpp>
#include <mango/profile.hpp>
#include <rendering/light_stack.hpp>
#include <util/helpers.hpp>

using namespace mango;

light_stack::light_stack()
    : m_allocator(524288) // 0.5 MiB
{
}

light_stack::~light_stack() {}

bool light_stack::init()
{
    m_allocator.init();

    bool success = m_skylight_builder.init();

    // success &= m_atmosphere_builder.init();

    create_brdf_lookup();
    if (!m_brdf_integration_lut)
        return false;

    return success;
}

void light_stack::create_brdf_lookup()
{
    texture_configuration texture_config;
    texture_config.generate_mipmaps        = 1;
    texture_config.is_standard_color_space = false;
    texture_config.is_cubemap              = false;
    texture_config.texture_min_filter      = texture_parameter::filter_linear;
    texture_config.texture_mag_filter      = texture_parameter::filter_linear;
    texture_config.texture_wrap_s          = texture_parameter::wrap_clamp_to_edge;
    texture_config.texture_wrap_t          = texture_parameter::wrap_clamp_to_edge;
    m_brdf_integration_lut                 = texture::create(texture_config);
    if (!check_creation(m_brdf_integration_lut.get(), "brdf integration lookup texture"))
        return;

    m_brdf_integration_lut->set_data(format::rgba16f, brdf_lut_size, brdf_lut_size, format::rgba, format::t_float, nullptr);

    shader_configuration shader_config;
    // compute shader to build the brdf integration lookup texture for image based lighting. Could be done only once.
    shader_config.path                  = "res/shader/pbr_compute/c_brdf_integration.glsl";
    shader_config.type                  = shader_type::compute_shader;
    shader_ptr brdf_integration_compute = shader::create(shader_config);
    if (!check_creation(brdf_integration_compute.get(), "brdf integration compute shader"))
        return;

    m_build_integration_lut = shader_program::create_compute_pipeline(brdf_integration_compute);
    if (!check_creation(m_build_integration_lut.get(), "brdf integration compute shader program"))
        return;

    // creating a temporal command buffer for compute shader execution.
    command_buffer_ptr<min_key> compute_commands = command_buffer<min_key>::create(256);

    // build integration look up texture
    bind_shader_program_command* bsp = compute_commands->create<bind_shader_program_command>(command_keys::no_sort);
    bsp->shader_program_name         = m_build_integration_lut->get_name();

    // bind output lut
    bind_image_texture_command* bit = compute_commands->create<bind_image_texture_command>(command_keys::no_sort);
    bit->binding                    = 0;
    bit->texture_name               = m_brdf_integration_lut->get_name();
    bit->level                      = 0;
    bit->layered                    = false;
    bit->layer                      = 0;
    bit->access                     = base_access::write_only;
    bit->element_format             = format::rgba16f;

    // bind uniform
    glm::vec2 out                    = glm::vec2(m_brdf_integration_lut->get_width(), m_brdf_integration_lut->get_height());
    bind_single_uniform_command* bsu = compute_commands->create<bind_single_uniform_command>(command_keys::no_sort, sizeof(out));
    bsu->count                       = 1;
    bsu->location                    = 0;
    bsu->type                        = shader_resource_type::fvec2;
    bsu->uniform_value               = compute_commands->map_spare<bind_single_uniform_command>();
    memcpy(bsu->uniform_value, &out, sizeof(out));

    // execute compute
    dispatch_compute_command* dc = compute_commands->create<dispatch_compute_command>(command_keys::no_sort);
    dc->num_x_groups             = m_brdf_integration_lut->get_width() / 8;
    dc->num_y_groups             = m_brdf_integration_lut->get_height() / 8;
    dc->num_z_groups             = 1;

    add_memory_barrier_command* amb = compute_commands->create<add_memory_barrier_command>(command_keys::no_sort);
    amb->barrier_bit                = memory_barrier_bit::shader_image_access_barrier_bit;

    bsp                      = compute_commands->create<bind_shader_program_command>(command_keys::no_sort);
    bsp->shader_program_name = 0;

    {
        GL_NAMED_PROFILE_ZONE("Generating brdf lookup");
        compute_commands->execute();
    }
}

void light_stack::push(light_id id, mango_light* light)
{
    // push to correct stack
    light_entry le;
    le.light = light;
    le.id    = id;
    switch (light->model)
    {
    case light_model::directional:
        m_directional_stack.push_back(le);
        break;
    case light_model::atmosphere:
        m_atmosphere_stack.push_back(le);
        break;
    case light_model::skylight:
    {
        auto s = static_cast<skylight*>(light);
        if (!s->local)
            m_global_skylight = id;
        m_skylight_stack.push_back(le);
        break;
    }
    default:
        break;
    }
}

void light_stack::update()
{
    for (auto& c : m_light_cache)
        c.second.expired = true;

    m_current_shadow_casters.clear();

    m_lighting_dirty = m_global_skylight != m_last_skylight; // TODO Paul: Make this more clear!

    // order is important!
    update_directional_lights();
    update_atmosphere_lights();
    update_skylights();

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
    m_last_skylight   = m_global_skylight;
    m_global_skylight = invalid_light_id;
}

void light_stack::bind_light_buffers(const command_buffer_ptr<min_key>& global_binding_commands, gpu_buffer_ptr frame_uniform_buffer)
{
    // for now put everything in one buffer
    bind_buffer_command* bb = global_binding_commands->create<bind_buffer_command>(command_keys::no_sort);
    bb->target              = buffer_target::uniform_buffer;
    bb->index               = UB_SLOT_LIGHT_DATA;
    bb->size                = sizeof(m_light_buffer);
    bb->buffer_name         = frame_uniform_buffer->buffer_name();
    bb->offset              = frame_uniform_buffer->write_data(sizeof(light_buffer), &m_light_buffer);

    // reset
    m_light_buffer.directional_light.valid = false;
    m_light_buffer.skylight.valid          = false;
}

void light_stack::update_directional_lights()
{
    for (auto& d : m_directional_stack)
    {
        int64 checksum = calculate_checksum(reinterpret_cast<uint8*>(d.light), sizeof(directional_light));

        // find in light cache
        auto entry = m_light_cache.find(d.id);
        bool found = entry != m_light_cache.end();
        d.dirty    = !found || (entry->second.light_checksum != checksum);
        if (found)
            m_light_cache.at(d.id).expired = false;

        // create light cache entry if non existent or update it on change
        if (d.dirty)
        {
            if (found)
            {
                m_light_cache.at(d.id).light_checksum = checksum;
            }
            else
            {
                cache_entry new_entry;
                new_entry.light_checksum = checksum;
                new_entry.expired        = false;
                m_light_cache.insert({ d.id, new_entry });
            }
        }
    }

    if (m_directional_stack.empty())
        return;
    // atm there is only one directional light bound :D
    auto light                                    = static_cast<directional_light*>(m_directional_stack.back().light);
    m_light_buffer.directional_light.valid        = true;
    m_light_buffer.directional_light.direction    = light->direction;
    m_light_buffer.directional_light.color        = light->light_color;
    m_light_buffer.directional_light.intensity    = light->intensity;
    m_light_buffer.directional_light.cast_shadows = light->cast_shadows;
    if (light->cast_shadows)
        m_current_shadow_casters.push_back(light);
}

void light_stack::update_atmosphere_lights()
{
    // TODO Paul: TBD!
    for (auto& a : m_atmosphere_stack)
    {
        int64 checksum = calculate_checksum(reinterpret_cast<uint8*>(a.light), sizeof(atmosphere_light));

        // find in light cache
        auto entry = m_light_cache.find(a.id);
        bool found = entry != m_light_cache.end();
        a.dirty    = !found || (entry->second.light_checksum != checksum);
        if (found)
            m_light_cache.at(a.id).expired = false;

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

        */

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
    }
}

void light_stack::update_skylights()
{
    for (auto& s : m_skylight_stack)
    {
        int64 checksum = calculate_checksum(reinterpret_cast<uint8*>(s.light), sizeof(skylight));

        // find in light cache
        auto entry = m_light_cache.find(s.id);
        bool found = entry != m_light_cache.end();
        s.dirty    = !found || (entry->second.light_checksum != checksum);
        if (found)
            m_light_cache.at(s.id).expired = false;

        // if skylight captures (does not depend on hdr image) check dependencies
        // (dependencies are only atmosphere lights for now)

        auto light = static_cast<skylight*>(s.light);
        if (!light->use_texture)
        {
            for (auto& a : m_atmosphere_stack)
            {
                // m_skylight_builder.add_atmosphere_influence(static_cast<atmosphere_light*>(a.light));
                s.dirty |= a.dirty;
            }
            s.dirty |= m_skylight_builder.needs_rebuild();
        }

        // create light cache entry if non existent or update it on change
        if (s.dirty)
        {
            // recreate skylight cubemaps
            void* cached_data = m_allocator.allocate(sizeof(skylight_cache));
            MANGO_ASSERT(cached_data, "Light Stack Out Of Memory!");
            memset(cached_data, 0, sizeof(skylight_cache));
            m_skylight_builder.build(light, static_cast<skylight_cache*>(cached_data));

            if (found)
            {
                m_light_cache.at(s.id).light_checksum = checksum;
                m_allocator.free_memory(m_light_cache.at(s.id).data);
                m_light_cache.at(s.id).data = static_cast<skylight_cache*>(cached_data);
            }
            else
            {
                cache_entry new_entry;
                new_entry.light_checksum = checksum;
                new_entry.data           = static_cast<skylight_cache*>(cached_data);
                new_entry.expired        = false;
                m_light_cache.insert({ s.id, new_entry });
            }
        }
        m_lighting_dirty |= (m_global_skylight == s.id) && s.dirty;

        // atm there is only one skylight bound and ist has to be the global one :D
        if (!light->local)
        {
            m_light_buffer.skylight.valid     = true;
            m_light_buffer.skylight.intensity = light->intensity;
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
