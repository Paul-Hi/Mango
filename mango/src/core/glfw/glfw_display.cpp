//! \file      glfw_display.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#include <core/glfw/glfw_display.hpp>
#include <mango/log.hpp>
#include <mango/profile.hpp>

using namespace mango;

int32 glfw_display::s_glfw_windows = 0;

static void glfw_error_callback(int error_code, const char* description)
{
    MANGO_LOG_ERROR("GLFW Error {0}: {1}", error_code, description);
}

glfw_display::glfw_display(const display_info& create_info)
{
    m_glfw_display_data.info        = create_info;
    m_glfw_display_data.initialized = initialize();
}

glfw_display::~glfw_display()
{
    if (m_glfw_display_data.native_handle)
    {
        glfwDestroyWindow(m_glfw_display_data.native_handle);
        s_glfw_windows--;
    }
    if (!s_glfw_windows)
        glfwTerminate();
}

void glfw_display::change_size(int32 width, int32 height)
{
    MANGO_ASSERT(m_glfw_display_data.native_handle, "Display native handle is not valid!");
    m_glfw_display_data.info.width  = width;
    m_glfw_display_data.info.height = height;
    glfwSetWindowSize(m_glfw_display_data.native_handle, m_glfw_display_data.info.width, m_glfw_display_data.info.height);
}

void glfw_display::quit()
{
    MANGO_ASSERT(m_glfw_display_data.native_handle, "Display native handle is not valid!");
    glfwSetWindowShouldClose(m_glfw_display_data.native_handle, GLFW_TRUE);
}

bool glfw_display::is_initialized() const
{
    return m_glfw_display_data.initialized;
}

int32 glfw_display::get_x_position() const
{
    return m_glfw_display_data.info.x;
}

int32 glfw_display::get_y_position() const
{
    return m_glfw_display_data.info.y;
}

int32 glfw_display::get_width() const
{
    return m_glfw_display_data.info.width;
}

int32 glfw_display::get_height() const
{
    return m_glfw_display_data.info.height;
}

const char* glfw_display::get_title() const
{
    return m_glfw_display_data.info.title;
}

bool glfw_display::is_decorated() const
{
    return m_glfw_display_data.info.decorated;
}

display_configuration::native_renderer_type glfw_display::get_native_renderer_type() const
{
    return m_glfw_display_data.info.native_renderer;
}

void glfw_display::poll_events() const
{
    glfwPollEvents(); // TODO Paul: This seems to be not instance related, or is it?
}

bool glfw_display::should_close() const
{
    MANGO_ASSERT(m_glfw_display_data.native_handle, "Display native handle is not valid!");
    return glfwWindowShouldClose(m_glfw_display_data.native_handle);
}

display_impl::native_window_handle glfw_display::native_handle() const
{
    MANGO_ASSERT(m_glfw_display_data.native_handle, "Display native handle is not valid!");
    return m_glfw_display_data.native_handle;
}

bool glfw_display::initialize()
{
    PROFILE_ZONE;
    if (!s_glfw_windows) // Only intialize once, even though we could have multiple displays.
    {
        if (!glfwInit())
        {
            MANGO_LOG_ERROR("Initilization of glfw failed! Display can not be created!");
            return false;
        }
    }
    switch (m_glfw_display_data.info.native_renderer)
    {
    case display_configuration::native_renderer_type::opengl:
        return create_glfw_opengl();
    default:
        MANGO_LOG_ERROR("Unknown native renderer type. Display can not be created!");
        return false;
    }
}

