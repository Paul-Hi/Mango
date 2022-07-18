//! \file      scene_impl.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2022
//! \copyright Apache License 2.0

#ifndef MANGO_SCENE_IMPL_HPP
#define MANGO_SCENE_IMPL_HPP

#include <graphics/graphics.hpp>
#include <mango/slotmap.hpp>
#include <mango/scene.hpp>
#include <map>
#include <queue>
#include <rendering/light_stack.hpp>
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

        key add_node(const string& name, optional<key> parent_node = NONE) override;
        optional<key> add_perspective_camera(perspective_camera& new_perspective_camera, key node_id) override;
        optional<key> add_orthographic_camera(orthographic_camera& new_orthographic_camera, key node_id) override;
        optional<key> add_directional_light(directional_light& new_directional_light, key node_id) override;
        optional<key> add_skylight(skylight& new_skylight, key node_id) override;
        optional<key> add_atmospheric_light(atmospheric_light& new_atmospheric_light, key node_id) override;

        optional<key> build_material(material& new_material) override;
        key load_texture_from_image(const string& path, bool standard_color_space, bool high_dynamic_range) override;

        key load_model_from_gltf(const string& path) override;
        void add_model_to_scene(model model_to_add, key scenario_id, key node_id) override;

        optional<key> add_skylight_from_hdr(const string& path, key node_id) override;

        void remove_node(key node_id) override;
        void remove_perspective_camera(key node_id) override;
        void remove_orthographic_camera(key node_id) override;
        void remove_mesh(key node_id); // For now no override since we only add them internally as well...
        void remove_directional_light(key node_id) override;
        void remove_skylight(key node_id) override;
        void remove_atmospheric_light(key node_id) override;
        void unload_gltf_model(key model_id) override;

        optional<node&> get_node(key node_id) override;
        optional<transform&> get_transform(key node_id) override;
        optional<perspective_camera&> get_perspective_camera(key node_id) override;
        optional<orthographic_camera&> get_orthographic_camera(key node_id) override;
        optional<directional_light&> get_directional_light(key node_id) override;
        optional<skylight&> get_skylight(key node_id) override;
        optional<atmospheric_light&> get_atmospheric_light(key node_id) override;

        optional<model&> get_model(key instance_id) override;
        optional<mesh&> get_mesh(key instance_id) override;
        optional<material&> get_material(key instance_id) override;
        optional<texture&> get_texture(key instance_id) override;

        key get_root_node() override;
        optional<key> get_active_camera_key() override;

        void set_main_camera(optional<key> node_id) override;

        void attach(key child_node, key parent_node) override;
        void detach(key child_node, key parent_node) override;

        //! \brief Removes a \a texture for a given \a key.
        //! \param[in] instance_id The \a key of the \a texture to remove for from the \a scene.
        void remove_texture(key texture_id);

        //! \brief Retrieves a \a primitive from the \a scene.
        //! \param[in] instance_id The \a key of the \a primitive to retrieve from the \a scene.
        //! \return An optional \a primitive reference.
        optional<primitive&> get_primitive(key instance_id);

        //! \brief Retrieves a global transformation matrix from the \a scene.
        //! \param[in] instance_id The \a key of the matrix to retrieve from the \a scene.
        //! \return An optional matrix reference.
        optional<mat4&> get_global_transformation_matrix(key instance_id);

        //! \brief Retrieves a \a texture_gpu_data from the \a scene.
        //! \param[in] instance_id The \a key of the \a texture_gpu_data to retrieve from the \a scene.
        //! \return An optional \a texture_gpu_data reference.
        optional<texture_gpu_data&> get_texture_gpu_data(key instance_id);

        //! \brief Retrieves a \a material_gpu_data from the \a scene.
        //! \param[in] instance_id The \a key of the \a material_gpu_data to retrieve from the \a scene.
        //! \return An optional \a material_gpu_data reference.
        optional<material_gpu_data&> get_material_gpu_data(key instance_id);

        //! \brief Retrieves a \a primitive_gpu_data from the \a scene.
        //! \param[in] instance_id The \a key of the \a primitive_gpu_data to retrieve from the \a scene.
        //! \return An optional \a primitive_gpu_data reference.
        optional<primitive_gpu_data&> get_primitive_gpu_data(key instance_id);

        //! \brief Retrieves a \a mesh_gpu_data from the \a scene.
        //! \param[in] instance_id The \a key of the \a mesh_gpu_data to retrieve from the \a scene.
        //! \return An optional \a mesh_gpu_data reference.
        optional<mesh_gpu_data&> get_mesh_gpu_data(key instance_id);

        //! \brief Retrieves a \a camera_gpu_data from the \a scene.
        //! \param[in] instance_id The \a key of the \a camera_gpu_data to retrieve from the \a scene.
        //! \return An optional \a camera_gpu_data reference.
        optional<camera_gpu_data&> get_camera_gpu_data(key instance_id);

        //! \brief Retrieves the \a light_gpu_data from the \a scene.
        //! \return The constant \a light_gpu_data reference.
        const light_gpu_data& get_light_gpu_data();

        //! \brief Retrieves a \a buffer_view from the \a scene.
        //! \param[in] instance_id The \a key of the \a buffer_view to retrieve from the \a scene.
        //! \return An optional \a buffer_view reference.
        optional<buffer_view&> get_buffer_view(key instance_id);

        //! \brief Retrieves the \a camera_gpu_data from the active \a camera from the \a scene.
        //! \return The optional \a camera_gpu_data referenced from the active \a camera.
        optional<camera_gpu_data&> get_active_camera_gpu_data();

        //! \brief Retrieves a list of loaded \a model \a uids from the \a scene.
        //! \return A list of loaded \a model \a uids from the \a scene.
        inline const slotmap<model>& get_imported_models()
        {
            return m_models;
        }

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
        //! \param[in,out] selected The \a key of the selected node.
        //! \details Does not create an ImGui window, only draws contents.
        void draw_scene_hierarchy(optional<key>& selected);

        //! \brief Sets the average luminance for camera auto exposure calculations.
        //! \param[in] avg_luminance The average luminance to use.
        inline void set_average_luminance(float avg_luminance)
        {
            m_average_luminance = avg_luminance;
        }

        //! \brief Retrieves the \a light_stack of the \a scene.
        //! \return The \a light_stack.
        inline const light_stack& get_light_stack()
        {
            return m_light_stack;
        }

        //! \brief Returns if a camera in the \a scene requires auto exposure calculations.
        //! \return True if a camera in the \a scene requires auto exposure calculations, else false.
        inline bool calculate_auto_exposure()
        {
            return m_requires_auto_exposure;
        }

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
        std::vector<key> load_model_from_file(const string& path, int32& default_scenario);

        //! \brief Builds a \a node from a tinygltf model node.
        //! \param[in] m The loaded tinygltf model.
        //! \param[in] n The tinygltf model node.
        //! \param[in] buffer_view_ids The \a uids of all loaded \a buffer_views.
        //! \return The \a key of the created \a node.
        key build_model_node(tinygltf::Model& m, tinygltf::Node& n, const std::vector<key>& buffer_view_ids);

        //! \brief Builds a \a camera from a tinygltf model camera.
        //! \param[in] t_camera The loaded tinygltf model camera.
        //! \param[in] node_id The \a key of the \a node the \a camera should be added to.
        //! \param[in] target The target vector of the \a camera to build.
        //! \param[out] out_type The \a camera_type of the imported \a camera.
        //! \return The \a key of the created \a camera.
        key build_model_camera(tinygltf::Camera& t_camera, key node_id, const vec3& target, camera_type& out_type);

        //! \brief Builds a \a mesh from a tinygltf model mesh.
        //! \param[in] m The loaded tinygltf model.
        //! \param[in] t_mesh The loaded tinygltf model mesh.
        //! \param[in] node_id The \a key of the \a node the \a mesh should be added to.
        //! \param[in] buffer_view_ids The \a uids of all loaded \a buffer_views.
        //! \return The \a key of the created \a mesh or NONE on error.
        optional<key> build_model_mesh(tinygltf::Model& m, tinygltf::Mesh& t_mesh, key node_id, const std::vector<key>& buffer_view_ids);

        //! \brief Builds a \a material from a tinygltf model material.
        //! \param[in] primitive_material The loaded tinygltf model material.
        //! \param[in] m The loaded tinygltf model.
        //! \return The \a key of the created \a material.
        optional<key> load_material(const tinygltf::Material& primitive_material, tinygltf::Model& m);

        //! \brief Returns the \a key of the default material and creates it if not already done.
        key default_material();

        //! \brief Instantiates some \a scene in the scene graph.
        //! \param[in] node_id The \a scene \a node \a key to instantiate.
        //! \param[in] parent_id The parent \a node \a key.
        void instantiate_model_scene(key node_id, key parent_id);

        //! \brief Removes a \a node belonging to a \a model.
        //! \param[in] node_id The \a key of the \a node to remove.
        void remove_model_node(key node_id);

        //! \brief Traverses a \a node and its children in the scene graph and updates the transformation if necessary; also creates render instances.
        //! \param[in] node_id The \a key of the node.
        //! \param[in] parent_id The \a key of the parent.
        //! \param[in] force_update Force the update (when the parent had to be updated).
        void update_scene_graph(key node_id, optional<key> parent_id, bool force_update);

        //! \brief Mangos internal context for shared usage in all \a scenes.
        shared_ptr<context_impl> m_shared_context;

        //! \brief The light stack managing all lights.
        light_stack m_light_stack;
        //! \brief The \a light_gpu_data in the \a scene.
        light_gpu_data m_light_gpu_data;
        //! \brief The \a slotmap for all \a models in the \a scene.
        slotmap<model> m_models;
        //! \brief The \a slotmap for all \a scenarios in the \a scene.
        slotmap<scenario> m_scenarios;
        //! \brief The \a slotmap for all \a nodes in the \a scene.
        slotmap<node> m_nodes;
        //! \brief The \a slotmap for all \a transforms in the \a scene.
        slotmap<transform> m_transforms;
        //! \brief The \a slotmap for all world transformations of the \a nodes in the \a scene.
        slotmap<mat4> m_global_transformation_matrices;

        //! \brief The \a slotmap for all \a meshes in the \a scene.
        slotmap<mesh> m_meshes;
        //! \brief The \a slotmap for all \a mesh_gpu_data in the \a scene.
        slotmap<mesh_gpu_data> m_mesh_gpu_data;
        //! \brief The \a slotmap for all \a primitives in the \a scene.
        slotmap<primitive> m_primitives;
        //! \brief The \a slotmap for all \a primitive_gpu_data in the \a scene.
        slotmap<primitive_gpu_data> m_primitive_gpu_data;
        //! \brief The \a slotmap for all \a materials in the \a scene.
        slotmap<material> m_materials;
        //! \brief The \a slotmap for all \a material_gpu_data in the \a scene.
        slotmap<material_gpu_data> m_material_gpu_data;
        //! \brief The \a slotmap for all \a textures in the \a scene.
        slotmap<texture> m_textures;
        //! \brief The \a slotmap for all \a texture_gpu_data in the \a scene.
        slotmap<texture_gpu_data> m_texture_gpu_data;

        //! \brief The \a slotmap for all \a perspective_cameras in the \a scene.
        slotmap<perspective_camera> m_perspective_cameras;
        //! \brief The \a slotmap for all \a orthographic_cameras in the \a scene.
        slotmap<orthographic_camera> m_orthographic_cameras;
        //! \brief The \a slotmap for all \a camera_gpu_data in the \a scene.
        slotmap<camera_gpu_data> m_camera_gpu_data;

        //! \brief The \a slotmap for all \a directional_lights in the \a scene.
        slotmap<directional_light> m_directional_lights;
        //! \brief The \a slotmap for all \a skylights in the \a scene.
        slotmap<skylight> m_skylights;
        //! \brief The \a slotmap for all \a atmospheric_lights in the \a scene.
        slotmap<atmospheric_light> m_atmospheric_lights;

        //! \brief The \a slotmap for all \a buffer_views in the \a scene.
        slotmap<buffer_view> m_buffer_views;

        //! \brief Maps names of materials to already loaded \a material \a uids.
        std::map<string, key> material_name_to_key;

        //! \brief The internal recursive function to draw the hierarchy of \a nodes in a ui widget.
        //! \param[in] current The current \a nodes \a key to inspect and draw.
        //! \param[in] parent The current \a nodes parent \a key.
        //! \param[in,out] selected The \a key of the selected node.
        //! \return The list of \a uids of \a nodes that should be removed, after the hierarchy is drawn.
        std::vector<key> draw_scene_hierarchy_internal(key current, optional<key> parent, optional<key>& selected);

        //! \brief Returns a name for a \a node_type and a \a node name.
        //! \param[in] type The \a node_type of the \a node.
        //! \param[in] name The name of the \a node.
        //! \return The name.
        string get_display_name(node_type type, const string name);

        //! \brief The \a graphics_device of the \a scene.
        const graphics_device_handle& m_scene_graphics_device;

        //! \brief The current list if \a render_instances.
        std::vector<render_instance> m_render_instances;

        //! \brief The \a key of the root \a node.
        key m_root_node;

        //! \brief The \a key of \a node of the current main \a camera.
        optional<key> m_main_camera_node;

        //! \brief The \a key of the \a node currently selected.
        optional<key> m_ui_selected_key;

        //! \brief The \a key of the default material.
        optional<key> m_default_material;

        //! \brief The average luminance (which can be set)
        float m_average_luminance = 1.0f;

        //! \brief True if a camera in the \a scene requires auto exposure calculations, else false.
        bool m_requires_auto_exposure = false;
    };
} // namespace mango

#endif // MANGO_SCENE_IMPL_HPP
