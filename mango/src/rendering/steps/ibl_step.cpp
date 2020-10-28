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
#include <imgui.h>
#include <mango/profile.hpp>
#include <mango/render_system.hpp>
#include <mango/scene_ecs.hpp>
#include <rendering/steps/ibl_step.hpp>
#include <util/helpers.hpp>

using namespace mango;

static const float cubemap_vertices[36] = { -1.0f, 1.0f, 1.0f,  1.0f, 1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f,
                                            -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f, -1.0f, 1.0f };

static const uint8 cubemap_indices[18] = { 8, 9, 0, 2, 1, 3, 3, 2, 5, 4, 7, 6, 6, 0, 7, 1, 10, 11 };

//! \brief Default texture that is bound to every texture unit not in use to prevent warnings.
texture_ptr default_ibl_texture;

bool ibl_step::create()
{
    PROFILE_ZONE;

    m_ibl_command_buffer = command_buffer<min_key>::create(512);

    // compute shader to convert from equirectangular projected hdr textures to a cube map.
    shader_configuration shader_config;
    shader_config.m_path       = "res/shader/c_equi_to_cubemap.glsl";
    shader_config.m_type       = shader_type::compute_shader;
    shader_ptr to_cube_compute = shader::create(shader_config);
    if (!check_creation(to_cube_compute.get(), "cubemap compute shader", "Ibl Step"))
        return false;

    m_equi_to_cubemap = shader_program::create_compute_pipeline(to_cube_compute);
    if (!check_creation(m_equi_to_cubemap.get(), "cubemap compute shader program", "Ibl Step"))
        return false;

    // compute shader to build the irradiance cubemap for image based lighting.
    shader_config.m_path              = "res/shader/c_irradiance_map.glsl";
    shader_config.m_type              = shader_type::compute_shader;
    shader_ptr irradiance_map_compute = shader::create(shader_config);
    if (!check_creation(irradiance_map_compute.get(), "irradiance map compute shader", "Ibl Step"))
        return false;

    m_build_irradiance_map = shader_program::create_compute_pipeline(irradiance_map_compute);
    if (!check_creation(m_build_irradiance_map.get(), "irradiance map compute shader program", "Ibl Step"))
        return false;

    // compute shader to build the prefiltered specular cubemap for image based lighting.
    shader_config.m_path                        = "res/shader/c_prefilter_specular_map.glsl";
    shader_config.m_type                        = shader_type::compute_shader;
    shader_ptr specular_prefiltered_map_compute = shader::create(shader_config);
    if (!check_creation(specular_prefiltered_map_compute.get(), "prefilter specular ibl compute shader", "Ibl Step"))
        return false;

    m_build_specular_prefiltered_map = shader_program::create_compute_pipeline(specular_prefiltered_map_compute);
    if (!check_creation(m_build_specular_prefiltered_map.get(), "prefilter specular ibl compute shader program", "Ibl Step"))
        return false;

    // compute shader to build the brdf integration lookup texture for image based lighting. Could be done only once.
    shader_config.m_path                = "res/shader/c_brdf_integration.glsl";
    shader_config.m_type                = shader_type::compute_shader;
    shader_ptr brdf_integration_compute = shader::create(shader_config);
    if (!check_creation(brdf_integration_compute.get(), "ibl brdf integration compute shader", "Ibl Step"))
        return false;

    m_build_integration_lut = shader_program::create_compute_pipeline(brdf_integration_compute);
    if (!check_creation(m_build_integration_lut.get(), "ibl brdf integration compute shader program", "Ibl Step"))
        return false;

    // cubemap rendering
    shader_config.m_path      = "res/shader/v_cubemap.glsl";
    shader_config.m_type      = shader_type::vertex_shader;
    shader_ptr cubemap_vertex = shader::create(shader_config);
    if (!check_creation(cubemap_vertex.get(), "cubemap vertex shader", "Ibl Step"))
        return false;

    shader_config.m_path        = "res/shader/f_cubemap.glsl";
    shader_config.m_type        = shader_type::fragment_shader;
    shader_ptr cubemap_fragment = shader::create(shader_config);
    if (!check_creation(cubemap_fragment.get(), "cubemap fragment shader", "Ibl Step"))
        return false;

    m_draw_environment = shader_program::create_graphics_pipeline(cubemap_vertex, nullptr, nullptr, nullptr, cubemap_fragment);
    if (!check_creation(m_draw_environment.get(), "cubemap rendering shader program", "Ibl Step"))
        return false;

    m_cube_geometry = vertex_array::create();
    if (!check_creation(m_cube_geometry.get(), "cubemap geometry vertex array", "Ibl Step"))
        return false;

    buffer_configuration b_config;
    b_config.m_access   = buffer_access::none;
    b_config.m_size     = sizeof(cubemap_vertices);
    b_config.m_target   = buffer_target::vertex_buffer;
    const void* vb_data = static_cast<const void*>(cubemap_vertices);
    b_config.m_data     = vb_data;
    buffer_ptr vb       = buffer::create(b_config);

    m_cube_geometry->bind_vertex_buffer(0, vb, 0, sizeof(float) * 3);
    m_cube_geometry->set_vertex_attribute(0, 0, format::rgb32f, 0);

    b_config.m_size     = sizeof(cubemap_indices);
    b_config.m_target   = buffer_target::index_buffer;
    const void* ib_data = static_cast<const void*>(cubemap_indices);
    b_config.m_data     = ib_data;
    buffer_ptr ib       = buffer::create(b_config);

    m_cube_geometry->bind_index_buffer(ib);

    m_ibl_data.current_rotation_scale = glm::mat3(1.0f);
    m_ibl_data.render_level           = 0.0f;

    texture_configuration texture_config;
    texture_config.m_generate_mipmaps        = 1;
    texture_config.m_is_standard_color_space = false;
    texture_config.m_is_cubemap              = false;
    texture_config.m_texture_min_filter      = texture_parameter::filter_linear;
    texture_config.m_texture_mag_filter      = texture_parameter::filter_linear;
    texture_config.m_texture_wrap_s          = texture_parameter::wrap_clamp_to_edge;
    texture_config.m_texture_wrap_t          = texture_parameter::wrap_clamp_to_edge;
    m_brdf_integration_lut                   = texture::create(texture_config);
    m_brdf_integration_lut->set_data(format::rgba16f, m_integration_lut_width, m_integration_lut_height, format::rgba, format::t_float, nullptr);

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

    // default texture needed
    texture_config.m_texture_min_filter = texture_parameter::filter_nearest;
    texture_config.m_texture_mag_filter = texture_parameter::filter_nearest;
    texture_config.m_is_cubemap         = true;
    default_ibl_texture                 = texture::create(texture_config);
    if (!check_creation(default_ibl_texture.get(), "default ibl texture", "Ibl Step"))
        return false;

    g_ubyte albedo[3] = { 127, 127, 127 };
    default_ibl_texture->set_data(format::rgb8, 1, 1, format::rgb, format::t_unsigned_byte, albedo);

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
    if (m_cubemap)
        bt->texture_name = m_prefiltered_specular->get_name();
    else
        bt->texture_name = default_ibl_texture->get_name();

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

void ibl_step::load_from_hdr(const texture_ptr& hdr_texture)
{
    PROFILE_ZONE;
    if (!hdr_texture)
    {
        m_cubemap              = nullptr;
        m_irradiance_map       = nullptr;
        m_prefiltered_specular = nullptr;
        return;
    }
    texture_configuration texture_config;
    texture_config.m_generate_mipmaps        = calculate_mip_count(m_cube_width, m_cube_height);
    texture_config.m_is_standard_color_space = false;
    texture_config.m_is_cubemap              = true;
    texture_config.m_texture_min_filter      = texture_parameter::filter_linear_mipmap_linear;
    texture_config.m_texture_mag_filter      = texture_parameter::filter_linear;
    texture_config.m_texture_wrap_s          = texture_parameter::wrap_clamp_to_edge;
    texture_config.m_texture_wrap_t          = texture_parameter::wrap_clamp_to_edge;

    if (m_cubemap)
        m_cubemap->release();
    m_cubemap = texture::create(texture_config);
    m_cubemap->set_data(format::rgba16f, m_cube_width, m_cube_height, format::rgba, format::t_float, nullptr);

    texture_config.m_generate_mipmaps = calculate_mip_count(m_prefiltered_base_width, m_prefiltered_base_height);
    if (m_prefiltered_specular)
        m_prefiltered_specular->release();
    m_prefiltered_specular = texture::create(texture_config);
    m_prefiltered_specular->set_data(format::rgba16f, m_prefiltered_base_width, m_prefiltered_base_height, format::rgba, format::t_float, nullptr);

    texture_config.m_generate_mipmaps   = 1;
    texture_config.m_texture_min_filter = texture_parameter::filter_linear;
    if (m_irradiance_map)
        m_irradiance_map->release();
    m_irradiance_map = texture::create(texture_config);
    m_irradiance_map->set_data(format::rgba16f, m_irradiance_width, m_irradiance_height, format::rgba, format::t_float, nullptr);

    // creating a temporal command buffer for compute shader execution.
    command_buffer_ptr<min_key> compute_commands = command_buffer<min_key>::create(4096);

    // equirectangular to cubemap
    bind_shader_program_command* bsp = compute_commands->create<bind_shader_program_command>(command_keys::no_sort);
    bsp->shader_program_name         = m_equi_to_cubemap->get_name();

    // bind input hdr texture
    bind_texture_command* bt = compute_commands->create<bind_texture_command>(command_keys::no_sort);
    bt->binding              = 0;
    bt->sampler_location     = 0;
    bt->texture_name         = hdr_texture->get_name();

    // bind output cubemap
    bind_image_texture_command* bit = compute_commands->create<bind_image_texture_command>(command_keys::no_sort);
    bit->binding                    = 1;
    bit->texture_name               = m_cubemap->get_name();
    bit->level                      = 0;
    bit->layered                    = true;
    bit->layer                      = 0;
    bit->access                     = base_access::write_only;
    bit->element_format             = format::rgba16f;

    // bind uniform
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

    // build irradiance map
    bsp                      = compute_commands->create<bind_shader_program_command>(command_keys::no_sort);
    bsp->shader_program_name = m_build_irradiance_map->get_name();

    // bind input cubemap
    bt                   = compute_commands->create<bind_texture_command>(command_keys::no_sort);
    bt->binding          = 0;
    bt->sampler_location = 0;
    bt->texture_name     = m_cubemap->get_name();

    // bind output irradiance map
    bit                 = compute_commands->create<bind_image_texture_command>(command_keys::no_sort);
    bit->binding        = 1;
    bit->texture_name   = m_irradiance_map->get_name();
    bit->level          = 0;
    bit->layered        = true;
    bit->layer          = 0;
    bit->access         = base_access::write_only;
    bit->element_format = format::rgba16f;

    // bind uniform
    out                = glm::vec2(m_irradiance_map->get_width(), m_irradiance_map->get_height());
    bsu                = compute_commands->create<bind_single_uniform_command>(command_keys::no_sort, sizeof(out));
    bsu->count         = 1;
    bsu->location      = 1;
    bsu->type          = shader_resource_type::fvec2;
    bsu->uniform_value = compute_commands->map_spare<bind_single_uniform_command>();
    memcpy(bsu->uniform_value, &out, sizeof(out));

    // execute compute
    dp               = compute_commands->create<dispatch_compute_command>(command_keys::no_sort);
    dp->num_x_groups = m_irradiance_map->get_width() / 32;
    dp->num_y_groups = m_irradiance_map->get_height() / 32;
    dp->num_z_groups = 6;

    amb              = compute_commands->create<add_memory_barrier_command>(command_keys::no_sort);
    amb->barrier_bit = memory_barrier_bit::shader_image_access_barrier_bit;

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

    {
        GL_NAMED_PROFILE_ZONE("Generating IBL");
        compute_commands->execute();
    }
}

texture_ptr ibl_step::get_irradiance_map()
{
    return m_cubemap ? m_irradiance_map : default_ibl_texture;
}

texture_ptr ibl_step::get_prefiltered_specular()
{
    return m_cubemap ? m_prefiltered_specular : default_ibl_texture;
}

texture_ptr ibl_step::get_brdf_lookup()
{
    return m_brdf_integration_lut;
}

// if (m_cubemap)
//{
//    command_buffer->bind_texture(5, m_irradiance_map, 5);       // TODO Paul: Binding and location...
//    command_buffer->bind_texture(6, m_prefiltered_specular, 6); // TODO Paul: Binding and location...
//    command_buffer->bind_texture(7, m_brdf_integration_lut, 7); // TODO Paul: Binding and location...
//}
// else
//{
//    command_buffer->bind_texture(5, default_ibl_texture, 5);    // TODO Paul: Binding and location...
//    command_buffer->bind_texture(6, default_ibl_texture, 6);    // TODO Paul: Binding and location...
//    command_buffer->bind_texture(7, m_brdf_integration_lut, 7); // TODO Paul: Binding and location...
//}

void ibl_step::on_ui_widget()
{
    // Render Level 0.0 - 8.0
    bool should_render = !(m_ibl_data.render_level < -1e-5f);
    static float tmp   = 0.0f;
    ImGui::Checkbox("Render IBL Visualization##ibl_step", &should_render);
    if (!should_render)
    {
        m_ibl_data.render_level = -1.0f;
    }
    else
    {
        m_ibl_data.render_level = tmp;
        float& render_level     = m_ibl_data.render_level;
        ImGui::SliderFloat("Blur Level##ibl_step", &render_level, 0.0f, 8.0f);
        tmp = m_ibl_data.render_level;
    }
}
