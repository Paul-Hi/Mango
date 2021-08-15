//! \file      gl_graphics_device.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#include <graphics/opengl/gl_graphics_device.hpp>
#include <graphics/opengl/gl_graphics_device_context.hpp>
#include <graphics/opengl/gl_graphics_resources.hpp>
#define GLFW_INCLUDE_NONE // Do not include gl headers, will be done by ourselfs later on.
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <mango/profile.hpp>

using namespace mango;

#ifdef MANGO_DEBUG
static void GLAPIENTRY debugCallback(gl_enum source, gl_enum type, uint32 id, gl_enum severity, int32 length, const char* message, const void* userParam);
#endif // MANGO DEBUG

gl_graphics_device::gl_graphics_device(display_impl::native_window_handle display_window_handle)
    : m_display_window_handle(display_window_handle)
{
    MANGO_ASSERT(m_display_window_handle, "Native window handle is invalid! Can not create gl_graphics_device!");
    glfwMakeContextCurrent(static_cast<GLFWwindow*>(m_display_window_handle));
    GLADloadproc proc = reinterpret_cast<GLADloadproc>(glfwGetProcAddress);

    if (!gladLoadGLLoader(proc))
    {
        MANGO_LOG_CRITICAL("Initilization of glad failed! No opengl context is available!");
        return;
    }
    GL_PROFILED_CONTEXT;

    MANGO_LOG_INFO("-------------------------------------------");
    MANGO_LOG_INFO("--  API: OpenGL                            ");
    MANGO_LOG_INFO("--  Version: {0}                           ", glGetString(GL_VERSION));
    MANGO_LOG_INFO("--  Shader Version: {0}                    ", glGetString(GL_SHADING_LANGUAGE_VERSION));
    MANGO_LOG_INFO("--  Vendor: {0}                            ", glGetString(GL_VENDOR));
    MANGO_LOG_INFO("--  Renderer: {0}                          ", glGetString(GL_RENDERER));
#ifdef MANGO_DEBUG
    MANGO_LOG_INFO("-------------------------------------------");
    MANGO_LOG_INFO("--  Debug Context Enabled                  ");
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(debugCallback, 0);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
    MANGO_LOG_INFO("--  GL Debug Output Enabled               ");
#endif // MANGO DEBUG
    MANGO_LOG_INFO("-------------------------------------------");

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS); // TODO Paul: This should at least be a specified feature!

    m_shared_graphics_state = make_gfx_handle<gl_graphics_state>();
    m_shader_program_cache  = make_gfx_handle<gl_shader_program_cache>();
    m_framebuffer_cache     = make_gfx_handle<gl_framebuffer_cache>();
    m_vertex_array_cache    = make_gfx_handle<gl_vertex_array_cache>();

    // In OpenGL we can not get render target textures for the default framebuffer, so lets fake them.
    texture_create_info info;
    int32 dimensions[4] = { 0 };
    glGetIntegerv(GL_VIEWPORT, dimensions);
    info.width        = dimensions[2];
    info.height       = dimensions[3];
    info.texture_type = gfx_texture_type::texture_type_2d;

    // TODO Paul: At the moment we know we have rgb8 or rgba8. Is this always true?
    int32 alpha_bits = 0;
    glGetNamedFramebufferAttachmentParameteriv(0, GL_BACK_LEFT, GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE, &alpha_bits);
    if (alpha_bits == 0)
        info.texture_format = gfx_format::rgb8;
    else
        info.texture_format = gfx_format::rgba8;

    gl_texture render_target;
    render_target.m_info              = info;
    render_target.m_texture_gl_handle = 0; // 0 is default. // FIXME We need to handle this everywhere.
    m_swap_chain_render_target        = make_gfx_handle<const gl_texture>(render_target);

    int32 depth_bits   = 0;
    int32 stencil_bits = 0;
    glGetNamedFramebufferAttachmentParameteriv(0, GL_BACK_LEFT, GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE, &depth_bits);
    glGetNamedFramebufferAttachmentParameteriv(0, GL_BACK_LEFT, GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE, &stencil_bits);
    if (stencil_bits == 8)
        info.texture_format = gfx_format::depth_stencil; // FIXME
    else if (depth_bits == 16)
        info.texture_format = gfx_format::depth_component16; // FIXME
    else
        info.texture_format = gfx_format::depth_component24; // FIXME

    gl_texture d_s_target;
    d_s_target.m_info                 = info;
    d_s_target.m_texture_gl_handle    = 0; // 0 is default. // FIXME We need to handle this everywhere.
    m_swap_chain_depth_stencil_target = make_gfx_handle<const gl_texture>(d_s_target);
}

gl_graphics_device::~gl_graphics_device() {}

graphics_device_context_handle gl_graphics_device::create_graphics_device_context(bool immediate) const
{
    MANGO_ASSERT(immediate, "Currently only immediate contexts are supported!");
    return mango::make_unique<gl_graphics_device_context>(m_display_window_handle, m_shared_graphics_state, m_shader_program_cache, m_framebuffer_cache, m_vertex_array_cache);
}

gfx_handle<const gfx_shader_stage> gl_graphics_device::create_shader_stage(const shader_stage_create_info& info) const
{
    return make_gfx_handle<const gl_shader_stage>(std::forward<const shader_stage_create_info&>(info));
}

