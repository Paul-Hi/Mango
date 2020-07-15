//! \file      command_buffer.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <graphics/buffer.hpp>
#include <graphics/command_buffer.hpp>
#include <graphics/framebuffer.hpp>
#include <graphics/shader_program.hpp>
#include <graphics/texture.hpp>
#include <graphics/vertex_array.hpp>

using namespace mango;

command_buffer::command_buffer()
    : m_building_state()
    , m_execution_state()
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
        head->execute(m_execution_state);
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

        void execute(graphics_state& state) override
        {
            glViewport(m_vp.x, m_vp.y, m_vp.width, m_vp.height);
            state.set_viewport(m_vp.x, m_vp.y, m_vp.width, m_vp.height);
        }
    };

    if (m_building_state.set_viewport(x, y, width, height))
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

        void execute(graphics_state& state) override
        {
            if (m_enabled)
            {
                glEnable(GL_DEPTH_TEST);
            }
            else
            {
                glDisable(GL_DEPTH_TEST);
            }
            state.set_depth_test(m_enabled);
        }
    };

    if (m_building_state.set_depth_test(enabled))
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

        void execute(graphics_state& state) override
        {
            glDepthFunc(compare_operation_to_gl(m_compare_op));
            state.set_depth_func(m_compare_op);
        }
    };

    if (m_building_state.set_depth_func(op))
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

        void execute(graphics_state& state) override
        {
            glPolygonMode(polygon_face_to_gl(m_face), polygon_mode_to_gl(m_mode));
            state.set_polygon_mode(m_face, m_mode);
        }
    };

    if (m_building_state.set_polygon_mode(face, mode))
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

        void execute(graphics_state& state) override
        {
            if (nullptr != m_vertex_array)
                glBindVertexArray(m_vertex_array->get_name());
            else
                glBindVertexArray(0);
            state.bind_vertex_array(m_vertex_array);
        }
    };

    if (m_building_state.bind_vertex_array(vertex_array))
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

        void execute(graphics_state& state) override
        {
            if (m_shader_program)
            {
                m_shader_program->use();
            }
            else
            {
                glUseProgram(0);
            }

            state.bind_shader_program(m_shader_program);
        }
    };

    if (m_building_state.bind_shader_program(shader_program))
    {
        submit<bind_shader_program_cmd>(shader_program);
    }
}

void command_buffer::bind_single_uniform(g_uint location, void* uniform_value, g_intptr data_size)
{
    class bind_single_uniform_cmd : public command
    {
      public:
        std::vector<uint8> m_data;
        g_uint m_location;
        bind_single_uniform_cmd(g_uint location, void* uniform_value, g_intptr data_size)
            : m_location(location)
        {
            m_data.resize(data_size);
            memcpy(&m_data[0], static_cast<uint8*>(uniform_value), data_size);
        }

        void execute(graphics_state& state) override
        {
            void* data    = static_cast<void*>(m_data.data());
            auto uniforms = state.m_internal_state.shader_program->get_single_bindings();
            auto it       = uniforms.listed_data.find(m_location);
            if (it == uniforms.listed_data.end())
                return; // Ignore.

            uniform_binding_data::uniform u = it->second;

            switch (u.type)
            {
            case shader_resource_type::FLOAT:
            {
                glUniform1f(m_location, *static_cast<float*>(data));
                break;
            }
            case shader_resource_type::FVEC2:
            {
                float* vec = static_cast<float*>(data);
                glUniform2f(m_location, vec[0], vec[1]);
                break;
            }
            case shader_resource_type::FVEC3:
            {
                float* vec = static_cast<float*>(data);
                glUniform3f(m_location, vec[0], vec[1], vec[2]);
                break;
            }
            case shader_resource_type::FVEC4:
            {
                float* vec = static_cast<float*>(data);
                glUniform4f(m_location, vec[0], vec[1], vec[2], vec[3]);
                break;
            }
            case shader_resource_type::INT:
            {
                glUniform1i(m_location, *static_cast<int*>(data));
                break;
            }
            case shader_resource_type::IVEC2:
            {
                int* vec = static_cast<int*>(data);
                glUniform2i(m_location, vec[0], vec[1]);
                break;
            }
            case shader_resource_type::IVEC3:
            {
                int* vec = static_cast<int*>(data);
                glUniform3i(m_location, vec[0], vec[1], vec[2]);
                break;
            }
            case shader_resource_type::IVEC4:
            {
                int* vec = static_cast<int*>(data);
                glUniform4i(m_location, vec[0], vec[1], vec[2], vec[3]);
                break;
            }
            case shader_resource_type::MAT3:
            {
                glUniformMatrix3fv(m_location, 1, GL_FALSE, static_cast<float*>(data));
                break;
            }
            case shader_resource_type::MAT4:
            {
                glUniformMatrix4fv(m_location, 1, GL_FALSE, static_cast<float*>(data));
                break;
            }
            default:
                MANGO_LOG_ERROR("Unknown uniform type!");
            }
            state.bind_single_uniform();
        }
    };

    if (m_building_state.bind_single_uniform())
    {
        submit<bind_single_uniform_cmd>(location, uniform_value, data_size);
    }
}

