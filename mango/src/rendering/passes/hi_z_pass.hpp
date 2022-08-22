//! \file      hi_z_pass.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2022
//! \copyright Apache License 2.0

#ifndef MANGO_HI_Z_PASS_HPP
#define MANGO_HI_Z_PASS_HPP

#include <graphics/graphics.hpp>
#include <rendering/passes/render_pass.hpp>

namespace mango
{
    //! \brief A \a render_pass calculating a min/max mip chain for the depth buffer.
    class hi_z_pass : public render_pass
    {
      public:
        hi_z_pass()
            : m_depth_width(1)
            , m_depth_height(1){};
        ~hi_z_pass() = default;

        void attach(const shared_ptr<context_impl>& context) override;
        void execute(graphics_device_context_handle& device_context) override;

        void on_ui_widget() override{};

        inline render_pass_execution_info get_info() override
        {
            return s_rpei;
        }

        //! \brief Retrieve the generated hierarchical depth texture.
        //! \return The computed hierarchical depth texture.
        inline gfx_handle<const gfx_texture> get_hierarchical_depth_buffer()
        {
            return m_hi_z_texture;
        }

        //! \brief Set depth texture.
        //! \param[in] depth_texture The depth texture.
        inline void set_depth_texture(const gfx_handle<const gfx_texture>& depth_texture)
        {
            m_depth_texture = depth_texture;
        }

        //! \brief Set a nearest sampler.
        //! \param[in] nearest_sampler The nearest sampler.
        inline void set_nearest_sampler(const gfx_handle<const gfx_sampler>& nearest_sampler)
        {
            m_nearest_sampler = nearest_sampler;
        }

        //! \brief Set the size of the depth texture.
        //! \param[in] width Input texture width.
        //! \param[in] height Input texture height.
        inline void set_depth_size(int32 width, int32 height)
        {
            if (m_depth_width != width || m_depth_height != height)
            {
                m_depth_width  = width;
                m_depth_height = height;
                recreate_hi_z_texture();
            }
        }

      private:
        //! \brief Execution info of this pass.
        static const render_pass_execution_info s_rpei;

        bool create_pass_resources() override;

        //! \brief Recreate th hierarchical depth texture on resize.
        bool recreate_hi_z_texture();

        //! \brief The compute \a shader_stage to calculate and write the downsampled mips.
        gfx_handle<const gfx_shader_stage> m_hi_z_compute;

        //! \brief Compute pipeline constructing a hierarchical depth buffer.
        gfx_handle<const gfx_pipeline> m_hi_z_construction_pipeline;

        //! \brief The depth texture to calculate mip chain for.
        gfx_handle<const gfx_texture> m_depth_texture;

        //! \brief Nearest sampler.
        gfx_handle<const gfx_sampler> m_nearest_sampler;

        //! \brief The hierarchical depth texture to fill.
        gfx_handle<const gfx_texture> m_hi_z_texture;

        //! \brief The uniform buffer for the hi-z data.
        gfx_handle<const gfx_buffer> m_hi_z_data_buffer;

        //! \brief The current \a hi_z_data.
        hi_z_data m_hi_z_data;

        //! \brief The depth textures width.
        int32 m_depth_width;
        //! \brief The depth textures height.
        int32 m_depth_height;
    };
} // namespace mango

#endif // MANGO_HI_Z_PASS_HPP
