//! \file      deferred_pbr_render_system.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <core/window_system_impl.hpp>
#include <glad/glad.h>
#include <graphics/buffer.hpp>
#include <graphics/shader.hpp>
#include <graphics/shader_program.hpp>
#include <graphics/texture.hpp>
#include <graphics/vertex_array.hpp>
#include <imgui.h>
#include <mango/profile.hpp>
#include <mango/scene.hpp>
#include <rendering/pipelines/deferred_pbr_render_system.hpp>
#include <rendering/steps/ibl_step.hpp>
#include <rendering/steps/shadow_map_step.hpp>
#include <util/helpers.hpp>

using namespace mango;

#ifdef MANGO_DEBUG

static void GLAPIENTRY debugCallback(g_enum source, g_enum type, g_uint id, g_enum severity, g_sizei length, const g_char* message, const void* userParam);
#endif // MANGO_DEBUG

//! \brief Default vertex array object for second pass with geometry shader generated geometry.
vertex_array_ptr default_vao;
//! \brief Default texture that is bound to every texture unit not in use to prevent warnings.
texture_ptr default_texture;
//! \brief Default texture array that is bound to every texture unit not in use to prevent warnings.
texture_ptr default_texture_array;

deferred_pbr_render_system::deferred_pbr_render_system(const shared_ptr<context_impl>& context)
    : render_system_impl(context)
{
}

deferred_pbr_render_system::~deferred_pbr_render_system() {}

bool deferred_pbr_render_system::create()
{
    PROFILE_ZONE;
    m_shared_context->make_current();
    GLADloadproc proc = static_cast<GLADloadproc>(m_shared_context->get_gl_loading_procedure());
    if (!gladLoadGLLoader(proc))
    {
        MANGO_LOG_ERROR("Initilization of glad failed! No opengl context is available!");
        return false;
    }
    m_hardware_stats.api_version = "OpenGL ";
    m_hardware_stats.api_version.append(string((const char*)glGetString(GL_VERSION)));
    MANGO_LOG_INFO("Using: {0}", m_hardware_stats.api_version);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS); // TODO Paul: Better place?
    GL_PROFILED_CONTEXT;

#ifdef MANGO_DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(debugCallback, 0);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
    // glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_OTHER, 0, GL_DEBUG_SEVERITY_NOTIFICATION, -1, "Test Message GLDebug!");
    MANGO_LOG_DEBUG("Debug Output Enabled");
