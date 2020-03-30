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

    // hello_rectangle
    // clang-format off
    float vertices[] = { 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
                         0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
                        -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
                        -0.5f,  0.5f, 0.0f, 0.0f, 1.0f};
    uint32 indices[] = { 0, 1, 2, 3 };

    buffer_layout layout = buffer_layout::create({ buffer_attribute::create("v_position", gpu_vec3, 3, 3 * sizeof(float), false),
                                                   buffer_attribute::create("v_texcoord",    gpu_vec2, 2, 2 * sizeof(float), false) });
    // clang-format on

    buffer_configuration buffers = { vertex_buffer_static, index_buffer_static, vertices, sizeof(vertices), layout, indices, sizeof(indices) };
    uint32 vao                   = create_vertex_array_object(buffers);

    texture_configuration logo_config = { "logo", filter_linear_mipmap_linear, filter_linear, wrap_repeat, wrap_repeat, false, true };

    shared_ptr<resource_system> rs = m_context->get_resource_system_internal().lock();
    MANGO_ASSERT(rs, "Resource System is expired!");
    const shared_ptr<texture> logo_texture = rs->load_texture("res/textures/logo.png", logo_config);
    uint32 logo_handle                     = logo_texture->handle;

    resource_binding_data logo_texture_data;
    logo_texture_data.handle               = logo_handle;
    logo_texture_data.binding_name         = "u_color_texture";
    logo_texture_data.resource_type        = gpu_sampler_texture_2d;
    render_command texture_binding_command = { input_binding, &logo_texture_data };

    shader_configuration shaders[2]     = { { "res/shader/v_hello_rectangle.glsl", vertex_shader }, { "res/shader/f_hello_rectangle.glsl", fragment_shader } };
    shader_program_configuration config = { 2, &shaders[0] };
    vao_binding_data vao_data;
    vao_data.handle = vao;

    shared_ptr<shader_system> shs = m_context->get_shader_system_internal().lock();
    MANGO_ASSERT(shs, "Shader System is expired!");

    shader_program_binding_data shader_structures;
    const shared_ptr<shader_program> program = shs->get_shader_program(config);
    MANGO_ASSERT(program, "Shader program not available!");
    shader_structures.handle       = program->handle;
    shader_structures.binding_data = program->binding_data;

    render_command vao_bind_command    = { vao_binding, &vao_data };
    render_command shader_bind_command = { shader_program_binding, &shader_structures };
    render_state state;
    state.changed  = true;
    state.blending = blend_src_alpha_and_one_minus_src_aplha;
    state.cull = cull_off;
    draw_call_data rectangle_draw_data;
    rectangle_draw_data.gpu_call          = draw_elements;
    rectangle_draw_data.state             = state;
    rectangle_draw_data.count             = 6;
    rectangle_draw_data.gpu_primitive     = triangle_strip;
    render_command rectangle_draw_command = { draw_call, &rectangle_draw_data };

    uniform_binding_data uniform_model;
    uniform_model.binding_name           = "u_model_matrix";
    glm::mat4 model                      = glm::mat4(1.0f);
    uniform_model.value                  = (void*)&model[0];
    render_command uniform_model_command = { uniform_binding, &uniform_model };
    // hello_rectangle end

    // clear command
    state.changed     = true;
    state.color_clear = { 1.0f, 0.8f, 0.133f, 1.0f };
    draw_call_data clear_data;
    clear_data.gpu_call          = clear_call;
    clear_data.state             = state;
    render_command clear_command = { draw_call, &clear_data };

    float change = 0.0f;
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
        //model = glm::scale(glm::rotate(glm::mat4(1.0f), change, glm::vec3(0.0f, 0.0f, -1.0f)), glm::vec3(4.0f, 4.0f, 1.0f));
        change += 0.025f;

        // render
        rs->start_frame();

        rs->submit(clear_command);
        rs->submit(shader_bind_command);

        scene->render();

        rs->submit(vao_bind_command);
        rs->submit(uniform_model_command);
        rs->submit(texture_binding_command);
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
