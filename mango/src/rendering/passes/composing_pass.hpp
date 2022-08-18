//! \file      composing_pass.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2022
//! \copyright Apache License 2.0

#ifndef MANGO_COMPOSING_PASS_HPP
#define MANGO_COMPOSING_PASS_HPP

#include <graphics/graphics.hpp>
#include <rendering/passes/render_pass.hpp>

namespace mango
{
    class composing_pass : public render_pass
    {
      public:
        composing_pass() = default;
        ~composing_pass() = default;

        void attach(const shared_ptr<context_impl>& context) override;
        void execute(graphics_device_context_handle& device_context) override;

        void on_ui_widget() override{};

        inline render_pass_execution_info get_info() override
        {
            return s_rpei;
        }

        inline void set_camera_data_buffer(const gfx_handle<const gfx_buffer>& camera_data_buffer)
        {
            m_camera_data_buffer = camera_data_buffer;
        }

        inline void set_renderer_data_buffer(const gfx_handle<const gfx_buffer>& renderer_data_buffer)
        {
            m_renderer_data_buffer = renderer_data_buffer;
        }

        inline void set_viewport(const gfx_viewport& viewport)
        {
            m_viewport = viewport;
        }

        inline void set_hdr_input(const gfx_handle<const gfx_texture>& hdr_input)
        {
            m_hdr_input = hdr_input;
        }

        inline void set_hdr_input_sampler(const gfx_handle<const gfx_sampler>& hdr_input_sampler)
        {
            m_hdr_input_sampler = hdr_input_sampler;
        }

        inline void set_depth_input(const gfx_handle<const gfx_texture>& depth_input)
        {
            m_depth_input = depth_input;
        }

        inline void set_depth_input_sampler(const gfx_handle<const gfx_sampler>& depth_input_sampler)
        {
            m_depth_input_sampler = depth_input_sampler;
        }

        inline void set_render_targets(const std::vector<gfx_handle<const gfx_texture>>& render_targets)
        {
            m_render_targets = render_targets;
        }

      private:
        //! \brief Execution info of this pass.
        static const render_pass_execution_info s_rpei;

        bool create_pass_resources() override;

        //! \brief The vertex \a gfx_shader_stage producing a screen space triangle.
        gfx_handle<const gfx_shader_stage> m_screen_space_triangle_vertex;
        //! \brief The fragment \a gfx_shader_stage for the composing pass.
        gfx_handle<const gfx_shader_stage> m_composing_pass_fragment;
        //! \brief Graphics \a gfx_pipeline for composing
        gfx_handle<const gfx_pipeline> m_composing_pass_pipeline;

        gfx_viewport m_viewport;

        std::vector<gfx_handle<const gfx_texture>> m_render_targets;

        gfx_handle<const gfx_buffer> m_camera_data_buffer;
        gfx_handle<const gfx_buffer> m_renderer_data_buffer;

        gfx_handle<const gfx_texture> m_hdr_input;
        gfx_handle<const gfx_sampler> m_hdr_input_sampler;
        gfx_handle<const gfx_texture> m_depth_input;
        gfx_handle<const gfx_sampler> m_depth_input_sampler;
    };
} // namespace mango

#endif // MANGO_COMPOSING_PASS_HPP