#endif // MANGO_DEBUG

    shared_ptr<window_system_impl> ws = m_shared_context->get_window_system_internal().lock();
    MANGO_ASSERT(ws, "Window System is expired!");
    int32 w = ws->get_width();
    int32 h = ws->get_height();

    m_render_queue = command_buffer::create();

    m_hardware_stats.last_frame.canvas_x      = 0;
    m_hardware_stats.last_frame.canvas_y      = 0;
    m_hardware_stats.last_frame.canvas_width  = w;
    m_hardware_stats.last_frame.canvas_height = h;

    texture_configuration attachment_config;
    attachment_config.m_generate_mipmaps        = 1;
    attachment_config.m_is_standard_color_space = false;
    attachment_config.m_texture_min_filter      = texture_parameter::filter_nearest;
    attachment_config.m_texture_mag_filter      = texture_parameter::filter_nearest;
    attachment_config.m_texture_wrap_s          = texture_parameter::wrap_clamp_to_edge;
    attachment_config.m_texture_wrap_t          = texture_parameter::wrap_clamp_to_edge;

    framebuffer_configuration gbuffer_config;
    gbuffer_config.m_color_attachment0 = texture::create(attachment_config);
    gbuffer_config.m_color_attachment0->set_data(format::rgba8, w, h, format::rgba, format::t_unsigned_int_8_8_8_8, nullptr);
    gbuffer_config.m_color_attachment1 = texture::create(attachment_config);
    gbuffer_config.m_color_attachment1->set_data(format::rgb10_a2, w, h, format::rgba, format::t_unsigned_int_10_10_10_2, nullptr);
    gbuffer_config.m_color_attachment2 = texture::create(attachment_config);
    gbuffer_config.m_color_attachment2->set_data(format::rgba8, w, h, format::rgba, format::t_unsigned_int_8_8_8_8, nullptr);
    gbuffer_config.m_color_attachment3 = texture::create(attachment_config);
    gbuffer_config.m_color_attachment3->set_data(format::rgba8, w, h, format::rgba, format::t_unsigned_int_8_8_8_8, nullptr);
    gbuffer_config.m_depth_attachment = texture::create(attachment_config);
    gbuffer_config.m_depth_attachment->set_data(format::depth_component32f, w, h, format::depth_component, format::t_float, nullptr);

    gbuffer_config.m_width  = w;
    gbuffer_config.m_height = h;

    m_gbuffer = framebuffer::create(gbuffer_config);

    if (!check_creation(m_gbuffer.get(), "gbuffer", "Render System"))
        return false;

    // HDR for auto exposure
    framebuffer_configuration hdr_buffer_config;
    attachment_config.m_generate_mipmaps  = calculate_mip_count(w, h);
    hdr_buffer_config.m_color_attachment0 = texture::create(attachment_config);
    hdr_buffer_config.m_color_attachment0->set_data(format::rgba32f, w, h, format::rgba, format::t_float, nullptr);
    attachment_config.m_generate_mipmaps = 1;
    hdr_buffer_config.m_depth_attachment = texture::create(attachment_config);
    hdr_buffer_config.m_depth_attachment->set_data(format::depth_component32f, w, h, format::depth_component, format::t_float, nullptr);

    hdr_buffer_config.m_width  = w;
    hdr_buffer_config.m_height = h;

    m_hdr_buffer = framebuffer::create(hdr_buffer_config);

    if (!check_creation(m_hdr_buffer.get(), "hdr buffer", "Render System"))
        return false;

    // backbuffer

    framebuffer_configuration backbuffer_config;
    backbuffer_config.m_color_attachment0 = texture::create(attachment_config);
    backbuffer_config.m_color_attachment0->set_data(format::rgb8, w, h, format::rgb, format::t_unsigned_int, nullptr);
    backbuffer_config.m_depth_attachment = texture::create(attachment_config);
    backbuffer_config.m_depth_attachment->set_data(format::depth_component32f, w, h, format::depth_component, format::t_float, nullptr);

    backbuffer_config.m_width  = w;
    backbuffer_config.m_height = h;

    m_backbuffer = framebuffer::create(backbuffer_config);

    if (!check_creation(m_backbuffer.get(), "backbuffer", "Render System"))
        return false;

    // frame uniform buffer
    m_frame_uniform_buffer = uniform_buffer::create();
    if (!check_creation(m_frame_uniform_buffer.get(), "framw uniform buffer", "Render System"))
        return false;

    if (!m_frame_uniform_buffer->init(524288, buffer_technique::triple_buffering)) // Triple Buffering with 0.5 MiB per Frame.
        return false;

    // scene geometry pass
    shader_configuration shader_config;
    shader_config.m_path = "res/shader/v_scene_gltf.glsl";
    shader_config.m_type = shader_type::vertex_shader;
    shader_ptr d_vertex  = shader::create(shader_config);
    if (!check_creation(d_vertex.get(), "geometry pass vertex shader", "Render System"))
        return false;

    shader_config.m_path  = "res/shader/f_scene_gltf.glsl";
    shader_config.m_type  = shader_type::fragment_shader;
    shader_ptr d_fragment = shader::create(shader_config);
    if (!check_creation(d_fragment.get(), "geometry pass fragment shader", "Render System"))
        return false;

    m_scene_geometry_pass = shader_program::create_graphics_pipeline(d_vertex, nullptr, nullptr, nullptr, d_fragment);
    if (!check_creation(m_scene_geometry_pass.get(), "geometry pass shader program", "Render System"))
        return false;

    // lighting pass
    shader_config.m_path = "res/shader/v_screen_space_triangle.glsl";
    shader_config.m_type = shader_type::vertex_shader;
    d_vertex             = shader::create(shader_config);
    if (!check_creation(d_vertex.get(), "screen space triangle vertex shader", "Render System"))
        return false;

    shader_config.m_path = "res/shader/f_deferred_lighting.glsl";
    shader_config.m_type = shader_type::fragment_shader;
    d_fragment           = shader::create(shader_config);
    if (!check_creation(d_fragment.get(), "lighting pass fragment shader", "Render System"))
        return false;

    m_lighting_pass = shader_program::create_graphics_pipeline(d_vertex, nullptr, nullptr, nullptr, d_fragment);
    if (!check_creation(m_lighting_pass.get(), "lighting pass shader program", "Render System"))
        return false;

    // composing pass
    shader_config.m_path = "res/shader/f_composing.glsl";
    shader_config.m_type = shader_type::fragment_shader;
    d_fragment           = shader::create(shader_config);
    if (!check_creation(d_fragment.get(), "composing pass fragment shader", "Render System"))
        return false;

    m_composing_pass = shader_program::create_graphics_pipeline(d_vertex, nullptr, nullptr, nullptr, d_fragment);
    if (!check_creation(m_composing_pass.get(), "composing pass shader program", "Render System"))
        return false;

    // luminance compute for auto exposure
    shader_config.m_path                  = "res/shader/c_construct_luminance_buffer.glsl";
    shader_config.m_type                  = shader_type::compute_shader;
    shader_ptr construct_luminance_buffer = shader::create(shader_config);
    if (!check_creation(construct_luminance_buffer.get(), "luminance construction compute shader", "Render System"))
        return false;

    m_construct_luminance_buffer = shader_program::create_compute_pipeline(construct_luminance_buffer);
    if (!check_creation(m_construct_luminance_buffer.get(), "luminance construction compute shader program", "Render System"))
        return false;

    shader_config.m_path               = "res/shader/c_luminance_buffer_reduction.glsl";
    shader_config.m_type               = shader_type::compute_shader;
    shader_ptr reduce_luminance_buffer = shader::create(shader_config);
    if (!check_creation(reduce_luminance_buffer.get(), "luminance reduction compute shader", "Render System"))
        return false;

    m_reduce_luminance_buffer = shader_program::create_compute_pipeline(reduce_luminance_buffer);
    if (!check_creation(m_reduce_luminance_buffer.get(), "luminance reduction compute shader program", "Render System"))
        return false;

    buffer_configuration b_config;
    b_config.m_access            = buffer_access::mapped_access_read_write;
    b_config.m_size              = 256 * sizeof(uint32) + sizeof(float);
    b_config.m_target            = buffer_target::shader_storage_buffer;
    m_luminance_histogram_buffer = buffer::create(b_config);

    m_luminance_data_mapping = static_cast<luminance_data*>(m_luminance_histogram_buffer->map(0, b_config.m_size, buffer_access::mapped_access_write));
    if (!check_mapping(m_luminance_data_mapping, "luminance data", "Render System"))
        return false;

    memset(&m_luminance_data_mapping->histogram[0], 0, 256 * sizeof(uint32));
    m_luminance_data_mapping->luminance = 1.0f;

    // default vao needed
    default_vao = vertex_array::create();
    if (!check_creation(default_vao.get(), "default vertex array object", "Render System"))
        return false;
    // default texture needed (config is not relevant)
    default_texture = texture::create(attachment_config);
    if (!check_creation(default_texture.get(), "default texture", "Render System"))
        return false;
    g_ubyte albedo[3] = { 127, 127, 127 };
    default_texture->set_data(format::rgb8, 1, 1, format::rgb, format::t_unsigned_byte, albedo);
    attachment_config.m_layers = 3;
    default_texture_array      = texture::create(attachment_config);
    if (!check_creation(default_texture_array.get(), "default texture array", "Render System"))
        return false;
    default_texture_array->set_data(format::rgb8, 1, 1, format::rgb, format::t_unsigned_byte, albedo);

    for (int32 i = 0; i < 9; ++i)
        m_lighting_pass_data.debug_views.debug[i] = false;
    m_lighting_pass_data.debug_view_enabled             = false;
    m_lighting_pass_data.debug_options.show_cascades    = false;
    m_lighting_pass_data.debug_options.draw_shadow_maps = false;

    return true;
}

