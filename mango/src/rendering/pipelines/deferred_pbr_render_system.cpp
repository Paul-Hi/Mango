//! \file      deferred_pbr_render_system.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <core/window_system_impl.hpp>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <graphics/buffer.hpp>
#include <graphics/framebuffer.hpp>
#include <graphics/shader.hpp>
#include <graphics/shader_program.hpp>
#include <graphics/texture.hpp>
#include <graphics/vertex_array.hpp>
#include <mango/scene.hpp>
#include <rendering/pipelines/deferred_pbr_render_system.hpp>

#ifdef MANGO_DEBUG
static void GLAPIENTRY debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
#endif // MANGO_DEBUG

using namespace mango;

struct lighting_pass_uniforms
{
    glm::mat4 inverse_view_projection;
    glm::vec3 camera_position;
};

vertex_array_ptr default_vao;
texture_ptr default_texture;

deferred_pbr_render_system::deferred_pbr_render_system(const shared_ptr<context_impl>& context)
    : render_system_impl(context)
{
}

deferred_pbr_render_system::~deferred_pbr_render_system() {}

bool deferred_pbr_render_system::create()
{
    m_shared_context->make_current();
    GLADloadproc proc = static_cast<GLADloadproc>(m_shared_context->get_gl_loading_procedure());
    if (!gladLoadGLLoader(proc))
    {
        MANGO_LOG_ERROR("Initilization of glad failed! No opengl context is available!");
        return false;
    }

#ifdef MANGO_DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(debugCallback, 0);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
    // glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_OTHER, 0, GL_DEBUG_SEVERITY_NOTIFICATION, -1, "Test Message GLDebug!");
#endif // MANGO_DEBUG

    shared_ptr<window_system_impl> ws = m_shared_context->get_window_system_internal().lock();
    MANGO_ASSERT(ws, "Window System is expireds!");
    uint32 w = ws->get_width();
    uint32 h = ws->get_height();

    framebuffer_configuration config;
    texture_configuration attachment_config;
    attachment_config.m_generate_mipmaps        = false;
    attachment_config.m_is_standard_color_space = false;
    attachment_config.m_texture_min_filter      = texture_parameter::FILTER_NEAREST;
    attachment_config.m_texture_mag_filter      = texture_parameter::FILTER_NEAREST;
    attachment_config.m_texture_wrap_s          = texture_parameter::WRAP_CLAMP_TO_EDGE;
    attachment_config.m_texture_wrap_t          = texture_parameter::WRAP_CLAMP_TO_EDGE;

    config.m_color_attachment0 = texture::create(attachment_config);
    config.m_color_attachment0->set_data(format::RGBA8, w, h, format::RGBA, format::UNSIGNED_BYTE, nullptr);
    config.m_color_attachment1 = texture::create(attachment_config);
    config.m_color_attachment1->set_data(format::RGBA8, w, h, format::RGBA, format::UNSIGNED_BYTE, nullptr);
    config.m_color_attachment2 = texture::create(attachment_config);
    config.m_color_attachment2->set_data(format::RGBA8, w, h, format::RGBA, format::UNSIGNED_BYTE, nullptr);
    config.m_depth_attachment = texture::create(attachment_config);
    // glTextureParameteri(config.m_depth_attachment->get_name(), GL_TEXTURE_COMPARE_MODE, GL_NONE);
    // glTextureParameteri(config.m_depth_attachment->get_name(), GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);
    config.m_depth_attachment->set_data(format::DEPTH_COMPONENT32F, w, h, format::DEPTH_COMPONENT, format::FLOAT, nullptr);

    config.m_width  = w;
    config.m_height = h;

    m_gbuffer = framebuffer::create(config);

    if (!m_gbuffer)
    {
        MANGO_LOG_ERROR("Creation of gbuffer failed! Render system not available!");
        return false;
    }

    // scene uniform buffers
    buffer_configuration v_buffer_config(sizeof(scene_vertex_uniforms), buffer_target::UNIFORM_BUFFER, buffer_access::MAPPED_ACCESS_WRITE);
    m_scene_vertex_uniform_buffer  = buffer::create(v_buffer_config);
    m_active_scene_vertex_uniforms = static_cast<scene_vertex_uniforms*>(m_scene_vertex_uniform_buffer->map(0, m_scene_vertex_uniform_buffer->byte_length(), buffer_access::MAPPED_ACCESS_WRITE));

    buffer_configuration m_buffer_config(sizeof(scene_material_uniforms), buffer_target::UNIFORM_BUFFER, buffer_access::MAPPED_ACCESS_WRITE);
    m_scene_material_uniform_buffer = buffer::create(m_buffer_config);
    m_active_scene_material_uniforms =
        static_cast<scene_material_uniforms*>(m_scene_material_uniform_buffer->map(0, m_scene_material_uniform_buffer->byte_length(), buffer_access::MAPPED_ACCESS_WRITE));
    // scene geometry pass

    shader_configuration shader_config;
    shader_config.m_path = "res/shader/v_scene_gltf.glsl";
    shader_config.m_type = shader_type::VERTEX_SHADER;
    shader_ptr d_vertex  = shader::create(shader_config);

    shader_config.m_path  = "res/shader/f_scene_gltf.glsl";
    shader_config.m_type  = shader_type::FRAGMENT_SHADER;
    shader_ptr d_fragment = shader::create(shader_config);

    m_scene_geometry_pass = shader_program::create_graphics_pipeline(d_vertex, nullptr, nullptr, nullptr, d_fragment);
    // shader light pass

    shader_config.m_path = "res/shader/v_empty.glsl";
    shader_config.m_type = shader_type::VERTEX_SHADER;
    d_vertex             = shader::create(shader_config);

    shader_config.m_path  = "res/shader/g_create_screen_space_quad.glsl";
    shader_config.m_type  = shader_type::GEOMETRY_SHADER;
    shader_ptr d_geometry = shader::create(shader_config);

    shader_config.m_path = "res/shader/f_deferred_lighting.glsl";
    shader_config.m_type = shader_type::FRAGMENT_SHADER;
    d_fragment           = shader::create(shader_config);

    m_lighting_pass = shader_program::create_graphics_pipeline(d_vertex, nullptr, nullptr, d_geometry, d_fragment);

    // default vao needed
    default_vao = vertex_array::create();
    // default texture needed (config is not relevant)
    default_texture = texture::create(attachment_config);
    default_texture->set_data(format::R8, w, h, format::R8, format::UNSIGNED_BYTE, nullptr);

    return true;
}

