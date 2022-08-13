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
