//! \file      scene.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_SCENE_HPP
#define MANGO_SCENE_HPP

#include <mango/scene_structures.hpp>

namespace mango
{
    //! \brief The \a scene of mango.
    //! \details Responsible for handling content in mango.
    class scene
    {
      public:
        virtual ~scene() = default;

        //! \brief Adds a \a node to the \a scene.
        //! \param[in] name The name of the new \a node to add to the \a scene.
        //! \param[in] parent_node An optional \a uid of the parent node to add the new \a node to.
        //! \return The \a uid referencing the added \a node.
        virtual uid add_node(const string& name, uid parent_node = invalid_uid) = 0;

        //! \brief Adds a \a perspective_camera to the \a scene.
        //! \param[in] new_perspective_camera The \a perspective_camera to add to the \a scene.
        //! \param[in] node_id The \a uid of the \a node that should contain the \a perspective_camera.
        //! \return The node \a uid referencing the added \a perspective_camera.
        virtual uid add_perspective_camera(perspective_camera& new_perspective_camera, uid node_id) = 0;

        //! \brief Adds an \a orthographic_camera to the \a scene.
        //! \param[in] new_orthographic_camera The \a orthographic_camera to add to the \a scene.
        //! \param[in] node_id The \a uid of the \a node that should contain the \a orthographic_camera.
        //! \return The node \a uid referencing the added \a orthographic_camera.
        virtual uid add_orthographic_camera(orthographic_camera& new_orthographic_camera, uid node_id) = 0;

        // virtual uid add_mesh(mesh& new_mesh, uid node_id) = 0;

        //! \brief Adds a \a directional_light to the \a scene.
        //! \param[in] new_directional_light The \a directional_light to add to the \a scene.
        //! \param[in] node_id The \a uid of the \a node that should contain the \a directional_light.
        //! \return The node \a uid referencing the added \a directional_light.
        virtual uid add_directional_light(directional_light& new_directional_light, uid node_id) = 0;

        //! \brief Adds a \a skylight to the \a scene.
        //! \param[in] new_skylight The \a skylight to add to the \a scene.
        //! \param[in] node_id The \a uid of the \a node that should contain the \a skylight.
        //! \return The node \a uid referencing the added \a skylight.
        virtual uid add_skylight(skylight& new_skylight, uid node_id) = 0;

        //! \brief Adds a \a atmospheric_light to the \a scene.
        //! \param[in] new_atmospheric_light The \a atmospheric_light to add to the \a scene.
        //! \param[in] node_id The \a uid of the \a node that should contain the \a atmospheric_light.
        //! \return The node \a uid referencing the added \a atmospheric_light.
        virtual uid add_atmospheric_light(atmospheric_light& new_atmospheric_light, uid node_id) = 0;

        //! \brief Builds a \a material.
        //! \param[in] new_material The \a material to build.
        //! \return The \a uid of the created \a material.
        virtual uid build_material(material& new_material) = 0;

        //! \brief Loads an image and creates a \a texture.
        //! \param[in] path The full path to the image to load.
        //! \param[in] standard_color_space True if the image should be loaded in standard color space, else false.
        //! \param[in] high_dynamic_range True if the image should be loaded as high dynamic range, else false.
        //! \return The node \a uid of the created \a texture.
        virtual uid load_texture_from_image(const string& path, bool standard_color_space, bool high_dynamic_range) = 0;

        //! \brief Loads a \a model loaded from a gltf file.
        //! \details Only loads. Does not add anything into the scene but the data.
        //! \param[in] path The path to the gltf model to load.
        //! \return The \a uid of the created \a model.
        virtual uid load_model_from_gltf(const string& path) = 0;

        //! \brief Adds a \a model to the \a scene.
        //! \param[in] model_to_add The \a uid of the \a model to add.
        //! \param[in] scenario_id The \a uid of the \a scenario from the \a model to add.
        //! \param[in] node_id The \a uid of the \a node that should contain the \a model.
        virtual void add_model_to_scene(uid model_to_add, uid scenario_id, uid node_id) = 0;

        //! \brief Creates a \a skylight from a hdr image.
        //! The environment texture is preprocessed, prefiltered and can be rendered as a cube.
        //! \param[in] path The path to the hdr image to load.
        //! \param[in] node_id The \a uid of the \a node that should contain the \a skylight.
        //! \return The \a uid referencing the added \a skylight.
        virtual uid add_skylight_from_hdr(const string& path, uid node_id) = 0;

        //! \brief Removes a \a node from the \a scene.
        //! \param[in] node_id The \a uid of the \a node to remove from the \a scene.
        virtual void remove_node(uid node_id) = 0;

        //! \brief Removes a \a perspective_camera from the \a scene.
        //! \param[in] node_id The \a uid of the containing \a node of the \a perspective_camera to remove from the \a scene.
        virtual void remove_perspective_camera(uid node_id) = 0;

        //! \brief Removes a \a orthographic_camera from the \a scene.
        //! \param[in] node_id The \a uid of the containing \a node of the \a orthographic_camera to remove from the \a scene.
        virtual void remove_orthographic_camera(uid node_id) = 0;

        // virtual void remove_mesh(uid node_id) = 0;

        //! \brief Removes a \a directional_light from the \a scene.
        //! \param[in] node_id The \a uid of the containing \a node of the \a directional_light to remove from the \a scene.
        virtual void remove_directional_light(uid node_id) = 0;

