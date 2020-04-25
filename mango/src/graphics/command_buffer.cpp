//! \file      command_buffer.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <graphics/command_buffer.hpp>
#include <graphics/shader_program.hpp>
#include <graphics/vertex_array.hpp>

using namespace mango;

command_buffer::command_buffer()
    : m_state()
    , m_first(nullptr)
    , m_last(nullptr)
{
}

command_buffer::~command_buffer()
{
    unique_ptr<command> head = std::move(m_first);

    while (head)
    {
        head = std::move(head->m_next);
    }
    m_last = nullptr;
}

void command_buffer::execute()
{
    MANGO_ASSERT(m_first != nullptr, "Command buffer is empty!");
    MANGO_ASSERT(m_last != nullptr, "Command buffer is empty!");

    // This is also clearing the buffer
    unique_ptr<command> head = std::move(m_first);

    while (head)
    {
        head->execute(m_state);
        head = std::move(head->m_next);
    }
    m_last = nullptr;
}

void command_buffer::set_viewport(uint32 x, uint32 y, uint32 width, uint32 height)
{
    class set_viewport_cmd : public command
    {
      public:
        struct
        {
            uint32 x;
            uint32 y;
            uint32 width;
            uint32 height;
        } m_vp;

        set_viewport_cmd(uint32 x, uint32 y, uint32 width, uint32 height)
            : m_vp{ x, y, width, height }
        {
        }

        void execute(graphics_state&) override
        {
            glViewport(m_vp.x, m_vp.y, m_vp.width, m_vp.height);
        }
    };

    if (m_state.set_viewport(x, y, width, height))
    {
        submit<set_viewport_cmd>(x, y, width, height);
    }
}

void command_buffer::set_depth_test(bool enabled)
{
    class set_depth_test_cmd : public command
    {
      public:
        bool m_enabled;
        set_depth_test_cmd(bool enabled)
            : m_enabled(enabled)
        {
        }

        void execute(graphics_state&) override
        {
            if (m_enabled)
            {
                glEnable(GL_DEPTH_TEST);
            }
            else
            {
                glDisable(GL_DEPTH_TEST);
            }
        }
    };

    if (m_state.set_depth_test(enabled))
    {
        submit<set_depth_test_cmd>(enabled);
    }
}

void command_buffer::set_depth_func(compare_operation op)
{
    class set_depth_func_cmd : public command
    {
      public:
        compare_operation m_compare_op;
        set_depth_func_cmd(compare_operation op)
            : m_compare_op(op)
        {
        }

        void execute(graphics_state&) override
        {
            glDepthFunc(compare_operation_to_gl(m_compare_op));
        }
    };

    if (m_state.set_depth_func(op))
    {
        submit<set_depth_func_cmd>(op);
    }
}

void command_buffer::set_polygon_mode(polygon_face face, polygon_mode mode)
{
    class set_polygon_mode_cmd : public command
    {
      public:
        polygon_face m_face;
        polygon_mode m_mode;
        set_polygon_mode_cmd(polygon_face face, polygon_mode mode)
            : m_face(face)
            , m_mode(mode)
        {
        }

        void execute(graphics_state&) override
        {
            glPolygonMode(polygon_face_to_gl(m_face), polygon_mode_to_gl(m_mode));
        }
    };

    if (m_state.set_polygon_mode(face, mode))
    {
        submit<set_polygon_mode_cmd>(face, mode);
    }
}

void command_buffer::bind_vertex_array(vertex_array_ptr vertex_array)
{
    class bind_vertex_array_cmd : public command
    {
      public:
      vertex_array_ptr m_vertex_array;
        bind_vertex_array_cmd(vertex_array_ptr vertex_array)
            : m_vertex_array(vertex_array)
        {
        }

        void execute(graphics_state&) override
        {
            glBindVertexArray(m_vertex_array->get_name());
        }
    };

    if (m_state.bind_vertex_array(vertex_array))
    {
        submit<bind_vertex_array_cmd>(vertex_array);
    }
}

void command_buffer::bind_shader_program(shader_program_ptr shader_program)
{
    class bind_shader_program_cmd : public command
    {
      public:
        shader_program_ptr m_shader_program;
        bind_shader_program_cmd(shader_program_ptr shader_program)
            : m_shader_program(shader_program)
        {
        }

        void execute(graphics_state&) override
        {
            m_shader_program->use();
        }
    };

    if (m_state.bind_shader_program(shader_program))
    {
        submit<bind_shader_program_cmd>(shader_program);
    }
}

