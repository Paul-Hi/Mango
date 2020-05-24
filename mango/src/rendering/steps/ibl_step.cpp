//! \file      ibl_step.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <graphics/buffer.hpp>
#include <graphics/shader.hpp>
#include <graphics/shader_program.hpp>
#include <graphics/vertex_array.hpp>
#include <rendering/steps/ibl_step.hpp>

using namespace mango;

static const float cubemap_vertices[36] = { -1.0f, 1.0f, 1.0f,  1.0f, 1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f,
                                            -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f, -1.0f, 1.0f };

static const uint8 cubemap_indices[18] = { 8, 9, 0, 2, 1, 3, 3, 2, 5, 4, 7, 6, 6, 0, 7, 1, 10, 11 };

bool ibl_step::create()
{
    // compute shader to convert from equirectangular projected hdr textures to a cube map.
    shader_configuration shader_config;
    // shader_config.m_path       = "res/shader/c_equi_to_cubemap.glsl";
    // shader_config.m_type       = shader_type::COMPUTE_SHADER;
    // shader_ptr to_cube_compute = shader::create(shader_config);
    // if (!to_cube_compute)
    // {
    //     MANGO_LOG_ERROR("Creation of cubemap compute shader failed! Ibl step not available!");
    //     return false;
    // }
    //
    // m_equi_to_cubemap = shader_program::create_compute_pipeline(to_cube_compute);
    // if (!m_equi_to_cubemap)
    // {
    //     MANGO_LOG_ERROR("Creation of cubemap compute shader program failed! Ibl step not available!");
    //     return false;
    // }

    // compute shader to prefilter the cube map for image based lighting.
    // shader_config.m_path             = "res/shader/c_ibl_prefilter.glsl";
    // shader_config.m_type             = shader_type::COMPUTE_SHADER;
    // shader_ptr prefilter_ibl_compute = shader::create(shader_config);
    // if (!prefilter_ibl_compute)
    // {
    //     MANGO_LOG_ERROR("Creation of ibl prefilter compute shader failed! Ibl step not available!");
    //     return false;
    // }

    // m_prefilter_ibl = shader_program::create_compute_pipeline(prefilter_ibl_compute);
    // if (!m_prefilter_ibl)
    // {
    //     MANGO_LOG_ERROR("Creation of ibl prefilter compute shader program failed! Ibl step not available!");
    //     return false;
    // }

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

    return true;
}

void ibl_step::update(float dt) {}

void ibl_step::attach() {}

void ibl_step::execute(command_buffer_ptr& command_buffer)
{
    command_buffer->bind_shader_program(m_draw_environment);
    command_buffer->bind_vertex_array(m_cube_geometry);
    command_buffer->bind_single_uniform(0, &m_current_rotation_scale, sizeof(m_current_rotation_scale));
    command_buffer->bind_texture(0, m_cubemap, 1);

    command_buffer->draw_elements(primitive_topology::TRIANGLE_STRIP, 0, 18, index_type::UBYTE);

    command_buffer->bind_texture(0, nullptr, 1);
    command_buffer->bind_vertex_array(nullptr);
    command_buffer->bind_shader_program(nullptr);
}

void ibl_step::destroy() {}

void ibl_step::load_from_hdr(const texture_ptr& hdr_texture)
{
    // dumb test
    m_cubemap = hdr_texture;
}