void deferred_pbr_render_system::configure(const render_configuration& configuration)
{
    PROFILE_ZONE;
    auto ws = m_shared_context->get_window_system_internal().lock();
    MANGO_ASSERT(ws, "Window System is expired!");
    ws->set_vsync(configuration.is_vsync_enabled());

    // additional render steps
    if (configuration.get_render_steps()[mango::render_step::ibl])
    {
        // create an extra object that is capable to create cubemaps from equirectangular hdr, preprocess everything and
        // do all the rendering for the environment.
        auto step_ibl = std::make_shared<ibl_step>();
        step_ibl->create();
        m_pipeline_steps[mango::render_step::ibl] = std::static_pointer_cast<pipeline_step>(step_ibl);
    }
    if (configuration.get_render_steps()[mango::render_step::shadow_map])
    {
        auto step_shadow_map = std::make_shared<shadow_map_step>();
        step_shadow_map->create();
        m_pipeline_steps[mango::render_step::shadow_map] = std::static_pointer_cast<pipeline_step>(step_shadow_map);
    }
}

void deferred_pbr_render_system::setup_ibl_step(const ibl_step_configuration& configuration)
{
    if (m_pipeline_steps[mango::render_step::ibl])
    {
        auto step = std::static_pointer_cast<ibl_step>(m_pipeline_steps[mango::render_step::ibl]);
        step->configure(configuration);
    }
}

void deferred_pbr_render_system::setup_shadow_map_step(const shadow_step_configuration& configuration)
{
    if (m_pipeline_steps[mango::render_step::shadow_map])
    {
        auto step = std::static_pointer_cast<shadow_map_step>(m_pipeline_steps[mango::render_step::shadow_map]);
        step->configure(configuration);
    }
}

void deferred_pbr_render_system::begin_render()
{
    PROFILE_ZONE;
    m_hardware_stats.last_frame.draw_calls = 0;
    m_hardware_stats.last_frame.meshes     = 0;
    m_hardware_stats.last_frame.primitives = 0;
    m_hardware_stats.last_frame.materials  = 0;

    // TODO Paul: This should not be done here, this is pretty bad!
    m_command_buffer->clear_framebuffer(clear_buffer_mask::color_and_depth_stencil, attachment_mask::all, 0.1f, 0.1f, 0.1f, 1.0f);
    m_command_buffer->clear_framebuffer(clear_buffer_mask::color_and_depth, attachment_mask::all_draw_buffers_and_depth, 0.0f, 0.0f, 0.0f, 1.0f, m_gbuffer);
    m_command_buffer->clear_framebuffer(clear_buffer_mask::color_and_depth, attachment_mask::all_draw_buffers_and_depth, 0.0f, 0.0f, 0.0f, 1.0f, m_hdr_buffer);
    m_command_buffer->clear_framebuffer(clear_buffer_mask::color_and_depth, attachment_mask::all_draw_buffers_and_depth, 0.0f, 0.0f, 0.0f, 1.0f, m_backbuffer);
    if (m_pipeline_steps[mango::render_step::shadow_map])
    {
        auto step = std::static_pointer_cast<shadow_map_step>(m_pipeline_steps[mango::render_step::shadow_map]);
        step->clear_shadow_buffer(m_command_buffer);
    }

    // TODO Paul: Is there a better way?
    m_frame_uniform_buffer->begin_frame(m_command_buffer);

    {
        GL_NAMED_PROFILE_ZONE("Deferred Renderer Begin");
        m_command_buffer->execute();
    }
}

