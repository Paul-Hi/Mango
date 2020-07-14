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
#include <rendering/steps/ibl_step.hpp>

using namespace mango;

static const float cubemap_vertices[36] = { -1.0f, 1.0f, 1.0f,  1.0f, 1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f,
                                            -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f, -1.0f, 1.0f };

static const uint8 cubemap_indices[18] = { 8, 9, 0, 2, 1, 3, 3, 2, 5, 4, 7, 6, 6, 0, 7, 1, 10, 11 };

//! \brief Default texture that is bound to every texture unit not in use to prevent warnings.
texture_ptr default_ibl_texture;

bool ibl_step::create()
{
    // compute shader to convert from equirectangular projected hdr textures to a cube map.
    shader_configuration shader_config;
    shader_config.m_path       = "res/shader/c_equi_to_cubemap.glsl";
    shader_config.m_type       = shader_type::COMPUTE_SHADER;
    shader_ptr to_cube_compute = shader::create(shader_config);
    if (!to_cube_compute)
    {
        MANGO_LOG_ERROR("Creation of cubemap compute shader failed! Ibl step not available!");
        return false;
    }

    m_equi_to_cubemap = shader_program::create_compute_pipeline(to_cube_compute);
    if (!m_equi_to_cubemap)
    {
        MANGO_LOG_ERROR("Creation of cubemap compute shader program failed! Ibl step not available!");
        return false;
    }

    // compute shader to build the irradiance cubemap for image based lighting.
    shader_config.m_path              = "res/shader/c_irradiance_map.glsl";
    shader_config.m_type              = shader_type::COMPUTE_SHADER;
    shader_ptr irradiance_map_compute = shader::create(shader_config);
    if (!irradiance_map_compute)
    {
        MANGO_LOG_ERROR("Creation of ibl irradiance map compute shader failed! Ibl step not available!");
        return false;
    }

    m_build_irradiance_map = shader_program::create_compute_pipeline(irradiance_map_compute);
    if (!m_build_irradiance_map)
    {
        MANGO_LOG_ERROR("Creation of ibl irradiance map compute shader program failed! Ibl step not available!");
        return false;
    }

    // compute shader to build the prefiltered specular cubemap for image based lighting.
    shader_config.m_path                        = "res/shader/c_prefilter_specular_map.glsl";
    shader_config.m_type                        = shader_type::COMPUTE_SHADER;
    shader_ptr specular_prefiltered_map_compute = shader::create(shader_config);
    if (!specular_prefiltered_map_compute)
    {
        MANGO_LOG_ERROR("Creation of ibl prefiltered specular compute shader failed! Ibl step not available!");
        return false;
    }

    m_build_specular_prefiltered_map = shader_program::create_compute_pipeline(specular_prefiltered_map_compute);
    if (!m_build_specular_prefiltered_map)
    {
        MANGO_LOG_ERROR("Creation of ibl prefiltered specular map compute shader program failed! Ibl step not available!");
        return false;
    }

    // compute shader to build the brdf integration lookup texture for image based lighting. Could be done only once.
    shader_config.m_path                = "res/shader/c_brdf_integration.glsl";
    shader_config.m_type                = shader_type::COMPUTE_SHADER;
    shader_ptr brdf_integration_compute = shader::create(shader_config);
    if (!brdf_integration_compute)
    {
        MANGO_LOG_ERROR("Creation of ibl brdf integration compute shader failed! Ibl step not available!");
        return false;
    }

    m_build_integration_lut = shader_program::create_compute_pipeline(brdf_integration_compute);
    if (!m_build_integration_lut)
    {
        MANGO_LOG_ERROR("Creation of ibl brdf integration compute shader program failed! Ibl step not available!");
        return false;
    }

    // cubemap rendering
    shader_config.m_path      = "res/shader/v_cubemap.glsl";
    shader_config.m_type      = shader_type::VERTEX_SHADER;
    shader_ptr cubemap_vertex = shader::create(shader_config);
    if (!cubemap_vertex)
    {
        MANGO_LOG_ERROR("Creation of cubemap vertex shader failed! Ibl step not available!");
        return false;
    }

    shader_config.m_path        = "res/shader/f_cubemap.glsl";
    shader_config.m_type        = shader_type::FRAGMENT_SHADER;
    shader_ptr cubemap_fragment = shader::create(shader_config);
    if (!cubemap_fragment)
    {
        MANGO_LOG_ERROR("Creation of cubemap fragment shader failed! Ibl step not available!");
        return false;
    }

    m_draw_environment = shader_program::create_graphics_pipeline(cubemap_vertex, nullptr, nullptr, nullptr, cubemap_fragment);
    if (!m_draw_environment)
    {
        MANGO_LOG_ERROR("Creation of cubemap rendering shader program failed! Ibl step not available!");
        return false;
    }

    m_cube_geometry = vertex_array::create();

    buffer_configuration b_config;
    b_config.m_access   = buffer_access::NONE;
    b_config.m_size     = sizeof(cubemap_vertices);
    b_config.m_target   = buffer_target::VERTEX_BUFFER;
    const void* vb_data = static_cast<const void*>(cubemap_vertices);
    b_config.m_data     = vb_data;
    buffer_ptr vb       = buffer::create(b_config);

    m_cube_geometry->bind_vertex_buffer(0, vb, 0, sizeof(float) * 3);
    m_cube_geometry->set_vertex_attribute(0, 0, format::RGB32F, 0);

    b_config.m_size     = sizeof(cubemap_indices);
    b_config.m_target   = buffer_target::INDEX_BUFFER;
    const void* ib_data = static_cast<const void*>(cubemap_indices);
    b_config.m_data     = ib_data;
    buffer_ptr ib       = buffer::create(b_config);

    m_cube_geometry->bind_index_buffer(ib);

    m_current_rotation_scale = glm::mat3(1.0f);
    m_render_level           = 0;

    texture_configuration texture_config;
    texture_config.m_generate_mipmaps        = 1;
    texture_config.m_is_standard_color_space = false;
    texture_config.m_is_cubemap              = false;
    texture_config.m_texture_min_filter      = texture_parameter::FILTER_LINEAR;
    texture_config.m_texture_mag_filter      = texture_parameter::FILTER_LINEAR;
    texture_config.m_texture_wrap_s          = texture_parameter::WRAP_CLAMP_TO_EDGE;
    texture_config.m_texture_wrap_t          = texture_parameter::WRAP_CLAMP_TO_EDGE;
    m_brdf_integration_lut                   = texture::create(texture_config);
    m_brdf_integration_lut->set_data(format::RGBA16F, m_integration_lut_width, m_integration_lut_height, format::RGBA, format::FLOAT, nullptr);

    // creating a temporal command buffer for compute shader execution.
    command_buffer_ptr compute_commands = command_buffer::create();

    // build integration look up texture
    compute_commands->bind_shader_program(m_build_integration_lut);

    // bind output lut
    compute_commands->bind_image_texture(0, m_brdf_integration_lut, 0, false, 0, base_access::WRITE_ONLY, format::RGBA16F);
    // bind uniforms
    glm::vec2 out = glm::vec2(m_brdf_integration_lut->get_width(), m_brdf_integration_lut->get_height());
    compute_commands->bind_single_uniform(0, &(out), sizeof(out));
    // execute compute
    compute_commands->dispatch_compute(m_brdf_integration_lut->get_width() / 8, m_brdf_integration_lut->get_height() / 8, 1);

    compute_commands->add_memory_barrier(memory_barrier_bit::SHADER_IMAGE_ACCESS_BARRIER_BIT);

    compute_commands->bind_shader_program(nullptr);

    compute_commands->execute();

    // default texture needed
    texture_config.m_texture_min_filter = texture_parameter::FILTER_NEAREST;
    texture_config.m_texture_mag_filter = texture_parameter::FILTER_NEAREST;
    texture_config.m_is_cubemap         = true;
    default_ibl_texture                 = texture::create(texture_config);
    if (!default_ibl_texture)
    {
        MANGO_LOG_ERROR("Creation of default texture failed! Render system not available!");
        return false;
    }
    g_ubyte dark_blue = 0;
    default_ibl_texture->set_data(format::R8, 1, 1, format::RED, format::UNSIGNED_BYTE, &dark_blue);

    return true;
}