void command_buffer::bind_uniform_buffer(g_uint index, buffer_ptr uniform_buffer)
{
    class bind_uniform_buffer_cmd : public command
    {
      public:
        g_uint m_index;
        buffer_ptr m_buffer;
        bind_uniform_buffer_cmd(g_uint index, buffer_ptr uniform_buffer)
            : m_index(index)
            , m_buffer(uniform_buffer)
        {
        }
        void execute(graphics_state&) override
        {
            m_buffer->bind(buffer_target::UNIFORM_BUFFER, m_index, 0);
        }
    };

    if (m_building_state.bind_uniform_buffer(index, uniform_buffer))
    {
        submit<bind_uniform_buffer_cmd>(index, uniform_buffer);
    }
}

void command_buffer::bind_texture(uint32 binding, texture_ptr texture, g_uint uniform_location)
{
    class bind_texture_cmd : public command
    {
      public:
        uint32 m_binding;
        texture_ptr m_texture;
        uint32 m_uniform_location;
        bind_texture_cmd(uint32 binding, texture_ptr texture, g_uint uniform_location)
            : m_binding(binding)
            , m_texture(texture)
            , m_uniform_location(uniform_location)
        {
        }

        void execute(graphics_state& state) override
        {
            if (m_texture)
            {
                m_texture->bind_texture_unit(m_binding);
                state.bind_texture(m_binding, m_texture->get_name());
                glUniform1i(m_uniform_location, m_binding);
            }
            else
            {
                glBindTextureUnit(m_binding, 0);
                state.bind_texture(m_binding, 0);
            }
        }
    };

    if (m_building_state.bind_texture(binding, texture ? texture->get_name() : 0))
    {
        submit<bind_texture_cmd>(binding, texture, uniform_location);
    }
}

void command_buffer::bind_image_texture(uint32 binding, texture_ptr texture, g_int level, bool layered, g_int layer, base_access access, format element_format)
{
    class bind_image_texture_cmd : public command
    {
      public:
        uint32 m_binding;
        texture_ptr m_texture;
        g_int m_level;
        bool m_layered;
        g_int m_layer;
        g_enum m_access;
        g_enum m_element_format;
        bind_image_texture_cmd(uint32 binding, texture_ptr texture, g_int level, bool layered, g_int layer, base_access access, format element_format)
            : m_binding(binding)
            , m_texture(texture)
            , m_level(level)
            , m_layered(layered)
            , m_layer(layer)
            , m_access(base_access_to_gl(access))
            , m_element_format(static_cast<g_enum>(element_format))
        {
        }

        void execute(graphics_state&) override
        {
            if (m_texture)
            {
                glBindImageTexture(m_binding, m_texture->get_name(), m_level, m_layered, m_layer, m_access, m_element_format);
            }
            else
            {
                glBindImageTexture(m_binding, 0, m_level, m_layered, m_layer, m_access, m_element_format);
            }
        }
    };

    //if (m_building_state.bind_texture(binding, texture ? texture->get_name() : 0)) // TODO Paul: We need extra handling in the state. Because of layer access.
    //{
        submit<bind_image_texture_cmd>(binding, texture, level, layered, layer, access, element_format);
    //}
}

void command_buffer::bind_framebuffer(framebuffer_ptr framebuffer)
{
    class bind_framebuffer_cmd : public command
    {
      public:
        framebuffer_ptr m_framebuffer;
        bind_framebuffer_cmd(framebuffer_ptr framebuffer)
            : m_framebuffer(framebuffer)
        {
        }

        void execute(graphics_state& state) override
        {
            if (nullptr != m_framebuffer)
                m_framebuffer->bind();
            else
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            state.bind_framebuffer(m_framebuffer);
        }
    };

    if (m_building_state.bind_framebuffer(framebuffer))
    {
        submit<bind_framebuffer_cmd>(framebuffer);
    }
}

