//! \file      scene_ecs.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_SCENE_ECS_HPP
#define MANGO_SCENE_ECS_HPP

#include <mango/types.hpp>

namespace mango
{
    //! \brief Maximum number of scene \a pool entries in mango.
    const uint32 max_pool_entries = 10000; // Extend if necessary.
    //! \brief Maximum number of \a entities in mango.
    const uint32 max_entities = max_pool_entries;

    //! \brief An \a entity. Just a positive integer used as an id.
    using entity = uint32;
    //! \brief Invalid \a entity.
    const entity invalid_entity = 0;

    template <typename component> // FWD
    class scene_component_pool;   // FWD

    //! \brief A templated base class for all ecs systems that require one component.
    template <typename component>
    class ecsystem_1
    {
      public:
        //! \brief The execute function for the \a ecsystem.
        //! \param[in] dt The time since the last call.
        //! \param[in] components A \a component_pool.
        virtual void execute(float dt, scene_component_pool<component>& components);
    };

    //! \brief A templated base class for all ecs systems that require two components.
    template <typename component_1, typename component_2>
    class ecsystem_2
    {
      public:
        //! \brief The execute function for the \a ecsystem.
        //! \param[in] dt The time since the last call.
        //! \param[in] components_1 First \a component_pool.
        //! \param[in] components_2 Second \a component_pool.
        virtual void execute(float dt, scene_component_pool<component_1>& components_1, scene_component_pool<component_2>& components_2);
    };

    //! \brief A templated base class for all ecs systems that require three components.
    template <typename component_1, typename component_2, typename component_3>
    class ecsystem_3
    {
      public:
        //! \brief The execute function for the \a ecsystem.
        //! \param[in] dt The time since the last call.
        //! \param[in] components_1 First \a component_pool.
        //! \param[in] components_2 Second \a component_pool.
        //! \param[in] components_3 Third \a component_pool.
        virtual void execute(float dt, scene_component_pool<component_1>& components_1, scene_component_pool<component_2>& components_2, scene_component_pool<component_3>& components_3);
    };

    // fwd
    class vertex_array;
    struct material;
    class texture;

    //! \brief Component used for to give an \a entity a name.
    struct tag_component
    {
        string tag_name; //!< The name.
    };

    //! \brief Component used to transform anything in the scene.
    struct transform_component
    {
        glm::vec3 position      = glm::vec3(0.0f);                     //!< The local position.
        glm::quat rotation      = glm::quat(glm::vec3(0.0, 0.0, 0.0)); //!< The local rotation quaternion.
        glm::vec3 rotation_hint = glm::vec3(0.0, 0.0, 0.0);            //!< The local rotation hint (used for editor controls).
        glm::vec3 scale         = glm::vec3(1.0f);                     //!< The local scale.

        glm::mat4 local_transformation_matrix = glm::mat4(1.0f); //!< The local transformation.
        glm::mat4 world_transformation_matrix = glm::mat4(1.0f); //!< The world transformation. If there is no parent this is also the local transformation.
    };

    //! \brief Component used to build a graph like structure. This is necessary for parenting.
    struct node_component
    {
        entity parent_entity = invalid_entity; //!< The parents entity id.

        int32 children_count    = 0;              //!< The number of childs.
        entity child_entities   = invalid_entity; //!< The first child entity id. (Linked list)
        entity next_sibling     = invalid_entity; //!< The next sibling entity id.
        entity previous_sibling = invalid_entity; //!< The previous sibling entity id.
    };

    //! \brief A type for mesh_primitives.
    enum class mesh_primitive_type : uint8
    {
        plane,     //!< A plane.
        box,       //!< A box.
        sphere,    //!< A sphere.
        custom     //!< Custom type. Normaly used when loaded from file.
    };

    //! \brief Component used to describe a mesh primitive draw call.
    struct mesh_primitive_component
    {
        shared_ptr<vertex_array> vertex_array_object; //!< The vertex array object of the mesh primitive.
        primitive_topology topology;                  //!< Topology of the mesh primitive data.
        int32 first;                                  //!< First index.
        int32 count;                                  //!< Number of elements/vertices.
        index_type type_index;                        //!< The type of the values in the index buffer.
        int32 instance_count;                         //!< Number of instances. Usually 1.
        bool has_normals;                             //!< Specifies if the mesh primitive has normals.
        bool has_tangents;                            //!< Specifies if the mesh primitive has tangents.
        mesh_primitive_type tp;                       //!< Specifies if the type of mesh primitive.
    };

    //! \brief Component used for materials.
    struct material_component
    {
        string material_name;                    //!< The name of the material.
        shared_ptr<material> component_material; //!< The material holding all properties, textures etc.
    };

    //! \brief Component used for gltf models.
    struct model_component
    {
        string model_file_path; //!< The models location.
        // TODO Paul: Extract bounds into own class.
        glm::vec3 min_extends; //!< The minimum extends of the gltf model.
        glm::vec3 max_extends; //!< The maximum extends of the gltf model.
    };

