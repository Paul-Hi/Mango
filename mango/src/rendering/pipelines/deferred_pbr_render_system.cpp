//! \file      deferred_pbr_render_system.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <glad/glad.h>
#include <rendering/pipelines/deferred_pbr_render_system.hpp>

#ifdef MANGO_DEBUG
static void GLAPIENTRY debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
#endif // MANGO_DEBUG

using namespace mango;

deferred_pbr_render_system::deferred_pbr_render_system(const shared_ptr<context_impl>& context)
    : render_system_impl(context)
{
    MANGO_UNUSED(context);
    m_command_queue = std::queue<render_command>();
}

deferred_pbr_render_system::~deferred_pbr_render_system() {}

bool deferred_pbr_render_system::create()
{
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
    // glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
    // glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_OTHER, 0, GL_DEBUG_SEVERITY_NOTIFICATION, -1, "Test Message GLDebug!");
#endif // MANGO_DEBUG

    return true;
}

void deferred_pbr_render_system::configure(const render_configuration& configuration)
{
    MANGO_UNUSED(configuration);
}

void deferred_pbr_render_system::start_frame()
{
    // clear command queue
    std::queue<render_command>().swap(m_command_queue);
}

void deferred_pbr_render_system::submit(const render_command& command)
{
    m_command_queue.push(command);
}

void deferred_pbr_render_system::finish_frame() {}

void deferred_pbr_render_system::render()
{
    // TODO Paul: This handling should not be done in an if-else. This function has to be done before continuing other work!
    for (; !m_command_queue.empty(); m_command_queue.pop())
    {
        render_command& command = m_command_queue.front();
        if (command.type == render_command_type::draw_call)
        {
            draw_call_data* data = static_cast<draw_call_data*>(command.data);
            if (data->state.changed)
                updateState(data->state);
            // state updates
            if (m_render_state.depth == depth_less)
                glEnable(GL_DEPTH_TEST);
            if (m_render_state.depth == depth_off)
                glDisable(GL_DEPTH_TEST);
            if (m_render_state.cull == cull_backface)
                glEnable(GL_CULL_FACE);
            if (m_render_state.cull == cull_off)
                glDisable(GL_CULL_FACE);
            if (m_render_state.wireframe == wireframe_off)
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            if (m_render_state.wireframe == wireframe_on)
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            if (m_render_state.blending == blend_off)
                glDisable(GL_BLEND);

            if (data->gpu_call == gpu_draw_call::clear_call)
            {
                glClearColor(m_render_state.color_clear.r, m_render_state.color_clear.g, m_render_state.color_clear.b, m_render_state.color_clear.a);
                glClear(GL_COLOR_BUFFER_BIT);
            }
        }
    }
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
