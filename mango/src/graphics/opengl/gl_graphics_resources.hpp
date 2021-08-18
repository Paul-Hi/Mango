//! \file      gl_graphics_resources.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_GL_GRAPHICS_RESOURCES_HPP
#define MANGO_GL_GRAPHICS_RESOURCES_HPP

#include <glad/glad.h>
#include <graphics/graphics_resources.hpp>
#include <vector>

namespace mango
{
    //! \brief Handle of opengl resources.
    using gl_handle = uint32;
    //! \brief Opengl synchronization structure.
    using gl_sync = void*;

    //! \brief An opengl \a gfx_shader_stage.
    class gl_shader_stage : public gfx_shader_stage
    {
      public:
        //! \brief Constructs a \a gl_shader_stage.
        //! \param[in] info The \a shader_stage_create_info providing info for creation.
        gl_shader_stage(const shader_stage_create_info& info);

        ~gl_shader_stage();
        void* native_handle() const override
        {
            return (void*)(uintptr)m_shader_stage_gl_handle;
        }

        //! \brief The \a shader_stage_create_info used for creation.
        shader_stage_create_info m_info;
        //! \brief The native opengl handle.
        gl_handle m_shader_stage_gl_handle = 0;

      private:
        //! \brief Creates the shader stage from a shader source.
        void create_shader_from_source();
        // void reflect();
    };

    //! \brief An opengl \a gfx_buffer.
    class gl_buffer : public gfx_buffer
    {
      public:
        //! \brief Constructs a \a gl_buffer.
        //! \param[in] info The \a buffer_create_info providing info for creation.
        gl_buffer(const buffer_create_info& info);
        ~gl_buffer();
        void* native_handle() const override
        {
            return (void*)(uintptr)m_buffer_gl_handle;
        }

        //! \brief Creates a dummy \a gl_buffer.
        //! \return A \a gl_buffer without any gpu interaction.
        static gl_buffer dummy(); // TODO Paul: This could really be done better!

        //! \brief The \a buffer_create_info used for creation.
        buffer_create_info m_info;
        //! \brief The native opengl handle.
        gl_handle m_buffer_gl_handle = 0;

      private:
        gl_buffer() = default;
    };

    //! \brief An opengl \a gfx_texture.
    class gl_texture : public gfx_texture
    {
      public:
        //! \brief Constructs a \a gl_texture.
        //! \param[in] info The \a texture_create_info providing info for creation.
        gl_texture(const texture_create_info& info);
        ~gl_texture();
        void* native_handle() const override
        {
            return (void*)(uintptr)m_texture_gl_handle;
        }

        const vec2 get_size() const override
        {
            return vec2(m_info.width, m_info.height); // TODO
        }

        const gfx_texture_type& get_type() const override
        {
            return m_info.texture_type; // TODO
        }

        //! \brief Creates a dummy \a gl_texture.
        //! \return A \a gl_texture without any gpu interaction.
        static gl_texture dummy(); // TODO Paul: This could really be done better!

        //! \brief The \a texture_create_info used for creation.
        texture_create_info m_info;
        //! \brief The native opengl handle.
        gl_handle m_texture_gl_handle = 0;

      private:
        friend class gl_graphics_device;
        gl_texture() = default;
    };

    //! \brief An opengl \a gfx_image_texture_view.
    class gl_image_texture_view : public gfx_image_texture_view
    {
      public:
        //! \brief Constructs a \a gl_image_texture_view.
        //! \param[in] texture The \a gfx_texture to create a view for.
        //! \param[in] level The level of the \a gfx_texture to create a view for.
        gl_image_texture_view(gfx_handle<const gfx_texture> texture, int32 level = 0)
            : m_texture(static_gfx_handle_cast<const gl_texture>(texture)) // TODO Paul!
            , m_level(level)
        {
        }
        ~gl_image_texture_view() = default;
        void* native_handle() const override
        {
            return m_texture->native_handle();
        }

        //! \brief The \a gfx_texture the view provides access for.
        gfx_handle<const gl_texture> m_texture;
        //! \brief The level of the \a gfx_texture the view was created for.
        int32 m_level = 0;
    };

    //! \brief An opengl \a gfx_sampler.
    class gl_sampler : public gfx_sampler
    {
      public:
        //! \brief Constructs a \a gl_sampler.
        //! \param[in] info The \a sampler_create_info providing info for creation.
        gl_sampler(const sampler_create_info& info);
        ~gl_sampler();
        void* native_handle() const override
        {
            return (void*)(uintptr)m_sampler_gl_handle;
        }

        //! \brief Creates a dummy \a gl_sampler.
        //! \return A \a gl_sampler without any gpu interaction.
        static gl_sampler dummy(); // TODO Paul: This could really be done better!

        //! \brief The \a sampler_create_info used for creation.
        sampler_create_info m_info;
        //! \brief The native opengl handle.
        gl_handle m_sampler_gl_handle = 0;

      private:
        gl_sampler() = default;
    };

