//! \file      editor.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2022
//! \copyright Apache License 2.0

#include "editor.hpp"

using namespace mango;

MANGO_DEFINE_APPLICATION_MAIN(editor)

bool editor::create()
{
    PROFILE_ZONE;
    shared_ptr<context> mango_context = get_context().lock();
    MANGO_ASSERT(mango_context, "Context is expired!");

    display_configuration display_config;
    display_config.set_x_position_hint(100)
        .set_y_position_hint(100)
        .set_width(1920)
        .set_height(1080)
        .set_title(get_name())
        .set_native_renderer_type(display_configuration::native_renderer_type::opengl);

    m_main_display = mango_context->create_display(display_config);
    MANGO_ASSERT(m_main_display, "Display creation failed!");

    renderer_configuration renderer_config;
    renderer_config.set_base_render_pipeline(render_pipeline::deferred_pbr).set_vsync(true).set_frustum_culling(true).draw_wireframe(false).draw_debug_bounds(false);

    environment_display_settings eds;
    eds.set_render_level(0.1f);
    renderer_config.display_environment(eds);
    shadow_settings shs;
    shs.set_resolution(1024)
        .set_sample_count(16)
        .set_filter_mode(shadow_filtering::pcss_shadows)
        .set_light_size(1.5f)
        .set_offset(12.0f)
        .set_cascade_count(3)
        .set_split_lambda(0.6f)
        .set_cascade_interpolation_range(0.25f);
    renderer_config.enable_shadow_maps(shs);
    fxaa_settings fs;
    renderer_config.enable_fxaa(fs);

    m_main_renderer = mango_context->create_renderer(renderer_config);
    MANGO_ASSERT(m_main_renderer, "Renderer creation failed!");

    ui_configuration ui_config;
    ui_config.enable_dock_space(true)
        .show_widget(mango::ui_widget::render_view)
        .show_widget(mango::ui_widget::graphics_info)
        .show_widget(mango::ui_widget::renderer_ui)
        .show_widget(mango::ui_widget::scene_inspector)
        .show_widget(mango::ui_widget::scene_object_component_inspector)
        .show_widget(mango::ui_widget::primitive_material_inspector)
        .submit_custom("Editor",
                       [this](bool& enabled)
                       {
                           ImGui::Begin("Editor", &enabled);
                           ImGui::End();
                       });

    m_main_ui = mango_context->create_ui(ui_config);
    MANGO_ASSERT(m_main_ui, "UI creation failed!");

    m_current_scene = mango_context->create_scene("text_scene");
    MANGO_ASSERT(m_current_scene, "Scene creation failed!");

    // camera
    m_main_camera_node_hnd = m_current_scene->add_node("Editor Camera");
    perspective_camera editor_cam;
    editor_cam.aspect                 = 16.0f / 9.0f;
    editor_cam.z_near                 = 0.05f;
    editor_cam.z_far                  = 28.0f;
    editor_cam.vertical_field_of_view = deg_to_rad(45.0f);
    editor_cam.target                 = vec3(0.0f, 0.0f, 0.0f);
    m_current_scene->add_perspective_camera(editor_cam, m_main_camera_node_hnd); // TODO: Error check
    optional<transform&> cam_transform = m_current_scene->get_transform(m_main_camera_node_hnd);
    MANGO_ASSERT(cam_transform, "Something is broken - Main camera does not have a transform!");
    cam_transform->position = vec3(0.0f, 2.5f, 5.0f);
    cam_transform->changed  = true; // TODO Paul: This has to be called, but is forgotten too easy.
    m_current_scene->set_main_camera_node(m_main_camera_node_hnd);

    // test settings comment in to have some example scene
    {
        handle<model> bb = m_current_scene->load_model_from_gltf("res/models/WaterBottle/WaterBottle.glb");
        // handle<model> bb = m_current_scene->load_model_from_gltf("D:/Users/paulh/Documents/gltf_2_0_sample_models/2.0/Sponza/glTF/Sponza.gltf");
        // handle<model> bb                      = m_current_scene->load_model_from_gltf("D:/Users/paulh/Documents/gltf_2_0_sample_models/lumberyard_bistro/Bistro_v5_1/BistroExterior.gltf");
        optional<mango::model&> mod = m_current_scene->get_model(bb);
        MANGO_ASSERT(mod, "Model not existent!");
        handle<node> model_instance_root = m_current_scene->add_node("WaterBottle");
        m_current_scene->add_model_to_scene(bb, mod->scenarios.at(mod->default_scenario), model_instance_root);
        optional<transform&> mod_transform = m_current_scene->get_transform(model_instance_root);
        MANGO_ASSERT(mod_transform, "Model instance transform not existent!");
        mod_transform->scale *= 10.0f;
        mod_transform->changed = true;

        handle<node> directional_light_node = m_current_scene->add_node("Directional Sun Light");
        directional_light dl;
        dl.direction                = vec3(0.2f, 1.0f, 0.15f); // vec3(0.9f, 0.05f, 0.65f);
        dl.intensity                = default_directional_intensity;
        dl.color                    = mango::color_rgb(1.0f, 0.387f, 0.207f);
        dl.cast_shadows             = true;
        dl.contribute_to_atmosphere = false;
        m_current_scene->add_directional_light(dl, directional_light_node);

        handle<node> skylight_node = m_current_scene->add_node("Venice Sunset Skylight");
        m_current_scene->add_skylight_from_hdr("res/textures/venice_sunset_4k.hdr", skylight_node);
    }
    // test end

    input_handle mango_input = mango_context->get_input();
    MANGO_ASSERT(mango_input, "Input does not exist!");

    // temporary editor camera controls
    m_camera_rotation     = vec2(deg_to_rad(90.0f), deg_to_rad(45.0f));
    m_target_offset       = vec2(0.0f, 0.0f);
    m_last_mouse_position = vec2(0.0f, 0.0f);
    mango_input->register_cursor_position_callback(
        [this](double x_position, double y_position)
        {
            shared_ptr<context> mango_context = get_context().lock();
            MANGO_ASSERT(mango_context, "Context is expired!");
            optional<perspective_camera&> cam = mango_context->get_current_scene()->get_perspective_camera(m_main_camera_node_hnd);
            if (!cam)
                return;

            input_handle mango_input = mango_context->get_input();
            MANGO_ASSERT(mango_input, "Input does not exist!");

            bool no_rotation         = mango_input->get_mouse_button(mouse_button::mouse_button_left) == input_action::release;
            bool no_panning          = mango_input->get_mouse_button(mouse_button::mouse_button_middle) == input_action::release;
            vec2 diff                = vec2(x_position, y_position) - m_last_mouse_position;
            bool offset_not_relevant = diff.norm() < 1.0f; // In pixels.

            m_last_mouse_position = vec2(x_position, y_position);
            // mango_input->hide_cursor(false); TODO

            if ((no_rotation && no_panning) || offset_not_relevant)
                return;

            if (!no_rotation)
            {
                // mango_input->hide_cursor(true); TODO
                vec2 rot = diff;
                rot.y() *= -1.0f;
                m_camera_rotation += rot * 0.005f;
                m_camera_rotation.y() = clamp(m_camera_rotation.y(), deg_to_rad(15.0f), deg_to_rad(165.0f));
                m_camera_rotation.x() = m_camera_rotation.x() < 0.0f ? m_camera_rotation.x() + deg_to_rad(360.0f) : m_camera_rotation.x();
                m_camera_rotation.x() = fmodf(m_camera_rotation.x(), deg_to_rad(360.0f));
                return;
            }

            if (!no_panning)
            {
                // mango_input->hide_cursor(true); TODO
                vec2 pan        = diff;
                m_target_offset = pan * 0.005f;
            }
        });

    m_camera_radius = 10.0f;
    mango_input->register_scroll_callback(
        [this](double, double y_offset)
        {
            shared_ptr<context> mango_context = get_context().lock();
            MANGO_ASSERT(mango_context, "Context is expired!");
            optional<perspective_camera&> cam = mango_context->get_current_scene()->get_perspective_camera(m_main_camera_node_hnd);
            if (!cam)
                return;

            if (y_offset < 0)
            {
                m_camera_radius *= 1.04f;
            }
            else
            {
                m_camera_radius /= 1.04f;
            }
            m_camera_radius = clamp(m_camera_radius, 0.01f, 20.0f);
        });

    return true;
}

