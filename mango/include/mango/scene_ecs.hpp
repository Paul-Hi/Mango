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
    const uint32 max_pool_entries = 1000; // Extend if necessary.
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
        glm::vec3 position = glm::vec3(0.0f);                     //!< The local position.
        glm::quat rotation = glm::quat(glm::vec3(0.0, 0.0, 0.0)); //!< The local rotation quaternion.
        glm::vec3 scale    = glm::vec3(1.0f);                     //!< The local scale.

        glm::mat4 local_transformation_matrix = glm::mat4(1.0f); //!< The local transformation.
        glm::mat4 world_transformation_matrix = glm::mat4(1.0f); //!< The world transformation. If there is no parent this is also the local transformation.
    };

    //! \brief Component used to build a graph like structure. This is necessary for parenting.
    struct node_component
    {
        entity parent_entity = invalid_entity; //!< The parents entity id.

        int32 children_count    = 0;              //!< The number of childs.
        entity child_entities   = invalid_entity; //!< The first child entity id. (Linked list)
        entity next_sibling     = invalid_entity; //!< The next child entity id.
        entity previous_sibling = invalid_entity; //!< The previous child entity id.
    };

    //! \brief Component used to describe a primitive draw call. Used by \a mesh_component.
    struct primitive_component
    {
        shared_ptr<vertex_array> vertex_array_object; //!< The vertex array object of the primitive.
        primitive_topology topology;                  //!< Topology of the primitive data.
        int32 first;                                  //!< First index.
        int32 count;                                  //!< Number of elements/vertices.
        index_type type_index;                        //!< The type of the values in the index buffer.
        int32 instance_count;                         //!< Number of instances. Usually 1.
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

    //! \brief Component used for renderable mesh geometry. Used for drawing.
    struct mesh_component
    {
        //! \brief A list of \a primitive_components.
        std::vector<primitive_component> primitives;
        //! \brief A list of \a material_components.
        std::vector<material_component> materials;

        //! \brief Specifies if the mesh has normals.
        bool has_normals;
        //! \brief Specifies if the mesh has tangents.
        bool has_tangents;
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

    //! \brief The default intensity of an environment. Is approx. the intensity of a sunny sky.
    const float default_environment_intensity = 30000.0f;

    //! \brief Component used for the scene environment.
    struct environment_component
    {
        glm::mat3 rotation_scale_matrix = glm::mat3(1.0f); //!< The rotation and scale of the environment.
        shared_ptr<texture> hdr_texture;                   //!< The hdr texture used to build the environment.
        float intensity = default_environment_intensity;   //!< Intensity in cd/m^2. Default 30000 (sunny sky).
    };

    //! \brief Light types used in \a light_components.
    enum class light_type : uint8
    {
        directional //!< Simple directional light.
    };

    //! \brief Base light data for every light.
    struct light_data
    {
    };

    //! \brief The default intensity of a directional light. Is approx. the intensity of the sun.
    const float default_directional_intensity = 110000.0f;

    //! \brief Light data for directional lights.
    struct directional_light_data : public light_data
    {
        glm::vec3 direction = glm::vec3(1.0f);             //!< The light direction.
        color_rgb light_color;                             //!< The light color. Will get multiplied by the intensity.
        float intensity   = default_directional_intensity; //!< The instensity of the light in lumen (111000 would f.e. be a basic sun)
        bool cast_shadows = false;                         //!< True if the light should cast shadows.
    };

    //! \brief Component used for all lights excluding image based lights.
    struct light_component
    {
        light_type type_of_light    = light_type::directional;                    //!< The type of the light.
        shared_ptr<light_data> data = std::make_shared<directional_light_data>(); //!< Light specific data.
    };

    //! \brief Structure used for collecting all the camera data of the current active camera.
    struct camera_data
    {
        entity active_camera_entity;    //!< The entity.
        camera_component* camera_info;  //!< The camera info.
        transform_component* transform; //!< The cameras transform.
    };

    //! \brief Structure used for collecting all the environment data of the current active environment.
    struct environment_data
    {
        entity active_environment_entity;        //!< The entity.
        environment_component* environment_info; //!< The environment info.
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
    };

    template <>
    struct type_name<transform_component>
    {
        static const char* get()
        {
            return "transform_component";
        }
    };
    template <>
    struct type_name<node_component>
    {
        static const char* get()
        {
            return "node_component";
        }
    };
    template <>
    struct type_name<primitive_component>
    {
        static const char* get()
        {
            return "primitive_component";
        }
    };
    template <>
    struct type_name<material_component>
    {
        static const char* get()
        {
            return "material_component";
        }
    };
    template <>
    struct type_name<model_component>
    {
        static const char* get()
        {
            return "model_component";
        }
    };
    template <>
    struct type_name<mesh_component>
    {
        static const char* get()
        {
            return "mesh_component";
        }
    };
    template <>
    struct type_name<camera_component>
    {
        static const char* get()
        {
            return "camera_component";
        }
    };
    template <>
    struct type_name<environment_component>
    {
        static const char* get()
        {
            return "environment_component";
        }
    };
    template <>
    struct type_name<light_component>
    {
        static const char* get()
        {
            return "light_component";
        }
    };
    //! \endcond
} // namespace mango

#endif // MANGO_SCENE_ECS_HPP