void deferred_pbr_render_system::finish_render(float dt)
{
    PROFILE_ZONE;
    auto scene  = m_shared_context->get_current_scene();
    auto camera = scene->get_active_camera_data();
    auto env    = scene->get_active_environment_data();

    if (m_pipeline_steps[mango::render_step::shadow_map])
    {
        if (m_lighting_pass_data.directional.intensity > 1e-5f && !m_lighting_pass_data.debug_view_enabled && camera.camera_info && m_lighting_pass_data.directional.cast_shadows)
        {
            std::static_pointer_cast<shadow_map_step>(m_pipeline_steps[mango::render_step::shadow_map])
                ->update_cascades(dt, camera.camera_info->z_near, camera.camera_info->z_far, camera.camera_info->view_projection, m_lighting_pass_data.directional.direction);

            m_pipeline_steps[mango::render_step::shadow_map]->execute(m_command_buffer, m_frame_uniform_buffer);
        }
        else
            std::static_pointer_cast<shadow_map_step>(m_pipeline_steps[mango::render_step::shadow_map])->get_caster_queue()->clear();
    }

    // reset viewport to be sure.
    auto x = m_hardware_stats.last_frame.canvas_x;
    auto y = m_hardware_stats.last_frame.canvas_y;
    auto w = m_hardware_stats.last_frame.canvas_width;
    auto h = m_hardware_stats.last_frame.canvas_height;
    m_command_buffer->set_viewport(x, y, w, h);

    // bind the renderer uniform buffer.
    bind_renderer_data_buffer(camera);

    // geometry pass
    {
        m_command_buffer->set_depth_test(true);
        m_command_buffer->set_depth_func(compare_operation::less);
        m_command_buffer->set_face_culling(true);
        m_command_buffer->set_cull_face(polygon_face::face_back);
        m_command_buffer->bind_framebuffer(m_gbuffer);
        m_command_buffer->bind_shader_program(m_scene_geometry_pass);

        if (m_wireframe)
            m_command_buffer->set_polygon_mode(polygon_face::face_front_and_back, polygon_mode::line);

        m_command_buffer->attach(m_render_queue);
    }

    // Bind lighting pass uniform buffer.
    bind_lighting_pass_buffer(camera, env);

    // lighting pass
    {
        m_command_buffer->bind_framebuffer(m_hdr_buffer); // bind hdr.
        m_command_buffer->bind_shader_program(m_lighting_pass);
        m_command_buffer->set_polygon_mode(polygon_face::face_front_and_back, polygon_mode::fill);

        m_command_buffer->bind_texture(0, m_gbuffer->get_attachment(framebuffer_attachment::color_attachment0), 0);
        m_command_buffer->bind_texture(1, m_gbuffer->get_attachment(framebuffer_attachment::color_attachment1), 1);
        m_command_buffer->bind_texture(2, m_gbuffer->get_attachment(framebuffer_attachment::color_attachment2), 2);
        m_command_buffer->bind_texture(3, m_gbuffer->get_attachment(framebuffer_attachment::color_attachment3), 3);
        m_command_buffer->bind_texture(4, m_gbuffer->get_attachment(framebuffer_attachment::depth_attachment), 4);
        if (m_pipeline_steps[mango::render_step::shadow_map])
        {
            std::static_pointer_cast<shadow_map_step>(m_pipeline_steps[mango::render_step::shadow_map])->bind_shadow_data(m_command_buffer, m_frame_uniform_buffer);
        }
        else
            m_command_buffer->bind_texture(8, default_texture_array, 8);
        if (m_pipeline_steps[mango::render_step::ibl])
        {
            std::static_pointer_cast<ibl_step>(m_pipeline_steps[mango::render_step::ibl])->bind_image_based_light_maps(m_command_buffer);
        }
        else
        {
            m_command_buffer->bind_texture(5, default_texture, 5);
            m_command_buffer->bind_texture(6, default_texture, 6);
            m_command_buffer->bind_texture(7, default_texture, 7);
        }

        // TODO Paul: Check if the binding is better for performance or not.
        m_command_buffer->bind_vertex_array(default_vao);

        m_command_buffer->draw_arrays(primitive_topology::triangles, 0, 3);
        m_hardware_stats.last_frame.draw_calls++; // TODO Paul: This measurements should be done, on glCalls.
    }

    // environment drawing
    if (m_pipeline_steps[mango::render_step::ibl] && !m_lighting_pass_data.debug_view_enabled)
    {
        shared_ptr<ibl_step> imgbl = std::static_pointer_cast<ibl_step>(m_pipeline_steps[mango::render_step::ibl]);
        m_command_buffer->set_depth_func(compare_operation::less_equal);
        m_command_buffer->set_cull_face(polygon_face::face_front);
        m_pipeline_steps[mango::render_step::ibl]->execute(m_command_buffer, m_frame_uniform_buffer);
    }

    // auto exposure compute shaders
    if (camera.camera_info && camera.camera_info->physical.adaptive_exposure && !m_lighting_pass_data.debug_view_enabled)
    {
        m_command_buffer->bind_shader_program(m_construct_luminance_buffer);
        auto tex = m_hdr_buffer->get_attachment(framebuffer_attachment::color_attachment0);
        m_command_buffer->calculate_mipmaps(tex);
        m_command_buffer->add_memory_barrier(memory_barrier_bit::shader_image_access_barrier_bit);
        int32 mip_level  = 0;
        int32 tex_width  = tex->get_width();
        int32 tex_height = tex->get_height();
        while (tex_width >> mip_level > 512 && tex_height >> mip_level > 512) // we can make it smaller, when we have some better focussing.
        {
            ++mip_level;
        }
        tex_width >>= mip_level;
        tex_height >>= mip_level;
        m_command_buffer->bind_image_texture(0, tex, mip_level, false, 0, base_access::read_only, format::rgba32f);
        m_command_buffer->bind_buffer(1, m_luminance_histogram_buffer, buffer_target::shader_storage_buffer);
        glm::vec2 params = glm::vec2(-8.0f, 1.0f / 40.0f); // min -8.0, max +32.0
        m_command_buffer->bind_single_uniform(1, &(params), sizeof(params));

        m_command_buffer->dispatch_compute(tex_width / 16, tex_height / 16, 1);

        m_command_buffer->add_memory_barrier(memory_barrier_bit::shader_storage_barrier_bit);

        m_command_buffer->bind_shader_program(m_reduce_luminance_buffer);
        m_command_buffer->bind_buffer(0, m_luminance_histogram_buffer, buffer_target::shader_storage_buffer);

        // time coefficient with tau = 1.1;
        float tau              = 0.75f;
        float time_coefficient = 1.0f - expf(-dt * tau);
        glm::vec4 red_params   = glm::vec4(time_coefficient, tex_width * tex_height, -8.0f, 40.0f); // min -8.0, max +32.0
        m_command_buffer->bind_single_uniform(0, &(red_params), sizeof(red_params));

        m_command_buffer->dispatch_compute(1, 1, 1);

        m_command_buffer->add_memory_barrier(memory_barrier_bit::shader_storage_barrier_bit);

        apply_auto_exposure(camera); // for next frame.
    }

    // composite
    {
        m_command_buffer->set_depth_func(compare_operation::less);
        m_command_buffer->set_cull_face(polygon_face::face_back);
        m_command_buffer->bind_framebuffer(m_backbuffer); // bind backbuffer.
        m_command_buffer->bind_shader_program(m_composing_pass);
        float camera_exposure = 1.0f;
        if (camera.camera_info)
        {
            camera.camera_info->physical.aperture      = glm::clamp(camera.camera_info->physical.aperture, min_aperture, max_aperture);
            camera.camera_info->physical.shutter_speed = glm::clamp(camera.camera_info->physical.shutter_speed, min_shutter_speed, max_shutter_speed);
            camera.camera_info->physical.iso           = glm::clamp(camera.camera_info->physical.iso, min_iso, max_iso);
            float ape                                  = camera.camera_info->physical.aperture;
            float shu                                  = camera.camera_info->physical.shutter_speed;
            float iso                                  = camera.camera_info->physical.iso;
            float e                                    = ((ape * ape) * 100.0f) / (shu * iso);
            camera_exposure                            = 1.0f / (1.2f * e);
        }
        m_command_buffer->bind_single_uniform(1, &camera_exposure, sizeof(camera_exposure));
        int32 b_val = m_lighting_pass_data.debug_view_enabled ? 1 : (m_lighting_pass_data.debug_options.draw_shadow_maps ? 2 : 0);
        m_command_buffer->bind_single_uniform(2, &(b_val), sizeof(b_val));

        m_command_buffer->bind_texture(0, m_hdr_buffer->get_attachment(framebuffer_attachment::color_attachment0), 0);

        // TODO Paul: Check if the binding is better for performance or not.
        m_command_buffer->bind_vertex_array(default_vao);

        m_command_buffer->draw_arrays(primitive_topology::triangles, 0, 3);
        m_hardware_stats.last_frame.draw_calls++; // TODO Paul: This measurements should be done, on glCalls.
    }

    m_command_buffer->bind_framebuffer(nullptr);
    m_command_buffer->bind_vertex_array(nullptr);
    // We need to unbind the program so we can make changes to the textures.
    m_command_buffer->bind_shader_program(nullptr);

    m_frame_uniform_buffer->end_frame(m_command_buffer);

    {
        GL_NAMED_PROFILE_ZONE("Deferred Renderer Finish");
        m_command_buffer->execute();
    }
}

