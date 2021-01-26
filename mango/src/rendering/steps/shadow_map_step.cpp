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
#include <mango/imgui_helper.hpp>
#include <mango/profile.hpp>
#include <mango/render_system.hpp>
#include <rendering/steps/shadow_map_step.hpp>
#include <util/helpers.hpp>

using namespace mango;

bool shadow_map_step::create()
{
    PROFILE_ZONE;

    m_cascade_data.lambda = 0.65f;

    bool success = true;
    success      = success & setup_buffers();
    success      = success & setup_shader_programs();

    return success;
}

bool shadow_map_step::setup_shader_programs()
{
    PROFILE_ZONE;
    shader_configuration shader_config;
    shader_config.path            = "res/shader/shadow/v_shadow_pass.glsl";
    shader_config.type            = shader_type::vertex_shader;
    shader_ptr shadow_pass_vertex = shader::create(shader_config);
    if (!check_creation(shadow_pass_vertex.get(), "shadow pass vertex shader"))
        return false;

    shader_config.path              = "res/shader/shadow/g_shadow_pass.glsl";
    shader_config.type              = shader_type::geometry_shader;
    shader_ptr shadow_pass_geometry = shader::create(shader_config);
    if (!check_creation(shadow_pass_geometry.get(), "shadow pass geometry shader"))
        return false;

    shader_config.path              = "res/shader/shadow/f_shadow_pass.glsl";
    shader_config.type              = shader_type::fragment_shader;
    shader_ptr shadow_pass_fragment = shader::create(shader_config);
    if (!check_creation(shadow_pass_fragment.get(), "shadow pass fragment shader"))
        return false;

    m_shadow_pass = shader_program::create_graphics_pipeline(shadow_pass_vertex, nullptr, nullptr, shadow_pass_geometry, shadow_pass_fragment);
    if (!check_creation(m_shadow_pass.get(), "shadow pass shader program"))
        return false;
    return true;
}

bool shadow_map_step::setup_buffers()
{
    PROFILE_ZONE;
    m_shadow_command_buffer = command_buffer<max_key>::create(524288 * 2); // 1 MiB?

    texture_configuration shadow_map_config;
    shadow_map_config.generate_mipmaps        = 1;
    shadow_map_config.is_standard_color_space = false;
    shadow_map_config.texture_min_filter      = texture_parameter::filter_nearest;
    shadow_map_config.texture_mag_filter      = texture_parameter::filter_nearest;
    shadow_map_config.texture_wrap_s          = texture_parameter::wrap_clamp_to_edge;
    shadow_map_config.texture_wrap_t          = texture_parameter::wrap_clamp_to_edge;
    shadow_map_config.layers                  = max_shadow_mapping_cascades;

    framebuffer_configuration fb_config;
    fb_config.depth_attachment = texture::create(shadow_map_config);
    fb_config.depth_attachment->set_data(format::depth_component24, m_shadow_data.resolution, m_shadow_data.resolution, format::depth_component, format::t_float, nullptr);
    fb_config.width  = m_shadow_data.resolution;
    fb_config.height = m_shadow_data.resolution;

    m_shadow_buffer = framebuffer::create(fb_config);
    if (!check_creation(m_shadow_buffer.get(), "shadow buffer"))
        return false;

    return true;
}

void shadow_map_step::update(float dt)
{
    MANGO_UNUSED(dt);
}

void shadow_map_step::attach() {}

void shadow_map_step::configure(const shadow_step_configuration& configuration)
{
    m_shadow_data.resolution                  = configuration.get_resolution();
    m_shadow_data.sample_count                = configuration.get_sample_count();
    m_shadow_data.light_size                  = configuration.get_light_size();
    m_shadow_map_offset                       = configuration.get_offset();
    m_shadow_data.cascade_count               = configuration.get_cascade_count();
    m_shadow_data.slope_bias                  = configuration.get_slope_bias();
    m_shadow_data.normal_bias                 = configuration.get_normal_bias();
    m_shadow_data.filter_mode                 = static_cast<int32>(configuration.get_filter_mode());
    m_cascade_data.lambda                     = configuration.get_split_lambda();
    m_shadow_data.cascade_interpolation_range = configuration.get_cascade_interpolation_range();
    MANGO_ASSERT(m_shadow_data.resolution % 2 == 0, "Shadow Map Resolution has to be a multiple of 2!");
    MANGO_ASSERT(m_shadow_data.sample_count >= 16 && m_shadow_data.sample_count <= 64, "Sample count is not in valid range 16 - 64!");
    MANGO_ASSERT(m_shadow_data.cascade_count > 0 && m_shadow_data.cascade_count < 5, "Cascade count has to be between 1 and 4!");
    MANGO_ASSERT(m_cascade_data.lambda > 0.0f && m_cascade_data.lambda < 1.0f, "Lambda has to be between 0.0 and 1.0!");
    m_dirty_cascades = true;
}

