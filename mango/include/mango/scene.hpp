//! \file      scene.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_SCENE_HPP
#define MANGO_SCENE_HPP

#include <mango/scene_component_manager.hpp>
#include <mango/scene_types.hpp>
#include <map>
#include <queue>

namespace tinygltf
{
    struct Model;
    struct Node;
    struct Mesh;
    struct Primitive;
} // namespace tinygltf

namespace mango
{
    class context_impl;
    class shader_program;
    class buffer;
    //! \brief The \a scene of mango.
    //! \details A collection of entities, components and systems. Responsible for handling content in mango.
    class scene
    {
      public:
        //! \brief Constructs a new \a scene with a \a name.
        //! \param[in] name The name of the new \a scene.
        scene(const string& name);
        ~scene();

        //! \brief Updates the \a scene.
        //! \details Includes all system functions and reactions to input.
        //! \param[in] dt Past time since last call.
        void update(float dt);

        //! \brief Renders the \a scene.
        //! \details Retrieves all relevant \a render_commands from different components and submits them to the \a render_system.
        void render();

        //! \brief Creates an empty entity with no components.
        entity create_empty();

        //! \brief Removes an \a entity.
        //! \details Removes all components.
        //! \param[in] e The \a entity to remove.
        void remove_entity(entity e);

        //! \brief Creates a camera entity.
        //! \details An entity with \a camera_component and \a transform_component.
        //! All the components are prefilled. Camera has a perspective projection.
        //! \return The created camera entity.
        entity create_default_camera();

        //! \brief Creates entities from a model loaded from a gltf file.
        //! \details Internally creates entities with \a mesh_components, \a material_components, \a transform_components and \a node_components.
        //! All the components are filled with data loaded from the gltf file.
        //! \param[in] path The path to the gltf model to load.
        //! \return A list of all created entities.
        std::vector<entity> create_entities_from_model(const string& path);

        //! \brief Creates an environment entity.
        //! \details An entity with \a environment_component.
        //! The environment texture is preprocessed, prefiltered and can be rendered as a cube. This is done with a \a pipeline_step.
        //! \param[in] path The path to the hdr image to load.
        //! \param[in] rendered_mip_level The mipmap level to render the texture. -1 if no visualization is wanted.
        //! \return The created environment entity.
        entity create_environment_from_hdr(const string& path, float rendered_mip_level);

        //! \brief Attach an \a entity to another entity in a child <-> parent realationship.
        //! \details Adds a \a node_component. This is mostly useful to describe child objects inheriting the parents transform.
        //! \param[in] child The \a entity used as a child.
        //! \param[in] parent The \a entity used as a parent.
        void attach(entity child, entity parent);

        //! \brief Detach an \a entity.
        //! \details Removes the \a node_component. Parent relation is lost.
        //! \param[in] child The \a entity to detach.
        void detach(entity child);

        //! \brief Retrieves the \a transform_component from a specific \a entity.
        //! \param[in] e The \a entity to get the \a transform_component for.
        //! \return The \a transform_component or nullptr if non-existent.
        inline transform_component* get_transform_component(entity e)
        {
            return m_transformations.get_component_for_entity(e);
        }

        //! \brief Retrieves the \a camera_component from a specific \a entity.
        //! \param[in] e The \a entity to get the \a camera_component for.
        //! \return The \a camera_component or nullptr if non-existent.
        inline camera_component* get_camera_component(entity e)
        {
            return m_cameras.get_component_for_entity(e);
        }

        //! \brief Retrieves the \a camera_data for the currently active camera.
        //! \details Has to be checked if pointer are null. Also can only be used for a short time.
        //! \return The \a camera_data.
        inline camera_data get_active_camera_data()
        {
            camera_data result;
            result.camera_info = m_cameras.get_component_for_entity(m_active_camera);
            result.transform   = m_transformations.get_component_for_entity(m_active_camera);
            return result;
        }

      private:
        //! \brief Builds one or more entities that describe an entire model with data loaded by tinygltf.
        //! \details Internally called by create_entities_from_model(...).
        //! This also creates the hierarchy incl. \a transform_components and \a node_components.
        //! \param[in] entities The entity array where all entities are inserted.
        //! \param[in] m The model loaded by tinygltf.
        //! \param[in] n The node loaded by tinygltf.
        //! \param[in] parent_world The parents world transformation matrix.
        //! \param[in] buffer_map The mapped buffers of the model.
        //! \return The root node of the function call.
        entity build_model_node(std::vector<entity>& entities, tinygltf::Model& m, tinygltf::Node& n, const glm::mat4& parent_world, const std::map<int, shared_ptr<buffer>>& buffer_map);

        //! \brief Attaches a \a mesh_component to an \a entity with data loaded by tinygltf.
        //! \details Internally called by create_entities_from_model(...).
        //! \param[in] node The entity that the \a mesh_component should be attached to.
        //! \param[in] m The model loaded by tinygltf.
        //! \param[in] mesh The mesh loaded by tinygltf.
        //! \param[in] buffer_map The mapped buffers of the model.
        void build_model_mesh(entity node, tinygltf::Model& m, tinygltf::Mesh& mesh, const std::map<int, shared_ptr<buffer>>& buffer_map);

        //! \brief Loads a \a material and stores it in the component.
        //! \details Loads all supported component values and textures if they exist.
        //! \param[out] material The component to store the material in.
        //! \param[in] primitive The tinygltf primitive the material is linked to.
        //! \param[in] m The model loaded by tinygltf.
        void load_material(material_component& material, const tinygltf::Primitive& primitive, tinygltf::Model& m);

        friend class context_impl; // TODO Paul: Could this be avoided?
        //! \brief Mangos internal context for shared usage in all \a render_systems.
        shared_ptr<context_impl> m_shared_context;

        //! \brief All free entities.
        std::queue<entity> m_free_entities;
        //! \brief All \a node_components.
        scene_component_manager<node_component> m_nodes;
        //! \brief All \a transform_components.
        scene_component_manager<transform_component> m_transformations;
        //! \brief All \a mesh_components.
        scene_component_manager<mesh_component> m_meshes;
        //! \brief All \a camera_components.
        scene_component_manager<camera_component> m_cameras;
        //! \brief All \a environment_components. There is only one unique at the moment.
        scene_component_manager<environment_component> m_environments;
        //! \brief The currently active camera entity.
        entity m_active_camera;

        //! \brief Scene boundaries.
        struct scene_bounds
        {
            glm::vec3 min; //!< Minimum geometry values.
            glm::vec3 max; //!< Maximum geometry values.
        } m_scene_boundaries; //!< The boundaries of the current scene.
    };

} // namespace mango

#endif // MANGO_SCENE_HPP