    //! \brief An opengl \a gfx_semaphore.
    class gl_semaphore : public gfx_semaphore
    {
      public:
        //! \brief Constructs a \a gl_semaphore.
        //! \param[in] info The \a semaphore_create_info providing info for creation.
        gl_semaphore(const semaphore_create_info& info);
        ~gl_semaphore();
        void* native_handle() const override
        {
            return (void*)(uintptr)m_semaphore_gl_handle;
        }

        //! \brief The \a semaphore_create_info used for creation.
        semaphore_create_info m_info;
        //! \brief The \a gl_sync attached to this semaphore.
        gl_sync m_semaphore_gl_handle = 0;
    };

    //! \brief An opengl \a shader_resource_mapping.
    class gl_shader_resource_mapping : public shader_resource_mapping
    {
      public:
        //! \brief List of \a gl_buffers.
        std::vector<resource_pair<const gl_buffer>> m_buffers;
        //! \brief List of \a gl_textures.
        std::vector<resource_pair<const gl_texture>> m_textures;
        //! \brief List of \a gl_samplers.
        std::vector<resource_pair<const gl_sampler>> m_samplers;
        //! \brief List of \a gl_image_texture_views.
        std::vector<resource_pair<const gl_image_texture_view>> m_texture_images;

        bool set(const string variable_name, gfx_handle<const gfx_device_object> resource) override;
    };

    //! \brief An opengl \a pipeline_resource_layout.
    class gl_pipeline_resource_layout : public pipeline_resource_layout
    {
      public:
        //! \brief Constructs a \a gl_pipeline_resource_layout.
        //! \param[in] bindings A list of \a shader_resource_bindings the layout consists of.
        gl_pipeline_resource_layout(std::initializer_list<shader_resource_binding> bindings);

        //! \brief List of \a shader_resource_bindings the layout consists of.
        std::vector<shader_resource_binding> m_bindings;
    };

    //! \brief An opengl \a gfx_pipeline interface.
    class gl_pipeline : public gfx_pipeline
    {
      public:
        gfx_handle<shader_resource_mapping> get_resource_mapping() const override;
        void submit_pipeline_resources(gfx_handle<gfx_graphics_state> shared_graphics_state) const override;
        void* native_handle() const override
        {
            return nullptr; // TODO Paul: Is that fine?
        }

      protected:
        gl_pipeline() = default;

        //! \brief Queries all shader resource information for a given binding and type.
        //! \param[in] binding The binding of the shader resource to query.
        //! \param[in] shader_info The \a shader_create_info including informations for all shader stages.
        //! \param[in] type The type of the shader resource to query.
        //! \param[out] array_size The number of array entries of the shader resource to query.
        //! \return The variables name in the shader.
        const char* query_shader_info(int32 binding, const shader_stage_create_info& shader_info, gfx_shader_resource_type type, int32& array_size);

        //! \brief The \a gl_shader_resource_mapping of the \a gl_pipeline.
        gfx_handle<gl_shader_resource_mapping> m_mapping;
    };

    //! \brief An opengl graphics \a gfx_pipeline.
    class gl_graphics_pipeline : public gl_pipeline
    {
      public:
        //! \brief Constructs a \a gl_graphics_pipeline.
        //! \details Builds the mapping on construction.
        //! \param[in] info The \a graphics_pipeline_create_info describing the graphics pipeline.
        gl_graphics_pipeline(const graphics_pipeline_create_info& info);

        //! \brief The \a graphics_pipeline_create_info used for creation.
        graphics_pipeline_create_info m_info;
    };

    //! \brief An opengl compute \a gfx_pipeline.
    class gl_compute_pipeline : public gl_pipeline
    {
      public:
        //! \brief Constructs a \a gl_compute_pipeline.
        //! \details Builds the mapping on construction.
        //! \param[in] info The \a compute_pipeline_create_info describing the graphics pipeline.
        gl_compute_pipeline(const compute_pipeline_create_info& info);

        //! \brief The \a compute_pipeline_create_info used for creation.
        compute_pipeline_create_info m_info;
    };

    //
    // Required conversions.
    //

    //! \brief A opengl enumeration.
    using gl_enum = uint32;
    //! \brief A opengl bitfield.
    using gl_bitfield = uint32;

    //! \brief Converts a \a gfx_shader_stage_type to a \a gl_enum.
    //! \param[in] tp The \a gfx_shader_stage_type to convert.
    //! \return The \a gl_enum representing the given \a gfx_shader_stage_type.
    inline gl_enum gfx_shader_stage_type_to_gl(const gfx_shader_stage_type& tp)
    {
        switch (tp)
        {
        case gfx_shader_stage_type::shader_stage_vertex:
            return GL_VERTEX_SHADER;
        case gfx_shader_stage_type::shader_stage_tesselation_control:
            return GL_TESS_CONTROL_SHADER;
        case gfx_shader_stage_type::shader_stage_tesselation_evaluation:
            return GL_TESS_EVALUATION_SHADER;
        case gfx_shader_stage_type::shader_stage_geometry:
            return GL_GEOMETRY_SHADER;
        case gfx_shader_stage_type::shader_stage_fragment:
            return GL_FRAGMENT_SHADER;
        case gfx_shader_stage_type::shader_stage_compute:
            return GL_COMPUTE_SHADER;
        default:
            MANGO_ASSERT(false, "Unknown shader type!");
            return GL_NONE;
        }
    }