void deferred_pbr_render_system::set_viewport(int32 x, int32 y, int32 width, int32 height)
{
    PROFILE_ZONE;
    MANGO_ASSERT(x >= 0, "Viewport x position has to be positive!");
    MANGO_ASSERT(y >= 0, "Viewport y position has to be positive!");
    MANGO_ASSERT(width >= 0, "Viewport width has to be positive!");
    MANGO_ASSERT(height >= 0, "Viewport height has to be positive!");
    m_command_buffer->set_viewport(x, y, width, height);
    m_gbuffer->resize(width, height);
    m_backbuffer->resize(width, height);
    m_hdr_buffer->resize(width, height);

    m_hardware_stats.last_frame.canvas_x      = x;
    m_hardware_stats.last_frame.canvas_y      = y;
    m_hardware_stats.last_frame.canvas_width  = width;
    m_hardware_stats.last_frame.canvas_height = height;
}

void deferred_pbr_render_system::update(float dt)
{
    MANGO_UNUSED(dt);
}

void deferred_pbr_render_system::destroy() {}

render_pipeline deferred_pbr_render_system::get_base_render_pipeline()
{
    return render_pipeline::deferred_pbr;
}

void deferred_pbr_render_system::begin_mesh(const glm::mat4& model_matrix, bool has_normals, bool has_tangents)
{
    PROFILE_ZONE;

    model_data d{ model_matrix, glm::transpose(glm::inverse(model_matrix)), has_normals, has_tangents, 0, 0 };

    bind_uniform_buffer_cmd cmd = m_frame_uniform_buffer->bind_uniform_buffer(UB_SLOT_MODEL_DATA, sizeof(model_data), &d);

    m_render_queue->submit<bind_uniform_buffer_cmd>(cmd);
    if (m_pipeline_steps[mango::render_step::shadow_map])
    {
        std::static_pointer_cast<shadow_map_step>(m_pipeline_steps[mango::render_step::shadow_map])->get_caster_queue()->submit<bind_uniform_buffer_cmd>(cmd);
    }

    m_hardware_stats.last_frame.meshes++;
}

