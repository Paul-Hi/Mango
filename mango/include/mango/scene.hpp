//! \file      scene.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_SCENE_HPP
#define MANGO_SCENE_HPP

#include <mango/scene_component_pool.hpp>
#include <mango/scene_ecs.hpp>
#include <map>
#include <queue>

namespace tinygltf
{
    class Model;
    class Node;
    struct Mesh;
    struct Primitive;
    struct Camera;
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
        //! \details Internally creates entities with \a model_components, \a mesh_components, \a material_components, \a primitive_components, \a transform_components and \a node_components.
        //! All the components are filled with data loaded from the gltf file.
        //! \param[in] path The path to the gltf model to load.
        //! \param[in] gltf_root If there is already a entity created to work as root it should be passed here.
        //! \return The root entity of the model.
        entity create_entities_from_model(const string& path, entity gltf_root = invalid_entity);

        //! \brief Creates an environment entity.
        //! \details An entity with \a environment_component.
        //! The environment texture is preprocessed, prefiltered and can be rendered as a cube. This is done with a \a pipeline_step.
        //! \param[in] path The path to the hdr image to load.
        //! \return The created environment entity.
        entity create_environment_from_hdr(const string& path);

        //! \brief Attach an \a entity to another entity in a child <-> parent realationship.
        //! \details Adds a \a node_component. Used for building hierarchies.
        //! \param[in] child The \a entity used as a child.
        //! \param[in] parent The \a entity used as a parent.
        void attach(entity child, entity parent);

        //! \brief Detach an \a entity from the parent.
        //! \details Removes the parent, the component may still exits, if the node has children.
        //! \param[in] node The \a entity to detach.
        void detach(entity node);

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

        //! \brief Retrieves the \a model_component from a specific \a entity.
        //! \param[in] e The \a entity to get the \a model_component for.
        //! \return The \a model_component or nullptr if non-existent.
        inline model_component* get_model_component(entity e)
        {
            return m_models.get_component_for_entity(e);
        }

        //! \brief Retrieves the \a mesh_component from a specific \a entity.
        //! \param[in] e The \a entity to get the \a mesh_component for.
        //! \return The \a mesh_component or nullptr if non-existent.
        inline mesh_component* get_mesh_component(entity e)
        {
            return m_meshes.get_component_for_entity(e);
        }

        //! \brief Retrieves the \a tag_component from a specific \a entity.
        //! \param[in] e The \a entity to get the \a tag_component for.
        //! \return The \a tag_component or nullptr if non-existent.
        inline tag_component* get_tag(entity e)
        {
            return m_tags.get_component_for_entity(e);
        }

        //! \brief Retrieves the \a environment_component from a specific \a entity.
        //! \param[in] e The \a entity to get the \a environment_component for.
        //! \return The \a environment_component or nullptr if non-existent.
        inline environment_component* get_environment_component(entity e)
        {
            return m_environments.get_component_for_entity(e);
        }

        //! \brief Retrieves the \a light_component from a specific \a entity.
        //! \param[in] e The \a entity to get the \a light_component for.
        //! \return The \a light_component or nullptr if non-existent.
        inline light_component* get_light_component(entity e)
        {
            return m_lights.get_component_for_entity(e);
        }

        //! \brief Queries the \a transform_component from a specific \a entity.
        //! \details Does the same as get, but is non verbose, when component is non existent.
        //! \param[in] e The \a entity to get the \a transform_component for.
        //! \return The \a transform_component or nullptr if non-existent.
        inline transform_component* query_transform_component(entity e)
        {
            return m_transformations.get_component_for_entity(e, true);
        }

        //! \brief Queries the \a camera_component from a specific \a entity.
        //! \details Does the same as get, but is non verbose, when component is non existent.
        //! \param[in] e The \a entity to get the \a camera_component for.
        //! \return The \a camera_component or nullptr if non-existent.
        inline camera_component* query_camera_component(entity e)
        {
            return m_cameras.get_component_for_entity(e, true);
        }