    //! \brief Converts \a gfx_buffer_access to a \a gl_bitfield.
    //! \param[in] ac The \a gfx_buffer_access to convert.
    //! \return The \a gl_bitfield representing the given \a gfx_buffer_access.
    inline gl_bitfield gfx_buffer_access_to_gl(const gfx_buffer_access& ac)
    {
        gl_bitfield access_bits = GL_NONE;
        if ((ac & gfx_buffer_access::buffer_access_dynamic_storage) != gfx_buffer_access::buffer_access_none)
        {
            access_bits |= GL_DYNAMIC_STORAGE_BIT;
        }
        if ((ac & gfx_buffer_access::buffer_access_mapped_access_read) != gfx_buffer_access::buffer_access_none)
        {
            access_bits |= GL_MAP_READ_BIT;
            access_bits |= GL_MAP_PERSISTENT_BIT;
            access_bits |= GL_MAP_COHERENT_BIT;
        }
        if ((ac & gfx_buffer_access::buffer_access_mapped_access_write) != gfx_buffer_access::buffer_access_none)
        {
            access_bits |= GL_MAP_WRITE_BIT;
            access_bits |= GL_MAP_PERSISTENT_BIT;
            access_bits |= GL_MAP_COHERENT_BIT;
        }
        return access_bits;
    }

    //! \brief Converts a \a gfx_texture_type to a \a gl_enum.
    //! \param[in] tp The \a gfx_texture_type to convert.
    //! \return The \a gl_enum representing the given \a gfx_texture_type.
    inline gl_enum gfx_texture_type_to_gl(const gfx_texture_type& tp)
    {
        switch (tp)
        {
        case gfx_texture_type::texture_type_1d:
            return GL_TEXTURE_1D;
        case gfx_texture_type::texture_type_2d:
            return GL_TEXTURE_2D;
        case gfx_texture_type::texture_type_3d:
            return GL_TEXTURE_3D;
        case gfx_texture_type::texture_type_1d_array:
            return GL_TEXTURE_1D_ARRAY;
        case gfx_texture_type::texture_type_2d_array:
            return GL_TEXTURE_2D_ARRAY;
        case gfx_texture_type::texture_type_cube_map:
            return GL_TEXTURE_CUBE_MAP;
        case gfx_texture_type::texture_type_cube_map_array:
            return GL_TEXTURE_CUBE_MAP_ARRAY;
        case gfx_texture_type::texture_type_rectangle:
            return GL_TEXTURE_RECTANGLE;
        case gfx_texture_type::texture_type_buffer:
            return GL_TEXTURE_BUFFER;
        case gfx_texture_type::texture_type_2d_multisample:
            return GL_TEXTURE_2D_MULTISAMPLE;
        case gfx_texture_type::texture_type_2d_multisample_array:
            return GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
        default:
            MANGO_ASSERT(false, "Unknown texture type!");
            return GL_NONE;
        }
    }

    //! \brief Converts a \a gfx_sampler_filter to a \a gl_enum.
    //! \param[in] filter The \a gfx_sampler_filter to convert.
    //! \return The \a gl_enum representing the given \a gfx_sampler_filter.
    inline gl_enum gfx_sampler_filter_to_gl(const gfx_sampler_filter& filter)
    {
        switch (filter)
        {
        case gfx_sampler_filter::sampler_filter_nearest:
            return GL_NEAREST;
        case gfx_sampler_filter::sampler_filter_linear:
            return GL_LINEAR;
        case gfx_sampler_filter::sampler_filter_nearest_mipmap_nearest:
            return GL_NEAREST_MIPMAP_NEAREST;
        case gfx_sampler_filter::sampler_filter_linear_mipmap_nearest:
            return GL_LINEAR_MIPMAP_NEAREST;
        case gfx_sampler_filter::sampler_filter_nearest_mipmap_linear:
            return GL_NEAREST_MIPMAP_LINEAR;
        case gfx_sampler_filter::sampler_filter_linear_mipmap_linear:
            return GL_LINEAR_MIPMAP_LINEAR;
        default:
            MANGO_ASSERT(false, "Unknown sampler filter!");
            return GL_NONE;
        }
    }

