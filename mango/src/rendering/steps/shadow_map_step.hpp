//! \file      shadow_map_step.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_SHADOW_MAP_STEP_HPP
#define MANGO_SHADOW_MAP_STEP_HPP

#include <graphics/framebuffer.hpp>
#include <rendering/steps/pipeline_step.hpp>

namespace mango
{
    //! \brief A pipeline step adding shadow mapping.
    class shadow_map_step : public pipeline_step
    {
      public:
        bool create() override;
        void update(float dt) override;

        void attach() override;
        void execute(command_buffer_ptr& command_buffer) override;

        void destroy() override;

        command_buffer_ptr get_caster_queue()
        {
            return m_caster_queue;
        }

        void bind_shadow_maps(command_buffer_ptr& command_buffer);

        inline void set_directional_light_view_projection_matrix(const glm::mat4& view_projection)
        {
            m_view_projection = view_projection;
        }

      private:
        command_buffer_ptr m_caster_queue;
        framebuffer_ptr m_shadow_buffer;
        shader_program_ptr m_shadow_pass;

        int32 m_width  = 2048;
        int32 m_height = 2048; // hardcoded

        glm::mat4 m_view_projection;
    };
} // namespace mango

#endif // MANGO_SHADOW_MAP_STEP_HPP
