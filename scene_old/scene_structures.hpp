//! \file      scene_structures.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_SCENE_STRUCTURES
#define MANGO_SCENE_STRUCTURES

#include <mango/packed_freelist.hpp>
#include <mango/types.hpp>

namespace mango
{
    //! \brief Macro to add some default operators and constructors to every scene structure.
#define DECLARE_SCENE_STRUCTURE(c)    \
    sid instance_id;                  \
    ~c()        = default;            \
    c(const c&) = default;            \
    c(c&&)      = default;            \
    c& operator=(const c&) = default; \
    c& operator=(c&&) = default;

    //! \brief Describes the type of an element referenced by a \a sid.
    enum class scene_structure_type : uint8
    {
        scene_structure_unknown,
        scene_structure_transform,
        scene_structure_perspective_camera,
        scene_structure_orthographic_camera,
        scene_structure_directional_light,
        scene_structure_skylight,
        scene_structure_atmospheric_light,
        scene_structure_texture,
        scene_structure_material,
        scene_structure_primitive,
        scene_structure_mesh,
        scene_structure_node,
        scene_structure_scenario,
        scene_structure_model,
        scene_structure_internal_buffer,
        scene_structure_internal_buffer_view,
        scene_structure_count = scene_structure_internal_buffer_view
    };

    //! \brief An Id for all scene objects.
    //! \details Similar to an entity.
    struct sid
    {
      public:
        sid()
            : m_pf_id()
            , m_structure_type(scene_structure_type::scene_structure_unknown)
        {
        }
        ~sid() = default;
        //! \brief Copy constructor.
        sid(const sid&) = default;
        //! \brief Move constructor.
        sid(sid&&) = default;
        //! \brief Assignment operator.
        sid& operator=(const sid&) = default;
        //! \brief Move assignment operator.
        sid& operator=(sid&&) = default;

        //! \brief Checks if the \a sid is valid.
        //! \return True if the \a sid is valid and the structure is known, else false.
        inline bool is_valid()
        {
            return m_structure_type != scene_structure_type::scene_structure_unknown;
        }

        //! \brief Comparison operator equal.
        //! \param other The other \a sid.
        //! \return True if other \a sid is equal to the current one, else false.
        bool operator==(const sid& other) const
        {
            return (m_pf_id.get() == other.id().get()) && (m_structure_type == other.m_structure_type);
        }

        //! \brief Comparison operator not equal.
        //! \param other The other \a sid.
        //! \return True if other \a sid is not equal to the current one, else false.
        bool operator!=(const sid& other) const
        {
            return (m_pf_id.get() != other.id().get()) || (m_structure_type != other.m_structure_type);
        }

        //! \brief Comparison operator less.
        //! \param other The other \a sid.
        //! \return True if other \a sid is less then the current one, else false.
        bool operator<(const sid& other) const
        {
            return (m_pf_id.get() == other.id().get());
        }

        //! \brief Returns the internal \a packed_freelist_id.
        //! \return A constant reference to the internal \a packed_freelist_id.
        inline const packed_freelist_id& id() const
        {
            return m_pf_id;
        }

        //! \brief Retrieves the \a scene_structure_type of the element referenced by the id.
        //! \return A constant reference to the \a scene_structure_type of the element referenced by the id.
        inline const scene_structure_type& structure_type() const
        {
            return m_structure_type;
        }

      private:
        friend class scene_impl;
        friend struct sid_hash;

        //! \brief Constructor for internal creation of \a sids.
        //! \param[in] pf_id The \a packed_freelist_id of the new \a sid.
        //! \param[in] tp The \a scene_structure_type of the new \a sid.
        sid(packed_freelist_id pf_id, scene_structure_type tp)
            : m_pf_id(pf_id)
            , m_structure_type(tp)
        {
        }

        //! \brief Creates a new \a sid.
        //! \param[in] pf_id The \a packed_freelist_id of the new \a sid.
        //! \param[in] tp The \a scene_structure_type of the new \a sid.
        //! \return The created \a sid.
        inline static sid create(packed_freelist_id pf_id, scene_structure_type tp)
        {
            return sid(pf_id, tp);
        }

