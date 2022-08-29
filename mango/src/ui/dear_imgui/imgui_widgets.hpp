//! \file      imgui_widgets.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2022
//! \copyright Apache License 2.0

#ifndef MANGO_IMGUI_WIDGETS_HPP
#define MANGO_IMGUI_WIDGETS_HPP

#include "tinyfiledialogs.h"
#include <core/context_impl.hpp>
#include <mango/application.hpp>
#include <mango/imgui_helper.hpp>
#include <mango/mesh_factory.hpp>
#include <scene/scene_impl.hpp>
#include <ui/dear_imgui/icons_font_awesome_5.hpp>
#include <ui/dear_imgui/imgui_glfw.hpp>

namespace mango
{
    //! \brief Opens a file dialog to load any kind of image and returns a texture_ptr after loading it to a \a texture.
    //! \param[in] application_scene The current \a scene of the \a application.
    //! \param[in] standard_color_space True if texture image is srgb, else false.
    //! \param[in] high_dynamic_range True if texture image is hdr, else false.
    //! \param[in] filter A list of file extensions to filter them.
    //! \param[in] num_filters The number of elements in filter.
    //! \return The \a handle referencing the created \a texture and the \a key referencing the created \a texture_gpu_data.
    std::pair<handle<texture>, optional<key>> load_texture_dialog(const unique_ptr<scene_impl>& application_scene, bool standard_color_space, bool high_dynamic_range, char const* const* const filter,
                                                                  int32 num_filters);

    //! \brief This is an imgui widget drawing the render view and the frame produced by the renderer.
    //! \param[in] renderer_backbuffer The native handle of the renderer backbuffer to draw in the render view.
    //! \param[in] enabled Specifies if window is rendered or not and can be set by imgui.
    //! \return The size of the viewport.
    ImVec2 render_view_widget(void* renderer_backbuffer, bool& enabled);

    //! \brief This is an imgui widget drawing some info of the framework.
    //! \param[in] shared_context The shared context.
    //! \param[in] enabled Specifies if window is rendered or not and can be set by imgui.
    void graphics_info_widget(const shared_ptr<context_impl>& shared_context, bool& enabled);

    //! \brief Draws the render system widget for a given \a renderer_impl.
    //! \param[in] rs The \a renderer_impl.
    //! \param[in,out] enabled True if the window is open, else false.
    void renderer_widget(const unique_ptr<renderer_impl>& rs, bool& enabled);

    namespace details
    {
        //! \brief Draws ui for a component.
        //! \param[in] comp_name The display name of the component.
        //! \param[in] component_draw_function A callback function for the main ui of the component.
        //! \param[in] additional Callback for additional ui use in an options button.
        void draw_component(const std::string& comp_name, std::function<void()> component_draw_function, std::function<bool()> additional = nullptr);

        //! \brief Draws ui for a given \a node.
        //! \param[in] node_hnd The \a handle of the \a node.
        //! \param[in] node The \a node to draw ui for.
        //! \param[in] application_scene The current \a scene of the \a application.
        void inspect_node(handle<node> node_hnd, node& node, const unique_ptr<scene_impl>& application_scene);

        //! \brief Draws ui for a given \a directional_light.
        //! \param[in] node_hnd The \a handle of the \a node the \a directional_light is in.
        //! \param[in] application_scene The current \a scene of the \a application.
        void inspect_directional_light(handle<node> node_hnd, const unique_ptr<scene_impl>& application_scene);

        //! \brief Draws ui for a given \a skylight.
        //! \param[in] node_hnd The \a handle of the \a node the \a skylight is in.
        //! \param[in] application_scene The current \a scene of the \a application.
        void inspect_skylight(handle<node> node_hnd, const unique_ptr<scene_impl>& application_scene);

        //! \brief Draws ui for a given \a atmospheric_light.
        //! \param[in] node_hnd The \a handle of the \a node the \a atmospheric_light is in.
        //! \param[in] application_scene The current \a scene of the \a application.
        void inspect_atmospheric_light(handle<node> node_hnd, const unique_ptr<scene_impl>& application_scene);

