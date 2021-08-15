//! \file      graphics_device.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_GRAPHICS_DEVICE_HPP
#define MANGO_GRAPHICS_DEVICE_HPP

#include <graphics/graphics_device_context.hpp>
#include <graphics/graphics_resources.hpp>
#include <graphics/graphics_types.hpp>

namespace mango
{
    //! \brief The device interface managing all the graphics related things.
    //! \details Initializes the graphics api and provides an acbstract interface to interact with it.
    class graphics_device
    {
      public:
        virtual ~graphics_device() = default;

        //! \brief Creates a \a graphics_device_context to use for submitting commands to the gpu.
        //! \param[in] immediate True when context should be an immediate one, else false.
        //! \return A unique handle to the created context.
        virtual graphics_device_context_handle create_graphics_device_context(bool immediate = true) const = 0;

        //
        // Resource creation.
        //

        //! \brief Creates a \a gfx_shader_stage to attach to \a gfx_pipelines.
        //! \param[in] info The \a shader_create_info providing info for creation.
        //! \return A \a gfx_handle of the created \a gfx_shader_stage.
        virtual gfx_handle<const gfx_shader_stage> create_shader_stage(const shader_stage_create_info& info) const = 0;

        //! \brief Creates a \a gfx_pipeline_resource_layout to attach to \a gfx_pipelines.
        //! \param[in] bindings A list of \a shader_resource_bindings the layout consists of.
        //! \return A \a gfx_handle of the created \a gfx_pipeline_resource_layout.
        virtual gfx_handle<const pipeline_resource_layout> create_pipeline_resource_layout(std::initializer_list<shader_resource_binding> bindings) const = 0;

        //! \brief Provides a prefilled \a graphics_pipeline_create_info to use for the creation of graphics \a gfx_pipelines.
        //! \details The resulting info should be adapted to the required pipeline info.
        //! \return A prefilled \a graphics_pipeline_create_info.
        virtual graphics_pipeline_create_info provide_graphics_pipeline_create_info() = 0;

        //! \brief Provides a prefilled \a compute_pipeline_create_info to use for the creation of compute \a gfx_pipelines.
        //! \details The resulting info should be adapted to the required pipeline info.
        //! \return A prefilled \a compute_pipeline_create_info.
        virtual compute_pipeline_create_info provide_compute_pipeline_create_info() = 0;

        //! \brief Creates a graphics \a gfx_pipeline.
        //! \param[in] info The \a graphics_pipeline_create_info describing the graphics pipeline.
        //! \return A \a gfx_handle of the created graphics \a gfx_pipeline.
        virtual gfx_handle<const gfx_pipeline> create_graphics_pipeline(const graphics_pipeline_create_info& info) const = 0;

        //! \brief Creates a compute \a gfx_pipeline.
        //! \param[in] info The \a compute_pipeline_create_info describing the compute pipeline.
        //! \return A \a gfx_handle of the created compute \a gfx_pipeline.
        virtual gfx_handle<const gfx_pipeline> create_compute_pipeline(const compute_pipeline_create_info& info) const = 0;

        //! \brief Creates a \a gfx_buffer.
        //! \param[in] info The \a buffer_create_info providing info for creation.
        //! \return A \a gfx_handle of the \a gfx_buffer.
        virtual gfx_handle<const gfx_buffer> create_buffer(const buffer_create_info& info) const = 0;

        //! \brief Creates a \a gfx_texture.
        //! \param[in] info The \a texture_create_info providing info for creation.
        //! \return A \a gfx_handle of the created \a gfx_texture.
        virtual gfx_handle<const gfx_texture> create_texture(const texture_create_info& info) const = 0;

        //! \brief Creates a \a gfx_image_texture_view for a given \a gfx_texture.
        //! \param[in] texture The \a gfx_texture to create a view for.
        //! \param[in] level The level of the \a gfx_texture to create a view for.
        //! \return A \a gfx_handle of the created \a gfx_image_texture_view.
        virtual gfx_handle<const gfx_image_texture_view> create_image_texture_view(gfx_handle<const gfx_texture> texture, int32 level = 0) const = 0;

        //! \brief Creates a \a gfx_sampler.
        //! \param[in] info The \a sampler_create_info providing info for creation.
        //! \return A \a gfx_handle of the created \a gfx_sampler.
        virtual gfx_handle<const gfx_sampler> create_sampler(const sampler_create_info& info) const = 0;

        //
        // Getters for swap chain targets.
        //

        //! \brief Returns a \a gfx_texture representing the color render target of the swap chain.
        //! \return A \a gfx_handle of the \a gfx_texture representing the color render target of the swap chain.
        virtual gfx_handle<const gfx_texture> get_swap_chain_render_target() = 0;

        //! \brief Returns a \a gfx_texture representing the depth stancil target of the swap chain.
        //! \return A \a gfx_handle of the \a gfx_texture representing the depth stancil target of the swap chain.
        virtual gfx_handle<const gfx_texture> get_swap_chain_depth_stencil_target() = 0;

        //
        // Callback.
        //

        //! \brief Callback called on framebuffer size changes.
        //! \details Used to resize the swap chain targets.
        //! \param[in] width The width of the required target attachments.
        //! \param[in] height The height of the required target attachments.
        virtual void on_display_framebuffer_resize(int32 width, int32 height) = 0;
    };

    //! \brief A unique pointer holding a \a graphics_device.
    using graphics_device_handle = std::unique_ptr<graphics_device>;
} // namespace mango

#endif // MANGO_GRAPHICS_DEVICE_HPP