void shadow_map_step::execute(gpu_buffer_ptr frame_uniform_buffer)
{
    PROFILE_ZONE;

    max_key k = command_keys::create_key<max_key>(command_keys::key_template::max_key_material_front_to_back);
    command_keys::add_base_mode(k, command_keys::base_mode::to_front);
    bind_framebuffer_command* bf = m_shadow_command_buffer->create<bind_framebuffer_command>(k);
    bf->framebuffer_name         = m_shadow_buffer->get_name();

    bind_shader_program_command* bsp = m_shadow_command_buffer->append<bind_shader_program_command, bind_framebuffer_command>(bf);
    bsp->shader_program_name         = m_shadow_pass->get_name();

    set_viewport_command* sv = m_shadow_command_buffer->append<set_viewport_command, bind_shader_program_command>(bsp);
    sv->x                    = 0;
    sv->y                    = 0;
    sv->width                = m_shadow_data.resolution;
    sv->height               = m_shadow_data.resolution;

    set_face_culling_command* sfc = m_shadow_command_buffer->append<set_face_culling_command, set_viewport_command>(sv);
    sfc->enabled                  = false;

    set_polygon_offset_command* spo = m_shadow_command_buffer->append<set_polygon_offset_command, set_face_culling_command>(sfc);
    spo->factor                     = 1.1f;
    spo->units                      = 4.0f;

    bind_buffer_command* bb = m_shadow_command_buffer->append<bind_buffer_command, set_polygon_offset_command>(spo);
    bb->target              = buffer_target::uniform_buffer;
    bb->index               = UB_SLOT_SHADOW_DATA;
    bb->size                = sizeof(shadow_data);
    bb->buffer_name         = frame_uniform_buffer->buffer_name();
    bb->offset              = frame_uniform_buffer->write_data(sizeof(shadow_data), &m_shadow_data);
}

void shadow_map_step::destroy() {}

