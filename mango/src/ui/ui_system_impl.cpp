//! \file      ui_system_impl.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#define GLFW_INCLUDE_NONE // for glad
#include <GLFW/glfw3.h>
#include <core/input_system_impl.hpp>
#include <core/window_system_impl.hpp>
#include <glad/glad.h>
#include <imgui.h>
#include <mango/application.hpp>
#include <mango/profile.hpp>
#include <ui/dear_imgui/imgui_glfw.hpp>
#include <ui/dear_imgui/imgui_opengl3.hpp>
#include <ui/dear_imgui/imgui_widgets.hpp>
#include <ui/ui_system_impl.hpp>

using namespace mango;

ui_system_impl::ui_system_impl(const shared_ptr<context_impl>& context)
{
    m_shared_context = context;
}

ui_system_impl::~ui_system_impl() {}

bool ui_system_impl::create()
{
    PROFILE_ZONE;
    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport / Platform Windows
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;   // Enable docking for all windows.
    // io.ConfigViewportsNoAutoMerge = true;
    // io.ConfigViewportsNoTaskBarIcon = true;

    io.FontDefault = io.Fonts->AddFontFromFileTTF("res/fonts/OpenSans-Regular.ttf", 18.0);

    static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    ImFontConfig icons_config;
    icons_config.MergeMode  = true;
    icons_config.PixelSnapH = true;
    io.Fonts->AddFontFromFileTTF(FONT_ICON_FILE_NAME_FAS, 14.0f, &icons_config, icons_ranges);

    io.ConfigWindowsMoveFromTitleBarOnly = true; // Only move from title bar

    struct color_setup
    {
        float main_hue = 0.0f / 360.0f;
        float main_sat = 0.0f / 100.0f;
        float main_val = 5.0f / 100.0f;
        float text_hue = 0.0f / 360.0f;
        float text_sat = 0.0f / 100.0f;
        float text_val = 100.0f / 100.0f;
        float back_hue = 0.0f / 360.0f;
        float back_sat = 0.0f / 100.0f;
        float back_val = 50.0f / 100.0f;
        float area_hue = 0.0f / 360.0f;
        float area_sat = 0.0f / 100.0f;
        float area_val = 25.0f / 100.0f;
        float rounding = 0.0f;
    } setup;

    ImVec4 text = ImColor::HSV(setup.text_hue, setup.text_sat, setup.text_val);
    ImVec4 main = ImColor::HSV(setup.main_hue, setup.main_sat, setup.main_val);
    ImVec4 back = ImColor::HSV(setup.back_hue, setup.back_sat, setup.back_val);
    ImVec4 area = ImColor::HSV(setup.area_hue, setup.area_sat, setup.area_val);

    // Setup Dear ImGui style
    ImVec4* colors                         = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text]                  = ImVec4(text.x, text.y, text.z, 1.00f);
    colors[ImGuiCol_TextDisabled]          = ImVec4(text.x, text.y, text.z, 0.58f);
    colors[ImGuiCol_WindowBg]              = ImVec4(area.x, area.y, area.z, 1.00f);
    colors[ImGuiCol_ChildBg]               = ImVec4(area.x, area.y, area.z, 1.00f);
    colors[ImGuiCol_PopupBg]               = ImVec4(area.x * 0.8f, area.y * 0.8f, area.z * 0.8f, 1.00f);
    colors[ImGuiCol_Border]                = ImVec4(text.x, text.y, text.z, 0.30f);
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]               = ImVec4(back.x, back.y, back.z, 0.31f);
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(back.x, back.y, back.z, 0.68f);
    colors[ImGuiCol_FrameBgActive]         = ImVec4(back.x, back.y, back.z, 1.00f);
    colors[ImGuiCol_TitleBg]               = ImVec4(main.x, main.y, main.z, 1.0f);
    colors[ImGuiCol_TitleBgActive]         = ImVec4(main.x, main.y, main.z, 1.0f);
    colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(main.x, main.y, main.z, 1.0f);
    colors[ImGuiCol_MenuBarBg]             = ImVec4(area.x, area.y, area.z, 1.0f);
    colors[ImGuiCol_ScrollbarBg]           = ImVec4(area.x, area.y, area.z, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(main.x, main.y, main.z, 0.31f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(main.x, main.y, main.z, 0.78f);
    colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(main.x, main.y, main.z, 1.00f);
    colors[ImGuiCol_CheckMark]             = ImVec4(text.x, text.y, text.z, 0.80f);
    colors[ImGuiCol_SliderGrab]            = ImVec4(main.x, main.y, main.z, 0.54f);
    colors[ImGuiCol_SliderGrabActive]      = ImVec4(main.x, main.y, main.z, 1.00f);
    colors[ImGuiCol_Button]                = ImVec4(main.x, main.y, main.z, 0.44f);
    colors[ImGuiCol_ButtonHovered]         = ImVec4(main.x, main.y, main.z, 0.86f);
    colors[ImGuiCol_ButtonActive]          = ImVec4(main.x, main.y, main.z, 1.00f);
    colors[ImGuiCol_Header]                = ImVec4(main.x, main.y, main.z, 0.46f);
    colors[ImGuiCol_HeaderHovered]         = ImVec4(main.x, main.y, main.z, 1.00f);
    colors[ImGuiCol_HeaderActive]          = ImVec4(main.x, main.y, main.z, 0.86f);
    colors[ImGuiCol_Separator]             = ImVec4(main.x, main.y, main.z, 0.44f);
    colors[ImGuiCol_SeparatorHovered]      = ImVec4(main.x, main.y, main.z, 0.86f);
    colors[ImGuiCol_SeparatorActive]       = ImVec4(main.x, main.y, main.z, 1.00f);
    colors[ImGuiCol_ResizeGrip]            = ImVec4(main.x, main.y, main.z, 0.20f);
    colors[ImGuiCol_ResizeGripHovered]     = ImVec4(main.x, main.y, main.z, 0.78f);
    colors[ImGuiCol_ResizeGripActive]      = ImVec4(main.x, main.y, main.z, 1.00f);
    colors[ImGuiCol_Tab]                   = colors[ImGuiCol_Header];
    colors[ImGuiCol_TabHovered]            = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_TabActive]             = colors[ImGuiCol_HeaderActive];
    colors[ImGuiCol_TabUnfocused]          = colors[ImGuiCol_Tab];
    colors[ImGuiCol_TabUnfocusedActive]    = colors[ImGuiCol_TabActive];
    colors[ImGuiCol_DockingPreview]        = colors[ImGuiCol_Header];
    colors[ImGuiCol_DockingEmptyBg]        = ImVec4(area.x * 0.4f, area.y * 0.4f, area.z * 0.4f, 1.00f);
    colors[ImGuiCol_PlotLines]             = ImVec4(text.x, text.y, text.z, 0.63f);
    colors[ImGuiCol_PlotLinesHovered]      = ImVec4(main.x, main.y, main.z, 1.00f);
    colors[ImGuiCol_PlotHistogram]         = ImVec4(text.x, text.y, text.z, 0.63f);
    colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(main.x, main.y, main.z, 1.00f);
    colors[ImGuiCol_TextSelectedBg]        = ImVec4(main.x, main.y, main.z, 0.43f);
    colors[ImGuiCol_DragDropTarget]        = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_NavHighlight]          = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_NavWindowingHighlight] = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_NavWindowingDimBg]     = colors[ImGuiCol_Header];
    colors[ImGuiCol_ModalWindowDimBg]      = colors[ImGuiCol_Header];

    bool invert = false; // TODO Paul: Expose setting.
    if (invert)
    {
        for (int32 i = 0; i < ImGuiCol_COUNT; i++)
        {
            ImVec4& col = colors[i];
            float h, s, v;
            ImGui::ColorConvertRGBtoHSV(col.x, col.y, col.z, h, s, v);
            if (s < 0.1f)
            {
                v = 1.0f - v;
            }
            ImGui::ColorConvertHSVtoRGB(h, s, v, col.x, col.y, col.z);
        }
    }

    ImGuiStyle& style    = ImGui::GetStyle();
    style.FrameRounding  = setup.rounding;
    style.WindowRounding = setup.rounding;

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        style.WindowRounding = 0.0f;

    return true;
}

