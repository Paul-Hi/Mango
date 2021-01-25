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
    render_config.set_base_render_pipeline(render_pipeline::deferred_pbr)
        .set_vsync(true)
        .enable_render_step(mango::render_step::cubemap)
        .enable_render_step(mango::render_step::shadow_map)
        .enable_render_step(mango::render_step::fxaa);
    shared_ptr<render_system> mango_rs = mango_context->get_render_system().lock();
    MANGO_ASSERT(mango_rs, "Render System is expired!");
    mango_rs->configure(render_config);

    ui_configuration ui_config;
    ui_config.enable_dock_space(true)
        .show_widget(mango::ui_widget::render_view)
        .show_widget(mango::ui_widget::hardware_info)
        .show_widget(mango::ui_widget::scene_inspector)
        .show_widget(mango::ui_widget::entity_component_inspector)
        .show_widget(mango::ui_widget::render_system_ui)
        .submit_custom("Editor", [this](bool& enabled) {
            ImGui::Begin("Editor", &enabled);
            shared_ptr<context> mango_context = get_context().lock();
            MANGO_ASSERT(mango_context, "Context is expired!");
            auto application_scene = mango_context->get_current_scene();
            entity main_cam        = m_main_camera;
            mango::custom_info("Mango Editor Custom", [&main_cam, &application_scene]() {
                if (main_cam == invalid_entity || !application_scene->is_entity_alive(main_cam))
                {
                    if (ImGui::Button("Create Editor Camera"))
                        main_cam = application_scene->create_default_camera();
                    application_scene->get_component<tag_component>(main_cam)->tag_name = "Editor Camera";
                }
            });
            m_main_camera = main_cam;
            ImGui::End();
        });

    shared_ptr<ui_system> mango_uis = mango_context->get_ui_system().lock();
    MANGO_ASSERT(mango_uis, "UI System is expired!");
    mango_uis->configure(ui_config);

    shared_ptr<scene> application_scene = std::make_shared<scene>("test_scene");
    mango_context->register_scene(application_scene);

    // camera
    m_main_camera                                                            = application_scene->create_default_camera();
    application_scene->get_component<tag_component>(m_main_camera)->tag_name = "Editor Camera";

    mango_context->make_scene_current(application_scene);

    // test settings comment in to have some example scene
    {
        cubemap_step_configuration ibl_config;
        ibl_config.set_render_level(0.1f);
        mango_rs->setup_cubemap_step(ibl_config);

        shadow_step_configuration shadow_config;
        shadow_config.set_resolution(2048).set_sample_count(16).set_offset(12.0f).set_cascade_count(3).set_split_lambda(0.5f).set_cascade_interpolation_range(0.5f);
        mango_rs->setup_shadow_map_step(shadow_config);

        fxaa_step_configuration fxaa_config;
        fxaa_config.set_quality_preset(fxaa_quality_preset::medium_quality).set_subpixel_filter(0.0f);
        mango_rs->setup_fxaa_step(fxaa_config);

        // application_scene->create_entities_from_model("res/models/MetalRoughSpheresNoTextures/MetalRoughSpheresNoTextures.glb");
        entity sphere = application_scene->create_empty();
        application_scene->add_component<material_component>(sphere);
        application_scene->get_component<tag_component>(sphere)->tag_name = "Created Sphere";
        auto mpc = application_scene->add_component<mesh_primitive_component>(sphere);
        auto sf  = mesh_factory::get_sphere_factory();
        sf->set_normals(true).set_rings(22).set_segments(44).set_uv_tiling({ 2.0f, 2.0f });
        sf->create_mesh_primitive_component(mpc);

        entity lighting                                                     = application_scene->create_skylight_from_hdr("res/textures/venice_sunset_4k.hdr");
        application_scene->get_component<tag_component>(lighting)->tag_name = "Global Lighting";
        auto d_l_c                                                          = application_scene->add_component<directional_light_component>(lighting);
        if (d_l_c)
        {
            d_l_c->light.direction     = glm::vec3(0.9f, 0.05f, 0.65f);
            d_l_c->light.intensity     = 89000.0f;
            d_l_c->light.light_color   = mango::color_rgb({ 1.0f, 0.387f, 0.207f });
            d_l_c->light.cast_shadows  = true;
            d_l_c->light.atmospherical = true;
        }
    }
    // test end

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

        bool no_rotation         = mango_is->get_mouse_button(mouse_button::mouse_button_left) == input_action::release;
        bool no_panning          = mango_is->get_mouse_button(mouse_button::mouse_button_middle) == input_action::release;
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
        m_camera_radius = glm::clamp(m_camera_radius, 0.01f, 20.0f);
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
    auto cam_transform = application_scene->get_component<transform_component>(m_main_camera);
    auto cam_data      = application_scene->get_component<camera_component>(m_main_camera);
    if (!cam_data || !cam_transform)
        return;

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