void deferred_pbr_render_system::configure(const render_configuration& configuration)
{
    auto ws = m_shared_context->get_window_system_internal().lock();
    MANGO_ASSERT(ws, "Window System is expired!");
    ws->set_vsync(configuration.is_vsync_enabled());
}

void deferred_pbr_render_system::begin_render()
{
    m_command_buffer->set_depth_test(true);
    m_command_buffer->set_cull_face(polygon_face::FACE_BACK);
    m_command_buffer->bind_framebuffer(m_gbuffer);
    m_command_buffer->clear_framebuffer(clear_buffer_mask::COLOR_AND_DEPTH, attachment_mask::ALL_DRAW_BUFFERS_AND_DEPTH, 0.0f, 0.0f, 0.0f, 0.0f, m_gbuffer);
    m_command_buffer->bind_shader_program(m_scene_geometry_pass);

    auto scene  = m_shared_context->get_current_scene();
    auto camera = scene->get_active_camera_data();
    if (camera && camera->camera_info && camera->transform)
    {
        m_active_scene_vertex_uniforms->view_projection = camera->camera_info->view_projection;
    }

    m_command_buffer->bind_uniform_buffer(0, m_scene_vertex_uniform_buffer);
    m_command_buffer->bind_uniform_buffer(1, m_scene_material_uniform_buffer);

    // m_command_buffer->set_polygon_mode(polygon_face::FACE_FRONT_AND_BACK, polygon_mode::LINE);
}

