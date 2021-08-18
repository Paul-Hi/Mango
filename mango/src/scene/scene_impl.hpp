//! \file      scene_impl.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_SCENE_IMPL_HPP
#define MANGO_SCENE_IMPL_HPP

#include <graphics/graphics.hpp>
#include <mango/packed_freelist.hpp>
#include <mango/scene.hpp>
#include <map>
#include <queue>
#include <scene/scene_internals.hpp>
#include <util/helpers.hpp>

namespace mango
{
    //! \brief Implementation of the \a scene.
    class scene_impl : public scene
    {
        MANGO_DISABLE_COPY_AND_ASSIGNMENT(scene_impl)
      public:
        //! \brief Constructs a new \a scene_impl with a \a name.
        //! \param[in] name The name of the new \a scene_impl.
        //! \param[in] context The internally shared context of mango.
        scene_impl(const string& name, const shared_ptr<context_impl>& context);
        ~scene_impl();

        sid add_node(node& new_node) override;
        sid add_perspective_camera(perspective_camera& new_perspective_camera, sid containing_node_id) override;
        sid add_orthographic_camera(orthographic_camera& new_orthographic_camera, sid containing_node_id) override;
        sid add_directional_light(directional_light& new_directional_light, sid containing_node_id) override;
        sid add_skylight(skylight& new_skylight, sid containing_node_id) override;
        sid add_atmospheric_light(atmospheric_light& new_atmospheric_light, sid containing_node_id) override;

        sid build_material(material& new_material) override;
        sid load_texture_from_image(const string& path, bool standard_color_space, bool high_dynamic_range) override;

        sid load_model_from_gltf(const string& path) override;
        sid add_model_to_scene(sid model_to_add, sid scenario_id, sid containing_node_id) override;

        sid add_skylight_from_hdr(const string& path, sid containing_node_id) override;

        void remove_node(sid node_id) override;
        void remove_perspective_camera(sid node_id) override;
        void remove_orthographic_camera(sid node_id) override;
        void remove_mesh(sid node_id) override;
        void remove_directional_light(sid node_id) override;
        void remove_skylight(sid node_id) override;
        void remove_atmospheric_light(sid node_id) override;

        optional<node&> get_node(sid node_id) override;
        optional<transform&> get_transform(sid node_id) override;
        optional<perspective_camera&> get_perspective_camera(sid node_id) override;
        optional<orthographic_camera&> get_orthographic_camera(sid node_id) override;
        optional<directional_light&> get_directional_light(sid node_id) override;
        optional<skylight&> get_skylight(sid node_id) override;
        optional<atmospheric_light&> get_atmospheric_light(sid node_id) override;

        optional<model&> get_model(sid instance_id) override;
        optional<mesh&> get_mesh(sid instance_id) override;
        optional<material&> get_material(sid instance_id) override;
        optional<texture&> get_texture(sid instance_id) override;

        sid get_root_node() override;
        sid get_active_scene_camera_node_sid() override;

        void set_main_camera(sid node_id) override;

        void attach(sid child_node, sid parent_node) override;
        void detach(sid node) override;

        //! \brief Retrieves a \a scene_node from the \a scene.
        //! \param[in] instance_id The \a sid of the \a scene_node to retrieve from the \a scene.
        //! \return An optional \a scene_node reference.
        optional<scene_node&> get_scene_node(sid instance_id);

        //! \brief Retrieves a \a scene_transform from the \a scene.
        //! \param[in] instance_id The \a sid of the \a scene_transform to retrieve from the \a scene.
        //! \return An optional \a scene_transform reference.
        optional<scene_transform&> get_scene_transform(sid instance_id);

        //! \brief Retrieves a \a scene_camera from the \a scene.
        //! \param[in] instance_id The \a sid of the \a scene_camera to retrieve from the \a scene.
        //! \return An optional \a scene_camera reference.
        optional<scene_camera&> get_scene_camera(sid instance_id);

        //! \brief Retrieves a \a scene_light from the \a scene.
        //! \param[in] instance_id The \a sid of the \a scene_light to retrieve from the \a scene.
        //! \return An optional \a scene_light reference.
        optional<scene_light&> get_scene_light(sid instance_id);

