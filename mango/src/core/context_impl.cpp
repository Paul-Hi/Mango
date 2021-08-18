//! \file      context_impl.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#include <core/context_impl.hpp>
#include <core/display_event_handler_impl.hpp>
#include <core/input_impl.hpp>
#include <graphics/graphics.hpp>
#include <mango/application.hpp>
#include <mango/assert.hpp>
#include <mango/profile.hpp>
#include <rendering/pipelines/deferred_pbr_renderer.hpp>
#include <rendering/renderer_impl.hpp>
#include <resources/resources_impl.hpp>
#include <scene/scene_impl.hpp>
#include <ui/ui_impl.hpp>
#if defined(WIN32)
#include <core/glfw/glfw_display.hpp>
#elif defined(LINUX)
#include <core/glfw/glfw_display.hpp>
#endif

using namespace mango;

context_impl::context_impl() {}

context_impl::~context_impl() {}

void context_impl::set_application(const shared_ptr<application>& application)
{
    PROFILE_ZONE;
    if (m_application)
    {
        m_application->destroy();
        MANGO_LOG_INFO("Destroying the current application '{0}'.", m_application->get_name());
    }
    m_application = application;
    MANGO_LOG_INFO("Setting the application to '{0}'.", m_application->get_name());
    bool success = m_application->create();
    MANGO_ASSERT(success, "Creation of application '{0}' failed!", m_application->get_name());
}

scene_handle context_impl::create_scene(const string& name)
{
    MANGO_ASSERT(m_current_scene == nullptr, "Only one scene is allowed at the moment!");

    m_current_scene = mango::make_unique<scene_impl>(name, shared_from_this()); // TODO Paul: Only one scene at the moment!

    return m_current_scene.get();
}

void context_impl::destroy_scene(scene_handle scene_in)
{
    // TODO Paul: Only one scene at the moment!
    MANGO_ASSERT(m_current_scene.get() == scene_in, "Only one scene is allowed at the moment!");
    m_current_scene.release();
    m_current_scene = nullptr;
}

scene_handle context_impl::get_current_scene()
{
    return m_current_scene.get();
}

shared_ptr<application> context_impl::get_application()
{
    return m_application;
}

display_handle context_impl::create_display(const display_configuration& config)
{
    MANGO_ASSERT(m_display == nullptr, "Only one display is allowed at the moment!");
    MANGO_ASSERT(m_event_handler, "Context is not created!"); // TODO Paul

    display_info info;
    info.x               = config.get_x_position_hint();
    info.y               = config.get_y_position_hint();
    info.width           = config.get_width();
    info.height          = config.get_height();
    info.native_renderer = config.get_native_renderer_type();
    info.title           = config.get_title();
    info.decorated       = config.is_decorated();

    info.pixel_format          = display_info::pixel_format::rgb888;          // TODO Paul: Settings.
    info.depth_stencil_format  = display_info::depth_stencil_format::depth24; // TODO Paul: Settings.
    info.display_event_handler = m_event_handler;

    m_display = mango::make_unique<glfw_display>(std::forward<const display_info&>(info)); // TODO Paul: Only one display at the moment!

    m_graphics_device = graphics::create_graphics_device(m_display->native_handle()); // TODO Paul: One graphics_device per display???

    return m_display.get();
}

void context_impl::destroy_display(display_handle display_in)
{
    // TODO Paul: Only one display at the moment!
    MANGO_ASSERT(m_display.get() == display_in, "Only one display is allowed at the moment!");
    m_display->quit();
    m_display.release();
    m_display = nullptr;

    m_graphics_device = nullptr;
}

display_handle context_impl::get_display()
{
    return m_display.get();
}

const unique_ptr<display_impl>& context_impl::get_internal_display()
{
    return m_display;
}

const unique_ptr<renderer_impl>& context_impl::get_internal_renderer()
{
    return m_renderer;
}

const unique_ptr<scene_impl>& context_impl::get_internal_scene()
{
    return m_current_scene;
}

input_handle context_impl::get_input()
{
    return m_input.get();
}