void shadow_map_step::update_cascades(float dt, float camera_near, float camera_far, const glm::mat4& camera_view_projection, const glm::vec3& directional_direction)
{
    // Update only with 30 fps
    static float fps_lock = 0.0f;
    fps_lock += dt;
    if (fps_lock * 1000.0f < 1.0f / 30.0f)
        return;
    fps_lock -= 1.0f / 30.0f;

    m_cascade_data.camera_near           = camera_near;
    m_cascade_data.camera_far            = camera_far;
    m_cascade_data.directional_direction = directional_direction;

    auto near = camera_near;
    auto far  = camera_far;

    if (m_dirty_cascades || (glm::abs(m_shadow_data.split_depth[0] - near) > 1e-5f) || (glm::abs(m_shadow_data.split_depth[m_shadow_data.cascade_count] - far) > 1e-5f))
    {
        m_dirty_cascades                                       = false;
        m_shadow_data.split_depth[0]                           = near;
        m_shadow_data.split_depth[m_shadow_data.cascade_count] = far;
        for (int32 i = 1; i < m_shadow_data.cascade_count; ++i)
        {
            float p                      = static_cast<float>(i) / static_cast<float>(m_shadow_data.cascade_count);
            float log                    = near * std::pow((far / near), p);
            float uniform                = near + (far - near) * p;
            float C_i                    = m_cascade_data.lambda * log + (1.0f - m_cascade_data.lambda) * uniform;
            m_shadow_data.split_depth[i] = C_i;
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

    for (int32 casc = 0; casc < m_shadow_data.cascade_count; ++casc)
    {
        glm::vec3 center = glm::vec3(0.0f);
        glm::vec3 current_frustum_corners[8];
        for (int32 i = 0; i < 4; ++i)
        {
            glm::vec3 corner_ray = frustum_corners[i + 4] - frustum_corners[i];
            glm::vec3 near_ray =
                corner_ray * static_cast<float>((m_shadow_data.split_depth[casc] - m_shadow_data.cascade_interpolation_range) / m_shadow_data.split_depth[m_shadow_data.cascade_count]);
            glm::vec3 far_ray =
                corner_ray * static_cast<float>((m_shadow_data.split_depth[casc + 1] + m_shadow_data.cascade_interpolation_range) / m_shadow_data.split_depth[m_shadow_data.cascade_count]);
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
        glm::vec3 up = GLOBAL_UP;
        if (1.0f - glm::dot(up, glm::normalize(m_cascade_data.directional_direction)) < 1e-5f)
            up = GLOBAL_RIGHT;
        view                           = glm::lookAt(center + glm::normalize(m_cascade_data.directional_direction) * (-min_value.z + m_shadow_map_offset), center, up);
        projection                     = glm::ortho(min_value.x, max_value.x, min_value.y, max_value.y, 0.0f, (max_value.z - min_value.z) + m_shadow_map_offset);
        m_shadow_data.far_planes[casc] = (max_value.z - min_value.z) + m_shadow_map_offset;

        glm::mat4 shadow_matrix = projection * view;
        glm::vec4 origin        = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        origin                  = shadow_matrix * origin;
        origin *= static_cast<float>(m_shadow_data.resolution) * 0.5f;

        glm::vec4 offset = glm::round(origin) - origin;
        offset *= 2.0f / static_cast<float>(m_shadow_data.resolution);
        offset.z = 0.0f;
        offset.w = 0.0f;
        projection[3] += offset;

        m_shadow_data.view_projection_matrices[casc] = projection * view;
    }
}

void shadow_map_step::on_ui_widget()
{
    ImGui::PushID("shadow_step");
    // Resolution 512, 1024, 2048, 4096
    const char* resolutions[4] = { "512", "1024", "2048", "4096" };
    int32 r                    = m_shadow_data.resolution;
    int32 current              = r > 2048 ? 3 : (r > 1024 ? 2 : (r > 512 ? 1 : 0));
    combo("Shadow Map Resolution", resolutions, 4, current, 2);
    m_shadow_data.resolution = 512 * static_cast<int32>(glm::pow(2, current));
    if (m_shadow_data.resolution != r)
        m_shadow_buffer->resize(m_shadow_data.resolution, m_shadow_data.resolution);

    // Filter Type
    const char* filter[4] = { "Hard Shadows", "Softer Shadows", "Soft Shadows", "PCSS Shadows" };
    int32& current_filter = m_shadow_data.filter_mode;
    combo("Shadow Filter Mode", filter, 4, current_filter, 1);

    int32 default_ivalue[1] = { 16 };
    float default_value[1]  = { 4.0f };

    if (m_shadow_data.filter_mode > 0)
    {
        int32& sample_count = m_shadow_data.sample_count;
        slider_int_n("Sample Count", &sample_count, 1, default_ivalue, 16, 64);
        if (m_shadow_data.filter_mode > 1)
        {
            float& l_size = m_shadow_data.light_size;
            slider_float_n("Light Size", &l_size, 1, default_value, 1.0f, 100.0f);
        }
    }

    // Offset 0.0 - 100.0
    slider_float_n("Shadow Map Offset", &m_shadow_map_offset, 1, default_value, 0.0f, 100.0f);

    float& s_bias    = m_shadow_data.slope_bias;
    default_value[0] = 0.005f;
    drag_float_n("Shadow Map Slope Bias", &s_bias, 1, default_value, 0.001f, 0.0f, 0.5f);

    float& n_bias    = m_shadow_data.normal_bias;
    default_value[0] = 0.01f;
    drag_float_n("Shadow Map Normal Bias", &n_bias, 1, default_value, 0.001f, 0.0f, 0.5f);

    // Cascades 1, 2, 3, 4
    int32& shadow_cascades = m_shadow_data.cascade_count;
    default_ivalue[0]      = 3;
    slider_int_n("Number Of Shadow Cascades", &shadow_cascades, 1, default_ivalue, 1, 4);
    float& interpolation_range = m_shadow_data.cascade_interpolation_range;
    default_value[0]           = 0.5f;
    slider_float_n("Cascade Interpolation Range", &interpolation_range, 1, default_value, 0.0f, 10.0f);
    slider_float_n("Cascade Splits Lambda", &m_cascade_data.lambda, 1, default_value, 0.0f, 1.0f);
    m_dirty_cascades = true; // For now always in debug.
    ImGui::PopID();
}
