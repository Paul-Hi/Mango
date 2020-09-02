//! \file      shadow_map_step.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <graphics/buffer.hpp>
#include <graphics/shader.hpp>
#include <graphics/shader_program.hpp>
#include <graphics/texture.hpp>
#include <graphics/vertex_array.hpp>
#include <mango/profile.hpp>
#include <rendering/steps/shadow_map_step.hpp>
#include <util/helpers.hpp>

using namespace mango;

bool shadow_map_step::create()
{
    PROFILE_ZONE;

    m_caster_queue = command_buffer::create();

    shader_configuration shader_config;
    shader_config.m_path          = "res/shader/v_shadow_pass.glsl";
    shader_config.m_type          = shader_type::VERTEX_SHADER;
    shader_ptr shadow_pass_vertex = shader::create(shader_config);
    if (!check_creation(shadow_pass_vertex.get(), "shadow pass vertex shader", "Render System"))
        return false;

    shader_config.m_path            = "res/shader/f_shadow_pass.glsl";
    shader_config.m_type            = shader_type::FRAGMENT_SHADER;
    shader_ptr shadow_pass_fragment = shader::create(shader_config);
    if (!check_creation(shadow_pass_fragment.get(), "shadow pass fragment shader", "Render System"))
        return false;

    m_shadow_pass = shader_program::create_graphics_pipeline(shadow_pass_vertex, nullptr, nullptr, nullptr, shadow_pass_fragment);
    if (!check_creation(m_shadow_pass.get(), "shadow pass shader program", "Render System"))
        return false;

    texture_configuration shadow_map_config;
    shadow_map_config.m_generate_mipmaps        = 1;
    shadow_map_config.m_is_standard_color_space = false;
    shadow_map_config.m_texture_min_filter      = texture_parameter::FILTER_LINEAR;
    shadow_map_config.m_texture_mag_filter      = texture_parameter::FILTER_LINEAR;
    shadow_map_config.m_texture_wrap_s          = texture_parameter::WRAP_CLAMP_TO_EDGE;
    shadow_map_config.m_texture_wrap_t          = texture_parameter::WRAP_CLAMP_TO_EDGE;

    m_shadow_map_fb_config.m_depth_attachment = texture::create(shadow_map_config); // directional is depth.
    m_shadow_map_fb_config.m_depth_attachment->set_data(format::DEPTH_COMPONENT24, m_width, m_height, format::DEPTH_COMPONENT, format::FLOAT, nullptr);
    // multiple shadow maps should be all attached to one fb in the future.
    m_shadow_map_fb_config.m_width  = m_width;
    m_shadow_map_fb_config.m_height = m_height;

    m_shadow_buffer = framebuffer::create(m_shadow_map_fb_config);
    if (!check_creation(m_shadow_buffer.get(), "shadow buffer", "Render System"))
        return false;

    return true;
}

void shadow_map_step::update(float dt)
{
    MANGO_UNUSED(dt);
}

void shadow_map_step::attach() {}

void shadow_map_step::execute(command_buffer_ptr& command_buffer)
{
    PROFILE_ZONE;
    command_buffer->bind_framebuffer(m_shadow_buffer);
    command_buffer->clear_framebuffer(clear_buffer_mask::DEPTH_BUFFER, attachment_mask::DEPTH_BUFFER, 0.0f, 0.0f, 0.0f, 1.0f, m_shadow_buffer);
    command_buffer->bind_shader_program(m_shadow_pass);
    command_buffer->set_viewport(0, 0, m_width, m_height);
    command_buffer->bind_single_uniform(0, &m_view_projection, sizeof(glm::mat4)); // shadow view projection
    GL_NAMED_PROFILE_ZONE("Draw Geometry Shadow Map");
    command_buffer->execute();
    m_caster_queue->execute();
}

void shadow_map_step::destroy() {}

void shadow_map_step::bind_shadow_maps(command_buffer_ptr& command_buffer)
{
    PROFILE_ZONE;
    command_buffer->bind_texture(5, m_shadow_buffer->get_attachment(framebuffer_attachment::DEPTH_ATTACHMENT), 5); // TODO Paul: Location, Binding?
}