resources_handle context_impl::get_resources()
{
    return m_resources.get();
}

const unique_ptr<resources_impl>& context_impl::get_internal_resources()
{
    return m_resources;
}

const unique_ptr<graphics_device>& context_impl::get_graphics_device()
{
    return m_graphics_device;
}

ui_handle context_impl::create_ui(const ui_configuration& config)
{
    m_ui = mango::make_unique<ui_impl>(config, shared_from_this()); // TODO Paul: Only one ui at the moment!

    return m_ui.get();
}

void context_impl::destroy_ui(ui_handle ui_in)
{
    // TODO Paul: Only one ui at the moment!
    MANGO_ASSERT(m_ui.get() == ui_in, "Only one ui is allowed at the moment!");
    m_ui.release();
    m_ui = nullptr;
}

ui_handle context_impl::get_ui()
{
    return m_ui.get();
}

renderer_handle context_impl::create_renderer(const renderer_configuration& config)
{
    // TODO Paul: Only one renderer at the moment!
    render_pipeline configured_pipeline = config.get_base_render_pipeline();

    switch (configured_pipeline)
    {
    case deferred_pbr:
        m_renderer = mango::make_unique<deferred_pbr_renderer>(config, shared_from_this());
        break;
    default:
        MANGO_LOG_ERROR("Render pipeline is unknown and the renderer cannot be created!");
        break;
    }

    return m_renderer.get();
}

void context_impl::destroy_renderer(renderer_handle renderer_in)
{
    // TODO Paul: Only one renderer at the moment!
    MANGO_ASSERT(m_renderer.get() == renderer_in, "Only one renderer is allowed at the moment!");
    m_renderer.release();
    m_renderer = nullptr;
}

renderer_handle context_impl::get_renderer()
{
    return m_renderer.get();
}

bool context_impl::create()
{
    NAMED_PROFILE_ZONE("Context Creation");

    return startup();
}

void context_impl::poll_events()
{
    if (m_display)
        m_display->poll_events();
}

bool context_impl::should_shutdown()
{
    if (!m_display)
        return false;

    return m_display->should_close();
}

void context_impl::update(float dt)
{
    m_resources->update(dt);
    if (m_ui)
    {
        m_ui->update(dt);
        auto size = m_ui->get_content_size();
        m_renderer->set_viewport(0, 0, size.x, size.y);
    }

    if (m_current_scene)
        m_current_scene->update(dt);
    if (m_renderer)
        m_renderer->update(dt);
}

void context_impl::render(float dt)
{
    if (!m_renderer)
    {
        MANGO_LOG_DEBUG("No active renderer.");
        return;
    }

    m_renderer->render(m_current_scene.get(), dt);

    if (m_ui)
        m_ui->draw_ui();

    m_renderer->present(); // Calls the present function of the graphics device -> swaps buffers.
    MARK_FRAME;
}

void context_impl::destroy()
{
    NAMED_PROFILE_ZONE("Context Destruction");
    shutdown();
}

bool context_impl::startup()
{
    NAMED_PROFILE_ZONE("Startup");

    m_input = mango::make_unique<input_impl>();
    if (!m_input)
        return false;

    m_event_handler = std::make_shared<mango_display_event_handler>(m_input.get());

    m_resources = mango::make_unique<resources_impl>();
    if (!m_resources)
        return false;

    return true;
}

void context_impl::shutdown()
{
    NAMED_PROFILE_ZONE("Shutdown");

    MANGO_ASSERT(m_resources, "Resources are invalid!");
    MANGO_ASSERT(m_event_handler, "Display Event Handler is invalid!");
    MANGO_ASSERT(m_input, "Input is invalid!");

    if (m_renderer) // Only one renderer at the moment.
        destroy_renderer(m_renderer.get());

    if (m_ui) // Only one ui at the moment.
        destroy_ui(m_ui.get());

    if (m_current_scene) // Only one scene at the moment.
        destroy_scene(m_current_scene.get());

    if (m_display) // Only one display at the moment.
        destroy_display(m_display.get());
}