bool glfw_display::create_glfw_opengl()
{
    PROFILE_ZONE;

    // OpenGL version hints
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    // OpenGL profile
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef MANGO_DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif // MANGO_DEBUG

    // Swap Chain Buffer Hints
    glfwWindowHint(GLFW_RED_BITS, 8);
    glfwWindowHint(GLFW_GREEN_BITS, 8);
    glfwWindowHint(GLFW_BLUE_BITS, 8);
    if (m_glfw_display_data.info.pixel_format == display_info::pixel_format::rgba8888)
        glfwWindowHint(GLFW_ALPHA_BITS, 8);

    switch (m_glfw_display_data.info.depth_stencil_format)
    {
    case display_info::depth_stencil_format::depth16:
        glfwWindowHint(GLFW_DEPTH_BITS, 16);
        break;
    case display_info::depth_stencil_format::depth24:
        glfwWindowHint(GLFW_DEPTH_BITS, 24);
        break;
    case display_info::depth_stencil_format::depth16_stencil8:
        glfwWindowHint(GLFW_DEPTH_BITS, 16);
        glfwWindowHint(GLFW_STENCIL_BITS, 8);
        break;
    case display_info::depth_stencil_format::depth24_stencil8:
        glfwWindowHint(GLFW_DEPTH_BITS, 24);
        glfwWindowHint(GLFW_STENCIL_BITS, 8);
        break;
    default:
        MANGO_LOG_ERROR("Unknown depth stencil format. Disabling depth stencil!");
        break;
    }

    glfwWindowHint(GLFW_DECORATED, m_glfw_display_data.info.decorated ? GLFW_TRUE : GLFW_FALSE);

    // GLFWmonitor* monitor = glfwGetPrimaryMonitor();

    m_glfw_display_data.native_handle = glfwCreateWindow(m_glfw_display_data.info.width, m_glfw_display_data.info.height, m_glfw_display_data.info.title, NULL, NULL);
    if (!m_glfw_display_data.native_handle)
    {
        const char* description;
        glfwGetError(&description);
        MANGO_LOG_ERROR("glfwCreateWindow failed! No display is created! Error: {0}", description);
        return false;
    }

    glfwSetWindowUserPointer(m_glfw_display_data.native_handle, &m_glfw_display_data);

    glfwSetWindowPos(m_glfw_display_data.native_handle, m_glfw_display_data.info.x, m_glfw_display_data.info.y);

    // TODO Paul: Fancy icon please?
    // glfwSetWindowIcon(window, 2, images);

    // Event callback setup
    if (m_glfw_display_data.info.display_event_handler)
    {
        // Error callback
        glfwSetErrorCallback(&glfw_error_callback);

        //
        // Window callbacks.
        //

        // Window position callback
        glfwSetWindowPosCallback(m_glfw_display_data.native_handle, [](GLFWwindow* window, int x_pos, int y_pos) {
            glfw_display_data* data = static_cast<glfw_display_data*>(glfwGetWindowUserPointer(window));
            data->info.x          = x_pos;
            data->info.y          = y_pos;
            data->info.display_event_handler->on_window_position(x_pos, x_pos);
        });

        // Window size callback
        glfwSetWindowSizeCallback(m_glfw_display_data.native_handle, [](GLFWwindow* window, int w, int h) {
            glfw_display_data* data = static_cast<glfw_display_data*>(glfwGetWindowUserPointer(window));
            data->info.width      = w;
            data->info.height     = h;
            data->info.display_event_handler->on_window_resize(w, h);
        });

        // Window close callback
        glfwSetWindowCloseCallback(m_glfw_display_data.native_handle, [](GLFWwindow* window) {
            glfw_display_data* data = static_cast<glfw_display_data*>(glfwGetWindowUserPointer(window));
            data->info.display_event_handler->on_window_close();
        });

        // Window refresh callback
        glfwSetWindowRefreshCallback(m_glfw_display_data.native_handle, [](GLFWwindow* window) {
            glfw_display_data* data = static_cast<glfw_display_data*>(glfwGetWindowUserPointer(window));
            data->info.display_event_handler->on_window_refresh();
        });

        // Window focus callback
        glfwSetWindowFocusCallback(m_glfw_display_data.native_handle, [](GLFWwindow* window, int focused) {
            glfw_display_data* data = static_cast<glfw_display_data*>(glfwGetWindowUserPointer(window));
            data->info.display_event_handler->on_window_focus(focused);
        });

        // Window iconify callback
        glfwSetWindowIconifyCallback(m_glfw_display_data.native_handle, [](GLFWwindow* window, int iconified) {
            glfw_display_data* data = static_cast<glfw_display_data*>(glfwGetWindowUserPointer(window));
            data->info.display_event_handler->on_window_iconify(iconified);
        });

        // Window maximize callback
        glfwSetWindowMaximizeCallback(m_glfw_display_data.native_handle, [](GLFWwindow* window, int maximized) {
            glfw_display_data* data = static_cast<glfw_display_data*>(glfwGetWindowUserPointer(window));
            data->info.display_event_handler->on_window_maximize(maximized);
        });

        // Window framebuffer size callback
        glfwSetFramebufferSizeCallback(m_glfw_display_data.native_handle, [](GLFWwindow* window, int w, int h) {
            glfw_display_data* data = static_cast<glfw_display_data*>(glfwGetWindowUserPointer(window));
            data->info.display_event_handler->on_window_framebuffer_resize(w, h);
        });

        // Window content scale callback
        glfwSetWindowContentScaleCallback(m_glfw_display_data.native_handle, [](GLFWwindow* window, float x_scale, float y_scale) {
            glfw_display_data* data = static_cast<glfw_display_data*>(glfwGetWindowUserPointer(window));
            data->info.display_event_handler->on_window_content_scale(x_scale, y_scale);
        });

        //
        // Input callbacks.
        //

        // Mouse button callback
        glfwSetMouseButtonCallback(m_glfw_display_data.native_handle, [](GLFWwindow* window, int button, int action, int mods) {
            glfw_display_data* data = static_cast<glfw_display_data*>(glfwGetWindowUserPointer(window));
            data->info.display_event_handler->on_input_mouse_button(static_cast<mouse_button>(button), static_cast<input_action>(action), static_cast<modifier>(mods));
        });

        // Cursor position callback
        glfwSetCursorPosCallback(m_glfw_display_data.native_handle, [](GLFWwindow* window, double x_pos, double y_pos) {
            glfw_display_data* data = static_cast<glfw_display_data*>(glfwGetWindowUserPointer(window));
            data->info.display_event_handler->on_input_cursor_position(x_pos, y_pos);
        });

        // Cursor enter callback
        glfwSetCursorEnterCallback(m_glfw_display_data.native_handle, [](GLFWwindow* window, int entered) {
            glfw_display_data* data = static_cast<glfw_display_data*>(glfwGetWindowUserPointer(window));
            data->info.display_event_handler->on_input_cursor_enter(entered);
        });

        // Scroll callback
        glfwSetScrollCallback(m_glfw_display_data.native_handle, [](GLFWwindow* window, double x_off, double y_off) {
            glfw_display_data* data = static_cast<glfw_display_data*>(glfwGetWindowUserPointer(window));
            data->info.display_event_handler->on_input_scroll(x_off, y_off);
        });

        // Key callback
        glfwSetKeyCallback(m_glfw_display_data.native_handle, [](GLFWwindow* window, int key, int, int action, int mods) {
            glfw_display_data* data = static_cast<glfw_display_data*>(glfwGetWindowUserPointer(window));
            data->info.display_event_handler->on_input_key(static_cast<key_code>(key), static_cast<input_action>(action), static_cast<modifier>(mods));
        });

        // Drop callback
        glfwSetDropCallback(m_glfw_display_data.native_handle, [](GLFWwindow* window, int path_count, const char** paths) {
            glfw_display_data* data = static_cast<glfw_display_data*>(glfwGetWindowUserPointer(window));
            data->info.display_event_handler->on_input_drop(path_count, paths);
        });
    }

    MANGO_LOG_DEBUG("Created new opengl display!");
    MANGO_LOG_DEBUG("Display Position is ({0}, {1})", m_glfw_display_data.info.x, m_glfw_display_data.info.y);
    MANGO_LOG_DEBUG("Display Size is {0} x {1}", m_glfw_display_data.info.width, m_glfw_display_data.info.height);

    s_glfw_windows++;

    return true;
}