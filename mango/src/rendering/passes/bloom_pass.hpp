//! \file      bloom_pass.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2022
//! \copyright Apache License 2.0

#ifndef MANGO_BLOOM_PASS_HPP
#define MANGO_BLOOM_PASS_HPP

#include <graphics/graphics.hpp>
#include <rendering/passes/render_pass.hpp>

namespace mango
{
    //! \brief A \a render_pass calculating physically based bloom.
    class bloom_pass : public render_pass
    {
      public:
        //! \brief Constructs a the \a bloom_pass.
        //! \param[in] settings The \a bloom_settings to use.
        bloom_pass(const bloom_settings& settings);
        ~bloom_pass() = default;

        void attach(const shared_ptr<context_impl>& context) override;
        void execute(graphics_device_context_handle& device_context) override;

        void on_ui_widget() override;

        inline render_pass_execution_info get_info() override
        {
            return m_rpei;
        }

        //! \brief Set the viewport.
        //! \param[in] viewport The viewport to use.
        inline void set_viewport(const gfx_viewport& viewport)
        {
            if (m_viewport.width != viewport.width || m_viewport.height != viewport.height)
            {
                m_viewport = viewport;
                create_bloom_texture();
                return;
            }

            m_viewport = viewport;
        }

        //! \brief Set the hdr texture to apply bloom to.
        //! \param[in] hdr_texture The hdr texture to apply bloom to.
        inline void set_hdr_texture(const gfx_handle<const gfx_texture>& hdr_texture)
        {
            m_hdr_texture = hdr_texture;
        }

        //! \brief Set a mipmapped linear sampler.
        //! \param[in] mipmapped_linear_sampler The mipmapped linear sampler.
        inline void set_mipmapped_linear_sampler(const gfx_handle<const gfx_sampler>& mipmapped_linear_sampler)
        {
            m_mipmapped_linear_sampler = mipmapped_linear_sampler;
        }

        //! \brief Set a default 2d texture.
        //! \param[in] default_texture_2D The default 2d texture.
        inline void set_default_texture_2D(const gfx_handle<const gfx_texture>& default_texture_2D)
        {
            m_default_texture_2D = default_texture_2D;
        }

      private:
        //! \brief Execution info of this pass.
        render_pass_execution_info m_rpei;

        bool create_pass_resources() override;

        bool create_bloom_texture();

        //! \brief The \a bloom_settings for the pass.
        bloom_settings m_settings;

        //! \brief The vertex \a gfx_shader_stage producing a screen space triangle.
        gfx_handle<const gfx_shader_stage> m_screen_space_triangle_vertex;

        //! \brief The fragment \a gfx_shader_stage to blit hdr to bloom buffer.
        gfx_handle<const gfx_shader_stage> m_blit_fragment;

        //! \brief The fragment \a gfx_shader_stage to downsample for the bloom mipchain.
        gfx_handle<const gfx_shader_stage> m_downsample_fragment;

        //! \brief The fragment \a gfx_shader_stage to upsample and blur the bloom mipchain.
        gfx_handle<const gfx_shader_stage> m_upsample_and_blur_fragment;

        //! \brief The fragment \a gfx_shader_stage to apply bloom to hdr.
        gfx_handle<const gfx_shader_stage> m_apply_fragment;

        //! \brief Graphics pipeline to blit the hdr texture to the bloom buffer.
        gfx_handle<const gfx_pipeline> m_blit_pipeline;

        //! \brief Graphics pipeline to downsample the bloom.
        gfx_handle<const gfx_pipeline> m_downsample_pipeline;

        //! \brief Graphics pipeline to upsample and blur the bloom.
        gfx_handle<const gfx_pipeline> m_upsample_and_blur_pipeline;

        //! \brief Graphics pipeline to apply the bloom to the hdr texture.
        gfx_handle<const gfx_pipeline> m_apply_pipeline;

        //! \brief The bloom buffer with the bloom mipchain.
        gfx_handle<const gfx_texture> m_bloom_buffer;

        //! \brief The bloom buffer mip level views.
        std::vector<gfx_handle<const gfx_image_texture_view>> m_bloom_buffer_levels;

        //! \brief The \a gfx_texture to calculate and apply the bloom.
        gfx_handle<const gfx_texture> m_hdr_texture;

        //! \brief The optional lens \a gfx_texture to use.
        gfx_handle<const gfx_texture> m_lens_texture;

        //! \brief Linear sampler.
        gfx_handle<const gfx_sampler> m_mipmapped_linear_sampler;

        //! \brief The \a gfx_viewport to render to.
        gfx_viewport m_viewport;

        //! \brief The default 2d \a gfx_texture.
        gfx_handle<const gfx_texture> m_default_texture_2D;

        //! \brief The uniform buffer for the bloom data.
        gfx_handle<const gfx_buffer> m_bloom_data_buffer;

        //! \brief The current \a bloom_data.
        bloom_data m_bloom_data;

        //! \brief The mip count of the bloom buffer.
        int32 m_mip_count;
    };
} // namespace mango

#endif // MANGO_BLOOM_PASS_HPP