        //! \brief Retrieves a \a scene_material from the \a scene.
        //! \param[in] instance_id The \a sid of the \a scene_material to retrieve from the \a scene.
        //! \return An optional \a scene_material reference.
        optional<scene_material&> get_scene_material(sid instance_id);

        //! \brief Retrieves a \a scene_model from the \a scene.
        //! \param[in] instance_id The \a sid of the \a scene_model to retrieve from the \a scene.
        //! \return An optional \a scene_model reference.
        optional<scene_model&> get_scene_model(sid instance_id);

        //! \brief Retrieves a \a scene_mesh from the \a scene.
        //! \param[in] instance_id The \a sid of the \a scene_mesh to retrieve from the \a scene.
        //! \return An optional \a scene_mesh reference.
        optional<scene_mesh&> get_scene_mesh(sid instance_id);

        //! \brief Retrieves a \a scene_primitive from the \a scene.
        //! \param[in] instance_id The \a sid of the \a scene_primitive to retrieve from the \a scene.
        //! \return An optional \a scene_primitive reference.
        optional<scene_primitive&> get_scene_primitive(sid instance_id);

        //! \brief Retrieves a \a scene_texture from the \a scene.
        //! \param[in] instance_id The \a sid of the \a scene_texture to retrieve from the \a scene.
        //! \return An optional \a scene_texture reference.
        optional<scene_texture&> get_scene_texture(sid instance_id);

        //! \brief Retrieves the active \a scene_camera from the \a scene.
        //! \param[out] position_out The position of the active \a scene_camera queried from the \a transform.
        //! \return The optional \a scene_camera reference from the active \a scene_camera.
        optional<scene_camera&> get_active_scene_camera(vec3& position_out);

        //! \brief Retrieves the \a sid of the active \a scene_camera from the \a scene.
        //! \return The \a sid of the active \a scene_camera.
        sid get_active_camera_sid();

        //! \brief Updates the \a scene.
        //! \param[in] dt Past time since last call.
        void update(float dt);

        //! \brief Retrieves a list of \a scene_render_instances from the \a scene to render.
        //! \details Used by the \a renderer to query and draw all the stuff from the \a scene.
        //! \return The list of \a scene_render_instances from the \a scene to render.
        inline const std::vector<scene_render_instance>& get_render_instances()
        {
            return m_render_instances;
        }

        //! \brief Draws the hierarchy of \a nodes in a ui widget.
        //! \details Does not create an ImGui window, only draws contents.
        void draw_scene_hierarchy(sid& selected);

        //! \brief Retrieves the display name of a certain element referenced by a given \a sid.
        //! \param[in] object The \a sid of the referenced element to get the display name for.
        //! \return The display name string of a certain element referenced by a given \a sid.
        string get_display_name(sid object);

      private:
        //! \brief Loads an image from a path and creates and returns a \a gfx_texture and \a gfx_sampler for the image.
        //! \param[in] path The full path to the image to load.
        //! \param[in] standard_color_space True if the image should be loaded in standard color space, else false.
        //! \param[in] high_dynamic_range True if the image should be loaded as high dynamic range, else false.
        //! \param[in] sampler_info The \a sampler_create_info required for the creation of the sampler.
        //! \return The created \a gfx_texture and \a gfx_sampler pair.
        std::pair<gfx_handle<const gfx_texture>, gfx_handle<const gfx_sampler>> create_gfx_texture_and_sampler(const string& path, bool standard_color_space, bool high_dynamic_range,
                                                                                                               const sampler_create_info& sampler_info);

        //! \brief Creates and returns a \a gfx_texture and \a gfx_sampler for a given image.
        //! \param[in] img The \a image_resource to use.
        //! \param[in] standard_color_space True if the image should be loaded in standard color space, else false.
        //! \param[in] high_dynamic_range True if the image should be loaded as high dynamic range, else false.
        //! \param[in] sampler_info The \a sampler_create_info required for the creation of the sampler.
        //! \return The created \a gfx_texture and \a gfx_sampler pair.
        std::pair<gfx_handle<const gfx_texture>, gfx_handle<const gfx_sampler>> create_gfx_texture_and_sampler(const image_resource& img, bool standard_color_space, bool high_dynamic_range,
                                                                                                               const sampler_create_info& sampler_info);