        //! \brief Queries the \a model_component from a specific \a entity.
        //! \details Does the same as get, but is non verbose, when component is non existent.
        //! \param[in] e The \a entity to get the \a model_component for.
        //! \return The \a model_component or nullptr if non-existent.
        inline model_component* query_model_component(entity e)
        {
            return m_models.get_component_for_entity(e, true);
        }

        //! \brief Queries the \a mesh_component from a specific \a entity.
        //! \details Does the same as get, but is non verbose, when component is non existent.
        //! \param[in] e The \a entity to get the \a mesh_component for.
        //! \return The \a mesh_component or nullptr if non-existent.
        inline mesh_component* query_mesh_component(entity e)
        {
            return m_meshes.get_component_for_entity(e, true);
        }

        //! \brief Queries the \a tag_component from a specific \a entity.
        //! \details Does the same as get, but is non verbose, when component is non existent.
        //! \param[in] e The \a entity to get the \a tag_component for.
        //! \return The \a tag_component or nullptr if non-existent.
        inline tag_component* query_tag(entity e)
        {
            return m_tags.get_component_for_entity(e, true);
        }

        //! \brief Queries the \a environment_component from a specific \a entity.
        //! \details Does the same as get, but is non verbose, when component is non existent.
        //! \param[in] e The \a entity to get the \a environment_component for.
        //! \return The \a environment_component or nullptr if non-existent.
        inline environment_component* query_environment_component(entity e)
        {
            return m_environments.get_component_for_entity(e, true);
        }

        //! \brief Queries the \a light_component from a specific \a entity.
        //! \details Does the same as get, but is non verbose, when component is non existent.
        //! \param[in] e The \a entity to get the \a light_component for.
        //! \return The \a light_component or nullptr if non-existent.
        inline light_component* query_light_component(entity e)
        {
            return m_lights.get_component_for_entity(e, true);
        }

        //! \brief Adds \a transform_component to a specific \a entity.
        //! \param[in] e The \a entity to add the \a transform_component to.
        //! \return A reference to the created \a transform_component.
        inline transform_component& add_transform_component(entity e)
        {
            return m_transformations.create_component_for(e);
        }

        //! \brief Adds \a camera_component to a specific \a entity.
        //! \param[in] e The \a entity to add the \a camera_component to.
        //! \return A reference to the created \a camera_component.
        inline camera_component& add_camera_component(entity e)
        {
            return m_cameras.create_component_for(e);
        }

        //! \brief Adds \a model_component to a specific \a entity.
        //! \param[in] e The \a entity to add the \a model_component to.
        //! \return A reference to the created \a model_component.
        inline model_component& add_model_component(entity e)
        {
            return m_models.create_component_for(e);
        }

        //! \brief Adds \a mesh_component to a specific \a entity.
        //! \param[in] e The \a entity to add the \a mesh_component to.
        //! \return A reference to the created \a mesh_component.
        inline mesh_component& add_mesh_component(entity e)
        {
            return m_meshes.create_component_for(e);
        }

        //! \brief Adds \a tag_component to a specific \a entity.
        //! \param[in] e The \a entity to add the \a tag_component to.
        //! \return A reference to the created \a tag_component.
        inline tag_component& add_tag(entity e)
        {
            return m_tags.create_component_for(e);
        }

        //! \brief Adds \a environment_component to a specific \a entity.
        //! \param[in] e The \a entity to add the \a environment_component to.
        //! \return A reference to the created \a environment_component.
        inline environment_component& add_environment_component(entity e)
        {
            return m_environments.create_component_for(e);
        }

        //! \brief Adds \a light_component to a specific \a entity.
        //! \param[in] e The \a entity to add the \a light_component to.
        //! \return A reference to the created \a light_component.
        inline light_component& add_light_component(entity e)
        {
            return m_lights.create_component_for(e);
        }

        //! \brief Removes \a transform_component from a specific \a entity.
        //! \param[in] e The \a entity to remove the \a transform_component from.
        inline void remove_transform_component(entity e)
        {
            m_transformations.remove_component_from(e);
        }

