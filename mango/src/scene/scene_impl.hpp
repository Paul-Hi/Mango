//! \file      scene_impl.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2022
//! \copyright Apache License 2.0

#ifndef MANGO_SCENE_IMPL_HPP
#define MANGO_SCENE_IMPL_HPP

#include <graphics/graphics.hpp>
#include <mango/scene.hpp>
#include <mango/slotmap.hpp>
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

        handle<node> add_node(const string& name, handle<node> parent_node = NULL_HND<node>) override;
        handle<perspective_camera> add_perspective_camera(perspective_camera& new_perspective_camera, handle<node> node_hnd) override;
        handle<orthographic_camera> add_orthographic_camera(orthographic_camera& new_orthographic_camera, handle<node> node_hnd) override;
        handle<directional_light> add_directional_light(directional_light& new_directional_light, handle<node> node_hnd) override;
        handle<skylight> add_skylight(skylight& new_skylight, handle<node> node_hnd) override;
        handle<atmospheric_light> add_atmospheric_light(atmospheric_light& new_atmospheric_light, handle<node> node_hnd) override;

        handle<material> build_material(material& new_material) override;
        handle<texture> load_texture_from_image(const string& path, bool standard_color_space, bool high_dynamic_range) override;

        handle<model> load_model_from_gltf(const string& path) override;
        void add_model_to_scene(handle<model> model_to_add, handle<scenario> scenario_hnd, handle<node> node_hnd) override;

        handle<skylight> add_skylight_from_hdr(const string& path, handle<node> node_hnd) override;

        void remove_node(handle<node> node_hnd) override;
        void remove_perspective_camera(handle<node> node_hnd) override;
        void remove_orthographic_camera(handle<node> node_hnd) override;
        void remove_mesh(handle<node> node_hnd); // For now no override since we only add them internally as well...
        void remove_directional_light(handle<node> node_hnd) override;
        void remove_skylight(handle<node> node_hnd) override;
        void remove_atmospheric_light(handle<node> node_hnd) override;
        void unload_gltf_model(handle<model> model_hnd) override;

        optional<node&> get_node(handle<node> node_hnd) override;
        optional<transform&> get_transform(handle<node> node_hnd) override;
        optional<perspective_camera&> get_perspective_camera(handle<node> node_hnd) override;
        optional<orthographic_camera&> get_orthographic_camera(handle<node> node_hnd) override;
        optional<directional_light&> get_directional_light(handle<node> node_hnd) override;
        optional<skylight&> get_skylight(handle<node> node_hnd) override;
        optional<atmospheric_light&> get_atmospheric_light(handle<node> node_hnd) override;

        optional<model&> get_model(handle<model> instance_hnd) override;
        optional<mesh&> get_mesh(handle<mesh> instance_hnd) override;
        optional<material&> get_material(handle<material> instance_hnd) override;
        optional<texture&> get_texture(handle<texture> instance_hnd) override;

        handle<node> get_root_node() override;
        handle<node> get_active_camera_node() override;

        void set_main_camera_node(handle<node> node_hnd) override;

        void attach(handle<node> child_node, handle<node> parent_node) override;
        void detach(handle<node> child_node, handle<node> parent_node) override;

        //! \brief Removes a \a texture for a given \a handle.
        //! \param[in] instance_hnd The \a handle of the \a texture to remove for from the \a scene.
        void remove_texture(handle<texture> instance_hnd);

        //! \brief Retrieves a \a primitive from the \a scene.
        //! \param[in] instance_hnd The \a handle of the \a primitive to retrieve from the \a scene.
        //! \return An optional \a primitive reference.
        optional<primitive&> get_primitive(handle<primitive> instance_hnd);

        //! \brief Retrieves a global transformation matrix from the \a scene.
        //! \param[in] instance_hnd The \a handle of the \a mat4 to retrieve from the \a scene.
        //! \return An optional matrix reference.
        optional<mat4&> get_global_transformation_matrix(handle<mat4> instance_hnd);

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
        //! \param[in] instance_hnd The \a handle of the \a buffer_view to retrieve from the \a scene.
        //! \return An optional \a buffer_view reference.
        optional<buffer_view&> get_buffer_view(handle<buffer_view> instance_hnd);

        //! \brief Retrieves the \a camera_gpu_data from the active \a camera from the \a scene.
        //! \return The optional \a camera_gpu_data referenced from the active \a camera.
        optional<camera_gpu_data&> get_active_camera_gpu_data();

        //! \brief Retrieves a list of loaded \a model \a handles from the \a scene.
        //! \return A list of loaded \a model \a handles from the \a scene.
        inline std::vector<handle<model>> get_imported_models()
        {
            auto keys = m_models.keys();
            std::vector<handle<model>> result;
            for (const key& k : keys)
                result.push_back(handle<model>(k));

            return result;
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
        //! \param[in,out] selected The \a handle of the selected \a node.
        //! \details Does not create an ImGui window, only draws contents.
        void draw_scene_hierarchy(handle<node> selected);

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

        //! \brief Loads a model file and creates a \a scenario list with \a handles of all \a scenarios in the model.
        //! \details Creates and stores all necessary resources and structures to add the model to a scene.
        //! \param[in] path The full path to the model to load.
        //! \param[out] default_scenario The default \a scenario in the loaded \a model.
        //! \return The list with \a handles of all \a scenarios in the model.
        std::vector<handle<scenario>> load_model_from_file(const string& path, int32& default_scenario);

        //! \brief Builds a \a node from a tinygltf model node.
        //! \param[in] m The loaded tinygltf model.
        //! \param[in] n The tinygltf model node.
        //! \param[in] buffer_view_ids The \a keys of all loaded \a buffer_views.
        //! \return The \a handle of the created \a node.
        handle<node> build_model_node(tinygltf::Model& m, tinygltf::Node& n, const std::vector<key>& buffer_view_ids);

        //! \brief Builds a \a camera from a tinygltf model camera.
        //! \param[in] t_camera The loaded tinygltf model camera.
        //! \param[in] node_hnd The \a key of the \a handle of the \a node the \a camera should be added to.
        //! \param[in] target The target vector of the \a camera to build.
        void build_model_camera(tinygltf::Camera& t_camera, handle<node> node_hnd, const vec3& target);

        //! \brief Builds a \a mesh from a tinygltf model mesh.
        //! \param[in] m The loaded tinygltf model.
        //! \param[in] t_mesh The loaded tinygltf model mesh.
        //! \param[in] node_id The \a handle of the \a node the \a mesh should be added to.
        //! \param[in] buffer_view_ids The \a keys of all loaded \a buffer_views.
        //! \return The \a handle of the created \a mesh or NULL_HND on error.
        handle<mesh> build_model_mesh(tinygltf::Model& m, tinygltf::Mesh& t_mesh, handle<node> node_hnd, const std::vector<key>& buffer_view_ids);

        //! \brief Builds a \a material from a tinygltf model material.
        //! \param[in] primitive_material The loaded tinygltf model material.
        //! \param[in] m The loaded tinygltf model.
        //! \return The \a handle of the created \a material or NULL_HND on error.
        handle<material> load_material(const tinygltf::Material& primitive_material, tinygltf::Model& m);

        //! \brief Returns the \a handle of the default material and creates it if not already done.
        handle<material> default_material();

        //! \brief Instantiates some \a scene in the scene graph.
        //! \param[in] node_id The \a scene \a node \a handle to instantiate.
        //! \param[in] parent_hnd The parent \a node \a handle.
        void instantiate_model_scene(handle<node> node_hnd, handle<node> parent_hnd);

        //! \brief Removes a \a node belonging to a \a model.
        //! \param[in] node_id The \a handle of the \a node to remove.
        void remove_model_node(handle<node> node_hnd);

        //! \brief Traverses a \a node and its children in the scene graph and updates the transformation if necessary; also creates render instances.
        //! \param[in] node_id The \a handle of the node.
        //! \param[in] parent_hnd The \a handle of the parent or NULL_HND, when root.
        //! \param[in] force_update Force the update (when the parent had to be updated).
        void update_scene_graph(handle<node> node_hnd, handle<node> parent_hnd, bool force_update);

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

        //! \brief Maps names of materials to already loaded \a material \a handles.
        std::map<string, handle<material>> m_material_name_to_handle;

        //! \brief The internal recursive function to draw the hierarchy of \a nodes in a ui widget.
        //! \param[in] current The current \a nodes \a handle to inspect and draw.
        //! \param[in] parent The current \a nodes parent \a handle.
        //! \param[in,out] selected The \a handle of the selected node.
        //! \return The list of \a handles of \a nodes that should be removed, after the hierarchy is drawn.
        std::vector<handle<node>> draw_scene_hierarchy_internal(handle<node> current, handle<node> parent, handle<node> selected);

        //! \brief Returns a name for a \a node_type and a \a node name.
        //! \param[in] type The \a node_type of the \a node.
        //! \param[in] name The name of the \a node.
        //! \return The name.
        string get_display_name(node_type type, const string name);

        //! \brief The \a graphics_device of the \a scene.
        const graphics_device_handle& m_scene_graphics_device;

        //! \brief The current list if \a render_instances.
        std::vector<render_instance> m_render_instances;

        //! \brief The \a handle of the root \a node.
        handle<node> m_root_node;

        //! \brief The \a handle of \a node of the current main \a camera.
        handle<node> m_main_camera_node;

        //! \brief The \a handle of the \a node currently selected.
        handle<node> m_ui_selected_handle;

        //! \brief The \a handle of the default \a material.
        handle<material> m_default_material;

        //! \brief The average luminance (which can be set)
        float m_average_luminance = 1.0f;

        //! \brief True if a camera in the \a scene requires auto exposure calculations, else false.
        bool m_requires_auto_exposure = false;
    };
} // namespace mango

#endif // MANGO_SCENE_IMPL_HPP
