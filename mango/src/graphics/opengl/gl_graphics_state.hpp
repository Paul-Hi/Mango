//! \file      gl_graphics_state.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_GL_GRAPHICS_STATE_HPP
#define MANGO_GL_GRAPHICS_STATE_HPP

#include <graphics/graphics_state.hpp>
#include <graphics/opengl/gl_graphics_resources.hpp>

namespace mango
{
    //! \brief An opengl \a gfx_graphics_state.
    struct gl_graphics_state : public gfx_graphics_state
    {
        gl_graphics_state()  = default;
        ~gl_graphics_state() = default;

        //! \brief The currently bound \a gfx_pipeline.
        gfx_handle<const gl_pipeline> bound_pipeline;
        //! \brief True if the \a gfx_pipeline shader resources were submitted, else false.
        bool pipeline_resources_submitted;

        //! \brief The number of currently attached color render targets.
        int32 color_target_count;
        //! \brief The number of currently attached depth render targets.
        int32 depth_target_count;
        //! \brief The number of currently attached stencil render targets.
        int32 stencil_target_count;
        //! \brief The number of currently attached depth stencil render targets.
        int32 depth_stencil_target_count;
        //! \brief The \a gfx_handles of the \a gl_textures currently set as render targets.
        gfx_handle<const gl_texture> set_render_targets[8 + 1]; // TODO Paul: Query max attachments.

        //! \brief The number of currently bound vertex buffers.
        int32 vertex_buffer_count;

        //! \brief The \a vertex_buffer_data of the currently bound vertex buffers.
        vertex_buffer_data set_vertex_buffers[16]; // TODO Paul: Query max vertex buffers. GL_MAX_VERTEX_ATTRIB_BINDINGS

        //! \brief The \a gfx_handle of the \a gfx_buffer currently bound as index buffer.
        gfx_handle<const gfx_buffer> set_index_buffer;
        //! \brief The \a gfx_format specifying the current index component type.
        gfx_format index_type;

        struct
        {
            //! \brief The currently bound framebuffer \a gl_handle. Represented as int32 to make invalidation possible.
            int32 framebuffer_name = -1;
            //! \brief The currently bound vertex array \a gl_handle. Represented as int32 to make invalidation possible.
            int32 vertex_array_name = -1;
        } internal; //!< Internal data.

        struct
        {
            //! \brief The number of currently active \a gfx_viewports.
            int32 viewport_count;
            //! \brief The list currently active \a gfx_viewports.
            gfx_viewport viewports[16]; // TODO Paul: Query max viewports.
            //! \brief The number of currently active \a gfx_scissor_rectangles.
            int32 scissor_count;
            //! \brief The list currently active \a gfx_scissor_rectangles.
            gfx_scissor_rectangle scissors[16]; // TODO Paul: Query max scissors.

            //! \brief The currently set line width.
            float line_width;
            //! \brief The currently set blend constants.
            float blend_constants[4];

            struct
            {
                //! \brief The currently set constant bias.
                float constant_bias;
                //! \brief The currently set slope bias.
                float slope_bias;
                //! \brief The currently set bias clamp.
                float bias_clamp;
            } depth; //!< Depth specific settings.

            struct
            {
                //! \brief The currently set \a gfx_stencil_face_flag_bits for comparison.
                gfx_stencil_face_flag_bits compare_face_mask;
                //! \brief The currently set \a gfx_stencil_face_flag_bits for writing.
                gfx_stencil_face_flag_bits write_face_mask;
                //! \brief The currently set \a gfx_stencil_face_flag_bits for reference.
                gfx_stencil_face_flag_bits reference_face_mask;
                //! \brief The currently set bitset for comparison.
                uint32 compare_mask;
                //! \brief The currently set bitset for writing.
                uint32 write_mask;
                //! \brief The currently set bitset for reference.
                uint32 reference;
            } stencil; //!< Stencil specific settings.

        } dynamic_state_cache; //!< Cache data for possible dynamic state.

        struct
        {
            //! \brief List of \a gl_handles of the currently bound uniform buffers.
            std::array<gl_handle, 128> uniform_buffers; // TODO Paul: See shader_stage_create_info in graphics_resources -> Should be queried.

            //! \brief List of \a gl_handles of the currently bound shader storage buffers.
            std::array<gl_handle, 128> shader_storage_buffers; // TODO Paul: See shader_stage_create_info in graphics_resources -> Should be queried.

            //! \brief List of \a gl_handles of the currently bound texture buffers.
            std::array<gl_handle, 128> texture_buffers; // TODO Paul: See shader_stage_create_info in graphics_resources -> Should be queried.

        } resources; //!< Cache data for resources.

        bool is_buffer_bound(gfx_buffer_target target, int32 idx, void* native_handle) override
        {
            MANGO_ASSERT(idx < 128, "Index does exceed maximum binding!");
            switch (target)
            {
            case gfx_buffer_target::buffer_target_uniform:
                return resources.uniform_buffers[idx] == static_cast<gl_handle>((uintptr)native_handle);
            case gfx_buffer_target::buffer_target_shader_storage:
                return resources.shader_storage_buffers[idx] == static_cast<gl_handle>((uintptr)native_handle);
            case gfx_buffer_target::buffer_target_texture:
                return resources.texture_buffers[idx] == static_cast<gl_handle>((uintptr)native_handle);
            default:
                MANGO_ASSERT(false, "Buffer target is not a valid resource!");
                break;
            }
            return false;
        }
        void record_buffer_binding(gfx_buffer_target target, int32 idx, void* native_handle) override
        {
            MANGO_ASSERT(idx < 128, "Index does exceed maximum binding!");
            switch (target)
            {
            case gfx_buffer_target::buffer_target_uniform:
                resources.uniform_buffers[idx] = static_cast<gl_handle>((uintptr)native_handle);
                break;
            case gfx_buffer_target::buffer_target_shader_storage:
                resources.shader_storage_buffers[idx] = static_cast<gl_handle>((uintptr)native_handle);
                break;
            case gfx_buffer_target::buffer_target_texture:
                resources.texture_buffers[idx] = static_cast<gl_handle>((uintptr)native_handle);
                break;
            default:
                MANGO_ASSERT(false, "Buffer target is not a valid resource!");
                break;
            }
        }
    };
} // namespace mango

#endif // MANGO_GL_GRAPHICS_STATE_HPP
