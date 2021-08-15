//! \file      graphics_resources.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_GRAPHICS_RESOURCES_HPP
#define MANGO_GRAPHICS_RESOURCES_HPP

#include <array>
#include <graphics/graphics_state.hpp>
#include <graphics/graphics_types.hpp>

namespace mango
{
    //! \brief Description providing information regarding shader resources in specific shader stages.
    struct shader_resource_description
    {
        //! \brief One or multiple shader stages the shader resource can be accessed.
        gfx_shader_stage_type stage;
        //! \brief The binding point.
        int32 binding;
        //! \brief The shader resource variable name in the shader.
        const char* variable_name;
        //! \brief The type of the shader resource.
        gfx_shader_resource_type type;
        //! \brief The number of elements in the array when shader resource is an array. Default should be 1.
        int32 array_size;
    };

    //! \brief Description for shader sources.
    struct shader_source_description
    {
        //! \brief Source string.
        const char* source;
        //! \brief Size of the source string in bytes.
        int32 size;
        //! \brief An entry point for the shader. Default should be "main".
        const char* entry_point;
    };

    //! \brief Create info for shader stages.
    struct shader_stage_create_info
    {
        //! \brief Description of the shader source.
        shader_source_description shader_source;
        //! \brief The stage the shader is used for.
        gfx_shader_stage_type stage;
        //! \brief The number of shader resource descriptions.
        int32 resource_count;
        //! \brief Pointer to an array of \a shader_resource_description structures.
        std::array<shader_resource_description, 128> resources; // TODO Paul: Could probably be queried by API internally. Problem -> OpenGL does only allow querying the program...
        /*
         should be maximum of GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS, GL_MAX_UNIFORM_BUFFER_BINDINGS
        */
    };

    //! \brief Binding description for shader resources used by the \a gfx_pipeline_resource_layout.
    struct shader_resource_binding
    {
        //! \brief One or multiple shader stages the shader resource binding can be accessed.
        gfx_shader_stage_type stage;
        //! \brief The binding point.
        int32 binding;
        //! \brief The type of the shader resource.
        gfx_shader_resource_type type;
        //! \brief The access in the shader. Defines update possibilities.
        gfx_shader_resource_access access;
    };

    //! \brief Description specifying vertex input bindings.
    struct vertex_input_binding_description
    {
        //! \brief The binding this structure describes.
        int32 binding;
        //! \brief The distance in bytes between two consecutive elements within the buffer.
        int32 stride;
        //! \brief Rate specifying whether vertex attribute addressing is a function of the vertex index or of the instance index.
        gfx_vertex_input_rate input_rate;
    };

    //! \brief Description specifying vertex input attributes.
    struct vertex_input_attribute_description
    {
        //! \brief The shader binding location number for this attribute.
        int32 location;
        //! \brief The binding number for this attribute to the take its data from.
        int32 binding;
        //! \brief Format describing size and type of the vertex attribute data.
        gfx_format attribute_format;
        //! \brief A byte offset of this attribute relative to the start of an element in the vertex input binding.
        int32 offset;
    };

    //! \brief Descriptor specifying the vertex input layout.
    struct vertex_input_descriptor
    {
        //! \brief The number of vertex binding descriptions.
        int32 binding_description_count;
        //! \brief Pointer to an array of \a vertex_input_binding_description structures.
        std::array<vertex_input_binding_description, 16> binding_descriptions; // TODO Paul: Query max vertex buffers. GL_MAX_VERTEX_ATTRIB_BINDINGS
        //! \brief The number of vertex attribute descriptions.
        int32 attribute_description_count;
        //! \brief Pointer to an array of \a vertex_input_attribute_description structures.
        std::array<vertex_input_attribute_description, 16> attribute_descriptions; // TODO Paul: Query max vertex buffers. GL_MAX_VERTEX_ATTRIB_BINDINGS
    };

    //! \brief Descriptor specifying the input assembly.
    struct input_assembly_descriptor
    {
        //! \brief The primitive topology used for drawing.
        gfx_primitive_topology topology;
        //! \brief Controls wether a special vertex index value is treated as restarting the assembly of primitives.
        // bool enable_primitive_restart; // TODO Paul: Support later.
    };