    //! \brief The minimum valid value for the camera aperture.
    const float min_aperture = 0.5f;
    //! \brief The default value for the camera aperture.
    const float default_aperture = 16.0f;
    //! \brief The maximum valid value for the camera aperture.
    const float max_aperture = 64.0f;
    //! \brief The minimum valid value for the camera shutter speed.
    const float min_shutter_speed = 1.0f / 25000.0f;
    //! \brief The default value for the camera shutter speed.
    const float default_shutter_speed = 1.0f / 125.0f;
    //! \brief The maximum valid value for the camera shutter speed.
    const float max_shutter_speed = 60.0f;
    //! \brief The minimum valid value for the camera iso.
    const float min_iso = 10.0f;
    //! \brief The default value for the camera iso.
    const float default_iso = 100.0f;
    //! \brief The maximum valid value for the camera iso.
    const float max_iso = 204800.0f;

    //! \brief Camera types used in \a camera_components.
    enum class camera_type : uint8
    {
        perspective_camera, //!< Perspective projection. Usually useful for 3D scenes.
        orthographic_camera //!< Orthographic projection. Usually useful for 2D scenes or UI.
    };

    //! \brief Component used for camera entities.
    struct camera_component
    {
        camera_type cam_type; //!< The type of camera projection.

        float z_near; //!< Distance of the near plane.
        float z_far;  //!< Distance of the far plane.
        struct
        {
            float vertical_field_of_view; //!< Vertical field of view in radians.
            float aspect;                 //!< Aspect ratio. Width divided by height.
        } perspective;                    //!< Parameters for perspective projection.
        struct
        {
            float x_mag; //!< Magnification in x direction.
            float y_mag; //!< Magnification in y direction.
        } orthographic;  //!< Parameters for orthographic projection.
        struct
        {
            float aperture         = default_aperture;      //!< The aperture.
            float shutter_speed    = default_shutter_speed; //!< The shutter speed.
            float iso              = default_iso;           //!< The iso.
            bool adaptive_exposure = true;                  //!< True if the exposure and all corresponding parameters should be adapted automatically, else false.

        } physical;       //!< Physical parameters.
        glm::vec3 up;     //!< The cameras up vector.
        glm::vec3 target; //!< The target to look at.
        //! \brief The view matrix of the \a camera_component.
        glm::mat4 view;
        //! \brief The projection matrix of the \a camera_component.
        glm::mat4 projection;
        //! \brief The view projection matrix of the \a camera_component.
        glm::mat4 view_projection;
    };

    //! \brief Base component of all light components.
    struct base_light_component
    {
        //! \brief ID of the light.
        uint32 l_id;
        //! \brief Counstructs a \a base_light_component.
        base_light_component()
        {
            static uint32 id = 1;
            l_id             = id++;
        } // TODO Paul: These should be done differently! 0 is invalid.
        //! \brief Active flag.
        bool active = true;
    };

    //! \brief Component for directional lights.
    struct directional_light_component : base_light_component
    {
        //! \brief The directional light.
        directional_light light;
    };

    //! \brief Component for environmental / image based lighting.
    struct skylight_component : base_light_component
    {
        //! \brief The skylight.
        skylight light;
    };

    //! \brief Component for atmospheric Lighting.
    struct atmosphere_light_component : base_light_component
    {
        //! \brief The atmospheric light.
        atmosphere_light light;
    };

    //! \brief Structure used for collecting all the camera data of the current active camera.
    struct camera_data
    {
        entity active_camera_entity;    //!< The entity.
        camera_component* camera_info;  //!< The camera info.
        transform_component* transform; //!< The cameras transform.
    };

    // TODO Paul: This will be reworked when we need the reflection for the components.
    //! \cond NO_COND
    template <typename T>
    struct type_name
    {
        static const char* get()
        {
            return typeid(T).name();
        }

        static int32 id()
        {
            return -1;
        }
    };

    template <>
    struct type_name<tag_component>
    {
        static const char* get()
        {
            return "Tag Component";
        }

        static int32 id()
        {
            return 0;
        }
    };
    template <>
    struct type_name<transform_component>
    {
        static const char* get()
        {
            return "Transform Component";
        }

        static int32 id()
        {
            return 1;
        }
    };
    template <>
    struct type_name<node_component>
    {
        static const char* get()
        {
            return "Node Component";
        }

        static int32 id()
        {
            return 2;
        }
    };
    template <>
    struct type_name<mesh_primitive_component>
    {
        static const char* get()
        {
            return "Mesh Primitive Component";
        }

        static int32 id()
        {
            return 3;
        }
    };
    template <>
    struct type_name<material_component>
    {
        static const char* get()
        {
            return "Material Component";
        }

        static int32 id()
        {
            return 4;
        }
    };
    template <>
    struct type_name<model_component>
    {
        static const char* get()
        {
            return "Model Component";
        }

        static int32 id()
        {
            return 5;
        }
    };
    template <>
    struct type_name<camera_component>
    {
        static const char* get()
        {
            return "Camera Component";
        }

        static int32 id()
        {
            return 6;
        }
    };
    template <>
    struct type_name<directional_light_component>
    {
        static const char* get()
        {
            return "Directional Light Component";
        }

        static int32 id()
        {
            return 7;
        }
    };
    template <>
    struct type_name<atmosphere_light_component>
    {
        static const char* get()
        {
            return "Atmosphere Light Component";
        }

        static int32 id()
        {
            return 8;
        }
    };
    template <>
    struct type_name<skylight_component>
    {
        static const char* get()
        {
            return "Skylight Component";
        }

        static int32 id()
        {
            return 9;
        }
    };
    //! \endcond
} // namespace mango

#endif // MANGO_SCENE_ECS_HPP