void deferred_pbr_render_system::use_material(const material_ptr& mat)
{
    PROFILE_ZONE;

    command_buffer_ptr caster_queue;
    if (m_pipeline_steps[mango::render_step::shadow_map])
    {
        caster_queue = std::static_pointer_cast<shadow_map_step>(m_pipeline_steps[mango::render_step::shadow_map])->get_caster_queue();
    }

    material_data d;

    d.base_color     = static_cast<glm::vec4>(mat->base_color);
    d.emissive_color = static_cast<glm::vec3>(mat->emissive_color);
    d.metallic       = (g_float)mat->metallic;
    d.roughness      = (g_float)mat->roughness;
    if (mat->use_base_color_texture)
    {
        d.base_color_texture = true;
        m_render_queue->bind_texture(0, mat->base_color_texture, 0);
        if (caster_queue)
            caster_queue->bind_texture(0, mat->base_color_texture, 0);
    }
    else
    {
        d.base_color_texture = false;
        m_render_queue->bind_texture(0, default_texture, 0);
        if (caster_queue)
            caster_queue->bind_texture(0, default_texture, 0);
    }
    if (mat->use_roughness_metallic_texture)
    {
        d.roughness_metallic_texture = true;
        m_render_queue->bind_texture(1, mat->roughness_metallic_texture, 1);
    }
    else
    {
        d.roughness_metallic_texture = false;
        m_render_queue->bind_texture(1, default_texture, 2);
    }
    if (mat->use_occlusion_texture)
    {
        d.occlusion_texture = true;
        m_render_queue->bind_texture(2, mat->occlusion_texture, 2);
        d.packed_occlusion = false;
    }
    else
    {
        d.occlusion_texture = false;
        m_render_queue->bind_texture(2, default_texture, 2);
        // eventually it is packed
        d.packed_occlusion = mat->packed_occlusion && mat->use_packed_occlusion;
    }
    if (mat->use_normal_texture)
    {
        d.normal_texture = true;
        m_render_queue->bind_texture(3, mat->normal_texture, 3);
    }
    else
    {
        d.normal_texture = false;
        m_render_queue->bind_texture(3, default_texture, 3);
    }
    if (mat->use_emissive_color_texture)
    {
        d.emissive_color_texture = true;
        m_render_queue->bind_texture(4, mat->emissive_color_texture, 4);
    }
    else
    {
        d.emissive_color_texture = false;
        m_render_queue->bind_texture(4, default_texture, 4);
    }

    d.alpha_mode   = static_cast<g_int>(mat->alpha_rendering);
    d.alpha_cutoff = static_cast<g_float>(mat->alpha_cutoff);

    if (mat->double_sided)
    {
        m_render_queue->set_face_culling(false);
        if (caster_queue)
            caster_queue->set_face_culling(false);
    }

    bind_uniform_buffer_cmd cmd = m_frame_uniform_buffer->bind_uniform_buffer(UB_SLOT_MATERIAL_DATA, sizeof(material_data), &d);

    m_render_queue->submit<bind_uniform_buffer_cmd>(cmd);
    if (m_pipeline_steps[mango::render_step::shadow_map])
    {
        std::static_pointer_cast<shadow_map_step>(m_pipeline_steps[mango::render_step::shadow_map])->get_caster_queue()->submit<bind_uniform_buffer_cmd>(cmd);
    }
}

void deferred_pbr_render_system::draw_mesh(const vertex_array_ptr& vertex_array, primitive_topology topology, int32 first, int32 count, index_type type, int32 instance_count)
{
    PROFILE_ZONE;

    m_render_queue->bind_vertex_array(vertex_array);

    command_buffer_ptr caster_queue;
    if (m_pipeline_steps[mango::render_step::shadow_map])
    {
        caster_queue = std::static_pointer_cast<shadow_map_step>(m_pipeline_steps[mango::render_step::shadow_map])->get_caster_queue();
        caster_queue->bind_vertex_array(vertex_array);
    }

    MANGO_ASSERT(first >= 0, "The first index has to be greater than 0!");
    MANGO_ASSERT(count >= 0, "The index count has to be greater than 0!");
    MANGO_ASSERT(instance_count >= 0, "The instance count has to be greater than 0!");

    if (type == index_type::none)
    {
        m_render_queue->draw_arrays(topology, first, count, instance_count);
        if (caster_queue)
            caster_queue->draw_arrays(topology, first, count, instance_count);
    }
    else
    {
        m_render_queue->draw_elements(topology, first, count, type, instance_count);
        if (caster_queue)
            caster_queue->draw_elements(topology, first, count, type, instance_count);
    }

    m_hardware_stats.last_frame.draw_calls++; // TODO Paul: This measurements should be done on glCalls. Wrong, shadow map render calls not counted.
    m_hardware_stats.last_frame.primitives++;
    m_hardware_stats.last_frame.materials++;

    m_render_queue->set_face_culling(true);
    m_render_queue->bind_vertex_array(nullptr);

#ifdef MANGO_DEBUG
    // This needs to be done.
    // TODO Paul: State synchronization is not perfect.
    m_render_queue->bind_texture(0, nullptr, 0);
    m_render_queue->bind_texture(1, nullptr, 1);
    m_render_queue->bind_texture(2, nullptr, 2);
    m_render_queue->bind_texture(3, nullptr, 3);
    m_render_queue->bind_texture(4, nullptr, 4);
#endif // MANGO_DEBUG

    if (caster_queue)
    {
        caster_queue->set_face_culling(true);
        caster_queue->bind_vertex_array(nullptr);
#ifdef MANGO_DEBUG
        caster_queue->bind_texture(0, nullptr, 0);
        caster_queue->bind_texture(1, nullptr, 1);
        caster_queue->bind_texture(2, nullptr, 2);
        caster_queue->bind_texture(3, nullptr, 3);
        caster_queue->bind_texture(4, nullptr, 4);
#endif // MANGO_DEBUG
    }
}

