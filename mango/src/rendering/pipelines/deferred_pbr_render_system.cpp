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

static void setUniform(std::pair<gpu_resource_type, uint32>& binding, void* value);

deferred_pbr_render_system::deferred_pbr_render_system(const shared_ptr<context_impl>& context)
    : render_system_impl(context)
{
    MANGO_UNUSED(context);
    m_command_queue = std::queue<render_command>();
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

void deferred_pbr_render_system::finish_frame()
{
    // TODO Paul: Add all the deferred pipeline steps to the command queue.
    // It should look like this in a first version:
    // Before -> The queue is filled with some simple scene.
    // m_command_queue = { clear, shader_bind, vao_bind, draw}
    // Now -> We add input and output from the pipeline.
    // m_command_queue = { out_framebuffer_bind(gbuffer),
    //                     clear, shader_bind, vao_bind, draw,
    //                     out_framebuffer_bind(backbuffer),
    //                     in_framebuffer_textures_bind(gbuffer),
    //                     shader_bind(copy_shader), draw }
    // This queue now has a geometry pass and a lighting pass IS a deferred pipeline.
    // Left to think about: What to do, if custom geometry shaders do not have all the gbuffer outputs?
    // Left to insert: Check, if there is a custom shader, if not add standard or material specific geometry shaders.
}

void deferred_pbr_render_system::render()
{
    // TODO Paul: This handling should not be done in an if-else.
    for (; !m_command_queue.empty(); m_command_queue.pop())
    {
        render_command& command = m_command_queue.front();
        switch (command.type)
        {
        case vao_binding:
        {
            vao_binding_data* data = static_cast<vao_binding_data*>(command.data);
            glBindVertexArray(data->handle);
            break;
        }
        case shader_program_binding:
        {
            shader_program_binding_data* data = static_cast<shader_program_binding_data*>(command.data);
            glUseProgram(data->handle);
            m_current_binding_data = &(data->binding_data);
            break;
        }
        case input_binding: // TODO Paul
        {
            resource_binding_data* data = static_cast<resource_binding_data*>(command.data);
            MANGO_UNUSED(data);
            // TODO Paul: Check with the shaders binding_data.
            break;
        }
        case output_binding: // TODO Paul
        {
            resource_binding_data* data = static_cast<resource_binding_data*>(command.data);
            MANGO_UNUSED(data);
            // TODO Paul: Check with the shaders binding_data.
            break;
        }
        case uniform_binding:
        {
            uniform_binding_data* data = static_cast<uniform_binding_data*>(command.data);
            MANGO_UNUSED(data);
            auto binding = m_current_binding_data->find(data->binding_name);
            if (binding != m_current_binding_data->end())
            {
                setUniform(binding->second, data->value);
            }
            break;
        }
        case draw_call:
        {
            draw_call_data* data = static_cast<draw_call_data*>(command.data);
            if (data->state.changed)
                updateState(data->state);

            if (data->gpu_call == clear_call)
            {
                glClearColor(m_render_state.color_clear.r, m_render_state.color_clear.g, m_render_state.color_clear.b, m_render_state.color_clear.a);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // TODO Paul: This should be specified in the command as well I guess.
            }
            else // geometry draw calls
            {
                // clang-format off
                GLenum mode = data->gpu_primitive == triangles      ? GL_TRIANGLES      :
                              data->gpu_primitive == triangle_strip ? GL_TRIANGLE_STRIP :
                              data->gpu_primitive == lines          ? GL_LINES          :
                              data->gpu_primitive == line_strip     ? GL_LINE_STRIP     :
                              data->gpu_primitive == points         ? GL_POINTS         :
                                                                      GL_INVALID_ENUM;
                // clang-format on

                if (mode == GL_INVALID_ENUM)
                    return; // We don't print warnings and just ignore strange or invalid commands.

                switch (data->gpu_call)
                {
                case draw_arrays:
                    glDrawArrays(mode, 0, data->count);
                    break;
                case draw_elements:
                    glDrawElements(mode, data->count, GL_UNSIGNED_INT, 0); // TODO Paul: Always UNSIGNED_INT?
                    break;
                case draw_arrays_instanced:
                    glDrawArraysInstanced(mode, 0, data->count, data->instances);
                    break;
                case draw_elements_instanced:
                    glDrawElementsInstanced(mode, data->count, GL_UNSIGNED_INT, 0, data->instances); // TODO Paul: Always UNSIGNED_INT?
                    break;
                default: // We don't print warnings and just ignore strange or invalid commands.
                    break;
                }
            }
            break;
        }
        default:
            break;
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

void deferred_pbr_render_system::updateState(const render_state& state)
{
    render_system_impl::updateState(state);

    // depth
    if (m_render_state.depth == depth_off)
        glDisable(GL_DEPTH_TEST);
    if (m_render_state.depth == depth_less)
    {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
    }

    // culling
    if (m_render_state.cull == cull_off)
        glDisable(GL_CULL_FACE);
    if (m_render_state.cull == cull_backface)
    {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    }
    if (m_render_state.cull == cull_frontface)
    {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
    }

    // wireframe
    if (m_render_state.wireframe == wireframe_off)
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    if (m_render_state.wireframe == wireframe_on)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // blending
    if (m_render_state.blending == blend_off)
        glDisable(GL_BLEND);
    if (m_render_state.blending == blend_src_alpha_and_one_minus_src_aplha)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
}

static void setUniform(std::pair<gpu_resource_type, uint32>& binding, void* value)
{
    switch (binding.first)
    {
    case gpu_float:
    {
        glUniform1f(binding.second, *static_cast<float*>(value));
        break;
    }
    case gpu_vec2:
    {
        float* vec2 = static_cast<float*>(value);
        glUniform2f(binding.second, vec2[0], vec2[1]);
        break;
    }
    case gpu_vec3:
    {
        float* vec3 = static_cast<float*>(value);
        glUniform3f(binding.second, vec3[0], vec3[1], vec3[2]);
        break;
    }
    case gpu_vec4:
    {
        float* vec4 = static_cast<float*>(value);
        glUniform4f(binding.second, vec4[0], vec4[1], vec4[2], vec4[3]);
        break;
    }

    case gpu_int:
    {
        glUniform1i(binding.second, *static_cast<int*>(value));
        break;
    }
    case gpu_ivec2:
    {
        int* vec2 = static_cast<int*>(value);
        glUniform2i(binding.second, vec2[0], vec2[1]);
        break;
    }
    case gpu_ivec3:
    {
        int* vec3 = static_cast<int*>(value);
        glUniform3i(binding.second, vec3[0], vec3[1], vec3[2]);
        break;
    }
    case gpu_ivec4:
    {
        int* vec4 = static_cast<int*>(value);
        glUniform4i(binding.second, vec4[0], vec4[1], vec4[2], vec4[3]);
        break;
    }
    case gpu_mat3:
    {
        float* mat3 = static_cast<float*>(value);
        glUniformMatrix3fv(binding.second, 1, GL_FALSE, mat3);
        break;
    }
    case gpu_mat4:
    {
        float* mat4 = static_cast<float*>(value);
        glUniformMatrix4fv(binding.second, 1, GL_FALSE, mat4);
        break;
    }
    case gpu_sampler_texture_2d: // TODO Paul: Check if this is correct.
    {
        glUniform1i(binding.second, binding.second);
        glActiveTexture(GL_TEXTURE0 + binding.second);
        glBindTexture(GL_TEXTURE_2D, *static_cast<uint32*>(value));
        break;
    }
    case gpu_sampler_texture_cube: // TODO Paul: Implement!
    {
        break;
    }

    default:
        break;
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
