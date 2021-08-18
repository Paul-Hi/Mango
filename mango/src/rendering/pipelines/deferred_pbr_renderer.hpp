//! \file      deferred_pbr_renderer.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_DEFERRED_PBR_RENDERER_HPP
#define MANGO_DEFERRED_PBR_RENDERER_HPP

#include <rendering/debug_drawer.hpp>
#include <rendering/light_stack.hpp>
#include <rendering/renderer_impl.hpp>
#include <rendering/renderer_pipeline_cache.hpp>
#include <rendering/steps/render_step.hpp>

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
        //! \brief A sampler with mipmapped linear filtering and "clamp to edge" edge handling.
        gfx_handle<const gfx_sampler> m_mipmapped_linear_sampler;

        //! \brief The current \a renderer_data.
        renderer_data m_renderer_data;
        //! \brief The graphics uniform buffer for uploading \a renderer_data.
        gfx_handle<const gfx_buffer> m_renderer_data_buffer;

        //! \brief The current \a camera_data.
        camera_data m_camera_data;
        //! \brief The graphics uniform buffer for uploading \a camera_data.
        gfx_handle<const gfx_buffer> m_camera_data_buffer;

        //! \brief The current \a model_data.
        model_data m_model_data;
        //! \brief The graphics uniform buffer for uploading \a model_data.
        gfx_handle<const gfx_buffer> m_model_data_buffer;

        //! \brief The current \a material_data.
        material_data m_material_data;
        //! \brief The graphics uniform buffer for uploading \a material_data.
        gfx_handle<const gfx_buffer> m_material_data_buffer;

        //! \brief The graphics uniform buffer for uploading \a light_data. Filled with data provided by the \a light_stack.
        gfx_handle<const gfx_buffer> m_light_data_buffer;

        //! \brief The vertex \a shader_stage for the deferred geometry pass.
        gfx_handle<const gfx_shader_stage> m_geometry_pass_vertex;
        //! \brief The fragment \a shader_stage for the deferred geometry pass.
        gfx_handle<const gfx_shader_stage> m_geometry_pass_fragment;
        //! \brief The fragment \a shader_stage for the forward transparent pass.
        gfx_handle<const gfx_shader_stage> m_transparent_pass_fragment;
        //! \brief The vertex \a shader_stage producing a screen space triangle.
        gfx_handle<const gfx_shader_stage> m_screen_space_triangle_vertex;
        //! \brief The fragment \a shader_stage for the deferred lighting pass.
        gfx_handle<const gfx_shader_stage> m_lighting_pass_fragment;
        //! \brief The fragment \a shader_stage for the composing pass.
        gfx_handle<const gfx_shader_stage> m_composing_pass_fragment;

        //! \brief The compute \a shader_stage for the luminance buffer construction pass.
        gfx_handle<const gfx_shader_stage> m_luminance_construction_compute;
        //! \brief The compute \a shader_stage for the luminance buffer reduction pass.
        gfx_handle<const gfx_shader_stage> m_luminance_reduction_compute;

        //! \brief Graphics pipeline calculating the lighting for opaque geometry.
        gfx_handle<const gfx_pipeline> m_lighting_pass_pipeline;
        //! \brief Graphics pipeline composing everything, applying tonemapping and gamma correction.
        gfx_handle<const gfx_pipeline> m_composing_pass_pipeline;
        //! \brief Compute pipeline constructing a luminance buffer.
        gfx_handle<const gfx_pipeline> m_luminance_construction_pipeline;
        //! \brief Compute pipeline reducing a luminance buffer and calculating an average luminance.
        gfx_handle<const gfx_pipeline> m_luminance_reduction_pipeline;

        //! \brief The \a renderers \a renderer_pipeline_cache to create and cache \a gfx_pipelines for the geometry.
        renderer_pipeline_cache m_pipeline_cache;

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
        //! \brief Function used to create shader stages.
        //! \return True on success, else false.
        bool create_shader_stages();
        //! \brief Function used to create pipeline resources.
        //! \return True on success, else false.
        bool create_pipeline_resources();

        //! \brief The light stack managing all lights.
        light_stack m_light_stack;

        //! \brief The \a debug_drawer to debug draw.
        debug_drawer m_debug_drawer;

        //! \brief Optional additional steps of the deferred pipeline.
        shared_ptr<render_step> m_pipeline_steps[mango::render_pipeline_step::number_of_steps];

        //! \brief The shader storage buffer mapping for the luminance data.
        gfx_handle<const gfx_buffer> m_luminance_data_buffer;

        //! \brief The mapped luminance data from the data calculation.
        luminance_data* m_luminance_data_mapping;

        //! \brief The \a gfx_semaphore used to synchronize luminance calculation.
        gfx_handle<const gfx_semaphore> m_luminance_semaphore;

        //! \brief True if the renderer should draw wireframe, else false.
        bool m_wireframe;

        //! \brief True if the renderer should draw debug bounds, else false.
        bool m_debug_bounds;

        //! \brief True if the renderer should cull primitives against camera and shadow frusta, else false.
        bool m_frustum_culling;

        //! \brief The \a gfx_semaphore used to synchronize \a renderer frames.
        gfx_handle<const gfx_semaphore> m_frame_semaphore;

        //! \brief Calculates exposure and adapts physical camera parameters.
        //! \param[in,out] camera The current \a scene_camera.
        //! \param[in] adaptive True if the exposure should be adaptive, else false.
        //! \return Returns the calculated camera exposure.
        float apply_exposure(scene_camera& camera, bool adaptive);
    };

} // namespace mango

#endif // MANGO_DEFERRED_PBR_RENDERER_HPP