        //! \brief Removes a \a skylight from the \a scene.
        //! \param[in] node_id The \a uid of the containing \a node of the \a skylight to remove from the \a scene.
        virtual void remove_skylight(uid node_id) = 0;

        //! \brief Removes a \a atmospheric_light from the \a scene.
        //! \param[in] node_id The \a uid of the containing \a node of the \a atmospheric_light to remove from the \a scene.
        virtual void remove_atmospheric_light(uid node_id) = 0;

        //! \brief Unloads a \a model loaded from a gltf file.
        //! \details This should only be called, when every instance is removed from the scene, since it corrupts children at the moment.
        //! \param[in] model_id The \a uid of the loaded model to remove.
        virtual void unload_gltf_model(uid model_id) = 0;

        //! \brief Retrieves a \a node from the \a scene.
        //! \param[in] node_id The \a uid of the \a node to retrieve from the \a scene.
        //! \return An optional \a node reference.
        virtual optional<node&> get_node(uid node_id) = 0;

        //! \brief Retrieves a \a transform from the \a scene.
        //! \param[in] node_id The \a uid of the containing \a node of the \a transform to retrieve from the \a scene.
        //! \return An optional \a transform reference.
        virtual optional<transform&> get_transform(uid node_id) = 0;

        //! \brief Retrieves a \a perspective_camera from the \a scene.
        //! \param[in] node_id The \a uid of the containing \a node of the \a perspective_camera to retrieve from the \a scene.
        //! \return An optional \a perspective_camera reference.
        virtual optional<perspective_camera&> get_perspective_camera(uid node_id) = 0;

        //! \brief Retrieves a \a orthographic_camera from the \a scene.
        //! \param[in] node_id The \a uid of the containing \a node of the \a orthographic_camera to retrieve from the \a scene.
        //! \return An optional \a orthographic_camera reference.
        virtual optional<orthographic_camera&> get_orthographic_camera(uid node_id) = 0;

        // virtual optionsl<> get_mesh_primitive(uid node_id) = 0;

        //! \brief Retrieves a \a directional_light from the \a scene.
        //! \param[in] node_id The \a uid of the containing \a node of the \a directional_light to retrieve from the \a scene.
        //! \return An optional \a directional_light reference.
        virtual optional<directional_light&> get_directional_light(uid node_id) = 0;

        //! \brief Retrieves a \a skylight from the \a scene.
        //! \param[in] node_id The \a uid of the containing \a node of the \a skylight to retrieve from the \a scene.
        //! \return An optional \a skylight reference.
        virtual optional<skylight&> get_skylight(uid node_id) = 0;

        //! \brief Retrieves a \a atmospheric_light from the \a scene.
        //! \param[in] node_id The \a uid of the containing \a node of the \a atmospheric_light to retrieve from the \a scene.
        //! \return An optional \a atmospheric_light reference.
        virtual optional<atmospheric_light&> get_atmospheric_light(uid node_id) = 0;

        //! \brief Retrieves a \a model from the \a scene.
        //! \param[in] instance_id The \a uid of the \a model instance to retrieve.
        //! \return An optional \a model reference.
        virtual optional<model&> get_model(uid instance_id) = 0;

        //! \brief Retrieves a \a mesh.
        //! \param[in] instance_id The \a uid of the \a mesh instance to retrieve.
        //! \return An optional \a mesh reference.
        virtual optional<mesh&> get_mesh(uid instance_id) = 0;

        //! \brief Retrieves a \a material.
        //! \param[in] instance_id The \a uid of the \a material instance to retrieve.
        //! \return An optional \a material reference.
        virtual optional<material&> get_material(uid instance_id) = 0;

        //! \brief Retrieves a \a texture.
        //! \param[in] instance_id The \a uid of the \a texture instance to retrieve.
        //! \return An optional \a texture reference.
        virtual optional<texture&> get_texture(uid instance_id) = 0;

        //! \brief Retrieves the root \a node \a uid of the \a scene.
        //! \return The \a uid of the root \a node of the scene graph.
        virtual uid get_root_node() = 0;

        //! \brief Retrieves the \a uid of the \a node holding the active camera of the \a scene.
        //! \return The \a uid of the \a node holding the active camera of the \a scene.
        virtual uid get_active_camera_uid() = 0;

        //! \brief Sets the active camera of the \a scene.
        //! \param[in] node_id The \a uid of the \a node holding the camera to set to the active one in the \a scene.
        virtual void set_main_camera(uid node_id) = 0;

        // virtual bool is_object_alive(uid node_id) = 0;

        //! \brief Attach a \a node to another one in a child <-> parent relationship.
        //! \details Used for building hierarchies.
        //! \param[in] child_node The \a uid of the \a node to use as a child.
        //! \param[in] parent_node The \a uid of the \a node to use as a parent.
        virtual void attach(uid child_node, uid parent_node) = 0;

        //! \brief Detach a \a node from the parent.
        //! \details Attaches the detached node to the root node.
        //! \param[in] child_node The \a uid of the \a node used as a child.
        //! \param[in] parent_node The \a uid of the \a node used as a parent.
        virtual void detach(uid child_node, uid parent_node) = 0;
    };

    //! \brief A unique pointer holding a \a scene.
    using scene_ptr = std::unique_ptr<scene>;

    //! \brief A pointer pointing to a \a scene.
    using scene_handle = scene*;
} // namespace mango

#endif // MANGO_SCENE_HPP
