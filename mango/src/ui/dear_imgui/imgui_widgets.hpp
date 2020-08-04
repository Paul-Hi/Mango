//! \file      imgui_widgets.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_IMGUI_WIDGETS_HPP
#define MANGO_IMGUI_WIDGETS_HPP

#include <core/context_impl.hpp>
#include <graphics/framebuffer.hpp>
#include <graphics/texture.hpp>
#include <imgui.h>
#include <mango/scene.hpp>
#include <mango/types.hpp>
#include <rendering/render_system_impl.hpp>
#include <ui/dear_imgui/imgui_glfw.hpp>

namespace mango
{

    //! \brief This is an imgui widget drawing the render view and the frame produced by the renderer.
    //! \param[in] shared_context The shared context.
    //! \param[in] enabled Specifies if window is rendered or not and can be set by imgui.
    void render_view_widget(const shared_ptr<context_impl>& shared_context, bool& enabled)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
        ImGui::Begin("Render View", &enabled);
        bool bar = ImGui::IsItemHovered() || ImGui::IsItemFocused();

        ImGui_ImplGlfw_FrameHovered(!bar && ImGui::IsWindowHovered());
        ImGui_ImplGlfw_FrameFocused(!bar && ImGui::IsWindowFocused());

        ImVec2 position         = ImGui::GetCursorScreenPos();
        ImVec2 size             = ImGui::GetWindowSize();
        static ImVec2 last_size = ImVec2(0.0f, 0.0f);

        ImGui::GetWindowDrawList()->AddImage((void*)shared_context->get_render_system_internal().lock()->get_backbuffer()->get_attachment(framebuffer_attachment::COLOR_ATTACHMENT0)->get_name(),
                                             position, ImVec2(position.x + size.x, position.y + size.y), ImVec2(0, 1), ImVec2(1, 0));
        ImGui::PopStyleVar();
        ImGui::End();

        if ((last_size.x != size.x || last_size.y != size.y) && size.x > 0 && size.y > 0)
        {
            auto cam_info = shared_context->get_current_scene()->get_active_camera_data().camera_info;
            if (cam_info)
                cam_info->aspect = (float)size.x / (float)size.y;
            shared_context->get_render_system_internal().lock()->set_viewport(0, 0, static_cast<int32>(size.x), static_cast<int32>(size.y));
            last_size = size;
        }
    }

    //! \brief This is an imgui widget drawing some stats of the framework.
    //! \param[in] shared_context The shared context.
    //! \param[in] enabled Specifies if window is rendered or not and can be set by imgui.
    //! \param[in] dt The time since the last call.
    void hardware_info_widget(const shared_ptr<context_impl>& shared_context, bool& enabled, float dt)
    {
        ImGui::Begin("Hardware Info", &enabled);
        if (ImGui::CollapsingHeader("Editor Stats"))
        {
            ImGui::Text("Approx. Frame Time: %.2f ms", dt * 1000.0f);
        }
        if (ImGui::CollapsingHeader("Renderer Stats"))
        {
            auto stats = shared_context->get_render_system_internal().lock()->get_hardware_stats();
            ImGui::Text("API Version: %s", stats.api_version.c_str());
            ImGui::Text("Draw Calls: %d", stats.last_frame.draw_calls);
            ImGui::Text("Canvas Size: (%d x %d) px", stats.last_frame.canvas_width, stats.last_frame.canvas_height);
        }
        ImGui::End();
    }

} // namespace mango

#endif // MANGO_IMGUI_WIDGETS_HPP