void deferred_pbr_render_system::finish_render()
{
    m_command_buffer->bind_framebuffer(nullptr); // bind default.
    m_command_buffer->clear_framebuffer(clear_buffer_mask::COLOR_AND_DEPTH_STENCIL, attachment_mask::ALL, 1.0f, 0.8f, 0.133f, 1.0f);
    m_command_buffer->set_polygon_mode(polygon_face::FACE_FRONT_AND_BACK, polygon_mode::FILL);
    m_command_buffer->bind_shader_program(m_lighting_pass);

    auto scene  = m_shared_context->get_current_scene();
    auto camera = scene->get_active_camera_data();
    lighting_pass_uniforms lp_uniforms;
    if (camera && camera->camera_info && camera->transform)
    {
        lp_uniforms.inverse_view_projection = glm::inverse(camera->camera_info->view_projection);
        lp_uniforms.camera_position         = camera->transform->local_transformation_matrix[3];
    }
    else
    {
        MANGO_LOG_ERROR("No active camera is set! Rendering will be broken!");
    }
    m_command_buffer->bind_single_uniform(0, &lp_uniforms.inverse_view_projection, sizeof(lp_uniforms.inverse_view_projection));
    m_command_buffer->bind_single_uniform(1, &lp_uniforms.camera_position, sizeof(lp_uniforms.camera_position));
    m_command_buffer->bind_texture(0, m_gbuffer->get_attachment(framebuffer_attachment::COLOR_ATTACHMENT0), 2);
    m_command_buffer->bind_texture(1, m_gbuffer->get_attachment(framebuffer_attachment::COLOR_ATTACHMENT1), 3);
    m_command_buffer->bind_texture(2, m_gbuffer->get_attachment(framebuffer_attachment::COLOR_ATTACHMENT2), 4);
    m_command_buffer->bind_texture(3, m_gbuffer->get_attachment(framebuffer_attachment::DEPTH_ATTACHMENT), 5);

    // TODO Paul: Check if the binding is better for performance or not.
    m_command_buffer->bind_vertex_array(default_vao);

    m_command_buffer->draw_arrays(primitive_topology::POINTS, 0, 1);

    // We try to reset the default state as possible, without crashing all optimizations.
    m_command_buffer->bind_texture(0, nullptr, 2);
    m_command_buffer->bind_texture(1, nullptr, 3);
    m_command_buffer->bind_texture(2, nullptr, 4);
    m_command_buffer->bind_texture(3, nullptr, 5);
    m_command_buffer->bind_vertex_array(nullptr);
    // We need to unbind the program so we can make changes to the textures.
    m_command_buffer->bind_shader_program(nullptr);

    m_command_buffer->execute();
}

void deferred_pbr_render_system::set_viewport(uint32 x, uint32 y, uint32 width, uint32 height)
{
    m_command_buffer->set_viewport(x, y, width, height);
    m_gbuffer->resize(width, height);
}

void deferred_pbr_render_system::update(float dt)
{
    MANGO_UNUSED(dt);
}

void deferred_pbr_render_system::destroy() {}

render_pipeline deferred_pbr_render_system::get_base_render_pipeline()
{
    return render_pipeline::deferred_pbr;
}

void deferred_pbr_render_system::set_model_matrix(const glm::mat4& model_matrix)
{
    class set_model_matrix_cmd : public command
    {
      public:
        scene_vertex_uniforms* m_uniform_ptr;
        glm::mat4 m_model_matrix;
        set_model_matrix_cmd(scene_vertex_uniforms* uniform_ptr, const glm::mat4& model_matrix)
            : m_uniform_ptr(uniform_ptr)
            , m_model_matrix(model_matrix)
        {
        }

        void execute(graphics_state&) override
        {
            MANGO_ASSERT(m_uniform_ptr, "Uniforms do not exist anymore.");
            m_uniform_ptr->model_matrix  = std140_mat4(m_model_matrix);
            m_uniform_ptr->normal_matrix = std140_mat3(glm::transpose(glm::inverse(m_model_matrix)));
        }
    };

    m_command_buffer->submit<set_model_matrix_cmd>(m_active_scene_vertex_uniforms, model_matrix);
}

