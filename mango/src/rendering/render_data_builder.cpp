//! \file      render_data_builder.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <graphics/shader.hpp>
#include <graphics/shader_program.hpp>
#include <graphics/texture.hpp>
#include <mango/profile.hpp>
#include <rendering/render_data_builder.hpp>
#include <util/helpers.hpp>

using namespace mango;

bool skylight_builder::init()
{
    // compute shader to convert from equirectangular projected hdr textures to a cube map.
    shader_configuration shader_config;
    shader_config.path         = "res/shader/c_equi_to_cubemap.glsl";
    shader_config.type         = shader_type::compute_shader;
    shader_ptr to_cube_compute = shader::create(shader_config);
    if (!check_creation(to_cube_compute.get(), "cubemap compute shader"))
        return false;

    m_equi_to_cubemap = shader_program::create_compute_pipeline(to_cube_compute);
    if (!check_creation(m_equi_to_cubemap.get(), "cubemap compute shader program"))
        return false;

    // compute shader to create a cube map with atmospheric scattering.
    shader_config.path                     = "res/shader/atmospheric_scattering/c_atmospheric_scattering_cubemap.glsl";
    shader_config.type                     = shader_type::compute_shader;
    shader_ptr atmospheric_cubemap_compute = shader::create(shader_config);
    if (!check_creation(atmospheric_cubemap_compute.get(), "atmospheric scattering cubemap compute shader"))
        return false;

    m_atmospheric_cubemap = shader_program::create_compute_pipeline(atmospheric_cubemap_compute);
    if (!check_creation(m_atmospheric_cubemap.get(), "atmospheric scattering cubemap compute shader program"))
        return false;

    // compute shader to build the irradiance cubemap for image based lighting.
    shader_config.path                = "res/shader/pbr_compute/c_irradiance_map.glsl";
    shader_config.type                = shader_type::compute_shader;
    shader_ptr irradiance_map_compute = shader::create(shader_config);
    if (!check_creation(irradiance_map_compute.get(), "irradiance map compute shader"))
        return false;

    m_build_irradiance_map = shader_program::create_compute_pipeline(irradiance_map_compute);
    if (!check_creation(m_build_irradiance_map.get(), "irradiance map compute shader program"))
        return false;

    // compute shader to build the prefiltered specular cubemap for image based lighting.
    shader_config.path                          = "res/shader/pbr_compute/c_prefilter_specular_map.glsl";
    shader_config.type                          = shader_type::compute_shader;
    shader_ptr specular_prefiltered_map_compute = shader::create(shader_config);
    if (!check_creation(specular_prefiltered_map_compute.get(), "prefilter specular cubemap compute shader"))
        return false;

    m_build_specular_prefiltered_map = shader_program::create_compute_pipeline(specular_prefiltered_map_compute);
    if (!check_creation(m_build_specular_prefiltered_map.get(), "prefilter specular cubemap compute shader program"))
        return false;
    return true;
}