        //! \brief Draws ui for a given \a mesh.
        //! \param[in] node_hnd The \a handle of the \a node the \a mesh is in.
        //! \param[in] instance The \a handle of the \a mesh instance.
        //! \param[in] application_scene The current \a scene of the \a application.
        //! \param[in,out] selected_primitive The \a handle of the currently selected \a primitive.
        void inspect_mesh(handle<node> node_hnd, handle<mesh> instance, const unique_ptr<scene_impl>& application_scene, handle<primitive>& selected_primitive);

        //! \brief Draws ui for a given \a perspective_camera.
        //! \param[in] node_hnd The \a handle of the \a node the \a perspective_camera is in.
        //! \param[in] application_scene The current \a scene of the \a application.
        //! \param[in] viewport_size The current viewport size.
        void inspect_perspective_camera(handle<node> node_hnd, const unique_ptr<scene_impl>& application_scene, const ImVec2& viewport_size);

        //! \brief Draws ui for a given \a orthographic_camera.
        //! \param[in] node_hnd The \a handle of the \a node the \a orthographic_camera is in.
        //! \param[in] application_scene The current \a scene of the \a application.
        //! \param[in] viewport_size The current viewport size.
        void inspect_orthographic_camera(handle<node> node_hnd, const unique_ptr<scene_impl>& application_scene, const ImVec2& viewport_size);

        //! \brief Draws ui for a given \a transform.
        //! \param[in] node_hnd The \a handle of the \a node the \a transform is for.
        //! \param[in] application_scene The current \a scene of the \a application.
        //! \param[in] is_camera True if the \a node containing this \a transform also contains a \a camera, else false.
        //! \param[in] is_light True if the \a node containing this \a transform also contains a \a light, else false.
        void inspect_transform(handle<node> node_hnd, const unique_ptr<scene_impl>& application_scene, bool is_camera, bool is_light);

        //! \brief Draws ui for a given \a model.
        //! \param[in] object The \a handle of the \a model to draw ui for.
        //! \param[in] application_scene The current \a scene of the \a application.
        void inspect_model(handle<model> object, const unique_ptr<scene_impl>& application_scene);

        //! \brief Draws ui for a given \a primitive.
        //! \param[in] object The \a handle of the \a primitive to draw ui for.
        //! \param[in] application_scene The current \a scene of the \a application.
        void inspect_primitive(handle<primitive> object, const unique_ptr<scene_impl>& application_scene);

        //! \brief Draws ui for a given \a material.
        //! \param[in] object The \a key of the \a material to draw ui for.
        //! \param[in] application_scene The current \a scene of the \a application.
        void inspect_material(handle<material> object, const unique_ptr<scene_impl>& application_scene);
    } // namespace details

    //! \brief Draws a scene graph in the user interface.
    //! \param[in] application_scene The current \a scene of the \a application.
    //! \param[in,out] enabled True if the window is open, else false.
    //! \param[in,out] selected The \a handle of the currently selected \a node, can be updated by this function.
    void scene_inspector_widget(const unique_ptr<scene_impl>& application_scene, bool& enabled, handle<node>& selected);

    //! \brief Draws the scene object component inspector for a given object in the user interface.
    //! \param[in] shared_context The shared context of mango.
    //! \param[in,out] enabled True if the window is open, else false.
    //! \param[in] node_hnd The \a handle of the \a node that should be inspected.
    //! \param[in] viewport_size The size of the render_view, when enabled, else some base size.
    //! \param[in,out] selected_primitive The last selected primitive -> Should be updated by inspect_mesh().
    void scene_object_component_inspector_widget(const shared_ptr<context_impl>& shared_context, bool& enabled, handle<node> node_hnd, const ImVec2& viewport_size,
                                                 handle<primitive>& selected_primitive);

    //! \brief Draws the primitive - material inspector for a given primitive in the user interface.
    //! \param[in] shared_context The shared context of mango.
    //! \param[in,out] enabled True if the window is open, else false.
    //! \param[in] selected_primitive The selected primitive that should be inspected.
    void primitive_material_inspector_widget(const shared_ptr<context_impl>& shared_context, bool& enabled, handle<primitive>& selected_primitive);
} // namespace mango

#endif // MANGO_IMGUI_WIDGETS_HPP
