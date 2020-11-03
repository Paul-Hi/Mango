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
#include <mango/imgui_helper.hpp>
#include <mango/scene.hpp>
#include <rendering/render_system_impl.hpp>
#include <resources/resource_system.hpp>
#include <ui/dear_imgui/icons_font_awesome_5.hpp>
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
        const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding;
        if (ImGui::CollapsingHeader("Editor Stats", flags))
        {
            float frametime = shared_context->get_application()->frame_time();
            custom_info("Frame Time:", [frametime]() {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%.2f ms", frametime * 1000.0f);
            });

            static int32 idx = 0;
            static float frametimes[60];
            frametimes[idx] = 1.0f / frametime;
            float* ft       = frametimes;
            int32 idx_      = idx;
            idx++;
            idx %= 60;
            float max = shared_context->get_window_system_internal().lock()->vsync() ? 75.0f : 650.0f;
            custom_info("Frame Rate:", [ft, idx_, max]() {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%.2f fps", ft[idx_]);
                ImGui::PlotLines("", ft, 60, 0, "", 0.0f, max, ImVec2(0, 64));
            });
        }
        auto stats = shared_context->get_render_system_internal().lock()->get_hardware_stats();
        if (ImGui::CollapsingHeader("Renderer Stats", flags))
        {
            column_split("split", 2, ImGui::GetContentRegionAvail().x * 0.33f);

            text_wrapped("API Version:");
            column_next();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%s", stats.api_version.c_str());
            column_next();
            ImGui::SeparatorEx(ImGuiSeparatorFlags_SpanAllColumns | ImGuiSeparatorFlags_Horizontal);
            text_wrapped("Rendered Meshes:");
            column_next();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%d", stats.last_frame.meshes);
            column_next();
            ImGui::SeparatorEx(ImGuiSeparatorFlags_SpanAllColumns | ImGuiSeparatorFlags_Horizontal);
            text_wrapped("Draw Calls:");
            column_next();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%d", stats.last_frame.draw_calls);
            column_next();
            ImGui::SeparatorEx(ImGuiSeparatorFlags_SpanAllColumns | ImGuiSeparatorFlags_Horizontal);
            text_wrapped("Rendered Primitives:");
            column_next();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%d", stats.last_frame.primitives);
            column_next();
            ImGui::SeparatorEx(ImGuiSeparatorFlags_SpanAllColumns | ImGuiSeparatorFlags_Horizontal);
            text_wrapped("Rendered Materials:");
            column_next();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%d", stats.last_frame.materials);
            column_next();
            ImGui::SeparatorEx(ImGuiSeparatorFlags_SpanAllColumns | ImGuiSeparatorFlags_Horizontal);
            text_wrapped("Canvas Size:");
            column_next();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("(%d x %d) px", stats.last_frame.canvas_width, stats.last_frame.canvas_height);

            column_merge();
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

        string get_icon_for_entity(const shared_ptr<scene>& application_scene, entity e)
        {
            if (application_scene->query_tag(e)->tag_name == "Scene")
                return string(ICON_FA_SITEMAP);
            else if (application_scene->query_model_component(e) || application_scene->query_mesh_component(e))
                return string(ICON_FA_DICE_D6);
            else if (application_scene->query_camera_component(e))
                return string(ICON_FA_VIDEO);
            else if (application_scene->query_light_component(e) || application_scene->query_environment_component(e))
                return string(ICON_FA_LIGHTBULB);
            else if (application_scene->query_transform_component(e))
                return string(ICON_FA_VECTOR_SQUARE);
            return "";
        }

        //! \brief Draws the subtree for a given entity in the user interface.
        //! \param[in] application_scene The current \a scene of the \a application.
        //! \param[in] e The entity to draw the subtree for.
        //! \param[in,out] selected The currently selected entity, can be updated by this function.
        void draw_entity_tree(const shared_ptr<scene>& application_scene, entity e, entity& selected)
        {
            auto tag = application_scene->query_tag(e);
            if (!tag)
                tag = &application_scene->add_tag(e);
            auto name     = (tag && !tag->tag_name.empty()) ? tag->tag_name : "Unnamed Entity";
            auto children = application_scene->get_children(e);

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 5));
            const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_FramePadding |
                                             ImGuiTreeNodeFlags_AllowItemOverlap | ((selected == e) ? ImGuiTreeNodeFlags_Selected : 0) | (children.empty() ? ImGuiTreeNodeFlags_Leaf : 0);

            ImGui::PushID(e);

            if (selected != invalid_entity && selected != e)
            {
                for (auto child : children) // TODO Paul: We need a better way, this can potentially kill performance ...
                {
                    if (child == selected)
                    {
                        ImGui::SetNextItemOpen(true);
                        break;
                    }
                }
            }

            string icon = get_icon_for_entity(application_scene, e);
            bool open   = ImGui::TreeNodeEx(name.c_str(), flags, "%s", (icon + "  " + name).c_str());

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
            ImGui::PopID();
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
                ImGui::PushID(material.get());
                texture_configuration config;
                config.m_generate_mipmaps        = 1;
                config.m_is_standard_color_space = true;
                config.m_texture_min_filter      = texture_parameter::filter_linear_mipmap_linear;
                config.m_texture_mag_filter      = texture_parameter::filter_linear;
                config.m_texture_wrap_s          = texture_parameter::wrap_repeat;
                config.m_texture_wrap_t          = texture_parameter::wrap_repeat;
                char const* filter[4]            = { "*.png", "*.jpg", "*.jpeg", "*.bmp" };

                ImVec2 canvas_p0;
                ImDrawList* draw_list          = ImGui::GetWindowDrawList();
                const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding;

                // base color

                if (ImGui::CollapsingHeader("Base Color", flags | ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::PushID("Base Color");
                    bool changed  = false;
                    bool load_new = false;
                    changed |= image_load("Base Color Texture", material->base_color_texture ? material->base_color_texture->get_name() : -1, glm::vec2(64, 64), load_new);

                    if (load_new)
                    {
                        config.m_is_standard_color_space = true;
                        material->base_color_texture     = load_texture(config, rs, filter, 4);
                        material->use_base_color_texture = (material->base_color_texture != nullptr);
                    }
                    else if (changed)
                    {
                        material->base_color_texture     = nullptr;
                        material->use_base_color_texture = false;
                    }

                    changed = checkbox("Use Texture", &material->use_base_color_texture, false);
                    if (changed && material->use_base_color_texture && !material->base_color_texture)
                    {
                        config.m_is_standard_color_space = true;
                        material->base_color_texture     = load_texture(config, rs, filter, 4);
                        material->use_base_color_texture = (material->base_color_texture != nullptr);
                    }
                    ImGui::Separator();

                    if (!material->use_base_color_texture)
                        color_edit("Color", &material->base_color[0], 4, 1.0f);

                    ImGui::Separator();
                    ImGui::PopID();
                }

                // roughness metallic

                if (ImGui::CollapsingHeader("Roughness And Metallic", flags))
                {
                    ImGui::PushID("Roughness And Metallic");
                    bool changed  = false;
                    bool load_new = false;
                    changed |= image_load("Roughness And Metallic Texture", material->roughness_metallic_texture ? material->roughness_metallic_texture->get_name() : -1, glm::vec2(64, 64), load_new);

                    if (load_new)
                    {
                        config.m_is_standard_color_space         = false;
                        material->roughness_metallic_texture     = load_texture(config, rs, filter, 4);
                        material->use_roughness_metallic_texture = (material->roughness_metallic_texture != nullptr);
                    }
                    else if (changed)
                    {
                        material->roughness_metallic_texture     = nullptr;
                        material->use_roughness_metallic_texture = false;
                    }

                    changed = checkbox("Use Texture", &material->use_roughness_metallic_texture, false);
                    if (changed && material->use_roughness_metallic_texture && !material->roughness_metallic_texture)
                    {
                        config.m_is_standard_color_space         = false;
                        material->roughness_metallic_texture     = load_texture(config, rs, filter, 4);
                        material->use_roughness_metallic_texture = (material->roughness_metallic_texture != nullptr);
                    }
                    ImGui::Separator();

                    if (material->use_roughness_metallic_texture)
                    {
                        checkbox("Has Packed AO", &material->packed_occlusion, false);
                        if (material->packed_occlusion)
                            checkbox("Use Packed AO", &material->use_packed_occlusion, true);
                    }
                    else
                    {
                        slider_float_n("Roughness", material->roughness.type_data(), 1, 0.5f, 0.0f, 1.0f);
                        slider_float_n("Metallic", material->metallic.type_data(), 1, 0.5f, 0.0f, 1.0f);
                    }

                    ImGui::Separator();
                    ImGui::PopID();
                }

                // normal

                if (ImGui::CollapsingHeader("Normal Map", flags))
                {
                    ImGui::PushID("Normal Map");
                    bool changed  = false;
                    bool load_new = false;
                    changed |= image_load("Normal Texture", material->normal_texture ? material->normal_texture->get_name() : -1, glm::vec2(64, 64), load_new);

                    if (load_new)
                    {
                        config.m_is_standard_color_space = false;
                        material->normal_texture         = load_texture(config, rs, filter, 4);
                        material->use_normal_texture     = (material->normal_texture != nullptr);
                    }
                    else if (changed)
                    {
                        material->normal_texture     = nullptr;
                        material->use_normal_texture = false;
                    }

                    changed = checkbox("Use Texture", &material->use_normal_texture, false);
                    if (changed && material->use_normal_texture && !material->normal_texture)
                    {
                        config.m_is_standard_color_space = false;
                        material->normal_texture         = load_texture(config, rs, filter, 4);
                        material->use_normal_texture     = (material->normal_texture != nullptr);
                    }

                    ImGui::Separator();
                    ImGui::PopID();
                }

                // occlusion

                if (ImGui::CollapsingHeader("Occlusion Map", flags))
                {
                    ImGui::PushID("Occlusion Map");
                    bool changed  = false;
                    bool load_new = false;
                    changed |= image_load("Occlusion Texture", material->occlusion_texture ? material->occlusion_texture->get_name() : -1, glm::vec2(64, 64), load_new);

                    if (load_new)
                    {
                        config.m_is_standard_color_space = false;
                        material->occlusion_texture      = load_texture(config, rs, filter, 4);
                        material->use_occlusion_texture  = (material->occlusion_texture != nullptr);
                    }
                    else if (changed)
                    {
                        material->occlusion_texture     = nullptr;
                        material->use_occlusion_texture = false;
                    }

                    changed = checkbox("Use Texture", &material->use_occlusion_texture, false);
                    if (changed && material->use_occlusion_texture && !material->occlusion_texture)
                    {
                        config.m_is_standard_color_space = false;
                        material->occlusion_texture      = load_texture(config, rs, filter, 4);
                        material->use_occlusion_texture  = (material->occlusion_texture != nullptr);
                    }

                    ImGui::Separator();
                    ImGui::PopID();
                }

                // emissive

                if (ImGui::CollapsingHeader("Emissive", flags))
                {
                    ImGui::PushID("Emissive");
                    bool changed  = false;
                    bool load_new = false;
                    changed |= image_load("Emissive Color Texture", material->emissive_color_texture ? material->emissive_color_texture->get_name() : -1, glm::vec2(64, 64), load_new);

                    if (load_new)
                    {
                        config.m_is_standard_color_space     = true;
                        material->emissive_color_texture     = load_texture(config, rs, filter, 4);
                        material->use_emissive_color_texture = (material->emissive_color_texture != nullptr);
                    }
                    else if (changed)
                    {
                        material->emissive_color_texture     = nullptr;
                        material->use_emissive_color_texture = false;
                    }

                    changed = checkbox("Use Texture", &material->use_emissive_color_texture, false);
                    if (changed && material->use_emissive_color_texture && !material->emissive_color_texture)
                    {
                        config.m_is_standard_color_space     = true;
                        material->emissive_color_texture     = load_texture(config, rs, filter, 4);
                        material->use_emissive_color_texture = (material->emissive_color_texture != nullptr);
                    }
                    ImGui::Separator();

                    if (!material->use_emissive_color_texture)
                        color_edit("Color", &material->emissive_color[0], 3, 1.0f);

                    ImGui::Separator();
                    ImGui::PopID();
                }
                ImGui::Separator();
                ImGui::Spacing();

                checkbox("Double Sided", &material->double_sided, false);

                ImGui::Separator();

                const char* types[4] = { "Opaque", "Masked", "Blended", "Dithered" };
                int32 idx            = static_cast<int32>(material->alpha_rendering);
                combo("Alpha Mode", types, 4, idx, 0);
                material->alpha_rendering = static_cast<alpha_mode>(idx);

                if (material->alpha_rendering == alpha_mode::mode_mask)
                    slider_float_n("Alpha CutOff", material->alpha_cutoff.type_data(), 1, 0.5f, 0.0f, 1.0f, "%.2f");
                if (material->alpha_rendering == alpha_mode::mode_blend)
                    custom_info(
                        "Blending With Basic Over Operator!", []() {}, 0.0f, ImGui::GetContentRegionAvail().x);
                if (material->alpha_rendering == alpha_mode::mode_dither)
                    custom_info(
                        "Dithering ... Just For Fun!", []() {}, 0.0f, ImGui::GetContentRegionAvail().x);

                ImGui::PopID();
            }
        }

        template <typename component>
        void draw_component(entity e, component* comp, std::function<void()> component_draw_function, std::function<void()> additional = nullptr)
        {
            if (!comp)
                return;

            std::string comp_name = std::string(type_name<component>::get());

            ImGui::PushID(comp_name.c_str());
            const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_DefaultOpen;
            bool open                      = (ImGui::CollapsingHeader(comp_name.c_str(), flags));

            if (additional)
            {
                float line_height     = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
                float avaliable_width = ImGui::GetContentRegionAvail().x;
                ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));
                ImGui::SameLine(avaliable_width - line_height * 0.5f);
                ImGui::PushID("additional");
                if (ImGui::Button("+", ImVec2(line_height, line_height)))
                {
                    ImGui::OpenPopup("##");
                }
                if (ImGui::BeginPopup("##"))
                {
                    additional();
                    ImGui::EndPopup();
                }
                ImGui::PopStyleVar();
                ImGui::PopID();
            }

            if (open)
            {
                ImGui::Spacing();
                component_draw_function();
                ImGui::Separator();
            }

            ImGui::PopID();
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
        if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered())
        {
            if (ImGui::IsMouseClicked(0))
                selected = invalid_entity;
            if (!ImGui::IsPopupOpen("##scene_menu") && ImGui::IsMouseClicked(1))
                ImGui::OpenPopup("##scene_menu");
        }
        if (ImGui::BeginPopup("##scene_menu"))
        {
            if (ImGui::Selectable("Add Entity##scene_menu"))
            {
                selected = application_scene->create_empty();
                application_scene->attach(selected, root);
            }

            ImGui::EndPopup();
        }

        details::draw_entity_tree(application_scene, root, selected);
        ImGui::End();
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
            ImGui::PushID(e);
            static int32 current_id = 0;
            auto current_name       = !mesh->materials[current_id].material_name.empty() ? mesh->materials[current_id].material_name : "Unnamed Material " + std::to_string(current_id);
            int32 return_id         = current_id;
            custom_aligned("Materials", [&mesh, &current_name, &return_id](bool reset) {
                bool value_changed = false;
                ImGui::PushItemWidth(ImGui::CalcItemWidth());
                if (ImGui::BeginCombo("", current_name.c_str()))
                {
                    for (int32 n = 0; n < static_cast<int32>(mesh->materials.size()); ++n)
                    {
                        bool is_selected = (current_name == mesh->materials[n].material_name);
                        auto name        = !mesh->materials[n].material_name.empty() ? mesh->materials[n].material_name : "Unnamed Material " + std::to_string(n);
                        if (ImGui::Selectable(name.c_str(), is_selected))
                        {
                            return_id     = n;
                            value_changed = true;
                        }
                        if (is_selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                ImGui::PopItemWidth();
                if (reset)
                    return_id = 0;
                return value_changed;
            });
            current_id = return_id;
            if (entity_changed)
                current_id = 0;
            details::draw_material(mesh->materials[current_id].component_material, rs, e);
            ImGui::PopID();
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

            ImGui::PushID(e);

            // Tag
            if (!tag_comp)
                tag_comp = &application_scene->add_tag(e);
            std::array<char, 32> tmp_string; // TODO Paul: Max length? 32 enough for now?
            auto icon = details::get_icon_for_entity(application_scene, e);
            ImGui::Text(icon.c_str());
            ImGui::SameLine();
            ImGui::PushItemWidth(-1);
            strcpy(tmp_string.data(), tag_comp->tag_name.c_str()); // TODO Paul: Kind of fishy.
            ImGui::InputTextWithHint("##tag", "Enter Entity Tag", tmp_string.data(), 32);
            tag_comp->tag_name = tmp_string.data();
            ImGui::PopItemWidth();

            if (ImGui::Button(ICON_FA_DOT_CIRCLE "  Add Component", ImVec2(-1, 0)))
            {
                ImGui::OpenPopup("##component_addition_popup");
            }

            ImGui::Spacing();

            if (ImGui::BeginPopup("##component_addition_popup"))
            {
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

            // Transform
            details::draw_component<mango::transform_component>(e, transform_comp, [e, &application_scene, &transform_comp, &environment_comp, &camera_comp]() {
                ImGui::BeginGroup();
                if (environment_comp)
                {
                    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                }

                // translation
                drag_float_n("Translation", &transform_comp->position[0], 3, 0.0f, 0.08f, 0.0f, 0.0f, "%.2f", true);

                if (camera_comp && !environment_comp)
                {
                    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                }

                // rotation
                glm::vec3 rotation_hint_before = transform_comp->rotation_hint;
                drag_float_n("Rotation", &transform_comp->rotation_hint[0], 3, 0.0f, 0.08f, 0.0f, 0.0f, "%.2f", true);
                glm::quat x_quat         = glm::angleAxis(glm::radians(transform_comp->rotation_hint.x - rotation_hint_before.x), glm::vec3(1.0f, 0.0f, 0.0f));
                glm::quat y_quat         = glm::angleAxis(glm::radians(transform_comp->rotation_hint.y - rotation_hint_before.y), glm::vec3(0.0f, 1.0f, 0.0f));
                glm::quat z_quat         = glm::angleAxis(glm::radians(transform_comp->rotation_hint.z - rotation_hint_before.z), glm::vec3(0.0f, 0.0f, 1.0f));
                transform_comp->rotation = x_quat * y_quat * z_quat * transform_comp->rotation;

                // scale
                drag_float_n("Scale", &transform_comp->scale[0], 3, 1.0f, 0.08f, 0.0f, 0.0f, "%.2f", true);

                ImGui::EndGroup();
                if (camera_comp || environment_comp)
                {
                    ImGui::PopItemFlag();
                    ImGui::PopStyleVar();
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Disabled For %s", (environment_comp ? "Environments" : "Cameras"));
                }
                // TODO Paul: Removing transforms? .... Should be possible.
            });

            // Model
            details::draw_component<mango::model_component>(
                e, model_comp,
                [e, &application_scene, &model_comp, &transform_comp]() {
                    if (model_comp->model_file_path.empty())
                        custom_info(
                            "No Model Loaded!", []() {}, 0.0f, ImGui::GetContentRegionAvail().x);
                    else
                        custom_info("Model Loaded:", [&model_comp]() {
                            ImGui::AlignTextToFramePadding();
                            text_wrapped(model_comp->model_file_path.c_str());
                        });

                    ImGui::PushItemWidth(-1);
                    if (ImGui::Button("Import"))
                    {
                        char const* filter[2] = { "*.gltf", "*.glb" };

                        char* query_path = tinyfd_openFileDialog("", "res/", 2, filter, NULL, 0);
                        if (query_path)
                        {
                            string queried = string(query_path);
                            auto ext       = queried.substr(queried.find_last_of(".") + 1);
                            if (ext == "glb" || ext == "gltf")
                            {
                                application_scene->remove_model_component(e); // TODO Paul: This could be done cleaner.
                                application_scene->create_entities_from_model(queried, e);
                            }
                        }
                    }
                    ImGui::PopItemWidth();
                },
                [e, &application_scene]() {
                    // TODO Paul: We can not remove the component without removing model entities?
                    if (ImGui::Selectable("Remove"))
                        application_scene->remove_model_component(e);
                });

            // Mesh
            details::draw_component<mango::mesh_component>(
                e, mesh_comp,
                [e, &application_scene, &mesh_comp]() {
                    auto primitive_count = mesh_comp->primitives.size();
                    custom_info("Primitive Count", [primitive_count]() {
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text((std::to_string(primitive_count)).c_str());
                    });
                    auto material_count = mesh_comp->materials.size();
                    custom_info("Material Count", [material_count]() {
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text((std::to_string(material_count)).c_str());
                    });
                    checkbox("Has Normals", &mesh_comp->has_normals, false);
                    checkbox("Has Tangents", &mesh_comp->has_tangents, false);
                },
                [e, &application_scene]() {
                    if (ImGui::Selectable("Remove"))
                        application_scene->remove_mesh_component(e);
                });

            // Camera
            details::draw_component<mango::camera_component>(
                e, camera_comp,
                [e, &application_scene, &camera_comp, &viewport_size]() {
                    entity cam   = application_scene->get_active_camera_data().active_camera_entity;
                    bool active  = cam == e;
                    bool changed = checkbox("Set active", &active, false);

                    if ((changed || cam != e) && active)
                        application_scene->set_active_camera(e);
                    else if (cam == e && changed && !active)
                        application_scene->set_active_camera(invalid_entity);

                    ImGui::Separator();
                    const char* types[2] = { "Perspective", "Orthographic" };
                    int32 idx            = static_cast<int32>(camera_comp->cam_type);
                    combo("Camera Type", types, 2, idx, 0);
                    camera_comp->cam_type = static_cast<camera_type>(idx);
                    ImGui::Separator();

                    slider_float_n("Near Plane", &camera_comp->z_near, 1, 0.4f, 0.0f, camera_comp->z_far);
                    slider_float_n("Far Plane", &camera_comp->z_far, 1, 40.0f, camera_comp->z_near, 10000.0f);
                    if (camera_comp->cam_type == camera_type::perspective_camera)
                    {
                        float degree_fov = glm::degrees(camera_comp->perspective.vertical_field_of_view);
                        slider_float_n("Vertical FOV", &degree_fov, 1, 45.0f, 1.75f, 175.0f, "%.1f°");
                        camera_comp->perspective.vertical_field_of_view = glm::radians(degree_fov);
                        custom_aligned("Aspect", [&camera_comp, &viewport_size](bool reset) {
                            ImGui::AlignTextToFramePadding();
                            ImGui::Text((std::to_string(camera_comp->perspective.aspect) + " ").c_str());
                            ImGui::SameLine();
                            if (ImGui::Button("Aspect To Viewport", ImVec2(-1, 0)))
                            {
                                camera_comp->perspective.aspect = viewport_size.x / viewport_size.y;
                                return true;
                            }
                            if (reset)
                            {
                                camera_comp->perspective.aspect = 16.0f / 9.0f;
                                return true;
                            }
                            return false;
                        });
                    }
                    else // orthographic
                    {
                        slider_float_n("Magnification X", &camera_comp->orthographic.x_mag, 1, 1.0f, 0.1f, 100.0f, "%.1f");
                        slider_float_n("Magnification Y", &camera_comp->orthographic.y_mag, 1, 1.0f, 0.1f, 100.0f, "%.1f");
                        custom_aligned("Magnification", [&camera_comp, &viewport_size](bool reset) {
                            if (ImGui::Button("Magnification To Viewport", ImVec2(-1, 0)))
                            {
                                camera_comp->orthographic.x_mag = viewport_size.x / viewport_size.y;
                                camera_comp->orthographic.y_mag = 1.0f;
                                return true;
                            }
                            if (reset)
                            {
                                camera_comp->orthographic.x_mag = 1.0f;
                                camera_comp->orthographic.y_mag = 1.0f;
                                return true;
                            }
                            return false;
                        });
                    }
                    ImGui::Separator();
                    drag_float_n("Target", &camera_comp->target.x, 3, 0.0f, 0.1f, 0.0, 0.0, "%.1f", true);

                    ImGui::Separator();
                    checkbox("Adaptive Exposure", &camera_comp->physical.adaptive_exposure, false);

                    ImGui::BeginGroup();
                    if (camera_comp->physical.adaptive_exposure)
                    {
                        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                    }

                    drag_float_n("Aperture", &camera_comp->physical.aperture, 1, mango::default_aperture, 0.1f, mango::min_aperture, mango::max_aperture, "%.1f");
                    drag_float_n("Shutter Speed", &camera_comp->physical.shutter_speed, 1, mango::default_shutter_speed, 0.0001f, mango::min_shutter_speed, mango::max_shutter_speed, "%.5f");
                    drag_float_n("Iso", &camera_comp->physical.iso, 1, mango::default_iso, 0.1f, mango::min_iso, mango::max_iso, "%.1f");

                    ImGui::EndGroup();
                    if (camera_comp->physical.adaptive_exposure)
                    {
                        ImGui::PopItemFlag();
                        ImGui::PopStyleVar();
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("Adaptive Exposure Controlled");
                    }
                },
                [e, &application_scene]() {
                    if (ImGui::Selectable("Remove"))
                        application_scene->remove_camera_component(e);
                });

            // Environment
            details::draw_component<mango::environment_component>(
                e, environment_comp,
                [e, &application_scene, &environment_comp, &rs]() {
                    entity env  = application_scene->get_active_environment_data().active_environment_entity;
                    bool active = env == e;

                    bool changed  = false;
                    bool load_new = false;
                    changed |= image_load("Environment Image", environment_comp->hdr_texture ? environment_comp->hdr_texture->get_name() : -1, glm::vec2(128, 64), load_new);
                    if (load_new)
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
                    }
                    else if (changed)
                    {
                        environment_comp->hdr_texture = nullptr;
                        active                        = false;
                    }
                    if (environment_comp->hdr_texture)
                    {
                        ImGui::Separator();
                        changed |= checkbox("Set Active", &active, false);
                        ImGui::Separator();
                        slider_float_n("Intensity", &environment_comp->intensity, 1, mango::default_environment_intensity, 0.0f, 50000.0f, "%.1f", false);
                    }
                    if ((changed || env != e) && active)
                        application_scene->set_active_environment(e);
                    else if (env == e && changed && !active)
                        application_scene->set_active_environment(invalid_entity);
                },
                [e, &application_scene]() {
                    if (ImGui::Selectable("Remove"))
                    {
                        if (application_scene->get_active_environment_data().active_environment_entity == e)
                            application_scene->set_active_environment(invalid_entity);
                        application_scene->remove_environment_component(e);
                    }
                });

            // Light
            details::draw_component<mango::light_component>(
                e, light_comp,
                [e, &application_scene, &light_comp]() {
                    light_type current      = light_comp->type_of_light;
                    const char* possible[1] = { "Directional Light" };
                    int32 idx               = static_cast<int32>(current);
                    combo("Light Type", possible, 1, idx, 0);
                    current = static_cast<light_type>(idx);

                    ImGui::Separator();

                    if (current == light_type::directional) // Always true atm.
                    {
                        auto d_data = static_cast<directional_light_data*>(light_comp->data.get());

                        drag_float_n("Direction", &d_data->direction[0], 3, 1.0f, 0.08f, 0.0f, 0.0f, "%.2f", true);

                        color_edit("Color", &d_data->light_color[0], 3, 1.0f);

                        slider_float_n("Intensity", &d_data->intensity, 1, mango::default_directional_intensity, 0.0f, 500000.0f, "%.1f", false);

                        checkbox("Cast Shadows", &d_data->cast_shadows, false);
                    }
                },
                [e, &application_scene]() {
                    if (ImGui::Selectable("Remove"))
                    {
                        application_scene->remove_light_component(e);
                    }
                });

            ImGui::PopStyleVar();
            ImGui::PopID();
        }
        ImGui::End();
    } // namespace mango

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