        //! \brief Removes \a camera_component from a specific \a entity.
        //! \param[in] e The \a entity to remove the \a camera_component from.
        inline void remove_camera_component(entity e)
        {
            m_cameras.remove_component_from(e);
        }

        //! \brief Removes \a model_component from a specific \a entity.
        //! \param[in] e The \a entity to remove the \a model_component from.
        inline void remove_model_component(entity e)
        {
            m_models.remove_component_from(e);
        }

        //! \brief Removes \a mesh_component from a specific \a entity.
        //! \param[in] e The \a entity to remove the \a mesh_component from.
        inline void remove_mesh_component(entity e)
        {
            m_meshes.remove_component_from(e);
        }

        //! \brief Removes \a tag_component from a specific \a entity.
        //! \param[in] e The \a entity to remove the \a tag_component from.
        inline void remove_tag(entity e)
        {
            m_tags.remove_component_from(e);
        }

        //! \brief Removes \a environment_component from a specific \a entity.
        //! \param[in] e The \a entity to remove the \a environment_component from.
        inline void remove_environment_component(entity e)
        {
            m_environments.remove_component_from(e);
        }

        //! \brief Removes \a light_component from a specific \a entity.
        //! \param[in] e The \a entity to remove the \a light_component from.
        inline void remove_light_component(entity e)
        {
            m_lights.remove_component_from(e);
        }

        //! \brief Retrieves the \a camera_data for the currently active camera.
        //! \details Has to be checked if pointers are null. Also can only be used for a short time.
        //! \return The \a camera_data.
        inline camera_data get_active_camera_data()
        {
            camera_data result;
            result.active_camera_entity = m_active.camera;
            if (m_active.camera == invalid_entity)
            {
                result.camera_info = nullptr;
                result.transform   = nullptr;
                return result;
            }
            result.camera_info = m_cameras.get_component_for_entity(m_active.camera);
            result.transform   = m_transformations.get_component_for_entity(m_active.camera);
            return result;
        }

        //! \brief Sets the active camera to an \a entity.
        //! \param[in] e The \a entity to set the active camera to.
        inline void set_active_camera(entity e)
        {
            auto next_comp = m_cameras.get_component_for_entity(e);
            if (!next_comp)
                return;

            m_active.camera = e;
        }

        //! \brief Retrieves the \a environment_data for the currently active environment.
        //! \details Has to be checked if pointers are null. Also can only be used for a short time.
        //! \return The \a environment_data.
        inline environment_data get_active_environment_data()
        {
            environment_data result;
            result.active_environment_entity = m_active.environment;
            if (m_active.environment == invalid_entity)
            {
                result.environment_info = nullptr;
                return result;
            }
            result.environment_info = m_environments.get_component_for_entity(m_active.environment);
            return result;
        }

        //! \brief Sets the active environment to an \a entity.
        //! \param[in] e The \a entity to set the active environment to.
        void set_active_environment(entity e);

        //! \brief Retrieves the \a scene root \a entity.
        //! \return The \a scene root \a entity.
        inline entity get_root()
        {
            return m_root_entity;
        }

        //! \brief Retrieves and returns all children of an \a entity.
        //! \param[in] e The \a entity to retrieve the children for.
        //! \returns A list of children.
        inline std::vector<entity> get_children(entity e)
        {
            std::vector<entity> children;
            auto node = m_nodes.get_component_for_entity(e);
            if (!node)
                return children;
            auto child_entity = node->child_entities;
            for (int32 i = 0; i < node->children_count; ++i)
            {
                children.push_back(child_entity);
                auto child_node = m_nodes.get_component_for_entity(child_entity);
                child_entity    = child_node->next_sibling;
            }
            return children;
        }

        //! \brief Checks if the \a entity is still alive.
        //! \details This is temporary and has to be fixed soon, because it is not always correct.
        //! \return True if \a entity is still alive, else False.
        inline bool is_entity_alive(entity e)
        {
            return std::find(m_free_entities.begin(), m_free_entities.end(), e) == m_free_entities.end(); // TODO Paul: This is not correct, need entity versions.
        }

