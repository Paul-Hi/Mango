//! \file      ibl_step.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <graphics/buffer.hpp>
#include <graphics/shader.hpp>
#include <graphics/shader_program.hpp>
#include <graphics/texture.hpp>
#include <graphics/vertex_array.hpp>
#include <mango/imgui_helper.hpp>
#include <mango/profile.hpp>
#include <mango/render_system.hpp>
#include <mango/scene_ecs.hpp>
#include <rendering/steps/ibl_step.hpp>
#include <util/helpers.hpp>

using namespace mango;

static const float cubemap_vertices[36] = { -1.0f, 1.0f, 1.0f,  1.0f, 1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f,
                                            -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f, -1.0f, 1.0f };

static const uint8 cubemap_indices[18] = { 8, 9, 0, 2, 1, 3, 3, 2, 5, 4, 7, 6, 6, 0, 7, 1, 10, 11 };

bool ibl_step::create()
{
    PROFILE_ZONE;

    bool success = true;
    success      = success & setup_buffers();
    success      = success & setup_shader_programs();

    if (success)
    {
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

    return success;
}

bool ibl_step::setup_shader_programs()
{
    PROFILE_ZONE;

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

    m_equi_to_cubemap = shader_program::create_compute_pipeline(to_cube_compute);
    if (!check_creation(m_equi_to_cubemap.get(), "cubemap compute shader program"))
        return false;

    // compute shader to create a cube map with atmospheric scattering.
    shader_config.path                     = "res/shader/c_atmospheric_scattering_cubemap.glsl";
    shader_config.type                     = shader_type::compute_shader;
    shader_ptr atmospheric_cubemap_compute = shader::create(shader_config);
    if (!check_creation(atmospheric_cubemap_compute.get(), "atmospheric scattering cubemap compute shader"))
        return false;

    m_atmospheric_cubemap = shader_program::create_compute_pipeline(atmospheric_cubemap_compute);
    if (!check_creation(m_atmospheric_cubemap.get(), "atmospheric scattering cubemap compute shader program"))
        return false;

    // compute shader to build the irradiance cubemap for image based lighting.
    shader_config.path                = "res/shader/c_irradiance_map.glsl";
    shader_config.type                = shader_type::compute_shader;
    shader_ptr irradiance_map_compute = shader::create(shader_config);
    if (!check_creation(irradiance_map_compute.get(), "irradiance map compute shader"))
        return false;

    m_build_irradiance_map = shader_program::create_compute_pipeline(irradiance_map_compute);
    if (!check_creation(m_build_irradiance_map.get(), "irradiance map compute shader program"))
        return false;

    // compute shader to build the prefiltered specular cubemap for image based lighting.
    shader_config.path                          = "res/shader/c_prefilter_specular_map.glsl";
    shader_config.type                          = shader_type::compute_shader;
    shader_ptr specular_prefiltered_map_compute = shader::create(shader_config);
    if (!check_creation(specular_prefiltered_map_compute.get(), "prefilter specular ibl compute shader"))
        return false;

    m_build_specular_prefiltered_map = shader_program::create_compute_pipeline(specular_prefiltered_map_compute);
    if (!check_creation(m_build_specular_prefiltered_map.get(), "prefilter specular ibl compute shader program"))
        return false;

    // compute shader to build the brdf integration lookup texture for image based lighting. Could be done only once.
    shader_config.path                  = "res/shader/c_brdf_integration.glsl";
    shader_config.type                  = shader_type::compute_shader;
    shader_ptr brdf_integration_compute = shader::create(shader_config);
    if (!check_creation(brdf_integration_compute.get(), "ibl brdf integration compute shader"))
        return false;

    m_build_integration_lut = shader_program::create_compute_pipeline(brdf_integration_compute);
    if (!check_creation(m_build_integration_lut.get(), "ibl brdf integration compute shader program"))
        return false;

    // cubemap rendering
    shader_config.path        = "res/shader/v_cubemap.glsl";
    shader_config.type        = shader_type::vertex_shader;
    shader_ptr cubemap_vertex = shader::create(shader_config);
    if (!check_creation(cubemap_vertex.get(), "cubemap vertex shader"))
        return false;

    shader_config.path          = "res/shader/f_cubemap.glsl";
    shader_config.type          = shader_type::fragment_shader;
    shader_ptr cubemap_fragment = shader::create(shader_config);
    if (!check_creation(cubemap_fragment.get(), "cubemap fragment shader"))
        return false;

    m_draw_environment = shader_program::create_graphics_pipeline(cubemap_vertex, nullptr, nullptr, nullptr, cubemap_fragment);
    if (!check_creation(m_draw_environment.get(), "cubemap rendering shader program"))
        return false;

    return true;
}

bool ibl_step::setup_buffers()
{
    PROFILE_ZONE;

    m_ibl_command_buffer = command_buffer<min_key>::create(512);

    buffer_configuration b_config;
    b_config.access          = buffer_access::mapped_access_read_write;
    b_config.size            = sizeof(atmosphere_ub_data);
    b_config.target          = buffer_target::shader_storage_buffer;
    m_atmosphere_data_buffer = buffer::create(b_config);

    m_atmosphere_data_mapping = static_cast<atmosphere_ub_data*>(m_atmosphere_data_buffer->map(0, b_config.size, buffer_access::mapped_access_write));
    if (!check_mapping(m_atmosphere_data_mapping, "atmosphere compute shader data"))
        return false;

    m_cube_geometry = vertex_array::create();
    if (!check_creation(m_cube_geometry.get(), "cubemap geometry vertex array"))
        return false;

    b_config.access     = buffer_access::none;
    b_config.size       = sizeof(cubemap_vertices);
    b_config.target     = buffer_target::vertex_buffer;
    const void* vb_data = static_cast<const void*>(cubemap_vertices);
    b_config.data       = vb_data;
    buffer_ptr vb       = buffer::create(b_config);

    m_cube_geometry->bind_vertex_buffer(0, vb, 0, sizeof(float) * 3);
    m_cube_geometry->set_vertex_attribute(0, 0, format::rgb32f, 0);

    b_config.size       = sizeof(cubemap_indices);
    b_config.target     = buffer_target::index_buffer;
    const void* ib_data = static_cast<const void*>(cubemap_indices);
    b_config.data       = ib_data;
    buffer_ptr ib       = buffer::create(b_config);

    m_cube_geometry->bind_index_buffer(ib);

    m_ibl_data.current_rotation_scale = glm::mat3(1.0f);
    m_ibl_data.render_level           = 0.0f;

    texture_configuration texture_config;
    texture_config.generate_mipmaps        = 1;
    texture_config.is_standard_color_space = false;
    texture_config.is_cubemap              = false;
    texture_config.texture_min_filter      = texture_parameter::filter_linear;
    texture_config.texture_mag_filter      = texture_parameter::filter_linear;
    texture_config.texture_wrap_s          = texture_parameter::wrap_clamp_to_edge;
    texture_config.texture_wrap_t          = texture_parameter::wrap_clamp_to_edge;
    m_brdf_integration_lut                 = texture::create(texture_config);
    if (!check_creation(m_brdf_integration_lut.get(), "brdf integration look up texture"))
        return false;

    m_brdf_integration_lut->set_data(format::rgba16f, m_integration_lut_width, m_integration_lut_height, format::rgba, format::t_float, nullptr);

    // default texture needed
    texture_config.texture_min_filter = texture_parameter::filter_nearest;
    texture_config.texture_mag_filter = texture_parameter::filter_nearest;
    texture_config.is_cubemap         = true;
    m_default_ibl_texture             = texture::create(texture_config);
    if (!check_creation(m_default_ibl_texture.get(), "default ibl texture"))
        return false;

    g_ubyte albedo[4] = { 1, 1, 1, 255 };
    m_default_ibl_texture->set_data(format::rgba8, 1, 1, format::rgba, format::t_unsigned_byte, albedo);

    return true;
}

void ibl_step::update(float dt)
{
    MANGO_UNUSED(dt);
}

void ibl_step::attach() {}

void ibl_step::configure(const ibl_step_configuration& configuration)
{
    m_ibl_data.render_level = configuration.get_render_level();
    MANGO_ASSERT(m_ibl_data.render_level > 0.0f && m_ibl_data.render_level < 8.1f, "Shadow Map Resolution has to be between 0.0 and 8.0f!");
}

void ibl_step::execute(gpu_buffer_ptr frame_uniform_buffer)
{
    PROFILE_ZONE;
    if (m_ibl_data.render_level < 0.0f)
        return;

    set_depth_test_command* sdt = m_ibl_command_buffer->create<set_depth_test_command>(command_keys::no_sort);
    sdt->enabled                = true;
    set_depth_func_command* sdf = m_ibl_command_buffer->create<set_depth_func_command>(command_keys::no_sort);
    sdf->operation              = compare_operation::less_equal;

    set_cull_face_command* scf = m_ibl_command_buffer->create<set_cull_face_command>(command_keys::no_sort);
    scf->face                  = polygon_face::face_front;

    set_polygon_mode_command* spm = m_ibl_command_buffer->create<set_polygon_mode_command>(command_keys::no_sort);
    spm->face                     = polygon_face::face_front_and_back;
    spm->mode                     = polygon_mode::fill;

    set_blending_command* bl = m_ibl_command_buffer->create<set_blending_command>(command_keys::no_sort);
    bl->enabled              = false;

    bind_shader_program_command* bsp = m_ibl_command_buffer->create<bind_shader_program_command>(command_keys::no_sort);
    bsp->shader_program_name         = m_draw_environment->get_name();

    bind_vertex_array_command* bva = m_ibl_command_buffer->create<bind_vertex_array_command>(command_keys::no_sort);
    bva->vertex_array_name         = m_cube_geometry->get_name();

    bind_buffer_command* bb = m_ibl_command_buffer->create<bind_buffer_command>(command_keys::no_sort);
    bb->target              = buffer_target::uniform_buffer;
    bb->index               = UB_SLOT_IBL_DATA;
    bb->size                = sizeof(ibl_data);
    bb->buffer_name         = frame_uniform_buffer->buffer_name();
    bb->offset              = frame_uniform_buffer->write_data(sizeof(ibl_data), &m_ibl_data);

    bind_texture_command* bt = m_ibl_command_buffer->create<bind_texture_command>(command_keys::no_sort);
    bt->binding              = 0;
    bt->sampler_location     = 0;
    if (m_prefiltered_specular)
        bt->texture_name = m_prefiltered_specular->get_name();
    else
        bt->texture_name = m_default_ibl_texture->get_name();

    draw_elements_command* de = m_ibl_command_buffer->create<draw_elements_command>(command_keys::no_sort);
    de->topology              = primitive_topology::triangle_strip;
    de->first                 = 0;
    de->count                 = 18;
    de->type                  = index_type::ubyte;
    de->instance_count        = 1;

#ifdef MANGO_DEBUG
    bva                    = m_ibl_command_buffer->create<bind_vertex_array_command>(command_keys::no_sort);
    bva->vertex_array_name = 0;

    bsp                      = m_ibl_command_buffer->create<bind_shader_program_command>(command_keys::no_sort);
    bsp->shader_program_name = 0;
#endif // MANGO_DEBUG
}

void ibl_step::destroy() {}

void ibl_step::clear()
{
    PROFILE_ZONE;
    m_cubemap              = nullptr;
    m_irradiance_map       = nullptr;
    m_prefiltered_specular = nullptr;
    return;
}

void ibl_step::create_image_based_light_data(const mango::environment_light_data* el_data)
{
    if (!el_data->create_atmosphere)
    {
        if (el_data->hdr_texture)
            load_from_hdr(el_data);
        else
            clear();
    }
    else
    {
        create_with_atmosphere(el_data);
    }
}

void ibl_step::load_from_hdr(const mango::environment_light_data* el_data)
{
    PROFILE_ZONE;
    if (!el_data->hdr_texture)
        return;
    texture_configuration texture_config;
    texture_config.generate_mipmaps        = calculate_mip_count(m_cube_width, m_cube_height);
    texture_config.is_standard_color_space = false;
    texture_config.is_cubemap              = true;
    texture_config.texture_min_filter      = texture_parameter::filter_linear_mipmap_linear;
    texture_config.texture_mag_filter      = texture_parameter::filter_linear;
    texture_config.texture_wrap_s          = texture_parameter::wrap_clamp_to_edge;
    texture_config.texture_wrap_t          = texture_parameter::wrap_clamp_to_edge;

    if (m_cubemap)
        m_cubemap->release();
    m_cubemap = texture::create(texture_config);
    if (!check_creation(m_cubemap.get(), "environment cubemap texture"))
        return;

    m_cubemap->set_data(format::rgba16f, m_cube_width, m_cube_height, format::rgba, format::t_float, nullptr);

    // creating a temporal command buffer for compute shader execution.
    command_buffer_ptr<min_key> compute_commands = command_buffer<min_key>::create(4096);

    // equirectangular to cubemap
    bind_shader_program_command* bsp = compute_commands->create<bind_shader_program_command>(command_keys::no_sort);
    bsp->shader_program_name         = m_equi_to_cubemap->get_name();

    // bind input hdr texture
    bind_texture_command* bt = compute_commands->create<bind_texture_command>(command_keys::no_sort);
    bt->binding              = 0;
    bt->sampler_location     = 0;
    bt->texture_name         = el_data->hdr_texture->get_name();

    // bind output cubemap
    bind_image_texture_command* bit = compute_commands->create<bind_image_texture_command>(command_keys::no_sort);
    bit->binding                    = 1;
    bit->texture_name               = m_cubemap->get_name();
    bit->level                      = 0;
    bit->layered                    = true;
    bit->layer                      = 0;
    bit->access                     = base_access::write_only;
    bit->element_format             = format::rgba16f;

    // bind uniforms
    glm::vec2 out                    = glm::vec2(m_cubemap->get_width(), m_cubemap->get_height());
    bind_single_uniform_command* bsu = compute_commands->create<bind_single_uniform_command>(command_keys::no_sort, sizeof(out));
    bsu->count                       = 1;
    bsu->location                    = 1;
    bsu->type                        = shader_resource_type::fvec2;
    bsu->uniform_value               = compute_commands->map_spare<bind_single_uniform_command>();
    memcpy(bsu->uniform_value, &out, sizeof(out));

    // execute compute
    dispatch_compute_command* dp = compute_commands->create<dispatch_compute_command>(command_keys::no_sort);
    dp->num_x_groups             = m_cubemap->get_width() / 32;
    dp->num_y_groups             = m_cubemap->get_height() / 32;
    dp->num_z_groups             = 6;

    // We need to recalculate mipmaps
    calculate_mipmaps_command* cm = compute_commands->create<calculate_mipmaps_command>(command_keys::no_sort);
    cm->texture_name              = m_cubemap->get_name();

    add_memory_barrier_command* amb = compute_commands->create<add_memory_barrier_command>(command_keys::no_sort);
    amb->barrier_bit                = memory_barrier_bit::shader_image_access_barrier_bit;

    calculate_ibl_maps(compute_commands);

    {
        GL_NAMED_PROFILE_ZONE("Generating IBL");
        compute_commands->execute();
    }
}

void ibl_step::create_with_atmosphere(const mango::environment_light_data* el_data)
{
    PROFILE_ZONE;
    m_atmosphere_data_mapping->sun_dir                          = glm::normalize(el_data->sun_data.direction);
    m_atmosphere_data_mapping->sun_intensity                    = el_data->sun_data.intensity * 0.0025f;
    m_atmosphere_data_mapping->scatter_points                   = el_data->scatter_points;
    m_atmosphere_data_mapping->scatter_points_second_ray        = el_data->scatter_points_second_ray;
    m_atmosphere_data_mapping->rayleigh_scattering_coefficients = el_data->rayleigh_scattering_coefficients;
    m_atmosphere_data_mapping->mie_scattering_coefficient       = el_data->mie_scattering_coefficient;
    m_atmosphere_data_mapping->density_multiplier               = el_data->density_multiplier;
    m_atmosphere_data_mapping->ground_radius                    = el_data->ground_radius;
    m_atmosphere_data_mapping->atmosphere_radius                = el_data->atmosphere_radius;
    m_atmosphere_data_mapping->ray_origin                       = glm::vec3(0.0f, el_data->ground_radius + el_data->view_height, 0.0f);
    m_atmosphere_data_mapping->mie_preferred_scattering_dir     = el_data->mie_preferred_scattering_dir;

    texture_configuration texture_config;
    texture_config.generate_mipmaps        = calculate_mip_count(m_cube_width, m_cube_height);
    texture_config.is_standard_color_space = false;
    texture_config.is_cubemap              = true;
    texture_config.texture_min_filter      = texture_parameter::filter_linear_mipmap_linear;
    texture_config.texture_mag_filter      = texture_parameter::filter_linear;
    texture_config.texture_wrap_s          = texture_parameter::wrap_clamp_to_edge;
    texture_config.texture_wrap_t          = texture_parameter::wrap_clamp_to_edge;

    if (m_cubemap)
        m_cubemap->release();
    m_cubemap = texture::create(texture_config);
    if (!check_creation(m_cubemap.get(), "environment cubemap texture"))
        return;

    m_cubemap->set_data(format::rgba16f, m_cube_width, m_cube_height, format::rgba, format::t_float, nullptr);

    // creating a temporal command buffer for compute shader execution.
    command_buffer_ptr<min_key> compute_commands = command_buffer<min_key>::create(4096);

    // atmospheric scattering to cubemap
    bind_shader_program_command* bsp = compute_commands->create<bind_shader_program_command>(command_keys::no_sort);
    bsp->shader_program_name         = m_atmospheric_cubemap->get_name();

    // bind output cubemap
    bind_image_texture_command* bit = compute_commands->create<bind_image_texture_command>(command_keys::no_sort);
    bit->binding                    = 0;
    bit->texture_name               = m_cubemap->get_name();
    bit->level                      = 0;
    bit->layered                    = true;
    bit->layer                      = 0;
    bit->access                     = base_access::write_only;
    bit->element_format             = format::rgba16f;

    // bind uniforms
    glm::vec2 out                    = glm::vec2(m_cubemap->get_width(), m_cubemap->get_height());
    bind_single_uniform_command* bsu = compute_commands->create<bind_single_uniform_command>(command_keys::no_sort, sizeof(out));
    bsu->count                       = 1;
    bsu->location                    = 0;
    bsu->type                        = shader_resource_type::fvec2;
    bsu->uniform_value               = compute_commands->map_spare<bind_single_uniform_command>();
    memcpy(bsu->uniform_value, &out, sizeof(out));

    bind_buffer_command* bb = compute_commands->create<bind_buffer_command>(command_keys::no_sort);
    bb->index               = UB_SLOT_COMPUTE_DATA;
    bb->buffer_name         = m_atmosphere_data_buffer->get_name();
    bb->offset              = 0;
    bb->target              = buffer_target::uniform_buffer;
    bb->size                = m_atmosphere_data_buffer->byte_length();

    // execute compute
    dispatch_compute_command* dp = compute_commands->create<dispatch_compute_command>(command_keys::no_sort);
    dp->num_x_groups             = m_cubemap->get_width() / 32;
    dp->num_y_groups             = m_cubemap->get_height() / 32;
    dp->num_z_groups             = 6;

    // We need to recalculate mipmaps
    calculate_mipmaps_command* cm = compute_commands->create<calculate_mipmaps_command>(command_keys::no_sort);
    cm->texture_name              = m_cubemap->get_name();

    add_memory_barrier_command* amb = compute_commands->create<add_memory_barrier_command>(command_keys::no_sort);
    amb->barrier_bit                = memory_barrier_bit::shader_image_access_barrier_bit;

    calculate_ibl_maps(compute_commands);

    {
        GL_NAMED_PROFILE_ZONE("Generating IBL");
        compute_commands->execute();
    }
}

texture_ptr ibl_step::get_irradiance_map()
{
    return m_irradiance_map ? m_irradiance_map : m_default_ibl_texture;
}

texture_ptr ibl_step::get_prefiltered_specular()
{
    return m_prefiltered_specular ? m_prefiltered_specular : m_default_ibl_texture;
}

texture_ptr ibl_step::get_brdf_lookup()
{
    return m_brdf_integration_lut;
}

void ibl_step::on_ui_widget()
{
    ImGui::PushID("ibl_step");
    // Render Level 0.0 - 8.0
    bool should_render = !(m_ibl_data.render_level < -1e-5f);
    static float tmp   = 0.0f;
    checkbox("Render IBL Visualization", &should_render, false);
    if (!should_render)
    {
        m_ibl_data.render_level = -1.0f;
    }
    else
    {
        m_ibl_data.render_level = tmp;
        float& render_level     = m_ibl_data.render_level;
        float default_value     = 0.0f;
        slider_float_n("Blur Level", &render_level, 1, &default_value, 0.0f, 8.0f);
        tmp = m_ibl_data.render_level;
    }
    ImGui::PopID();
}

void ibl_step::calculate_ibl_maps(const command_buffer_ptr<min_key>& compute_commands)
{
    if (!m_cubemap)
    {
        MANGO_LOG_ERROR("Can not calculate ibl maps without a cubemap!");
        return; // Should not be possible;
    }

    texture_configuration texture_config;
    texture_config.generate_mipmaps        = calculate_mip_count(m_prefiltered_base_width, m_prefiltered_base_height);
    texture_config.is_standard_color_space = false;
    texture_config.is_cubemap              = true;
    texture_config.texture_min_filter      = texture_parameter::filter_linear_mipmap_linear;
    texture_config.texture_mag_filter      = texture_parameter::filter_linear;
    texture_config.texture_wrap_s          = texture_parameter::wrap_clamp_to_edge;
    texture_config.texture_wrap_t          = texture_parameter::wrap_clamp_to_edge;

    if (m_prefiltered_specular)
        m_prefiltered_specular->release();
    m_prefiltered_specular = texture::create(texture_config);
    if (!check_creation(m_prefiltered_specular.get(), "prefiltered specular texture"))
        return;

    m_prefiltered_specular->set_data(format::rgba16f, m_prefiltered_base_width, m_prefiltered_base_height, format::rgba, format::t_float, nullptr);

    texture_config.generate_mipmaps   = 1;
    texture_config.texture_min_filter = texture_parameter::filter_linear;
    if (m_irradiance_map)
        m_irradiance_map->release();
    m_irradiance_map = texture::create(texture_config);
    if (!check_creation(m_irradiance_map.get(), "irradiance texture"))
        return;

    m_irradiance_map->set_data(format::rgba16f, m_irradiance_width, m_irradiance_height, format::rgba, format::t_float, nullptr);

    // build irradiance map
    bind_shader_program_command* bsp = compute_commands->create<bind_shader_program_command>(command_keys::no_sort);
    bsp->shader_program_name         = m_build_irradiance_map->get_name();

    // bind input cubemap
    bind_texture_command* bt = compute_commands->create<bind_texture_command>(command_keys::no_sort);
    bt->binding              = 0;
    bt->sampler_location     = 0;
    bt->texture_name         = m_cubemap->get_name();

    // bind output irradiance map
    bind_image_texture_command* bit = compute_commands->create<bind_image_texture_command>(command_keys::no_sort);
    bit->binding                    = 1;
    bit->texture_name               = m_irradiance_map->get_name();
    bit->level                      = 0;
    bit->layered                    = true;
    bit->layer                      = 0;
    bit->access                     = base_access::write_only;
    bit->element_format             = format::rgba16f;

    // bind uniform
    glm::vec2 out                    = glm::vec2(m_irradiance_map->get_width(), m_irradiance_map->get_height());
    bind_single_uniform_command* bsu = compute_commands->create<bind_single_uniform_command>(command_keys::no_sort, sizeof(out));
    bsu->count                       = 1;
    bsu->location                    = 1;
    bsu->type                        = shader_resource_type::fvec2;
    bsu->uniform_value               = compute_commands->map_spare<bind_single_uniform_command>();
    memcpy(bsu->uniform_value, &out, sizeof(out));

    // execute compute
    dispatch_compute_command* dp = compute_commands->create<dispatch_compute_command>(command_keys::no_sort);
    dp->num_x_groups             = m_irradiance_map->get_width() / 32;
    dp->num_y_groups             = m_irradiance_map->get_height() / 32;
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
    bt->texture_name     = m_cubemap->get_name();

    uint32 mip_count = m_prefiltered_specular->mipmaps();
    for (uint32 mip = 0; mip < mip_count; ++mip)
    {
        const uint32 mipmap_width  = m_prefiltered_base_width >> mip;
        const uint32 mipmap_height = m_prefiltered_base_height >> mip;
        float roughness            = (float)mip / (float)(mip_count - 1);

        // bind correct mipmap
        bit                 = compute_commands->create<bind_image_texture_command>(command_keys::no_sort);
        bit->binding        = 1;
        bit->texture_name   = m_prefiltered_specular->get_name();
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
        dp->num_x_groups = m_prefiltered_specular->get_width() / 32;
        dp->num_y_groups = m_prefiltered_specular->get_height() / 32;
        dp->num_z_groups = 6;
    }

    bsp                      = compute_commands->create<bind_shader_program_command>(command_keys::no_sort);
    bsp->shader_program_name = 0;

    amb              = compute_commands->create<add_memory_barrier_command>(command_keys::no_sort);
    amb->barrier_bit = memory_barrier_bit::shader_image_access_barrier_bit;
}