    //! \brief Converts a \a gfx_sampler_edge_wrap to a \a gl_enum.
    //! \param[in] wrap The \a gfx_sampler_edge_wrap to convert.
    //! \return The \a gl_enum representing the given \a gfx_sampler_edge_wrap.
    inline gl_enum gfx_sampler_edge_wrap_to_gl(const gfx_sampler_edge_wrap& wrap)
    {
        switch (wrap)
        {
        case gfx_sampler_edge_wrap::sampler_edge_wrap_repeat:
            return GL_REPEAT;
        case gfx_sampler_edge_wrap::sampler_edge_wrap_repeat_mirrored:
            return GL_MIRRORED_REPEAT_IBM;
        case gfx_sampler_edge_wrap::sampler_edge_wrap_clamp_to_edge:
            return GL_CLAMP_TO_EDGE;
        case gfx_sampler_edge_wrap::sampler_edge_wrap_clamp_to_border:
            return GL_CLAMP_TO_BORDER;
        case gfx_sampler_edge_wrap::sampler_edge_wrap_clamp_to_edge_mirrored:
            return GL_MIRROR_CLAMP_TO_EDGE;
        default:
            MANGO_ASSERT(false, "Unknown sampler edge wrap!");
            return GL_NONE;
        }
    }

    //! \brief Converts a \a gfx_compare_operator to a \a gl_enum.
    //! \param[in] op The \a gfx_compare_operator to convert.
    //! \return The \a gl_enum representing the given \a gfx_compare_operator.
    inline gl_enum gfx_compare_operator_to_gl(const gfx_compare_operator& op)
    {
        switch (op)
        {
        case gfx_compare_operator::compare_operator_never:
            return GL_NEVER;
        case gfx_compare_operator::compare_operator_less:
            return GL_LESS;
        case gfx_compare_operator::compare_operator_equal:
            return GL_EQUAL;
        case gfx_compare_operator::compare_operator_less_equal:
            return GL_LEQUAL;
        case gfx_compare_operator::compare_operator_greater:
            return GL_GREATER;
        case gfx_compare_operator::compare_operator_not_equal:
            return GL_NOTEQUAL;
        case gfx_compare_operator::compare_operator_greater_equal:
            return GL_GEQUAL;
        case gfx_compare_operator::compare_operator_always:
            return GL_ALWAYS;
        default:
            MANGO_ASSERT(false, "Unknown compare operator!");
            return GL_NONE;
        }
    }

    //! \brief Converts a \a gfx_buffer_target to a \a gl_enum.
    //! \param[in] target The \a gfx_buffer_target to convert.
    //! \return The \a gl_enum representing the given \a gfx_buffer_target.
    inline gl_enum gfx_buffer_target_to_gl(const gfx_buffer_target& target)
    {
        switch (target)
        {
        case gfx_buffer_target::buffer_target_vertex:
            return GL_ARRAY_BUFFER;
        case gfx_buffer_target::buffer_target_index:
            return GL_ELEMENT_ARRAY_BUFFER;
        case gfx_buffer_target::buffer_target_uniform:
            return GL_UNIFORM_BUFFER;
        case gfx_buffer_target::buffer_target_shader_storage:
            return GL_SHADER_STORAGE_BUFFER;
        case gfx_buffer_target::buffer_target_texture:
            return GL_TEXTURE_BUFFER;
        default:
            MANGO_ASSERT(false, "Unknown sampler filter type!");
            return GL_NONE;
        }
    }

    //! \brief Converts a \a gfx_format to a \a gl_enum.
    //! \param[in] format The \a gfx_format to convert.
    //! \return The \a gl_enum representing the given \a gfx_format.
    inline gl_enum gfx_format_to_gl(const gfx_format& format)
    {
        // Formats are formats of opengl atm.
        // TODO Paul: Validate!
        return static_cast<gl_enum>(format);
    }

    //! \brief Calculates the number of mipmap levels in opengl for a given width and height.
    //! \param[in] width The width to use for calculation.
    //! \param[in] height The height to use for calculation.
    //! \return The number of mipmap levels.
    inline int32 gfx_calculate_max_miplevels(int32 width, int32 height)
    {
        // This is the same calculation opengl is using internally.
        int32 levels = 1;
        while ((width | height) >> levels)
        {
            ++levels;
        }
        return levels;
    }

    //! \brief Converts a \a gfx_polygon_mode to a \a gl_enum.
    //! \param[in] mode The \a gfx_polygon_mode to convert.
    //! \return The \a gl_enum representing the given \a gfx_polygon_mode.
    inline gl_enum gfx_polygon_mode_to_gl(const gfx_polygon_mode& mode)
    {
        switch (mode)
        {
        case gfx_polygon_mode::polygon_mode_point:
            return GL_POINT;
        case gfx_polygon_mode::polygon_mode_line:
            return GL_LINE;
        case gfx_polygon_mode::polygon_mode_fill:
            return GL_FILL;
        default:
            MANGO_ASSERT(false, "Unknown polygon mode!");
            return GL_NONE;
        }
    }

