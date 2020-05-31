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
#include <rendering/steps/ibl_step.hpp>

#ifdef MANGO_DEBUG
static void GLAPIENTRY debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
#endif // MANGO_DEBUG

using namespace mango;

//! \brief Single uniforms for the lighting pass accessed and utilized only internally by the renderer at the moment.
struct lighting_pass_uniforms
{
    glm::mat4 inverse_view_projection; //!< The inverse of the queried camera view projection matrix of the current scene.
    glm::vec3 camera_position;         //!< The position of the queried camera of the current scene.
};

//! \brief Default vertex array object for second pass with geometry shader generated geometry.
vertex_array_ptr default_vao;
//! \brief Default texture that is bound to every texture unit not in use to prevent warnings.
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
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS); // TODO Paul: Better place?

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
    attachment_config.m_generate_mipmaps        = 1;
    attachment_config.m_is_standard_color_space = false;
    attachment_config.m_texture_min_filter      = texture_parameter::FILTER_NEAREST;
    attachment_config.m_texture_mag_filter      = texture_parameter::FILTER_NEAREST;
    attachment_config.m_texture_wrap_s          = texture_parameter::WRAP_CLAMP_TO_EDGE;
    attachment_config.m_texture_wrap_t          = texture_parameter::WRAP_CLAMP_TO_EDGE;

    config.m_color_attachment0 = texture::create(attachment_config);
    config.m_color_attachment0->set_data(format::RGBA8, w, h, format::RGBA, format::UNSIGNED_INT_8_8_8_8, nullptr);
    config.m_color_attachment1 = texture::create(attachment_config);
    config.m_color_attachment1->set_data(format::RGB10_A2, w, h, format::RGBA, format::UNSIGNED_INT_10_10_10_2, nullptr);
    config.m_color_attachment2 = texture::create(attachment_config);
    config.m_color_attachment2->set_data(format::RGBA8, w, h, format::RGBA, format::UNSIGNED_INT_8_8_8_8, nullptr);
    config.m_color_attachment3 = texture::create(attachment_config);
    config.m_color_attachment3->set_data(format::RGBA8, w, h, format::RGBA, format::UNSIGNED_INT_8_8_8_8, nullptr);
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

    // frame uniform buffer
    buffer_configuration uniform_buffer_config(uniform_buffer_size, buffer_target::UNIFORM_BUFFER, buffer_access::MAPPED_ACCESS_WRITE);
    m_frame_uniform_buffer = buffer::create(uniform_buffer_config);
    if (!m_frame_uniform_buffer)
    {
        MANGO_LOG_ERROR("Creation of frame uniform buffer failed! Render system not available!");
        return false;
    }

    m_mapped_uniform_memory = m_frame_uniform_buffer->map(0, m_frame_uniform_buffer->byte_length(), buffer_access::MAPPED_ACCESS_WRITE);
    if (!m_mapped_uniform_memory)
    {
        MANGO_LOG_ERROR("Mapping of uniforms failed! Render system not available!");
        return false;
    }
    m_frame_uniform_offset = 0;

    // scene geometry pass
    shader_configuration shader_config;
    shader_config.m_path = "res/shader/v_scene_gltf.glsl";
    shader_config.m_type = shader_type::VERTEX_SHADER;
    shader_ptr d_vertex  = shader::create(shader_config);
    if (!d_vertex)
    {
        MANGO_LOG_ERROR("Creation of geometry pass vertex shader failed! Render system not available!");
        return false;
    }

    shader_config.m_path  = "res/shader/f_scene_gltf.glsl";
    shader_config.m_type  = shader_type::FRAGMENT_SHADER;
    shader_ptr d_fragment = shader::create(shader_config);
    if (!d_fragment)
    {
        MANGO_LOG_ERROR("Creation of geometry pass fragment shader failed! Render system not available!");
        return false;
    }

    m_scene_geometry_pass = shader_program::create_graphics_pipeline(d_vertex, nullptr, nullptr, nullptr, d_fragment);
    // shader light pass

    shader_config.m_path = "res/shader/v_empty.glsl";
    shader_config.m_type = shader_type::VERTEX_SHADER;
    d_vertex             = shader::create(shader_config);
    if (!d_vertex)
    {
        MANGO_LOG_ERROR("Creation of lighting vertex shader failed! Render system not available!");
        return false;
    }

    shader_config.m_path  = "res/shader/g_create_screen_space_quad.glsl";
    shader_config.m_type  = shader_type::GEOMETRY_SHADER;
    shader_ptr d_geometry = shader::create(shader_config);
    if (!d_geometry)
    {
        MANGO_LOG_ERROR("Creation of lighting geometry shader failed! Render system not available!");
        return false;
    }

    shader_config.m_path = "res/shader/f_deferred_lighting.glsl";
    shader_config.m_type = shader_type::FRAGMENT_SHADER;
    d_fragment           = shader::create(shader_config);
    if (!d_fragment)
    {
        MANGO_LOG_ERROR("Creation of lighting fragment shader failed! Render system not available!");
        return false;
    }

    m_lighting_pass = shader_program::create_graphics_pipeline(d_vertex, nullptr, nullptr, d_geometry, d_fragment);
    if (!m_lighting_pass)
    {
        MANGO_LOG_ERROR("Creation of lighting pass failed! Render system not available!");
        return false;
    }

    // default vao needed
    default_vao = vertex_array::create();
    if (!default_vao)
    {
        MANGO_LOG_ERROR("Creation of default vao failed! Render system not available!");
        return false;
    }
    // default texture needed (config is not relevant)
    default_texture = texture::create(attachment_config);
    if (!default_texture)
    {
        MANGO_LOG_ERROR("Creation of default texture failed! Render system not available!");
        return false;
    }
    g_ubyte zero = 0;
    default_texture->set_data(format::R8, 1, 1, format::RED, format::UNSIGNED_BYTE, &zero);

    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &m_uniform_buffer_alignment);

    return true;
}

