//! \file      imgui_widgets.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_IMGUI_WIDGETS_HPP
#define MANGO_IMGUI_WIDGETS_HPP

//! \cond NO_COND
#define GLM_FORCE_SILENT_WARNINGS 1
//! \endcond
#include "tinyfiledialogs.h"
#include <core/context_impl.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <mango/imgui_helper.hpp>
#include <mango/mesh_factory.hpp>
#include <scene/scene_impl.hpp>
#include <ui/dear_imgui/icons_font_awesome_5.hpp>
#include <ui/dear_imgui/imgui_glfw.hpp>

namespace mango
{
    //! \brief This is an imgui widget drawing the render view and the frame produced by the renderer.
    //! \param[in] renderer_backbuffer The native handle of the renderer backbuffer to draw in the render view.
    //! \param[in] enabled Specifies if window is rendered or not and can be set by imgui.
    //! \return The size of the viewport.
    ImVec2 render_view_widget(void* renderer_backbuffer, bool& enabled)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
        ImGui::Begin("Render View", &enabled);
        bool bar = ImGui::IsItemHovered() || ImGui::IsItemFocused();

        ImGui_ImplGlfw_FrameHovered(!bar && ImGui::IsWindowHovered());
        ImGui_ImplGlfw_FrameFocused(!bar && ImGui::IsWindowFocused());

        ImVec2 position = ImGui::GetCursorScreenPos();
        ImVec2 size     = ImGui::GetWindowSize();

