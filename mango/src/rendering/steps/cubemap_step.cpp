//! \file      cubemap_step.cpp
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
#include <rendering/steps/cubemap_step.hpp>
#include <util/helpers.hpp>

using namespace mango;

static const float cubemap_vertices[36] = { -1.0f, 1.0f, 1.0f,  1.0f, 1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f,
                                            -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f, -1.0f, 1.0f };

static const uint8 cubemap_indices[18] = { 8, 9, 0, 2, 1, 3, 3, 2, 5, 4, 7, 6, 6, 0, 7, 1, 10, 11 };

bool cubemap_step::create()
{
    PROFILE_ZONE;

    bool success = true;
    success      = success & setup_buffers();
    success      = success & setup_shader_programs();

    return success;
}

bool cubemap_step::setup_shader_programs()
{
    PROFILE_ZONE;
    // cubemap rendering
    shader_configuration shader_config;
    shader_config.path        = "res/shader/post/v_cubemap.glsl";
    shader_config.type        = shader_type::vertex_shader;
    shader_ptr cubemap_vertex = shader::create(shader_config);
    if (!check_creation(cubemap_vertex.get(), "cubemap vertex shader"))
        return false;

    shader_config.path          = "res/shader/post/f_cubemap.glsl";
    shader_config.type          = shader_type::fragment_shader;
    shader_ptr cubemap_fragment = shader::create(shader_config);
    if (!check_creation(cubemap_fragment.get(), "cubemap fragment shader"))
        return false;

    m_draw_environment = shader_program::create_graphics_pipeline(cubemap_vertex, nullptr, nullptr, nullptr, cubemap_fragment);
    if (!check_creation(m_draw_environment.get(), "cubemap rendering shader program"))
        return false;

    return true;
}

bool cubemap_step::setup_buffers()
{
    PROFILE_ZONE;

    m_cubemap_command_buffer = command_buffer<min_key>::create(512);

    m_cube_geometry = vertex_array::create();
    if (!check_creation(m_cube_geometry.get(), "cubemap geometry vertex array"))
        return false;

    buffer_configuration b_config;
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

    m_cubemap_data.model_matrix = glm::mat4(1.0f);
    m_cubemap_data.render_level           = 0.0f;

    return true;
}

void cubemap_step::update(float dt)
{
    MANGO_UNUSED(dt);
}

void cubemap_step::attach() {}

void cubemap_step::configure(const cubemap_step_configuration& configuration)
{
    m_cubemap_data.render_level = configuration.get_render_level();
    MANGO_ASSERT(m_cubemap_data.render_level > 0.0f && m_cubemap_data.render_level < 8.1f, "Cubemap render level has to be between 0.0 and 8.0f!");
}

void cubemap_step::execute(gpu_buffer_ptr frame_uniform_buffer)
{
    PROFILE_ZONE;
    if (!m_current_cubemap || m_cubemap_data.render_level < 0.0f)
        return;

    set_depth_test_command* sdt = m_cubemap_command_buffer->create<set_depth_test_command>(command_keys::no_sort);
    sdt->enabled                = true;
    set_depth_func_command* sdf = m_cubemap_command_buffer->create<set_depth_func_command>(command_keys::no_sort);
    sdf->operation              = compare_operation::less_equal;

    set_cull_face_command* scf = m_cubemap_command_buffer->create<set_cull_face_command>(command_keys::no_sort);
    scf->face                  = polygon_face::face_front;

    set_polygon_mode_command* spm = m_cubemap_command_buffer->create<set_polygon_mode_command>(command_keys::no_sort);
    spm->face                     = polygon_face::face_front_and_back;
    spm->mode                     = polygon_mode::fill;

    set_blending_command* bl = m_cubemap_command_buffer->create<set_blending_command>(command_keys::no_sort);
    bl->enabled              = false;

    bind_shader_program_command* bsp = m_cubemap_command_buffer->create<bind_shader_program_command>(command_keys::no_sort);
    bsp->shader_program_name         = m_draw_environment->get_name();

    bind_vertex_array_command* bva = m_cubemap_command_buffer->create<bind_vertex_array_command>(command_keys::no_sort);
    bva->vertex_array_name         = m_cube_geometry->get_name();

    bind_buffer_command* bb = m_cubemap_command_buffer->create<bind_buffer_command>(command_keys::no_sort);
    bb->target              = buffer_target::uniform_buffer;
    bb->index               = UB_SLOT_CUBEMAP_DATA;
    bb->size                = sizeof(cubemap_data);
    bb->buffer_name         = frame_uniform_buffer->buffer_name();
    bb->offset              = frame_uniform_buffer->write_data(sizeof(cubemap_data), &m_cubemap_data);

    bind_texture_command* bt = m_cubemap_command_buffer->create<bind_texture_command>(command_keys::no_sort);
    bt->binding              = 0;
    bt->sampler_location     = 0;
    if (m_current_cubemap)
        bt->texture_name = m_current_cubemap->get_name();

    draw_elements_command* de = m_cubemap_command_buffer->create<draw_elements_command>(command_keys::no_sort);
    de->topology              = primitive_topology::triangle_strip;
    de->first                 = 0;
    de->count                 = 18;
    de->type                  = index_type::ubyte;
    de->instance_count        = 1;

#ifdef MANGO_DEBUG
    bva                    = m_cubemap_command_buffer->create<bind_vertex_array_command>(command_keys::no_sort);
    bva->vertex_array_name = 0;

    bsp                      = m_cubemap_command_buffer->create<bind_shader_program_command>(command_keys::no_sort);
    bsp->shader_program_name = 0;
#endif // MANGO_DEBUG
}

void cubemap_step::destroy() {}

void cubemap_step::on_ui_widget()
{
    ImGui::PushID("cubemap_step");
    // Render Level 0.0 - 8.0
    bool should_render = !(m_cubemap_data.render_level < -1e-5f);
    static float tmp   = 0.0f;
    checkbox("Render Global Skylight Cubemap", &should_render, false);
    if (!should_render)
    {
        m_cubemap_data.render_level = -1.0f;
    }
    else
    {
        m_cubemap_data.render_level = tmp;
        float& render_level         = m_cubemap_data.render_level;
        float default_value         = 0.0f;
        slider_float_n("Blur Level", &render_level, 1, &default_value, 0.0f, 10.0f);
        tmp = m_cubemap_data.render_level;
    }
    ImGui::PopID();
}