        //! \brief Loads a model file and creates a \a scenario list with \a sids of all \a scenarios in the model.
        //! \details Creates and stores all necessary resources and structures to add the model to a scene.
        //! \param[in] path The full path to the model to load.
        //! \param[out] default_scenario The default \a scenario in the loaded \a model.
        //! \return The list with \a sids of all \a scenarios in the model.
        std::vector<sid> load_model_from_file(const string& path, int32& default_scenario);

        //! \brief Builds a \a node from a tinygltf model node.
        //! \param[in] m The loaded tinygltf model.
        //! \param[in] n The tinygltf model node.
        //! \param[in] buffer_view_ids The \a sids of all loaded \a scene_buffer_views.
        //! \param[in,out] scenario_nodes All \a sids of all \a nodes added to the \a scenario.
        //! \param[in] parent_node_id The \a sid of the parent \a node.
        //! \param[in] scenario_id The \a sid of the \a scenario.
        void build_model_node(tinygltf::Model& m, tinygltf::Node& n, const std::vector<sid>& buffer_view_ids, std::vector<sid>& scenario_nodes, sid parent_node_id, sid scenario_id);

        //! \brief Builds a \a scene_camera from a tinygltf model camera.
        //! \param[in] camera The loaded tinygltf model camera.
        //! \param[in] containing_node_id The \a sid of the \a node the \a scene_camera should be added to.
        //! \return The \a sid of the created \a scene_camera.
        sid build_model_camera(tinygltf::Camera& camera, sid containing_node_id);

        //! \brief Builds a \a scene_mesh from a tinygltf model mesh.
        //! \param[in] m The loaded tinygltf model.
        //! \param[in] mesh The loaded tinygltf model mesh.
        //! \param[in] buffer_view_ids The \a sids of all loaded \a scene_buffer_views.
        //! \param[in] containing_node_id The \a sid of the \a node the \a scene_mesh should be added to.
        //! \return The \a sid of the created \a scene_mesh.
        sid build_model_mesh(tinygltf::Model& m, tinygltf::Mesh& mesh, const std::vector<sid>& buffer_view_ids, sid containing_node_id);

        //! \brief Builds a \a material from a tinygltf model material.
        //! \param[out] mat The \a material to load into.
        //! \param[in] primitive_material The loaded tinygltf model material.
        //! \param[in] m The loaded tinygltf model.
        void load_material(material& mat, const tinygltf::Material& primitive_material, tinygltf::Model& m);

        //! \brief Internal function to abstract the creation of the basic \a skylight structure.
        //! \param[in] new_skylight The \a skylight to add.
        //! \param[in] containing_node_id The \a sid of the \a node the \a skylight should be added to.
        sid add_skylight_structure(const skylight& new_skylight, sid containing_node_id);

        //! \brief Mangos internal context for shared usage in all \a scenes.
        shared_ptr<context_impl> m_shared_context;

        // TODO Paul: Cleanup this hardcoded mess.

        //! \brief The \a packed_freelist for all \a scene_textures in the \a scene.
        packed_freelist<scene_texture, 32768> m_scene_textures;

        //! \brief The \a packed_freelist for all \a scene_materials in the \a scene.
        packed_freelist<scene_material, 16384> m_scene_materials;

        //! \brief The \a packed_freelist for all \a scene_buffers in the \a scene.
        packed_freelist<scene_buffer, 4096> m_scene_buffers;

        //! \brief The \a packed_freelist for all \a scene_buffer_views in the \a scene.
        packed_freelist<scene_buffer_view, 8192> m_scene_buffer_views;

        //! \brief The \a packed_freelist for all \a scene_meshes in the \a scene.
        packed_freelist<scene_mesh, 16384> m_scene_meshes;

        //! \brief The \a packed_freelist for all \a scene_primitives in the \a scene.
        packed_freelist<scene_primitive, 32768> m_scene_primitives;