void command_buffer::add_memory_barrier(memory_barrier_bit barrier_bit)
{
    class add_memory_barrier_cmd : public command
    {
      public:
        g_enum m_barrier_bit;
        add_memory_barrier_cmd(memory_barrier_bit barrier_bit)
            : m_barrier_bit(memory_barrier_bit_to_gl(barrier_bit))
        {
        }

        void execute(graphics_state&) override
        {
            glMemoryBarrier(m_barrier_bit);
        }
    };

    // TODO Paul: Store that in the state?
    submit<add_memory_barrier_cmd>(barrier_bit);
}

void command_buffer::lock_buffer(buffer_ptr buffer)
{
    class lock_buffer_cmd : public command
    {
      public:
        buffer_ptr m_buffer;
        lock_buffer_cmd(buffer_ptr buffer)
            : m_buffer(buffer)
        {
        }

        void execute(graphics_state&) override
        {
            m_buffer->lock();
        }
    };

    // TODO Paul: Store that in the state?
    submit<lock_buffer_cmd>(buffer);
}

void command_buffer::wait_for_buffer(buffer_ptr buffer)
{
    class wait_for_buffer_cmd : public command
    {
      public:
        buffer_ptr m_buffer;
        wait_for_buffer_cmd(buffer_ptr buffer)
            : m_buffer(buffer)
        {
        }

        void execute(graphics_state&) override
        {
            m_buffer->request_wait();
        }
    };

    // TODO Paul: Store that in the state?
    submit<wait_for_buffer_cmd>(buffer);
}

void command_buffer::calculate_mipmaps(texture_ptr texture)
{
    class calculate_mipmaps_cmd : public command
    {
      public:
        texture_ptr m_texture;
        calculate_mipmaps_cmd(texture_ptr texture)
            : m_texture(texture)
        {
        }

        void execute(graphics_state&) override
        {
            if(m_texture->mipmaps())
            {
                glGenerateTextureMipmap(m_texture->get_name());
            }
        }
    };

    submit<calculate_mipmaps_cmd>(texture);
}

void command_buffer::clear_framebuffer(clear_buffer_mask buffer_mask, attachment_mask att_mask, g_float r, g_float g, g_float b, g_float a, framebuffer_ptr framebuffer)
{
    class clear_cmd : public command
    {
      public:
        framebuffer_ptr m_framebuffer;
        clear_buffer_mask m_buffer_mask;
        attachment_mask m_attachment_mask;
        g_float m_r, m_g, m_b, m_a;
        clear_cmd(framebuffer_ptr framebuffer, clear_buffer_mask buffer_mask, attachment_mask att_mask, g_float r, g_float g, g_float b, g_float a)
            : m_framebuffer(framebuffer)
            , m_buffer_mask(buffer_mask)
            , m_attachment_mask(att_mask)
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
                if (nullptr == m_framebuffer)
                {
                    const float rgb[4] = { m_r, m_g, m_b, m_a };
                    glClearNamedFramebufferfv(0, GL_COLOR, 0, rgb);
                }
                else
                {
                    // We asume that all attached color textures are also draw buffers.
                    const float rgb[4] = { m_r, m_g, m_b, m_a };
                    for (uint32 i = 0; i < 4; ++i)
                    {
                        if (m_framebuffer->get_attachment(static_cast<framebuffer_attachment>(i)) &&
                            (m_attachment_mask & static_cast<attachment_mask>(1 << i)) != attachment_mask::NONE)
                        {
                            glClearNamedFramebufferfv(m_framebuffer->get_name(), GL_COLOR, i, rgb);
                        }
                    }
                }
            }
            if ((m_buffer_mask & clear_buffer_mask::DEPTH_BUFFER) != clear_buffer_mask::NONE)
            {
                // The initial value is 1.
                const g_float d = 1.0f; // TODO Paul: Parameter!
                if (nullptr == m_framebuffer)
                {
                    glClearNamedFramebufferfv(0, GL_DEPTH, 0, &d);
                }
                else if (m_framebuffer->get_attachment(framebuffer_attachment::DEPTH_ATTACHMENT) && (m_attachment_mask & attachment_mask::DEPTH_BUFFER) != attachment_mask::NONE)
                {
                    glClearNamedFramebufferfv(m_framebuffer->get_name(), GL_DEPTH, 0, &d);
                }
            }
            if ((m_buffer_mask & clear_buffer_mask::STENCIL_BUFFER) != clear_buffer_mask::NONE)
            {
                // The initial value is 0.
                const g_int s = 1; // TODO Paul: Parameter!
                if (nullptr == m_framebuffer)
                {
                    glClearNamedFramebufferiv(0, GL_STENCIL, 0, &s);
                }
                else if (m_framebuffer->get_attachment(framebuffer_attachment::STENCIL_ATTACHMENT) && (m_attachment_mask & attachment_mask::STENCIL_BUFFER) != attachment_mask::NONE)
                {
                    glClearNamedFramebufferiv(m_framebuffer->get_name(), GL_STENCIL, 0, &s);
                }
            }
            if ((m_buffer_mask & clear_buffer_mask::DEPTH_STENCIL_BUFFER) != clear_buffer_mask::NONE)
            {
                // The initial value is 1.
                const g_float d = 1.0f; // TODO Paul: Parameter!
                // The initial value is 0.
                const g_int s = 1; // TODO Paul: Parameter!
                if (nullptr == m_framebuffer)
                {
                    glClearNamedFramebufferfi(0, GL_DEPTH_STENCIL, 0, d, s);
                }
                else if (m_framebuffer->get_attachment(framebuffer_attachment::DEPTH_STENCIL_ATTACHMENT) && (m_attachment_mask & attachment_mask::DEPTH_STENCIL_BUFFER) != attachment_mask::NONE)
                {
                    glClearNamedFramebufferfi(m_framebuffer->get_name(), GL_DEPTH_STENCIL, 0, d, s);
                }
            }
        }
    };

    submit<clear_cmd>(framebuffer, buffer_mask, att_mask, r, g, b, a);
}