bool skylight_builder::needs_rebuild()
{
    if (old_dependencies.size() != new_dependencies.size())
        return true;

    // order change is important here
    for (int32 i = 0; i < static_cast<int32>(old_dependencies.size()); ++i)
    {
        if (old_dependencies.at(i) != new_dependencies.at(i))
            return true;
    }
    return false;
}
/*
void skylight_builder::add_atmosphere_influence(atmosphere_light* light)
{
    new_dependencies.push_back(light);
}
*/
void skylight_builder::build(skylight* light, skylight_cache* render_data)
{
    PROFILE_ZONE;
    old_dependencies = new_dependencies;
    new_dependencies.clear();

    command_buffer_ptr<min_key> compute_commands = command_buffer<min_key>::create(4096);

    // HDR Texture
    if (light->use_texture)
    {
        if (!light->hdr_texture)
        {
            clear(render_data);
            return;
        }
        load_from_hdr(compute_commands, light, render_data);
    }
    else // TODO Paul: capture ... will be done soon....
    {
        // capture(compute_commands, render_data);
    }

    {
        GL_NAMED_PROFILE_ZONE("Generating IBL");
        compute_commands->execute();
    }
}
/*
void skylight_builder::capture(const command_buffer_ptr<min_key>& compute_commands, skylight_cache* render_data)
{
    PROFILE_ZONE;
    texture_configuration texture_config;
    texture_config.generate_mipmaps        = calculate_mip_count(global_cubemap_size, global_cubemap_size);
    texture_config.is_standard_color_space = false;
    texture_config.is_cubemap              = true;
    texture_config.texture_min_filter      = texture_parameter::filter_linear_mipmap_linear;
    texture_config.texture_mag_filter      = texture_parameter::filter_linear;
    texture_config.texture_wrap_s          = texture_parameter::wrap_clamp_to_edge;
    texture_config.texture_wrap_t          = texture_parameter::wrap_clamp_to_edge;

    if (render_data->cubemap)
        render_data->cubemap->release();
    render_data->cubemap = texture::create(texture_config);
    if (!check_creation(render_data->cubemap.get(), "environment cubemap texture"))
        return;

    render_data->cubemap->set_data(format::rgba16f, global_cubemap_size, global_cubemap_size, format::rgba, format::t_float, nullptr);

    // creating a temporal command buffer for compute shader execution.

    // capturing shader (we can try to do it in one call like with the shadows)
    // bind_shader_program_command* bsp = compute_commands->create<bind_shader_program_command>(command_keys::no_sort);
    // bsp->shader_program_name         = m_capture_scene->get_name();

    // bind output cubemap
    bind_image_texture_command* bit = compute_commands->create<bind_image_texture_command>(command_keys::no_sort);
    bit->binding                    = 1;
    bit->texture_name               = render_data->cubemap->get_name();
    bit->level                      = 0;
    bit->layered                    = true;
    bit->layer                      = 0;
    bit->access                     = base_access::write_only;
    bit->element_format             = format::rgba16f;

    // bind uniforms
    glm::vec2 out                    = glm::vec2(render_data->cubemap->get_width(), render_data->cubemap->get_height());
    bind_single_uniform_command* bsu = compute_commands->create<bind_single_uniform_command>(command_keys::no_sort, sizeof(out));
    bsu->count                       = 1;
    bsu->location                    = 1;
    bsu->type                        = shader_resource_type::fvec2;
    bsu->uniform_value               = compute_commands->map_spare<bind_single_uniform_command>();
    memcpy(bsu->uniform_value, &out, sizeof(out));

    // execute draw calls
    if (m_draw_commands)
    {
        m_draw_commands->execute();
        m_draw_commands->invalidate();
    }

    // We need to recalculate mipmaps
    calculate_mipmaps_command* cm = compute_commands->create<calculate_mipmaps_command>(command_keys::no_sort);
    cm->texture_name              = render_data->cubemap->get_name();

    add_memory_barrier_command* amb = compute_commands->create<add_memory_barrier_command>(command_keys::no_sort);
    amb->barrier_bit                = memory_barrier_bit::shader_image_access_barrier_bit;

    calculate_ibl_maps(compute_commands, render_data);
}
*/
void skylight_builder::load_from_hdr(const command_buffer_ptr<min_key>& compute_commands, skylight* light, skylight_cache* render_data)
{
    PROFILE_ZONE;
    texture_configuration texture_config;
    texture_config.generate_mipmaps        = calculate_mip_count(global_cubemap_size, global_cubemap_size);
    texture_config.is_standard_color_space = false;
    texture_config.is_cubemap              = true;
    texture_config.texture_min_filter      = texture_parameter::filter_linear_mipmap_linear;
    texture_config.texture_mag_filter      = texture_parameter::filter_linear;
    texture_config.texture_wrap_s          = texture_parameter::wrap_clamp_to_edge;
    texture_config.texture_wrap_t          = texture_parameter::wrap_clamp_to_edge;

    if (render_data->cubemap)
        render_data->cubemap->release();
    render_data->cubemap = texture::create(texture_config);
    if (!check_creation(render_data->cubemap.get(), "environment cubemap texture"))
        return;

    render_data->cubemap->set_data(format::rgba16f, global_cubemap_size, global_cubemap_size, format::rgba, format::t_float, nullptr);

    // creating a temporal command buffer for compute shader execution.

    // equirectangular to cubemap
    bind_shader_program_command* bsp = compute_commands->create<bind_shader_program_command>(command_keys::no_sort);
    bsp->shader_program_name         = m_equi_to_cubemap->get_name();

    // bind input hdr texture
    bind_texture_command* bt = compute_commands->create<bind_texture_command>(command_keys::no_sort);
    bt->binding              = 0;
    bt->sampler_location     = 0;
    bt->texture_name         = light->hdr_texture->get_name();

    // bind output cubemap
    bind_image_texture_command* bit = compute_commands->create<bind_image_texture_command>(command_keys::no_sort);
    bit->binding                    = 1;
    bit->texture_name               = render_data->cubemap->get_name();
    bit->level                      = 0;
    bit->layered                    = true;
    bit->layer                      = 0;
    bit->access                     = base_access::write_only;
    bit->element_format             = format::rgba16f;

    // bind uniforms
    glm::vec2 out                    = glm::vec2(render_data->cubemap->get_width(), render_data->cubemap->get_height());
    bind_single_uniform_command* bsu = compute_commands->create<bind_single_uniform_command>(command_keys::no_sort, sizeof(out));
    bsu->count                       = 1;
    bsu->location                    = 1;
    bsu->type                        = shader_resource_type::fvec2;
    bsu->uniform_value               = compute_commands->map_spare<bind_single_uniform_command>();
    memcpy(bsu->uniform_value, &out, sizeof(out));

    // execute compute
    dispatch_compute_command* dp = compute_commands->create<dispatch_compute_command>(command_keys::no_sort);
    dp->num_x_groups             = render_data->cubemap->get_width() / 32;
    dp->num_y_groups             = render_data->cubemap->get_height() / 32;
    dp->num_z_groups             = 6;

    // We need to recalculate mipmaps
    calculate_mipmaps_command* cm = compute_commands->create<calculate_mipmaps_command>(command_keys::no_sort);
    cm->texture_name              = render_data->cubemap->get_name();

    add_memory_barrier_command* amb = compute_commands->create<add_memory_barrier_command>(command_keys::no_sort);
    amb->barrier_bit                = memory_barrier_bit::shader_image_access_barrier_bit;

    calculate_ibl_maps(compute_commands, render_data);
}