void command_buffer::bind_single_uniforms(void* uniform_data, g_intptr data_size)
{
    class bind_single_uniforms_cmd : public command
    {
      public:
        std::vector<uint8> m_data;
        bind_single_uniforms_cmd(void* uniform_data, g_intptr data_size)
        {
            m_data.reserve(data_size);
            memcpy(&m_data[0], static_cast<uint8*>(uniform_data), data_size);
        }

        void execute(graphics_state& state) override
        {
            uint8* count  = static_cast<uint8*>(m_data.data());
            auto uniforms = state.m_internal_state.shader_program->get_single_bindings();
            for (uniform_binding_data::uniform u : uniforms.listed_data)
            {
                void* curr = static_cast<void*>(count);
                switch (u.type)
                {
                case shader_resource_type::FLOAT:
                {
                    glUniform1f(u.location, *static_cast<float*>(curr));
                    break;
                }
                case shader_resource_type::FVEC2:
                {
                    float* vec = static_cast<float*>(curr);
                    glUniform2f(u.location, vec[0], vec[1]);
                    break;
                }
                case shader_resource_type::FVEC3:
                {
                    float* vec = static_cast<float*>(curr);
                    glUniform3f(u.location, vec[0], vec[1], vec[2]);
                    break;
                }
                case shader_resource_type::FVEC4:
                {
                    float* vec = static_cast<float*>(curr);
                    glUniform4f(u.location, vec[0], vec[1], vec[2], vec[3]);
                    break;
                }
                case shader_resource_type::INT:
                {
                    glUniform1i(u.location, *static_cast<int*>(curr));
                    break;
                }
                case shader_resource_type::IVEC2:
                {
                    int* vec = static_cast<int*>(curr);
                    glUniform2i(u.location, vec[0], vec[1]);
                    break;
                }
                case shader_resource_type::IVEC3:
                {
                    int* vec = static_cast<int*>(curr);
                    glUniform3i(u.location, vec[0], vec[1], vec[2]);
                    break;
                }
                case shader_resource_type::IVEC4:
                {
                    int* vec = static_cast<int*>(curr);
                    glUniform4i(u.location, vec[0], vec[1], vec[2], vec[3]);
                    break;
                }
                case shader_resource_type::MAT3:
                {
                    glUniformMatrix3fv(u.location, 1, GL_FALSE, static_cast<float*>(curr));
                    break;
                }
                case shader_resource_type::MAT4:
                {
                    glUniformMatrix4fv(u.location, 1, GL_FALSE, static_cast<float*>(curr));
                    break;
                }
                default:
                    MANGO_LOG_ERROR("Unknown uniform type!");
                }
                count += 64;
            }
        }
    };

    if (m_state.bind_single_uniforms())
    {
        submit<bind_single_uniforms_cmd>(uniform_data, data_size);
    }
}

void command_buffer::bind_texture(uint32 location, texture_ptr texture, sampler_ptr sampler)
{
    MANGO_UNUSED(location);
    MANGO_UNUSED(texture);
    MANGO_UNUSED(sampler);
}

void command_buffer::bind_framebuffer(framebuffer_ptr framebuffer)
{
    MANGO_UNUSED(framebuffer);
}

void command_buffer::clear_framebuffer(clear_buffer_mask buffer_mask, attachement_mask attachment_mask, g_float r, g_float g, g_float b, g_float a, framebuffer_ptr framebuffer)
{
    class clear_cmd : public command
    {
      public:
        g_uint m_framebuffer_handle;
        clear_buffer_mask m_buffer_mask;
        attachement_mask m_attachment_mask;
        g_float m_r, m_g, m_b, m_a;
        clear_cmd(g_uint framebuffer_handle, clear_buffer_mask buffer_mask, attachement_mask attachment_mask, g_float r, g_float g, g_float b, g_float a)
            : m_framebuffer_handle(framebuffer_handle)
            , m_buffer_mask(buffer_mask)
            , m_attachment_mask(attachment_mask)
            , m_r(r)
            , m_g(g)
            , m_b(b)
            , m_a(a)
        {
        }

        void execute(graphics_state&) override
        {
            // TODO Paul: Check if these clear functions do always clear correct *fv, *uiv ..... etc.
            if ((m_buffer_mask & clear_buffer_mask::COLOR_BUFFER) != clear_buffer_mask::NONE)
            {
                if (m_framebuffer_handle == 0)
                {
                    const float rgb[4] = { m_r, m_g, m_b, m_a };
                    glClearNamedFramebufferfv(m_framebuffer_handle, GL_COLOR, 0, rgb);
                }
            }
            if ((m_buffer_mask & clear_buffer_mask::DEPTH_BUFFER) != clear_buffer_mask::NONE)
            {
                // The initial value is 1.
                const g_float d = 1.0f; // TODO Paul: Parameter!
                glClearNamedFramebufferfv(m_framebuffer_handle, GL_DEPTH, 0, &d);
            }
            if ((m_buffer_mask & clear_buffer_mask::STENCIL_BUFFER) != clear_buffer_mask::NONE)
            {
                // The initial value is 0.
                const g_int s = 1; // TODO Paul: Parameter!
                glClearNamedFramebufferiv(m_framebuffer_handle, GL_STENCIL, 0, &s);
            }
            if ((m_buffer_mask & clear_buffer_mask::DEPTH_STENCIL_BUFFER) != clear_buffer_mask::NONE)
            {
                // The initial value is 1.
                const g_float d = 1.0f; // TODO Paul: Parameter!
                // The initial value is 0.
                const g_int s = 1; // TODO Paul: Parameter!
                glClearNamedFramebufferfi(m_framebuffer_handle, GL_DEPTH_STENCIL, 0, d, s);
            }
        }
    };

    g_uint handle = 0; // default framebuffer

    if (framebuffer)
    {
        // handle = framebuffer->get_name();
    }

    submit<clear_cmd>(handle, buffer_mask, attachment_mask, r, g, b, a);
}