void deferred_pbr_render_system::push_material(const material_ptr& mat)
{
    class push_material_cmd : public command
    {
      public:
        scene_material_uniforms* m_uniform_ptr;
        material_ptr m_mat;
        push_material_cmd(scene_material_uniforms* uniform_ptr, const material_ptr& mat)
            : m_uniform_ptr(uniform_ptr)
            , m_mat(mat)
        {
        }

        void execute(graphics_state& state) override
        {
            MANGO_ASSERT(m_uniform_ptr, "Uniforms do not exist anymore.");
            m_uniform_ptr->base_color = std140_vec4(m_mat->base_color);
            m_uniform_ptr->metallic   = (g_float)m_mat->metallic;
            m_uniform_ptr->roughness  = (g_float)m_mat->roughness;
            if (m_mat->base_color_texture)
            {
                m_uniform_ptr->base_color_texture = true;
                m_mat->base_color_texture->bind_texture_unit(0);
                state.bind_texture(0, m_mat->base_color_texture->get_name());
                glUniform1i(0, 0);
            }
            else
            {
                m_uniform_ptr->base_color_texture = false;
                default_texture->bind_texture_unit(0);
                state.bind_texture(0, default_texture->get_name());
            }
            if (m_mat->metallic_texture)
            {
                m_uniform_ptr->metallic_texture = true;
                m_mat->metallic_texture->bind_texture_unit(1);
                state.bind_texture(1, m_mat->metallic_texture->get_name());
                glUniform1i(1, 1);
            }
            else
            {
                m_uniform_ptr->metallic_texture = false;
                default_texture->bind_texture_unit(1);
                state.bind_texture(1, default_texture->get_name());
            }
            if (m_mat->roughness_texture)
            {
                m_uniform_ptr->roughness_texture = true;
                m_mat->roughness_texture->bind_texture_unit(2);
                state.bind_texture(2, m_mat->roughness_texture->get_name());
                glUniform1i(2, 2);
            }
            else
            {
                m_uniform_ptr->roughness_texture = false;
                default_texture->bind_texture_unit(2);
                state.bind_texture(2, default_texture->get_name());
            }
            if (m_mat->normal_texture)
            {
                m_uniform_ptr->normal_texture = true;
                m_mat->normal_texture->bind_texture_unit(3);
                state.bind_texture(3, m_mat->normal_texture->get_name());
                glUniform1i(3, 3);
            }
            else
            {
                m_uniform_ptr->normal_texture = false;
                default_texture->bind_texture_unit(3);
                state.bind_texture(3, default_texture->get_name());
            }
        }
    };

    m_command_buffer->submit<push_material_cmd>(m_active_scene_material_uniforms, mat);
}

#ifdef MANGO_DEBUG

static const char* getStringForType(GLenum type)
{
    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR:
        return "Error";
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        return "Deprecated behavior";
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        return "Undefined behavior";
    case GL_DEBUG_TYPE_PORTABILITY:
        return "Portability issue";
    case GL_DEBUG_TYPE_PERFORMANCE:
        return "Performance issue";
    case GL_DEBUG_TYPE_MARKER:
        return "Stream annotation";
    case GL_DEBUG_TYPE_OTHER:
        return "Other";
    default:
        assert(false);
        return "";
    }
}

static const char* getStringForSource(GLenum source)
{
    switch (source)
    {
    case GL_DEBUG_SOURCE_API:
        return "API";
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        return "Window system";
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
        return "Shader compiler";
    case GL_DEBUG_SOURCE_THIRD_PARTY:
        return "Third party";
    case GL_DEBUG_SOURCE_APPLICATION:
        return "Application";
    case GL_DEBUG_SOURCE_OTHER:
        return "Other";
    default:
        assert(false);
        return "";
    }
}

static const char* getStringForSeverity(GLenum severity)
{
    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH:
        return "High";
    case GL_DEBUG_SEVERITY_MEDIUM:
        return "Medium";
    case GL_DEBUG_SEVERITY_LOW:
        return "Low";
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        return "Notification";
    default:
        assert(false);
        return ("");
    }
}

static void GLAPIENTRY debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    (void)id;
    (void)length;
    (void)userParam;
    std::cout << "\n--------------OpenGL Debug Output--------------" << std::endl;
    std::cout << "Source: " << getStringForSource(source) << std::endl;
    std::cout << "Type: " << getStringForType(type) << std::endl;
    std::cout << "Severity: " << getStringForSeverity(severity) << std::endl;
    std::cout << "Debug call: " << message << std::endl;
    std::cout << "-----------------------------------------------" << std::endl;
}

#endif // MANGO_DEBUG
