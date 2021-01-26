//! \file      fxaa_step.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <graphics/framebuffer.hpp>
#include <graphics/shader.hpp>
#include <graphics/shader_program.hpp>
#include <graphics/texture.hpp>
#include <mango/imgui_helper.hpp>
#include <mango/profile.hpp>
#include <mango/render_system.hpp>
#include <rendering/steps/fxaa_step.hpp>
#include <util/helpers.hpp>

using namespace mango;

bool fxaa_step::create()
{
    PROFILE_ZONE;

    bool success = true;
    success      = success & setup_buffers();
    success      = success & setup_shader_programs();

    return success;
}

bool fxaa_step::setup_shader_programs()
{
    PROFILE_ZONE;
    shader_configuration shader_config;
    shader_config.path     = "res/shader/v_screen_space_triangle.glsl";
    shader_config.type     = shader_type::vertex_shader;
    shader_ptr fxaa_vertex = shader::create(shader_config);
    if (!check_creation(fxaa_vertex.get(), "screen space triangle vertex shader"))
        return false;

    shader_config.path       = "res/shader/post/f_fxaa.glsl";
    shader_config.type       = shader_type::fragment_shader;
    shader_ptr fxaa_fragment = shader::create(shader_config);
    if (!check_creation(fxaa_fragment.get(), "fxaa pass fragment shader"))
        return false;

    m_fxaa_pass = shader_program::create_graphics_pipeline(fxaa_vertex, nullptr, nullptr, nullptr, fxaa_fragment);
    if (!check_creation(m_fxaa_pass.get(), "fxaa pass shader program"))
        return false;

    return true;
}

bool fxaa_step::setup_buffers()
{
    PROFILE_ZONE;

    m_fxaa_command_buffer = command_buffer<min_key>::create(512);

    return true;
}

void fxaa_step::update(float dt)
{
    MANGO_UNUSED(dt);
}

void fxaa_step::attach() {}

void fxaa_step::configure(const fxaa_step_configuration& configuration)
{
    m_quality_preset  = configuration.get_quality_preset();
    m_subpixel_filter = configuration.get_subpixel_filter();
}

void fxaa_step::execute(gpu_buffer_ptr)
{
    PROFILE_ZONE;
    if (!m_fxaa_command_buffer->dirty() || !m_input_texture || !m_output_buffer)
        return;

    set_depth_test_command* sdt      = m_fxaa_command_buffer->create<set_depth_test_command>(command_keys::no_sort);
    sdt->enabled                     = true;
    set_depth_write_command* sdw     = m_fxaa_command_buffer->create<set_depth_write_command>(command_keys::no_sort);
    sdw->enabled                     = true;
    set_depth_func_command* sdf      = m_fxaa_command_buffer->create<set_depth_func_command>(command_keys::no_sort);
    sdf->operation                   = compare_operation::less;
    set_polygon_mode_command* spm    = m_fxaa_command_buffer->create<set_polygon_mode_command>(command_keys::no_sort);
    spm->face                        = polygon_face::face_front_and_back;
    spm->mode                        = polygon_mode::fill;
    set_blending_command* bl         = m_fxaa_command_buffer->create<set_blending_command>(command_keys::no_sort);
    bl->enabled                      = false;
    set_face_culling_command* sfc    = m_fxaa_command_buffer->create<set_face_culling_command>(command_keys::no_sort);
    sfc->enabled                     = true;
    set_cull_face_command* scf       = m_fxaa_command_buffer->create<set_cull_face_command>(command_keys::no_sort);
    scf->face                        = polygon_face::face_back;
    bind_framebuffer_command* bf     = m_fxaa_command_buffer->create<bind_framebuffer_command>(command_keys::no_sort);
    bf->framebuffer_name             = m_output_buffer->get_name();
    bind_shader_program_command* bsp = m_fxaa_command_buffer->create<bind_shader_program_command>(command_keys::no_sort);
    bsp->shader_program_name         = m_fxaa_pass->get_name();

    bind_texture_command* bt = m_fxaa_command_buffer->create<bind_texture_command>(command_keys::no_sort);
    bt->binding              = 0;
    bt->sampler_location     = 0;
    bt->texture_name         = m_input_texture->get_name();

    glm::vec2 inv_screen             = glm::vec2(1.0f / m_output_buffer->get_width(), 1.0f / m_output_buffer->get_height());
    bind_single_uniform_command* bsu = m_fxaa_command_buffer->create<bind_single_uniform_command>(command_keys::no_sort, sizeof(inv_screen));
    bsu->count                       = 1;
    bsu->location                    = 1;
    bsu->type                        = shader_resource_type::fvec2;
    bsu->uniform_value               = m_fxaa_command_buffer->map_spare<bind_single_uniform_command>();
    memcpy(bsu->uniform_value, &inv_screen, sizeof(inv_screen));

    int32 preset_type  = static_cast<int32>(m_quality_preset);
    bsu                = m_fxaa_command_buffer->create<bind_single_uniform_command>(command_keys::no_sort, sizeof(preset_type));
    bsu->count         = 1;
    bsu->location      = 2;
    bsu->type          = shader_resource_type::isingle;
    bsu->uniform_value = m_fxaa_command_buffer->map_spare<bind_single_uniform_command>();
    memcpy(bsu->uniform_value, &preset_type, sizeof(preset_type));

    bsu                = m_fxaa_command_buffer->create<bind_single_uniform_command>(command_keys::no_sort, sizeof(m_subpixel_filter));
    bsu->count         = 1;
    bsu->location      = 3;
    bsu->type          = shader_resource_type::fsingle;
    bsu->uniform_value = m_fxaa_command_buffer->map_spare<bind_single_uniform_command>();
    memcpy(bsu->uniform_value, &m_subpixel_filter, sizeof(m_subpixel_filter));

    draw_arrays_command* da = m_fxaa_command_buffer->create<draw_arrays_command>(command_keys::no_sort);
    da->topology            = primitive_topology::triangles;
    da->first               = 0;
    da->count               = 3;
    da->instance_count      = 1;

#ifdef MANGO_DEBUG
    bsp                      = m_fxaa_command_buffer->create<bind_shader_program_command>(command_keys::no_sort);
    bsp->shader_program_name = 0;
#endif // MANGO_DEBUG
}

void fxaa_step::destroy() {}

void fxaa_step::on_ui_widget()
{
    ImGui::PushID("fxaa_step");

    // Quality Preset
    const char* presets[3] = { "Medium Quality", "High Quality", "Extreme Quality" };
    int32 current_filter   = static_cast<int32>(m_quality_preset);
    bool changed           = combo("FXAA Mode", presets, 3, current_filter, 1);
    m_quality_preset       = static_cast<fxaa_quality_preset>(current_filter);

    float default_value = 0.0f;
    changed |= slider_float_n("Subpixel Filter", &m_subpixel_filter, 1, &default_value, 0.0f, 1.0f);

    if (changed)
        m_fxaa_command_buffer->invalidate();

    ImGui::PopID();
}