    //! \brief Converts a \a gfx_stencil_operation to a \a gl_enum.
    //! \param[in] op The \a gfx_stencil_operation to convert.
    //! \return The \a gl_enum representing the given \a gfx_stencil_operation.
    inline gl_enum gfx_stencil_operation_to_gl(const gfx_stencil_operation& op)
    {
        switch (op)
        {
        case gfx_stencil_operation::stencil_operation_keep:
            return GL_KEEP;
        case gfx_stencil_operation::stencil_operation_zero:
            return GL_ZERO;
        case gfx_stencil_operation::stencil_operation_replace:
            return GL_REPLACE;
        case gfx_stencil_operation::stencil_operation_increment_and_clamp:
            return GL_INCR;
        case gfx_stencil_operation::stencil_operation_decrement_and_clamp:
            return GL_DECR;
        case gfx_stencil_operation::stencil_operation_increment_and_wrap:
            return GL_INCR_WRAP;
        case gfx_stencil_operation::stencil_operation_decrement_and_wrap:
            return GL_DECR_WRAP;
        case gfx_stencil_operation::stencil_operation_invert:
            return GL_INVERT;
        default:
            MANGO_ASSERT(false, "Unknown stencil operation!");
            return GL_NONE;
        }
    }

    //! \brief Converts a \a gfx_logic_operator to a \a gl_enum.
    //! \param[in] op The \a gfx_logic_operator to convert.
    //! \return The \a gl_enum representing the given \a gfx_logic_operator.
    inline gl_enum gfx_logic_operator_to_gl(const gfx_logic_operator& op)
    {
        switch (op)
        {
        case gfx_logic_operator::logic_clear:
            return GL_CLEAR;
        case gfx_logic_operator::logic_and:
            return GL_AND;
        case gfx_logic_operator::logic_and_reverse:
            return GL_AND_REVERSE;
        case gfx_logic_operator::logic_copy:
            return GL_COPY;
        case gfx_logic_operator::logic_and_inverted:
            return GL_AND_INVERTED;
        case gfx_logic_operator::logic_no_op:
            return GL_NOOP;
        case gfx_logic_operator::logic_xor:
            return GL_XOR;
        case gfx_logic_operator::logic_or:
            return GL_OR;
        case gfx_logic_operator::logic_nor:
            return GL_NOR;
        case gfx_logic_operator::logic_equivalent:
            return GL_EQUIV;
        case gfx_logic_operator::logic_invert:
            return GL_INVERT;
        case gfx_logic_operator::logic_or_reverse:
            return GL_OR_REVERSE;
        case gfx_logic_operator::logic_copy_inverted:
            return GL_COPY_INVERTED;
        case gfx_logic_operator::logic_or_inverted:
            return GL_OR_INVERTED;
        case gfx_logic_operator::logic_nand:
            return GL_NAND;
        case gfx_logic_operator::logic_set:
            return GL_SET;
        default:
            MANGO_ASSERT(false, "Unknown logic operator!");
            return GL_NONE;
        }
    }

    //! \brief Converts a \a gfx_blend_operation to a \a gl_enum.
    //! \param[in] op The \a gfx_blend_operation to convert.
    //! \return The \a gl_enum representing the given \a gfx_blend_operation.
    inline gl_enum gfx_blend_operation_to_gl(const gfx_blend_operation& op)
    {
        switch (op)
        {
        case gfx_blend_operation::blend_operation_add:
            return GL_FUNC_ADD;
        case gfx_blend_operation::blend_operation_subtract:
            return GL_FUNC_SUBTRACT;
        case gfx_blend_operation::blend_operation_reverse_subtract:
            return GL_FUNC_REVERSE_SUBTRACT;
        case gfx_blend_operation::blend_operation_take_min:
            return GL_MIN;
        case gfx_blend_operation::blend_operation_take_max:
            return GL_MAX;
        default:
            MANGO_ASSERT(false, "Unknown blend operation!");
            return GL_NONE;
        }
    }

    //! \brief Converts a \a gfx_blend_factor to a \a gl_enum.
    //! \param[in] factor The \a gfx_blend_factor to convert.
    //! \return The \a gl_enum representing the given \a gfx_blend_factor.
    inline gl_enum gfx_blend_factor_to_gl(const gfx_blend_factor& factor)
    {
        switch (factor)
        {
        case gfx_blend_factor::blend_factor_zero:
            return GL_ZERO;
        case gfx_blend_factor::blend_factor_one:
            return GL_ONE;
        case gfx_blend_factor::blend_factor_src_color:
            return GL_SRC_COLOR;
        case gfx_blend_factor::blend_factor_one_minus_src_color:
            return GL_ONE_MINUS_SRC_COLOR;
        case gfx_blend_factor::blend_factor_dst_color:
            return GL_DST_COLOR;
        case gfx_blend_factor::blend_factor_one_minus_dst_color:
            return GL_ONE_MINUS_DST_COLOR;
        case gfx_blend_factor::blend_factor_src_alpha:
            return GL_SRC_ALPHA;
        case gfx_blend_factor::blend_factor_one_minus_src_alpha:
            return GL_ONE_MINUS_SRC_ALPHA;
        case gfx_blend_factor::blend_factor_dst_alpha:
            return GL_DST_ALPHA;
        case gfx_blend_factor::blend_factor_one_minus_dst_alpha:
            return GL_ONE_MINUS_DST_ALPHA;
        case gfx_blend_factor::blend_factor_constant_color:
            return GL_CONSTANT_COLOR;
        case gfx_blend_factor::blend_factor_one_minus_constant_color:
            return GL_ONE_MINUS_CONSTANT_COLOR;
        case gfx_blend_factor::blend_factor_constant_alpha:
            return GL_CONSTANT_ALPHA;
        case gfx_blend_factor::blend_factor_one_minus_constant_alpha:
            return GL_ONE_MINUS_CONSTANT_ALPHA;
        case gfx_blend_factor::blend_factor_src_alpha_saturate:
            return GL_SRC_ALPHA_SATURATE;
        case gfx_blend_factor::blend_factor_src1_color:
            return GL_SRC1_COLOR;
        case gfx_blend_factor::blend_factor_one_minus_src1_color:
            return GL_ONE_MINUS_SRC1_COLOR;
        case gfx_blend_factor::blend_factor_src1_alpha:
            return GL_SRC1_ALPHA;
        case gfx_blend_factor::blend_factor_one_minus_src1_alpha:
            return GL_ONE_MINUS_SRC1_ALPHA;
        default:
            MANGO_ASSERT(false, "Unknown blend factor!");
            return GL_NONE;
        }
    }

