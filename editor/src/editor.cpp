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

    auto entities = application_scene->create_entities_from_model("res/models/BoomBox/BoomBox.gltf");

    entity top = entities.at(0);

    auto& trafo = application_scene->get_transform_component(top)->local_transformation_matrix;
    trafo = glm::scale(glm::rotate(glm::translate(trafo, glm::vec3(0.0f, -1.0f, 0.0f)), glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::vec3(400.0f));

    mango_context->make_scene_current(application_scene);

    return true;
}

void editor::update(float dt)
{
    MANGO_UNUSED(dt);
    static float v = 0.0f;
    v += 0.01f;
    shared_ptr<context> mango_context = get_context().lock();
    MANGO_ASSERT(mango_context, "Context is expired!");
    auto application_scene = mango_context->get_current_scene();
    auto& trafo = application_scene->get_transform_component(m_main_camera)->local_transformation_matrix;
    trafo = glm::translate(glm::mat4(1.0), glm::vec3(sin(v) * 20.0f, 0.0f, cos(v) * 20.0f));
}

void editor::destroy() {}
