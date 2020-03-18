//! \file      linux_window_system.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <GLFW/glfw3.h>
#include <core/linux_window_system.hpp>
#include <mango/assert.hpp>

using namespace mango;

linux_window_system::linux_window_system()
    : m_window_configuration()
    , m_window_handle(nullptr)
{
}

linux_window_system::~linux_window_system() {}

bool linux_window_system::create()
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

void linux_window_system::swap_buffers()
{
    MANGO_ASSERT(m_window_handle, "Window Handle is not valid!");
    glfwSwapBuffers(static_cast<GLFWwindow*>(m_window_handle));
}

void linux_window_system::configure(const window_configuration& configuration)
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
}

void linux_window_system::update(float dt)
{
    MANGO_UNUSED(dt);
}

void linux_window_system::poll_events()
{
    glfwPollEvents();
}

bool linux_window_system::should_close()
{
    MANGO_ASSERT(m_window_handle, "Window Handle is not valid!");
    return glfwWindowShouldClose(static_cast<GLFWwindow*>(m_window_handle));
}

void linux_window_system::destroy()
{
    MANGO_ASSERT(m_window_handle, "Window Handle is not valid!");
    glfwDestroyWindow(static_cast<GLFWwindow*>(m_window_handle));
    glfwTerminate();
}