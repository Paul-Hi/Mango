//! \file      renderer_impl.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2022
//! \copyright Apache License 2.0

#ifndef MANGO_RENDERER_IMPL_HPP
#define MANGO_RENDERER_IMPL_HPP

#include <core/context_impl.hpp>
#include <graphics/graphics.hpp>
#include <mango/renderer.hpp>
#include <queue>
#include <util/helpers.hpp>

namespace mango
{
    //! \brief The binding point for the \a renderer_data buffer.
#define RENDERER_DATA_BUFFER_BINDING_POINT 0
    //! \brief The binding point for the \a camera_data buffer.
#define CAMERA_DATA_BUFFER_BINDING_POINT 1
    //! \brief The binding point for the \a model_data buffer.
#define MODEL_DATA_BUFFER_BINDING_POINT 2
    //! \brief The binding point for the \a material_data buffer.
#define MATERIAL_DATA_BUFFER_BINDING_POINT 3
    //! \brief The binding point for the \a light_data buffer.
#define LIGHT_DATA_BUFFER_BINDING_POINT 4
    //! \brief The binding point for the \a shadow_data buffer.
#define SHADOW_DATA_BUFFER_BINDING_POINT 5
    //! \brief The binding point for the \a luminance_data buffer.
#define LUMINANCE_DATA_BUFFER_BINDING_POINT 6

    //! \brief The vertex input binding point for the position vertex attribute.
#define VERTEX_INPUT_POSITION 0
    //! \brief The vertex input binding point for the normal vertex attribute.
#define VERTEX_INPUT_NORMAL 1
    //! \brief The vertex input binding point for the uv vertex attribute.
#define VERTEX_INPUT_TEXCOORD 2
    //! \brief The vertex input binding point for the tangent vertex attribute.
#define VERTEX_INPUT_TANGENT 3

    //! \brief The target binding point for the gbuffer color attachment 0.
#define GBUFFER_OUTPUT_TARGET0 0
    //! \brief The target binding point for the gbuffer color attachment 1.
#define GBUFFER_OUTPUT_TARGET1 1
    //! \brief The target binding point for the gbuffer color attachment 2.
#define GBUFFER_OUTPUT_TARGET2 2
    //! \brief The target binding point for the gbuffer color attachment 3.
#define GBUFFER_OUTPUT_TARGET3 3

    //! \brief The sampler and texture binding point for the gbuffer color attachment 0.
#define GBUFFER_TEXTURE_SAMPLER_TARGET0 0
    //! \brief The sampler and texture binding point for the gbuffer color attachment 1.
#define GBUFFER_TEXTURE_SAMPLER_TARGET1 1
    //! \brief The sampler and texture binding point for the gbuffer color attachment 2.
#define GBUFFER_TEXTURE_SAMPLER_TARGET2 2
    //! \brief The sampler and texture binding point for the gbuffer color attachment 3.
#define GBUFFER_TEXTURE_SAMPLER_TARGET3 3
    //! \brief The sampler and texture binding point for the gbuffer depth stencil attachment.
#define GBUFFER_TEXTURE_SAMPLER_DEPTH 4

    //! \brief The sampler and texture binding point for the geometry base color texture.
#define GEOMETRY_TEXTURE_SAMPLER_BASE_COLOR 0
    //! \brief The sampler and texture binding point for the geometry roughness, metallic (, occlusion) texture.
#define GEOMETRY_TEXTURE_SAMPLER_ROUGHNESS_METALLIC 1
    //! \brief The sampler and texture binding point for the geometry occlusion texture.
#define GEOMETRY_TEXTURE_SAMPLER_OCCLUSION 2
    //! \brief The sampler and texture binding point for the geometry normal texture.
#define GEOMETRY_TEXTURE_SAMPLER_NORMAL 3
    //! \brief The sampler and texture binding point for the geometry emissive color texture.
#define GEOMETRY_TEXTURE_SAMPLER_EMISSIVE_COLOR 4

