//! \file      gl_graphics_device.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_GL_GRAPHICS_DEVICE_HPP
#define MANGO_GL_GRAPHICS_DEVICE_HPP

#include <core/display_impl.hpp>
#include <graphics/graphics_device.hpp>
#include <graphics/opengl/gl_framebuffer_cache.hpp>
#include <graphics/opengl/gl_graphics_state.hpp>
#include <graphics/opengl/gl_shader_program_cache.hpp>
#include <graphics/opengl/gl_vertex_array_cache.hpp>

namespace mango
{
    //! \brief An opengl \a graphics_device.
    class gl_graphics_device : public graphics_device
    {
      public:
        //! \brief Constructs a new \a gl_graphics_device.
        //! \param[in] display_window_handle The handle of the platform window used to create the graphics api.
        gl_graphics_device(display_impl::native_window_handle display_window_handle);
        ~gl_graphics_device();

        graphics_device_context_handle create_graphics_device_context(bool immediate = true) const override;
        gfx_handle<const gfx_shader_stage> create_shader_stage(const shader_stage_create_info& info) const override;
        gfx_handle<const pipeline_resource_layout> create_pipeline_resource_layout(std::initializer_list<shader_resource_binding> bindings) const override;
        graphics_pipeline_create_info provide_graphics_pipeline_create_info() override;
        compute_pipeline_create_info provide_compute_pipeline_create_info() override;
        gfx_handle<const gfx_pipeline> create_graphics_pipeline(const graphics_pipeline_create_info& info) const override;
        gfx_handle<const gfx_pipeline> create_compute_pipeline(const compute_pipeline_create_info& info) const override;
        gfx_handle<const gfx_buffer> create_buffer(const buffer_create_info& info) const override;
        gfx_handle<const gfx_texture> create_texture(const texture_create_info& info) const override;
        gfx_handle<const gfx_image_texture_view> create_image_texture_view(gfx_handle<const gfx_texture> texture, int32 level) const override;
        gfx_handle<const gfx_sampler> create_sampler(const sampler_create_info& info) const override;

        gfx_handle<const gfx_texture> get_swap_chain_render_target() override;
        gfx_handle<const gfx_texture> get_swap_chain_depth_stencil_target() override;

        void on_display_framebuffer_resize(int32 width, int32 height) override;

      private:
        //! \brief The handle of the platform window used to create the graphics api.
        display_impl::native_window_handle m_display_window_handle;

        //! \brief The \a gfx_texture representing the swap chain color render target.
        gfx_handle<const gfx_texture> m_swap_chain_render_target;
        //! \brief The \a gfx_texture representing the swap chain depth stencil render target.
        gfx_handle<const gfx_texture> m_swap_chain_depth_stencil_target;

        //! \brief The shared \a gl_graphics_state of the \a graphics_device.
        gfx_handle<gl_graphics_state> m_shared_graphics_state;
        //! \brief The shared \a gl_shader_program_cache of the \a graphics_device.
        gfx_handle<gl_shader_program_cache> m_shader_program_cache;
        //! \brief The shared \a gl_framebuffer_cache of the \a graphics_device.
        gfx_handle<gl_framebuffer_cache> m_framebuffer_cache;
        //! \brief The shared \a gl_vertex_array_cache of the \a graphics_device.
        gfx_handle<gl_vertex_array_cache> m_vertex_array_cache;
    };
} // namespace mango

#endif // MANGO_GL_GRAPHICS_DEVICE_HPP