void deferred_pbr_render_system::configure(const render_configuration& configuration)
{
    auto ws = m_shared_context->get_window_system_internal().lock();
    MANGO_ASSERT(ws, "Window System is expired!");
    ws->set_vsync(configuration.is_vsync_enabled());

    // additional render steps
    if (configuration.get_render_steps()[mango::render_step::ibl])
    {
        // create an extra object that is capable to create cubemaps from equirectangular hdr, preprocess everything and
        // do all the rendering for the environment.
        auto step_ibl = std::make_shared<ibl_step>();
        step_ibl->create();
        m_pipeline_steps[mango::render_step::ibl] = std::static_pointer_cast<pipeline_step>(step_ibl);
    }
}

void deferred_pbr_render_system::begin_render()
{
    m_command_buffer->set_depth_test(true);
    m_command_buffer->set_depth_func(compare_operation::LESS);
    m_command_buffer->set_face_culling(true);
    m_command_buffer->set_cull_face(polygon_face::FACE_BACK);
    m_command_buffer->bind_framebuffer(m_gbuffer);
    m_command_buffer->clear_framebuffer(clear_buffer_mask::COLOR_AND_DEPTH, attachment_mask::ALL_DRAW_BUFFERS_AND_DEPTH, 0.0f, 0.0f, 0.0f, 0.0f, m_gbuffer);
    m_command_buffer->bind_shader_program(m_scene_geometry_pass);

    auto scene  = m_shared_context->get_current_scene();
    auto camera = scene->get_active_camera_data();
    if (camera.camera_info)
    {
        set_view_projection_matrix(camera.camera_info->view_projection);
    }

    m_command_buffer->wait_for_buffer(m_frame_uniform_buffer);
    // m_command_buffer->set_polygon_mode(polygon_face::FACE_FRONT_AND_BACK, polygon_mode::LINE);
}

