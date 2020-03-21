//! \file      win32_window_system.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#define GLFW_INCLUDE_NONE // for glad
#include <GLFW/glfw3.h>
#include <core/win32_window_system.hpp>
#include <mango/assert.hpp>

using namespace mango;

win32_window_system::win32_window_system(const shared_ptr<context_impl>& context)
    : m_window_configuration()
    , m_window_handle(nullptr)
{
    m_shared_context = context;
}

win32_window_system::~win32_window_system() {}

bool win32_window_system::create()
{
    if (!glfwInit())
    {
        MANGO_LOG_ERROR("Initilization of glfw failed! No window is created!");
        return false;
    }

    uint32 width      = m_window_configuration.get_width();
    uint32 height     = m_window_configuration.get_height();
    const char* title = m_window_configuration.get_title();

    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (!window)
    {
        MANGO_LOG_ERROR("glfwCreateWindow failed! No window is created!");
        return false;
    }
    m_window_handle = static_cast<void*>(window);

    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    uint32 pos_x            = mode->width / 2 - width / 2;
    uint32 pos_y            = mode->height / 2 - height / 2;
    glfwSetWindowPos(window, pos_x, pos_y);

    MANGO_LOG_DEBUG("Window Position is ({0}, {1})", pos_x, pos_y);
    MANGO_LOG_DEBUG("Window Size is {0} x {1}", width, height);

    return true;
}

void win32_window_system::swap_buffers()
{
    MANGO_ASSERT(m_window_handle, "Window Handle is not valid!");
    glfwSwapBuffers(static_cast<GLFWwindow*>(m_window_handle));
}

void win32_window_system::configure(const window_configuration& configuration)
{
    MANGO_ASSERT(m_window_handle, "Window Handle is not valid!");
    glfwDestroyWindow(static_cast<GLFWwindow*>(m_window_handle));

    m_window_configuration = configuration;

    uint32 width      = m_window_configuration.get_width();
    uint32 height     = m_window_configuration.get_height();
    const char* title = m_window_configuration.get_title();

    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);

    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    uint32 pos_x            = mode->width / 2 - width / 2;
    uint32 pos_y            = mode->height / 2 - height / 2;
    glfwSetWindowPos(window, pos_x, pos_y);

    MANGO_LOG_DEBUG("Window Position is ({0}, {1})", pos_x, pos_y);
    MANGO_LOG_DEBUG("Window Size is {0} x {1}", width, height);

#ifdef MANGO_DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif // MANGO_DEBUG

    glfwMakeContextCurrent(window);                                                                       // TODO Paul: Should this be done here or before creating the gl context.
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
    MANGO_ASSERT(m_window_handle, "Window Handle is not valid!");
    return glfwWindowShouldClose(static_cast<GLFWwindow*>(m_window_handle));
}

void win32_window_system::destroy()
{
    MANGO_ASSERT(m_window_handle, "Window Handle is not valid!");
    glfwDestroyWindow(static_cast<GLFWwindow*>(m_window_handle));
    m_window_handle = nullptr;
    glfwTerminate();
}