void editor::update(float dt)
{
    PROFILE_ZONE;
    MANGO_UNUSED(dt);
    shared_ptr<context> mango_context = get_context().lock();

    MANGO_ASSERT(mango_context, "Context is expired!");
    if (m_main_camera_node_hnd == mango_context->get_current_scene()->get_active_camera_node())
    {
        optional<perspective_camera&> cam  = mango_context->get_current_scene()->get_perspective_camera(m_main_camera_node_hnd);
        optional<transform&> cam_transform = mango_context->get_current_scene()->get_transform(m_main_camera_node_hnd);
        if (!cam || !cam_transform)
            return;

        if (m_target_offset.norm() > 0.0f)
        {
            vec3 front = (cam->target - cam_transform->position).normalized();

            if (front.norm() > 1e-5)
            {
                front = front.normalized();
            }
            else
            {
                front = GLOBAL_FORWARD;
            }

            auto right = (GLOBAL_UP.cross(front)).normalized();
            auto up    = (front.cross(right)).normalized();

            cam->target += right * m_target_offset.x() * min(m_camera_radius, 10.0f);
            cam->target += up * m_target_offset.y() * min(m_camera_radius, 10.0f);
            m_target_offset = vec2(0.0f, 0.0f);
        }

        cam_transform->position.x() = cam->target.x() + m_camera_radius * (sinf(m_camera_rotation.y()) * cosf(m_camera_rotation.x()));
        cam_transform->position.y() = cam->target.y() + m_camera_radius * (cosf(m_camera_rotation.y()));
        cam_transform->position.z() = cam->target.z() + m_camera_radius * (sinf(m_camera_rotation.y()) * sinf(m_camera_rotation.x()));
        cam_transform->changed      = true;

        auto ui = mango_context->get_ui();
        if (ui)
        {
            auto size = ui->get_content_size();
            if (size.y() > 0)
                cam->aspect = static_cast<float>(size.x()) / static_cast<float>(size.y());
        }
    }
}

void editor::destroy() {}