gfx_handle<const pipeline_resource_layout> gl_graphics_device::create_pipeline_resource_layout(std::initializer_list<shader_resource_binding> bindings) const
{
    return make_gfx_handle<const gl_pipeline_resource_layout>(std::forward<std::initializer_list<shader_resource_binding>>(bindings));
}

graphics_pipeline_create_info gl_graphics_device::provide_graphics_pipeline_create_info()
{
    graphics_pipeline_create_info result;
    // Some defaults
    result.vertex_input_state.attribute_description_count = 0;
    result.vertex_input_state.binding_description_count   = 0;
    // result.input_assembly_state.enable_primitive_restart  = false;
    result.input_assembly_state.topology                  = gfx_primitive_topology::primitive_topology_triangle_list;
    result.viewport_state.viewport_count                  = 0;
    result.viewport_state.scissor_count                   = 0;
    result.rasterization_state.enable_depth_bias          = false;
    result.rasterization_state.front_face                 = gfx_front_face::counter_clockwise;
    result.rasterization_state.cull_mode                  = gfx_cull_mode_flag_bits::mode_back;
    result.rasterization_state.line_width                 = 1.0f;
    result.rasterization_state.polygon_mode               = gfx_polygon_mode::polygon_mode_fill;
    result.depth_stencil_state.enable_depth_test          = true;
    result.depth_stencil_state.enable_depth_write         = true;
    result.depth_stencil_state.enable_stencil_test        = false;
    result.depth_stencil_state.depth_compare_operator     = gfx_compare_operator::compare_operator_less;
    result.blend_state.enable_logical_operation           = false;
    result.blend_state.blend_description.enable_blend     = false;
    result.blend_state.blend_description.color_write_mask = gfx_color_component_flag_bits::components_rgba;
    result.blend_state.blend_constants[0]                 = 1.0f;
    result.blend_state.blend_constants[1]                 = 1.0f;
    result.blend_state.blend_constants[2]                 = 1.0f;
    result.blend_state.blend_constants[3]                 = 1.0f;

    return result;
}

compute_pipeline_create_info gl_graphics_device::provide_compute_pipeline_create_info()
{
    compute_pipeline_create_info result;
    // Some defaults

    return result;
}

gfx_handle<const gfx_pipeline> gl_graphics_device::create_graphics_pipeline(const graphics_pipeline_create_info& info) const
{
    return make_gfx_handle<const gl_graphics_pipeline>(std::forward<const graphics_pipeline_create_info&>(info));
}

gfx_handle<const gfx_pipeline> gl_graphics_device::create_compute_pipeline(const compute_pipeline_create_info& info) const
{
    return make_gfx_handle<const gl_compute_pipeline>(std::forward<const compute_pipeline_create_info&>(info));
}

gfx_handle<const gfx_buffer> gl_graphics_device::create_buffer(const buffer_create_info& info) const
{
    return make_gfx_handle<const gl_buffer>(std::forward<const buffer_create_info&>(info));
}

gfx_handle<const gfx_texture> gl_graphics_device::create_texture(const texture_create_info& info) const
{
    return make_gfx_handle<const gl_texture>(std::forward<const texture_create_info&>(info));
}

gfx_handle<const gfx_image_texture_view> gl_graphics_device::create_image_texture_view(gfx_handle<const gfx_texture> texture, int32 level) const
{
    return make_gfx_handle<const gl_image_texture_view>(std::forward<gfx_handle<const gfx_texture>>(texture), std::forward<int32>(level));
}

gfx_handle<const gfx_sampler> gl_graphics_device::create_sampler(const sampler_create_info& info) const
{
    return make_gfx_handle<const gl_sampler>(std::forward<const sampler_create_info&>(info));
}

gfx_handle<const gfx_texture> gl_graphics_device::get_swap_chain_render_target()
{
    return m_swap_chain_render_target;
}

gfx_handle<const gfx_texture> gl_graphics_device::get_swap_chain_depth_stencil_target()
{
    return m_swap_chain_depth_stencil_target;
}

void gl_graphics_device::on_display_framebuffer_resize(int32 width, int32 height)
{
    // Swap chain framebuffers are resized with the window in opengl.
    MANGO_UNUSED(width);
    MANGO_UNUSED(height);
}

#ifdef MANGO_DEBUG
static const char* getStringForType(gl_enum type)
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

static const char* getStringForSource(gl_enum source)
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

static const char* getStringForSeverity(gl_enum severity)
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

static void GLAPIENTRY debugCallback(gl_enum source, gl_enum type, uint32 id, gl_enum severity, int32 length, const char* message, const void* userParam)
{
    (void)id;
    (void)length;
    (void)userParam;
    MANGO_LOG_ERROR("-------------- OpenGL Debug Output --------------");
    MANGO_LOG_ERROR("Source: {0}", getStringForSource(source));
    MANGO_LOG_ERROR("Type: {0}", getStringForType(type));
    MANGO_LOG_ERROR("Severity: {0}", getStringForSeverity(severity));
    MANGO_LOG_ERROR("Debug Message: {0}", message);
    MANGO_LOG_ERROR("------------------------------------------------");
}
#endif // MANGO_DEBUG
