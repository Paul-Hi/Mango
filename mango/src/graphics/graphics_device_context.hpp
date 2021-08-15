//! \file      graphics_device_context.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_GRAPHICS_DEVICE_CONTEXT_HPP
#define MANGO_GRAPHICS_DEVICE_CONTEXT_HPP

#include <graphics/graphics_resources.hpp>
#include <graphics/graphics_types.hpp>

namespace mango
{
    //! \brief Object describing a list of graphics commands to be executed on the gpu.
    class graphics_device_context
    {
      public:
        virtual ~graphics_device_context() = default;

        //! \brief Begins a list of graphics commands.
        //! \details Has to be called before submitting any commands.
        virtual void begin() = 0;

        //
        // static commands
        //

        //! \brief Makes the \a graphics_device_content the currently active one.
        virtual void make_current() = 0;

        //! \brief Sets the swap interval.
        //! \param[in] swap The swap intervall. 1 == vertical synchronization on, 0 == off.
        virtual void set_swap_interval(int32 swap) = 0;

        //! \brief Sets the data of a \a gfx_buffer on the gpu.
        //! \param[in] buffer_handle The \a gfx_handle of the \a gfx_buffer to set the data for.
        //! \param[in] offset The offset in the \a gfx_buffer to start setting the data from.
        //! \param[in] size The size of the data to set in bytes.
        //! \param[in] data Pointer to the data to set.
        virtual void set_buffer_data(gfx_handle<const gfx_buffer> buffer_handle, int32 offset, int32 size, void* data) = 0;

        //! \brief Maps and returns a pointer to the data of a \a gfx_buffer on the gpu.
        //! \param[in] buffer_handle The \a gfx_handle of the \a gfx_buffer to retrieve the data for.
        //! \param[in] offset The offset in the \a gfx_buffer to start retrieving the data from.
        //! \param[in] size The size of the data to retrieve in bytes.
        //! \return A pointer to the data.
        virtual void* map_buffer_data(gfx_handle<const gfx_buffer> buffer_handle, int32 offset, int32 size) = 0;

        //! \brief Sets the data of a \a gfx_texture on the gpu.
        //! \param[in] texture_handle The \a gfx_handle of the \a gfx_texture to set the data for.
        //! \param[in] desc The \a texture_set_description holding all information how and where exactly to set the data.
        //! \param[in] data Pointer to the data to set.
        virtual void set_texture_data(gfx_handle<const gfx_texture> texture_handle, const texture_set_description& desc, void* data) = 0;

        //
        // dynamic state
        //

        //! \brief Sets one or more \a gfx_viewports for rendering on the gpu.
        //! \details Requires a bound \a gfx_pipeline with enabled \a dynamic_state_viewport.
        //! \param[in] first The first viewport to set.
        //! \param[in] count The number of viewports to set.
        //! \param[in] viewports Pointer to the list of \a gfx_viewports to set.
        virtual void set_viewport(int32 first, int32 count, const gfx_viewport* viewports) = 0;

        //! \brief Sets one or more \a gfx_scissor_rectangles for rendering on the gpu.
        //! \details Requires a bound \a gfx_pipeline with enabled \a dynamic_state_scissor.
        //! \param[in] first The first scissors to set.
        //! \param[in] count The number of scissors to set.
        //! \param[in] scissors Pointer to the list of \a gfx_scissor_rectangles to set.
        virtual void set_scissor(int32 first, int32 count, const gfx_scissor_rectangle* scissors) = 0;

        //! \brief Sets the width of lines for rendering on the gpu.
        //! \details Requires a bound \a gfx_pipeline with enabled \a dynamic_state_line_width.
        //! \param[in] width The width of the lines.
        virtual void set_line_width(float width) = 0;