void deferred_pbr_render_system::set_environment_texture(const texture_ptr& hdr_texture)
{
    PROFILE_ZONE;
    if (m_pipeline_steps[mango::render_step::ibl])
    {
        auto ibl = std::static_pointer_cast<ibl_step>(m_pipeline_steps[mango::render_step::ibl]);
        ibl->load_from_hdr(hdr_texture);
    }
}

void deferred_pbr_render_system::submit_light(light_type type, light_data* data)
{
    PROFILE_ZONE;
    if (type == light_type::directional) // currently always true
    {
        auto directional_data                         = static_cast<directional_light_data*>(data);
        m_lighting_pass_data.directional.direction    = directional_data->direction;
        m_lighting_pass_data.directional.color        = static_cast<glm::vec3>(directional_data->light_color);
        m_lighting_pass_data.directional.intensity    = directional_data->intensity;
        m_lighting_pass_data.directional.cast_shadows = directional_data->cast_shadows;
    }
}

void deferred_pbr_render_system::bind_lighting_pass_buffer(camera_data& camera, environment_data& environment)
{
    PROFILE_ZONE;

    if (camera.camera_info && camera.transform)
    {
        m_lighting_pass_data.inverse_view_projection = glm::inverse(camera.camera_info->view_projection);
        m_lighting_pass_data.view                    = camera.camera_info->view;
        m_lighting_pass_data.camera_position         = static_cast<glm::vec3>(camera.transform->world_transformation_matrix[3]);
        m_lighting_pass_data.camera_params           = glm::vec4(camera.camera_info->z_near, camera.camera_info->z_far, 0.0f, 0.0f);
    }
    else
    {
        MANGO_LOG_ERROR("Lighting pass uniforms can not be set! No active camera!");
    }
    m_lighting_pass_data.ambient.intensity = mango::default_environment_intensity;
    if (environment.environment_info)
        m_lighting_pass_data.ambient.intensity = environment.environment_info->intensity;

    m_lighting_pass_data.directional.cast_shadows = m_lighting_pass_data.directional.cast_shadows && (m_pipeline_steps[mango::render_step::shadow_map] != nullptr);

    bind_uniform_buffer_cmd cmd = m_frame_uniform_buffer->bind_uniform_buffer(UB_SLOT_LIGHTING_PASS_DATA, sizeof(lighting_pass_data), &m_lighting_pass_data);

    m_command_buffer->submit<bind_uniform_buffer_cmd>(cmd);
}

void deferred_pbr_render_system::bind_renderer_data_buffer(camera_data& camera)
{
    PROFILE_ZONE;

    if (camera.camera_info && camera.transform)
    {
        m_renderer_data.view_matrix            = camera.camera_info->view;
        m_renderer_data.projection_matrix      = camera.camera_info->projection;
        m_renderer_data.view_projection_matrix = camera.camera_info->view_projection;
    }
    else
    {
        MANGO_LOG_ERROR("Renderer Data not complete! No active camera! Attempting to use last valid data!");
    }

    bind_uniform_buffer_cmd cmd = m_frame_uniform_buffer->bind_uniform_buffer(UB_SLOT_RENDERER_FRAME, sizeof(renderer_data), &m_renderer_data);

    m_command_buffer->submit<bind_uniform_buffer_cmd>(cmd);
}

void deferred_pbr_render_system::apply_auto_exposure(camera_data& camera)
{
    PROFILE_ZONE;
    float avg_luminance = m_luminance_data_mapping->luminance;

    // Start with the default assumption.
    float ape = mango::default_aperture;

    // Start with the default assumption.
    float shu = mango::default_shutter_speed;

    // K is a light meter calibration constant
    static const float K = 12.5f;
    static const float S = 100.0f;
    float target_ev      = glm::log2(avg_luminance * S / K);

    // Compute the resulting ISO if we left both shutter and aperture here
    float iso           = glm::clamp(((ape * ape) * 100.0f) / (shu * glm::exp2(target_ev)), mango::min_iso, mango::max_iso);
    float unclamped_iso = (shu * glm::exp2(target_ev));
    MANGO_UNUSED(unclamped_iso);

    // Apply half the difference in EV to the aperture
    float ev_diff = target_ev - glm::log2(((ape * ape) * 100.0f) / (shu * iso));
    ape           = glm::clamp(ape * glm::pow(glm::sqrt(2.0f), ev_diff * 0.5f), mango::min_aperture, mango::max_aperture);

    // Apply the remaining difference to the shutter speed
    ev_diff = target_ev - glm::log2(((ape * ape) * 100.0f) / (shu * iso));
    shu     = glm::clamp(shu * glm::pow(2.0f, -ev_diff), mango::min_shutter_speed, mango::max_shutter_speed);

    // Adapt camera settings.
    camera.camera_info->physical.aperture      = ape;
    camera.camera_info->physical.shutter_speed = shu;
    camera.camera_info->physical.iso           = iso;
}