    //! \brief The sampler and texture binding point for the image based lighting irradiance map.
#define IBL_SAMPLER_IRRADIANCE_MAP 5
    //! \brief The sampler and texture binding point for the image based lighting radiance map.
#define IBL_SAMPLER_RADIANCE_MAP 6
    //! \brief The sampler and texture binding point for the image based lighting lookup table texture.
#define IBL_SAMPLER_LOOKUP 7
    //! \brief The samplerShadow and texture binding point for the shadow map.
#define SAMPLER_SHADOW_SHADOW_MAP 8
    //! \brief The sampler and texture binding point for the shadow map.
#define SAMPLER_SHADOW_MAP 9

    //! \brief The sampler and texture binding point for the output target color hdr attachment to compose.
#define COMPOSING_HDR_SAMPLER 0
    //! \brief The sampler and texture binding point for the output target depth attachment to pass through.
#define COMPOSING_DEPTH_SAMPLER 1

    //! \brief The image binding point for the output target color hdr attachment to compute the average luminance for.
#define HDR_IMAGE_LUMINANCE_COMPUTE 0


    //! \brief The implementation of the \a renderer.
    //! \details This class only manages the configuration of the base \a renderer and forwards everything else to the real implementation of the specific configured one.
    class renderer_impl : public renderer
    {
        MANGO_DISABLE_COPY_AND_ASSIGNMENT(renderer_impl)
      public:
        //! \brief Constructs the \a renderer_impl.
        //! \param[in] context The internally shared context of mango.
        //! \param[in] configuration The \a renderer_configuration to use for setting up the \a renderer implementation.
        renderer_impl(const renderer_configuration& configuration, const shared_ptr<context_impl>& context);
        virtual ~renderer_impl();

        //! \brief Calls the \a renderer update routine.
        //! \param[in] dt Past time since last call.
        virtual void update(float dt) = 0;

        //! \brief Presents the frame.
        //! \details Calls the present function of the graphics device and swaps buffers.
        virtual void present() = 0;

        //! \brief Renders the scene.
        //! \param[in] scene Pointer to the scene to render.
        //! \param[in] dt Past time since last call.
        virtual void render(scene_impl* scene, float dt) = 0;

        //! \brief Sets the viewport.
        //! \details  Should be called on resizing events, instead of scheduling a viewport command directly.
        //! This manages the resizing of eventually created \a framebuffers internally and schedules the \a command as well.
        //! \param[in] x The x start coordinate. Has to be a positive value.
        //! \param[in] y The y start coordinate. Has to be a positive value.
        //! \param[in] width The width of the viewport after this call. Has to be a positive value.
        //! \param[in] height The height of the viewport after this call. Has to be a positive value.
        virtual void set_viewport(int32 x, int32 y, int32 width, int32 height) = 0;

        //! \brief Retrieves and returns the base \a render_pipeline of the real implementation of the \a renderer.
        //! \details This needs to be overridden by th real \a renderer_impl.
        //! \return The current set base \a render_pipeline of the \a renderer.
        virtual render_pipeline get_base_render_pipeline() = 0;

        //! \brief Returns the output render target of the a renderer.
        //! \return The output render target.
        virtual gfx_handle<const gfx_texture> get_ouput_render_target() = 0;

        //! \brief Custom UI function.
        //! \details This can be called by any \a ui_widget and displays settings for the active \a render_step.
        //! This does not draw any window, so it needs one surrounding it.
        virtual void on_ui_widget() = 0;

        //! \brief Returns \a renderer related informations.
        //! \return The informations.
        inline const renderer_info& get_renderer_info() const override
        {
            return m_renderer_info;
        }

        inline bool is_vsync_enabled() const override
        {
            return m_vsync;
        }

        //! \brief Returns the average luminance of the last frame.
        //! \return The average luminance.
        virtual float get_average_luminance() const = 0;

      protected:
        //! \brief Mangos internal context for shared usage in all \a renderers.
        shared_ptr<context_impl> m_shared_context;

        //! \brief The hardware stats.
        renderer_info m_renderer_info; // TODO Paul: Not in use!

        //! \brief True if vertical synchronization is enabled, else false.
        bool m_vsync;
    };

} // namespace mango

#endif // #define MANGO_RENDERER_IMPL_HPP
