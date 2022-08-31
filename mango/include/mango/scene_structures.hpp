//! \file      scene_structures.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2022
//! \copyright Apache License 2.0

#ifndef MANGO_SCENE_STRUCTURES
#define MANGO_SCENE_STRUCTURES

#include <mango/intersect.hpp>
#include <mango/slotmap.hpp>
#include <mango/types.hpp>

namespace mango
{
    //! \brief Macro to add some default operators and constructors to every scene structure.
#define DECLARE_SCENE_STRUCTURE(c)       \
    bool changed              = true;    \
    ~c()                      = default; \
    c(const c&)               = default; \
    c(c&&)                    = default; \
    c& operator=(const c&)    = default; \
    c& operator=(c&&)         = default;

    // fwd
    struct node;
    struct texture;

    //! \brief Public structure holding transformation informations.
    //! \details Used to store position, rotation and scale for a node.
    struct transform
    {
        //! \brief The position of the \a transform.
        vec3 position;
        //! \brief The rotation of the \a transform.
        quat rotation;
        //! \brief The scale of the \a transform.
        vec3 scale;

        //! \brief Rotation hint. Equal to the transforms quaternion rotation, but converted to euler angles.
        vec3 rotation_hint;

        transform()
            : position(make_vec3(0.0f))
            , rotation(1.0f, 0.0f, 0.0f, 0.0f)
            , scale(make_vec3(1.0f))
        {
        }
        //! \brief \a Transform is a scene structure.
        DECLARE_SCENE_STRUCTURE(transform);
    };

    //! \brief Public structure holding informations for a perspective camera.
    struct perspective_camera
    {
        //! \brief The aspect ratio of the \a perspective_camera.
        float aspect;
        //! \brief The vertical field of view of the \a perspective_camera in radians.
        float vertical_field_of_view;
        //! \brief The far plane distance of the \a perspective_camera.
        float z_far;
        //! \brief The near plane distance of the \a perspective_camera.
        float z_near;

        struct
        {
            float aperture;      //!< Camera aperture.
            float shutter_speed; //!< Camera shutter speed.
            float iso;           //!< Camera ISO.
        } physical;              //!< Physical parameters.

        //! \brief True if the exposure settings should be handled automatically, else false.
        bool adaptive_exposure;

        //! \brief The target point of the \a perspective_camera.
        vec3 target;

        //! \brief The \a handle of the \a node of the \a perspective_camera.
        handle<node> node_hnd;

        //! \brief The \a key of the GPU data of the \a perspective_camera.
        key gpu_data;

        perspective_camera()
            : aspect(0.0f)
            , vertical_field_of_view(0.0f)
            , z_far(0.0f)
            , z_near(0.0f)
            , adaptive_exposure(true)
            , target(make_vec3(0.0f))
        {
            physical.aperture      = default_camera_aperture;
            physical.shutter_speed = default_camera_shutter_speed;
            physical.iso           = default_camera_iso;
        }
        //! \brief \a Perspective_camera is a scene structure.
        DECLARE_SCENE_STRUCTURE(perspective_camera);
    };

    //! \brief Public structure holding informations for an orthographic camera.
    struct orthographic_camera
    {
        //! \brief The zoom factor of the \a orthographic_camera in x direction.
        float x_mag;
        //! \brief The zoom factor of the \a orthographic_camera in y direction.
        float y_mag;
        //! \brief The far plane distance of the \a orthographic_camera.
        float z_far;
        //! \brief The near plane distance of the \a orthographic_camera.
        float z_near;

        struct
        {
            float aperture;      //!< Camera aperture.
            float shutter_speed; //!< Camera shutter speed.
            float iso;           //!< Camera ISO.
        } physical;              //!< Physical parameters.

        //! \brief True if the exposure settings should be handled automatically, else false.
        bool adaptive_exposure;

        //! \brief The target point of the \a orthographic_camera.
        vec3 target;

        //! \brief The \a handle of the \a node of the \a orthographic_camera.
        handle<node> node_hnd;

        //! \brief The \a key of the GPU data of the \a orthographic_camera.
        key gpu_data;

