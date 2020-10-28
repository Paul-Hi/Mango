//! \file      imgui_widgets.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_IMGUI_WIDGETS_HPP
#define MANGO_IMGUI_WIDGETS_HPP

#include "tinyfiledialogs.h"
#include <core/context_impl.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <graphics/framebuffer.hpp>
#include <graphics/texture.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#include <mango/scene.hpp>
#include <rendering/render_system_impl.hpp>
#include <resources/resource_system.hpp>
#include <ui/dear_imgui/imgui_glfw.hpp>

namespace mango
{
    //! \brief This is an imgui widget drawing the render view and the frame produced by the renderer.
    //! \param[in] shared_context The shared context.
    //! \param[in] enabled Specifies if window is rendered or not and can be set by imgui.
    //! \return The size of the viewport.
    ImVec2 render_view_widget(const shared_ptr<context_impl>& shared_context, bool& enabled)
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
            (void*)(intptr_t)shared_context->get_render_system_internal().lock()->get_backbuffer()->get_attachment(framebuffer_attachment::color_attachment0)->get_name(), position,
            ImVec2(position.x + size.x, position.y + size.y), ImVec2(0, 1), ImVec2(1, 0));
        ImGui::PopStyleVar();
        ImGui::End();

        if ((last_size.x != size.x || last_size.y != size.y) && size.x > 0 && size.y > 0)
        {
            auto cam_info = shared_context->get_current_scene()->get_active_camera_data().camera_info;
            if (cam_info)
            {
                cam_info->perspective.aspect = size.x / size.y;
                cam_info->orthographic.x_mag = size.x / size.y;
                cam_info->orthographic.y_mag = 1.0f;
            }
            shared_context->get_render_system_internal().lock()->set_viewport(0, 0, static_cast<int32>(size.x), static_cast<int32>(size.y));
            last_size = size;
        }
        return size;
    }

    //! \brief This is an imgui widget drawing some stats of the framework.
    //! \param[in] shared_context The shared context.
    //! \param[in] enabled Specifies if window is rendered or not and can be set by imgui.
    void hardware_info_widget(const shared_ptr<context_impl>& shared_context, bool& enabled)
    {
        ImGui::Begin("Hardware Info", &enabled);
        if (ImGui::CollapsingHeader("Editor Stats"))
        {
            float frametime = shared_context->get_application()->frame_time();
            ImGui::Text("Frame Time: %.2f ms", frametime * 1000.0f);
            ImGui::Text("Framerate: %.2f fps", 1.0f / frametime);
        }
        auto stats = shared_context->get_render_system_internal().lock()->get_hardware_stats();
        if (ImGui::CollapsingHeader("Renderer Stats"))
        {
            ImGui::Text("API Version: %s", stats.api_version.c_str());
            ImGui::Text("Rendered Meshes: %d", stats.last_frame.meshes);
            ImGui::Text("Draw Calls: %d", stats.last_frame.draw_calls);
            ImGui::Text("Rendered Primitives: %d", stats.last_frame.primitives);
            ImGui::Text("Rendered Materials: %d", stats.last_frame.materials);
            ImGui::Text("Canvas Size: (%d x %d) px", stats.last_frame.canvas_width, stats.last_frame.canvas_height);
        }
        ImGui::End();
    }

    namespace details
    {
        //! \brief Returns internal format, format and type for an image depending on a few infos.
        //! \param[in] srgb True if image is in standard color space, else False.
        //! \param[in] components The number of components in the image.
        //! \param[in] bits The number of bits in the image.
        //! \param[out] f The format to choose.
        //! \param[out] internal The internal format to choose.
        //! \param[out] type The type to choose.
        //! \param[in] is_hdr True if image is hdr, else False.
        void get_formats_and_types_for_image(bool srgb, int32 components, int32 bits, format& f, format& internal, format& type, bool is_hdr)
        {
            if (is_hdr)
            {
                f        = format::rgb;
                internal = format::rgb32f;
                type     = format::t_float;

                if (components == 4)
                {
                    f        = format::rgba;
                    internal = format::rgba32f;
                }
                return;
            }

            f        = format::rgba;
            internal = srgb ? format::srgb8_alpha8 : format::rgba8;

            if (components == 1)
            {
                f = format::red;
            }
            else if (components == 2)
            {
                f = format::rg;
            }
            else if (components == 3)
            {
                f        = format::rgb;
                internal = srgb ? format::srgb8 : format::rgb8;
            }

            type = format::t_unsigned_byte;
            if (bits == 16)
            {
                type = format::t_unsigned_short;
            }
            else if (bits == 32)
            {
                type = format::t_unsigned_int;
            }
        }

        //! \brief Draws the subtree for a given entity in the user interface.
        //! \param[in] application_scene The current \a scene of the \a application.
        //! \param[in] e The entity to draw the subtree for.
        //! \param[in,out] selected The currently selected entity, can be updated by this function.
        void draw_entity_tree(const shared_ptr<scene>& application_scene, entity e, entity& selected)
        {
            auto tag      = application_scene->query_tag(e);
            auto name     = (tag && !tag->tag_name.empty()) ? tag->tag_name : ("Unnamed Entity " + std::to_string(e)).c_str();
            auto children = application_scene->get_children(e);

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 5));
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_FramePadding | ((selected == e) ? ImGuiTreeNodeFlags_Selected : 0) |
                                       (children.empty() ? ImGuiTreeNodeFlags_Leaf : 0);
            bool open = ImGui::TreeNodeEx((name + "##id_" + std::to_string(e)).c_str(), flags, "%s", name.c_str());
            ImGui::PopStyleVar();

            if (ImGui::IsItemClicked(0))
            {
                selected = e;
            }
            if (ImGui::IsItemClicked(1) && !ImGui::IsPopupOpen(("##entity_menu" + std::to_string(e)).c_str()))
            {
                selected = e;
                ImGui::OpenPopup(("##entity_menu" + std::to_string(e)).c_str());
            }
            if (ImGui::BeginPopup(("##entity_menu" + std::to_string(e)).c_str()))
            {
                if (ImGui::Selectable(("Add Entity##entity_menu" + std::to_string(e)).c_str()))
                {
                    selected = application_scene->create_empty();
                    application_scene->attach(selected, e);
                }
                if (application_scene->get_root() != e && ImGui::Selectable(("Remove Entity##entity_menu" + std::to_string(e)).c_str()))
                {
                    application_scene->remove_entity(e);
                    if (selected == e)
                        selected = invalid_entity;
                }

                ImGui::EndPopup();
            }
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
            {
                ImGui::SetDragDropPayload("entity node", &e, sizeof(entity));
                ImGui::EndDragDropSource();
            }
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("entity node"))
                {
                    IM_ASSERT(payload->DataSize == sizeof(entity));
                    entity dropped = *(const entity*)payload->Data;
                    application_scene->detach(dropped);
                    application_scene->attach(dropped, e);
                }
                ImGui::EndDragDropTarget();
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

        //! \brief Opens a file dialog to load any kind of image and returns a texture_ptr after loading it to a \a texture.
        //! \param[in] config The \a texture_configuration used for creating the \a texture.
        //! \param[in] rs A pointer to the \a resource_system.
        //! \param[in] filter A list of file extensions to filter them.
        //! \param[in] num_filters The number of elements in filter.
        //! \return The texture_ptr to the created \a texture
        texture_ptr load_texture(texture_configuration& config, const shared_ptr<resource_system>& rs, const char* const* filter, int32 num_filters)
        {
            char* query_path = tinyfd_openFileDialog("", "res/", num_filters, filter, NULL, 0);
            if (query_path)
            {
                string queried = string(query_path);

                mango::image_configuration img_config;
                img_config.is_hdr                  = num_filters == 1; // TODO Paul: This is correct but fishy.
                img_config.is_standard_color_space = config.m_is_standard_color_space;
                auto start                         = queried.find_last_of("\\/") + 1;
                img_config.name                    = queried.substr(start, queried.find_last_of(".") - start);
                auto img                           = rs->get_image(queried, img_config);

                config.m_generate_mipmaps = calculate_mip_count(img->width, img->height);
                texture_ptr text          = texture::create(config);

                format f;
                format internal;
                format type;

                get_formats_and_types_for_image(config.m_is_standard_color_space, img->number_components, img->bits, f, internal, type, img_config.is_hdr);

                text->set_data(internal, img->width, img->height, f, type, img->data);
                return text;
            }
            return nullptr;
        }

        //! \brief Draws a \a material in the user interface.
        //! \param[in] material A shared_ptr to the \a material that should be represented in th UI.
        //! \param[in] rs A pointer to the \a resource_system.
        //! \param[in] e The entity the \a material belongs to.
        void draw_material(const shared_ptr<material>& material, const shared_ptr<resource_system>& rs, entity e)
        {
            if (material)
            {
                texture_configuration config;
                config.m_generate_mipmaps        = 1;
                config.m_is_standard_color_space = true;
                config.m_texture_min_filter      = texture_parameter::filter_linear_mipmap_linear;
                config.m_texture_mag_filter      = texture_parameter::filter_linear;
                config.m_texture_wrap_s          = texture_parameter::wrap_repeat;
                config.m_texture_wrap_t          = texture_parameter::wrap_repeat;
                char const* filter[4]            = { "*.png", "*.jpg", "*.jpeg", "*.bmp" };

                ImVec2 canvas_p0;
                ImDrawList* draw_list = ImGui::GetWindowDrawList();

                // base color

                if (ImGui::TreeNode(("Base Color##" + std::to_string(e)).c_str()))
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
                    canvas_p0 = ImGui::GetCursorScreenPos();
                    draw_list->AddRectFilled(canvas_p0, ImVec2(canvas_p0.x + 64, canvas_p0.y + 64), IM_COL32(127, 127, 127, 255), 2.0f);
                    if (material->base_color_texture)
                        ImGui::Image((void*)(intptr_t)material->base_color_texture->get_name(), ImVec2(64, 64));
                    else
                        ImGui::Dummy(ImVec2(64, 64));
                    if (ImGui::IsItemHovered())
                    {
                        if (material->base_color_texture)
                        {
                            ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.5, 0.5, 0.5, 1.0));
                            ImGui::BeginTooltip();
                            ImGui::Image((void*)(intptr_t)material->base_color_texture->get_name(), ImVec2(256, 256));
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
                        material->base_color_texture     = load_texture(config, rs, filter, 4);
                        material->use_base_color_texture = (material->base_color_texture != nullptr);
                    }
                    ImGui::SameLine();
                    bool tmp = material->use_base_color_texture;
                    ImGui::Checkbox("Use Texture##base_color", &material->use_base_color_texture);
                    if (!tmp && material->use_base_color_texture && !material->base_color_texture)
                    {
                        config.m_is_standard_color_space = true;
                        material->base_color_texture     = load_texture(config, rs, filter, 4);
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
                        ImGui::Image((void*)(intptr_t)material->roughness_metallic_texture->get_name(), ImVec2(64, 64));
                    else
                        ImGui::Dummy(ImVec2(64, 64));
                    if (ImGui::IsItemHovered())
                    {
                        if (material->roughness_metallic_texture)
                        {
                            ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.5, 0.5, 0.5, 1.0));
                            ImGui::BeginTooltip();
                            ImGui::Image((void*)(intptr_t)material->roughness_metallic_texture->get_name(), ImVec2(256, 256));
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
                        material->roughness_metallic_texture     = load_texture(config, rs, filter, 4);
                        material->use_roughness_metallic_texture = (material->roughness_metallic_texture != nullptr);
                    }
                    ImGui::SameLine();
                    bool tmp = material->use_roughness_metallic_texture;
                    ImGui::Checkbox("Use Texture##roughness_metallic", &material->use_roughness_metallic_texture);
                    if (!tmp && material->use_roughness_metallic_texture && !material->roughness_metallic_texture)
                    {
                        config.m_is_standard_color_space         = false;
                        material->roughness_metallic_texture     = load_texture(config, rs, filter, 4);
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
                        ImGui::Image((void*)(intptr_t)material->normal_texture->get_name(), ImVec2(64, 64));
                    else
                        ImGui::Dummy(ImVec2(64, 64));
                    if (ImGui::IsItemHovered())
                    {
                        if (material->normal_texture)
                        {
                            ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.5, 0.5, 0.5, 1.0));
                            ImGui::BeginTooltip();
                            ImGui::Image((void*)(intptr_t)material->normal_texture->get_name(), ImVec2(256, 256));
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
                        material->normal_texture         = load_texture(config, rs, filter, 4);
                        material->use_normal_texture     = (material->normal_texture != nullptr);
                    }
                    ImGui::SameLine();
                    bool tmp = material->use_normal_texture;
                    ImGui::Checkbox("Use Texture##normal", &material->use_normal_texture);
                    if (!tmp && material->use_normal_texture && !material->normal_texture)
                    {
                        config.m_is_standard_color_space = false;
                        material->normal_texture         = load_texture(config, rs, filter, 4);
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
                        ImGui::Image((void*)(intptr_t)material->occlusion_texture->get_name(), ImVec2(64, 64));
                    else
                        ImGui::Dummy(ImVec2(64, 64));
                    if (ImGui::IsItemHovered())
                    {
                        if (material->occlusion_texture)
                        {
                            ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.5, 0.5, 0.5, 1.0));
                            ImGui::BeginTooltip();
                            ImGui::Image((void*)(intptr_t)material->occlusion_texture->get_name(), ImVec2(256, 256));
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
                        material->occlusion_texture      = load_texture(config, rs, filter, 4);
                        material->use_occlusion_texture  = (material->occlusion_texture != nullptr);
                    }
                    ImGui::SameLine();
                    bool tmp = material->use_occlusion_texture;
                    ImGui::Checkbox("Use Texture##occlusion", &material->use_occlusion_texture);
                    if (!tmp && material->use_occlusion_texture && !material->occlusion_texture)
                    {
                        config.m_is_standard_color_space = false;
                        material->occlusion_texture      = load_texture(config, rs, filter, 4);
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
                        ImGui::Image((void*)(intptr_t)material->emissive_color_texture->get_name(), ImVec2(64, 64));
                    else
                        ImGui::Dummy(ImVec2(64, 64));
                    if (ImGui::IsItemHovered())
                    {
                        if (material->emissive_color_texture)
                        {
                            ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.5, 0.5, 0.5, 1.0));
                            ImGui::BeginTooltip();
                            ImGui::Image((void*)(intptr_t)material->emissive_color_texture->get_name(), ImVec2(256, 256));
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
                        material->emissive_color_texture     = load_texture(config, rs, filter, 4);
                        material->use_emissive_color_texture = (material->emissive_color_texture != nullptr);
                    }
                    ImGui::SameLine();
                    bool tmp = material->use_emissive_color_texture;
                    ImGui::Checkbox("Use Texture##emissive_color", &material->use_emissive_color_texture);
                    if (!tmp && material->use_emissive_color_texture && !material->emissive_color_texture)
                    {
                        config.m_is_standard_color_space     = true;
                        material->emissive_color_texture     = load_texture(config, rs, filter, 4);
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

                ImGui::Checkbox(("Double Sided##" + std::to_string(e)).c_str(), &material->double_sided);
                ImGui::Separator();
                const char* types    = "opaque\0masked\0blended\0dithered\0\0";
                int32 alpha_mode_int = static_cast<int32>(material->alpha_rendering);
                ImGui::Combo(("Alpha Mode##" + std::to_string(e)).c_str(), &alpha_mode_int, types);
                material->alpha_rendering = static_cast<alpha_mode>(alpha_mode_int);
                if (material->alpha_rendering == alpha_mode::mode_mask)
                    ImGui::SliderFloat("Alpha CutOff", material->alpha_cutoff.type_data(), 0.0f, 1.0f);
                if (material->alpha_rendering == alpha_mode::mode_blend)
                    ImGui::Text("Blending is supported (Basic Over Operator)!");
                if (material->alpha_rendering == alpha_mode::mode_dither)
                    ImGui::Text("Dithering ... Just for fun!");
            }
        }

    } // namespace details

    //! \brief Draws a scene graph in the user interface.
    //! \param[in] application_scene The current \a scene of the \a application.
    //! \param[in,out] enabled True if the window is open, else False.
    //! \param[in,out] selected The currently selected entity, can be updated by this function.
    void scene_inspector_widget(const shared_ptr<scene>& application_scene, bool& enabled, entity& selected)
    {
        ImGui::Begin("Scene Inspector", &enabled);
        entity root = application_scene->get_root();

        if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() && !ImGui::IsPopupOpen("##scene_menu") && ImGui::IsMouseClicked(1))
            ImGui::OpenPopup("##scene_menu");
        if (ImGui::BeginPopup("##scene_menu"))
        {
            if (ImGui::Selectable("Add Entity##scene_menu"))
            {
                selected = application_scene->create_empty();
                application_scene->attach(selected, root);
            }

            ImGui::EndPopup();
        }

        static entity selection = invalid_entity;
        details::draw_entity_tree(application_scene, root, selection);
        ImGui::End();
        selected = selection;
    }

    //! \brief Draws the material inspector for a given entity and it's \a mesh_component in the user interface.
    //! \details An entity and it's \a mesh_component could have multiple materials.
    //! \param[in] mesh The \a mesh_component with all materials.
    //! \param[in,out] enabled True if the window is open, else False.
    //! \param[in] entity_changed True if the entity changed since last call, else False.
    //! \param[in] e The entity the \a materials belongs to.
    //! \param[in] rs A pointer to the \a resource_system.
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
                for (int32 n = 0; n < static_cast<int32>(mesh->materials.size()); ++n)
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
            details::draw_material(current_material.component_material, rs, e);
        }
        ImGui::End();
    }

    //! \brief Draws the entity component inspector for a given entity in the user interface.
    //! \param[in] application_scene The current \a scene of the \a application.
    //! \param[in,out] enabled True if the window is open, else False.
    //! \param[in] e The entity the components should be inspected.
    //! \param[in] rs A pointer to the \a resource_system.
    //! \param[in] viewport_size The size of the render_view, when enabled, else some base size.
    void entity_component_inspector_widget(const shared_ptr<scene>& application_scene, bool& enabled, entity e, const shared_ptr<resource_system>& rs, const ImVec2& viewport_size)
    {
        ImGui::Begin("Entity Component Inspector", &enabled);
        if (e != invalid_entity)
        {
            auto tag_comp         = application_scene->query_tag(e);
            auto transform_comp   = application_scene->query_transform_component(e);
            auto model_comp       = application_scene->query_model_component(e);
            auto mesh_comp        = application_scene->query_mesh_component(e);
            auto camera_comp      = application_scene->query_camera_component(e);
            auto environment_comp = application_scene->query_environment_component(e);
            auto light_comp       = application_scene->query_light_component(e);

            if (ImGui::Button("Add Component"))
            {
                ImGui::OpenPopup("##component_addition_popup");
            }
            if (ImGui::BeginPopup("##component_addition_popup"))
            {
                if (!tag_comp && ImGui::Selectable("Tag Component"))
                {
                    application_scene->add_tag(e);
                }
                if (!transform_comp && ImGui::Selectable("Transform Component"))
                {
                    application_scene->add_transform_component(e);
                }
                // if (!mesh_comp && ImGui::Selectable("Mesh Component"))
                //{
                //    application_scene->add_mesh_component(e);
                //}
                if (!model_comp && ImGui::Selectable("Model Component"))
                {
                    application_scene->add_model_component(e);
                }
                if (!camera_comp && ImGui::Selectable("Camera Component"))
                {
                    application_scene->add_camera_component(e);
                }
                if (!environment_comp && ImGui::Selectable("Environment Component"))
                {
                    application_scene->add_environment_component(e);
                }
                if (!light_comp && ImGui::Selectable("Light Component"))
                {
                    application_scene->add_light_component(e);
                }

                ImGui::EndPopup();
            }
            ImGui::Separator();
            ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 0.0f);
            // Tag
            if (tag_comp)
            {
                if (ImGui::TreeNode(("Tag Component##" + std::to_string(e)).c_str()))
                {
                    ImGui::Spacing();
                    std::array<char, 32> tmp_string;                       // TODO Paul: Max length? 32 enough for now?
                    strcpy(tmp_string.data(), tag_comp->tag_name.c_str()); // TODO Paul: Kind of fishy.
                    ImGui::InputTextWithHint(("##tag" + std::to_string(e)).c_str(), "Enter Entity Tag", tmp_string.data(), 32);
                    tag_comp->tag_name = tmp_string.data();
                    ImGui::Spacing();
                    if (ImGui::Button(("Remove##tag" + std::to_string(e)).c_str()))
                        application_scene->remove_tag(e);
                    ImGui::Separator();
                    ImGui::TreePop();
                }
            }

            // Transform
            if (transform_comp)
            {
                if (ImGui::TreeNode(("Transform Component##" + std::to_string(e)).c_str()))
                {
                    ImGui::Spacing();
                    ImGui::BeginTabBar(("##local_transform" + std::to_string(e)).c_str());
                    if (ImGui::BeginTabItem(("Translation##translation" + std::to_string(e)).c_str()))
                    {
                        if (environment_comp)
                        {
                            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                            ImGui::BeginGroup();
                        }
                        ImGui::DragFloat(("X##t_delta_x" + std::to_string(e)).c_str(), &transform_comp->position.x, 0.1f, 0.0f, 0.0f, "%.1f");
                        ImGui::DragFloat(("Y##t_delta_y" + std::to_string(e)).c_str(), &transform_comp->position.y, 0.1f, 0.0f, 0.0f, "%.1f");
                        ImGui::DragFloat(("Z##t_delta_z" + std::to_string(e)).c_str(), &transform_comp->position.z, 0.1f, 0.0f, 0.0f, "%.1f");

                        ImGui::Spacing();
                        if (ImGui::Button(("Clear##translation" + std::to_string(e)).c_str()))
                            transform_comp->position = glm::vec3(0.0f);
                        if (environment_comp)
                        {
                            ImGui::EndGroup();
                            ImGui::PopItemFlag();
                            ImGui::PopStyleVar();
                            if (ImGui::IsItemHovered())
                                ImGui::SetTooltip("Disabled for environments");
                        }
                        ImGui::EndTabItem();
                    }
                    if (ImGui::BeginTabItem(("Rotation##rotation" + std::to_string(e)).c_str()))
                    {
                        if ((camera_comp || environment_comp))
                        {
                            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                            ImGui::BeginGroup();
                        }
                        static bool dragging[3] = { false, false, false };
                        float x                 = dragging[0] && ImGui::IsWindowFocused() ? ImGui::GetMouseDragDelta().x * 0.1f : 0.0f;
                        float y                 = dragging[1] && ImGui::IsWindowFocused() ? ImGui::GetMouseDragDelta().x * 0.1f : 0.0f;
                        float z                 = dragging[2] && ImGui::IsWindowFocused() ? ImGui::GetMouseDragDelta().x * 0.1f : 0.0f;
                        float t_x               = x;
                        float t_y               = y;
                        float t_z               = z;

                        ImGui::DragFloat(("X##r_delta_x" + std::to_string(e)).c_str(), &x, 0.1f, 0.0f, 0.0f, "%.1f°");
                        if (!ImGui::IsMouseDragging(0) && !ImGui::IsItemDeactivatedAfterChange())
                            t_x = x;
                        if (ImGui::IsItemActivated())
                        {
                            dragging[0] = true;
                            dragging[1] = false;
                            dragging[2] = false;
                        }
                        ImGui::DragFloat(("Y##r_delta_y" + std::to_string(e)).c_str(), &y, 0.1f, 0.0f, 0.0f, "%.1f°");
                        if (!ImGui::IsMouseDragging(0) && !ImGui::IsItemDeactivatedAfterChange())
                            t_y = y;
                        if (ImGui::IsItemActivated())
                        {
                            dragging[0] = false;
                            dragging[1] = true;
                            dragging[2] = false;
                        }
                        ImGui::DragFloat(("Z##r_delta_z" + std::to_string(e)).c_str(), &z, 0.1f, 0.0f, 0.0f, "%.1f°");
                        if (!ImGui::IsMouseDragging(0) && !ImGui::IsItemDeactivatedAfterChange())
                            t_z = z;
                        if (ImGui::IsItemActivated())
                        {
                            dragging[0] = false;
                            dragging[1] = false;
                            dragging[2] = true;
                        }

                        glm::quat x_quat = glm::angleAxis(glm::radians(x - t_x), glm::vec3(1.0f, 0.0f, 0.0f));
                        glm::quat y_quat = glm::angleAxis(glm::radians(y - t_y), glm::vec3(0.0f, 1.0f, 0.0f));
                        glm::quat z_quat = glm::angleAxis(glm::radians(z - t_z), glm::vec3(0.0f, 0.0f, 1.0f));

                        transform_comp->rotation = x_quat * y_quat * z_quat * transform_comp->rotation;

                        ImGui::Spacing();
                        if (ImGui::Button(("Clear##rotation" + std::to_string(e)).c_str()))
                            transform_comp->rotation = glm::quat(glm::vec3(0.0, 0.0, 0.0));
                        if ((camera_comp || environment_comp))
                        {
                            ImGui::EndGroup();
                            ImGui::PopItemFlag();
                            ImGui::PopStyleVar();
                            if (ImGui::IsItemHovered())
                                ImGui::SetTooltip("Disabled for %s", (environment_comp ? "environments" : "cameras"));
                        }
                        ImGui::EndTabItem();
                    }
                    if (ImGui::BeginTabItem(("Scale##scale" + std::to_string(e)).c_str()))
                    {
                        if ((camera_comp || environment_comp))
                        {
                            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                            ImGui::BeginGroup();
                        }

                        ImGui::DragFloat(("X##s_delta_x" + std::to_string(e)).c_str(), &transform_comp->scale.x, 0.01f, 0.0f, 0.0f, "%.2f");
                        ImGui::DragFloat(("Y##s_delta_y" + std::to_string(e)).c_str(), &transform_comp->scale.y, 0.01f, 0.0f, 0.0f, "%.2f");
                        ImGui::DragFloat(("Z##s_delta_z" + std::to_string(e)).c_str(), &transform_comp->scale.z, 0.01f, 0.0f, 0.0f, "%.2f");

                        ImGui::Spacing();
                        if (ImGui::Button(("Clear##scale" + std::to_string(e)).c_str()))
                            transform_comp->scale = glm::vec3(1.0f);
                        if ((camera_comp || environment_comp))
                        {
                            ImGui::EndGroup();
                            ImGui::PopItemFlag();
                            ImGui::PopStyleVar();
                            if (ImGui::IsItemHovered())
                                ImGui::SetTooltip("Disabled for %s", (environment_comp ? "environments" : "cameras"));
                        }
                        ImGui::EndTabItem();
                    }
                    ImGui::EndTabBar();
                    // TODO Paul: Removing transforms? .... Should be possible.
                    // ImGui::SameLine();
                    // if (ImGui::Button(("Remove##transform" + std::to_string(e)).c_str()))
                    //     application_scene->remove_transform_component(e);
                    ImGui::Separator();
                    ImGui::TreePop();
                }
            }

            // Model
            if (model_comp)
            {
                if (ImGui::TreeNode(("Model Component##" + std::to_string(e)).c_str()))
                {
                    ImGui::Spacing();
                    if (model_comp->model_file_path.empty())
                    {
                        ImGui::Text("No Model loaded!");
                        if (ImGui::Button("Load"))
                        {
                            char const* filter[2] = { "*.gltf", "*.glb" };

                            char* query_path = tinyfd_openFileDialog("", "res/", 2, filter, NULL, 0);
                            if (query_path)
                            {
                                string queried = string(query_path);
                                auto ext       = queried.substr(queried.find_last_of(".") + 1);
                                if (ext == "glb" || ext == "gltf")
                                    application_scene->create_entities_from_model(queried, e);
                            }
                        }
                        // Removing only possible when no model is loaded ... TODO Paul?
                        if (ImGui::Button(("Remove##model" + std::to_string(e)).c_str()))
                            application_scene->remove_model_component(e);
                    }
                    else
                    {
                        ImGui::Text(("Model loaded from: '" + model_comp->model_file_path + "'").c_str());
                        glm::vec3 min = model_comp->min_extends;
                        glm::vec3 max = model_comp->max_extends;
                        if (transform_comp)
                        {
                            min = glm::vec3(transform_comp->world_transformation_matrix * glm::vec4(min, 1.0f));
                            max = glm::vec3(transform_comp->world_transformation_matrix * glm::vec4(max, 1.0f));
                        }
                        // TODO Paul: These are only correct, if we do not change any mesh transformations.
                        ImGui::Text(("Min Extends: [ " + std::to_string(min.x) + ", " + std::to_string(min.y) + ", " + std::to_string(min.z) + "]").c_str());
                        ImGui::Text(("Max Extends: [ " + std::to_string(max.x) + ", " + std::to_string(max.y) + ", " + std::to_string(max.z) + "]").c_str());
                    }

                    ImGui::Separator();
                    ImGui::TreePop();
                }
            }

            // Mesh
            if (mesh_comp)
            {
                if (ImGui::TreeNode(("Mesh Component##" + std::to_string(e)).c_str()))
                {
                    ImGui::Spacing();
                    ImGui::Text("Primitive/Material List:");
                    if (ImGui::ListBoxHeader(("##primitives_materials_list_box" + std::to_string(e)).c_str(), ImVec2(ImGui::GetWindowWidth() * 0.5f, 64)))
                    {
                        for (auto m : mesh_comp->materials)
                            ImGui::Text(("Primitive with " + m.material_name + " material").c_str());
                        ImGui::ListBoxFooter();
                    }
                    ImGui::Checkbox(("Has normals##mesh" + std::to_string(e)).c_str(), &mesh_comp->has_normals);
                    ImGui::Checkbox(("Has tangents##mesh" + std::to_string(e)).c_str(), &mesh_comp->has_tangents);
                    ImGui::Spacing();
                    if (ImGui::Button(("Remove##mesh" + std::to_string(e)).c_str()))
                        application_scene->remove_mesh_component(e);
                    ImGui::Separator();
                    ImGui::TreePop();
                }
            }

            // Camera
            if (camera_comp)
            {
                if (ImGui::TreeNode(("Camera Component##" + std::to_string(e)).c_str()))
                {
                    ImGui::Spacing();
                    entity cam  = application_scene->get_active_camera_data().active_camera_entity;
                    bool active = cam == e;
                    ImGui::Checkbox(("Make Scene Camera##camera" + std::to_string(e)).c_str(), &active);
                    if (active)
                        application_scene->set_active_camera(e);
                    const char* types  = "perspective\0orthographic\0\0";
                    int32 cam_type_int = static_cast<int32>(camera_comp->cam_type);
                    ImGui::Combo(("Camera Type##camera" + std::to_string(e)).c_str(), &cam_type_int, types);
                    camera_comp->cam_type = static_cast<camera_type>(cam_type_int);
                    ImGui::SliderFloat(("Near Plane##camera" + std::to_string(e)).c_str(), &camera_comp->z_near, 0.0f, camera_comp->z_far);
                    ImGui::SliderFloat(("Far Plane##camera" + std::to_string(e)).c_str(), &camera_comp->z_far, camera_comp->z_near, 1000.0f);
                    if (camera_comp->cam_type == camera_type::perspective_camera)
                    {
                        ImGui::SliderAngle(("Vertical Field Of View##camera" + std::to_string(e)).c_str(), &camera_comp->perspective.vertical_field_of_view, 1.75f, 175.0f, "%.0f°");
                        ImGui::Text(("Aspect: " + std::to_string(camera_comp->perspective.aspect)).c_str());
                        if (ImGui::Button(("Aspect to viewport##camera" + std::to_string(e)).c_str()))
                        {
                            camera_comp->perspective.aspect = viewport_size.x / viewport_size.y;
                        }
                    }
                    else // orthographic
                    {
                        ImGui::SliderFloat(("Magnification X##camera" + std::to_string(e)).c_str(), &camera_comp->orthographic.x_mag, 0.1f, 100.0f);
                        ImGui::SliderFloat(("Magnification Y##camera" + std::to_string(e)).c_str(), &camera_comp->orthographic.y_mag, 0.1f, 100.0f);
                        if (ImGui::Button(("Magnification to viewport##camera" + std::to_string(e)).c_str()))
                        {
                            camera_comp->orthographic.x_mag = viewport_size.x / viewport_size.y;
                            camera_comp->orthographic.y_mag = 1.0f;
                        }
                    }
                    ImGui::DragFloat3(("Target##camera" + std::to_string(e)).c_str(), &camera_comp->target.x, 0.1f, 0.0f, 0.0f, "%.1f");

                    ImGui::Checkbox(("Adaptive Exposure##camera" + std::to_string(e)).c_str(), &camera_comp->physical.adaptive_exposure);

                    if (camera_comp->physical.adaptive_exposure)
                    {
                        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                        ImGui::BeginGroup();
                    }

                    ImGui::InputFloat(("Aperture##camera" + std::to_string(e)).c_str(), &camera_comp->physical.aperture, 0.1f, 1.0f, "%.1f");
                    ImGui::InputFloat(("Shutter Speed##camera" + std::to_string(e)).c_str(), &camera_comp->physical.shutter_speed, 0.0001f, 0.1f, "%.5f");
                    ImGui::InputFloat(("Iso##camera" + std::to_string(e)).c_str(), &camera_comp->physical.iso, 10.0f, 100.0f, "%.1f");

                    if (camera_comp->physical.adaptive_exposure)
                    {
                        ImGui::EndGroup();
                        ImGui::PopItemFlag();
                        ImGui::PopStyleVar();
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("Adaptive Exposure is activated");
                    }

                    ImGui::Spacing();
                    if (ImGui::Button(("Remove##camera" + std::to_string(e)).c_str()))
                        application_scene->remove_camera_component(e);
                    ImGui::Separator();
                    ImGui::TreePop();
                }
            }

            // Environment
            if (environment_comp)
            {
                if (ImGui::TreeNode(("Environment Component##" + std::to_string(e)).c_str()))
                {
                    ImGui::Spacing();
                    entity env  = application_scene->get_active_environment_data().active_environment_entity;
                    bool active = env == e;
                    if (environment_comp->hdr_texture)
                        ImGui::Checkbox(("Make Scene Environment##camera" + std::to_string(e)).c_str(), &active);
                    if (env != e && active)
                        application_scene->set_active_environment(e);

                    ImVec2 canvas_p0;
                    ImDrawList* draw_list = ImGui::GetWindowDrawList();
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
                    canvas_p0 = ImGui::GetCursorScreenPos();
                    draw_list->AddRectFilled(canvas_p0, ImVec2(canvas_p0.x + 128, canvas_p0.y + 64), IM_COL32(127, 127, 127, 255), 2.0f);
                    if (environment_comp->hdr_texture)
                        ImGui::Image((void*)(intptr_t)environment_comp->hdr_texture->get_name(), ImVec2(128, 64));
                    else
                        ImGui::Dummy(ImVec2(128, 64));
                    if (ImGui::IsItemHovered())
                    {
                        if (environment_comp->hdr_texture)
                        {
                            ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.5, 0.5, 0.5, 1.0));
                            ImGui::BeginTooltip();
                            ImGui::Image((void*)(intptr_t)environment_comp->hdr_texture->get_name(), ImVec2(512, 256));
                            ImGui::EndTooltip();
                            ImGui::PopStyleColor();
                        }
                        else
                            ImGui::SetTooltip("Load HDR image");
                        draw_list->AddRect(canvas_p0, ImVec2(canvas_p0.x + 128, canvas_p0.y + 64), IM_COL32(200, 200, 200, 255), 2.0f);
                    }
                    ImGui::PopStyleVar();
                    if (ImGui::IsItemClicked())
                    {
                        texture_configuration tex_config;
                        tex_config.m_generate_mipmaps        = 1;
                        tex_config.m_is_standard_color_space = false;
                        tex_config.m_texture_min_filter      = texture_parameter::filter_linear;
                        tex_config.m_texture_mag_filter      = texture_parameter::filter_linear;
                        tex_config.m_texture_wrap_s          = texture_parameter::wrap_clamp_to_edge;
                        tex_config.m_texture_wrap_t          = texture_parameter::wrap_clamp_to_edge;

                        char const* filter[1] = { "*.hdr" };

                        environment_comp->hdr_texture = details::load_texture(tex_config, rs, filter, 1);
                        application_scene->set_active_environment(e);
                    }
                    if (environment_comp->hdr_texture)
                    {
                        ImGui::SliderFloat(("Intensity##environment_intensity" + std::to_string(e)).c_str(), &environment_comp->intensity, 0.0f, 50000.0f);
                    }

                    ImGui::Spacing();
                    if (ImGui::Button(("Remove##environment" + std::to_string(e)).c_str()))
                    {
                        if (active)
                            application_scene->set_active_environment(invalid_entity);
                        application_scene->remove_environment_component(e);
                    }
                    ImGui::Separator();
                    ImGui::TreePop();
                }
            }

            // Light
            if (light_comp)
            {
                if (ImGui::TreeNode(("Light Component##" + std::to_string(e)).c_str()))
                {
                    ImGui::Spacing();
                    ImGui::Text("Light Type");
                    light_type current      = light_comp->type_of_light;
                    const char* possible[1] = { "Directional Light" };
                    int32 idx               = static_cast<int32>(current);
                    ImGui::ListBox(("##light_type_selection" + std::to_string(e)).c_str(), &idx, possible, 1);
                    current = static_cast<light_type>(idx);

                    if (current == light_type::directional) // Always true atm.
                    {
                        auto d_data = static_cast<directional_light_data*>(light_comp->data.get());

                        ImGui::DragFloat(("Direction X##d_light_direction_x" + std::to_string(e)).c_str(), &d_data->direction.x, 0.05f, 0.0f, 0.0f, "%.2f");
                        ImGui::DragFloat(("Direction Y##d_light_direction_y" + std::to_string(e)).c_str(), &d_data->direction.y, 0.05f, 0.0f, 0.0f, "%.2f");
                        ImGui::DragFloat(("Direction Z##d_light_direction_z" + std::to_string(e)).c_str(), &d_data->direction.z, 0.05f, 0.0f, 0.0f, "%.2f");

                        ImGui::ColorEdit4("Color##d_light_color", d_data->light_color, ImGuiColorEditFlags_NoInputs);

                        ImGui::SliderFloat(("Intensity##d_light_intensity" + std::to_string(e)).c_str(), &d_data->intensity, 0.0f, 500000.0f);
                        ImGui::Checkbox(("Cast Shadows##d_light_cast" + std::to_string(e)).c_str(), &d_data->cast_shadows);
                    }

                    ImGui::Spacing();
                    if (ImGui::Button(("Remove##light" + std::to_string(e)).c_str()))
                    {
                        application_scene->remove_light_component(e);
                    }
                    ImGui::Separator();
                    ImGui::TreePop();
                }
            }
            ImGui::PopStyleVar();
        }
        ImGui::End();
    }

    //! \brief Draws the render system widget for a given \a render_system_impl.
    //! \param[in] rs The \a render_system_impl.
    //! \param[in,out] enabled True if the window is open, else False.
    void render_system_widget(const shared_ptr<render_system_impl>& rs, bool& enabled)
    {
        // Because every pipeline could have different properties, we will have to delegate that.
        ImGui::Begin("Render System", &enabled);
        if (enabled)
            rs->on_ui_widget();
        ImGui::End();
    }

} // namespace mango

#endif // MANGO_IMGUI_WIDGETS_HPP