void deferred_pbr_render_system::finish_render()
{
    m_command_buffer->bind_vertex_array(nullptr);
    m_command_buffer->bind_shader_program(nullptr);

    m_command_buffer->bind_framebuffer(nullptr); // bind default.
    m_command_buffer->clear_framebuffer(clear_buffer_mask::COLOR_AND_DEPTH_STENCIL, attachment_mask::ALL, 0.0f, 0.0f, 0.2f, 1.0f);
    m_command_buffer->set_polygon_mode(polygon_face::FACE_FRONT_AND_BACK, polygon_mode::FILL);
    m_command_buffer->bind_shader_program(m_lighting_pass);

    auto scene  = m_shared_context->get_current_scene();
    auto camera = scene->get_active_camera_data();
    lighting_pass_uniforms lp_uniforms;
    if (camera.camera_info && camera.transform)
    {
        lp_uniforms.inverse_view_projection = glm::inverse(camera.camera_info->view_projection);
        lp_uniforms.camera_position         = camera.transform->world_transformation_matrix[3];
    }
    m_command_buffer->bind_single_uniform(0, &lp_uniforms.inverse_view_projection, sizeof(lp_uniforms.inverse_view_projection));
    m_command_buffer->bind_single_uniform(1, &lp_uniforms.camera_position, sizeof(lp_uniforms.camera_position));
    m_command_buffer->bind_texture(0, m_gbuffer->get_attachment(framebuffer_attachment::COLOR_ATTACHMENT0), 2);
    m_command_buffer->bind_texture(1, m_gbuffer->get_attachment(framebuffer_attachment::COLOR_ATTACHMENT1), 3);
    m_command_buffer->bind_texture(2, m_gbuffer->get_attachment(framebuffer_attachment::COLOR_ATTACHMENT2), 4);
    m_command_buffer->bind_texture(3, m_gbuffer->get_attachment(framebuffer_attachment::COLOR_ATTACHMENT3), 5);
    m_command_buffer->bind_texture(4, m_gbuffer->get_attachment(framebuffer_attachment::DEPTH_ATTACHMENT), 6);
    if (m_pipeline_steps[mango::render_step::ibl])
        std::static_pointer_cast<ibl_step>(m_pipeline_steps[mango::render_step::ibl])->bind_image_based_light_maps(m_command_buffer);

    // TODO Paul: Check if the binding is better for performance or not.
    m_command_buffer->bind_vertex_array(default_vao);

    m_command_buffer->draw_arrays(primitive_topology::POINTS, 0, 1);

    // We try to reset the default state as possible, without crashing all optimizations.

    m_command_buffer->bind_vertex_array(nullptr);
    // We need to unbind the program so we can make changes to the textures.
    m_command_buffer->bind_shader_program(nullptr);

    if (m_pipeline_steps[mango::render_step::ibl])
    {
        // We need the not translated view for skybox rendering.
        if (camera.camera_info)
            std::static_pointer_cast<ibl_step>(m_pipeline_steps[mango::render_step::ibl])->set_view_projection_matrix(camera.camera_info->projection * glm::mat4(glm::mat3(camera.camera_info->view)));

        m_command_buffer->set_depth_func(compare_operation::LESS_EQUAL);
        m_command_buffer->set_cull_face(polygon_face::FACE_FRONT);
        m_pipeline_steps[mango::render_step::ibl]->execute(m_command_buffer);
    }

    m_command_buffer->lock_buffer(m_frame_uniform_buffer);

    m_command_buffer->execute();

    m_frame_uniform_offset = 0;
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
        buffer_ptr m_uniform_buffer;
        uint32 m_offset;
        set_model_matrix_cmd(buffer_ptr uniform_buffer, uint32 offset)
            : m_uniform_buffer(uniform_buffer)
            , m_offset(offset)
        {
        }

        void execute(graphics_state&) override
        {
            MANGO_ASSERT(m_uniform_buffer, "Uniforms do not exist anymore.");
            m_uniform_buffer->bind(buffer_target::UNIFORM_BUFFER, 0, m_offset, sizeof(scene_vertex_uniforms));
        }
    };

    scene_vertex_uniforms u{ std140_mat4(model_matrix), std140_mat3(glm::transpose(glm::inverse(model_matrix))) };

    MANGO_ASSERT(m_frame_uniform_offset < uniform_buffer_size - sizeof(scene_vertex_uniforms), "Uniform buffer size is too small.");
    memcpy(static_cast<g_byte*>(m_mapped_uniform_memory) + m_frame_uniform_offset, &u, sizeof(scene_vertex_uniforms));

    m_command_buffer->submit<set_model_matrix_cmd>(m_frame_uniform_buffer, m_frame_uniform_offset);
    m_frame_uniform_offset += m_uniform_buffer_alignment;
}