    //! \brief Creates r,g,b,a color mask from \a gfx_color_component_flag_bits.
    //! \param[in] write_mask The \a gfx_color_component_flag_bits used for creation.
    //! \param[out] r True if the r component should be written, else false.
    //! \param[out] g True if the g component should be written, else false.
    //! \param[out] b True if the b component should be written, else false.
    //! \param[out] a True if the a component should be written, else false.
    //! \return The number of mipmap levels.
    inline void create_gl_color_mask(const gfx_color_component_flag_bits& write_mask, bool& r, bool& g, bool& b, bool& a)
    {
        r = g = b = a = false;

        if ((write_mask & gfx_color_component_flag_bits::components_rgba) == gfx_color_component_flag_bits::components_rgba)
        {
            r = g = b = a = true;
            return;
        }

        if ((write_mask & gfx_color_component_flag_bits::components_rgb) == gfx_color_component_flag_bits::components_rgb)
        {
            r = g = b = true;
            return;
        }

        if ((write_mask & gfx_color_component_flag_bits::component_r) != gfx_color_component_flag_bits::component_none)
        {
            r = true;
        }

        if ((write_mask & gfx_color_component_flag_bits::component_g) != gfx_color_component_flag_bits::component_none)
        {
            g = true;
        }

        if ((write_mask & gfx_color_component_flag_bits::component_b) != gfx_color_component_flag_bits::component_none)
        {
            b = true;
        }

        if ((write_mask & gfx_color_component_flag_bits::component_a) != gfx_color_component_flag_bits::component_none)
        {
            a = true;
        }
    }

    //! \brief Converts \a gfx_primitive_topology to a \a gl_enum.
    //! \param[in] topology The \a gfx_primitive_topology to convert.
    //! \return The \a gl_enum representing the given \a gfx_primitive_topology.
    inline gl_enum gfx_primitive_topology_to_gl(const gfx_primitive_topology& topology)
    {
        switch (topology)
        {
        case gfx_primitive_topology::primitive_topology_point_list:
            return GL_POINT;
        case gfx_primitive_topology::primitive_topology_line_list:
            return GL_LINES;
        case gfx_primitive_topology::primitive_topology_line_loop:
            return GL_LINE_LOOP;
        case gfx_primitive_topology::primitive_topology_line_strip:
            return GL_LINE_STRIP;
        case gfx_primitive_topology::primitive_topology_triangle_list:
            return GL_TRIANGLES;
        case gfx_primitive_topology::primitive_topology_triangle_strip:
            return GL_TRIANGLE_STRIP;
        case gfx_primitive_topology::primitive_topology_triangle_fan:
            return GL_TRIANGLE_FAN;
        default:
            MANGO_ASSERT(false, "Unknown primitive topology!");
            return GL_NONE;
        }
    }

