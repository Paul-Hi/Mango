//! \file      application.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <core/context_impl.hpp>
#include <core/window_system_impl.hpp>
#include <graphics/command_buffer.hpp>
#include <mango/application.hpp>
#include <mango/assert.hpp>
#include <mango/scene.hpp>
#include <rendering/render_system_impl.hpp>
#include <resources/resource_system.hpp>

// test
#include <graphics/texture.hpp>

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

    // texture test
    // shared_ptr<resource_system> res = m_context->get_resource_system_internal().lock();
    // MANGO_ASSERT(res, "Resource System is expired!");
    // image_configuration config;
    // config.generate_mipmaps        = true;
    // config.is_standard_color_space = true;
    // config.name                    = "WaterBottle_baseColor";
    // shared_ptr<image> base_color   = res->load_image("res/models/WaterBottle/WaterBottle_baseColor.png", config);
    // texture_configuration tex_config;
    // tex_config.m_generate_mipmaps        = true;
    // tex_config.m_is_standard_color_space = true;
    // tex_config.m_texture_min_filter      = texture_parameter::FILTER_LINEAR_MIPMAP_LINEAR;
    // tex_config.m_texture_mag_filter      = texture_parameter::FILTER_LINEAR;
    // tex_config.m_texture_wrap_s          = texture_parameter::WRAP_REPEAT;
    // tex_config.m_texture_wrap_t          = texture_parameter::WRAP_REPEAT;
    // texture_ptr tex                      = texture::create(tex_config);
    // tex->set_data(format::SRGB8, base_color->width, base_color->height, format::RGBA, format::UNSIGNED_BYTE, base_color->data);

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
        rs->begin_render();

        // auto cmdb = rs->get_command_buffer();
        // cmdb->bind_texture(0, tex);
        // g_uint t0 = 0;
        // rs->get_command_buffer()->bind_single_uniform(0, &t0, sizeof(t0));

        //
        scene->render();
        //

        rs->finish_render();

        // swap buffers
        ws->swap_buffers();
    }

    return 0;
}

weak_ptr<context> application::get_context()
{
    return m_context;
}