    //! \brief Descriptor specifying parameters of viewports and scissors.
    struct viewport_descriptor
    {
        //! \brief The number of viewports.
        int32 viewport_count;
        //! \brief A pointer to an array of \a gfx_viewport structures. If viewport state is dynamic, this is ignored.
        std::array<gfx_viewport, 16> viewports; // TODO Paul: Query GL_MAX_VIEWPORTS.
        //! \brief The number of scissors. Has to match the number of viewports.
        int32 scissor_count;
        //! \brief A pointer to an array of \a gfx_scissor_rectangles structures. If scissor state is dynamic, this is ignored.
        std::array<gfx_scissor_rectangle, 16> scissors; // TODO Paul: Query GL_MAX_VIEWPORTS.
    };

    //! \brief Descriptor specifying parameters of the rasterization state.
    struct raster_state_descriptor
    {
        //! \brief The rendering mode for polygons.
        gfx_polygon_mode polygon_mode;
        //! \brief The triangle facing direction used for primitive culling.
        gfx_cull_mode_flag_bits cull_mode;
        //! \brief Value specifying the front-facing triangle orientation to be used for culling.
        gfx_front_face front_face;
        //! \brief Controls wether to bias depth values.
        bool enable_depth_bias;
        //! \brief A scalar factor to add as a constant to each fragment. If depth bias is dynamic, this is ignored.
        float constant_depth_bias;
        //! \brief Maximum or minimum bias of a fragment. If depth bias is dynamic, this is ignored.
        float depth_bias_clamp;
        //! \brief A scalar factor applied to each fragment's slope. If depth bias is dynamic, this is ignored.
        float depth_bias_slope_factor;
        //! \brief The line width of rasterized line segments. If line width is dynamic, this is ignored.
        float line_width;
    };

    //! \brief Description specifying parameters required for stencil operations.
    struct stencil_operation_description
    {
        //! \brief Action performed for samples failing the stencil test.
        gfx_stencil_operation fail_operation;
        //! \brief Action performed for samples passing the depth and stencil tests.
        gfx_stencil_operation pass_operation;
        //! \brief Action performed for samples passing the stencil but failing the depth test.
        gfx_stencil_operation depth_fail_operation;
        //! \brief Compare operator used in stencil test.
        gfx_compare_operator compare_operator;
        //! \brief Selects the bits of the stencil values participating in the stencil test. If stencil compare mask is dynamic, this is ignored.
        uint32 compare_mask;
        //! \brief Selects the bits of the stencil values updated by the stencil test.  If stencil write mask is dynamic, this is ignored.
        uint32 write_mask;
        //! \brief Reference value used in stencil comparison. If stencil reference is dynamic, this is ignored.
        uint32 reference;
    };

    //! \brief Descriptor specifying parameters of the depth/stencil state.
    struct depth_stencil_descriptor
    {
        //! \brief Controls whether depth testing is enabled.
        bool enable_depth_test;
        //! \brief Controls whether depth writes are enabled. Always disabled when depth test is disabled.
        bool enable_depth_write;
        //! \brief Compare operator used in depth test.
        gfx_compare_operator depth_compare_operator;
        //! \brief Controls whether stencil testing is enabled.
        bool enable_stencil_test;
        //! \brief Stencil operation description for front facing polygons.
        stencil_operation_description front;
        //! \brief Stencil operation description for back facing polygons.
        stencil_operation_description back;
    };

    //! \brief Description for color attachment blending.
    struct blend_color_attachment_description
    {
        //! \brief Controls whether blending is enabled for the corresponding attachment.
        bool enable_blend;
        //! \brief The blend factor to determine source factors.
        gfx_blend_factor src_color_blend_factor;
        //! \brief The blend factor to determine destination factors.
        gfx_blend_factor dst_color_blend_factor;
        //! \brief Selects the blend operation for rgb values to write.
        gfx_blend_operation color_blend_operation;
        //! \brief The blend factor to determine source alpha.
        gfx_blend_factor src_alpha_blend_factor;
        //! \brief The blend factor to determine destination alpha.
        gfx_blend_factor dst_alpha_blend_factor;
        //! \brief Selects the blend operation for alpha values to write.
        gfx_blend_operation alpha_blend_operation;
        //! \brief Mask specifying which components are enabled for writing.
        //! \details The color write mask operation is applied regardless of whether blending is enabled.
        gfx_color_component_flag_bits color_write_mask;
    };