        orthographic_camera()
            : x_mag(0.0f)
            , y_mag(0.0f)
            , z_far(0.0f)
            , z_near(0.0f)
            , adaptive_exposure(true)
            , target(make_vec3(0.0f))
        {
            physical.aperture      = default_camera_aperture;
            physical.shutter_speed = default_camera_shutter_speed;
            physical.iso           = default_camera_iso;
        }
        //! \brief \a Orthographic_camera is a scene structure.
        DECLARE_SCENE_STRUCTURE(orthographic_camera);
    };

    //! \brief Structure holding informations for a directional light.
    struct directional_light
    {
        //! \brief The direction of the \a directional_light from the point to the light.
        vec3 direction;
        //! \brief The color of the \a directional_light. Values between 0.0 and 1.0.
        color_rgb color;
        //! \brief The intensity of the \a directional_light in lumen.
        float intensity;
        //! \brief True if the \a directional_light should cast shadows, else false.
        bool cast_shadows;

        //! \brief The \a handle of the \a node of the \a directional_light.
        handle<node> node_hnd;

        directional_light()
            : direction(make_vec3(0.0f))
            , color(0.0f)
            , intensity(default_directional_intensity)
            , cast_shadows(false)
        {
        }
        //! \brief \a Directional_light is a scene structure.
        DECLARE_SCENE_STRUCTURE(directional_light);

        bool operator==(const directional_light& other)
        {
            return node_hnd == other.node_hnd;
        }
    };

    //! \brief Public structure holding informations for a atmospheric light.
    struct atmospheric_light
    {
        float intensity_multiplier;
        // scattering parameters -> will be extended if necessary
        int32 scatter_points;
        int32 scatter_points_second_ray;
        vec3 rayleigh_scattering_coefficients;
        float mie_scattering_coefficient;
        vec2 density_multiplier;
        float ground_radius;
        float atmosphere_radius;
        float view_height;
        float mie_preferred_scattering_dir;
        bool draw_sun_disc;

        handle<node> sun;

        //! \brief The \a handle of the \a node of the \a atmospheric_light.
        handle<node> node_hnd;

        atmospheric_light()
            : intensity_multiplier(1.0f)
            , scatter_points(32)
            , scatter_points_second_ray(8)
            , rayleigh_scattering_coefficients(vec3(5.8e-6f, 13.5e-6f, 33.1e-6f))
            , mie_scattering_coefficient(21e-6f)
            , density_multiplier(vec2(8e3f, 1.2e3f))
            , ground_radius(6360e3f)
            , atmosphere_radius(6420e3f)
            , view_height(1e3f)
            , mie_preferred_scattering_dir(0.758f)
            , draw_sun_disc(false)
        {
        }
        //! \brief \a Atmospheric_light is a scene structure.
        DECLARE_SCENE_STRUCTURE(atmospheric_light);

        bool operator==(const atmospheric_light& other)
        {
            return node_hnd == other.node_hnd;
        }
    };

    //! \brief Public structure holding informations for a skylight.
    struct skylight
    {
        //! \brief The \a handle of the environment texture of the \a skylight.
        handle<texture> hdr_texture;
        //! \brief The intensity of the \a skylight in cd/m^2.
        float intensity;
        //! \brief True if the \a skylight should use a texture, else false.
        bool use_texture;
        //! \brief True if the \a skylight should be updated dynamically, else false.
        bool dynamic;
        //! \brief True if the \a skylight is a local skylight, else, if it is the global one, false.
        bool local;

        handle<node> atmosphere;

        //! \brief The \a handle of the \a node of the \a skylight.
        handle<node> node_hnd;

        skylight()
            : intensity(default_skylight_intensity)
            , use_texture(false)
            , dynamic(false)
            , local(false)
        {
        }
        //! \brief \a Skylight is a scene structure.
        DECLARE_SCENE_STRUCTURE(skylight);
    };

    //! \brief Public structure holding informations for a texture loaded from an image.
    struct texture
    {
        //! \brief The full file path of the image.
        string file_path;

        //! \brief True if the \a texture was loaded in standard color space, else false.
        bool standard_color_space;
        //! \brief True if the \a texture was loaded as high dynamic range, else false.
        bool high_dynamic_range;

