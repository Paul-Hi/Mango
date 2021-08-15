//! \file      gl_graphics_device_context.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_GL_GRAPHICS_DEVICE_CONTEXT_HPP
#define MANGO_GL_GRAPHICS_DEVICE_CONTEXT_HPP

#include <core/display_impl.hpp>
#include <graphics/graphics_device_context.hpp>
#include <graphics/opengl/gl_framebuffer_cache.hpp>
#include <graphics/opengl/gl_graphics_state.hpp>
#include <graphics/opengl/gl_shader_program_cache.hpp>
#include <graphics/opengl/gl_vertex_array_cache.hpp>

namespace mango
{
    //! \brief An opengl \a graphics_device_context.
    class gl_graphics_device_context : public graphics_device_context
    {
      public:
        //! \brief Constructs a new \a gl_graphics_device_context.
        //! \param[in] display_window_handle The handle of the platform window used to create the graphics api.
        //! \param[in] shared_state The shared \a gl_graphics_state of the \a graphics_device.
        //! \param[in] shader_program_cache The shared \a gl_shader_program_cache of the \a graphics_device.
        //! \param[in] framebuffer_cache The shared \a gl_framebuffer_cache of the \a graphics_device.
        //! \param[in] vertex_array_cache The shared \a gl_vertex_array_cache of the \a graphics_device.
        gl_graphics_device_context(display_impl::native_window_handle display_window_handle, gfx_handle<gl_graphics_state> shared_state, gfx_handle<gl_shader_program_cache> shader_program_cache,
                                   gfx_handle<gl_framebuffer_cache> framebuffer_cache, gfx_handle<gl_vertex_array_cache> vertex_array_cache);
        ~gl_graphics_device_context();

        void make_current() override;
        void set_swap_interval(int32 swap) override;
        void set_buffer_data(gfx_handle<const gfx_buffer> buffer_handle, int32 offset, int32 size, void* data) override;
        void* map_buffer_data(gfx_handle<const gfx_buffer> buffer_handle, int32 offset, int32 size) override;
        void set_texture_data(gfx_handle<const gfx_texture> texture_handle, const texture_set_description& desc, void* data) override;
        void begin() override;
        void set_viewport(int32 first, int32 count, const gfx_viewport* viewports) override;
        void set_scissor(int32 first, int32 count, const gfx_scissor_rectangle* scissors) override;
        void set_line_width(float width) override;
        void set_depth_bias(float constant_factor, float clamp, float slope_factor) override;
        void set_blend_constants(const float constants[4]) override;
        void set_stencil_compare_mask_and_reference(gfx_stencil_face_flag_bits face_mask, uint32 compare_mask, uint32 reference) override;
        void set_stencil_write_mask(gfx_stencil_face_flag_bits face_mask, uint32 write_mask) override;
        void set_render_targets(int32 count, gfx_handle<const gfx_texture>* render_targets, gfx_handle<const gfx_texture> depth_stencil_target) override;
        void calculate_mipmaps(gfx_handle<const gfx_texture> texture_handle) override;
        void clear_render_target(gfx_clear_attachment_flag_bits color_attachment, float clear_color[4]) override;
        void clear_depth_stencil(gfx_clear_attachment_flag_bits depth_stencil, float clear_depth, int32 clear_stencil) override;
        void set_vertex_buffers(int32 count, gfx_handle<const gfx_buffer>* buffers, int32* bindings, int32* offsets) override;
        void set_index_buffer(gfx_handle<const gfx_buffer> buffer_handle, gfx_format index_type) override;
        void bind_pipeline(gfx_handle<const gfx_pipeline> pipeline_handle) override;
        void submit_pipeline_state_resources() override;
        void draw(int32 vertex_count, int32 index_count, int32 instance_count, int32 base_vertex, int32 base_instance, int32 index_offset) override;
        void dispatch(int32 x, int32 y, int32 z) override;
        void end() override;
        void barrier(const barrier_description& desc) override;
        gfx_handle<const gfx_semaphore> fence(const semaphore_create_info& info) override;
        void client_wait(gfx_handle<const gfx_semaphore> semaphore) override;
        void wait(gfx_handle<const gfx_semaphore> semaphore) override;
        void present() override;
        void submit() override;

      private:
        //! \brief The handle of the platform window used to create the graphics api.
        display_impl::native_window_handle m_display_window_handle;
        //! \brief The shared \a gl_graphics_state of the \a graphics_device.
        gfx_handle<gl_graphics_state> m_shared_graphics_state;
        //! \brief The shared \a gl_shader_program_cache of the \a graphics_device.
        gfx_handle<gl_shader_program_cache> m_shader_program_cache;
        //! \brief The shared \a gl_framebuffer_cache of the \a graphics_device.
        gfx_handle<gl_framebuffer_cache> m_framebuffer_cache;
        //! \brief The shared \a gl_vertex_array_cache of the \a graphics_device.
        gfx_handle<gl_vertex_array_cache> m_vertex_array_cache;

        // These are only to restrict everything in case we add deferred contexts.
        //! \brief True if the \a gl_graphics_device context is currently in a recording state, else false.
        bool recording;
        //! \brief True if the \a gl_graphics_device context was submitted since the last begin() call, else false.
        bool submitted;
    };
} // namespace mango

#endif // MANGO_GL_GRAPHICS_DEVICE_CONTEXT_HPP