    //! \brief Descriptor specifying parameters of the blend state.
    struct blend_state_descriptor
    {
        //! \brief Controls whether logical operations should be applied.
        bool enable_logical_operation;
        //! \brief Logical operator used for application when the logical operations are enabled.
        gfx_logic_operator logic_operator;
        //! \brief Description for the actual blending. Only one for now, since we do not support independent blending.
        blend_color_attachment_description blend_description;
        //! \brief Constant rgba blend values. If blend constants are dynamic, this is ignored.
        std::array<float, 4> blend_constants;
    };

    //! \brief Descriptor specifying dynamic state.
    struct dynamic_state_descriptor
    {
        //! \brief Bitfield specifying which pieces of pipeline state will use the values from dynamic state commands rather than from pipeline state creation info.
        gfx_dynamic_state_flag_bits dynamic_states;
    };

    //! \brief Description to provide information for output targets.
    struct render_output_description
    {
        //! \brief The output color attachment formats.
        std::array<gfx_format, 4> color_render_targets; // TODO Paul: Query max attachments.

        //! \brief The output depth/stencil attachment format.
        gfx_format depth_stencil_format;
    };

    // fwd
    class pipeline_resource_layout;

    // fwd
    class gfx_shader_stage;

    //! \brief Descriptor specifying the active shader stages in a graphics \a gfx_pipeline.
    //! \details Will be extended when required.
    struct graphics_shader_stage_descriptor
    {
        //! \brief Vertex shader stage handle.
        gfx_handle<const gfx_shader_stage> vertex_shader_stage;
        //! \brief Geometry shader stage handle. Leave uninitialized when not in use.
        gfx_handle<const gfx_shader_stage> geometry_shader_stage;
        //! \brief Fragment shader stage handle. Leave uninitialized when not in use.
        gfx_handle<const gfx_shader_stage> fragment_shader_stage;
    };

    //! \brief Create info for graphics \a gfx_pipelines.
    struct graphics_pipeline_create_info
    {
        //! \brief Resource layout for all pipeline stages.
        gfx_handle<const pipeline_resource_layout> pipeline_layout;
        //! \brief Descriptor specifying the active \a gfx_pipeline shader stages.
        graphics_shader_stage_descriptor shader_stage_descriptor;
        //! \brief Descriptor providing the \a gfx_pipeline with information regarding it's vertex input.
        vertex_input_descriptor vertex_input_state;
        //! \brief Descriptor providing the \a gfx_pipeline with information regarding it's input assembly.
        input_assembly_descriptor input_assembly_state;
        // tesselation_descriptor tesselation_state; NOT IN USE ATM.
        //! \brief Descriptor specifying viewport and scissor parameters for the \a gfx_pipeline.
        viewport_descriptor viewport_state;
        //! \brief Descriptor providing the parameters for the \a gfx_pipeline rasterization state.
        raster_state_descriptor rasterization_state;
        // multisample_descriptor multisample_state; NOT IN USE ATM.
        //! \brief Descriptor providing the parameters for the \a gfx_pipeline depth/stencil state.
        depth_stencil_descriptor depth_stencil_state;
        //! \brief Descriptor providing the parameters for the \a gfx_pipeline blend state.
        blend_state_descriptor blend_state;
        //! \brief Descriptor specifying the \a gfx_pipeline dynamic state.
        dynamic_state_descriptor dynamic_state;
        //! \brief Describes the output formats of the \a gfx_pipeline.
        render_output_description output_description; // TODO Paul: UNUSED

        // gfx_handle<render_pass> render_pass; NO CONCEPT YET AND NOT REQUIRED.
        // int32 subpass_index; NO CONCEPT YET AND NOT REQUIRED.
    };

    //! \brief Descriptor specifying the active shader stages in a compute \a gfx_pipeline.
    struct compute_shader_stage_descriptor
    {
        //! \brief Compute shader stage handle.
        gfx_handle<const gfx_shader_stage> compute_shader_stage;
    };