void deferred_pbr_render_system::draw_mesh(const material_ptr& mat, primitive_topology topology, uint32 first, uint32 count, index_type type, uint32 instance_count)
{
    class push_material_cmd : public command
    {
      public:
        buffer_ptr m_uniform_buffer;
        uint32 m_offset;
        push_material_cmd(buffer_ptr uniform_buffer, uint32 offset)
            : m_uniform_buffer(uniform_buffer)
            , m_offset(offset)
        {
        }

        void execute(graphics_state&) override
        {
            MANGO_ASSERT(m_uniform_buffer, "Uniforms do not exist anymore.");
            m_uniform_buffer->bind(buffer_target::UNIFORM_BUFFER, 1, m_offset, sizeof(scene_material_uniforms));
        }
    };

    scene_material_uniforms u;
    auto& state = m_command_buffer->get_state();

    u.base_color = std140_vec4(mat->base_color);
    u.metallic   = (g_float)mat->metallic;
    u.roughness  = (g_float)mat->roughness;
    if (mat->base_color_texture)
    {
        u.base_color_texture = std140_bool(true);
        m_command_buffer->bind_texture(0, mat->base_color_texture, 1);
    }
    else
    {
        u.base_color_texture = std140_bool(false);
        m_command_buffer->bind_texture(0, default_texture, 1);
    }
    if (mat->roughness_metallic_texture)
    {
        u.roughness_metallic_texture = std140_bool(true);
        m_command_buffer->bind_texture(1, mat->roughness_metallic_texture, 2);
    }
    else
    {
        u.roughness_metallic_texture = std140_bool(false);
        m_command_buffer->bind_texture(1, default_texture, 2);
    }
    if (mat->occlusion_texture)
    {
        u.occlusion_texture = std140_bool(true);
        m_command_buffer->bind_texture(2, mat->occlusion_texture, 3);
        u.packed_occlusion = std140_bool(false);
    }
    else
    {
        u.occlusion_texture = std140_bool(false);
        m_command_buffer->bind_texture(2, default_texture, 3);
        // eventually it is packed
        u.packed_occlusion = std140_bool(mat->packed_occlusion);
    }
    if (mat->normal_texture)
    {
        u.normal_texture = std140_bool(true);
        m_command_buffer->bind_texture(3, mat->normal_texture, 4);
    }
    else
    {
        u.normal_texture = std140_bool(false);
        m_command_buffer->bind_texture(3, default_texture, 4);
    }
    if (mat->emissive_color_texture)
    {
        u.emissive_color_texture = std140_bool(true);
        m_command_buffer->bind_texture(4, mat->emissive_color_texture, 5);
    }
    else
    {
        u.emissive_color_texture = std140_bool(false);
        m_command_buffer->bind_texture(4, default_texture, 5);
    }

    MANGO_ASSERT(m_frame_uniform_offset < uniform_buffer_size - sizeof(scene_material_uniforms), "Uniform buffer size is too small.");
    memcpy(static_cast<g_byte*>(m_mapped_uniform_memory) + m_frame_uniform_offset, &u, sizeof(scene_material_uniforms));

    m_command_buffer->submit<push_material_cmd>(m_frame_uniform_buffer, m_frame_uniform_offset);
    m_frame_uniform_offset += m_uniform_buffer_alignment;

    if (mat->double_sided)
        m_command_buffer->set_face_culling(false);

    if (type == index_type::NONE)
        m_command_buffer->draw_arrays(topology, first, count, instance_count);
    else
        m_command_buffer->draw_elements(topology, first, count, type, instance_count);

    m_command_buffer->set_face_culling(true);
}

void deferred_pbr_render_system::set_view_projection_matrix(const glm::mat4& view_projection)
{
    m_command_buffer->bind_single_uniform(0, &const_cast<glm::mat4&>(view_projection), sizeof(glm::mat4));
}

void deferred_pbr_render_system::set_environment_texture(const texture_ptr& hdr_texture, float render_level)
{
    if (m_pipeline_steps[mango::render_step::ibl])
    {
        auto ibl = std::static_pointer_cast<ibl_step>(m_pipeline_steps[mango::render_step::ibl]);
        ibl->load_from_hdr(hdr_texture);
        ibl->set_render_level(render_level);
    }
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
    return;
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