void skylight_builder::calculate_ibl_maps(const command_buffer_ptr<min_key>& compute_commands, skylight_cache* render_data)
{
    if (!render_data->cubemap)
    {
        MANGO_LOG_ERROR("Can not calculate cubemap maps without a cubemap!");
        return; // Should not be possible;
    }

    texture_configuration texture_config;
    texture_config.generate_mipmaps        = calculate_mip_count(global_specular_convolution_map_size, global_specular_convolution_map_size);
    texture_config.is_standard_color_space = false;
    texture_config.is_cubemap              = true;
    texture_config.texture_min_filter      = texture_parameter::filter_linear_mipmap_linear;
    texture_config.texture_mag_filter      = texture_parameter::filter_linear;
    texture_config.texture_wrap_s          = texture_parameter::wrap_clamp_to_edge;
    texture_config.texture_wrap_t          = texture_parameter::wrap_clamp_to_edge;

    if (render_data->specular_prefiltered_cubemap)
        render_data->specular_prefiltered_cubemap->release();
    render_data->specular_prefiltered_cubemap = texture::create(texture_config);
    if (!check_creation(render_data->specular_prefiltered_cubemap.get(), "prefiltered specular texture"))
        return;

    render_data->specular_prefiltered_cubemap->set_data(format::rgba16f, global_specular_convolution_map_size, global_specular_convolution_map_size, format::rgba, format::t_float, nullptr);

    texture_config.generate_mipmaps   = 1;
    texture_config.texture_min_filter = texture_parameter::filter_linear;
    if (render_data->irradiance_cubemap)
        render_data->irradiance_cubemap->release();
    render_data->irradiance_cubemap = texture::create(texture_config);
    if (!check_creation(render_data->irradiance_cubemap.get(), "irradiance texture"))
        return;

    render_data->irradiance_cubemap->set_data(format::rgba16f, global_irradiance_map_size, global_irradiance_map_size, format::rgba, format::t_float, nullptr);

    // build irradiance map
    bind_shader_program_command* bsp = compute_commands->create<bind_shader_program_command>(command_keys::no_sort);
    bsp->shader_program_name         = m_build_irradiance_map->get_name();

    // bind input cubemap
    bind_texture_command* bt = compute_commands->create<bind_texture_command>(command_keys::no_sort);
    bt->binding              = 0;
    bt->sampler_location     = 0;
    bt->texture_name         = render_data->cubemap->get_name();

    // bind output irradiance map
    bind_image_texture_command* bit = compute_commands->create<bind_image_texture_command>(command_keys::no_sort);
    bit->binding                    = 1;
    bit->texture_name               = render_data->irradiance_cubemap->get_name();
    bit->level                      = 0;
    bit->layered                    = true;
    bit->layer                      = 0;
    bit->access                     = base_access::write_only;
    bit->element_format             = format::rgba16f;

    // bind uniform
    glm::vec2 out                    = glm::vec2(render_data->irradiance_cubemap->get_width(), render_data->irradiance_cubemap->get_height());
    bind_single_uniform_command* bsu = compute_commands->create<bind_single_uniform_command>(command_keys::no_sort, sizeof(out));
    bsu->count                       = 1;
    bsu->location                    = 1;
    bsu->type                        = shader_resource_type::fvec2;
    bsu->uniform_value               = compute_commands->map_spare<bind_single_uniform_command>();
    memcpy(bsu->uniform_value, &out, sizeof(out));

    // execute compute
    dispatch_compute_command* dp = compute_commands->create<dispatch_compute_command>(command_keys::no_sort);
    dp->num_x_groups             = render_data->irradiance_cubemap->get_width() / 32;
    dp->num_y_groups             = render_data->irradiance_cubemap->get_height() / 32;
    dp->num_z_groups             = 6;

    add_memory_barrier_command* amb = compute_commands->create<add_memory_barrier_command>(command_keys::no_sort);
    amb->barrier_bit                = memory_barrier_bit::shader_image_access_barrier_bit;

    // build prefiltered specular mipchain
    bsp                      = compute_commands->create<bind_shader_program_command>(command_keys::no_sort);
    bsp->shader_program_name = m_build_specular_prefiltered_map->get_name();

    // bind input cubemap // TODO Paul: Needed again?
    bt                   = compute_commands->create<bind_texture_command>(command_keys::no_sort);
    bt->binding          = 0;
    bt->sampler_location = 0;
    bt->texture_name     = render_data->cubemap->get_name();

    uint32 mip_count = render_data->specular_prefiltered_cubemap->mipmaps();
    for (uint32 mip = 0; mip < mip_count; ++mip)
    {
        const uint32 mipmap_width  = global_specular_convolution_map_size >> mip;
        const uint32 mipmap_height = global_specular_convolution_map_size >> mip;
        float roughness            = (float)mip / (float)(mip_count - 1);

        // bind correct mipmap
        bit                 = compute_commands->create<bind_image_texture_command>(command_keys::no_sort);
        bit->binding        = 1;
        bit->texture_name   = render_data->specular_prefiltered_cubemap->get_name();
        bit->level          = static_cast<g_int>(mip);
        bit->layered        = true;
        bit->layer          = 0;
        bit->access         = base_access::write_only;
        bit->element_format = format::rgba16f;

        out                = glm::vec2(mipmap_width, mipmap_height);
        bsu                = compute_commands->create<bind_single_uniform_command>(command_keys::no_sort, sizeof(out));
        bsu->count         = 1;
        bsu->location      = 1;
        bsu->type          = shader_resource_type::fvec2;
        bsu->uniform_value = compute_commands->map_spare<bind_single_uniform_command>();
        memcpy(bsu->uniform_value, &out, sizeof(out));

        bsu                = compute_commands->create<bind_single_uniform_command>(command_keys::no_sort, sizeof(out));
        bsu->count         = 1;
        bsu->location      = 2;
        bsu->type          = shader_resource_type::fsingle;
        bsu->uniform_value = compute_commands->map_spare<bind_single_uniform_command>();
        memcpy(bsu->uniform_value, &roughness, sizeof(roughness));

        dp               = compute_commands->create<dispatch_compute_command>(command_keys::no_sort);
        dp->num_x_groups = render_data->specular_prefiltered_cubemap->get_width() / 32;
        dp->num_y_groups = render_data->specular_prefiltered_cubemap->get_height() / 32;
        dp->num_z_groups = 6;
    }

    bsp                      = compute_commands->create<bind_shader_program_command>(command_keys::no_sort);
    bsp->shader_program_name = 0;

    amb              = compute_commands->create<add_memory_barrier_command>(command_keys::no_sort);
    amb->barrier_bit = memory_barrier_bit::shader_image_access_barrier_bit;
}

