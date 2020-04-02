//! \file      editor.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include "editor.hpp"
#include "glm/gtc/matrix_transform.hpp"

using namespace mango;

MANGO_DEFINE_APPLICATION_MAIN(editor)

bool editor::create()
{
    shared_ptr<context> mango_context = get_context().lock();
    MANGO_ASSERT(mango_context, "Context is expired!");

    window_configuration window_config;
    window_config.set_width(1920).set_height(1080).set_title(get_name());
    shared_ptr<window_system> mango_ws = mango_context->get_window_system().lock();
    MANGO_ASSERT(mango_ws, "Window System is expired!");
    mango_ws->configure(window_config);

    render_configuration render_config;
    render_config.set_base_render_pipeline(render_pipeline::deferred_pbr).set_vsync(true);
    shared_ptr<render_system> mango_rs = mango_context->get_render_system().lock();
    MANGO_ASSERT(mango_rs, "Render System is expired!");
    mango_rs->configure(render_config);

    shared_ptr<scene> application_scene = std::make_shared<scene>("test_scene");
    mango_context->register_scene(application_scene);

    m_main_camera = application_scene->create_default_camera();

    application_scene->create_entities_from_model("res/models/WaterBottle/WaterBottle.gltf");

    mango_context->make_scene_current(application_scene);

    return true;
}

void editor::update(float dt)
{
    MANGO_UNUSED(dt);
}

void editor::destroy() {}
