//! \file      imgui_widgets.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_IMGUI_WIDGETS_HPP
#define MANGO_IMGUI_WIDGETS_HPP

#include "tinyfiledialogs.h"
#include <core/context_impl.hpp>
#include <graphics/framebuffer.hpp>
#include <graphics/texture.hpp>
#include <imgui.h>
#include <mango/scene.hpp>
#include <rendering/render_system_impl.hpp>
#include <resources/resource_system.hpp>
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

        ImGui::GetWindowDrawList()->AddImage(
            reinterpret_cast<void*>(shared_context->get_render_system_internal().lock()->get_backbuffer()->get_attachment(framebuffer_attachment::COLOR_ATTACHMENT0)->get_name()), position,
            ImVec2(position.x + size.x, position.y + size.y), ImVec2(0, 1), ImVec2(1, 0));
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
            ImGui::Text("Rendered Meshes: %d", stats.last_frame.meshes);
            ImGui::Text("Rendered Primitives: %d", stats.last_frame.primitives);
            ImGui::Text("Rendered Materials: %d", stats.last_frame.materials);
            ImGui::Text("Canvas Size: (%d x %d) px", stats.last_frame.canvas_width, stats.last_frame.canvas_height);
        }
        ImGui::End();
    }

    namespace
    {
        void get_formats_and_types_for_image(bool srgb, int32 components, int32 bits, format& f, format& internal, format& type)
        {
            f        = format::RGBA;
            internal = srgb ? format::SRGB8_ALPHA8 : format::RGBA8;

            if (components == 1)
            {
                f = format::RED;
            }
            else if (components == 2)
            {
                f = format::RG;
            }
            else if (components == 3)
            {
                f        = format::RGB;
                internal = srgb ? format::SRGB8 : format::RGB8;
            }

            type = format::UNSIGNED_BYTE;
            if (bits == 16)
            {
                type = format::UNSIGNED_SHORT;
            }
            else if (bits == 32)
            {
                format::UNSIGNED_INT;
            }
        }

        void draw_entity_tree(const shared_ptr<scene>& application_scene, entity e, entity& selected)
        {
            auto tag      = application_scene->query_tag(e);
            auto name     = (tag && !tag->tag_name.empty()) ? tag->tag_name : ("Unnamed Entity " + std::to_string(e)).c_str();
            auto children = application_scene->get_children(e);

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 5));
            bool open = ImGui::TreeNodeEx((name + "##id_" + std::to_string(e)).c_str(),
                                          ImGuiTreeNodeFlags_FramePadding | ((selected == e) ? ImGuiTreeNodeFlags_Selected : 0) | (children.empty() ? ImGuiTreeNodeFlags_Leaf : 0), "%s", name.c_str());
            ImGui::PopStyleVar();

            if (ImGui::IsItemClicked())
            {
                selected = e;
            }

            if (open)
            {
                for (auto child : children)
                {
                    draw_entity_tree(application_scene, child, selected);
                }
                ImGui::TreePop();
            }
        }

        texture_ptr load_texture(texture_configuration& config, const shared_ptr<resource_system>& rs)
        {
            char const* filter[4] = { "*.png", "*.jpg", "*.jpeg", "*.bmp" };

            char* query_path = tinyfd_openFileDialog("", "res/", 4, filter, NULL, 0);
            if (query_path)
            {
                string queried = string(query_path);

                mango::image_configuration img_config;
                img_config.is_hdr                  = false;
                img_config.is_standard_color_space = config.m_is_standard_color_space;
                auto start                         = queried.find_last_of("\\/") + 1;
                img_config.name                    = queried.substr(start, queried.find_last_of(".") - start);
                auto img                           = rs->get_image(queried, img_config);

                config.m_generate_mipmaps = calculate_mip_count(img->width, img->height);
                texture_ptr text          = texture::create(config);

                format f;
                format internal;
                format type;

                get_formats_and_types_for_image(config.m_is_standard_color_space, img->number_components, img->bits, f, internal, type);

                text->set_data(internal, img->width, img->height, f, type, img->data);
                return text;
            }
            return nullptr;
        }

        void draw_material(const shared_ptr<material>& material, const shared_ptr<resource_system>& rs, entity e)
        {
            if (material)
            {
                texture_configuration config;
                config.m_generate_mipmaps        = 1;
                config.m_is_standard_color_space = true;
                config.m_texture_min_filter      = texture_parameter::FILTER_LINEAR_MIPMAP_LINEAR;
                config.m_texture_mag_filter      = texture_parameter::FILTER_LINEAR;
                config.m_texture_wrap_s          = texture_parameter::WRAP_REPEAT;
                config.m_texture_wrap_t          = texture_parameter::WRAP_REPEAT;

                ImVec2 canvas_p0;
                ImDrawList* draw_list = ImGui::GetWindowDrawList();

                // base color

                if (ImGui::TreeNode(("Base Color##" + std::to_string(e)).c_str()))
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
                    canvas_p0 = ImGui::GetCursorScreenPos();
                    draw_list->AddRectFilled(canvas_p0, ImVec2(canvas_p0.x + 64, canvas_p0.y + 64), IM_COL32(127, 127, 127, 255), 2.0f);
                    if (material->base_color_texture)
                        ImGui::Image(reinterpret_cast<void*>(material->base_color_texture->get_name()), ImVec2(64, 64));
                    else
                        ImGui::Dummy(ImVec2(64, 64));
                    if (ImGui::IsItemHovered())
                    {
                        if (material->base_color_texture)
                        {
                            ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.5, 0.5, 0.5, 1.0));
                            ImGui::BeginTooltip();
                            ImGui::Image(reinterpret_cast<void*>(material->base_color_texture->get_name()), ImVec2(256, 256));
                            ImGui::EndTooltip();
                            ImGui::PopStyleColor();
                        }
                        else
                            ImGui::SetTooltip("Load Texture");
                        draw_list->AddRect(canvas_p0, ImVec2(canvas_p0.x + 64, canvas_p0.y + 64), IM_COL32(200, 200, 200, 255), 2.0f);
                    }
                    ImGui::PopStyleVar();
                    if (ImGui::IsItemClicked())
                    {
                        config.m_is_standard_color_space = true;
                        material->base_color_texture     = load_texture(config, rs);
                        material->use_base_color_texture = (material->base_color_texture != nullptr);
                    }
                    ImGui::SameLine();
                    bool tmp = material->use_base_color_texture;
                    ImGui::Checkbox("Use Texture##base_color", &material->use_base_color_texture);
                    if (!tmp && material->use_base_color_texture && !material->base_color_texture)
                    {
                        config.m_is_standard_color_space = true;
                        material->base_color_texture     = load_texture(config, rs);
                        material->use_base_color_texture = (material->base_color_texture != nullptr);
                    }
                    if (!material->use_base_color_texture)
                    {
                        ImGui::SameLine();
                        ImGui::ColorEdit4("Color", material->base_color, ImGuiColorEditFlags_NoInputs);
                    }
                    ImGui::Separator();
                    ImGui::TreePop();
                }

                // roughness metallic

                if (ImGui::TreeNode(("Roughness and Metallic##" + std::to_string(e)).c_str()))
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
                    canvas_p0 = ImGui::GetCursorScreenPos();
                    draw_list->AddRectFilled(canvas_p0, ImVec2(canvas_p0.x + 64, canvas_p0.y + 64), IM_COL32(127, 127, 127, 255), 2.0f);
                    if (material->roughness_metallic_texture)
                        ImGui::Image(reinterpret_cast<void*>(material->roughness_metallic_texture->get_name()), ImVec2(64, 64));
                    else
                        ImGui::Dummy(ImVec2(64, 64));
                    if (ImGui::IsItemHovered())
                    {
                        if (material->roughness_metallic_texture)
                        {
                            ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.5, 0.5, 0.5, 1.0));
                            ImGui::BeginTooltip();
                            ImGui::Image(reinterpret_cast<void*>(material->roughness_metallic_texture->get_name()), ImVec2(256, 256));
                            ImGui::EndTooltip();
                            ImGui::PopStyleColor();
                        }
                        else
                            ImGui::SetTooltip("Load Texture");
                        draw_list->AddRect(canvas_p0, ImVec2(canvas_p0.x + 64, canvas_p0.y + 64), IM_COL32(200, 200, 200, 255), 2.0f);
                    }
                    ImGui::PopStyleVar();
                    if (ImGui::IsItemClicked())
                    {
                        config.m_is_standard_color_space         = false;
                        material->roughness_metallic_texture     = load_texture(config, rs);
                        material->use_roughness_metallic_texture = (material->roughness_metallic_texture != nullptr);
                    }
                    ImGui::SameLine();
                    bool tmp = material->use_roughness_metallic_texture;
                    ImGui::Checkbox("Use Texture##roughness_metallic", &material->use_roughness_metallic_texture);
                    if (!tmp && material->use_roughness_metallic_texture && !material->roughness_metallic_texture)
                    {
                        config.m_is_standard_color_space         = false;
                        material->roughness_metallic_texture     = load_texture(config, rs);
                        material->use_roughness_metallic_texture = (material->roughness_metallic_texture != nullptr);
                    }
                    if (material->roughness_metallic_texture && material->use_roughness_metallic_texture)
                    {
                        ImGui::Checkbox("Has packed ambient occlusion", &material->packed_occlusion);
                        if (material->packed_occlusion)
                            ImGui::Checkbox("Use packed ambient occlusion", &material->use_packed_occlusion);
                    }
                    if (!material->use_roughness_metallic_texture)
                    {
                        ImGui::SliderFloat("Roughness", material->roughness.type_data(), 0.0f, 1.0f);
                        ImGui::SliderFloat("Metallic", material->metallic.type_data(), 0.0f, 1.0f);
                    }
                    ImGui::Separator();
                    ImGui::TreePop();
                }

                // normal

                if (ImGui::TreeNode(("Normal Map##" + std::to_string(e)).c_str()))
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
                    canvas_p0 = ImGui::GetCursorScreenPos();
                    draw_list->AddRectFilled(canvas_p0, ImVec2(canvas_p0.x + 64, canvas_p0.y + 64), IM_COL32(127, 127, 127, 255), 2.0f);
                    if (material->normal_texture)
                        ImGui::Image(reinterpret_cast<void*>(material->normal_texture->get_name()), ImVec2(64, 64));
                    else
                        ImGui::Dummy(ImVec2(64, 64));
                    if (ImGui::IsItemHovered())
                    {
                        if (material->normal_texture)
                        {
                            ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.5, 0.5, 0.5, 1.0));
                            ImGui::BeginTooltip();
                            ImGui::Image(reinterpret_cast<void*>(material->normal_texture->get_name()), ImVec2(256, 256));
                            ImGui::EndTooltip();
                            ImGui::PopStyleColor();
                        }
                        else
                            ImGui::SetTooltip("Load Texture");
                        draw_list->AddRect(canvas_p0, ImVec2(canvas_p0.x + 64, canvas_p0.y + 64), IM_COL32(200, 200, 200, 255), 2.0f);
                    }
                    ImGui::PopStyleVar();
                    if (ImGui::IsItemClicked())
                    {
                        config.m_is_standard_color_space = false;
                        material->normal_texture         = load_texture(config, rs);
                        material->use_normal_texture     = (material->normal_texture != nullptr);
                    }
                    ImGui::SameLine();
                    bool tmp = material->use_normal_texture;
                    ImGui::Checkbox("Use Texture##normal", &material->use_normal_texture);
                    if (!tmp && material->use_normal_texture && !material->normal_texture)
                    {
                        config.m_is_standard_color_space = false;
                        material->normal_texture         = load_texture(config, rs);
                        material->use_normal_texture     = (material->normal_texture != nullptr);
                    }
                    ImGui::Separator();
                    ImGui::TreePop();
                }

                // occlusion

                if (ImGui::TreeNode(("Occlusion Map##" + std::to_string(e)).c_str()))
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
                    canvas_p0 = ImGui::GetCursorScreenPos();
                    draw_list->AddRectFilled(canvas_p0, ImVec2(canvas_p0.x + 64, canvas_p0.y + 64), IM_COL32(127, 127, 127, 255), 2.0f);
                    if (material->occlusion_texture)
                        ImGui::Image(reinterpret_cast<void*>(material->occlusion_texture->get_name()), ImVec2(64, 64));
                    else
                        ImGui::Dummy(ImVec2(64, 64));
                    if (ImGui::IsItemHovered())
                    {
                        if (material->occlusion_texture)
                        {
                            ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.5, 0.5, 0.5, 1.0));
                            ImGui::BeginTooltip();
                            ImGui::Image(reinterpret_cast<void*>(material->occlusion_texture->get_name()), ImVec2(256, 256));
                            ImGui::EndTooltip();
                            ImGui::PopStyleColor();
                        }
                        else
                            ImGui::SetTooltip("Load Texture");
                        draw_list->AddRect(canvas_p0, ImVec2(canvas_p0.x + 64, canvas_p0.y + 64), IM_COL32(200, 200, 200, 255), 2.0f);
                    }
                    ImGui::PopStyleVar();
                    if (ImGui::IsItemClicked())
                    {
                        config.m_is_standard_color_space = false;
                        material->occlusion_texture      = load_texture(config, rs);
                        material->use_occlusion_texture  = (material->occlusion_texture != nullptr);
                    }
                    ImGui::SameLine();
                    bool tmp = material->use_occlusion_texture;
                    ImGui::Checkbox("Use Texture##occlusion", &material->use_occlusion_texture);
                    if (!tmp && material->use_occlusion_texture && !material->occlusion_texture)
                    {
                        config.m_is_standard_color_space = false;
                        material->occlusion_texture      = load_texture(config, rs);
                        material->use_occlusion_texture  = (material->occlusion_texture != nullptr);
                    }
                    ImGui::Separator();
                    ImGui::TreePop();
                }

                // emissive

                if (ImGui::TreeNode(("Emissive##" + std::to_string(e)).c_str()))
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
                    canvas_p0 = ImGui::GetCursorScreenPos();
                    draw_list->AddRectFilled(canvas_p0, ImVec2(canvas_p0.x + 64, canvas_p0.y + 64), IM_COL32(127, 127, 127, 255), 2.0f);
                    if (material->emissive_color_texture)
                        ImGui::Image(reinterpret_cast<void*>(material->emissive_color_texture->get_name()), ImVec2(64, 64));
                    else
                        ImGui::Dummy(ImVec2(64, 64));
                    if (ImGui::IsItemHovered())
                    {
                        if (material->emissive_color_texture)
                        {
                            ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.5, 0.5, 0.5, 1.0));
                            ImGui::BeginTooltip();
                            ImGui::Image(reinterpret_cast<void*>(material->emissive_color_texture->get_name()), ImVec2(256, 256));
                            ImGui::EndTooltip();
                            ImGui::PopStyleColor();
                        }
                        else
                            ImGui::SetTooltip("Load Texture");
                        draw_list->AddRect(canvas_p0, ImVec2(canvas_p0.x + 64, canvas_p0.y + 64), IM_COL32(200, 200, 200, 255), 2.0f);
                    }
                    ImGui::PopStyleVar();
                    if (ImGui::IsItemClicked())
                    {
                        config.m_is_standard_color_space     = true;
                        material->emissive_color_texture     = load_texture(config, rs);
                        material->use_emissive_color_texture = (material->emissive_color_texture != nullptr);
                    }
                    ImGui::SameLine();
                    bool tmp = material->use_emissive_color_texture;
                    ImGui::Checkbox("Use Texture##emissive_color", &material->use_emissive_color_texture);
                    if (!tmp && material->use_emissive_color_texture && !material->emissive_color_texture)
                    {
                        config.m_is_standard_color_space     = true;
                        material->emissive_color_texture     = load_texture(config, rs);
                        material->use_emissive_color_texture = (material->emissive_color_texture != nullptr);
                    }
                    if (!material->use_emissive_color_texture)
                    {
                        ImGui::SameLine();
                        ImGui::ColorEdit3("Color", material->emissive_color, ImGuiColorEditFlags_NoInputs);
                    }
                    ImGui::Separator();
                    ImGui::TreePop();
                }
            }
        }

    } // namespace

    void scene_inspector_widget(const shared_ptr<scene>& application_scene, bool& enabled, entity& selected)
    {
        ImGui::Begin("Scene Inspector", &enabled);
        entity root             = application_scene->get_root();
        static entity selection = invalid_entity;
        draw_entity_tree(application_scene, root, selection);
        ImGui::End();
        selected = selection;
    }

    void material_inspector_widget(const mesh_component* mesh, bool& enabled, bool entity_changed, entity e, const shared_ptr<resource_system>& rs)
    {
        ImGui::Begin("Material Inspector", &enabled);
        if (mesh)
        {
            static material_component current_material;
            static int32 current_id;
            auto current_name = !current_material.material_name.empty() ? current_material.material_name : "Unnamed Material " + std::to_string(current_id);
            if (ImGui::BeginCombo("Materials", current_name.c_str()))
            {
                for (int32 n = 0; n < mesh->materials.size(); ++n)
                {
                    bool is_selected = (current_material.material_name == mesh->materials[n].material_name);
                    auto name        = !mesh->materials[n].material_name.empty() ? mesh->materials[n].material_name : "Unnamed Material " + std::to_string(n);
                    if (ImGui::Selectable(name.c_str(), is_selected))
                    {
                        current_material = mesh->materials[n];
                        current_id       = n;
                    }
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            if (entity_changed)
                current_material = mesh->materials.at(0);
            draw_material(current_material.component_material, rs, e);
        }
        ImGui::End();
    }

} // namespace mango

#endif // MANGO_IMGUI_WIDGETS_HPP