        if (renderer_backbuffer)
            ImGui::GetWindowDrawList()->AddImage(renderer_backbuffer, position, ImVec2(position.x + size.x, position.y + size.y), ImVec2(0, 1), ImVec2(1, 0));
        ImGui::PopStyleVar();
        ImGui::End();
        return size;
    }

    //! \brief This is an imgui widget drawing some info of the framework.
    //! \param[in] shared_context The shared context.
    //! \param[in] enabled Specifies if window is rendered or not and can be set by imgui.
    void graphics_info_widget(const shared_ptr<context_impl>& shared_context, bool& enabled)
    {
        ImGui::Begin("Hardware Info", &enabled);
        const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding;
        if (ImGui::CollapsingHeader("Editor Stats", flags))
        {
            float frametime = shared_context->get_application()->frame_time();
            custom_info("Frame Time:",
                        [frametime]()
                        {
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
            float max = shared_context->get_renderer()->is_vsync_enabled() ? 75.0f : 650.0f;
            custom_info("Frame Rate:",
                        [ft, idx_, max]()
                        {
                            ImGui::AlignTextToFramePadding();
                            ImGui::Text("%.2f fps", ft[idx_]);
                            ImGui::PlotLines("", ft, 60, 0, "", 0.0f, max, ImVec2(0, 64));
                        });
        }
        auto& info = shared_context->get_renderer()->get_renderer_info();
        if (ImGui::CollapsingHeader("Renderer Info", flags))
        {
            column_split("split", 2, ImGui::GetContentRegionAvail().x * 0.33f);

            text_wrapped("API Version:");
            column_next();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%s", info.api_version.c_str());
            column_next();
            ImGui::SeparatorEx(ImGuiSeparatorFlags_SpanAllColumns | ImGuiSeparatorFlags_Horizontal);
            text_wrapped("Rendered Meshes:");
            column_next();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%d", info.last_frame.meshes);
            column_next();
            ImGui::SeparatorEx(ImGuiSeparatorFlags_SpanAllColumns | ImGuiSeparatorFlags_Horizontal);
            text_wrapped("Draw Calls:");
            column_next();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%d", info.last_frame.draw_calls);
            column_next();
            ImGui::SeparatorEx(ImGuiSeparatorFlags_SpanAllColumns | ImGuiSeparatorFlags_Horizontal);
            text_wrapped("Rendered Primitives:");
            column_next();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%d", info.last_frame.primitives);
            column_next();
            ImGui::SeparatorEx(ImGuiSeparatorFlags_SpanAllColumns | ImGuiSeparatorFlags_Horizontal);
            text_wrapped("Rendered Vertices:");
            column_next();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%d", info.last_frame.vertices);
            column_next();
            ImGui::SeparatorEx(ImGuiSeparatorFlags_SpanAllColumns | ImGuiSeparatorFlags_Horizontal);
            text_wrapped("Rendered Triangles (WIP - NOT CORRECT):");
            column_next();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%d", info.last_frame.triangles);
            column_next();
            ImGui::SeparatorEx(ImGuiSeparatorFlags_SpanAllColumns | ImGuiSeparatorFlags_Horizontal);
            text_wrapped("Rendered Materials: (WIP - NOT CORRECT)");
            column_next();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%d", info.last_frame.materials);
            column_next();
            ImGui::SeparatorEx(ImGuiSeparatorFlags_SpanAllColumns | ImGuiSeparatorFlags_Horizontal);
            text_wrapped("Canvas Size:");
            column_next();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("(%d x %d) px", info.canvas.width, info.canvas.height);

            column_merge();
        }
        ImGui::End();
    }

    //! \brief Draws the render system widget for a given \a renderer_impl.
    //! \param[in] rs The \a renderer_impl.
    //! \param[in,out] enabled True if the window is open, else false.
    void renderer_widget(const unique_ptr<renderer_impl>& rs, bool& enabled)
    {
        // Because every pipeline could have different properties, we will have to delegate that.
        ImGui::Begin("Renderer", &enabled);
        if (enabled)
            rs->on_ui_widget();
        ImGui::End();
    }

    namespace details
    {
        //! \brief Draws ui for a component.
        //! \param[in] comp_name The display name of the component.
        //! \param[in] component_draw_function A callback function for the main ui of the component.
        //! \param[in] additional Callback for additional ui use in an options button.
        void draw_component(const std::string& comp_name, std::function<void()> component_draw_function, std::function<bool()> additional = nullptr)
        {
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
                    open &= additional();
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

        //! \brief Opens a file dialog to load any kind of image and returns a texture_ptr after loading it to a \a texture.
        //! \param[in] application_scene The current \a scene of the \a application.
        //! \param[in] standard_color_space True if texture image is srgb, else false.
        //! \param[in] high_dynamic_range True if texture image is hdr, else false.
        //! \param[in] filter A list of file extensions to filter them.
        //! \param[in] num_filters The number of elements in filter.
        //! \return The texture_ptr to the created \a texture
        sid load_texture_dialog(const unique_ptr<scene_impl>& application_scene, bool standard_color_space, bool high_dynamic_range, const char* const* filter, int32 num_filters)
        {
            char* query_path = tinyfd_openFileDialog("", "res/", num_filters, filter, NULL, 0);
            if (query_path)
            {
                string queried = string(query_path);

                return application_scene->load_texture_from_image(queried, standard_color_space, high_dynamic_range);
            }
            return invalid_sid;
        }

        //! \brief Draws ui for a given \a scene_node.
        //! \param[in] node The \a scene_node to draw ui for.
        //! \param[in] application_scene The current \a scene of the \a application.
        void inspect_node(scene_node& node, const unique_ptr<scene_impl>& application_scene)
        {
            std::array<char, 32> tmp_string; // TODO Paul: Max length? 32 enough for now?
            auto icon = string(ICON_FA_DOT_CIRCLE);
            ImGui::AlignTextToFramePadding();
            ImGui::Text(icon.c_str());
            ImGui::SameLine();
            strcpy(tmp_string.data(), node.public_data.name.c_str()); // TODO Paul: Kind of fishy.
            ImGui::InputTextWithHint("##tag", "Enter Node Name", tmp_string.data(), 32);
            node.public_data.name = tmp_string.data();
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_PLUS_CIRCLE, ImVec2(-1, 0)))
            {
                ImGui::OpenPopup("##component_addition_popup");
            }

            ImGui::Spacing();
            bool has_camera              = node.camera_id != invalid_sid;
            bool has_orthographic_camera = has_camera;
            bool has_perspective_camera  = has_camera;
            bool has_directional_light   = node.light_ids[static_cast<uint8>(light_type::directional)] != invalid_sid;
            bool has_skylight            = node.light_ids[static_cast<uint8>(light_type::skylight)] != invalid_sid;
            bool has_atmospheric_light   = node.light_ids[static_cast<uint8>(light_type::atmospheric)] != invalid_sid;

            if (ImGui::BeginPopup("##component_addition_popup"))
            {
                if (!has_perspective_camera && ImGui::Selectable("Add Perspective Camera"))
                {
                    auto pc = perspective_camera();
                    application_scene->add_perspective_camera(pc, node.public_data.instance_id);
                }
                if (!has_orthographic_camera && ImGui::Selectable("Add Orthographic Camera"))
                {
                    auto oc = orthographic_camera();
                    application_scene->add_orthographic_camera(oc, node.public_data.instance_id);
                }
                if (!has_directional_light && ImGui::Selectable("Add Directional Light"))
                {
                    auto dl = directional_light();
                    application_scene->add_directional_light(dl, node.public_data.instance_id);
                }
                if (!has_skylight && ImGui::Selectable("Add Skylight"))
                {
                    auto sl = skylight();
                    application_scene->add_skylight(sl, node.public_data.instance_id);
                }
                if (!has_atmospheric_light && ImGui::Selectable("Add Atmospheric Light"))
                {
                    auto al = atmospheric_light();
                    application_scene->add_atmospheric_light(al, node.public_data.instance_id);
                }

                ImGui::EndPopup();
            }
        }

        //! \brief Draws ui for a given \a scene_light.
        //! \param[in] object The \a sid of the \a scene_light to draw ui for.
        //! \param[in] application_scene The current \a scene of the \a application.
        void inspect_light(sid object, const unique_ptr<scene_impl>& application_scene)
        {
            optional<scene_light&> l = application_scene->get_scene_light(object);
            MANGO_ASSERT(l, "Light to inspect does not exist!");
            details::draw_component(
                "Light",
                [object, &application_scene, &l]()
                {
                    bool type_changed    = false;
                    const char* types[3] = { "Directional", "Skylight", "Atmospheric" };
                    int32 idx            = static_cast<int32>(l->type);
                    combo("Light Type", types, 3, idx, 0);
                    if (l->type != static_cast<light_type>(idx))
                    {
                        l->type      = static_cast<light_type>(idx);
                        type_changed = true;
                    }
                    ImGui::Separator();
                    if (l->type == light_type::directional)
                    {
                        if (type_changed)
                            l->public_data_as_directional_light = directional_light();
                        float default_fl3[3] = { 1.0f, 1.0f, 1.0f };
                        drag_float_n("Direction", &l->public_data_as_directional_light->direction[0], 3, default_fl3, 0.08f, 0.0f, 0.0f, "%.2f", true);

                        color_edit("Color", &l->public_data_as_directional_light->color[0], 3, default_fl3);

                        float default_value[1] = { mango::default_directional_intensity };
                        slider_float_n("Intensity", &l->public_data_as_directional_light->intensity, 1, default_value, 0.0f, 500000.0f, "%.1f", false);

                        checkbox("Cast Shadows", &l->public_data_as_directional_light->cast_shadows, false);

                        checkbox("Contribute To Atmosphere", &l->public_data_as_directional_light->contribute_to_atmosphere, false);
                    }
                    else if (l->type == light_type::skylight)
                    {
                        if (type_changed)
                            l->public_data_as_skylight = skylight();
                        // TODO Paul: NEXT
                        checkbox("Use HDR Texture", &l->public_data_as_skylight->use_texture, false);
                        if (l->public_data_as_skylight->use_texture) // hdr texture
                        {
                            if (l->public_data_as_skylight->hdr_texture == invalid_sid)
                            {
                                char const* filter[1]                   = { "*.hdr" };
                                l->public_data_as_skylight->hdr_texture = details::load_texture_dialog(application_scene, false, true, filter, 1);
                            }
                            else
                            {
                                ImGui::PushID("hdr_texture");
                                bool load_new                = false;
                                optional<scene_texture&> hdr = application_scene->get_scene_texture(l->public_data_as_skylight->hdr_texture);
                                MANGO_ASSERT(hdr, "Hdr texture does not exist!");
                                image_load("Hdr Image", hdr->graphics_texture->native_handle(), vec2(128, 64), load_new);
                                ImGui::Separator();
                                if (load_new)
                                {
                                    // TODO Paul: We should remove the old one should we?
                                    char const* filter[1]                   = { "*.hdr" };
                                    l->public_data_as_skylight->hdr_texture = details::load_texture_dialog(application_scene, false, true, filter, 1);
                                }

                                if (l->public_data_as_skylight->hdr_texture == invalid_sid)
                                    l->public_data_as_skylight->use_texture = false;

                                ImGui::PopID();
                            }
                            float default_value[1] = { mango::default_skylight_intensity };
                            slider_float_n("Skylight Intensity", &l->public_data_as_skylight->intensity, 1, default_value, 0.0f, 50000.0f, "%.1f", false);
                        }
                    }
                    else
                    {
                        if (type_changed)
                            l->public_data_as_atmospheric_light = atmospheric_light();
                        ImGui::Text("Not required yet!");
                        // float default_fl3[3] = { 1.0f, 1.0f, 1.0f };
                        // ImGui::PushID("atmosphere");
                        // int32 default_ivalue[1] = { 32 };
                        // changed |= slider_int_n("Scatter Points", &el_data->scatter_points, 1, default_ivalue, 1, 64);
                        // default_ivalue[0] = 8;
                        // changed |= slider_int_n("Scatter Points Second Ray", &el_data->scatter_points_second_ray, 1, default_ivalue, 1, 32);
                        // float default_fl3[3]              = { 5.8f, 13.5f, 33.1f };
                        // vec3 coefficients_normalized = el_data->rayleigh_scattering_coefficients * 1e6f;
                        // changed |= drag_float_n("Rayleigh Scattering Coefficients (entity-6)", &coefficients_normalized.x, 3, default_fl3);
                        // el_data->rayleigh_scattering_coefficients = coefficients_normalized * 1e-6f;
                        // default_value[0]                          = 21.0f;
                        // float coefficient_normalized              = el_data->mie_scattering_coefficient * 1e6f;
                        // changed |= drag_float_n("Mie Scattering Coefficients (entity-6)", &coefficient_normalized, 1, default_value);
                        // el_data->mie_scattering_coefficient = coefficient_normalized * 1e-6f;
                        // float default_fl2[2]                = { 8e3f, 1.2e3f };
                        // changed |= drag_float_n("Density Multipler", &el_data->density_multiplier.x, 2, default_fl2);
                        // default_value[0] = 0.758f;
                        // changed |= slider_float_n("Preferred Mie Scattering Direction", &el_data->mie_preferred_scattering_dir, 1, default_value, 0.0f, 1.0f);
                        // default_value[0] = 6360e3f;
                        // changed |= drag_float_n("Ground Height", &el_data->ground_radius, 1, default_value);
                        // default_value[0] = 6420e3f;
                        // changed |= drag_float_n("Atmosphere Height", &el_data->atmosphere_radius, 1, default_value);
                        // default_value[0] = 1e3f;
                        // changed |= drag_float_n("View Height", &el_data->view_height, 1, default_value);
                        //
                        // default_fl3[0] = 1.0f;
                        // default_fl3[1] = 1.0f;
                        // default_fl3[2] = 1.0f;
                        // changed |= drag_float_n("Sun Direction", &el_data->sun_data.direction[0], 3, default_fl3, 0.08f, 0.0f, 0.0f, "%.2f", true);
                        // default_value[0] = mango::default_directional_intensity;
                        // changed |= slider_float_n("Sun Intensity", &el_data->sun_data.intensity, 1, default_value, 0.0f, 500000.0f, "%.1f", false);
                        // changed |= checkbox("Draw Sun Disc (Always On ATM)", &el_data->draw_sun_disc, false);
                    }
                },

                [object, &l, &application_scene]()
                {
                    if (ImGui::Selectable("Remove"))
                    {
                        if (l->type == light_type::directional)
                            application_scene->remove_directional_light(l->public_data_as_directional_light->containing_node);
                        else if (l->type == light_type::skylight)
                            application_scene->remove_skylight(l->public_data_as_skylight->containing_node);
                        else
                            application_scene->remove_atmospheric_light(l->public_data_as_atmospheric_light->containing_node);
                        return false;
                    }
                    return true;
                });
            ;
        }

        //! \brief Draws ui for a given \a scene_mesh.
        //! \param[in] object The \a sid of the \a scene_mesh to draw ui for.
        //! \param[in] application_scene The current \a scene of the \a application.
        //! \param[in,out] selected_primitive The \a sid of the currently selected \a primitive.
        void inspect_mesh(sid object, const unique_ptr<scene_impl>& application_scene, sid& selected_primitive)
        {
            optional<scene_mesh&> m = application_scene->get_scene_mesh(object);
            MANGO_ASSERT(m, "Mesh to inspect does not exist!");
            details::draw_component(
                "Mesh",
                [object, &application_scene, &m, &selected_primitive]()
                {
                    custom_info("Name: ",
                                [&m]()
                                {
                                    ImGui::AlignTextToFramePadding();
                                    ImGui::Text(m->public_data.name.c_str());
                                });
                    ImGui::Spacing();

                    // Could be done with tables when they support clicking.
                    ImGui::Text("Primitives:");
                    ImGui::Spacing();
                    for (int32 p = 0; p < static_cast<int32>(m->scene_primitives.size()); p++)
                    {
                        optional<scene_material&> mat = application_scene->get_scene_material(object);
                        MANGO_ASSERT(mat, "Material referenced by primitive does not exist!");
                        string selectable = "Primitive " + std::to_string(p) + " - Material: " + mat->public_data.name;
                        bool selected     = selected_primitive == m->scene_primitives[p].public_data.instance_id;
                        if (ImGui::Selectable(selectable.c_str(), &selected))
                        {
                            selected_primitive = m->scene_primitives[p].public_data.instance_id;
                        }
                    }
                },
                [object, &m, &application_scene]()
                {
                    if (ImGui::Selectable("Remove"))
                    {
                        application_scene->remove_mesh(m->public_data.containing_node);
                    }
                    return true;
                });
        }

        //! \brief Draws ui for a given \a scene_camera.
        //! \param[in] object The \a sid of the \a scene_camera to draw ui for.
        //! \param[in] application_scene The current \a scene of the \a application.
        //! \param[in] viewport_size The current viewport size.
        void inspect_camera(sid object, const unique_ptr<scene_impl>& application_scene, const ImVec2& viewport_size)
        {
            optional<scene_camera&> cam = application_scene->get_scene_camera(object);
            MANGO_ASSERT(cam, "Camera to inspect does not exist!");
            details::draw_component(
                "Camera",
                [object, &cam, &application_scene, &viewport_size]()
                {
                    sid cam_id   = application_scene->get_active_camera_sid();
                    bool active  = cam_id == object;
                    bool changed = checkbox("Active", &active, false);

                    if ((changed || cam_id != object) && active)
                    {
                        if (cam->type == camera_type::perspective)
                            application_scene->set_main_camera(cam->public_data_as_perspective->containing_node);
                        else
                            application_scene->set_main_camera(cam->public_data_as_orthographic->containing_node);
                    }
                    else if (cam_id == object && changed && !active)
                        application_scene->set_main_camera(invalid_sid);

                    ImGui::Separator();
                    bool type_changed    = false;
                    const char* types[2] = { "Perspective", "Orthographic" };
                    int32 idx            = static_cast<int32>(cam->type);
                    combo("Camera Type", types, 2, idx, 0);
                    if (cam->type != static_cast<camera_type>(idx))
                    {
                        cam->type    = static_cast<camera_type>(idx);
                        type_changed = true;
                    }
                    ImGui::Separator();

                    if (cam->type == camera_type::perspective)
                    {
                        if (type_changed)
                            cam->public_data_as_perspective = perspective_camera();
                        float default_value[1] = { 0.4f };
                        slider_float_n("Near Plane", &cam->public_data_as_perspective->z_near, 1, default_value, 0.0f, cam->public_data_as_perspective->z_far);
                        default_value[0] = 40.0f;
                        slider_float_n("Far Plane", &cam->public_data_as_perspective->z_far, 1, default_value, cam->public_data_as_perspective->z_near, 10000.0f);
                        float degree_fov = glm::degrees(cam->public_data_as_perspective->vertical_field_of_view);
                        default_value[0] = 45.0f;
                        slider_float_n("Vertical FOV", &degree_fov, 1, default_value, 1.75f, 175.0f, "%.1f°");
                        cam->public_data_as_perspective->vertical_field_of_view = glm::radians(degree_fov);
                        custom_aligned("Aspect",
                                       [&cam, &viewport_size](bool reset)
                                       {
                                           ImGui::AlignTextToFramePadding();
                                           ImGui::Text((std::to_string(cam->public_data_as_perspective->aspect) + " ").c_str());
                                           ImGui::SameLine();
                                           if (ImGui::Button("Aspect To Viewport", ImVec2(-1, 0)))
                                           {
                                               cam->public_data_as_perspective->aspect = viewport_size.x / viewport_size.y;
                                               return true;
                                           }
                                           if (reset)
                                           {
                                               cam->public_data_as_perspective->aspect = 16.0f / 9.0f;
                                               return true;
                                           }
                                           return false;
                                       });
                        ImGui::Separator();
                        float default_fl3[3] = { 0.0f, 0.0f, 0.0f };
                        drag_float_n("Target", &cam->public_data_as_perspective->target.x, 3, default_fl3, 0.1f, 0.0, 0.0, "%.1f", true);

                        ImGui::Separator();
                        checkbox("Adaptive Exposure", &cam->public_data_as_perspective->adaptive_exposure, false);

                        ImGui::BeginGroup();
                        if (cam->public_data_as_perspective->adaptive_exposure)
                        {
                            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                        }

                        default_value[0] = mango::default_camera_aperture;
                        drag_float_n("Aperture", &cam->public_data_as_perspective->physical.aperture, 1, default_value, 0.1f, mango::min_camera_aperture, mango::max_camera_aperture, "%.1f");
                        default_value[0] = mango::default_camera_shutter_speed;
                        drag_float_n("Shutter Speed", &cam->public_data_as_perspective->physical.shutter_speed, 1, default_value, 0.0001f, mango::min_camera_shutter_speed,
                                     mango::max_camera_shutter_speed, "%.5f");
                        default_value[0] = mango::default_camera_iso;
                        drag_float_n("Iso", &cam->public_data_as_perspective->physical.iso, 1, default_value, 0.1f, mango::min_camera_iso, mango::max_camera_iso, "%.1f");

                        ImGui::EndGroup();
                        if (cam->public_data_as_perspective->adaptive_exposure)
                        {
                            ImGui::PopItemFlag();
                            ImGui::PopStyleVar();
                            if (ImGui::IsItemHovered())
                                ImGui::SetTooltip("Adaptive Exposure Controlled");
                        }
                    }
                    else // orthographic
                    {
                        if (type_changed)
                            cam->public_data_as_orthographic = orthographic_camera();
                        float default_value[1] = { 0.4f };
                        slider_float_n("Near Plane", &cam->public_data_as_orthographic->z_near, 1, default_value, 0.0f, cam->public_data_as_orthographic->z_far);
                        default_value[0] = 40.0f;
                        slider_float_n("Far Plane", &cam->public_data_as_orthographic->z_far, 1, default_value, cam->public_data_as_orthographic->z_near, 10000.0f);
                        default_value[0] = 1.0f;
                        slider_float_n("Magnification X", &cam->public_data_as_orthographic->x_mag, 1, default_value, 0.1f, 100.0f, "%.1f");
                        slider_float_n("Magnification Y", &cam->public_data_as_orthographic->y_mag, 1, default_value, 0.1f, 100.0f, "%.1f");
                        custom_aligned("Magnification",
                                       [&cam, &viewport_size](bool reset)
                                       {
                                           if (ImGui::Button("Magnification To Viewport", ImVec2(-1, 0)))
                                           {
                                               cam->public_data_as_orthographic->x_mag = viewport_size.x / viewport_size.y;
                                               cam->public_data_as_orthographic->y_mag = 1.0f;
                                               return true;
                                           }
                                           if (reset)
                                           {
                                               cam->public_data_as_orthographic->x_mag = 1.0f;
                                               cam->public_data_as_orthographic->y_mag = 1.0f;
                                               return true;
                                           }
                                           return false;
                                       });
                        ImGui::Separator();
                        float default_fl3[3] = { 0.0f, 0.0f, 0.0f };
                        drag_float_n("Target", &cam->public_data_as_orthographic->target.x, 3, default_fl3, 0.1f, 0.0, 0.0, "%.1f", true);

                        ImGui::Separator();
                        checkbox("Adaptive Exposure", &cam->public_data_as_orthographic->adaptive_exposure, false);

                        ImGui::BeginGroup();
                        if (cam->public_data_as_orthographic->adaptive_exposure)
                        {
                            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                        }

                        default_value[0] = mango::default_camera_aperture;
                        drag_float_n("Aperture", &cam->public_data_as_orthographic->physical.aperture, 1, default_value, 0.1f, mango::min_camera_aperture, mango::max_camera_aperture, "%.1f");
                        default_value[0] = mango::default_camera_shutter_speed;
                        drag_float_n("Shutter Speed", &cam->public_data_as_orthographic->physical.shutter_speed, 1, default_value, 0.0001f, mango::min_camera_shutter_speed,
                                     mango::max_camera_shutter_speed, "%.5f");
                        default_value[0] = mango::default_camera_iso;
                        drag_float_n("Iso", &cam->public_data_as_orthographic->physical.iso, 1, default_value, 0.1f, mango::min_camera_iso, mango::max_camera_iso, "%.1f");

                        ImGui::EndGroup();
                        if (cam->public_data_as_orthographic->adaptive_exposure)
                        {
                            ImGui::PopItemFlag();
                            ImGui::PopStyleVar();
                            if (ImGui::IsItemHovered())
                                ImGui::SetTooltip("Adaptive Exposure Controlled");
                        }
                    }
                },
                [object, &cam, &application_scene]()
                {
                    if (ImGui::Selectable("Remove"))
                    {
                        if (cam->type == camera_type::perspective)
                            application_scene->remove_perspective_camera(cam->public_data_as_perspective->containing_node);
                        else
                            application_scene->remove_orthographic_camera(cam->public_data_as_orthographic->containing_node);
                        return false;
                    }
                    return true;
                });
        }

        //! \brief Draws ui for a given \a scene_transform.
        //! \param[in] object The \a sid of the \a scene_transform to draw ui for.
        //! \param[in] application_scene The current \a scene of the \a application.
        //! \param[in] is_camera True if the \a scene_node containing this transform also contains a \a scene_camera, else false.
        //! \param[in] is_light True if the \a scene_node containing this transform also contains a \a scene_light, else false.
        void inspect_transform(sid object, const unique_ptr<scene_impl>& application_scene, bool is_camera, bool is_light)
        {
            optional<scene_transform&> tr = application_scene->get_scene_transform(object);
            MANGO_ASSERT(tr, "Transform to inspect does not exist!");
            details::draw_component("Transform",
                                    [object, &tr, is_camera, is_light]()
                                    {
                                        ImGui::BeginGroup();

                                        float default_value[3] = { 0.0f, 0.0f, 0.0f };

                                        if (is_light)
                                        {
                                            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                                            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                                        }

                                        bool changed = false;

                                        // translation
                                        changed |= drag_float_n("Translation", &tr->public_data.position[0], 3, default_value, 0.08f, 0.0f, 0.0f, "%.2f", true);

                                        if (is_camera && !is_light)
                                        {
                                            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                                            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                                        }

                                        // rotation
                                        vec3 rotation_hint_before = tr->rotation_hint;
                                        changed |= drag_float_n("Rotation", &tr->rotation_hint[0], 3, default_value, 0.08f, 0.0f, 0.0f, "%.2f", true);

                                        default_value[0] = 1.0f;
                                        default_value[1] = 1.0f;
                                        default_value[2] = 1.0f;
                                        // scale
                                        changed |= drag_float_n("Scale", &tr->public_data.scale[0], 3, default_value, 0.08f, 0.0f, 0.0f, "%.2f", true);

                                        if (changed)
                                        {
                                            quat x_quat         = glm::angleAxis(glm::radians(tr->rotation_hint.x - rotation_hint_before.x), vec3(1.0f, 0.0f, 0.0f));
                                            quat y_quat         = glm::angleAxis(glm::radians(tr->rotation_hint.y - rotation_hint_before.y), vec3(0.0f, 1.0f, 0.0f));
                                            quat z_quat         = glm::angleAxis(glm::radians(tr->rotation_hint.z - rotation_hint_before.z), vec3(0.0f, 0.0f, 1.0f));
                                            tr->public_data.rotation = x_quat * y_quat * z_quat * tr->public_data.rotation;
                                            tr->public_data.update();
                                        }

                                        ImGui::EndGroup();
                                        if (is_camera || is_light)
                                        {
                                            ImGui::PopItemFlag();
                                            ImGui::PopStyleVar();
                                            if (ImGui::IsItemHovered()) // TODO Paul: This does not seem to work correctly!
                                                ImGui::SetTooltip("Disabled For %s", is_camera ? "Cameras" : "Lights");
                                        }
                                    });
        }

        //! \brief Draws ui for a given \a scene_model.
        //! \param[in] object The \a sid of the \a scene_model to draw ui for.
        //! \param[in] application_scene The current \a scene of the \a application.
        void inspect_model(sid object, const unique_ptr<scene_impl>& application_scene)
        {
            optional<scene_model&> m = application_scene->get_scene_model(object);
            MANGO_ASSERT(m, "Model to inspect does not exist!");
            details::draw_component("Model",
                                    [object, &m]()
                                    {
                                        custom_info("Model Path: ",
                                                    [&m]()
                                                    {
                                                        ImGui::AlignTextToFramePadding();
                                                        ImGui::Text(m->public_data.file_path.c_str());
                                                    });
                                    });
        }

        //! \brief Draws ui for a given \a scene_primitive.
        //! \param[in] object The \a sid of the \a scene_primitive to draw ui for.
        //! \param[in] application_scene The current \a scene of the \a application.
        void inspect_primitive(sid object, const unique_ptr<scene_impl>& application_scene)
        {
            optional<scene_primitive&> prim = application_scene->get_scene_primitive(object);
            MANGO_ASSERT(prim, "Primitive to inspect does not exist!");
            details::draw_component("Primitive",
                                    [&prim]()
                                    {
                                        custom_info("Vertex Normals: ",
                                                    [&prim]()
                                                    {
                                                        ImGui::AlignTextToFramePadding();
                                                        ImGui::Text(prim->public_data.has_normals ? ICON_FA_CHECK : ICON_FA_TIMES);
                                                    });
                                        custom_info("Vertex Tangents: ",
                                                    [&prim]()
                                                    {
                                                        ImGui::AlignTextToFramePadding();
                                                        ImGui::Text(prim->public_data.has_tangents ? ICON_FA_CHECK : ICON_FA_TIMES);
                                                    });
                                    });
        }

        //! \brief Draws ui for a given \a scene_material.
        //! \param[in] object The \a sid of the \a scene_material to draw ui for.
        //! \param[in] application_scene The current \a scene of the \a application.
        void inspect_material(sid object, const unique_ptr<scene_impl>& application_scene)
        {
            optional<scene_material&> mat = application_scene->get_scene_material(object);
            MANGO_ASSERT(mat, "Material to inspect does not exist!");
            details::draw_component("Material",
                                    [&mat, &application_scene]()
                                    {
                                        char const* filter[4] = { "*.png", "*.jpg", "*.jpeg", "*.bmp" };

                                        const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding;

                                        // base color

                                        if (ImGui::CollapsingHeader("Base Color", flags | ImGuiTreeNodeFlags_DefaultOpen))
                                        {
                                            ImGui::PushID("Base Color");
                                            bool changed  = false;
                                            bool load_new = false;

                                            if (mat->public_data.base_color_texture != invalid_sid)
                                            {
                                                optional<scene_texture&> base_color_texture = application_scene->get_scene_texture(mat->public_data.base_color_texture);
                                                changed |= image_load("Base Color Texture", base_color_texture->graphics_texture->native_handle(), vec2(64, 64), load_new);
                                            }
                                            else
                                                changed |= image_load("Base Color Texture", nullptr, vec2(64, 64), load_new);

                                            if (load_new)
                                                mat->public_data.base_color_texture = details::load_texture_dialog(application_scene, true, false, filter, 4);
                                            else if (changed)
                                                mat->public_data.base_color_texture = invalid_sid;

                                            ImGui::Separator();

                                            float default_value[3] = { 1.0f, 1.0f, 1.0f };
                                            if (mat->public_data.base_color_texture == invalid_sid)
                                                color_edit("Color", &mat->public_data.base_color[0], 4, default_value);

                                            ImGui::Separator();
                                            ImGui::PopID();
                                        }

                                        // roughness metallic

                                        if (ImGui::CollapsingHeader("Roughness And Metallic", flags))
                                        {
                                            ImGui::PushID("Roughness And Metallic");

                                            bool changed  = false;
                                            bool load_new = false;

                                            if (mat->public_data.metallic_roughness_texture != invalid_sid)
                                            {
                                                optional<scene_texture&> r_m_texture = application_scene->get_scene_texture(mat->public_data.metallic_roughness_texture);
                                                changed |= image_load("Roughness And Metallic Texture", r_m_texture->graphics_texture->native_handle(), vec2(64, 64), load_new);
                                            }
                                            else
                                                changed |= image_load("Roughness And Metallic Texture", nullptr, vec2(64, 64), load_new);

                                            if (load_new)
                                                mat->public_data.metallic_roughness_texture = details::load_texture_dialog(application_scene, false, false, filter, 4);
                                            else if (changed)
                                                mat->public_data.metallic_roughness_texture = invalid_sid;

                                            ImGui::Separator();

                                            if (mat->public_data.metallic_roughness_texture != invalid_sid)
                                            {
                                                checkbox("Has Packed AO", &mat->public_data.packed_occlusion, false);
                                            }
                                            else
                                            {
                                                float default_value = 0.5f;
                                                slider_float_n("Roughness", mat->public_data.roughness.type_data(), 1, &default_value, 0.0f, 1.0f);
                                                slider_float_n("Metallic", mat->public_data.metallic.type_data(), 1, &default_value, 0.0f, 1.0f);
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

                                            if (mat->public_data.normal_texture != invalid_sid)
                                            {
                                                optional<scene_texture&> normal_texture = application_scene->get_scene_texture(mat->public_data.normal_texture);
                                                changed |= image_load("Normal Texture", normal_texture->graphics_texture->native_handle(), vec2(64, 64), load_new);
                                            }
                                            else
                                                changed |= image_load("Normal Texture", nullptr, vec2(64, 64), load_new);

                                            if (load_new)
                                                mat->public_data.normal_texture = details::load_texture_dialog(application_scene, false, false, filter, 4);
                                            else if (changed)
                                                mat->public_data.normal_texture = invalid_sid;

                                            ImGui::Separator();
                                            ImGui::PopID();
                                        }

                                        // occlusion

                                        if (ImGui::CollapsingHeader("Occlusion Map", flags))
                                        {
                                            ImGui::PushID("Occlusion Map");

                                            bool changed  = false;
                                            bool load_new = false;

                                            if (mat->public_data.occlusion_texture != invalid_sid)
                                            {
                                                optional<scene_texture&> occlusion_texture = application_scene->get_scene_texture(mat->public_data.occlusion_texture);
                                                changed |= image_load("Occlusion Texture", occlusion_texture->graphics_texture->native_handle(), vec2(64, 64), load_new);
                                            }
                                            else
                                                changed |= image_load("Occlusion Texture", nullptr, vec2(64, 64), load_new);

                                            if (load_new)
                                                mat->public_data.occlusion_texture = details::load_texture_dialog(application_scene, false, false, filter, 4);
                                            else if (changed)
                                                mat->public_data.occlusion_texture = invalid_sid;

                                            ImGui::Separator();
                                            ImGui::PopID();
                                        }

                                        // emissive

                                        if (ImGui::CollapsingHeader("Emissive", flags))
                                        {
                                            ImGui::PushID("Emissive");

                                            bool changed  = false;
                                            bool load_new = false;

                                            if (mat->public_data.emissive_texture != invalid_sid)
                                            {
                                                optional<scene_texture&> emissive_texture = application_scene->get_scene_texture(mat->public_data.emissive_texture);
                                                changed |= image_load("Emissive Texture", emissive_texture->graphics_texture->native_handle(), vec2(64, 64), load_new);
                                            }
                                            else
                                                changed |= image_load("Emissive Texture", nullptr, vec2(64, 64), load_new);

                                            if (load_new)
                                                mat->public_data.emissive_texture = details::load_texture_dialog(application_scene, true, false, filter, 4);
                                            else if (changed)
                                                mat->public_data.emissive_texture = invalid_sid;

                                            ImGui::Separator();

                                            float default_value_float = mango::default_emissive_intensity;
                                            slider_float_n("Intensity", &mat->public_data.emissive_intensity, 1, &default_value_float, 0.0f,
                                                           default_emissive_intensity * 10.0f); // TODO Paul: Range?

                                            float default_value[3] = { 1.0f, 1.0f, 1.0f };
                                            if (mat->public_data.emissive_texture == invalid_sid)
                                            {
                                                color_edit("Color", &mat->public_data.emissive_color[0], 4, default_value);
                                            }
                                            ImGui::Separator();
                                            ImGui::PopID();
                                        }

                                        ImGui::Separator();
                                        ImGui::Spacing();

                                        checkbox("Double Sided", &mat->public_data.double_sided, false);

                                        ImGui::Separator();

                                        const char* types[4] = { "Opaque", "Masked", "Blended", "Dithered" };
                                        int32 idx            = static_cast<int32>(mat->public_data.alpha_mode);
                                        combo("Alpha Mode", types, 4, idx, 0);
                                        mat->public_data.alpha_mode = static_cast<material_alpha_mode>(idx);

                                        float default_value = 0.5f;
                                        if (mat->public_data.alpha_mode == material_alpha_mode::mode_mask)
                                            slider_float_n("Alpha CutOff", mat->public_data.alpha_cutoff.type_data(), 1, &default_value, 0.0f, 1.0f, "%.2f");
                                        if (mat->public_data.alpha_mode == material_alpha_mode::mode_blend)
                                            custom_info(
                                                "Blending With Basic Over Operator!", []() {}, 0.0f, ImGui::GetContentRegionAvail().x);
                                        if (mat->public_data.alpha_mode == material_alpha_mode::mode_dither)
                                            custom_info(
                                                "Dithering ... Just For Fun!", []() {}, 0.0f, ImGui::GetContentRegionAvail().x);
                                    });
        }
    } // namespace details

    //! \brief Draws a scene graph in the user interface.
    //! \param[in] application_scene The current \a scene of the \a application.
    //! \param[in,out] enabled True if the window is open, else false.
    //! \param[in,out] selected The sid of the currently selected entity, can be updated by this function.
    void scene_inspector_widget(const unique_ptr<scene_impl>& application_scene, bool& enabled, sid& selected)
    {
        ImGui::Begin("Scene Inspector", &enabled);
        if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered())
        {
            if (ImGui::IsMouseClicked(0))
                selected = invalid_sid;
            if (!ImGui::IsPopupOpen("##scene_menu") && ImGui::IsMouseClicked(1))
                ImGui::OpenPopup("##scene_menu");
        }
        if (ImGui::BeginPopup("##scene_menu"))
        {
            if (ImGui::Selectable("Add Entity##scene_menu"))
            {
                auto new_node = node();
                selected      = application_scene->add_node(new_node);
            }
            if (ImGui::Selectable("Import And Add Model (temporary)##scene_menu"))
            {
                char const* filter[2] = { "*.gltf", "*.glb" };

                char* query_path = tinyfd_openFileDialog("", "res/", 2, filter, NULL, 0);
                if (query_path)
                {
                    string queried = string(query_path);
                    auto ext       = queried.substr(queried.find_last_of(".") + 1);
                    if (ext == "glb" || ext == "gltf")
                    {
                        sid m                       = application_scene->load_model_from_gltf(queried);
                        optional<mango::model&> mod = application_scene->get_model(m);
                        MANGO_ASSERT(mod, "Model not existent!");
                        auto start              = string(queried).find_last_of("\\/") + 1;
                        auto name               = string(queried).substr(start, queried.find_last_of(".") - start);
                        node instance_root_node = node(name);
                        sid model_instance_root = application_scene->add_node(instance_root_node);
                        application_scene->add_model_to_scene(m, mod->scenarios.at(mod->default_scenario), model_instance_root);
                    }
                }
            }

            ImGui::EndPopup();
        }

        application_scene->draw_scene_hierarchy(selected);
        ImGui::End();
    }

    //! \brief Draws the entity component inspector for a given object in the user interface.
    //! \param[in] shared_context The shared context of mango.
    //! \param[in,out] enabled True if the window is open, else false.
    //! \param[in] object The object the components should be inspected.
    //! \param[in] viewport_size The size of the render_view, when enabled, else some base size.
    //! \param[in,out] selected_primitive The last selected primitive -> Should be updated by inspect_mesh().
    void entity_component_inspector_widget(const shared_ptr<context_impl>& shared_context, bool& enabled, sid object, const ImVec2& viewport_size, sid& selected_primitive)
    {
        ImGui::Begin("Entity Component Inspector", &enabled);
        if (object != invalid_sid)
        {
            auto& application_scene  = shared_context->get_internal_scene();
            optional<scene_node&> nd = application_scene->get_scene_node(object);
            MANGO_ASSERT(nd, "Node to inspect does not exist!");

            ImGui::PushID(object.id().get());
            ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 0.0f);

            if (object.structure_type() == scene_structure_type::scene_structure_node)
            {
                optional<scene_node&> node = application_scene->get_scene_node(object);
                MANGO_ASSERT(node, "Node to inspect does not exist!");
                details::inspect_node(node.value(), application_scene);
                bool is_camera = (node->type & node_type::camera) != node_type::empty_leaf; // Each node in the hierarchy has a transform.
                bool is_light  = (node->type & node_type::light) != node_type::empty_leaf;  // Each node in the hierarchy has a transform.
                details::inspect_transform(node->node_transform, application_scene, is_camera, is_light);
                if (is_light)
                {
                    for (int32 i = 0; i < 3; ++i)
                    {
                        if (node->light_ids[i] == invalid_sid)
                            continue;
                        details::inspect_light(node->light_ids[i], application_scene);
                    }
                }
                if ((node->type & node_type::mesh) != node_type::empty_leaf)
                {
                    details::inspect_mesh(node->mesh_id, application_scene, selected_primitive);
                }
                if (is_camera)
                {
                    details::inspect_camera(node->camera_id, application_scene, viewport_size);
                }
            }
            else
                MANGO_ASSERT(false, "Scene object which is not a node in the scene graph!");

            /*

            // Material
            details::draw_component<mango::material_component>(
                material_comp, [entity, &application_scene, &material_comp, &rs]() { details::draw_material(material_comp->component_material, rs); },
                [entity, &application_scene]() {
                    if (ImGui::Selectable("Remove"))
                    {
                        application_scene->remove_component<material_component>(entity);
                        return false;
                    }
                    return true;
                });
            */

            ImGui::PopStyleVar();
            ImGui::PopID();
        }
        ImGui::End();
    }

    //! \brief Draws the primitive - material inspector for a given primitive in the user interface.
    //! \param[in] shared_context The shared context of mango.
    //! \param[in,out] enabled True if the window is open, else false.
    //! \param[in] selected_primitive The selected primitive that should be inspected.
    void primitive_material_inspector_widget(const shared_ptr<context_impl>& shared_context, bool& enabled, sid selected_primitive)
    {
        ImGui::Begin("Primitive - Material Inspector", &enabled);
        if (selected_primitive != invalid_sid)
        {
            auto& application_scene         = shared_context->get_internal_scene();
            optional<scene_primitive&> prim = application_scene->get_scene_primitive(selected_primitive);
            MANGO_ASSERT(prim, "Primitive to inspect does not exist!");

            ImGui::PushID(selected_primitive.id().get());
            ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 0.0f);

            details::inspect_primitive(selected_primitive, application_scene);
            details::inspect_material(prim->public_data.material, application_scene);
            ImGui::PopStyleVar();
            ImGui::PopID();
        }
        ImGui::End();
    }

} // namespace mango

#endif // MANGO_IMGUI_WIDGETS_HPP