        //! \brief Sets the depth bias for rendering on the gpu.
        //! \details Requires a bound \a gfx_pipeline with enabled \a dynamic_state_depth_bias.
        //! \param[in] constant_factor The constant bias.
        //! \param[in] clamp The bias to clamp to.
        //! \param[in] slope_factor The bias factor depending on the steepnes of the slope.
        virtual void set_depth_bias(float constant_factor, float clamp, float slope_factor) = 0;

        //! \brief Sets the blending constants on the gpu.
        //! \details Requires a bound \a gfx_pipeline with enabled \a dynamic_state_blend_constants.
        //! \param[in] constants The constant red, green, blue and alpha blending values. Values between 0.0 and 1.0.
        virtual void set_blend_constants(const float constants[4]) = 0;

        //! \brief Sets stencil compare and reference masks on the gpu.
        //! \details Requires a bound \a gfx_pipeline with enabled \a dynamic_state_stencil_compare_mask_reference.
        //! \param[in] face_mask The \a gfx_stencil_face_flag_bits describing which faces to influence.
        //! \param[in] compare_mask Bitmask to compare the stencil values to to.
        //! \param[in] reference The reference bitmask.
        virtual void set_stencil_compare_mask_and_reference(gfx_stencil_face_flag_bits face_mask, uint32 compare_mask, uint32 reference) = 0;

        //! \brief Sets the stencil write mask on the gpu.
        //! \details Requires a bound \a gfx_pipeline with enabled \a dynamic_state_stencil_write_mask.
        //! \param[in] face_mask The \a gfx_stencil_face_flag_bits describing which faces to influence.
        //! \param[in] write_mask Bitmask for values to be writen to the stencil.
        virtual void set_stencil_write_mask(gfx_stencil_face_flag_bits face_mask, uint32 write_mask) = 0;

        //
        // pipeline
        //

        //! \brief Sets one or more \a gfx_textures as color render targets and a \a gfx_texture as depth stencil target for rendering on the gpu.
        //! \param[in] count The number of color render targets to set.
        //! \param[in] render_targets Pointer to the list of \a gfx_texture to set as color render targets.
        //! \param[in] depth_stencil_target The \a gfx_texture to set as depth stencil target.
        virtual void set_render_targets(int32 count, gfx_handle<const gfx_texture>* render_targets, gfx_handle<const gfx_texture> depth_stencil_target) = 0;

        //! \brief Calculates the mipchain for a given \a gfx_texture.
        //! \param[in] texture_handle The \a gfx_handle of the \a gfx_texture to calculate the mipchain for.
        virtual void calculate_mipmaps(gfx_handle<const gfx_texture> texture_handle) = 0;

        //! \brief Clears one or more color attachments that are currently set.
        //! \details Targets to clear have to be set before calling this function.
        //! \param[in] color_attachment The \a gfx_clear_attachment_flag_bits specifying the attachments to clear.
        //! \param[in] clear_color The clear colors red, green, blue and alpha values. Values between 0.0 and 1.0.
        virtual void clear_render_target(gfx_clear_attachment_flag_bits color_attachment, float clear_color[4]) = 0;

        //! \brief Clears the currently set depth stencil attachment.
        //! \details Target to clear has to be set before calling this function.
        //! \param[in] depth_stencil The \a gfx_clear_attachment_flag_bits specifying the attachments to clear.
        //! \param[in] clear_depth The clear value for the depth component. Value between 0.0 and 1.0.
        //! \param[in] clear_stencil The clear mask for the stencil component.
        virtual void clear_depth_stencil(gfx_clear_attachment_flag_bits depth_stencil, float clear_depth, int32 clear_stencil) = 0;

        //! \brief Sets one or more \a gfx_buffers as vertex buffers on the gpu.
        //! \param[in] count The number \a gfx_buffers to set.
        //! \param[in] buffers The list of \a gfx_buffers to set as vertex buffers.
        //! \param[in] bindings The list of bindings for each vertex buffer.
        //! \param[in] offsets The list of offset for each vertex buffer binding.
        virtual void set_vertex_buffers(int32 count, gfx_handle<const gfx_buffer>* buffers, int32* bindings, int32* offsets) = 0;

