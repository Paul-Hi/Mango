//! \file      scene.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_SCENE_HPP
#define MANGO_SCENE_HPP

#include <mango/scene_component_manager.hpp>
#include <mango/scene_types.hpp>

namespace tinygltf
{
    struct Model;
    struct Node;
    struct Mesh;
} // namespace tinygltf

namespace mango
{
    class context_impl;
    class shader_program;
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

        //! \brief Creates a camera entity.
        //! \details An entity with \a camera_component and \a transform_component.
        //! All the components are prefilled. Camera has a perspective projection.
        //! \return The created camera entity.
        entity create_default_camera();

        //! \brief Creates entities from a model loaded from a gltf file.
        //! \details Internally creates entities with \a mesh_components, \a material_components, \a transform_components and \a node_components.
        //! All the components are filled with data loaded from the gltf file.
        //! \return A list of all created entities.
        std::vector<entity> create_entities_from_model(const string& path);

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
        //! \return The \a camera_data or nullptr if non-existent.
        inline shared_ptr<camera_data> get_active_camera_data()
        {
            return m_active_camera_data;
        }

      private:
        //! \brief Builds one or more entities that describe an entire model with data loaded by tinygltf.
        //! \details Internally called by create_entities_from_model(...).
        //! This also creates the hierarchy incl. \a transform_components and \a node_components.
        //! \param[in] entities The entity array where all entities are inserted.
        //! \param[in] m The model loaded by tinygltf.
        //! \param[in] n The node loaded by tinygltf.
        //! \return The root node of the function call.
        entity build_model_node(std::vector<entity>& entities, tinygltf::Model& m, tinygltf::Node& n);

        //! \brief Attaches a \a mesh_component to an \a entity with data loaded by tinygltf.
        //! \details Internally called by create_entities_from_model(...).
        //!\param[in] node The entity that the \a mesh_component should be attached to.
        //!\param[in] m The model loaded by tinygltf.
        //!\param[in] mesh The mesh loaded by tinygltf.
        void build_model_mesh(entity node, tinygltf::Model& m, tinygltf::Mesh& mesh);

        friend class context_impl; // TODO Paul: Better way?
        //! \brief Mangos internal context for shared usage in all \a render_systems.
        shared_ptr<context_impl> m_shared_context;

        //! \brief All \a node_components.
        scene_component_manager<node_component> m_nodes;
        //! \brief All \a transform_components.
        scene_component_manager<transform_component> m_transformations;
        //! \brief All \a mesh_components.
        scene_component_manager<mesh_component> m_meshes;
        //! \brief All \a camera_components.
        scene_component_manager<camera_component> m_cameras;
        //! \brief The \a camera_data for the currently active camera.
        shared_ptr<camera_data> m_active_camera_data;
    };

} // namespace mango

#endif // MANGO_SCENE_HPP