      private:
        friend class application; // TODO Paul: We maybe should handle this without a friend.
        //! \brief Updates the \a scene.
        //! \details Includes all system functions and reactions to input.
        //! \param[in] dt Past time since last call.
        void update(float dt);

        //! \brief Renders the \a scene.
        //! \details Retrieves all relevant \a render_commands from different components and submits them to the \a render_system.
        void render();

        //! \brief Builds one or more entities that describe an entire model with data loaded by tinygltf.
        //! \details Internally called by create_entities_from_model(...).
        //! This also creates the hierarchy incl. \a transform_components and \a node_components.
        //! \param[in] m The model loaded by tinygltf.
        //! \param[in] n The node loaded by tinygltf.
        //! \param[in] parent_world The parents world transformation matrix.
        //! \param[in] buffer_map The mapped buffers of the model.
        //! \return The root node of the function call.
        entity build_model_node(tinygltf::Model& m, tinygltf::Node& n, const glm::mat4& parent_world, const std::map<int, shared_ptr<buffer>>& buffer_map);

        //! \brief Attaches a \a mesh_component to an \a entity with data loaded by tinygltf.
        //! \details Internally called by create_entities_from_model(...).
        //! \param[in] node The entity that the \a mesh_component should be attached to.
        //! \param[in] m The model loaded by tinygltf.
        //! \param[in] mesh The mesh loaded by tinygltf.
        //! \param[in] buffer_map The mapped buffers of the model.
        void build_model_mesh(entity node, tinygltf::Model& m, tinygltf::Mesh& mesh, const std::map<int, shared_ptr<buffer>>& buffer_map);

        //! \brief Attaches a \a camera_component to an \a entity with data loaded by tinygltf.
        //! \details Internally called by create_entities_from_model(...).
        //! \param[in] node The entity that the \a mesh_component should be attached to.
        //! \param[in] camera The camera loaded by tinygltf.
        void build_model_camera(entity node, tinygltf::Camera& camera);

        //! \brief Loads a \a material and stores it in the component.
        //! \details Loads all supported component values and textures if they exist.
        //! \param[out] material The component to store the material in.
        //! \param[in] primitive The tinygltf primitive the material is linked to.
        //! \param[in] m The model loaded by tinygltf.
        void load_material(material_component& material, const tinygltf::Primitive& primitive, tinygltf::Model& m);

        //! \brief Detach an \a entity from the parent and the children.
        //! \details Removes the \a node_component.
        //! \param[in] node The \a entity to delete the node from.
        void delete_node(entity node);

        friend class context_impl; // TODO Paul: Could this be avoided?
        //! \brief Mangos internal context for shared usage in all \a render_systems.
        shared_ptr<context_impl> m_shared_context;

        //! \brief All free entity ids.
        std::deque<uint32> m_free_entities;

        //! \brief All \a tag_components.
        scene_component_pool<tag_component> m_tags;
        //! \brief All \a node_components.
        scene_component_pool<node_component> m_nodes;
        //! \brief All \a transform_components.
        scene_component_pool<transform_component> m_transformations;
        //! \brief All \a model_components.
        scene_component_pool<model_component> m_models;
        //! \brief All \a mesh_components.
        scene_component_pool<mesh_component> m_meshes;
        //! \brief All \a camera_components.
        scene_component_pool<camera_component> m_cameras;
        //! \brief All \a environment_components. There is only one unique at the moment.
        scene_component_pool<environment_component> m_environments;
        //! \brief All \a light_components.
        scene_component_pool<light_component> m_lights;
        //! \brief The current root entity of the scene.
        entity m_root_entity;

        struct
        {
            //! \brief The currently active camera entity.
            entity camera;
            //! \brief The currently active environment entity.
            entity environment;
        } m_active; //!< Storage for active scene singleton entities.

        //! \brief Scene boundaries.
        struct scene_bounds
        {
            glm::vec3 min;    //!< Minimum geometry values.
            glm::vec3 max;    //!< Maximum geometry values.
        } m_scene_boundaries; //!< The boundaries of the current scene.
    };

} // namespace mango

#endif // MANGO_SCENE_HPP
