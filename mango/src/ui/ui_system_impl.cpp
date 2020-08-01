//! \file      ui_system_impl.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#define GLFW_INCLUDE_NONE // for glad
#include <GLFW/glfw3.h>
#include <core/window_system_impl.hpp>
#include <glad/glad.h>
#include <imgui.h>
#include <ui/dear_imgui/imgui_glfw.hpp>
#include <ui/dear_imgui/imgui_opengl3.hpp>
#include <ui/ui_system_impl.hpp>

using namespace mango;

ui_system_impl::ui_system_impl(const shared_ptr<context_impl>& context)
{
    m_shared_context = context;
}

ui_system_impl::~ui_system_impl() {}

bool ui_system_impl::create()
{
    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;   // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport / Platform Windows
    // io.ConfigViewportsNoAutoMerge = true;
    // io.ConfigViewportsNoTaskBarIcon = true;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsClassic();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding              = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    return true;
}

void ui_system_impl::configure(const ui_configuration& configuration)
{
    MANGO_UNUSED(configuration);

    // We need to do this here, because before that we can not be sure to have GL bindings.
    auto platform_data = m_shared_context->get_window_system_internal().lock()->get_platform_data();

    ImGui_ImplGlfw_InitForOpenGL(static_cast<GLFWwindow*>(platform_data->native_window_handle), true);
    ImGui_ImplOpenGL3_Init();
}

void ui_system_impl::update(float dt)
{
    MANGO_UNUSED(dt);

    ImGuiIO& io = ImGui::GetIO(); (void)io;
     ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::ShowDemoWindow();
}

void ui_system_impl::destroy()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ui_system_impl::draw_ui()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }
}
