//! \file      scene_internals.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_SCENE_INTERNALS
#define MANGO_SCENE_INTERNALS

#include <graphics/graphics_resources.hpp>
#include <mango/scene_structures.hpp>
#include <util/intersect.hpp>

namespace mango
{
    //! \brief Macro to add some default operators and constructors to every internal scene structure.
#define DECLARE_SCENE_INTERNAL(c)     \
    ~c()        = default;            \
    c(const c&) = default;            \
    c(c&&)      = default;            \
    c& operator=(const c&) = default; \
    c& operator=(c&&) = default;

    //! \brief An internal \a texture.
    struct scene_texture
    {
        //! \brief The public \a texture data.
        texture public_data;

        //! \brief The gpu \a gfx_texture.
        gfx_handle<const gfx_texture> graphics_texture;
        //! \brief The gpu \a gfx_sampler.
        gfx_handle<const gfx_sampler> graphics_sampler;

        scene_texture()
            : graphics_texture(nullptr)
            , graphics_sampler(nullptr)
        {
        }
        //! \brief The \a scene_texture is an internal scene structure.
        DECLARE_SCENE_INTERNAL(scene_texture);

        //! \brief Notifies that all changes were adressed. Should be called after the \a scene_texture was updated.
        inline void changes_handled() // TODO Paul: Could be handled better.
        {
            public_data.changed = false;
        }
    };

    //! \brief An internal \a material.
    struct scene_material
    {
        //! \brief The public \a material data.
        material public_data;

        scene_material() = default;
        //! \brief The \a scene_material is an internal scene structure.
        DECLARE_SCENE_INTERNAL(scene_material);
    };

    //! \brief An internal buffer structure.
    struct scene_buffer
    {
        //! \brief The \a sid of this instance.
        sid instance_id;
        //! \brief The name of the \a scene_buffer.
        string name;

        //! \brief The data of the \a scene_buffer.
        std::vector<uint8> data;

        scene_buffer() = default;
        //! \brief The \a scene_buffer is an internal scene structure.
        DECLARE_SCENE_INTERNAL(scene_buffer);
    };

    //! \brief An internal buffer view structure.
    struct scene_buffer_view
    {
        //! \brief The \a sid of this instance.
        sid instance_id;

        //! \brief The \a sid of viewed \a scene_buffer.
        sid buffer;
        //! \brief The offset of the \a scene_buffer_view.
        int32 offset;
        //! \brief The size of the \a scene_buffer_view.
        int32 size;
        //! \brief The stride of the \a scene_buffer_view.
        int32 stride;

        //! \brief The gpu \a gfx_buffer of the \a scene_buffer_view or nullptr if it is only relevant for the cpu.
        gfx_handle<const gfx_buffer> graphics_buffer;

        scene_buffer_view()
            : offset(0)
            , size(0)
            , stride(0)
            , graphics_buffer(nullptr)
        {
        }
        //! \brief The \a scene_buffer_view is an internal scene structure.
        DECLARE_SCENE_INTERNAL(scene_buffer_view);
    };

    //! \brief An internal \a primitive.
    struct scene_primitive
    {
        //! \brief The public \a primitive data.
        primitive public_data; // TODO Paul: Remember to reload internals on change.

        //! \brief The \a scene_primitives \a vertex_input_descriptor describing the vertex input for the pipeline.
        //! \details The \a renderer does the pipeline setup and should also cache this.
        vertex_input_descriptor vertex_layout;
        //! \brief The \a scene_primitives \a input_assembly_descriptor describing the input assembly for the pipeline.
        //! \details The \a renderer does the pipeline setup and should also cache this.
        input_assembly_descriptor input_assembly;

        //! \brief The \a scene_buffer_views used as vertex buffers.
        std::vector<scene_buffer_view> vertex_buffer_views;
        //! \brief The \a scene_buffer_view used as index buffer.
        scene_buffer_view index_buffer_view;
        //! \brief The \a gfx_format of the indices.
        gfx_format index_type;
        //! \brief The \a draw_call_description providing information to schedule a draw call for this \a scene_primitive.
        draw_call_description draw_call_desc;

        //! \brief The \a axis_aligned_bounding_box of this \a scene_primitive.
        axis_aligned_bounding_box bounding_box;

        scene_primitive()
            : index_type(gfx_format::t_unsigned_byte)
        {
        }
        //! \brief The \a scene_primitive is an internal scene structure.
        DECLARE_SCENE_INTERNAL(scene_primitive);
    };

    //! \brief An internal \a mesh.
    struct scene_mesh
    {
        //! \brief The public \a mesh data.
        mesh public_data;

        //! \brief All \a sids of the \a scene_primives contained by this \a scene_mesh.
        std::vector<sid> scene_primitives;

