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
    PROFILE_ZONE;
    shared_ptr<context> mango_context = get_context().lock();
    MANGO_ASSERT(mango_context, "Context is expired!");

    window_configuration window_config;
    window_config.set_width(1920).set_height(1080).set_title(get_name());
    shared_ptr<window_system> mango_ws = mango_context->get_window_system().lock();
    MANGO_ASSERT(mango_ws, "Window System is expired!");
    mango_ws->configure(window_config);

    render_configuration render_config;
    render_config.set_base_render_pipeline(render_pipeline::deferred_pbr).set_vsync(true).enable_render_step(mango::render_step::ibl).enable_render_step(mango::render_step::shadow_map);
    shared_ptr<render_system> mango_rs = mango_context->get_render_system().lock();
    MANGO_ASSERT(mango_rs, "Render System is expired!");
    mango_rs->configure(render_config);

    ui_configuration ui_config;
    ui_config.enable_dock_space(true)
        .show_widget(mango::ui_widget::render_view)
        .show_widget(mango::ui_widget::hardware_info)
        .show_widget(mango::ui_widget::scene_inspector)
        .show_widget(mango::ui_widget::material_inspector)
        .show_widget(mango::ui_widget::entity_component_inspector)
        .show_widget(mango::ui_widget::render_system_ui)
        .submit_custom("Editor", [this](bool& enabled) {
            ImGui::Begin("Editor", &enabled);
            shared_ptr<context> mango_context = get_context().lock();
            MANGO_ASSERT(mango_context, "Context is expired!");
            auto application_scene = mango_context->get_current_scene();
            ImGui::Text("Load a GLTF Model.");
            if (ImGui::Button("Open"))
            {
                char const* filter[2] = { "*.gltf", "*.glb" };

                char* query_path = tinyfd_openFileDialog("", "res/", 2, filter, NULL, 1);
                if (query_path)
                {
                    string queried = string(query_path);

                    ptr_size pos = 0;
                    string token;
                    while ((pos = queried.find('|')) != string::npos)
                    {
                        token = queried.substr(0, pos);
                        try_open_path(application_scene, token);
                        queried.erase(0, pos + 1);
                    }
                    try_open_path(application_scene, queried);
                }
            }
            if (m_main_camera == invalid_entity || !application_scene->is_entity_alive(m_main_camera))
            {
                if (ImGui::Button("Create Editor Camera"))
                    m_main_camera = application_scene->create_default_camera();
            }

            ImGui::End();
        });

    shared_ptr<ui_system> mango_uis = mango_context->get_ui_system().lock();
    MANGO_ASSERT(mango_uis, "UI System is expired!");
    mango_uis->configure(ui_config);

    shared_ptr<scene> application_scene = std::make_shared<scene>("test_scene");
    mango_context->register_scene(application_scene);

    // camera
    m_main_camera = application_scene->create_default_camera();

    mango_context->make_scene_current(application_scene);

    // test load
    // try_open_path(application_scene, "res/models/shaderball/shaderball.glb");

    shared_ptr<input_system> mango_is = mango_context->get_input_system().lock();
    MANGO_ASSERT(mango_is, "Input System is expired!");
    // At the moment it is required to configure the window before setting any input related stuff.

    // temporary editor camera controls
    m_camera_rotation     = glm::vec2(0.0f, glm::radians(90.0f));
    m_target_offset       = glm::vec2(0.0f);
    m_last_mouse_position = glm::vec2(0.0f);
    mango_is->set_mouse_position_callback([this](float x_position, float y_position) {
        shared_ptr<context> mango_context = get_context().lock();
        MANGO_ASSERT(mango_context, "Context is expired!");
        auto application_scene = mango_context->get_current_scene();
        if (application_scene->get_active_camera_data().active_camera_entity != m_main_camera)
            return;

        shared_ptr<input_system> mango_is = mango_context->get_input_system().lock();
        MANGO_ASSERT(mango_is, "Input System is expired!");

        bool no_rotation         = mango_is->get_mouse_button(mouse_button::MOUSE_BUTTON_LEFT) == input_action::RELEASE;
        bool no_panning          = mango_is->get_mouse_button(mouse_button::MOUSE_BUTTON_RIGHT) == input_action::RELEASE;
        glm::vec2 diff           = glm::vec2(x_position, y_position) - m_last_mouse_position;
        bool offset_not_relevant = glm::length(diff) < 1.0f; // In pixels.

        m_last_mouse_position = glm::vec2(x_position, y_position);
        mango_is->hide_cursor(false);

        if ((no_rotation && no_panning) || offset_not_relevant)
            return;

        if (!no_rotation)
        {
            mango_is->hide_cursor(true);
            glm::vec2 rot = diff;
            rot.y *= -1.0f;
            m_camera_rotation += rot * 0.005f;
            m_camera_rotation.y = glm::clamp(m_camera_rotation.y, glm::radians(15.0f), glm::radians(165.0f));
            m_camera_rotation.x = m_camera_rotation.x < 0.0f ? m_camera_rotation.x + glm::radians(360.0f) : m_camera_rotation.x;
            m_camera_rotation.x = glm::mod(m_camera_rotation.x, glm::radians(360.0f));
            return;
        }

        if (!no_panning)
        {
            mango_is->hide_cursor(true);
            glm::vec2 pan   = diff;
            m_target_offset = pan * 0.005f;
        }
    });

    m_camera_radius = 10.0f;
    mango_is->set_mouse_scroll_callback([this](float, float y_offset) {
        shared_ptr<context> mango_context = get_context().lock();
        MANGO_ASSERT(mango_context, "Context is expired!");
        auto application_scene = mango_context->get_current_scene();
        if (application_scene->get_active_camera_data().active_camera_entity != m_main_camera)
            return;
        if (y_offset < 0)
        {
            m_camera_radius *= 1.04f;
        }
        else
        {
            m_camera_radius /= 1.04f;
        }
        m_camera_radius = glm::clamp(m_camera_radius, 0.01f, 100.0f);
    });

    return true;
}