void ibl_step::update(float dt)
{
    MANGO_UNUSED(dt);
}

void ibl_step::attach() {}

void ibl_step::execute(command_buffer_ptr& command_buffer)
{
    if (m_render_level < 0.0f || !m_cubemap)
        return;

    command_buffer->bind_shader_program(m_draw_environment);
    command_buffer->bind_vertex_array(m_cube_geometry);
    command_buffer->bind_single_uniform(0, &const_cast<glm::mat4&>(m_view_projection), sizeof(glm::mat4));
    command_buffer->bind_single_uniform(1, &m_current_rotation_scale, sizeof(m_current_rotation_scale));
    command_buffer->bind_texture(0, m_prefiltered_specular, 2);
    command_buffer->bind_single_uniform(3, &m_render_level, sizeof(m_render_level));

    command_buffer->draw_elements(primitive_topology::TRIANGLE_STRIP, 0, 18, index_type::UBYTE);

    command_buffer->bind_vertex_array(nullptr);
    command_buffer->bind_shader_program(nullptr);
}

void ibl_step::destroy() {}

void ibl_step::load_from_hdr(const texture_ptr& hdr_texture)
{
    texture_configuration texture_config;
    texture_config.m_generate_mipmaps        = calculate_mip_count(m_cube_width, m_cube_height);
    texture_config.m_is_standard_color_space = false;
    texture_config.m_is_cubemap              = true;
    texture_config.m_texture_min_filter      = texture_parameter::FILTER_LINEAR_MIPMAP_LINEAR;
    texture_config.m_texture_mag_filter      = texture_parameter::FILTER_LINEAR;
    texture_config.m_texture_wrap_s          = texture_parameter::WRAP_CLAMP_TO_EDGE;
    texture_config.m_texture_wrap_t          = texture_parameter::WRAP_CLAMP_TO_EDGE;

    m_cubemap = texture::create(texture_config);
    m_cubemap->set_data(format::RGBA16F, m_cube_width, m_cube_height, format::RGBA, format::FLOAT, nullptr);

    texture_config.m_generate_mipmaps = calculate_mip_count(m_prefiltered_base_width, m_prefiltered_base_height);
    m_prefiltered_specular            = texture::create(texture_config);
    m_prefiltered_specular->set_data(format::RGBA16F, m_prefiltered_base_width, m_prefiltered_base_height, format::RGBA, format::FLOAT, nullptr);

    texture_config.m_generate_mipmaps   = 1;
    texture_config.m_texture_min_filter = texture_parameter::FILTER_LINEAR;
    m_irradiance_map                    = texture::create(texture_config);
    m_irradiance_map->set_data(format::RGBA16F, m_irradiance_width, m_irradiance_height, format::RGBA, format::FLOAT, nullptr);

    glm::vec2 out;

    // creating a temporal command buffer for compute shader execution.
    command_buffer_ptr compute_commands = command_buffer::create();

    // equirectangular to cubemap
    compute_commands->bind_shader_program(m_equi_to_cubemap);

    // bind input hdr texture
    compute_commands->bind_texture(0, hdr_texture, 0);
    // bind output cubemap
    compute_commands->bind_image_texture(1, m_cubemap, 0, true, 0, base_access::WRITE_ONLY, format::RGBA16F);
    // bind uniforms
    out = glm::vec2(m_cubemap->get_width(), m_cubemap->get_height());
    compute_commands->bind_single_uniform(1, &(out), sizeof(out));
    // execute compute
    compute_commands->dispatch_compute(m_cubemap->get_width() / 32, m_cubemap->get_height() / 32, 6);

    // We need to recalculate mipmaps
    compute_commands->calculate_mipmaps(m_cubemap);

    compute_commands->add_memory_barrier(memory_barrier_bit::SHADER_IMAGE_ACCESS_BARRIER_BIT);

    // build irradiance map
    compute_commands->bind_shader_program(m_build_irradiance_map);

    // bind input cubemap
    compute_commands->bind_texture(0, m_cubemap, 0);
    // bind output irradiance map
    compute_commands->bind_image_texture(1, m_irradiance_map, 0, true, 0, base_access::WRITE_ONLY, format::RGBA16F);
    // bind uniforms
    out = glm::vec2(m_irradiance_map->get_width(), m_irradiance_map->get_height());
    compute_commands->bind_single_uniform(1, &(out), sizeof(out));
    // execute compute
    compute_commands->dispatch_compute(m_irradiance_map->get_width() / 32, m_irradiance_map->get_height() / 32, 6);

    compute_commands->add_memory_barrier(memory_barrier_bit::SHADER_IMAGE_ACCESS_BARRIER_BIT);

    // build prefiltered specular mipchain
    compute_commands->bind_shader_program(m_build_specular_prefiltered_map);
    // bind input cubemap
    compute_commands->bind_texture(0, m_cubemap, 0);
    uint32 mip_count = m_prefiltered_specular->mipmaps();
    for (uint32 mip = 0; mip < mip_count; ++mip)
    {
        const uint32 mipmap_width  = m_prefiltered_base_width >> mip;
        const uint32 mipmap_height = m_prefiltered_base_height >> mip;
        float roughness            = (float)mip / (float)(mip_count - 1);

        // bind correct mipmap
        compute_commands->bind_image_texture(1, m_prefiltered_specular, static_cast<g_int>(mip), true, 0, base_access::WRITE_ONLY, format::RGBA16F);

        out = glm::vec2(mipmap_width, mipmap_height);
        compute_commands->bind_single_uniform(1, &(out), sizeof(out));
        compute_commands->bind_single_uniform(2, &(roughness), sizeof(roughness));

        compute_commands->dispatch_compute(m_prefiltered_specular->get_width() / 32, m_prefiltered_specular->get_height() / 32, 6);
    }

    compute_commands->bind_shader_program(nullptr);

    compute_commands->execute();
}

void ibl_step::bind_image_based_light_maps(command_buffer_ptr& command_buffer)
{
    if (m_cubemap)
    {
        command_buffer->bind_texture(5, m_irradiance_map, 7);       // TODO Paul: Binding and location...
        command_buffer->bind_texture(6, m_prefiltered_specular, 8); // TODO Paul: Binding and location...
        command_buffer->bind_texture(7, m_brdf_integration_lut, 9); // TODO Paul: Binding and location...
    }
    else
    {
        command_buffer->bind_texture(5, default_ibl_texture, 7);    // TODO Paul: Binding and location...
        command_buffer->bind_texture(6, default_ibl_texture, 8);    // TODO Paul: Binding and location...
        command_buffer->bind_texture(7, m_brdf_integration_lut, 9); // TODO Paul: Binding and location...
    }
}
