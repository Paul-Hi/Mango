//! \file      deferred_pbr_renderer.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2022
//! \copyright Apache License 2.0

#ifndef MANGO_DEFERRED_PBR_RENDERER_HPP
#define MANGO_DEFERRED_PBR_RENDERER_HPP

#include <rendering/debug_drawer.hpp>
#include <rendering/light_stack.hpp>
#include <rendering/renderer_impl.hpp>
#include <rendering/renderer_pipeline_cache.hpp>
#include <rendering/renderer_bindings.hpp>
#include <rendering/passes/deferred_lighting_pass.hpp>
#include <rendering/passes/geometry_pass.hpp>
#include <rendering/passes/transparent_pass.hpp>
#include <rendering/passes/composing_pass.hpp>
#include <rendering/passes/auto_luminance_pass.hpp>

namespace mango
{
    //! \brief A \a renderer using a deferred base pipeline supporting physically based rendering.
    //! \details This system supports physically based materials with and without textures.
    class deferred_pbr_renderer : public renderer_impl
    {
      public:
        //! \brief Constructs the \a deferred_pbr_renderer.
        //! \param[in] configuration The \a renderer_configuration to use for setup.
        //! \param[in] context The internally shared context of mango.
        deferred_pbr_renderer(const renderer_configuration& configuration, const shared_ptr<context_impl>& context);
        ~deferred_pbr_renderer();

        void update(float dt) override;
        void render(scene_impl* scene, float dt) override;
        void present() override;
        void set_viewport(int32 x, int32 y, int32 width, int32 height) override;
        void on_ui_widget() override;

        inline render_pipeline get_base_render_pipeline() override
        {
            return render_pipeline::deferred_pbr;
        }

        gfx_handle<const gfx_texture> get_ouput_render_target() override
        {
            return m_output_target;
        }

      private:
        //! \brief The output color target of the deferred pipeline.
        gfx_handle<const gfx_texture> m_output_target;
        //! \brief The output depth target of the deferred pipeline.
        gfx_handle<const gfx_texture> m_ouput_depth_target;

        //! \brief The \a graphics_device of the \a renderer.
        const graphics_device_handle& m_graphics_device;
        //! \brief The gbuffer render targets of the deferred pipeline.
        std::vector<gfx_handle<const gfx_texture>> m_gbuffer_render_targets;
        //! \brief The hdr buffer render targets of the deferred pipeline. Used for auto exposure.
        std::vector<gfx_handle<const gfx_texture>> m_hdr_buffer_render_targets;
        //! \brief The postprocessing render targets of the deferred pipeline.
        std::vector<gfx_handle<const gfx_texture>> m_post_render_targets;

        //! \brief A sampler with nearest filtering and "clamp to edge" edge handling.
        gfx_handle<const gfx_sampler> m_nearest_sampler;
        //! \brief A sampler with linear filtering and "clamp to edge" edge handling.
        gfx_handle<const gfx_sampler> m_linear_sampler;
        //! \brief A sampler with linear filtering and "clamp to edge" edge handling, comparison mode is on.
        gfx_handle<const gfx_sampler> m_linear_compare_sampler;
        //! \brief A sampler with mipmapped linear filtering and "clamp to edge" edge handling.
        gfx_handle<const gfx_sampler> m_mipmapped_linear_sampler;

        //! \brief The current \a renderer_data.
        renderer_data m_renderer_data;
        //! \brief The graphics uniform buffer for uploading \a renderer_data.
        gfx_handle<const gfx_buffer> m_renderer_data_buffer;

        //! \brief The \a renderers \a deferred_lighting_pass.
        deferred_lighting_pass m_deferred_lighting_pass;
        //! \brief The \a renderers \a geometry_pass.
        geometry_pass m_opaque_geometry_pass;
        //! \brief The \a renderers \a transparent_pass.
        transparent_pass m_transparent_pass;
        //! \brief The \a renderers \a composing_pass.
        composing_pass m_composing_pass;
        //! \brief The \a renderers \a auto_luminance_pass.
        auto_luminance_pass m_auto_luminance_pass;

        //! \brief The \a renderers \a renderer_pipeline_cache to create and cache \a gfx_pipelines for the geometry.
        shared_ptr<renderer_pipeline_cache> m_pipeline_cache;

        //! \brief The \a renderers \a graphics_device_context to execute per frame graphics commands.
        graphics_device_context_handle m_frame_context;

        //! \brief Function used to create all renderer resources.
        //! \return True on success, else false.
        bool create_renderer_resources();

        //! \brief Function used to create textures and samplers.
        //! \return True on success, else false.
        bool create_textures_and_samplers();
        //! \brief Function used to create buffers.
        //! \return True on success, else false.
        bool create_buffers();
        //! \brief Function used to create passes.
        //! \return True on success, else false.
        bool create_passes();

        //! \brief Function used to update passes.
        //! \return True on success, else false.
        bool update_passes();

        //! \brief The \a debug_drawer to debug draw.
        shared_ptr<debug_drawer> m_debug_drawer;

        //! \brief Optional additional \a passes of the deferred pipeline.
        shared_ptr<render_pass> m_pipeline_extensions[mango::render_pipeline_extension::number_of_extensions];

        //! \brief True if the renderer should draw wireframe, else false.
        bool m_wireframe;

        //! \brief True if the renderer should draw debug bounds, else false.
        bool m_debug_bounds;

        //! \brief True if the renderer should cull primitives against camera and shadow frusta, else false.
        bool m_frustum_culling;

        float get_average_luminance() const override;
    };

} // namespace mango

#endif // MANGO_DEFERRED_PBR_RENDERER_HPP