        //! \brief The \a packed_freelist_id of the \a sid.
        packed_freelist_id m_pf_id;
        //! \brief The \a scene_structure_type of the \a sid.
        scene_structure_type m_structure_type;
    };

    //! \brief An invalid \a sid.
    static const sid invalid_sid;

    //! \brief Hash for the \a sid structure.
    struct sid_hash
    {
        //! \brief Function call operator.
        //! \details Hashes the \a sid.
        //! \param[in] k The \a sid to hash.
        //! \return The hash for the given \a sid.
        std::size_t operator()(const sid& k) const
        {
            // https://stackoverflow.com/questions/1646807/quick-and-simple-hash-code-combinations/

            size_t res = 17;
            res        = res * 31 + std::hash<uint32>()(k.id().get());
            res        = res * 31 + std::hash<uint8>()(static_cast<uint8>(k.m_structure_type));

            return res;
        };
    };

    //! \brief Public structure holding transformation informations.
    //! \details Used to store position, rotation and scale for a node.
    struct transform
    {
        //! \brief The \a sid of the containing node.
        sid containing_node;

        //! \brief The position of the \a transform.
        vec3 position;
        //! \brief The rotation of the \a transform.
        quat rotation;
        //! \brief The scale of the \a transform.
        vec3 scale;

        transform()
            : position(0.0f)
            , rotation(1.0f, 0.0f, 0.0f, 0.0f)
            , scale(1.0f)
            , changed(false)
        {
        }
        //! \brief \a Transform is a scene structure.
        DECLARE_SCENE_STRUCTURE(transform);

        //! \brief Marks the \a transform to be updated.
        //! \details Has to be called after making changes.
        inline void update()
        {
            changed = true;
        }

        //! \brief Checks if the \a transform has been updated.
        //! \return True if changes were made, else false.
        inline bool dirty() // TODO Paul: Could be handled better.
        {
            return changed;
        }

      private:
        friend struct scene_transform; // TODO Paul: Could be handled better.
        //! \brief Change flag. True if changes were made, else false.
        bool changed;
    };

    //! \brief Public structure holding informations for a perspective camera.
    struct perspective_camera
    {
        //! \brief The \a sid of the containing node.
        sid containing_node;

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

        perspective_camera()
            : aspect(0.0f)
            , vertical_field_of_view(0.0f)
            , z_far(0.0f)
            , z_near(0.0f)
            , adaptive_exposure(true)
            , target(0.0f)
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
        //! \brief The \a sid of the containing node.
        sid containing_node;

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

        orthographic_camera()
            : x_mag(0.0f)
            , y_mag(0.0f)
            , z_far(0.0f)
            , z_near(0.0f)
            , adaptive_exposure(true)
            , target(0.0f)
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
        //! \brief The \a sid of the containing node.
        sid containing_node;

        //! \brief The direction of the \a directional_light from the point to the light.
        vec3 direction;
        //! \brief The color of the \a directional_light. Values between 0.0 and 1.0.
        color_rgb color;
        //! \brief The intensity of the \a directional_light in lumen.
        float intensity;
        //! \brief True if the \a directional_light should cast shadows, else false.
        bool cast_shadows;
        //! \brief True if the \a directional_light should contribute to the atmosphere, when enabled, else false.
        bool contribute_to_atmosphere;

        directional_light()
            : direction(0.0f)
            , color(0.0f)
            , intensity(default_directional_intensity)
            , cast_shadows(false)
            , contribute_to_atmosphere(false)
        {
        }
        //! \brief \a Directional_light is a scene structure.
        DECLARE_SCENE_STRUCTURE(directional_light);
    };

    //! \brief Public structure holding informations for a skylight.
    struct skylight
    {
        //! \brief The \a sid of the containing node.
        sid containing_node;

        //! \brief The \a sid of the environment texture of the \a skylight.
        sid hdr_texture;
        //! \brief The intensity of the \a skylight in cd/m^2.
        float intensity;
        //! \brief True if the \a skylight should use a texture, else false.
        bool use_texture;
        //! \brief True if the \a skylight should be updated dynamically, else false.
        bool dynamic;
        //! \brief True if the \a skylight is a local skylight, else, if it is the global one, false.
        bool local;

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

