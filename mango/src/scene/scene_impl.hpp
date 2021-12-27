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
#include <scene/scene_structures_internal.hpp>
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

        uid add_node(const string& name, uid parent_node) override;
        uid add_perspective_camera(perspective_camera& new_perspective_camera, uid node_id) override;
        uid add_orthographic_camera(orthographic_camera& new_orthographic_camera, uid node_id) override;
        uid add_directional_light(directional_light& new_directional_light, uid node_id) override;
        uid add_skylight(skylight& new_skylight, uid node_id) override;
        uid add_atmospheric_light(atmospheric_light& new_atmospheric_light, uid node_id) override;

        uid build_material(material& new_material) override;
        uid load_texture_from_image(const string& path, bool standard_color_space, bool high_dynamic_range) override;

        uid load_model_from_gltf(const string& path) override;
        void add_model_to_scene(uid model_to_add, uid scenario_id, uid node_id) override;

        uid add_skylight_from_hdr(const string& path, uid node_id) override;

        void remove_node(uid node_id) override;
        void remove_perspective_camera(uid node_id) override;
        void remove_orthographic_camera(uid node_id) override;
        void remove_mesh(uid node_id) override;
        void remove_directional_light(uid node_id) override;
        void remove_skylight(uid node_id) override;
        void remove_atmospheric_light(uid node_id) override;

        optional<node&> get_node(uid node_id) override;
        optional<transform&> get_transform(uid node_id) override;
        optional<perspective_camera&> get_perspective_camera(uid node_id) override;
        optional<orthographic_camera&> get_orthographic_camera(uid node_id) override;
        optional<directional_light&> get_directional_light(uid node_id) override;
        optional<skylight&> get_skylight(uid node_id) override;
        optional<atmospheric_light&> get_atmospheric_light(uid node_id) override;

        optional<model&> get_model(uid instance_id) override;
        optional<mesh&> get_mesh(uid instance_id) override;
        optional<material&> get_material(uid instance_id) override;
        optional<texture&> get_texture(uid instance_id) override;

        uid get_root_node() override;
        uid get_active_camera_uid() override;

        void set_main_camera(uid node_id) override;

        void attach(uid child_node, uid parent_node) override;
        void detach(uid child_node, uid parent_node) override;

        //! \brief Retrieves a \a texture_gpu_data from the \a scene.
        //! \param[in] instance_id The \a uid of the \a texture_gpu_data to retrieve from the \a scene.
        //! \return An optional \a texture_gpu_data reference.
        optional<texture_gpu_data&> get_texture_gpu_data(uid instance_id);

        //! \brief Retrieves a \a material_gpu_data from the \a scene.
        //! \param[in] instance_id The \a uid of the \a material_gpu_data to retrieve from the \a scene.
        //! \return An optional \a material_gpu_data reference.
        optional<material_gpu_data&> get_material_gpu_data(uid instance_id);

        //! \brief Retrieves a \a primitive_gpu_data from the \a scene.
        //! \param[in] instance_id The \a uid of the \a primitive_gpu_data to retrieve from the \a scene.
        //! \return An optional \a primitive_gpu_data reference.
        optional<primitive_gpu_data&> get_primitive_gpu_data(uid instance_id);

        //! \brief Retrieves a \a mesh_gpu_data from the \a scene.
        //! \param[in] instance_id The \a uid of the \a mesh_gpu_data to retrieve from the \a scene.
        //! \return An optional \a mesh_gpu_data reference.
        optional<mesh_gpu_data&> get_mesh_gpu_data(uid instance_id);

        //! \brief Retrieves a \a camera_gpu_data from the \a scene.
        //! \param[in] instance_id The \a uid of the \a camera_gpu_data to retrieve from the \a scene.
        //! \return An optional \a camera_gpu_data reference.
        optional<camera_gpu_data&> get_camera_gpu_data(uid instance_id);

        //! \brief Retrieves a \a light_gpu_data from the \a scene.
        //! \param[in] instance_id The \a uid of the \a light_gpu_data to retrieve from the \a scene.
        //! \return An optional \a light_gpu_data reference.
        optional<light_gpu_data&> get_light_gpu_data(uid instance_id);

        //! \brief Retrieves a \a buffer_view from the \a scene.
        //! \param[in] instance_id The \a uid of the \a buffer_view to retrieve from the \a scene.
        //! \return An optional \a buffer_view reference.
        optional<buffer_view&> get_buffer_view(uid instance_id);

        //! \brief Retrieves the \a camera_gpu_data from the active \a camera from the \a scene.
        //! \return The optional \a camera_gpu_data referenced from the active \a camera.
        optional<camera_gpu_data&> get_active_camera_gpu_data();

        //! \brief Updates the \a scene.
        //! \param[in] dt Past time since last call.
        void update(float dt);

        //! \brief Retrieves a list of \a render_instances from the \a scene to render.
        //! \details Used by the \a renderer to query and draw all the stuff from the \a scene.
        //! \return The list of \a render_instances from the \a scene to render.
        inline const std::vector<render_instance>& get_render_instances()
        {
            return m_render_instances;
        }

        //! \brief Draws the hierarchy of \a nodes in a ui widget.
        //! \details Does not create an ImGui window, only draws contents.
        void draw_scene_hierarchy(uid& selected);
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

        //! \brief Loads a model file and creates a \a scenario list with \a uids of all \a scenarios in the model.
        //! \details Creates and stores all necessary resources and structures to add the model to a scene.
        //! \param[in] path The full path to the model to load.
        //! \param[out] default_scenario The default \a scenario in the loaded \a model.
        //! \return The list with \a uids of all \a scenarios in the model.
        std::vector<uid> load_model_from_file(const string& path, int32& default_scenario);

        //! \brief Builds a \a node from a tinygltf model node.
        //! \param[in] m The loaded tinygltf model.
        //! \param[in] n The tinygltf model node.
        //! \param[in] buffer_view_ids The \a uids of all loaded \a scene_buffer_views.
        //! \param[in,out] scenario_nodes All \a uids of all \a nodes added to the \a scenario.
        //! \param[in] parent_node_id The \a uid of the parent \a node.
        //! \param[in] scenario_id The \a uid of the \a scenario.
        void build_model_node(tinygltf::Model& m, tinygltf::Node& n, const std::vector<uid>& buffer_view_ids, std::vector<uid>& scenario_nodes, uid parent_node_id, uid scenario_id);

        //! \brief Builds a \a scene_camera from a tinygltf model camera.
        //! \param[in] camera The loaded tinygltf model camera.
        //! \param[in] node_id The \a uid of the \a node the \a scene_camera should be added to.
        //! \return The \a uid of the created \a scene_camera.
        uid build_model_camera(tinygltf::Camera& camera, uid node_id);

        //! \brief Builds a \a scene_mesh from a tinygltf model mesh.
        //! \param[in] m The loaded tinygltf model.
        //! \param[in] mesh The loaded tinygltf model mesh.
        //! \param[in] buffer_view_ids The \a uids of all loaded \a scene_buffer_views.
        //! \param[in] node_id The \a uid of the \a node the \a scene_mesh should be added to.
        //! \return The \a uid of the created \a scene_mesh.
        uid build_model_mesh(tinygltf::Model& m, tinygltf::Mesh& mesh, const std::vector<uid>& buffer_view_ids, uid node_id);

        //! \brief Builds a \a material from a tinygltf model material.
        //! \param[out] mat The \a material to load into.
        //! \param[in] primitive_material The loaded tinygltf model material.
        //! \param[in] m The loaded tinygltf model.
        void load_material(material& mat, const tinygltf::Material& primitive_material, tinygltf::Model& m);

        //! \brief Internal function to abstract the creation of the basic \a skylight structure.
        //! \param[in] new_skylight The \a skylight to add.
        //! \param[in] node_id The \a uid of the \a node the \a skylight should be added to.
        uid add_skylight_structure(const skylight& new_skylight, uid node_id);

        //! \brief Mangos internal context for shared usage in all \a scenes.
        shared_ptr<context_impl> m_shared_context;

        //! \brief The \a packed_freelist for all \a models in the \a scene.
        packed_freelist<model, 16> m_models;
        //! \brief The \a packed_freelist for all \a scenarios in the \a scene.
        packed_freelist<scenario, 32> m_scenarios;
        //! \brief The \a packed_freelist for all \a light_gpu_data in the \a scene. (per scenario)
        packed_freelist<light_gpu_data, 32> m_light_gpu_data;
        //! \brief The \a packed_freelist for all \a nodes in the \a scene.
        packed_freelist<node, 32768> m_nodes;
        //! \brief The \a packed_freelist for all \a transforms in the \a scene.
        packed_freelist<transform, 32768> m_transforms;

        //! \brief The \a packed_freelist for all \a meshes in the \a scene.
        packed_freelist<mesh, 8192> m_meshes;
        //! \brief The \a packed_freelist for all \a mesh_gpu_data in the \a scene.
        packed_freelist<mesh_gpu_data, 8192> m_mesh_gpu_data;
        //! \brief The \a packed_freelist for all \a primitives in the \a scene.
        packed_freelist<primitive, 16384> m_primitives;
        //! \brief The \a packed_freelist for all \a primitive_gpu_data in the \a scene.
        packed_freelist<primitive_gpu_data, 16384> m_primitive_gpu_data;
        //! \brief The \a packed_freelist for all \a materials in the \a scene.
        packed_freelist<material, 16384> m_materials;
        //! \brief The \a packed_freelist for all \a material_gpu_data in the \a scene.
        packed_freelist<material_gpu_data, 16384> m_material_gpu_data;
        //! \brief The \a packed_freelist for all \a textures in the \a scene.
        packed_freelist<texture, 32768> m_textures;
        //! \brief The \a packed_freelist for all \a texture_gpu_data in the \a scene.
        packed_freelist<texture_gpu_data, 32768> m_texture_gpu_data;

        //! \brief The \a packed_freelist for all \a perspective_cameras in the \a scene.
        packed_freelist<perspective_camera, 32> m_perspective_cameras;
        //! \brief The \a packed_freelist for all \a orthographic_cameras in the \a scene.
        packed_freelist<orthographic_camera, 32> m_orthographic_cameras;
        //! \brief The \a packed_freelist for all \a camera_gpu_data in the \a scene.
        packed_freelist<camera_gpu_data, 64> m_camera_gpu_data;

        //! \brief The \a packed_freelist for all \a directional_lights in the \a scene.
        packed_freelist<directional_light, 32> m_directional_lights;
        //! \brief The \a packed_freelist for all \a skylights in the \a scene.
        packed_freelist<skylight, 32> m_skylights;
        //! \brief The \a packed_freelist for all \a atmospheric_lights in the \a scene.
        packed_freelist<atmospheric_light, 32> m_atmospheric_lights;

        //! \brief The \a packed_freelist for all \a buffer_views in the \a scene.
        packed_freelist<buffer_view, 8192> m_buffer_views;

        //! \brief The internal recursive function to draw the hierarchy of \a nodes in a ui widget.
        //! \param[in] current The current \a nodes \a uid to inspect and draw.
        //! \param[in,out] selected The \a uid of the selected node.
        //! \return The list of \a uids of \a nodes that should be removed, after the hierarchy is drawn.
        std::vector<uid> draw_scene_hierarchy_internal(uid current, uid& selected);

        //! \brief Helper function to execute a function for all \a nodes in a breath first manner.
        //! \param[in] lambda The lambda function to call for each \a node.
        void sg_bfs_for_each(std::function<bool(node& node)> lambda)
        {
            std::queue<uid> stack;
            stack.push(m_root_node);

            while (!stack.empty())
            {
                uid current = stack.front();

                optional<node&> nd = get_node(current);
                MANGO_ASSERT(nd, "Node in scene graph does not exist!");

                if (!lambda(nd.value()))
                    break;

                for (auto& c : nd->children)
                    stack.push(c);

                stack.pop();
            }
        }

        //! \brief The current list if \a render_instances.
        std::vector<render_instance> m_render_instances;

        //! \brief The \a graphics_device of the \a scene.
        graphics_device_handle m_scene_graphics_device;

        //! \brief The \a uid of the root \a node.
        uid m_root_node;

        //! \brief The \a uid of \a node of the current main \a camera.
        uid m_main_camera_node;

        //! \brief The \a uid of the \a node currently selected.
        uid m_ui_selected_uid;
    };
} // namespace mango

#endif // MANGO_SCENE_IMPL_HPP
