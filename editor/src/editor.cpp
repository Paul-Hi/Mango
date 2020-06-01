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
    render_config.set_base_render_pipeline(render_pipeline::deferred_pbr).set_vsync(true).enable_render_step(mango::render_step::ibl);
    shared_ptr<render_system> mango_rs = mango_context->get_render_system().lock();
    MANGO_ASSERT(mango_rs, "Render System is expired!");
    mango_rs->configure(render_config);

    shared_ptr<scene> application_scene = std::make_shared<scene>("test_scene");
    mango_context->register_scene(application_scene);

    // camera
    m_main_camera = application_scene->create_default_camera();

    // scene and environment drag'n'drop
    mango_ws->set_drag_and_drop_callback([this](int count, const char** paths) {
        shared_ptr<context> mango_context = get_context().lock();
        MANGO_ASSERT(mango_context, "Context is expired!");
        auto application_scene = mango_context->get_current_scene();
        for (uint32 i = 0; i < count; i++)
        {
            string path = string(paths[i]);
            auto ext    = path.substr(path.find_last_of(".") + 1);
            if (ext == "hdr")
            {
                application_scene->remove_entity(m_environment);
                m_environment = application_scene->create_environment_from_hdr(path, 1.2);
            }
            else if (ext == "glb" || ext == "gltf")
            {
                for (entity e : m_model)
                    application_scene->remove_entity(e);

                m_model = application_scene->create_entities_from_model(path);
            }
        }
    });

    // gltf
    /*
    {
        auto entities = application_scene->create_entities_from_model("res/models/DamagedHelmet/DamagedHelmet.glb");

        entity top = entities.at(0);

        auto& trafo = application_scene->get_transform_component(top)->local_transformation_matrix;
        trafo       = glm::scale(glm::rotate(glm::translate(trafo, glm::vec3(6.0f, -1.0f, 0.0f)), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)), glm::vec3(4.0f));
    }
    {
        auto entities = application_scene->create_entities_from_model("res/models/BoomBox/BoomBox.glb");

        entity top = entities.at(0);

        auto& trafo = application_scene->get_transform_component(top)->local_transformation_matrix;
        trafo       = glm::scale(glm::rotate(glm::translate(trafo, glm::vec3(-6.0f, -1.5f, 0.0f)), glm::radians(-180.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::vec3(400.0f));
    }
    {
        auto entities = application_scene->create_entities_from_model("res/models/WaterBottle/WaterBottle.glb");

        entity top = entities.at(0);

        auto& trafo = application_scene->get_transform_component(top)->local_transformation_matrix;
        trafo       = glm::scale(glm::rotate(glm::translate(trafo, glm::vec3(0.0f, -1.0f, 0.0f)), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::vec3(24.0f));
    }

    // environment
    auto environment = application_scene->create_environment_from_hdr("res/textures/venice_sunset_4k.hdr", 0.0);
    */

    mango_context->make_scene_current(application_scene);

    return true;
}

void editor::update(float dt)
{
    static float v = 0.0f;
    v += dt * 0.5f;
    shared_ptr<context> mango_context = get_context().lock();
    MANGO_ASSERT(mango_context, "Context is expired!");
    auto application_scene = mango_context->get_current_scene();
    auto cam_transform = application_scene->get_transform_component(m_main_camera);
    auto cam_data = application_scene->get_camera_component(m_main_camera);
    cam_transform->position.x = cam_data->target.x + sin(v);
    cam_transform->position.z = cam_data->target.z + cos(v);
}

void editor::destroy() {}
