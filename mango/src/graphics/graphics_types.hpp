//! \file      graphics_types.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_GRAPHICS_TYPES_HPP
#define MANGO_GRAPHICS_TYPES_HPP

//! \cond NO_COND
#define GLM_FORCE_SILENT_WARNINGS 1
//! \endcond
#include <glm/gtx/matrix_major_storage.hpp>
#include <mango/types.hpp>

namespace mango
{
    //
    // Enum classes and types. Everything in here has suffix gfx_.
    //

    //! \brief A shared handle for a \a graphics_device_object.
    template <typename T>
    using gfx_handle = std::shared_ptr<T>;

    //! \brief Creates a \a gfx_handle of a \a graphics_device_object.
    //! \param[in]  args  Arguments for the \a T object's constructor.
    //! \return A \a gfx_handle that owns the newly created object.
    template <typename T, typename... Args>
    gfx_handle<T> make_gfx_handle(Args&&... args)
    {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

    //! \brief Cast a \a graphics_device_object managed by a \a gfx_handle moving the \a gfx_handle.
    //! \param[in]  old \a gfx_handle of type \a F to cast from.
    //! \return A \a gfx_handle holding the object pointer casted from \a F to \a T.
    template <typename T, typename F>
    gfx_handle<T> static_gfx_handle_cast(gfx_handle<F>&& old)
    {
        return std::static_pointer_cast<T>(std::move(old));
    }

    //! \brief Cast a \a graphics_device_object managed by a \a gfx_handle.
    //! \param[in]  old \a gfx_handle of type \a F to cast from.
    //! \return A \a gfx_handle holding the object pointer casted from \a F to \a T.
    template <typename T, typename F>
    gfx_handle<T> static_gfx_handle_cast(const gfx_handle<F>& old)
    {
        return std::static_pointer_cast<T>(old);
    }

    //! \brief A unique identifier for \a graphics_device_objects.
    using gfx_uid = int64;

    //! \brief An invalid \a gfx_uid.
    static const gfx_uid invalid_uid = -1;

    //! \brief Interface for all objects on the gpu or interactong with the gpu.
    class gfx_device_object
    {
      public:
        //! \brief Queries an integer type id for the specific \a gfx_device_object.
        //! \return An integer type id for the specific \a gfx_device_object.
        virtual int32 get_type_id() const = 0;
        //! \brief Returns the native handle for the specific \a gfx_device_object.
        //! \return A void* representing the native handle of the specific \a gfx_device_object.
        virtual void* native_handle() const = 0;

        //! \brief Queries an unique identifier for the specific \a gfx_device_object.
        //! \return An unique identifier for the specific \a gfx_device_object.
        gfx_uid get_uid() const
        {
            return uid;
        }

        // void invalidate_uid()
        // {
        //     int32 low  = get_uid_low();
        //     int32 high = get_uid_high();
        //
        //     set_uid(low + 1, high);
        // }

      private:
        //! \brief The unique identifier of the specific \a gfx_device_object.
        gfx_uid uid;

      protected:
        gfx_device_object()
        {
            static int32 uid_p0 = 0;

            set_uid(uid_p0, 0);

            uid_p0++;
        }

        //! \brief Sets the unique identifier of the specific \a gfx_device_object.
        //! \param[in] low The 32 least significant bits of the \a gfx_uid.
        //! \param[in] high The 32 most significant bits of the \a gfx_uid.
        void set_uid(int32 low, int32 high)
        {
            uid = (((uint64_t)high) << 32) | ((uint64_t)low);
        }

        //! \brief Gets the 32 most significant bits of the \a gfx_uid.
        //! \return The 32 most significant bits of the \a gfx_uid.
        int32 get_uid_high()
        {
            return uid >> 32;
        }

        //! \brief Gets the 32 least significant bits of the \a gfx_uid.
        //! \return The 32 least significant bits of the \a gfx_uid.
        int32 get_uid_low()
        {
            return static_cast<int32>(uid);
        }
    };

    //! \brief Type used to identify shader stages.
    enum class gfx_shader_stage_type : uint8
    {
        shader_stage_unknown                = 0,
        shader_stage_vertex                 = 1 << 0,
        shader_stage_tesselation_control    = 1 << 1,
        shader_stage_tesselation_evaluation = 1 << 2,
        shader_stage_geometry               = 1 << 3,
        shader_stage_fragment               = 1 << 4,
        shader_stage_compute                = 1 << 5,
        shader_stage_last                   = shader_stage_compute
    };
    MANGO_ENABLE_BITMASK_OPERATIONS(gfx_shader_stage_type)

    //! \brief Shader resource types.
    enum class gfx_shader_resource_type : uint8
    {
        shader_resource_unknown = 0,
        shader_resource_constant_buffer,  //! Uniform buffer
        shader_resource_texture,          //! Sampled image
        shader_resource_image_storage,    //! Image store
        shader_resource_buffer_storage,   //! Storage buffer
        shader_resource_sampler,          //! Seperate texture sampler
        shader_resource_input_attachment, //! Special type used for render pass input attachments
        shader_resource_last = shader_resource_input_attachment
    };

    //! \brief Shader resource access.
    enum class gfx_shader_resource_access : uint8
    {
        shader_access_unknown = 0,
        shader_access_static,
        shader_access_dynamic,
        shader_access_last = shader_access_dynamic
    };

    //! \brief Describes the topology of primitives used for rendering and interpreting geometry data.
    enum class gfx_primitive_topology : uint8
    {
        primitive_topology_unknown = 0,
        primitive_topology_point_list,
        primitive_topology_line_list,
        primitive_topology_line_loop,
        primitive_topology_line_strip,
        primitive_topology_triangle_list,
        primitive_topology_triangle_strip,
        primitive_topology_triangle_fan,
        primitive_topology_last = primitive_topology_triangle_fan
    };

    //! \brief Describes the rate at which an vertex attribute is changed.
    enum class gfx_vertex_input_rate : uint8
    {
        per_vertex,
        per_instance
    };

    //! \brief All kinds of format values.
    //! \details These are OpenGl values for now.
    enum class gfx_format : uint32
    {
        invalid = 0x0,
        // vertex attribute formats and buffer format types
        t_byte                        = 0x1400,
        t_unsigned_byte               = 0x1401,
        t_short                       = 0x1402,
        t_unsigned_short              = 0x1403,
        t_half_float                  = 0x140b,
        t_double                      = 0x140a,
        t_fixed                       = 0x140c,
        t_float                       = 0x1406,
        t_float_vec2                  = 0x8b50,
        t_float_vec3                  = 0x8b51,
        t_float_vec4                  = 0x8b52,
        t_int                         = 0x1404,
        t_int_vec2                    = 0x8b53,
        t_int_vec3                    = 0x8b54,
        t_int_vec4                    = 0x8b55,
        t_unsigned_int                = 0x1405,
        t_unsigned_int_vec2           = 0x8dc6,
        t_unsigned_int_vec3           = 0x8dc7,
        t_unsigned_int_vec4           = 0x8dc8,
        t_unsigned_byte_3_3_2         = 0x8032,
        t_unsigned_byte_2_3_3_rev     = 0x8362,
        t_unsigned_short_5_6_5        = 0x8363,
        t_unsigned_short_5_6_5_rev    = 0x8364,
        t_unsigned_short_4_4_4_4      = 0x8033,
        t_unsigned_short_4_4_4_4_rev  = 0x8365,
        t_unsigned_short_5_5_5_1      = 0x8034,
        t_unsigned_short_1_5_5_5_rev  = 0x8366,
        t_unsigned_int_8_8_8_8        = 0x8035,
        t_unsigned_int_8_8_8_8_rev    = 0x8367,
        t_unsigned_int_10_10_10_2     = 0x8036,
        t_unsigned_int_2_10_10_10_rev = 0x8368,
        t_int_2_10_10_10_rev          = 0x8d9f,
        // internal_formats
        r8                 = 0x8229,
        r16                = 0x822a,
        r16f               = 0x822d,
        r32f               = 0x822e,
        r8i                = 0x8231,
        r16i               = 0x8233,
        r32i               = 0x8235,
        r8ui               = 0x8232,
        r16ui              = 0x8234,
        r32ui              = 0x8236,
        rg8                = 0x822b,
        rg16               = 0x822c,
        rg16f              = 0x822f,
        rg32f              = 0x8230,
        rg8i               = 0x8237,
        rg16i              = 0x8239,
        rg32i              = 0x823b,
        rg8ui              = 0x8238,
        rg16ui             = 0x823a,
        rg32ui             = 0x823c,
        rgb4               = 0x804f,
        rgb5               = 0x8050,
        rgb8               = 0x8051,
        rgb10              = 0x8052,
        rgb12              = 0x8053,
        rgb16              = 0x8054,
        srgb8              = 0x8c41,
        srgb8_alpha8       = 0x8c43,
        rgb8ui             = 0x8d7d,
        rgb8i              = 0x8d8f,
        rgb16f             = 0x881b,
        rgb16ui            = 0x8d77,
        rgb16i             = 0x8d89,
        rgb32f             = 0x8815,
        rgb32i             = 0x8d83,
        rgb32ui            = 0x8d71,
        rgba2              = 0x8055,
        rgba4              = 0x8056,
        rgb5_a1            = 0x8057,
        rgba8              = 0x8058,
        rgb10_a2           = 0x8059,
        rgba12             = 0x805a,
        rgba16             = 0x805b,
        rgba16f            = 0x881a,
        rgba32f            = 0x8814,
        rgba8i             = 0x8d8e,
        rgba16i            = 0x8d88,
        rgba32i            = 0x8d82,
        rgba8ui            = 0x8d7c,
        rgba16ui           = 0x8d76,
        rgba32ui           = 0x8d70,
        depth_component32f = 0x8cac,
        depth_component16  = 0x81a5,
        depth_component24  = 0x81a6,
        depth_component32  = 0x81a7,
        depth24_stencil8   = 0x88f0,
        depth32f_stencil8  = 0x8cad,
        // pixel formats
        depth_component = 0x1902,
        stencil_index   = 0x1901,
        depth_stencil   = 0x84f9,
        red             = 0x1903,
        green           = 0x1904,
        blue            = 0x1905,
        rg              = 0x8227,
        rgb             = 0x1907,
        bgr             = 0x80e0,
        rgba            = 0x1908,
        bgra            = 0x80e1,
        red_integer     = 0x8d94,
        green_integer   = 0x8d95,
        blue_integer    = 0x8d96,
        rg_integer      = 0x8228,
        rgb_integer     = 0x8d98,
        bgr_integer     = 0x8d9a,
        rgba_integer    = 0x8d99,
        bgra_integer    = 0x8d9b,
        format_last     = bgra_integer
    };

    //! \brief Describes a viewport.
    struct gfx_viewport
    {
        float x;      //!< The upper left corner x position.
        float y;      //!< The upper left corner y position.
        float width;  //!< The viewport width.
        float height; //!< The viewport height.
    };

    //! \brief Describes a scissor rectangle.
    struct gfx_scissor_rectangle
    {
        int32 x_offset; //!< The upper left corner x offset.
        int32 y_offset; //!< The upper left corner y offset.
        int32 x_extent; //!< The extent in x direction.
        int32 y_extent; //!< The extent in y direction.
    };

    //! \brief Describes how a polygon should be drawn.
    enum class gfx_polygon_mode : uint8
    {
        polygon_mode_unknown = 0,
        polygon_mode_fill,
        polygon_mode_line,
        polygon_mode_point,
        polygon_mode_last = polygon_mode_point
    };

    //! \brief Describing the cull mode and face.
    enum class gfx_cull_mode_flag_bits : uint8
    {
        mode_none           = 0,
        mode_back           = 1 << 0,
        mode_front          = 1 << 1,
        mode_front_and_back = mode_back | mode_front,
        mode_last           = mode_front_and_back
    };
    MANGO_ENABLE_BITMASK_OPERATIONS(gfx_cull_mode_flag_bits)

    //! \brief Describes the order vertices required to be seen as front- or back-facing.
    enum class gfx_front_face : uint8
    {
        counter_clockwise,
        clockwise
    };

    //! \brief Describes the order vertices required to be seen as front- or back-facing.
    enum class gfx_sample_count : uint8
    {
        sample_unknown = 0,
        sample_1_bit,
        sample_2_bit,
        sample_4_bit,
        sample_8_bit,
        sample_16_bit,
        sample_32_bit,
        sample_64_bit,
        sample_last = sample_64_bit
    };

    //! \brief Compare operator used for depth and stencil tests.
    enum class gfx_compare_operator : uint8
    {
        compare_operator_unknown = 0,
        compare_operator_never,
        compare_operator_less,
        compare_operator_equal,
        compare_operator_less_equal,
        compare_operator_greater,
        compare_operator_not_equal,
        compare_operator_greater_equal,
        compare_operator_always,
        compare_operator_last = compare_operator_always
    };

    //! \brief Stencil operations used for stencil test.
    enum class gfx_stencil_operation : uint8
    {
        stencil_operation_unknown = 0,
        stencil_operation_keep,
        stencil_operation_zero,
        stencil_operation_replace,
        stencil_operation_increment_and_clamp,
        stencil_operation_decrement_and_clamp,
        stencil_operation_increment_and_wrap,
        stencil_operation_decrement_and_wrap,
        stencil_operation_invert,
        stencil_operation_last = stencil_operation_invert
    };

    //! \brief The blend factor used for blend operations.
    enum class gfx_blend_factor : uint8
    {
        blend_factor_unknown = 0,
        blend_factor_zero,
        blend_factor_one,
        blend_factor_src_color,
        blend_factor_one_minus_src_color,
        blend_factor_dst_color,
        blend_factor_one_minus_dst_color,
        blend_factor_src_alpha,
        blend_factor_one_minus_src_alpha,
        blend_factor_dst_alpha,
        blend_factor_one_minus_dst_alpha,
        blend_factor_constant_color,
        blend_factor_one_minus_constant_color,
        blend_factor_constant_alpha,
        blend_factor_one_minus_constant_alpha,
        blend_factor_src_alpha_saturate,
        blend_factor_src1_color,
        blend_factor_one_minus_src1_color,
        blend_factor_src1_alpha,
        blend_factor_one_minus_src1_alpha,
        blend_factor_last = blend_factor_one_minus_src1_alpha
    };

    //! \brief The blend operations.
    enum class gfx_blend_operation : uint8
    {
        blend_operation_unknown = 0,
        blend_operation_add,
        blend_operation_subtract,
        blend_operation_reverse_subtract,
        blend_operation_take_min,
        blend_operation_take_max,
        blend_operation_last = blend_operation_take_max
    };

    //! \brief Describing framebuffer logical operations.
    enum class gfx_logic_operator : uint8
    {
        logic_unknown       = 0,
        logic_clear         = 1,
        logic_and           = 2,
        logic_and_reverse   = 3,
        logic_copy          = 4,
        logic_and_inverted  = 5,
        logic_no_op         = 6,
        logic_xor           = 7,
        logic_or            = 8,
        logic_nor           = 9,
        logic_equivalent    = 10,
        logic_invert        = 11,
        logic_or_reverse    = 12,
        logic_copy_inverted = 13,
        logic_or_inverted   = 14,
        logic_nand          = 15,
        logic_set           = 16,
        logic_last          = logic_set
    };

    //! \brief Describing the update face for dynamic stencil mask updates.
    enum class gfx_stencil_face_flag_bits : uint8
    {
        stencil_face_none               = 0,
        stencil_face_back_bit           = 1 << 0,
        stencil_face_front_bit          = 1 << 1,
        stencil_face_front_and_back_bit = stencil_face_back_bit | stencil_face_front_bit,
        stencil_face_last               = stencil_face_front_and_back_bit
    };
    MANGO_ENABLE_BITMASK_OPERATIONS(gfx_stencil_face_flag_bits)

    //! \brief Describing a selection of color components.
    enum class gfx_color_component_flag_bits : uint8
    {
        component_none  = 0,
        component_r     = 1 << 0,
        component_g     = 1 << 1,
        component_b     = 1 << 2,
        component_a     = 1 << 3,
        components_rgb  = component_r | component_g | component_r,
        components_rgba = components_rgb | component_a,
        components_last = components_rgba
    };
    MANGO_ENABLE_BITMASK_OPERATIONS(gfx_color_component_flag_bits)

    //! \brief Indicate which dynamic state is taken from dynamic state commands.
    //! \details Can be extended in the future.
    enum class gfx_dynamic_state_flag_bits : uint16
    {
        dynamic_state_none                           = 0,
        dynamic_state_viewport                       = 1 << 0,
        dynamic_state_scissor                        = 1 << 1,
        dynamic_state_line_width                     = 1 << 2,
        dynamic_state_depth_bias                     = 1 << 3,
        dynamic_state_blend_constants                = 1 << 4,
        dynamic_state_stencil_compare_mask_reference = 1 << 5,
        dynamic_state_stencil_write_mask             = 1 << 6,
        dynamic_state_last                           = dynamic_state_stencil_write_mask
    };
    MANGO_ENABLE_BITMASK_OPERATIONS(gfx_dynamic_state_flag_bits)

    //! \brief Specification of barrier bits.
    enum class gfx_barrier_bit : uint16
    {
        unknown_barrier_bit             = 0,
        vertex_attrib_array_barrier_bit = 1 << 0,
        element_array_barrier_bit       = 1 << 1,
        uniform_barrier_bit             = 1 << 2,
        texture_fetch_barrier_bit       = 1 << 3,
        shader_image_access_barrier_bit = 1 << 4,
        command_barrier_bit             = 1 << 5,
        pixel_buffer_barrier_bit        = 1 << 6,
        texture_update_barrier_bit      = 1 << 7,
        buffer_update_barrier_bit       = 1 << 8,
        framebuffer_barrier_bit         = 1 << 9,
        transform_feedback_barrier_bit  = 1 << 10,
        atomic_counter_barrier_bit      = 1 << 11,
        shader_storage_barrier_bit      = 1 << 12,
        query_buffer_barrier_bit        = 1 << 13,
        last_barrier_bit                = query_buffer_barrier_bit
    };
    MANGO_ENABLE_BITMASK_OPERATIONS(gfx_barrier_bit)

    //! \brief Bit specification for clearing attachments.
    enum class gfx_clear_attachment_flag_bits : uint8
    {
        clear_flag_none                 = 0,
        clear_flag_draw_buffer0         = 1 << 0,
        clear_flag_draw_buffer1         = 1 << 1,
        clear_flag_draw_buffer2         = 1 << 2,
        clear_flag_draw_buffer3         = 1 << 3,
        clear_flag_draw_buffer4         = 1 << 4,
        clear_flag_draw_buffer5         = 1 << 5,
        clear_flag_depth_buffer         = 1 << 6,
        clear_flag_stencil_buffer       = 1 << 7,
        clear_flag_all_draw_buffers     = clear_flag_draw_buffer0 | clear_flag_draw_buffer1 | clear_flag_draw_buffer2,
        clear_flag_depth_stencil_buffer = clear_flag_depth_buffer | clear_flag_stencil_buffer,
        clear_flag_last                 = clear_flag_depth_stencil_buffer
    };
    MANGO_ENABLE_BITMASK_OPERATIONS(gfx_clear_attachment_flag_bits)

    //! \brief The targets buffer can be bound to.
    enum class gfx_buffer_target : uint8
    {
        buffer_target_unknown = 0,
        buffer_target_vertex,
        buffer_target_index,
        buffer_target_uniform,
        buffer_target_shader_storage,
        buffer_target_texture,
        buffer_target_last = buffer_target_texture
    };

    //! \brief Bit specification providing access information for buffers.
    enum class gfx_buffer_access : uint8
    {
        buffer_access_none                     = 0,
        buffer_access_dynamic_storage          = 1 << 0,
        buffer_access_mapped_access_read       = 1 << 1,
        buffer_access_mapped_access_write      = 1 << 2,
        buffer_access_mapped_access_read_write = buffer_access_mapped_access_read | buffer_access_mapped_access_write,
        buffer_access_last                     = buffer_access_mapped_access_read_write
    };
    MANGO_ENABLE_BITMASK_OPERATIONS(gfx_buffer_access)

    //! \brief The type of textures.
    enum class gfx_texture_type : uint8
    {
        texture_type_unknown = 0,
        texture_type_1d,
        texture_type_2d,
        texture_type_3d,
        texture_type_1d_array,
        texture_type_2d_array,
        texture_type_cube_map,
        texture_type_cube_map_array,
        texture_type_rectangle,
        texture_type_buffer,
        texture_type_2d_multisample,
        texture_type_2d_multisample_array,
        texture_type_last = texture_type_2d_multisample_array
    };

    //! \brief The filter possibilities for samplers.
    enum class gfx_sampler_filter : uint8
    {
        sampler_filter_unknown = 0,
        sampler_filter_nearest,
        sampler_filter_linear,
        sampler_filter_nearest_mipmap_nearest,
        sampler_filter_linear_mipmap_nearest,
        sampler_filter_nearest_mipmap_linear,
        sampler_filter_linear_mipmap_linear,
        sampler_filter_last = sampler_filter_linear_mipmap_linear
    };

    //! \brief Defines sampler edge case handling.
    enum class gfx_sampler_edge_wrap : uint8
    {
        sampler_edge_wrap_unknown = 0,
        sampler_edge_wrap_repeat,
        sampler_edge_wrap_repeat_mirrored,
        sampler_edge_wrap_clamp_to_edge,
        sampler_edge_wrap_clamp_to_border,
        sampler_edge_wrap_clamp_to_edge_mirrored,
        sampler_edge_wrap_last = sampler_edge_wrap_clamp_to_edge_mirrored
    };

    // TODO Should these be in here?
    //! \brief A boolean in the glsl std140 layout.
    struct std140_bool
    {
        //! \cond NO_COND
        std140_bool(const bool& b)
        {
            pad = 0;
            v   = b ? 1 : 0;
        }
        std140_bool()
            : pad(0)
        {
        }
        operator bool&()
        {
            return v;
        }
        void operator=(const bool& o)
        {
            pad = 0;
            v   = o;
        }

      private:
        union
        {
            bool v;
            int32 pad;
        };
        //! \endcond
    };

    //! \brief An integer in the glsl std140 layout.
    struct std140_int
    {
        //! \cond NO_COND
        std140_int(const int& i)
        {
            v = i;
        }
        std140_int()
            : v(0)
        {
        }
        operator int32&()
        {
            return v;
        }
        void operator=(const int& o)
        {
            v = o;
        }

      private:
        int32 v;
        //! \endcond
    };

    //! \brief A float in the glsl std140 layout.
    struct std140_float
    {
        //! \cond NO_COND
        std140_float(const float& f)
        {
            v = f;
        }
        std140_float()
            : v(0)
        {
        }
        operator float&()
        {
            return v;
        }
        void operator=(const float& o)
        {
            v = o;
        }

      private:
        float v;
        //! \endcond
    };

    //! \brief A float in the glsl std140 layout for arrays.
    struct std140_float_array
    {
        //! \cond NO_COND
        std140_float_array(const float& f)
        {
            v = f;
        }
        std140_float_array()
            : v(0)
        {
        }
        operator float&()
        {
            return v;
        }
        void operator=(const float& o)
        {
            v = o;
        }

      private:
        float v;
        float p0 = 0.0f;
        float p1 = 0.0f;
        float p2 = 0.0f;
        //! \endcond
    };

    //! \brief A vec2 in the glsl std140 layout.
    struct std140_vec2
    {
        //! \cond NO_COND
        std140_vec2(const vec2& vec)
            : v(vec)
        {
        }
        std140_vec2()
            : v(vec2(0.0f))
        {
        }
        operator vec2&()
        {
            return v;
        }
        void operator=(const vec2& o)
        {
            v = o;
        }
        float& operator[](int i)
        {
            return v[i];
        }

      private:
        vec2 v;
        //! \endcond
    };

    //! \brief A vec3 in the glsl std140 layout.
    struct std140_vec3
    {
        //! \cond NO_COND
        std140_vec3(const vec3& vec)
            : v(vec)
        {
        }
        std140_vec3()
            : v(vec3(0.0f))
        {
        }
        operator vec3&()
        {
            return v;
        }
        void operator=(const vec3& o)
        {
            v = o;
        }
        float& operator[](int i)
        {
            return v[i];
        }

      private:
        vec3 v;
        float pad = 0.0f;
        //! \endcond
    };

    //! \brief A vec4 in the glsl std140 layout.
    struct std140_vec4
    {
        //! \cond NO_COND
        std140_vec4(const vec4& vec)
            : v(vec)
        {
        }
        std140_vec4()
            : v(vec4(0.0f))
        {
        }
        operator vec4&()
        {
            return v;
        }
        void operator=(const vec4& o)
        {
            v = o;
        }
        float& operator[](int i)
        {
            return v[i];
        }

      private:
        vec4 v;
        //! \endcond
    };

    //! \brief A mat3 in the glsl std140 layout.
    struct std140_mat3
    {
        //! \cond NO_COND
        std140_mat3(const mat3& mat)
            : c0(mat[0])
            , c1(mat[1])
            , c2(mat[2])
        {
        }
        std140_mat3()
            : c0()
            , c1()
            , c2()
        {
        }
        void operator=(const mat3& o)
        {
            c0 = o[0];
            c1 = o[1];
            c2 = o[2];
        }
        vec3& operator[](int i)
        {
            switch (i)
            {
            case 0:
                return c0;
            case 1:
                return c1;
            case 2:
                return c2;
            default:
                MANGO_ASSERT(false, "3D Matrix has only 3 columns!"); // TODO Paul: Ouch!
            }
        }
        operator mat3()
        {
            return glm::colMajor3(vec3(c0), vec3(c1), vec3(c2));
        }

      private:
        std140_vec3 c0;
        std140_vec3 c1;
        std140_vec3 c2;
        //! \endcond
    };

    //! \brief A mat4 in the glsl std140 layout.
    struct std140_mat4
    {
        //! \cond NO_COND
        std140_mat4(const mat4& mat)
            : c0(mat[0])
            , c1(mat[1])
            , c2(mat[2])
            , c3(mat[3])
        {
        }
        std140_mat4()
            : c0()
            , c1()
            , c2()
            , c3()
        {
        }
        void operator=(const mat4& o)
        {
            c0 = o[0];
            c1 = o[1];
            c2 = o[2];
            c3 = o[3];
        }
        vec4& operator[](int i)
        {
            switch (i)
            {
            case 0:
                return c0;
            case 1:
                return c1;
            case 2:
                return c2;
            case 3:
                return c3;
            default:
                MANGO_ASSERT(false, "4D Matrix has only 4 columns!"); // TODO Paul: Ouch!
            }
        }
        operator mat4()
        {
            return glm::colMajor4(vec4(c0), vec4(c1), vec4(c2), vec4(c3));
        }

      private:
        std140_vec4 c0;
        std140_vec4 c1;
        std140_vec4 c2;
        std140_vec4 c3;
        //! \endcond
    };

} // namespace mango

#endif // MANGO_GRAPHICS_TYPES_HPP
