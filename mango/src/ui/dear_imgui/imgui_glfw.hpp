//! \file      imgui_glfw.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef IMGUI_GLFW_HPP
#define IMGUI_GLFW_HPP

#include <imgui.h>

//! \cond NO_COND

struct GLFWwindow;
struct GLFWmonitor;

IMGUI_IMPL_API bool ImGui_ImplGlfw_InitForOpenGL(GLFWwindow* window, bool install_callbacks);
IMGUI_IMPL_API bool ImGui_ImplGlfw_InitForVulkan(GLFWwindow* window, bool install_callbacks);
IMGUI_IMPL_API void ImGui_ImplGlfw_Shutdown();
IMGUI_IMPL_API void ImGui_ImplGlfw_NewFrame();
IMGUI_IMPL_API void ImGui_ImplGlfw_FrameHovered(bool hovered);
IMGUI_IMPL_API void ImGui_ImplGlfw_FrameFocused(bool focused);

IMGUI_IMPL_API void ImGui_ImplGlfw_MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
IMGUI_IMPL_API void ImGui_ImplGlfw_ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
IMGUI_IMPL_API void ImGui_ImplGlfw_KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
IMGUI_IMPL_API void ImGui_ImplGlfw_CharCallback(GLFWwindow* window, unsigned int c);
IMGUI_IMPL_API void ImGui_ImplGlfw_MonitorCallback(GLFWmonitor* monitor, int event);

//! \endcond

#endif // IMGUI_GLFW_HPP
