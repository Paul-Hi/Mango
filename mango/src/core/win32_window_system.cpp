//! \file      win32_window_system.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#define GLFW_INCLUDE_NONE // for glad
#include <GLFW/glfw3.h>
#include <core/input_system_impl.hpp>
#include <core/win32_window_system.hpp>
#include <mango/profile.hpp>

using namespace mango;

win32_window_system::win32_window_system(const shared_ptr<context_impl>& context)
    : m_window_configuration()
{
    m_shared_context                      = context;
    m_platform_data                       = std::make_shared<platform_data>();
    m_platform_data->native_window_handle = nullptr;
}

win32_window_system::~win32_window_system() {}

bool win32_window_system::create()
{
    PROFILE_ZONE;
    if (!glfwInit())
    {
        MANGO_LOG_ERROR("Initilization of glfw failed! No window is created!");
        return false;
    }

    int32 width       = m_window_configuration.get_width();
    int32 height      = m_window_configuration.get_height();
    const char* title = m_window_configuration.get_title();

    // Hints valid for all windows
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);

    GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (!window)
    {
        const char* description;
        glfwGetError(&description);
        MANGO_LOG_ERROR("glfwCreateWindow failed! No window is created! Error: {0}", description);
        return false;
    }
    m_platform_data->native_window_handle = static_cast<void*>(window);

    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    int32 pos_x             = mode->width / 2 - width / 2;
    int32 pos_y             = mode->height / 2 - height / 2;
    glfwSetWindowPos(window, pos_x, pos_y);

    MANGO_LOG_DEBUG("Window Position is ({0}, {1})", pos_x, pos_y);
    MANGO_LOG_DEBUG("Window Size is {0} x {1}", width, height);

    return true;
}

void win32_window_system::swap_buffers()
{
    MANGO_ASSERT(m_platform_data->native_window_handle, "Window Handle is not valid!");
    glfwSwapBuffers(static_cast<GLFWwindow*>(m_platform_data->native_window_handle));
    GL_PROFILE_COLLECT;
}

void win32_window_system::set_size(int32 width, int32 height)
{
    MANGO_ASSERT(m_platform_data->native_window_handle, "Window Handle is not valid!");
    m_window_configuration.set_width(width);
    m_window_configuration.set_height(height);
    glfwSetWindowSize(static_cast<GLFWwindow*>(m_platform_data->native_window_handle), width, height);
}

void win32_window_system::configure(const window_configuration& configuration)
{
    PROFILE_ZONE;
    MANGO_ASSERT(m_platform_data->native_window_handle, "Window Handle is not valid!");
    glfwDestroyWindow(static_cast<GLFWwindow*>(m_platform_data->native_window_handle));

    m_window_configuration = configuration;

    int32 width       = m_window_configuration.get_width();
    int32 height      = m_window_configuration.get_height();
    const char* title = m_window_configuration.get_title();

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (!window)
    {
        const char* description;
        glfwGetError(&description);
        MANGO_LOG_ERROR("glfwCreateWindow failed! No window is created! Error: {0}", description);
        return;
    }
    m_platform_data->native_window_handle = static_cast<void*>(window);

    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    int32 pos_x             = mode->width / 2 - width / 2;
    int32 pos_y             = mode->height / 2 - height / 2;
    glfwSetWindowPos(window, pos_x, pos_y);

    MANGO_LOG_DEBUG("Window Position is ({0}, {1})", pos_x, pos_y);
    MANGO_LOG_DEBUG("Window Size is {0} x {1}", width, height);

#ifdef MANGO_DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif // MANGO_DEBUG

    // TODO Paul: There has to be a cleaner solution for this. Right now the window configuration has to be done before any input related stuff.
    auto input = m_shared_context->get_input_system_internal().lock();
    MANGO_ASSERT(input, "Input system not valid!");
    input->set_platform_data(m_platform_data);

    make_window_context_current();
    m_shared_context->set_gl_loading_procedure(reinterpret_cast<mango_gl_load_proc>(glfwGetProcAddress)); // TODO Paul: Should this be done here or before creating the gl context.
}

void win32_window_system::update(float dt)
{
    MANGO_UNUSED(dt);
}

void win32_window_system::poll_events()
{
    glfwPollEvents();
}

bool win32_window_system::should_close()
{
    MANGO_ASSERT(m_platform_data->native_window_handle, "Window Handle is not valid!");
    return glfwWindowShouldClose(static_cast<GLFWwindow*>(m_platform_data->native_window_handle));
}

void win32_window_system::set_vsync(bool enabled)
{
    make_window_context_current();
    glfwSwapInterval(enabled ? 1 : 0);
}

void win32_window_system::make_window_context_current()
{
    MANGO_ASSERT(m_platform_data->native_window_handle, "Window Handle is not valid!");
    glfwMakeContextCurrent(static_cast<GLFWwindow*>(m_platform_data->native_window_handle));
}

void win32_window_system::destroy()
{
    MANGO_ASSERT(m_platform_data->native_window_handle, "Window Handle is not valid!");
    glfwDestroyWindow(static_cast<GLFWwindow*>(m_platform_data->native_window_handle));
    m_platform_data->native_window_handle = nullptr;
    glfwTerminate();
}