        //! \brief The \a key of the GPU data of the \a texture.
        key gpu_data;

        texture()
            : standard_color_space(false)
            , high_dynamic_range(false)
            , changed(false)
        {
        }
        //! \brief \a Texture is a scene structure.
        DECLARE_SCENE_STRUCTURE(texture);
    };

    //! \brief The alpha mode of a \a material.
    enum class material_alpha_mode : uint8
    {
        mode_opaque = 0u,
        mode_mask   = 1u,
        mode_blend  = 2u,
        mode_dither = 3u
    };

    //! \brief Public structure holding informations for a material.
    //! \details At the moment this is more or less a metallic roughness physically based workflow.
    struct material
    {
        //! \brief The name of the \a material.
        string name;

        //! \brief The base color of the \a material. Values between 0.0 and 1.0.
        color_rgba base_color;
        //! \brief The \a handle of the base color texture of the \a material. \a Texture should be in standard color space.
        handle<texture> base_color_texture;
        //! \brief The metallic property of the \a material. Value between 0.0 and 1.0.
        normalized_float metallic;
        //! \brief The roughness property of the \a material. Value between 0.0 and 1.0.
        normalized_float roughness;
        //! \brief The \a handle of the metallic and roughness texture of the \a material. \a Texture could also include an occlusion value in the blue component.
        handle<texture> metallic_roughness_texture;
        //! \brief True if the metallic roughness texture of the \a material includes an occlusion value in the blue component, else false.
        bool packed_occlusion;

        //! \brief The \a handle of the normal texture of the \a material.
        handle<texture> normal_texture;
        //! \brief The \a handle of the occlusion texture of the \a material.
        handle<texture> occlusion_texture;

        //! \brief The emissive color of the \a material. Values between 0.0 and 1.0.
        color_rgb emissive_color;
        //! \brief The \a handle of the emissive color texture of the \a material. \a Texture should be in standard color space.
        handle<texture> emissive_texture;
        //! \brief The emissive intensity of the \a material in lumen.
        float emissive_intensity;

        //! \brief True if the \a material should be rendered double sided, else false.
        bool double_sided;
        //! \brief The \a material_alpha_mode of the \a material.
        material_alpha_mode alpha_mode;
        //! \brief The alpha cutoff of the \a material. Value between 0.0 and 1.0.
        normalized_float alpha_cutoff;

        //! \brief The optional \a key of the base color texture gpu data.
        optional<key> base_color_texture_gpu_data;
        //! \brief The optional \a key of the metallic and roughness texture gpu data.
        optional<key> metallic_roughness_texture_gpu_data;
        //! \brief The optional \a key of the normal texture gpu data.
        optional<key> normal_texture_gpu_data;
        //! \brief The optional \a key of the occlusion texture gpu data.
        optional<key> occlusion_texture_gpu_data;
        //! \brief The optional \a key of the emissive texture gpu data.
        optional<key> emissive_texture_gpu_data;

        //! \brief The \a key of the GPU data of the \a material.
        key gpu_data;

        material()
            : base_color()
            , metallic(1.0f)
            , roughness(1.0f)
            , packed_occlusion(false)
            , emissive_intensity(default_emissive_intensity)
            , double_sided(false)
            , alpha_mode(material_alpha_mode::mode_opaque)
            , alpha_cutoff(1.0f)

        {
        }
        //! \brief \a Material is a scene structure.
        DECLARE_SCENE_STRUCTURE(material);
    };

    //! \brief Describes the type of a \a primitive.
    enum class primitive_type : uint8
    {
        cube,
        uvsphere,
        icosphere,
        plane,
        custom
    };

    //! \brief Public structure holding informations for a primitive of a mesh.
    struct primitive
    {
        //! \brief The \a primitive_type.
        primitive_type type;

        //! \brief True if the \a primitive has normals in the vertex data, else false.
        bool has_normals;
        //! \brief True if the \a primitive has tangents in the vertex data, else false.
        bool has_tangents;

        //! \brief The \a handle of the \a primitives \a material.
        handle<material> primitive_material;

        //! \brief The \a axis_aligned_bounding_box of this \a primitive.
        axis_aligned_bounding_box bounding_box;