    //! \brief Converts a \a gfx_format into required vertex attribute data.
    //! \param[in] attribute_format The \a gfx_format specifying the vertex attribute data format.
    //! \param[out] component_type The component type as \a gl_enum.
    //! \param[out] number_of_components The number of components in the format.
    //! \param[out] normalized True if the the attribute data is normalized, else false.
    //! \return The number of mipmap levels.
    inline void gfx_format_to_gl_attribute_data(const gfx_format& attribute_format, gl_enum& component_type, int32& number_of_components, bool& normalized)
    {
        switch (attribute_format)
        {
        case gfx_format::r8:
            component_type       = GL_UNSIGNED_BYTE;
            number_of_components = 1;
            normalized           = true;
            break;
        case gfx_format::r16:
            component_type       = GL_UNSIGNED_SHORT;
            number_of_components = 1;
            normalized           = true;
            break;
        case gfx_format::r16f:
            component_type       = GL_HALF_FLOAT;
            number_of_components = 1;
            normalized           = false;
            break;
        case gfx_format::r32f:
            component_type       = GL_FLOAT;
            number_of_components = 1;
            normalized           = false;
            break;
        case gfx_format::r8i:
            component_type       = GL_BYTE;
            number_of_components = 1;
            normalized           = true;
            break;
        case gfx_format::r16i:
            component_type       = GL_SHORT;
            number_of_components = 1;
            normalized           = true;
            break;
        case gfx_format::r32i:
            component_type       = GL_INT;
            number_of_components = 1;
            normalized           = true;
            break;
        case gfx_format::r8ui:
            component_type       = GL_UNSIGNED_BYTE;
            number_of_components = 1;
            normalized           = true;
            break;
        case gfx_format::r16ui:
            component_type       = GL_UNSIGNED_SHORT;
            number_of_components = 1;
            normalized           = true;
            break;
        case gfx_format::r32ui:
            component_type       = GL_UNSIGNED_INT;
            number_of_components = 1;
            normalized           = true;
            break;
        case gfx_format::rg8:
            component_type       = GL_UNSIGNED_BYTE;
            number_of_components = 2;
            normalized           = true;
            break;
        case gfx_format::rg16:
            component_type       = GL_UNSIGNED_SHORT;
            number_of_components = 2;
            normalized           = true;
            break;
        case gfx_format::rg16f:
            component_type       = GL_HALF_FLOAT;
            number_of_components = 2;
            normalized           = false;
            break;
        case gfx_format::rg32f:
            component_type       = GL_FLOAT;
            number_of_components = 2;
            normalized           = false;
            break;
        case gfx_format::rg8i:
            component_type       = GL_BYTE;
            number_of_components = 2;
            normalized           = true;
            break;
        case gfx_format::rg16i:
            component_type       = GL_SHORT;
            number_of_components = 2;
            normalized           = true;
            break;
        case gfx_format::rg32i:
            component_type       = GL_INT;
            number_of_components = 2;
            normalized           = true;
            break;
        case gfx_format::rg8ui:
            component_type       = GL_UNSIGNED_BYTE;
            number_of_components = 2;
            normalized           = true;
            break;
        case gfx_format::rg16ui:
            component_type       = GL_UNSIGNED_SHORT;
            number_of_components = 2;
            normalized           = true;
            break;
        case gfx_format::rg32ui:
            component_type       = GL_UNSIGNED_INT;
            number_of_components = 2;
            normalized           = true;
            break;
        case gfx_format::rgb8i:
            component_type       = GL_BYTE;
            number_of_components = 3;
            normalized           = true;
            break;
        case gfx_format::rgb8ui:
            component_type       = GL_UNSIGNED_BYTE;
            number_of_components = 3;
            normalized           = true;
            break;
        case gfx_format::rgb16f:
            component_type       = GL_HALF_FLOAT;
            number_of_components = 3;
            normalized           = false;
            break;
        case gfx_format::rgb16i:
            component_type       = GL_SHORT;
            number_of_components = 3;
            normalized           = true;
            break;
        case gfx_format::rgb16ui:
            component_type       = GL_UNSIGNED_SHORT;
            number_of_components = 3;
            normalized           = true;
            break;
        case gfx_format::rgb32f:
            component_type       = GL_FLOAT;
            number_of_components = 3;
            normalized           = false;
            break;
        case gfx_format::rgb32i:
            component_type       = GL_INT;
            number_of_components = 3;
            normalized           = true;
            break;
        case gfx_format::rgb32ui:
            component_type       = GL_UNSIGNED_INT;
            number_of_components = 3;
            normalized           = true;
            break;
        case gfx_format::rgba8:
            component_type       = GL_UNSIGNED_BYTE;
            number_of_components = 4;
            normalized           = true;
            break;
        case gfx_format::rgba16:
            component_type       = GL_SHORT;
            number_of_components = 4;
            normalized           = true;
            break;
        case gfx_format::rgba16f:
            component_type       = GL_HALF_FLOAT;
            number_of_components = 4;
            normalized           = false;
            break;
        case gfx_format::rgba32f:
            component_type       = GL_FLOAT;
            number_of_components = 4;
            normalized           = false;
            break;
        case gfx_format::rgba8i:
            component_type       = GL_BYTE;
            number_of_components = 4;
            normalized           = true;
            break;
        case gfx_format::rgba16i:
            component_type       = GL_UNSIGNED_BYTE;
            number_of_components = 4;
            normalized           = true;
            break;
        case gfx_format::rgba32i:
            component_type       = GL_UNSIGNED_BYTE;
            number_of_components = 4;
            normalized           = true;
            break;
        case gfx_format::rgba8ui:
            component_type       = GL_UNSIGNED_BYTE;
            number_of_components = 4;
            normalized           = true;
            break;
        case gfx_format::rgba16ui:
            component_type       = GL_UNSIGNED_SHORT;
            number_of_components = 4;
            normalized           = true;
            break;
        case gfx_format::rgba32ui:
            component_type       = GL_UNSIGNED_INT;
            number_of_components = 4;
            normalized           = true;
            break;
        default:
            MANGO_ASSERT(false, "Invalid format! Could also be, that I did not think of adding this here!");
            component_type = GL_NONE;
        }
    }

