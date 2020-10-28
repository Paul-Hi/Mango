//! \file      win32_input_system.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <core/win32_input_system.hpp>
#define GLFW_INCLUDE_NONE // for glad
#include <GLFW/glfw3.h>
#include <graphics/command_buffer.hpp>
#include <mango/assert.hpp>
#include <mango/profile.hpp>
#include <mango/scene.hpp>
#include <rendering/render_system_impl.hpp>

using namespace mango;

win32_input_system::win32_input_system(const shared_ptr<context_impl>& context)
{
    m_input_user_data.shared_context      = context;
    m_platform_data                       = std::make_shared<platform_data>();
    m_platform_data->native_window_handle = nullptr;
    m_last_scroll_offset                  = glm::vec2(0.0f);
    m_last_mods                           = modifier::none;
}

win32_input_system::~win32_input_system() {}

bool win32_input_system::create()
{
    return true;
}

void win32_input_system::set_platform_data(const shared_ptr<platform_data>& data)
{
    PROFILE_ZONE;
    MANGO_ASSERT(data->native_window_handle, "Invalid platform data!");
    m_platform_data = data;

    auto window = static_cast<GLFWwindow*>(m_platform_data->native_window_handle);

    glfwSetWindowUserPointer(window, static_cast<void*>(&m_input_user_data));

    // Set temporary window size callback.
    glfwSetWindowSizeCallback(window, [](GLFWwindow* window, int w, int h) {
        input_user_data* data = static_cast<input_user_data*>(glfwGetWindowUserPointer(window));
        context_impl* c       = data->shared_context.get();
        if (w > 0 && h > 0)
        {
            auto cam_info = c->get_current_scene()->get_active_camera_data().camera_info;
            if (cam_info)
            {
                cam_info->perspective.aspect = (float)w / (float)h;
                cam_info->orthographic.x_mag = (float)w / (float)h;
                cam_info->orthographic.y_mag = 1.0f;
            }
            c->get_render_system_internal().lock()->set_viewport(0, 0, w, h);
        }
    });

    // set all callbacks
    glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int, int action, int mods) {
        input_user_data* data = static_cast<input_user_data*>(glfwGetWindowUserPointer(window));
        data->key_change(static_cast<key_code>(key), static_cast<input_action>(action), static_cast<modifier>(mods));
    });
    glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods) {
        input_user_data* data = static_cast<input_user_data*>(glfwGetWindowUserPointer(window));
        data->mouse_button_change(static_cast<mouse_button>(button), static_cast<input_action>(action), static_cast<modifier>(mods));
    });
    glfwSetCursorPosCallback(window, [](GLFWwindow* window, double xpos, double ypos) {
        input_user_data* data = static_cast<input_user_data*>(glfwGetWindowUserPointer(window));
        data->mouse_position_change(static_cast<float>(xpos), static_cast<float>(ypos));
    });
    glfwSetScrollCallback(window, [](GLFWwindow* window, double xoffset, double yoffset) {
        input_user_data* data = static_cast<input_user_data*>(glfwGetWindowUserPointer(window));
        data->mouse_scroll_change(static_cast<float>(xoffset), static_cast<float>(yoffset));
    });
    glfwSetDropCallback(window, [](GLFWwindow* window, int count, const char** paths) {
        input_user_data* data = static_cast<input_user_data*>(glfwGetWindowUserPointer(window));
        data->drag_n_drop_change(count, paths);
    });

    // base key callback to get modifiers
    m_input_user_data.key_change.connect([this](key_code, input_action, modifier mods) { m_last_mods = mods; });
    m_input_user_data.mouse_button_change.connect([this](mouse_button, input_action, modifier mods) { m_last_mods = mods; });
    m_input_user_data.mouse_scroll_change.connect([this](float x_offset, float y_offset) { m_last_scroll_offset = glm::vec2(x_offset, y_offset); });
}

void win32_input_system::update(float dt)
{
    MANGO_UNUSED(dt);
}

void win32_input_system::destroy() {}

input_action win32_input_system::get_key(key_code key)
{
    MANGO_ASSERT(m_platform_data->native_window_handle, "Window Handle is not valid!");
    return static_cast<input_action>(glfwGetKey(static_cast<GLFWwindow*>(m_platform_data->native_window_handle), static_cast<int>(key)));
}

input_action win32_input_system::get_mouse_button(mouse_button button)
{
    MANGO_ASSERT(m_platform_data->native_window_handle, "Window Handle is not valid!");
    return static_cast<input_action>(glfwGetMouseButton(static_cast<GLFWwindow*>(m_platform_data->native_window_handle), static_cast<int>(button)));
}

modifier win32_input_system::get_modifiers()
{
    MANGO_ASSERT(m_platform_data->native_window_handle, "Window Handle is not valid!");
    return m_last_mods;
}

glm::vec2 win32_input_system::get_mouse_position()
{
    MANGO_ASSERT(m_platform_data->native_window_handle, "Window Handle is not valid!");
    double x, y;
    glfwGetCursorPos(static_cast<GLFWwindow*>(m_platform_data->native_window_handle), &x, &y);
    return glm::vec2(x, y);
}

glm::vec2 win32_input_system::get_mouse_scroll()
{
    MANGO_ASSERT(m_platform_data->native_window_handle, "Window Handle is not valid!");
    return m_last_scroll_offset;
}

void win32_input_system::set_key_callback(key_callback callback)
{
    MANGO_ASSERT(m_platform_data->native_window_handle, "Window Handle is not valid!");
    m_input_user_data.key_change.connect(callback);
}

void win32_input_system::set_mouse_button_callback(mouse_button_callback callback)
{
    MANGO_ASSERT(m_platform_data->native_window_handle, "Window Handle is not valid!");
    m_input_user_data.mouse_button_change.connect(callback);
}

void win32_input_system::set_mouse_position_callback(mouse_position_callback callback)
{
    MANGO_ASSERT(m_platform_data->native_window_handle, "Window Handle is not valid!");
    m_input_user_data.mouse_position_change.connect(callback);
}

void win32_input_system::set_mouse_scroll_callback(mouse_scroll_callback callback)
{
    MANGO_ASSERT(m_platform_data->native_window_handle, "Window Handle is not valid!");
    m_input_user_data.mouse_scroll_change.connect(callback);
}

void win32_input_system::set_drag_and_drop_callback(drag_n_drop_callback callback)
{
    MANGO_ASSERT(m_platform_data->native_window_handle, "Window Handle is not valid!");
    m_input_user_data.drag_n_drop_change.connect(callback);
}

void win32_input_system::hide_cursor(bool hide)
{
    MANGO_ASSERT(m_platform_data->native_window_handle, "Window Handle is not valid!");
    glfwSetInputMode(static_cast<GLFWwindow*>(m_platform_data->native_window_handle), GLFW_CURSOR, hide ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL);
}