void command_buffer::draw_arrays(primitive_topology topology, uint32 first, uint32 count, uint32 instance_count)
{
    class draw_arrays_cmd : public command
    {
      public:
        primitive_topology m_topology;
        uint32 m_first;
        uint32 m_count;
        uint32 m_instance_count;
        draw_arrays_cmd(primitive_topology topology, uint32 first, uint32 count, uint32 instance_count)
            : m_topology(topology)
            , m_first(first)
            , m_count(count)
            , m_instance_count(instance_count)
        {
        }

        void execute(graphics_state&) override
        {
            if (m_instance_count > 1)
            {
                glDrawArraysInstanced(static_cast<g_enum>(m_topology), m_first, m_count, m_instance_count);
            }
            else
            {
                glDrawArrays(static_cast<g_enum>(m_topology), m_first, m_count);
            }
        }
    };

    submit<draw_arrays_cmd>(topology, first, count, instance_count);
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

        void execute(graphics_state&) override
        {
            if (m_instance_count > 1)
            {
                glDrawElementsInstanced(static_cast<g_enum>(m_topology), m_count, static_cast<g_enum>(m_type), (g_byte*)NULL + m_first, m_instance_count);
            }
            else
            {
                glDrawElements(static_cast<g_enum>(m_topology), m_count, static_cast<g_enum>(m_type), (g_byte*)NULL + m_first);
            }
        }
    };

    submit<draw_elements_cmd>(topology, first, count, type, instance_count);
}

void command_buffer::dispatch_compute(uint32 num_x_groups, uint32 num_y_groups, uint32 num_z_groups)
{
    class dispatch_compute_cmd : public command
    {
      public:
        uint32 m_x_groups;
        uint32 m_y_groups;
        uint32 m_z_groups;
        dispatch_compute_cmd(uint32 num_x_groups, uint32 num_y_groups, uint32 num_z_groups)
            : m_x_groups(num_x_groups)
            , m_y_groups(num_y_groups)
            , m_z_groups(num_z_groups)
        {
        }

        void execute(graphics_state&) override
        {
            glDispatchCompute(m_x_groups, m_y_groups, m_z_groups);
        }
    };

    submit<dispatch_compute_cmd>(num_x_groups, num_y_groups, num_z_groups);
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

        void execute(graphics_state& state) override
        {
            if (m_enabled)
            {
                glEnable(GL_CULL_FACE);
            }
            else
            {
                glDisable(GL_CULL_FACE);
            }
            state.set_face_culling(m_enabled);
        }
    };

    if (m_building_state.set_face_culling(enabled))
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

        void execute(graphics_state& state) override
        {
            glCullFace(polygon_face_to_gl(m_face));
            state.set_cull_face(m_face);
        }
    };

    if (m_building_state.set_cull_face(face))
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

        void execute(graphics_state& state) override
        {
            if (m_enabled)
            {
                glEnable(GL_BLEND);
            }
            else
            {
                glDisable(GL_BLEND);
            }
            state.set_blending(m_enabled);
        }
    };

    if (m_building_state.set_blending(enabled))
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

        void execute(graphics_state& state) override
        {
            glBlendFunc(blend_factor_to_gl(m_source), blend_factor_to_gl(m_destination));
            state.set_blend_factors(m_source, m_destination);
        }
    };

    if (m_building_state.set_blend_factors(source, destination))
    {
        submit<set_blend_factors_cmd>(source, destination);
    }
}