        //! \brief The \a packed_freelist for all \a scene_cameras in the \a scene.
        packed_freelist<scene_camera, 64> m_scene_cameras;

        //! \brief The \a packed_freelist for all \a scene_lights in the \a scene.
        packed_freelist<scene_light, 16384> m_scene_lights;

        //! \brief The \a packed_freelist for all \a scene_transforms in the \a scene.
        packed_freelist<scene_transform, 32768> m_scene_transforms;

        //! \brief The \a packed_freelist for all \a scene_nodes in the \a scene.
        packed_freelist<scene_node, 32768> m_scene_nodes;

        //! \brief The \a packed_freelist for all \a scene_scenarios in the \a scene.
        packed_freelist<scene_scenario, 32> m_scene_scenarios;

        //! \brief The \a packed_freelist for all \a scene_models in the \a scene.
        packed_freelist<scene_model, 32> m_scene_models;

        //! \brief Structure to describe the scene graph in the \a scene.
        struct hierarchy_node
        {
            //! \brief The \a sid of the \a node referenced by this \a hierarchy_node.
            sid node_id;
            //! \brief List of unique pointers to the children of this \a hierarchy_node.
            std::vector<unique_ptr<hierarchy_node>> children;
        };

        //! \brief Detach a \a node from the parent and returns the unique pointer to the \a hierarchy_node.
        //! \details The same as detach, but instead of attaching the detached node to the root node it is returned.
        //! \param[in] node The \a node to detach.
        //! \return The unique pointer to the detached \a hierarchy_node.
        unique_ptr<hierarchy_node> detach_and_get(sid node);

        //! \brief The internal recursive function to draw the hierarchy of \a nodes in a ui widget.
        //! \param[in] current Pointer to the current \a hierarchy_node to inspect and draw.
        //! \param[in,out] selected The \a sid of the selected node.
        //! \return The list of \a sids of \a nodes that should be removed, after the hierarchy is drawn.
        std::vector<sid> draw_scene_hierarchy_internal(hierarchy_node* current, sid& selected);

        //! \brief Helper function to search all \a hierarchy_nodes for the one with a certain \a sid in a breath first search manner.
        //! \param[in] id The \a sid to search for.
        //! \param[out] out The \a hierarchy_node with \a sid id
        //! \return True if the \a hierarchy_node with the correct \a sid was found, else false.
        bool sg_bfs_node(sid id, hierarchy_node*& out)
        {
            std::queue<hierarchy_node*> stack;
            stack.push(m_scene_graph_root.get());

            while (!stack.empty())
            {
                auto current = stack.front();
                if (id == current->node_id)
                {
                    out = current;
                    return true;
                }

                for (auto& c : current->children)
                    stack.push(c.get());

                stack.pop();
            }
            return false;
        }

        //! \brief Helper function to execute a function for all \a hierarchy_nodes in a breath first manner.
        //! \param[in] lambda The lambda function to call for each \a hierarchy_node.
        void sg_bfs_for_each(std::function<bool(hierarchy_node& hn)> lambda)
        {
            std::queue<hierarchy_node*> stack;
            stack.push(m_scene_graph_root.get());

            while (!stack.empty())
            {
                auto current = stack.front();

                if (!lambda(*current))
                    break;

                for (auto& c : current->children)
                    stack.push(c.get());

                stack.pop();
            }
        }

        //! \brief The unique pointer holding the \a hierarchy_node representing the scene graphs root \a node.
        unique_ptr<hierarchy_node> m_scene_graph_root; // TODO Paul: This is a pretty barebones solution. But it should work for now.

        //! \brief The current list if \a scene_render_instances.
        std::vector<scene_render_instance> m_render_instances;

        //! \brief The \a graphics_device of the \a scene.
        graphics_device_handle m_scene_graphics_device;

        //! \brief The \a sid of the root \a node.
        sid m_root_node;

        //! \brief The \a sid of \a node of the current main \a scene_camera.
        sid m_main_camera_node;

        //! \brief The \a sid of the \a node currently selected.
        sid m_ui_selected_sid;
    };
} // namespace mango

#endif // MANGO_SCENE_IMPL_HPP