        //! \brief The \a key of the GPU data of the \a primitive.
        key gpu_data;

        primitive()
            : type(primitive_type::custom)
            , has_normals(false)
            , has_tangents(false)

        {
        }
        //! \brief \a Primitive is a scene structure.
        DECLARE_SCENE_STRUCTURE(primitive);
    };

    //! \brief Public structure holding informations for a mesh.
    struct mesh
    {
        //! \brief The name of the \a mesh.
        string name;

        //! \brief List of primitive \a handles of this mesh.
        std::vector<handle<primitive>> primitives;

        //! \brief The \a handle of the \a node of the \a mesh.
        handle<node> node_hnd;

        //! \brief The \a key of the GPU data of the \a mesh.
        key gpu_data;

        mesh() = default;
        //! \brief \a Mesh is a scene structure.
        DECLARE_SCENE_STRUCTURE(mesh);
    };

    //! \brief The type of a \a scene_node.
    //! \details Bitset.
    enum class node_type : uint8
    {
        hierarchy           = 0,
        mesh                = 1 << 0,
        perspective_camera  = 1 << 1,
        orthographic_camera = 1 << 2,
        directional_light   = 1 << 3,
        skylight            = 1 << 4,
        atmospheric_light   = 1 << 5
    };
    MANGO_ENABLE_BITMASK_OPERATIONS(node_type)

    //! \brief The type of a light.
    enum class light_type : uint8
    {
        directional = 0,
        skylight,
        atmospheric
    };

    //! \brief Public structure holding informations for a node.
    struct node
    {
        //! \brief The name of the \a node.
        string name;

        //! \brief List of \a handles referencing all children \a nodes. NULL_HND -> marked as deleted
        std::vector<handle<node>> children;

        //! \brief The type of the \a node.
        node_type type;

        //! \brief The \a handle of the nodes \a transform.
        handle<transform> transform_hnd;
        //! \brief The \a handle of the nodes \a mesh if \a node is one.
        handle<mesh> mesh_hnd;
        //! \brief The \a handle of the nodes \a perspective_camera if \a node is one.
        handle<perspective_camera> perspective_camera_hnd;
        //! \brief The \a handle of the nodes \a orthographic_camera if \a node is one.
        handle<orthographic_camera> orthographic_camera_hnd;
        //! \brief The \a handle of the nodes \a directional_light if \a node is one.
        handle<directional_light> directional_light_hnd;
        //! \brief The \a handle of the nodes \a skylight if \a node is one.
        handle<skylight> skylight_hnd;
        //! \brief The \a handle of the nodes \a atmospheric_light if \a node is one.
        handle<atmospheric_light> atmospheric_light_hnd;

        //! \brief The \a handle of the \a nodes cached global transformation matrix.
        handle<mat4> global_matrix_hnd;

        node() = default;

        //! \brief Constructor taking a name.
        //! \param[in] node_name The name for the new \a node.
        node(const string& node_name)
            : name(node_name)
            , type(node_type::hierarchy){};

        //! \brief \a Node is a scene structure.
        DECLARE_SCENE_STRUCTURE(node);
    };

    //! \brief Public structure holding informations for a scenario.
    struct scenario
    {
        scenario() = default;

        //! \brief List if \a handles referencing all root nodes in the \a scenario.
        std::vector<handle<node>> root_nodes;

        //! \brief \a Scenario is a scene structure.
        DECLARE_SCENE_STRUCTURE(scenario);
    };

    //! \brief Public structure holding informations for a loaded model.
    struct model
    {
        //! \brief The full file path of the loaded model.
        string file_path;

        //! \brief List of \a handles referencing all scenarios in the \a model.
        std::vector<handle<scenario>> scenarios;

        //! \brief Index in the list of scenarios providing the default \a scenario of the \a model.
        int32 default_scenario;

        model()
            : default_scenario(0)
        {
        }
        //! \brief \a Model is a scene structure.
        DECLARE_SCENE_STRUCTURE(model);
    };

#undef DECLARE_SCENE_STRUCTURE

} // namespace mango

#endif // MANGO_SCENE_STRUCTURES