    //! \brief Create info for compute \a gfx_pipelines.
    struct compute_pipeline_create_info
    {
        //! \brief Resource layout for all pipeline stages.
        gfx_handle<const pipeline_resource_layout> pipeline_layout;
        //! \brief Descriptor specifying the active \a gfx_pipeline shader stage.
        compute_shader_stage_descriptor shader_stage_descriptor;
    };

    //! \brief Description to provide information for \a gfx_barrier creation.
    struct barrier_description
    {
        //! \brief The \a gfx_barrier_bit specifying whick parts to block.
        gfx_barrier_bit barrier_bit;
    };

    //! \brief Description to provide information for setting the data of a \a gfx_texture.
    struct texture_set_description
    {
        //! \brief The level of the \a gfx_texture to set the data in.
        int32 level;
        //! \brief The x offset in the \a gfx_texture to set the data in.
        int32 x_offset;
        //! \brief The y offset in the \a gfx_texture to set the data in.
        int32 y_offset;
        //! \brief The z offset in the \a gfx_texture to set the data in.
        int32 z_offset;
        //! \brief The width of the data to set.
        int32 width;
        //! \brief The height of the data to set.
        int32 height;
        //! \brief The depth of the data to set.
        int32 depth;
        //! \brief Pixel format of the data.
        gfx_format pixel_format;
        //! \brief The \a gfx_format of each component.
        gfx_format component_type;
    };

    //
    // Resources create infos.
    //

    //! \brief Create info for buffers.
    struct buffer_create_info
    {
        //! \brief Defines a target.
        gfx_buffer_target buffer_target;
        //! \brief Defines access rights.
        gfx_buffer_access buffer_access;
        //! \brief The size of the buffer.
        int64 size;
    };

    //! \brief Create info for textures.
    struct texture_create_info
    {
        //! \brief The type of the texture.
        gfx_texture_type texture_type;
        //! \brief The internal format.
        gfx_format texture_format;
        //! \brief Width in pixels.
        int32 width;
        //! \brief Height in pixels.
        int32 height;
        //! \brief Number of mip levels. Will be clamped to maximum possible. Minimum value is 1.
        int32 miplevels;
        //! \brief Number of array layers. Default value should be 1.
        int32 array_layers;
    };

    //! \brief Create info for samplers.
    struct sampler_create_info
    {
        //! \brief A filter description for minification.
        gfx_sampler_filter sampler_min_filter;
        //! \brief A filter description formagnification.
        gfx_sampler_filter sampler_max_filter;
        //! \brief Comparison mode can be enabled. Normaly used for easier depth comparison.
        bool enable_comparison_mode;
        //! \brief The comparison operator when comparison mode is enabled.
        gfx_compare_operator comparison_operator;
        //! \brief Description for edge sample handling in u direction.
        gfx_sampler_edge_wrap edge_value_wrap_u;
        //! \brief Description for edge sample handling in v direction.
        gfx_sampler_edge_wrap edge_value_wrap_v;
        //! \brief Description for edge sample handling in w direction.
        gfx_sampler_edge_wrap edge_value_wrap_w;
        //! \brief The rgba values for border coloring when edge values are sampled with clamp to border.
        std::array<float, 4> border_color;
        //! \brief Enable seamless cubemap sampling.
        bool enable_seamless_cubemap;
    };

    //! \brief Create info for semaphores.
    struct semaphore_create_info
    {
    };

    //
    // Interfaces for API specific stuff.
    //

    //! \brief Binding description for shader resources used by the \a gfx_pipeline_resource_layout.
    class pipeline_resource_layout
    {
    };

    //! \brief Describes a draw call.
    struct draw_call_description
    {
        //! \brief The number of vertices to render. Only used, whe not indexed.
        int32 vertex_count;
        //! \brief The number of indices to use for render.
        int32 index_count;
        //! \brief The number of instances to render. Minimum 1.
        int32 instance_count;
        //! \brief Base vertex.
        int32 base_vertex;
        //! \brief Base instance.
        int32 base_instance;
        //! \brief Offset of indices.
        int32 index_offset;
    };