void skylight_builder::clear(skylight_cache* render_data)
{
    PROFILE_ZONE;
    render_data->cubemap                      = nullptr;
    render_data->irradiance_cubemap           = nullptr;
    render_data->specular_prefiltered_cubemap = nullptr;
    return;
}
/*
bool atmosphere_builder::init()
{
    return false;
}

bool atmosphere_builder::needs_rebuild()
{
    return false;
}

void atmosphere_builder::build(atmosphere_light* light, atmosphere_cache* render_data)
{
    MANGO_UNUSED(light);
    MANGO_UNUSED(render_data);
}

// buffer_ptr m_atmosphere_data_buffer;
// struct atmosphere_ub_data
// {
//     std140_vec3 sun_dir;
//     std140_vec3 rayleigh_scattering_coefficients;
//     std140_vec3 ray_origin;
//     std140_vec2 density_multiplier;
//     std140_float sun_intensity;
//     std140_float mie_scattering_coefficient;
//     std140_float ground_radius;
//     std140_float atmosphere_radius;
//     std140_float mie_preferred_scattering_dir;
//     std140_int scatter_points;
//     std140_int scatter_points_second_ray;
// };
// atmosphere_ub_data* m_atmosphere_data_mapping;

// void cubemap_step::create_with_atmosphere(const mango::environment_light_data* el_data)
// {
//     PROFILE_ZONE;
//     m_atmosphere_data_mapping->sun_dir                          = glm::normalize(el_data->sun_data.direction);
//     m_atmosphere_data_mapping->sun_intensity                    = el_data->sun_data.intensity * 0.0025f;
//     m_atmosphere_data_mapping->scatter_points                   = el_data->scatter_points;
//     m_atmosphere_data_mapping->scatter_points_second_ray        = el_data->scatter_points_second_ray;
//     m_atmosphere_data_mapping->rayleigh_scattering_coefficients = el_data->rayleigh_scattering_coefficients;
//     m_atmosphere_data_mapping->mie_scattering_coefficient       = el_data->mie_scattering_coefficient;
//     m_atmosphere_data_mapping->density_multiplier               = el_data->density_multiplier;
//     m_atmosphere_data_mapping->ground_radius                    = el_data->ground_radius;
//     m_atmosphere_data_mapping->atmosphere_radius                = el_data->atmosphere_radius;
//     m_atmosphere_data_mapping->ray_origin                       = glm::vec3(0.0f, el_data->ground_radius + el_data->view_height, 0.0f);
//     m_atmosphere_data_mapping->mie_preferred_scattering_dir     = el_data->mie_preferred_scattering_dir;
//
//     texture_configuration texture_config;
//     texture_config.generate_mipmaps        = calculate_mip_count(m_cube_width, m_cube_height);
//     texture_config.is_standard_color_space = false;
//     texture_config.is_cubemap              = true;
//     texture_config.texture_min_filter      = texture_parameter::filter_linear_mipmap_linear;
//     texture_config.texture_mag_filter      = texture_parameter::filter_linear;
//     texture_config.texture_wrap_s          = texture_parameter::wrap_clamp_to_edge;
//     texture_config.texture_wrap_t          = texture_parameter::wrap_clamp_to_edge;
//
//     if (m_cubemap)
//         m_cubemap->release();
//     m_cubemap = texture::create(texture_config);
//     if (!check_creation(m_cubemap.get(), "environment cubemap texture"))
//         return;
//
//     m_cubemap->set_data(format::rgba16f, m_cube_width, m_cube_height, format::rgba, format::t_float, nullptr);
//
//     // creating a temporal command buffer for compute shader execution.
//     command_buffer_ptr<min_key> compute_commands = command_buffer<min_key>::create(4096);
//
//     // atmospheric scattering to cubemap
//     bind_shader_program_command* bsp = compute_commands->create<bind_shader_program_command>(command_keys::no_sort);
//     bsp->shader_program_name         = m_atmospheric_cubemap->get_name();
//
//     // bind output cubemap
//     bind_image_texture_command* bit = compute_commands->create<bind_image_texture_command>(command_keys::no_sort);
//     bit->binding                    = 0;
//     bit->texture_name               = m_cubemap->get_name();
//     bit->level                      = 0;
//     bit->layered                    = true;
//     bit->layer                      = 0;
//     bit->access                     = base_access::write_only;
//     bit->element_format             = format::rgba16f;
//
//     // bind uniforms
//     glm::vec2 out                    = glm::vec2(m_cubemap->get_width(), m_cubemap->get_height());
//     bind_single_uniform_command* bsu = compute_commands->create<bind_single_uniform_command>(command_keys::no_sort, sizeof(out));
//     bsu->count                       = 1;
//     bsu->location                    = 0;
//     bsu->type                        = shader_resource_type::fvec2;
//     bsu->uniform_value               = compute_commands->map_spare<bind_single_uniform_command>();
//     memcpy(bsu->uniform_value, &out, sizeof(out));
//
//     bind_buffer_command* bb = compute_commands->create<bind_buffer_command>(command_keys::no_sort);
//     bb->index               = UB_SLOT_COMPUTE_DATA;
//     bb->buffer_name         = m_atmosphere_data_buffer->get_name();
//     bb->offset              = 0;
//     bb->target              = buffer_target::uniform_buffer;
//     bb->size                = m_atmosphere_data_buffer->byte_length();
//
//     // execute compute
//     dispatch_compute_command* dp = compute_commands->create<dispatch_compute_command>(command_keys::no_sort);
//     dp->num_x_groups             = m_cubemap->get_width() / 32;
//     dp->num_y_groups             = m_cubemap->get_height() / 32;
//     dp->num_z_groups             = 6;
//
//     // We need to recalculate mipmaps
//     calculate_mipmaps_command* cm = compute_commands->create<calculate_mipmaps_command>(command_keys::no_sort);
//     cm->texture_name              = m_cubemap->get_name();
//
//     add_memory_barrier_command* amb = compute_commands->create<add_memory_barrier_command>(command_keys::no_sort);
//     amb->barrier_bit                = memory_barrier_bit::shader_image_access_barrier_bit;
//
//     calculate_ibl_maps(compute_commands);
//
//     {
//         GL_NAMED_PROFILE_ZONE("Generating IBL");
//         compute_commands->execute();
//     }
// }
*/