    //! \brief Public structure holding informations for a atmospheric light.
    struct atmospheric_light
    {
        //! \brief The \a sid of the containing node.
        sid containing_node;

        // TODO Paul: Add documentation, when these work again.
        //! \cond NO_COND

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

        //! \endcond

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

        {
        }
        //! \brief \a Atmospheric_light is a scene structure.
        DECLARE_SCENE_STRUCTURE(atmospheric_light);
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

        texture()
            : standard_color_space(false)
            , high_dynamic_range(false)
            , changed(false)
        {
        }
        //! \brief \a Texture is a scene structure.
        DECLARE_SCENE_STRUCTURE(texture);

        //! \brief Marks the \a texture to be updated.
        //! \details Has to be called after making changes.
        inline void update()
        {
            changed = true;
        }

        //! \brief Checks if the \a texture has been updated.
        //! \return True if changes were made, else false.
        inline bool dirty() // TODO Paul: Could be handled better.
        {
            return changed;
        }

      private:
        friend struct scene_texture; // TODO Paul: Could be handled better.
        //! \brief Change flag. True if changes were made, else false.
        bool changed;
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
        //! \brief The \a sid of the base color texture of the \a material. \a Texture should be in standard color space.
        sid base_color_texture;
        //! \brief The metallic property of the \a material. Value between 0.0 and 1.0.
        normalized_float metallic;
        //! \brief The roughness property of the \a material. Value between 0.0 and 1.0.
        normalized_float roughness;
        //! \brief The \a sid of the metallic and roughness texture of the \a material. \a Texture could also include an occlusion value in the blue component.
        sid metallic_roughness_texture;
        //! \brief True if the metallic roughness texture of the \a material includes an occlusion value in the blue component, else false.
        bool packed_occlusion;

        //! \brief The \a sid of the normal texture of the \a material.
        sid normal_texture;
        //! \brief The \a sid of the occlusion texture of the \a material.
        sid occlusion_texture;

        //! \brief The emissive color of the \a material. Values between 0.0 and 1.0.
        color_rgb emissive_color;
        //! \brief The \a sid of the emissive color texture of the \a material. \a Texture should be in standard color space.
        sid emissive_texture;
        //! \brief The emissive intensity of the \a material in lumen.
        float emissive_intensity;

        //! \brief True if the \a material should be rendered double sided, else false.
        bool double_sided;
        //! \brief The \a material_alpha_mode of the \a material.
        material_alpha_mode alpha_mode;
        //! \brief The alpha cutoff of the \a material. Value between 0.0 and 1.0.
        normalized_float alpha_cutoff;

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

        //! \brief TThe \a sid of the \a primitives \a material.
        sid material;

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

        //! \brief The \a sid of the containing node.
        sid containing_node;

        mesh() = default;
        //! \brief \a Mesh is a scene structure.
        DECLARE_SCENE_STRUCTURE(mesh);
    };

    //! \brief Public structure holding informations for a node.
    struct node
    {
        //! \brief The name of the \a node.
        string name;

        //! \brief The \a sid of the \a scenario holding the \a node. Currently not really in use.
        sid containing_scenario;

        //! \brief The \a sid of the parent \a node.
        sid parent_node;

        node() = default;

        //! \brief Constructor taking a name.
        //! \param[in] node_name The name for the new node.
        node(const string& node_name)
            : name(node_name){};

        //! \brief \a Node is a scene structure.
        DECLARE_SCENE_STRUCTURE(node);
    };

    //! \brief Public structure holding informations for a scenario. At the moment not to relevant.
    struct scenario
    {
        scenario() = default;
        //! \brief \a Scenario is a scene structure.
        DECLARE_SCENE_STRUCTURE(scenario);
    };

    //! \brief Public structure holding informations for a loaded model.
    struct model
    {
        //! \brief The full file path of the loaded model.
        string file_path;

        //! \brief List if \a sids referencing all scenarios in the \a model.
        std::vector<sid> scenarios;
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