void editor::update(float dt)
{
    PROFILE_ZONE;
    MANGO_UNUSED(dt);
    shared_ptr<context> mango_context = get_context().lock();

    MANGO_ASSERT(mango_context, "Context is expired!");
    auto application_scene = mango_context->get_current_scene();
    if (!application_scene->is_entity_alive(m_main_camera))
        return;
    auto cam_transform = application_scene->get_transform_component(m_main_camera);
    auto cam_data      = application_scene->get_camera_component(m_main_camera);

    if (glm::length(m_target_offset) > 0.0f)
    {
        glm::vec3 front = glm::normalize(cam_data->target - cam_transform->position);
        auto up         = cam_data->up;
        auto right      = glm::normalize(glm::cross(up, front));
        cam_data->target += right * m_target_offset.x * glm::min(m_camera_radius, 10.0f);
        cam_data->target += up * m_target_offset.y * glm::min(m_camera_radius, 10.0f);
        m_target_offset = glm::vec3(0.0f);
    }

    cam_transform->position.x = cam_data->target.x + m_camera_radius * (sinf(m_camera_rotation.y) * cosf(m_camera_rotation.x));
    cam_transform->position.y = cam_data->target.y + m_camera_radius * (cosf(m_camera_rotation.y));
    cam_transform->position.z = cam_data->target.z + m_camera_radius * (sinf(m_camera_rotation.y) * sinf(m_camera_rotation.x));
}

void editor::destroy() {}

void editor::try_open_path(const shared_ptr<mango::scene>& application_scene, string path)
{
    PROFILE_ZONE;
    auto ext = path.substr(path.find_last_of(".") + 1);
    if (ext == "glb" || ext == "gltf")
    {
        application_scene->create_entities_from_model(path);
    }
}