        scene_mesh() = default;
        //! \brief The \a scene_mesh is an internal scene structure.
        DECLARE_SCENE_INTERNAL(scene_mesh);
    };

    //! \brief The type of a \a scene_camera.
    enum class camera_type : uint8
    {
        perspective = 0,
        orthographic
    };

    //! \brief An internal camera.
    struct scene_camera
    {
        //! \brief The type of the \a scene_camera.
        camera_type type;

        //! \brief Optional \a perspective_camera if the type is perspective.
        optional<perspective_camera> public_data_as_perspective;
        //! \brief Optional \a perspective_camera if the type is orthographic.
        optional<orthographic_camera> public_data_as_orthographic;

        scene_camera()
            : type(camera_type::perspective)
            , public_data_as_perspective()
        {
        }
        //! \brief The \a scene_camera is an internal scene structure.
        DECLARE_SCENE_INTERNAL(scene_camera);
    };

    //! \brief The type of a \a scene_light.
    enum class light_type : uint8
    {
        directional = 0,
        skylight,
        atmospheric
    };

    //! \brief An internal light.
    struct scene_light
    {
        //! \brief The type of the \a scene_light.
        light_type type;

        //! \brief Optional \a directional_light if the type is directional.
        optional<directional_light> public_data_as_directional_light;
        //! \brief Optional \a skylight if the type is skylight.
        optional<skylight> public_data_as_skylight;
        //! \brief Optional \a atmospheric_light if the type is atmospheric.
        optional<atmospheric_light> public_data_as_atmospheric_light;

        scene_light()
            : type(light_type::directional)
            , public_data_as_directional_light()
        {
        }
        //! \brief The \a scene_light is an internal scene structure.
        DECLARE_SCENE_INTERNAL(scene_light);
    };

    //! \brief An internal \a transform.
    struct scene_transform
    {
        //! \brief The public \a transform data.
        transform public_data;

        //! \brief Rotation hint. Equal to the transforms quaternion rotation, but converted to euler angles.
        vec3 rotation_hint;

        scene_transform()
            : rotation_hint(0.0f)
        {
        }
        //! \brief The \a scene_transform is an internal scene structure.
        DECLARE_SCENE_INTERNAL(scene_transform);

        //! \brief Notifies that all changes were adressed. Should be called after the \a scene_transform was updated.
        inline void changes_handled() // TODO Paul: Could be handled better.
        {
            rotation_hint = glm::degrees(glm::eulerAngles(public_data.rotation));
            public_data.changed = false;
        }
    };

    //! \brief An internal structure holding data for joints used for pose transformations.
    struct scene_joint
    {
        //! \brief The \a sid of this instance.
        sid instance_id;

        //! \brief The \a sid of the \a scene_skin this \a scene_joint is contained by.
        sid skin_id;

        //! \brief The \a sid of the \a scene_joints \a scene_node.
        sid node_id;

        //! \brief The inverse bind matrix of this joint.
        //! \details Matrix transforming the joint to the root joint in the default pose.
        mat4 inverse_bind_matrix;

        //! \brief The joint matrix of this joint.
        //! \details Matrix calculated for every frame in the update function.
        //! \details inverse(parent_global_transform) * joint_global_transform * inverse_bind_matrix.
        mat4 joint_matrix;

        //! \brief The index of the joint in the vertex attributes.
        //! \details Used to map the joint_matrix correctly later on.
        int32 vertex_attribute_joint_idx;

        scene_joint()
            : inverse_bind_matrix(1.0f)
            , joint_matrix(1.0f)
            , vertex_attribute_joint_idx(-1)
        {
        }
        //! \brief The \a scene_joint is an internal scene structure.
        DECLARE_SCENE_INTERNAL(scene_joint);
    };

    //! \brief An internal structure holding data for skins having bone information.
    struct scene_skin
    {
        //! \brief The \a sid of this instance.
        sid instance_id;
        //! \brief The name of the \a scene_skin.
        string name;

        //! \brief The \a sid of the \a scene_skins containing \a scene_node.
        sid containing_nide_id;

        //! \brief All the \a sids of the \a scene_joints contained by this \a scene_skin.
        std::vector<sid> scene_joints;

        scene_skin() = default;
        //! \brief The \a scene_skin is an internal scene structure.
        DECLARE_SCENE_INTERNAL(scene_skin);
    };

    enum class animation_target_path : uint8
    {
        unknown = 0,
        translation,
        rotation,
        scale,
        weights // Not supported atm
    };
    enum class animation_interpolation_type : uint8
    {
        unknown = 0,
        linear,
        step,
        cubic_spline
    };

    struct animation_channel
    {
        int32 sampler_idx;
        sid target;
        animation_target_path target_path;
    };

