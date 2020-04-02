//! \file      application.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <core/context_impl.hpp>
#include <core/window_system_impl.hpp>
#include <mango/application.hpp>
#include <mango/assert.hpp>
#include <mango/scene.hpp>
#include <rendering/render_system_impl.hpp>
#include <resources/resource_system.hpp>
#include <resources/shader_system.hpp>

// test
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <resources/geometry_objects.hpp>

using namespace mango;

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

    shader_configuration shaders[2]     = { { "res/shader/v_hello_gltf.glsl", vertex_shader }, { "res/shader/f_hello_gltf.glsl", fragment_shader } };
    shader_program_configuration config = { 2, &shaders[0] };

    shared_ptr<shader_system> shs = m_context->get_shader_system_internal().lock();
    MANGO_ASSERT(shs, "Shader System is expired!");

    shader_program_binding_data shader_structures;
    const shared_ptr<shader_program> program = shs->get_shader_program(config);
    MANGO_ASSERT(program, "Shader program not available!");
    shader_structures.handle       = program->handle;
    shader_structures.binding_data = program->binding_data;

    render_command shader_bind_command = { shader_program_binding, &shader_structures };
    uniform_binding_data uniform_model;
    uniform_model.binding_name           = "u_model_matrix";
    glm::mat4 model                      = glm::scale(glm::mat4(1.0f), glm::vec3(10.0f));
    uniform_model.value                  = (void*)&model[0];
    render_command uniform_model_command = { uniform_binding, &uniform_model };
    // hello_rectangle end

    // clear command
    render_state state;
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
        shared_ptr<scene> scene = m_context->get_current_scene();

        // poll events
        ws->poll_events();
        should_close = ws->should_close();

        // update
        update(0.0f);
        ws->update(0.0f);
        rs->update(0.0f);
        scene->update(0.0f);
        // render
        rs->start_frame();

        rs->submit(clear_command);
        rs->submit(shader_bind_command);

        rs->submit(uniform_model_command);
        scene->render();

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
