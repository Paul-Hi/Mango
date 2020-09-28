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
#include <imgui.h>
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

    shader_config.m_path            = "res/shader/g_shadow_pass.glsl";
    shader_config.m_type            = shader_type::GEOMETRY_SHADER;
    shader_ptr shadow_pass_geometry = shader::create(shader_config);
    if (!check_creation(shadow_pass_geometry.get(), "shadow pass geometry shader", "Render System"))
        return false;

    shader_config.m_path            = "res/shader/f_shadow_pass.glsl";
    shader_config.m_type            = shader_type::FRAGMENT_SHADER;
    shader_ptr shadow_pass_fragment = shader::create(shader_config);
    if (!check_creation(shadow_pass_fragment.get(), "shadow pass fragment shader", "Render System"))
        return false;

    m_shadow_pass = shader_program::create_graphics_pipeline(shadow_pass_vertex, nullptr, nullptr, shadow_pass_geometry, shadow_pass_fragment);
    if (!check_creation(m_shadow_pass.get(), "shadow pass shader program", "Render System"))
        return false;

    texture_configuration shadow_map_config;
    shadow_map_config.m_generate_mipmaps        = 1;
    shadow_map_config.m_is_standard_color_space = false;
    shadow_map_config.m_texture_min_filter      = texture_parameter::FILTER_LINEAR;
    shadow_map_config.m_texture_mag_filter      = texture_parameter::FILTER_LINEAR;
    shadow_map_config.m_texture_wrap_s          = texture_parameter::WRAP_CLAMP_TO_EDGE;
    shadow_map_config.m_texture_wrap_t          = texture_parameter::WRAP_CLAMP_TO_EDGE;
    shadow_map_config.m_layers                  = max_shadow_mapping_cascades;

    framebuffer_configuration fb_config;
    fb_config.m_depth_attachment = texture::create(shadow_map_config);
    fb_config.m_depth_attachment->set_data(format::DEPTH_COMPONENT24, m_resolution, m_resolution, format::DEPTH_COMPONENT, format::FLOAT, nullptr);
    fb_config.m_width  = m_resolution;
    fb_config.m_height = m_resolution;

    m_shadow_buffer = framebuffer::create(fb_config);
    if (!check_creation(m_shadow_buffer.get(), "shadow buffer", "Render System"))
        return false;

    m_cascade_data.lambda = 0.65f;

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
    command_buffer->bind_shader_program(m_shadow_pass);
    command_buffer->clear_framebuffer(clear_buffer_mask::DEPTH_BUFFER, attachment_mask::DEPTH_BUFFER, 0.0f, 0.0f, 0.0f, 1.0f, m_shadow_buffer);
    command_buffer->set_viewport(0, 0, m_resolution, m_resolution);
    command_buffer->set_face_culling(false);
    command_buffer->set_polygon_offset(1.1f, 4.0f);
    command_buffer->bind_single_uniform(0, &m_cascade_data.view_projection_matrices[0], sizeof(m_cascade_data.view_projection_matrices), max_shadow_mapping_cascades); // shadow view projections
    command_buffer->bind_single_uniform(4, &m_shadow_map_cascade_count, sizeof(m_shadow_map_cascade_count));
    command_buffer->attach(m_caster_queue);
    command_buffer->set_polygon_offset(0.0f, 0.0f);
}

void shadow_map_step::destroy() {}

