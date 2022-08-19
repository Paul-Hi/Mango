//! \file      auto_luminance_pass.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2022
//! \copyright Apache License 2.0

#ifndef MANGO_AUTO_LUMINANCE_PASS_HPP
#define MANGO_AUTO_LUMINANCE_PASS_HPP

#include <graphics/graphics.hpp>
#include <rendering/passes/render_pass.hpp>

namespace mango
{
    //! \brief A \a render_pass calculating luminance values for given input.
    class auto_luminance_pass : public render_pass
    {
      public:
        auto_luminance_pass()  = default;
        ~auto_luminance_pass() = default;

        void attach(const shared_ptr<context_impl>& context) override;
        void execute(graphics_device_context_handle& device_context) override;

        void on_ui_widget() override{};

        inline render_pass_execution_info get_info() override
        {
            return s_rpei;
        }

        //! \brief Get calculated average luminance value.
        //! \return Calculated average luminance.
        inline float get_average_luminance() const
        {
            return m_luminance_data_mapping->luminance;
        }

        //! \brief Set input texture.
        //! \param[in] hdr_input The input texture.
        inline void set_hdr_input(const gfx_handle<const gfx_texture>& hdr_input)
        {
            m_hdr_input = hdr_input;
        }


        //! \brief Set the size of the input texture.
        //! \param[in] width Input texture width.
        //! \param[in] height Input texture height.
        inline void set_input_size(int32 width, int32 height)
        {
            m_input_width  = width;
            m_input_height = height;
        }

        //! \brief Set the delta time.
        //! \param[in] dt The delta time.
        inline void set_delta_time(float dt)
        {
            m_dt = dt;
        }

      private:
        //! \brief Execution info of this pass.
        static const render_pass_execution_info s_rpei;

        bool create_pass_resources() override;

        //! \brief The compute \a shader_stage for the luminance buffer construction pass.
        gfx_handle<const gfx_shader_stage> m_luminance_construction_compute;
        //! \brief The compute \a shader_stage for the luminance buffer reduction pass.
        gfx_handle<const gfx_shader_stage> m_luminance_reduction_compute;

        //! \brief Compute pipeline constructing a luminance buffer.
        gfx_handle<const gfx_pipeline> m_luminance_construction_pipeline;
        //! \brief Compute pipeline reducing a luminance buffer and calculating an average luminance.
        gfx_handle<const gfx_pipeline> m_luminance_reduction_pipeline;

        //! \brief The shader storage buffer mapping for the luminance data.
        gfx_handle<const gfx_buffer> m_luminance_data_buffer;

        //! \brief The mapped luminance data from the data calculation.
        luminance_data* m_luminance_data_mapping;

        //! \brief The input texture to calculate the luminance for.
        gfx_handle<const gfx_texture> m_hdr_input;

        //! \brief The input textures width.
        int32 m_input_width;
        //! \brief The input textures height.
        int32 m_input_height;

        //! \brief The delta time to use for eye adaption.
        float m_dt;
    };
} // namespace mango

#endif // MANGO_AUTO_LUMINANCE_PASS_HPP
