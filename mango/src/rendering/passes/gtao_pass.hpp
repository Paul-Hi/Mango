//! \file      gtao_pass.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2022
//! \copyright Apache License 2.0

#ifndef MANGO_GTAO_PASS_HPP
#define MANGO_GTAO_PASS_HPP

#include <graphics/graphics.hpp>
#include <rendering/passes/render_pass.hpp>

namespace mango
{
    //! \brief A \a render_pass calculating ground truth ambient occlusion.
    class gtao_pass : public render_pass
    {
      public:
        //! \brief Constructs a the \a gtao_pass.
        //! \param[in] settings The \a gtao_settings to use.
        gtao_pass(const gtao_settings& settings);
        ~gtao_pass() = default;

        void attach(const shared_ptr<context_impl>& context) override;
        void execute(graphics_device_context_handle& device_context) override;

        void on_ui_widget() override;

        inline render_pass_execution_info get_info() override
        {
            return s_rpei;
        }

        //! \brief Set the camera data buffer.
        //! \param[in] camera_data_buffer The camera data buffer.
        inline void set_camera_data_buffer(const gfx_handle<const gfx_buffer>& camera_data_buffer)
        {
            m_camera_data_buffer = camera_data_buffer;
        }

        //! \brief Set the viewport.
        //! \param[in] viewport The viewport to use.
        inline void set_viewport(const gfx_viewport& viewport)
        {
            if (m_viewport.width != viewport.width || m_viewport.height != viewport.height)
            {
                m_viewport = viewport;
                create_ao_textures();
                return;
            }

            m_viewport = viewport;
        }

        //! \brief Set the occlusion roughness metallic texture from the gbuffer to put occlusion in.
        //! \param[in] orm_texture The occlusion roughness metallic texture from the gbuffer to put occlusion in.
        inline void set_gbuffer_orm_texture(const gfx_handle<const gfx_texture>& orm_texture)
        {
            m_orm_texture = orm_texture;
        }

        //! \brief Set the normal texture from the gbuffer.
        //! \param[in] normal_texture The normal texture from the gbuffer.
        inline void set_gbuffer_normal_texture(const gfx_handle<const gfx_texture>& normal_texture)
        {
            m_normal_texture = normal_texture;
        }

        //! \brief Set hierarchical depth texture.
        //! \param[in] depth_texture The hierarchical depth texture.
        inline void set_hierarchical_depth_texture(const gfx_handle<const gfx_texture>& depth_texture)
        {
            m_hierarchical_depth_texture = depth_texture;
        }

        //! \brief Set full res depth texture.
        //! \param[in] depth_texture The full res depth texture.
        inline void set_full_res_depth_texture(const gfx_handle<const gfx_texture>& depth_texture)
        {
            m_depth_texture = depth_texture;
        }

        //! \brief Set a nearest sampler.
        //! \param[in] nearest_sampler The nearest sampler.
        inline void set_nearest_sampler(const gfx_handle<const gfx_sampler>& nearest_sampler)
        {
            m_nearest_sampler = nearest_sampler;
        }

        //! \brief Set a linear sampler.
        //! \param[in] linear_sampler The nearest sampler.
        inline void set_linear_sampler(const gfx_handle<const gfx_sampler>& linear_sampler)
        {
            m_linear_sampler = linear_sampler;
        }

        //! \brief Set the number of mips in the depth hierarchy.
        //! \param[in] mip_count The number of mips in the depth hierarchy.
        inline void set_depth_mip_count(int32 mip_count)
        {
            m_mip_count = mip_count;
        }

      private:
        //! \brief Execution info of this pass.
        static const render_pass_execution_info s_rpei;

        bool create_pass_resources() override;

        bool create_ao_textures();

        //! \brief The \a gtao_settings for the pass.
        gtao_settings m_settings;

        //! \brief The vertex \a gfx_shader_stage producing a screen space triangle.
        gfx_handle<const gfx_shader_stage> m_screen_space_triangle_vertex;

        //! \brief The fragment \a gfx_shader_stage to calculate gtao.
        gfx_handle<const gfx_shader_stage> m_gtao_fragment;

        //! \brief The fragment \a gfx_shader_stage to denoise the gtao spatially.
        gfx_handle<const gfx_shader_stage> m_spatial_denoise_fragment;

        //! \brief The fragment \a gfx_shader_stage to upsample the gtao.
        gfx_handle<const gfx_shader_stage> m_upsample_fragment;

        //! \brief Graphics pipeline calculating ground truth ambient occlusion.
        gfx_handle<const gfx_pipeline> m_gtao_pass_pipeline;

        //! \brief Graphics pipeline to spatially denoise the gtao.
        gfx_handle<const gfx_pipeline> m_spatial_denoise_pipeline;

        //! \brief Graphics pipeline to upsample the gtao.
        gfx_handle<const gfx_pipeline> m_upsample_pipeline;

        //! \brief One ao texture.
        gfx_handle<const gfx_texture> m_gtao_texture0;

        //! \brief One ao texture.
        gfx_handle<const gfx_texture> m_gtao_texture1;

        //! \brief The occlusion roughness metallic texture from the gbuffer to put occlusion in.
        gfx_handle<const gfx_texture> m_orm_texture;

        //! \brief The normal texture from the gbuffer.
        gfx_handle<const gfx_texture> m_normal_texture;

        //! \brief The hierarchical depth texture.
        gfx_handle<const gfx_texture> m_hierarchical_depth_texture;

        //! \brief The full res depth texture.
        gfx_handle<const gfx_texture> m_depth_texture;

        //! \brief Linear sampler.
        gfx_handle<const gfx_sampler> m_linear_sampler;

        //! \brief Nearest sampler.
        gfx_handle<const gfx_sampler> m_nearest_sampler;

        //! \brief The \a gfx_viewport to render to.
        gfx_viewport m_viewport;

        //! \brief The camera data \a gfx_buffer.
        gfx_handle<const gfx_buffer> m_camera_data_buffer;

        //! \brief The uniform buffer for the gtao data.
        gfx_handle<const gfx_buffer> m_gtao_data_buffer;

        //! \brief The current \a gtao_data.
        gtao_data m_gtao_data;

        //! \brief The hierarchical depth textures mip count.
        int32 m_mip_count;
    };
} // namespace mango

#endif // MANGO_GTAO_PASS_HPP