void shadow_map_step::update_cascades(float camera_near, float camera_far, const glm::mat4& camera_view_projection, const glm::vec3& directional_direction)
{
    m_cascade_data.camera_near           = camera_near;
    m_cascade_data.camera_far            = camera_far;
    m_cascade_data.directional_direction = directional_direction;

    auto near = camera_near;
    auto far  = camera_far;

    if ((glm::abs(m_cascade_data.split_depth[0] - near) > 1e-5f) || (glm::abs(m_cascade_data.split_depth[m_shadow_map_cascade_count] - far) > 1e-5f) || m_dirty_cascades)
    {
        m_dirty_cascades                                       = false;
        m_cascade_data.split_depth[0]                          = near;
        m_cascade_data.split_depth[m_shadow_map_cascade_count] = far;
        for (int32 i = 1; i < m_shadow_map_cascade_count; ++i)
        {
            float p                       = static_cast<float>(i) / static_cast<float>(m_shadow_map_cascade_count);
            float log                     = near * std::pow((far / near), p);
            float uniform                 = near + (far - near) * p;
            float C_i                     = m_cascade_data.lambda * log + (1.0f - m_cascade_data.lambda) * uniform;
            m_cascade_data.split_depth[i] = C_i;
        }
    }

    // calculate camera frustum in world space
    // TODO Paul: As soon as we need that more often we should do this in the camera.
    glm::vec3 frustum_corners[8] = {
        glm::vec3(-1.0f, 1.0f, -1.0f), glm::vec3(1.0f, 1.0f, -1.0f), glm::vec3(1.0f, -1.0f, -1.0f), glm::vec3(-1.0f, -1.0f, -1.0f),
        glm::vec3(-1.0f, 1.0f, 1.0f),  glm::vec3(1.0f, 1.0f, 1.0f),  glm::vec3(1.0f, -1.0f, 1.0f),  glm::vec3(-1.0f, -1.0f, 1.0f),
    };

    glm::mat4 cam_inv_vp = glm::inverse(camera_view_projection);
    for (int32 i = 0; i < 8; ++i)
    {
        glm::vec4 inv      = cam_inv_vp * glm::vec4(frustum_corners[i], 1.0f);
        frustum_corners[i] = glm::vec3(inv / inv.w);
    }

    for (int32 casc = 0; casc < m_shadow_map_cascade_count; ++casc)
    {
        glm::vec3 center = glm::vec3(0.0f);
        glm::vec3 current_frustum_corners[8];
        for (int32 i = 0; i < 4; ++i)
        {
            glm::vec3 corner_ray           = frustum_corners[i + 4] - frustum_corners[i];
            glm::vec3 near_ray             = corner_ray * (m_cascade_data.split_depth[casc] / m_cascade_data.split_depth[m_shadow_map_cascade_count]);
            glm::vec3 far_ray              = corner_ray * (m_cascade_data.split_depth[casc + 1] / m_cascade_data.split_depth[m_shadow_map_cascade_count]);
            current_frustum_corners[i]     = frustum_corners[i] + near_ray;
            current_frustum_corners[i + 4] = frustum_corners[i] + far_ray;
            center += current_frustum_corners[i];
            center += current_frustum_corners[i + 4];
        }
        center /= 8.0f;

        float radius = 0.0f;
        for (int32 i = 0; i < 8; ++i)
        {
            float distance = glm::length(current_frustum_corners[i] - center);
            radius         = glm::max(radius, distance);
        }
        radius = std::ceil(radius * 16.0f) / 16.0f;

        glm::vec3 max_value = glm::vec3(radius);
        glm::vec3 min_value = -max_value;

        // calculate view projection

        glm::mat4 projection;
        glm::mat4 view;
        view                            = glm::lookAt(center + glm::normalize(directional_direction) * (-min_value.z + m_shadowmap_offset), center, GLOBAL_UP);
        projection                      = glm::ortho(min_value.x, max_value.x, min_value.y, max_value.y, 0.0f, (max_value.z - min_value.z) + m_shadowmap_offset);
        m_cascade_data.far_planes[casc] = (max_value.z - min_value.z) + m_shadowmap_offset;

        glm::mat4 shadow_matrix = projection * view;
        glm::vec4 origin        = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        origin                  = shadow_matrix * origin;
        origin *= static_cast<float>(m_resolution) * 0.5f;

        glm::vec4 offset = glm::round(origin) - origin;
        offset *= 2.0f / static_cast<float>(m_resolution);
        offset.z = 0.0f;
        offset.w = 0.0f;
        projection[3] += offset;

        m_cascade_data.view_projection_matrices[casc] = projection * view;
    }
}

void shadow_map_step::bind_shadow_maps_and_get_shadow_data(command_buffer_ptr& command_buffer, glm::mat4 (&out_view_projections)[max_shadow_mapping_cascades], glm::vec4& far_planes,
                                                           glm::vec4& cascade_info)
{
    PROFILE_ZONE;
    command_buffer->bind_texture(8, m_shadow_buffer->get_attachment(framebuffer_attachment::DEPTH_ATTACHMENT), 8); // TODO Paul: Location, Binding?
    out_view_projections[0] = m_cascade_data.view_projection_matrices[0];
    out_view_projections[1] = m_cascade_data.view_projection_matrices[1];
    out_view_projections[2] = m_cascade_data.view_projection_matrices[2];
    out_view_projections[3] = m_cascade_data.view_projection_matrices[3];
    far_planes.x            = m_cascade_data.far_planes[0];
    far_planes.y            = m_cascade_data.far_planes[1];
    far_planes.z            = m_cascade_data.far_planes[2];
    far_planes.w            = m_cascade_data.far_planes[3];
    cascade_info.x          = m_cascade_data.split_depth[1];
    cascade_info.y          = m_cascade_data.split_depth[2];
    cascade_info.z          = m_cascade_data.split_depth[3];
    cascade_info.w          = static_cast<float>(m_resolution);
    if (m_shadow_map_cascade_count < 4)
        far_planes[m_shadow_map_cascade_count] = far_planes.x - 10.0f; // set the not valid cascade smaller then the first value.
}

void shadow_map_step::on_ui_widget()
{
    // Resolution 512, 1024, 2048, 4096
    const char* resolutions = " 512 \0 1024 \0 2048 \0 4096 \0\0";
    int32 r                 = m_resolution;
    int32 current           = r > 2048 ? 3 : (r > 1024 ? 2 : (r > 512 ? 1 : 0));
    ImGui::Combo("Shadow Map Resolution##shadow_step", &current, resolutions);
    m_resolution = 512 * static_cast<int32>(glm::pow(2, current));
    if (m_resolution != r)
        m_shadow_buffer->resize(m_resolution, m_resolution);
    // Offset 0.0 - 100.0
    ImGui::SliderFloat("Shadow Map Offset##shadow_step", &m_shadowmap_offset, 0.0f, 100.0f);

    // Cascades 1, 2, 3, 4
    int32 tmp_c = m_shadow_map_cascade_count;
    ImGui::SliderInt("Number Of Shadow Cascades##shadow_step", &m_shadow_map_cascade_count, 1, 4);
    m_dirty_cascades = tmp_c != m_shadow_map_cascade_count;
    float tmp_l      = m_cascade_data.lambda;
    ImGui::SliderFloat("Cascade Splits Lambda##shadow_step", &m_cascade_data.lambda, 0.0f, 1.0f);
    m_dirty_cascades |= tmp_l != m_cascade_data.lambda;
}