void ui_system_impl::configure(const ui_configuration& configuration)
{
    PROFILE_ZONE;
    m_configuration = configuration;
    if (m_configuration.is_dock_space_enabled())
    {
        // When docking is enabled we setup a default layout.
    }
    else
    {
        // TODO Paul: Make an option available to disable render view and make the renderer draw directly into the hardware backbuffer.
        // When docking is disabled we also have to specify a position for the render view.
    }

    // TODO Paul: There has to be a cleaner solution for this. Right now the window and input configuration has to be done before any ui related stuff.
    auto ws = m_shared_context->get_window_system_internal().lock();
    MANGO_ASSERT(ws, "Window system is expired!");
    auto platform_data = ws->get_platform_data();

    ImGui_ImplGlfw_InitForOpenGL(static_cast<GLFWwindow*>(platform_data->native_window_handle), true);
    ImGui_ImplOpenGL3_Init();
}

void ui_system_impl::update(float dt)
{
    PROFILE_ZONE;
    MANGO_UNUSED(dt);
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // dock space
    bool dockspace_enabled            = m_configuration.is_dock_space_enabled();
    static ImGuiDockNodeFlags d_flags = ImGuiDockNodeFlags_PassthruCentralNode;
    static ImGuiWindowFlags w_flags   = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    ImGuiViewport* viewport           = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    w_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    w_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    w_flags |= ImGuiWindowFlags_NoBackground;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace", &dockspace_enabled, w_flags);
    ImGui::PopStyleVar(3);

    // real dock space
    if (dockspace_enabled)
    {
        ImGuiID dockspace_id = ImGui::GetID("MangoDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), d_flags);
    }

    static bool cinema_view                        = false;
    auto custom                                    = m_configuration.get_custom_ui_data();
    static bool custom_enabled                     = true;
    const bool* widgets                            = m_configuration.get_ui_widgets();
    static bool render_view_enabled                = true;
    static bool hardware_info_enabled              = true;
    static bool scene_inspector_enabled            = true;
    static bool entity_component_inspector_enabled = true;
    static bool render_system_widget_enabled       = true;

    // menu bar
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Exit"))
                m_shared_context->get_application()->close();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Widgets"))
        {
            if (widgets[ui_widget::render_view] && ImGui::MenuItem("Render View"))
                render_view_enabled = true;
            if (widgets[ui_widget::hardware_info] && ImGui::MenuItem("Hardware Info"))
                hardware_info_enabled = true;
            if (widgets[ui_widget::scene_inspector] && ImGui::MenuItem("Scene Inspector"))
                scene_inspector_enabled = true;
            if (widgets[ui_widget::entity_component_inspector] && ImGui::MenuItem("Entity Component Inspector"))
                entity_component_inspector_enabled = true;
            if (widgets[ui_widget::render_system_ui] && ImGui::MenuItem("Render System Settings"))
                render_system_widget_enabled = true;
            if (custom.function && !custom.always_open && ImGui::MenuItem(custom.window_name.c_str()))
                custom_enabled = true;
            ImGui::EndMenu();
        }
        if (ImGui::MenuItem("Toggle Cinema View"))
        {
            cinema_view = !cinema_view;
        }

        ImGui::EndMenuBar();
    }

    // Render View
    ImVec2 viewport_size = ImVec2(1080, 720);
    if (widgets[ui_widget::render_view] && render_view_enabled)
        viewport_size = render_view_widget(m_shared_context, render_view_enabled);

    // Hardware Info
    if (widgets[ui_widget::hardware_info] && hardware_info_enabled && !cinema_view)
        hardware_info_widget(m_shared_context, hardware_info_enabled);

    // Custom
    custom_enabled |= custom.always_open;
    if (custom.function && custom_enabled && !cinema_view)
        custom.function(custom_enabled);

    // Inspectors

    // Scene Inspector
    static entity selected = invalid_entity;
    auto application_scene = m_shared_context->get_current_scene();
    if (widgets[ui_widget::scene_inspector] && scene_inspector_enabled && !cinema_view)
    {
        scene_inspector_widget(application_scene, scene_inspector_enabled, selected);
    }

    auto rs = m_shared_context->get_resource_system_internal().lock();
    MANGO_ASSERT(rs, "Resource system is expired!");

    // ECS Inspector
    if (widgets[ui_widget::entity_component_inspector] && entity_component_inspector_enabled && !cinema_view)
        entity_component_inspector_widget(application_scene, entity_component_inspector_enabled, selected, rs, viewport_size);

    auto render_s = m_shared_context->get_render_system_internal().lock();
    MANGO_ASSERT(render_s, "Render system is expired!");
    // Render system widget
    if (widgets[ui_widget::render_system_ui] && render_system_widget_enabled && !cinema_view)
        render_system_widget(render_s, render_system_widget_enabled);

    ImGui::End(); // dock space end
}

void ui_system_impl::destroy()
{
    PROFILE_ZONE;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ui_system_impl::draw_ui()
{
    PROFILE_ZONE;
    GL_NAMED_PROFILE_ZONE("UI System Draw");
    auto ws = m_shared_context->get_window_system_internal().lock();
    MANGO_ASSERT(ws, "Window system is expired!");

    ImGuiIO& io    = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)ws->get_width(), (float)ws->get_height());

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }
}