void command_buffer::draw_arrays(primitive_topology topology, uint32 first, uint32 count, uint32 instance_count)
{
    MANGO_UNUSED(topology);
    MANGO_UNUSED(first);
    MANGO_UNUSED(count);
    MANGO_UNUSED(instance_count);
}

void command_buffer::draw_elements(primitive_topology topology, uint32 first, uint32 count, index_type type, uint32 instance_count)
{
    class draw_elements_cmd : public command
    {
      public:
        primitive_topology m_topology;
        uint32 m_first;
        uint32 m_count;
        index_type m_type;
        uint32 m_instance_count;
        draw_elements_cmd(primitive_topology topology, uint32 first, uint32 count, index_type type, uint32 instance_count)
            : m_topology(topology)
            , m_first(first)
            , m_count(count)
            , m_type(type)
            , m_instance_count(instance_count)
        {
        }

        void execute(graphics_state& state) override
        {
            if (m_instance_count > 1)
            {
                glDrawElementsInstanced(static_cast<g_enum>(m_topology), m_count, static_cast<g_enum>(m_type), nullptr, m_instance_count);
            }
            else
            {
                glDrawElements(static_cast<g_enum>(m_topology), m_count, static_cast<g_enum>(m_type), nullptr);
            }
        }
    };

    submit<draw_elements_cmd>(topology, first, count, type, instance_count);
}

void command_buffer::set_face_culling(bool enabled)
{
    class set_face_culling_cmd : public command
    {
      public:
        bool m_enabled;
        set_face_culling_cmd(bool enabled)
            : m_enabled(enabled)
        {
        }

        void execute(graphics_state&) override
        {
            if (m_enabled)
            {
                glEnable(GL_CULL_FACE);
            }
            else
            {
                glDisable(GL_CULL_FACE);
            }
        }
    };

    if (m_state.set_face_culling(enabled))
    {
        submit<set_face_culling_cmd>(enabled);
    }
}

void command_buffer::set_cull_face(polygon_face face)
{
    class set_cull_face_cmd : public command
    {
      public:
        polygon_face m_face;
        set_cull_face_cmd(polygon_face face)
            : m_face(face)
        {
        }

        void execute(graphics_state&) override
        {
            glCullFace(polygon_face_to_gl(m_face));
        }
    };

    if (m_state.set_cull_face(face))
    {
        submit<set_cull_face_cmd>(face);
    }
}

void command_buffer::set_blending(bool enabled)
{
    class set_blending_cmd : public command
    {
      public:
        bool m_enabled;
        set_blending_cmd(bool enabled)
            : m_enabled(enabled)
        {
        }

        void execute(graphics_state&) override
        {
            if (m_enabled)
            {
                glEnable(GL_BLEND);
            }
            else
            {
                glDisable(GL_BLEND);
            }
        }
    };

    if (m_state.set_blending(enabled))
    {
        submit<set_blending_cmd>(enabled);
    }
}

void command_buffer::set_blend_factors(blend_factor source, blend_factor destination)
{
    class set_blend_factors_cmd : public command
    {
      public:
        blend_factor m_source;
        blend_factor m_destination;
        set_blend_factors_cmd(blend_factor source, blend_factor destination)
            : m_source(source)
            , m_destination(destination)
        {
        }

        void execute(graphics_state&) override
        {
            glBlendFunc(blend_factor_to_gl(m_source), blend_factor_to_gl(m_destination));
        }
    };

    if (m_state.set_blend_factors(source, destination))
    {
        submit<set_blend_factors_cmd>(source, destination);
    }
}
