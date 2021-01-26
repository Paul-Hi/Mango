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
        entity create_default_scene_camera();

        //! \brief Creates a camera entity not attached to the scene.
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
        //! \details An entity with \a environment_component. ATTENTION: This creates the \a light_data and fills it. Do not recreate it.
        //! The environment texture is preprocessed, prefiltered and can be rendered as a cube. This is done with a \a pipeline_step.
        //! \param[in] path The path to the hdr image to load.
        //! \return The created environment entity.
        entity create_skylight_from_hdr(const string& path);

        // //! \brief Creates an environment entity.
        // //! \details An entity with \a environment_component. ATTENTION: This creates the \a light_data and fills it. Do not recreate it.
        // //! The atmosphere is generated, preprocessed, prefiltered and can be rendered as a cube. This is done with a \a pipeline_step.
        // //! \param[in] sun_direction The sun direction to use or vec3(-1.0f) if renderer should choose the sun.
        // //! \param[in] sun_intensity The sun intensity to use or -1.0f if renderer should choose the sun.
        // //! \return The created environment entity.
        // entity create_atmospheric_environment(const glm::vec3& sun_direction = glm::vec3(-1.0f), float sun_intensity = -1.0f);

        //! \brief Attach an \a entity to another entity in a child <-> parent realationship.
        //! \details Adds a \a node_component. Used for building hierarchies.
        //! \param[in] child The \a entity used as a child.
        //! \param[in] parent The \a entity used as a parent.
        void attach(entity child, entity parent);

        //! \brief Detach an \a entity from the parent.
        //! \details Removes the parent, the component may still exits, if the node has children.
        //! \param[in] node The \a entity to detach.
        void detach(entity node);

        //! \brief Retrieves a \a component from a specific \a entity.
        //! \details Should NOT be stored for a long time period.
        //! \param[in] e The \a entity to get the \a component for.
        //! \return A pointer to the \a component or nullptr if non-existent.
        template <typename comp>
        inline comp* get_component(entity e)
        {
            switch (type_name<comp>::id())
            {
            case 0:
                return (comp*)m_tags.get_component_for_entity(e);
            case 1:
                return (comp*)m_transformations.get_component_for_entity(e);
            case 2:
                return (comp*)m_nodes.get_component_for_entity(e);
            case 3:
                return (comp*)m_mesh_primitives.get_component_for_entity(e);
            case 4:
                return (comp*)m_materials.get_component_for_entity(e);
            case 5:
                return (comp*)m_models.get_component_for_entity(e);
            case 6:
                return (comp*)m_cameras.get_component_for_entity(e);
            case 7:
                return (comp*)m_directional_lights.get_component_for_entity(e);
            case 8:
                return (comp*)m_atmosphere_lights.get_component_for_entity(e);
            case 9:
                return (comp*)m_skylights.get_component_for_entity(e);
            default:
                MANGO_LOG_ERROR("No component id matches the component!");
                return nullptr;
            }
        }

        //! \brief Queries a \a component from a specific \a entity.
        //! \details Does the same as get, but is non verbose, when component is non existent.
        //! \details Should NOT be stored for a long time period.
        //! \param[in] e The \a entity to get the \a component for.
        //! \return A pointer to the \a component or nullptr if non-existent.
        template <typename comp>
        inline comp* query_component(entity e)
        {
            switch (type_name<comp>::id())
            {
            case 0:
                return (comp*)m_tags.get_component_for_entity(e, true);
            case 1:
                return (comp*)m_transformations.get_component_for_entity(e, true);
            case 2:
                return (comp*)m_nodes.get_component_for_entity(e, true);
            case 3:
                return (comp*)m_mesh_primitives.get_component_for_entity(e, true);
            case 4:
                return (comp*)m_materials.get_component_for_entity(e, true);
            case 5:
                return (comp*)m_models.get_component_for_entity(e, true);
            case 6:
                return (comp*)m_cameras.get_component_for_entity(e, true);
            case 7:
                return (comp*)m_directional_lights.get_component_for_entity(e, true);
            case 8:
                return (comp*)m_atmosphere_lights.get_component_for_entity(e, true);
            case 9:
                return (comp*)m_skylights.get_component_for_entity(e, true);
            default:
                MANGO_LOG_ERROR("No component id matches the component!");
                return nullptr;
            }
        }

        //! \brief Adds a \a component to a specific \a entity.
        //! \details Should NOT be stored for a long time period.
        //! \param[in] e The \a entity to add the \a component to.
        //! \return A pointer to the created \a component or nullptr if non-existent.
        template <typename comp>
        inline comp* add_component(entity e)
        {
            switch (type_name<comp>::id())
            {
            case 0:
                return (comp*)&m_tags.create_component_for(e);
            case 1:
                return (comp*)&m_transformations.create_component_for(e);
            case 2:
                return (comp*)&m_nodes.create_component_for(e);
            case 3:
                return (comp*)&m_mesh_primitives.create_component_for(e);
            case 4:
                return (comp*)&m_materials.create_component_for(e);
            case 5:
                return (comp*)&m_models.create_component_for(e);
            case 6:
                return (comp*)&m_cameras.create_component_for(e);
            case 7:
                return (comp*)&m_directional_lights.create_component_for(e);
            case 8:
                return (comp*)&m_atmosphere_lights.create_component_for(e);
            case 9:
                return (comp*)&m_skylights.create_component_for(e);
            default:
                MANGO_LOG_CRITICAL("No component id matches the component!");
                return nullptr;
            }
        }

        //! \brief Removes a \a component from a specific \a entity.
        //! \param[in] e The \a entity to remove the \a component from.
        template <typename comp>
        inline void remove_component(entity e)
        {
            auto children = get_children(e);
            switch (type_name<comp>::id())
            {
            case 0:
                m_tags.remove_component_from(e);
                return;
            case 1:
                m_transformations.remove_component_from(e);
                return;
            case 2:
                m_nodes.remove_component_from(e);
                return;
            case 3:
                m_mesh_primitives.remove_component_from(e);
                return;
            case 4:
                m_materials.remove_component_from(e);
                return;
            case 5:
                m_models.remove_component_from(e);
                for (auto child : children) // TODO Paul: Removes all children at the moment.
                {
                    remove_entity(child);
                }
                return;
            case 6:
                m_cameras.remove_component_from(e);
                return;
            case 7:
                m_directional_lights.remove_component_from(e);
                return;
            case 8:
                m_atmosphere_lights.remove_component_from(e);
                return;
            case 9:
                m_skylights.remove_component_from(e);
                return;
            default:
                MANGO_LOG_ERROR("No component id matches the component!");
                return;
            }
        }

        //! \brief Retrieves the \a camera_data for the currently active camera.
        //! \details Has to be checked if pointers are null. Also can only be used for a short time.
        //! \return The \a camera_data.
        inline camera_data get_active_camera_data()
        {
            camera_data result;
            result.active_camera_entity = m_active.camera;
            result.camera_info          = m_cameras.get_component_for_entity(m_active.camera, true);
            if (m_active.camera == invalid_entity || !result.camera_info)
            {
                result.active_camera_entity = invalid_entity;
                result.camera_info          = nullptr;
                result.transform            = nullptr;
                return result;
            }
            result.transform = m_transformations.get_component_for_entity(m_active.camera, true);
            return result;
        }

        //! \brief Sets the active camera to an \a entity.
        //! \param[in] e The \a entity to set the active camera to.
        inline void set_active_camera(entity e)
        {
            if (e == invalid_entity)
            {
                m_active.camera = e;
                return;
            }
            auto next_comp = m_cameras.get_component_for_entity(e);
            if (!next_comp)
                return;

            m_active.camera = e;
        }

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
        //! \brief All \a mesh_primitive_components.
        scene_component_pool<mesh_primitive_component> m_mesh_primitives;
        //! \brief All \a material_components.
        scene_component_pool<material_component> m_materials;
        //! \brief All \a camera_components.
        scene_component_pool<camera_component> m_cameras;
        //! \brief All \a directional_light_component.
        scene_component_pool<directional_light_component> m_directional_lights;
        //! \brief All \a atmosphere_light_component.
        scene_component_pool<atmosphere_light_component> m_atmosphere_lights;
        //! \brief All \a skylight_component.
        scene_component_pool<skylight_component> m_skylights;
        //! \brief The root entity of the ecs.
        entity m_root_entity;
        //! \brief The current root entity of the scene.
        entity m_scene_root;

        struct
        {
            //! \brief The currently active camera entity.
            entity camera;
        } m_active; //!< Storage for active scene singleton entities.

        //! \brief Scene boundaries.
        struct scene_bounds
        {
            glm::vec3 min;    //!< Minimum geometry values.
            glm::vec3 max;    //!< Maximum geometry values.
        } m_scene_boundaries; //!< The boundaries of the current scene.
    };                        // namespace mango

} // namespace mango

#endif // MANGO_SCENE_HPP