    //! \brief Converts a \a gfx_barrier_bit to a \a gl_bitfield.
    //! \param[in] bits The \a gfx_barrier_bit to convert.
    //! \return The \a gl_bitfield representing the given \a gfx_barrier_bit.
    inline gl_bitfield gfx_barrier_bit_to_gl(const gfx_barrier_bit& bits)
    {
        gl_bitfield barrier_bits = GL_NONE;

        if ((bits & gfx_barrier_bit::vertex_attrib_array_barrier_bit) != gfx_barrier_bit::unknown_barrier_bit)
            barrier_bits |= GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT;
        if ((bits & gfx_barrier_bit::element_array_barrier_bit) != gfx_barrier_bit::unknown_barrier_bit)
            barrier_bits |= GL_ELEMENT_ARRAY_BARRIER_BIT;
        if ((bits & gfx_barrier_bit::uniform_barrier_bit) != gfx_barrier_bit::unknown_barrier_bit)
            barrier_bits |= GL_UNIFORM_BARRIER_BIT;
        if ((bits & gfx_barrier_bit::texture_fetch_barrier_bit) != gfx_barrier_bit::unknown_barrier_bit)
            barrier_bits |= GL_TEXTURE_FETCH_BARRIER_BIT;
        if ((bits & gfx_barrier_bit::shader_image_access_barrier_bit) != gfx_barrier_bit::unknown_barrier_bit)
            barrier_bits |= GL_SHADER_IMAGE_ACCESS_BARRIER_BIT;
        if ((bits & gfx_barrier_bit::command_barrier_bit) != gfx_barrier_bit::unknown_barrier_bit)
            barrier_bits |= GL_COMMAND_BARRIER_BIT;
        if ((bits & gfx_barrier_bit::pixel_buffer_barrier_bit) != gfx_barrier_bit::unknown_barrier_bit)
            barrier_bits |= GL_PIXEL_BUFFER_BARRIER_BIT;
        if ((bits & gfx_barrier_bit::texture_update_barrier_bit) != gfx_barrier_bit::unknown_barrier_bit)
            barrier_bits |= GL_TEXTURE_UPDATE_BARRIER_BIT;
        if ((bits & gfx_barrier_bit::buffer_update_barrier_bit) != gfx_barrier_bit::unknown_barrier_bit)
            barrier_bits |= GL_BUFFER_UPDATE_BARRIER_BIT;
        if ((bits & gfx_barrier_bit::framebuffer_barrier_bit) != gfx_barrier_bit::unknown_barrier_bit)
            barrier_bits |= GL_FRAMEBUFFER_BARRIER_BIT;
        if ((bits & gfx_barrier_bit::transform_feedback_barrier_bit) != gfx_barrier_bit::unknown_barrier_bit)
            barrier_bits |= GL_TRANSFORM_FEEDBACK_BARRIER_BIT;
        if ((bits & gfx_barrier_bit::atomic_counter_barrier_bit) != gfx_barrier_bit::unknown_barrier_bit)
            barrier_bits |= GL_ATOMIC_COUNTER_BARRIER_BIT;
        if ((bits & gfx_barrier_bit::shader_storage_barrier_bit) != gfx_barrier_bit::unknown_barrier_bit)
            barrier_bits |= GL_SHADER_STORAGE_BARRIER_BIT;
        if ((bits & gfx_barrier_bit::query_buffer_barrier_bit) != gfx_barrier_bit::unknown_barrier_bit)
            barrier_bits |= GL_QUERY_BUFFER_BARRIER_BIT;

        return barrier_bits;
    }

    //
    // Required extras for opengl internal state management.
    //

    //! \brief Opengl internal data to store vertex buffer information.
    struct vertex_buffer_data
    {
        //! \brief The \a gfx_buffer used as vertex buffer.
        gfx_handle<const gfx_buffer> buffer;
        //! \brief The binding of the vertex buffer.
        int32 binding;
        //! \brief The offset in the vertex buffer.
        int32 offset;
    };

    //! \brief Opengl internal data to describe vertex arrays.
    //! \details Vertex arrays are not exposed and are only used internally.
    struct vertex_array_data_descriptor
    {
        // TODO Paul: Check if this is really enough to cache everything in an unique fashion.
        //! \brief A pointer to the \a vertex_input_descriptor located in a \a gfx_pipeline.
        const vertex_input_descriptor* input_descriptor;
        //! \brief The vertex count.
        int32 vertex_count;
        //! \brief The index count.
        int32 index_count;
        //! \brief The number of bound vertex buffers.
        int32 vertex_buffer_count;
        //! \brief The vertex count.
        gfx_format index_type;
        //! \brief A pointer to all bound vertex buffers including their binding information.
        const vertex_buffer_data* vertex_buffers;
        //! \brief A pointer to the \a gfx_buffer bound as index buffer.
        const gfx_handle<const gfx_buffer>* index_buffer;
    };

} // namespace mango

#endif // MANGO_GL_GRAPHICS_RESOURCES_HPP
