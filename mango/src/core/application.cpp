//! \file      application.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <core/context_impl.hpp>
#include <core/window_system_impl.hpp>
#include <mango/application.hpp>
#include <mango/assert.hpp>
#include <rendering/render_system_impl.hpp>
#include <resources/shader_system.hpp>

using namespace mango;

static void hello_rectangle(uint32& vao);

application::application()
{
    m_context = std::make_shared<context_impl>();
    m_context->create();
}

application::~application()
{
    MANGO_ASSERT(m_context, "Context is not valid!");
    m_context->destroy();
}

uint32 application::run(uint32 t_argc, char** t_argv)
{
    MANGO_UNUSED(t_argc);
    MANGO_UNUSED(t_argv);

    bool should_close = false;

    // hello_rectangle
    uint32 vao;
    hello_rectangle(vao);

    shader_configuration shaders[2]     = { { "res/shader/v_hello_rectangle.glsl", vertex_shader }, { "res/shader/f_hello_rectangle.glsl", fragment_shader } };
    shader_program_configuration config = { 2, &shaders[0] };

    shared_ptr<shader_system> shs = m_context->get_shader_system_internal().lock();
    MANGO_ASSERT(shs, "Shader System is expired!");

    shader_program_binding_data shader_structures;
    shader_structures.handle = shs->get_shader_program(config);
    vao_binding_data vao_data;
    vao_data.handle = vao;

    render_command vao_bind_command    = { vao_binding, &vao_data };
    render_command shader_bind_command = { shader_program_binding, &shader_structures };
    render_state state;
    state.changed   = false;
    draw_call_data rectangle_draw_data;
    rectangle_draw_data.gpu_call          = draw_elements;
    rectangle_draw_data.state             = state;
    rectangle_draw_data.count             = 6;
    rectangle_draw_data.gpu_primitive     = triangle_strip;
    render_command rectangle_draw_command = { draw_call, &rectangle_draw_data };
    // hello_rectangle end

    // clear command
    state.changed     = true;
    state.color_clear = { 1.0f, 0.8f, 0.133f, 1.0f };
    draw_call_data clear_data;
    clear_data.gpu_call          = clear_call;
    clear_data.state             = state;
    render_command clear_command = { draw_call, &clear_data };

    while (!should_close)
    {
        shared_ptr<window_system_impl> ws = m_context->get_window_system_internal().lock();
        MANGO_ASSERT(ws, "Window System is expired!");
        shared_ptr<render_system_impl> rs = m_context->get_render_system_internal().lock();
        MANGO_ASSERT(rs, "Render System is expired!");

        // poll events
        ws->poll_events();
        should_close = ws->should_close();

        // update
        ws->update(0.0f);
        rs->update(0.0f);

        // render
        rs->start_frame();

        rs->submit(clear_command);
        rs->submit(shader_bind_command);
        rs->submit(vao_bind_command);
        rs->submit(rectangle_draw_command);

        rs->finish_frame();

        rs->render();

        // swap buffers
        ws->swap_buffers();
    }

    return 0;
}

weak_ptr<context> application::get_context()
{
    return m_context;
}

#include <glad/glad.h>

static void hello_rectangle(uint32& vao)
{
    float vertices[] = { 0.5f, -0.5f, 0.0f, 0.5f, 0.5f, 0.0f, -0.5f, -0.5f, 0.0f, -0.5f, 0.5f, 0.0f };
    uint32 indices[] = { 0, 1, 2, 3 };

    uint32 vbo, ibo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ibo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}