    struct animation_sampler
    {
        std::vector<float> frames;
        std::vector<vec4> values; // Always storing vec4 even though they could be vec3.
        animation_interpolation_type interpolation_type;
    };

    //! \brief An internal structure holding animation data.
    struct scene_animation
    {
        //! \brief The \a sid of this instance.
        sid instance_id;
        //! \brief The name of the \a scene_animation.
        string name;
        //! \brief The \a sid of \a scene_model the animation was loaded with.
        sid model_id;

        //! \brief All  \a animation_channels of this \a scene_animation.
        std::vector<animation_channel> channels;

        //! \brief All  \a animation_samplers of this \a scene_animation.
        std::vector<animation_sampler> samplers;

        float duration;

        bool is_playing;

        float current_time;


        scene_animation()
            : duration(0.0f)
            , is_playing(false)
            , current_time(0.0f)
        {
        }
        //! \brief The \a scene_animation is an internal scene structure.
        DECLARE_SCENE_INTERNAL(scene_animation);
    };

    //! \brief The type of a \a scene_node.
    //! \details Bitset.
    enum class node_type : uint8
    {
        empty_leaf           = 0,
        is_parent            = 1 << 0,
        model                = 1 << 1,
        mesh                 = 1 << 2,
        camera               = 1 << 3,
        light                = 1 << 4,
        skin                 = 1 << 5,
        joint                = 1 << 6
    };
    MANGO_ENABLE_BITMASK_OPERATIONS(node_type)

    //! \brief An internal \a node.
    struct scene_node
    {
        //! \brief The type of the \a scene_node.
        node_type type;

        //! \brief The public \a node data.
        node public_data;

        //! \brief The \a sid of the nodes \a scene_transform.
        sid node_transform;

        //! \brief The number of the nodes children.
        int32 children;

        //! \brief The local transformation matrix.
        //! \details Relative to the parent.
        mat4 local_transformation_matrix;
        //! \brief The local transformation matrix.
        //! \details Relative to the world.
        mat4 global_transformation_matrix;

        //! \brief The \a sid of the nodes \a scene_mesh, or invalid_sid.
        sid mesh_id;
        //! \brief The \a sid of the nodes \a scene_camera, or invalid_sid.
        sid camera_id;
        //! \brief The \a sid of the nodes \a scene_lights, or invalid_sid.
        //! \details Ordered by type: 0 = directional, 1 = skylight, 2 = atmospheric_light.
        sid light_ids[3]; // Accessed via type.

        //! \brief The \a sid of the nodes \a scene_model, or invalid_sid.
        sid model_id;

        //! \brief The \a sid of the nodes \a scene_skin, or invalid_sid.
        sid skin_id;
        //! \brief The \a sid of the nodes \a scene_joint, or invalid_sid.
        sid joint_id;
        //! \brief The \a sid of the nodes \a scene_animation_controller, or invalid_sid.
        sid animation_controller_id;

        scene_node()
            : type(node_type::empty_leaf)
            , children(0)
            , local_transformation_matrix(1.0f)
            , global_transformation_matrix(1.0f)
        {
            light_ids[0] = light_ids[1] = light_ids[2] = invalid_sid;
        }
        //! \brief The \a scene_node is an internal scene structure.
        DECLARE_SCENE_INTERNAL(scene_node);
    };

    //! \brief An internal structure holding data for rendering.
    struct scene_render_instance
    {
        //! \brief The \a sid of the \a scene_render_instances \a scene_node.
        sid node_id;

        scene_render_instance() = default;
        //! \brief Constructs a \a scene_render_instance with a \a scene_node.
        //! \param[in] node The \a sid of the \a scene_node to construct the \a scene_render_instance with.
        scene_render_instance(const sid node)
            : node_id(node)
        {
        }
        //! \brief The \a scene_render_instance is an internal scene structure.
        DECLARE_SCENE_INTERNAL(scene_render_instance);
    };

    //! \brief An internal \a scenario.
    struct scene_scenario
    {
        //! \brief The public \a scenario data.
        scenario public_data;

        //! \brief The \a sids of all \a scene_nodes contained by this \a scene_scenario.
        std::vector<sid> nodes;

        scene_scenario() = default;
        //! \brief The \a scene_scenario is an internal scene structure.
        DECLARE_SCENE_INTERNAL(scene_scenario);
    };

    //! \brief An internal \a model.
    struct scene_model
    {
        //! \brief The public \a model data.
        model public_data;

        scene_model() = default;
        //! \brief The \a scene_model is an internal scene structure.
        DECLARE_SCENE_INTERNAL(scene_model);
    };

#undef DECLARE_SCENE_INTERNAL
} // namespace mango

#endif // MANGO_SCENE_INTERNALS