void deferred_pbr_render_system::on_ui_widget()
{
    const char* debug          = { " Default \0 Position \0 Normal \0 Depth \0 Base Color \0 Reflection Color \0 Emission \0 Occlusion \0 Roughness \0 Metallic " };
    static int32 current_debug = 0;
    bool change_flag           = false;
    ImGui::Text("Deferred PBR Render System");
    static bool has_ibl        = m_pipeline_steps[mango::render_step::ibl] != nullptr;
    static bool has_shadow_map = m_pipeline_steps[mango::render_step::shadow_map] != nullptr;
    if (ImGui::CollapsingHeader("Steps##deferred_pbr"))
    {
        change_flag = has_ibl;
        ImGui::Checkbox("IBL##deferred_pbr", &has_ibl);
        if (has_ibl != change_flag)
        {
            if (has_ibl)
            {
                auto step_ibl = std::make_shared<ibl_step>();
                step_ibl->create();
                m_pipeline_steps[mango::render_step::ibl] = std::static_pointer_cast<pipeline_step>(step_ibl);

                auto scene = m_shared_context->get_current_scene();
                auto env   = scene->get_active_environment_data();
                if (env.environment_info)
                    step_ibl->load_from_hdr(env.environment_info->hdr_texture);
            }
            else
            {
                m_pipeline_steps[mango::render_step::ibl] = nullptr;
            }
        }
        if (has_ibl && ImGui::TreeNode("IBL Step##deferred_pbr"))
        {
            m_pipeline_steps[mango::render_step::ibl]->on_ui_widget();
            ImGui::TreePop();
        }
        ImGui::Separator();
        change_flag = has_shadow_map;
        ImGui::Checkbox("Directional Shadows##deferred_pbr", &has_shadow_map);
        if (has_shadow_map != change_flag)
        {
            if (has_shadow_map)
            {
                auto step_shadow_map = std::make_shared<shadow_map_step>();
                step_shadow_map->create();
                m_pipeline_steps[mango::render_step::shadow_map] = std::static_pointer_cast<pipeline_step>(step_shadow_map);
            }
            else
            {
                m_pipeline_steps[mango::render_step::shadow_map] = nullptr;
            }
        }
        if (has_shadow_map && ImGui::TreeNode("Shadow Step##deferred_pbr"))
        {
            m_pipeline_steps[mango::render_step::shadow_map]->on_ui_widget();
            ImGui::TreePop();
        }
    }
    if (ImGui::CollapsingHeader("Debug##deferred_pbr"))
    {
        float occupancy = m_frame_uniform_buffer->get_occupancy();
        ImGui::Text(("Frame Uniform Buffer Occupancy: " + std::to_string(occupancy)).c_str());
        ImGui::Checkbox("Render Wireframe##deferred_pbr", &m_wireframe);

        for (int32 i = 0; i < 9; ++i)
            m_lighting_pass_data.debug_views.debug[i] = false;
        m_lighting_pass_data.debug_view_enabled = false;
        ImGui::Combo("Views##deferred_pbr", &current_debug, debug);
        if (current_debug)
        {
            m_lighting_pass_data.debug_views.debug[current_debug - 1] = true;
            m_lighting_pass_data.debug_view_enabled                   = true;
        }
        if (has_shadow_map)
        {
            bool casc = m_lighting_pass_data.debug_options.show_cascades;
            ImGui::Checkbox("Show Cascades##deferred_pbr", &casc);
            m_lighting_pass_data.debug_options.show_cascades = casc;

            bool sm = m_lighting_pass_data.debug_options.draw_shadow_maps;
            ImGui::Checkbox("Show Shadow Maps##deferred_pbr", &sm);
            m_lighting_pass_data.debug_options.draw_shadow_maps = sm;
        }
    }
}

#ifdef MANGO_DEBUG

static const char* getStringForType(g_enum type)
{
    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR:
        return "Error";
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        return "Deprecated behavior";
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        return "Undefined behavior";
    case GL_DEBUG_TYPE_PORTABILITY:
        return "Portability issue";
    case GL_DEBUG_TYPE_PERFORMANCE:
        return "Performance issue";
    case GL_DEBUG_TYPE_MARKER:
        return "Stream annotation";
    case GL_DEBUG_TYPE_OTHER:
        return "Other";
    default:
        assert(false);
        return "";
    }
}

static const char* getStringForSource(g_enum source)
{
    switch (source)
    {
    case GL_DEBUG_SOURCE_API:
        return "API";
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        return "Window system";
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
        return "Shader compiler";
    case GL_DEBUG_SOURCE_THIRD_PARTY:
        return "Third party";
    case GL_DEBUG_SOURCE_APPLICATION:
        return "Application";
    case GL_DEBUG_SOURCE_OTHER:
        return "Other";
    default:
        assert(false);
        return "";
    }
}

static const char* getStringForSeverity(g_enum severity)
{
    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH:
        return "High";
    case GL_DEBUG_SEVERITY_MEDIUM:
        return "Medium";
    case GL_DEBUG_SEVERITY_LOW:
        return "Low";
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        return "Notification";
    default:
        assert(false);
        return ("");
    }
}

static void GLAPIENTRY debugCallback(g_enum source, g_enum type, g_uint id, g_enum severity, g_sizei length, const g_char* message, const void* userParam)
{
    (void)id;
    (void)length;
    (void)userParam;
    std::cout << "\n--------------OpenGL Debug Output--------------" << std::endl;
    std::cout << "Source: " << getStringForSource(source) << std::endl;
    std::cout << "Type: " << getStringForType(type) << std::endl;
    std::cout << "Severity: " << getStringForSeverity(severity) << std::endl;
    std::cout << "Debug call: " << message << std::endl;
    std::cout << "-----------------------------------------------" << std::endl;
}

#endif // MANGO_DEBUG