    //! \brief Mapping to get, set and submit shader resources.
    class shader_resource_mapping
    {
      public:
        //! \brief Checks whether resource with correct access exists, returns a pointer to fill.
        //! \details Can also be an array of resources.
        //! \param[in] variable_name The variable name.
        //! \param[in] resource A \a gfx_handle of the resource to set.
        //! \return True on success, else false.
        virtual bool set(const string variable_name, gfx_handle<const gfx_device_object> resource) = 0;

        //! \brief The pair of an integer binding and a \a shader_resource_type.
        using binding_pair = std::pair<int32, gfx_shader_resource_type>;
        //! \brief Mapping of resource names to \a binding_pairs.
        std::unordered_map<string, binding_pair> m_name_to_binding_pair;

        //! \brief Status bit to describe static and dynamic resources.
        //! \details 0 = invalid, 1 = dynamic, 2 = static unset, 3 = static set.
        using status_bit = uint8; // TODO Should be done better.

        //! \brief The pair of \a gfx_handle of a shader resources and a \a status_bit.
        template <typename T>
        using resource_pair = std::pair<gfx_handle<T>, status_bit>;
    };

    //! \brief An uninitialized \a gfx_device_object.
    class gfx_uninitialized_device_object : public gfx_device_object
    {
      public:
        gfx_uninitialized_device_object() = default;
        int32 get_type_id() const override
        {
            return -1;
        };
    };

    //! \brief A big structure containing most of the relevant state for drawing or computing everything on and with the graphics card.
    class gfx_pipeline : public gfx_device_object
    {
      public:
        int32 get_type_id() const override
        {
            return 0;
        };

        //! \brief Retrieves and returns a handle to the \a shader_resource_mapping of this \a gfx_pipeline.
        //! \return The gfx_handle to the \a shader_resource_mapping.
        virtual gfx_handle<shader_resource_mapping> get_resource_mapping() const = 0;

      protected:
        //! \brief Submits all pipeline shader resources.
        //! \param[in] shared_graphics_state The graphics state shared for all devices.
        virtual void submit_pipeline_resources(gfx_handle<gfx_graphics_state> shared_graphics_state) const = 0;
    };

    //! \brief A \a gfx_device_object representing a buffer on the gpu.
    //! \details Vertex-/Index-/Uniform-/Constant-/ShaderStorage-Buffer.
    class gfx_buffer : public gfx_device_object
    {
      public:
        int32 get_type_id() const override
        {
            return 1;
        };
    };

    //! \brief A \a gfx_device_object representing a texture on the gpu.
    //! \details Texture2D, 3D, Cube, Render Target/Attachment, Color, DepthStencil, Storage.
    class gfx_texture : public gfx_device_object
    {
      public:
        int32 get_type_id() const override
        {
            return 2;
        };

        //! \brief Returns the size of the texture.
        //! \return The size of the texture in pixels.
        virtual const vec2 get_size() const             = 0; // TODO Paul: Should be done cleaner

        //! \brief Returns the \a gfx_texture_type of the texture.
        //! \return The \a gfx_texture_type of the texture.
        virtual const gfx_texture_type& get_type() const = 0; // TODO Paul: Should be done cleaner
    };

    //! \brief A \a gfx_device_object representing a texture view.
    //! \details Image texture with binding level.
    class gfx_image_texture_view : public gfx_device_object
    {
      public:
        int32 get_type_id() const override
        {
            return 3;
        };
    };

    //! \brief A \a gfx_device_object representing a sampler on the gpu.
    //! \details Used to access textures in shader.
    class gfx_sampler : public gfx_device_object
    {
      public:
        int32 get_type_id() const override
        {
            return 4;
        };
    };

    //! \brief A \a gfx_device_object representing a shader stage on the gpu.
    //! \details Minimal interface for a shader stage.
    class gfx_shader_stage : public gfx_device_object
    {
      public:
        int32 get_type_id() const override
        {
            return 5;
        };
    };

    //! \brief A \a gfx_device_object representing a semaphore/synchronization structure on the gpu.
    //! \details Used for barriers.
    class gfx_semaphore : public gfx_device_object
    {
      public:
        int32 get_type_id() const override
        {
            return 6;
        };
    };
} // namespace mango

#endif // MANGO_GRAPHICS_RESOURCES_HPP