        //! \brief Sets a \a gfx_buffer as index buffer on the gpu.
        //! \param[in] buffer_handle The \a gfx_handle of the \a gfx_buffer to set as index buffer.
        //! \param[in] index_type The \a gfx_format describing the type of the indices in the buffer.
        virtual void set_index_buffer(gfx_handle<const gfx_buffer> buffer_handle, gfx_format index_type) = 0;

        //! \brief Binds a \a gfx_pipeline on the gpu.
        //! \param[in] pipeline_handle The \a gfx_handle of the \a gfx_pipeline to bind.
        virtual void bind_pipeline(gfx_handle<const gfx_pipeline> pipeline_handle) = 0;

        //! \brief Submits the resources of a \a gfx_pipeline on the gpu.
        //! \details Requires a bound \a gfx_pipeline. Resources are set beforehand with the \a shader_resource_mapping attached to the \a gfx_pipeline.
        virtual void submit_pipeline_state_resources() = 0;

        //
        // pipeline execution
        //

        //! \brief Schedules a draw call on the gpu.
        //! \details Requires a bound \a gfx_pipeline.
        //! \param[in] vertex_count The number of vertices to draw or 0 when draw call is indexed.
        //! \param[in] index_count The number of indices to draw or o when draw call is not indexed.
        //! \param[in] instance_count The number of instances when rendering instanced or 0.
        //! \param[in] base_vertex Constant value that should be added to each element of indices.
        //! \param[in] base_instance Constant value that should be added when fetching instanced vertex attributes.
        //! \param[in] index_offset The offset in the index array.
        virtual void draw(int32 vertex_count, int32 index_count, int32 instance_count, int32 base_vertex, int32 base_instance, int32 index_offset) = 0;

        //! \brief Schedules a compute dispatch on the gpu.
        //! \details Requires a bound \a gfx_pipeline.
        //! \param[in] x The number of work groups to start in x dimension.
        //! \param[in] y The number of work groups to start in y dimension.
        //! \param[in] z The number of work groups to start in z dimension.
        virtual void dispatch(int32 x, int32 y, int32 z) = 0;

        //
        // synchronization
        //

        //! \brief Adds a barrier on the gpu.
        //! \param[in] desc The \a barrier_description describing the barrier to add.
        virtual void barrier(const barrier_description& desc) = 0;

        //! \brief Creates a fence on the gpu and returns a \a gfx_semaphore for synchronization.
        //! \param[in] info The \a semaphore_create_info to create the \a gfx_sempahore.
        //! \return A \a gfx_handle of the \a gfx_semaphore to check for the created synchronization status.
        virtual gfx_handle<const gfx_semaphore> fence(const semaphore_create_info& info) = 0;

        //! \brief Makes the client (cpu) wait for a certain synchronization point.
        //! \param[in] semaphore The \a gfx_semaphore to check for the synchronization status.
        virtual void client_wait(gfx_handle<const gfx_semaphore> semaphore) = 0;

        //! \brief Makes the gpu wait for a certain synchronization point.
        //! \param[in] semaphore The \a gfx_semaphore to check for the synchronization status.
        virtual void wait(gfx_handle<const gfx_semaphore> semaphore) = 0;

        //
        // submission
        //

        //! \brief Presents a frame. Swaps display hardware framebuffers.
        virtual void present() = 0;

        //! \brief Ends a list of graphics commands.
        //! \details Has to be called after submitting commands.
        virtual void end() = 0;

        //! \brief Submits a list of graphics commands.
        //! \details Has to be called after ending the list of commands.
        virtual void submit() = 0;
    };

    //! \brief A unique pointer holding a \a graphics_device_context.
    using graphics_device_context_handle = std::unique_ptr<graphics_device_context>;
} // namespace mango

#endif // MANGO_GRAPHICS_DEVICE_CONTEXT_HPP
