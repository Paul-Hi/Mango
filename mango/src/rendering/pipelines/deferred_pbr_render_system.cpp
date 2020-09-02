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
#endif // MANGO_DEBUG

    shared_ptr<window_system_impl> ws = m_shared_context->get_window_system_internal().lock();
    MANGO_ASSERT(ws, "Window System is expired!");
    int32 w = ws->get_width();
    int32 h = ws->get_height();

    m_render_queue = command_buffer::create();

    m_hardware_stats.last_frame.canvas_width  = w;
    m_hardware_stats.last_frame.canvas_height = h;

    texture_configuration attachment_config;
    attachment_config.m_generate_mipmaps        = 1;
    attachment_config.m_is_standard_color_space = false;
    attachment_config.m_texture_min_filter      = texture_parameter::FILTER_NEAREST;
    attachment_config.m_texture_mag_filter      = texture_parameter::FILTER_NEAREST;
    attachment_config.m_texture_wrap_s          = texture_parameter::WRAP_CLAMP_TO_EDGE;
    attachment_config.m_texture_wrap_t          = texture_parameter::WRAP_CLAMP_TO_EDGE;

    framebuffer_configuration gbuffer_config;
    gbuffer_config.m_color_attachment0 = texture::create(attachment_config);
    gbuffer_config.m_color_attachment0->set_data(format::RGBA8, w, h, format::RGBA, format::UNSIGNED_INT_8_8_8_8, nullptr);
    gbuffer_config.m_color_attachment1 = texture::create(attachment_config);
    gbuffer_config.m_color_attachment1->set_data(format::RGB10_A2, w, h, format::RGBA, format::UNSIGNED_INT_10_10_10_2, nullptr);
    gbuffer_config.m_color_attachment2 = texture::create(attachment_config);
    gbuffer_config.m_color_attachment2->set_data(format::RGBA8, w, h, format::RGBA, format::UNSIGNED_INT_8_8_8_8, nullptr);
    gbuffer_config.m_color_attachment3 = texture::create(attachment_config);
    gbuffer_config.m_color_attachment3->set_data(format::RGBA8, w, h, format::RGBA, format::UNSIGNED_INT_8_8_8_8, nullptr);
    gbuffer_config.m_depth_attachment = texture::create(attachment_config);
    gbuffer_config.m_depth_attachment->set_data(format::DEPTH_COMPONENT32F, w, h, format::DEPTH_COMPONENT, format::FLOAT, nullptr);

    gbuffer_config.m_width  = w;
    gbuffer_config.m_height = h;

    m_gbuffer = framebuffer::create(gbuffer_config);

    if (!check_creation(m_gbuffer.get(), "gbuffer", "Render System"))
        return false;

    // HDR for auto exposure
    framebuffer_configuration hdr_buffer_config;
    hdr_buffer_config.m_color_attachment0 = texture::create(attachment_config);
    hdr_buffer_config.m_color_attachment0->set_data(format::RGBA32F, w, h, format::RGBA, format::FLOAT, nullptr);
    hdr_buffer_config.m_depth_attachment = texture::create(attachment_config);
    hdr_buffer_config.m_depth_attachment->set_data(format::DEPTH_COMPONENT32F, w, h, format::DEPTH_COMPONENT, format::FLOAT, nullptr);

    hdr_buffer_config.m_width  = w;
    hdr_buffer_config.m_height = h;

    m_hdr_buffer = framebuffer::create(hdr_buffer_config);

    if (!check_creation(m_hdr_buffer.get(), "hdr buffer", "Render System"))
        return false;

    // backbuffer

    framebuffer_configuration backbuffer_config;
    backbuffer_config.m_color_attachment0 = texture::create(attachment_config);
    backbuffer_config.m_color_attachment0->set_data(format::RGB8, w, h, format::RGB, format::UNSIGNED_INT, nullptr);
    backbuffer_config.m_depth_attachment = texture::create(attachment_config);
    backbuffer_config.m_depth_attachment->set_data(format::DEPTH_COMPONENT32F, w, h, format::DEPTH_COMPONENT, format::FLOAT, nullptr);

    backbuffer_config.m_width  = w;
    backbuffer_config.m_height = h;

    m_backbuffer = framebuffer::create(backbuffer_config);

    if (!check_creation(m_backbuffer.get(), "backbuffer", "Render System"))
        return false;

    // frame uniform buffer
    buffer_configuration uniform_buffer_config(uniform_buffer_size, buffer_target::UNIFORM_BUFFER, buffer_access::MAPPED_ACCESS_WRITE);
    m_frame_uniform_buffer = buffer::create(uniform_buffer_config);

    if (!check_creation(m_frame_uniform_buffer.get(), "frame uniform buffer", "Render System"))
        return false;

    m_mapped_uniform_memory = m_frame_uniform_buffer->map(0, m_frame_uniform_buffer->byte_length(), buffer_access::MAPPED_ACCESS_WRITE);

    if (!check_mapping(m_frame_uniform_buffer.get(), "uniforms", "Render System"))
        return false;

    m_frame_uniform_offset = 0;

    // scene geometry pass
    shader_configuration shader_config;
    shader_config.m_path = "res/shader/v_scene_gltf.glsl";
    shader_config.m_type = shader_type::VERTEX_SHADER;
    shader_ptr d_vertex  = shader::create(shader_config);
    if (!check_creation(d_vertex.get(), "geometry pass vertex shader", "Render System"))
        return false;

    shader_config.m_path  = "res/shader/f_scene_gltf.glsl";
    shader_config.m_type  = shader_type::FRAGMENT_SHADER;
    shader_ptr d_fragment = shader::create(shader_config);
    if (!check_creation(d_fragment.get(), "geometry pass fragment shader", "Render System"))
        return false;

    m_scene_geometry_pass = shader_program::create_graphics_pipeline(d_vertex, nullptr, nullptr, nullptr, d_fragment);
    if (!check_creation(m_scene_geometry_pass.get(), "geometry pass shader program", "Render System"))
        return false;

    // lighting pass
    shader_config.m_path = "res/shader/v_empty.glsl";
    shader_config.m_type = shader_type::VERTEX_SHADER;
    d_vertex             = shader::create(shader_config);
    if (!check_creation(d_vertex.get(), "empty vertex shader", "Render System"))
        return false;

    shader_config.m_path  = "res/shader/g_create_screen_space_quad.glsl";
    shader_config.m_type  = shader_type::GEOMETRY_SHADER;
    shader_ptr d_geometry = shader::create(shader_config);
    if (!check_creation(d_geometry.get(), "geometry pass geometry shader", "Render System"))
        return false;

    shader_config.m_path = "res/shader/f_deferred_lighting.glsl";
    shader_config.m_type = shader_type::FRAGMENT_SHADER;
    d_fragment           = shader::create(shader_config);
    if (!check_creation(d_fragment.get(), "lighting pass fragment shader", "Render System"))
        return false;

    m_lighting_pass = shader_program::create_graphics_pipeline(d_vertex, nullptr, nullptr, d_geometry, d_fragment);
    if (!check_creation(m_lighting_pass.get(), "lighting pass shader program", "Render System"))
        return false;

    // composing pass
    shader_config.m_path = "res/shader/f_composing.glsl";
    shader_config.m_type = shader_type::FRAGMENT_SHADER;
    d_fragment           = shader::create(shader_config);
    if (!check_creation(d_fragment.get(), "composing pass fragment shader", "Render System"))
        return false;

    m_composing_pass = shader_program::create_graphics_pipeline(d_vertex, nullptr, nullptr, d_geometry, d_fragment);
    if (!check_creation(m_composing_pass.get(), "composing pass shader program", "Render System"))
        return false;

    shader_config.m_path                  = "res/shader/c_construct_luminance_buffer.glsl";
    shader_config.m_type                  = shader_type::COMPUTE_SHADER;
    shader_ptr construct_luminance_buffer = shader::create(shader_config);
    if (!check_creation(construct_luminance_buffer.get(), "luminance construction compute shader", "Render System"))
        return false;

    // luminance compute for auto exposure
    m_construct_luminance_buffer = shader_program::create_compute_pipeline(construct_luminance_buffer);
    if (!check_creation(m_construct_luminance_buffer.get(), "luminance construction compute shader program", "Render System"))
        return false;

    shader_config.m_path               = "res/shader/c_luminance_buffer_reduction.glsl";
    shader_config.m_type               = shader_type::COMPUTE_SHADER;
    shader_ptr reduce_luminance_buffer = shader::create(shader_config);
    if (!check_creation(reduce_luminance_buffer.get(), "luminance reduction compute shader", "Render System"))
        return false;

    m_reduce_luminance_buffer = shader_program::create_compute_pipeline(reduce_luminance_buffer);
    if (!check_creation(m_reduce_luminance_buffer.get(), "luminance reduction compute shader program", "Render System"))
        return false;

    buffer_configuration b_config;
    b_config.m_access            = buffer_access::MAPPED_ACCESS_READ_WRITE;
    b_config.m_size              = 256 * sizeof(uint32) + sizeof(float);
    b_config.m_target            = buffer_target::SHADER_STORAGE_BUFFER;
    m_luminance_histogram_buffer = buffer::create(b_config);

    m_luminance_data_mapping = static_cast<luminance_data*>(m_luminance_histogram_buffer->map(0, b_config.m_size, buffer_access::MAPPED_ACCESS_WRITE));
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
    default_texture->set_data(format::RGB8, 1, 1, format::RGB, format::UNSIGNED_BYTE, albedo);
    attachment_config.m_layers = 3;
    default_texture_array = texture::create(attachment_config);
    if (!check_creation(default_texture_array.get(), "default texture array", "Render System"))
        return false;
    default_texture_array->set_data(format::RGB8, 1, 1, format::RGB, format::UNSIGNED_BYTE, albedo);

    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &m_uniform_buffer_alignment);

    memset(m_lp_uniforms.debug_views.debug, 0, sizeof(m_lp_uniforms.debug_views.debug));
    m_lp_uniforms.debug = std140_bool(false);

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

void deferred_pbr_render_system::begin_render()
{
    PROFILE_ZONE;
    m_hardware_stats.last_frame.draw_calls = 0;
    m_hardware_stats.last_frame.meshes     = 0;
    m_hardware_stats.last_frame.primitives = 0;
    m_hardware_stats.last_frame.materials  = 0;

    // TODO Paul: This should not be done here, this is pretty bad!
    m_command_buffer->clear_framebuffer(clear_buffer_mask::COLOR_AND_DEPTH_STENCIL, attachment_mask::ALL, 0.1f, 0.1f, 0.1f, 1.0f);
    m_command_buffer->clear_framebuffer(clear_buffer_mask::COLOR_AND_DEPTH, attachment_mask::ALL_DRAW_BUFFERS_AND_DEPTH, 0.0f, 0.0f, 0.0f, 1.0f, m_gbuffer);
    m_command_buffer->clear_framebuffer(clear_buffer_mask::COLOR_AND_DEPTH, attachment_mask::ALL_DRAW_BUFFERS_AND_DEPTH, 0.0f, 0.0f, 0.0f, 1.0f, m_hdr_buffer);
    m_command_buffer->clear_framebuffer(clear_buffer_mask::COLOR_AND_DEPTH, attachment_mask::ALL_DRAW_BUFFERS_AND_DEPTH, 0.0f, 0.0f, 0.0f, 1.0f, m_backbuffer);

    m_command_buffer->set_depth_test(true);
    m_command_buffer->set_depth_func(compare_operation::LESS);
    m_command_buffer->set_face_culling(true);
    m_command_buffer->set_cull_face(polygon_face::FACE_BACK);
    m_command_buffer->bind_framebuffer(m_gbuffer);
    m_command_buffer->bind_shader_program(m_scene_geometry_pass);

    auto scene  = m_shared_context->get_current_scene();
    auto camera = scene->get_active_camera_data();
    if (camera.camera_info)
    {
        set_view_projection_matrix(camera.camera_info->view_projection);
    }

    m_command_buffer->wait_for_buffer(m_frame_uniform_buffer);
    if (m_wireframe)
        m_command_buffer->set_polygon_mode(polygon_face::FACE_FRONT_AND_BACK, polygon_mode::LINE);
}

void deferred_pbr_render_system::finish_render(float dt)
{
    PROFILE_ZONE;
    m_command_buffer->attach(m_render_queue);

    if (m_pipeline_steps[mango::render_step::shadow_map])
    {
        if (m_lp_uniforms.directional.intensity > 1e-5f && !m_lp_uniforms.debug.value())
        {
            std::static_pointer_cast<shadow_map_step>(m_pipeline_steps[mango::render_step::shadow_map])
                ->set_directional_light_view_projection_matrix(m_lp_uniforms.directional.view_projection.value());

            m_pipeline_steps[mango::render_step::shadow_map]->execute(m_command_buffer);
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

    // lighting pass
    {
        m_command_buffer->bind_framebuffer(m_hdr_buffer); // bind hdr.
        m_command_buffer->bind_shader_program(m_lighting_pass);
        m_command_buffer->set_polygon_mode(polygon_face::FACE_FRONT_AND_BACK, polygon_mode::FILL);

        bind_lighting_pass_uniform_buffer();
        m_command_buffer->bind_texture(0, m_gbuffer->get_attachment(framebuffer_attachment::COLOR_ATTACHMENT0), 0);
        m_command_buffer->bind_texture(1, m_gbuffer->get_attachment(framebuffer_attachment::COLOR_ATTACHMENT1), 1);
        m_command_buffer->bind_texture(2, m_gbuffer->get_attachment(framebuffer_attachment::COLOR_ATTACHMENT2), 2);
        m_command_buffer->bind_texture(3, m_gbuffer->get_attachment(framebuffer_attachment::COLOR_ATTACHMENT3), 3);
        m_command_buffer->bind_texture(4, m_gbuffer->get_attachment(framebuffer_attachment::DEPTH_ATTACHMENT), 4);
        if (m_pipeline_steps[mango::render_step::shadow_map])
            std::static_pointer_cast<shadow_map_step>(m_pipeline_steps[mango::render_step::shadow_map])->bind_shadow_maps(m_command_buffer);
        else
            m_command_buffer->bind_texture(8, default_texture_array, 8);
        if (m_pipeline_steps[mango::render_step::ibl])
            std::static_pointer_cast<ibl_step>(m_pipeline_steps[mango::render_step::ibl])->bind_image_based_light_maps(m_command_buffer);
        else
        {
            m_command_buffer->bind_texture(5, default_texture, 5);
            m_command_buffer->bind_texture(6, default_texture, 6);
            m_command_buffer->bind_texture(7, default_texture, 7);
            float intensity = 1.0f;
            m_command_buffer->bind_single_uniform(9, &intensity, sizeof(intensity));
        }

        // TODO Paul: Check if the binding is better for performance or not.
        m_command_buffer->bind_vertex_array(default_vao);

        m_command_buffer->draw_arrays(primitive_topology::POINTS, 0, 1);
        m_hardware_stats.last_frame.draw_calls++; // TODO Paul: This measurements should be done, on glCalls.
    }

    auto scene  = m_shared_context->get_current_scene();
    auto camera = scene->get_active_camera_data();

    // environment drawing

    if (m_pipeline_steps[mango::render_step::ibl] && !m_lp_uniforms.debug.value())
    {
        // We need the not translated view for skybox rendering.
        if (camera.camera_info)
            std::static_pointer_cast<ibl_step>(m_pipeline_steps[mango::render_step::ibl])->set_view_projection_matrix(camera.camera_info->projection * glm::mat4(glm::mat3(camera.camera_info->view)));

        m_command_buffer->set_depth_func(compare_operation::LESS_EQUAL);
        m_command_buffer->set_cull_face(polygon_face::FACE_FRONT);
        m_pipeline_steps[mango::render_step::ibl]->execute(m_command_buffer);
    }

    // auto exposure compute shaders
    if (camera.camera_info->physical.adaptive_exposure && !m_lp_uniforms.debug.value())
    {
        m_command_buffer->bind_shader_program(m_construct_luminance_buffer);
        auto tex = m_hdr_buffer->get_attachment(framebuffer_attachment::COLOR_ATTACHMENT0);
        m_command_buffer->bind_image_texture(0, tex, 0, false, 0, base_access::READ_ONLY, format::RGBA32F);
        m_command_buffer->bind_buffer(1, m_luminance_histogram_buffer, buffer_target::SHADER_STORAGE_BUFFER);
        glm::vec2 params = glm::vec2(-10.0f, 1.0f / 42.0f); // min -10.0, max +32.0
        m_command_buffer->bind_single_uniform(1, &(params), sizeof(params));

        m_command_buffer->dispatch_compute(tex->get_width() / 16, tex->get_height() / 16, 1);

        m_command_buffer->add_memory_barrier(memory_barrier_bit::SHADER_STORAGE_BARRIER_BIT);

        m_command_buffer->bind_shader_program(m_reduce_luminance_buffer);
        m_command_buffer->bind_buffer(0, m_luminance_histogram_buffer, buffer_target::SHADER_STORAGE_BUFFER);

        // time coefficient with tau = 1.1;
        float tau              = 1.1f;
        float time_coefficient = 1.0f - exp(-dt * tau);
        glm::vec4 red_params   = glm::vec4(time_coefficient, tex->get_width() * tex->get_height(), -10.0f, 42.0f); // min -10.0, max +32.0
        m_command_buffer->bind_single_uniform(0, &(red_params), sizeof(red_params));

        m_command_buffer->dispatch_compute(1, 1, 1);

        m_command_buffer->add_memory_barrier(memory_barrier_bit::SHADER_STORAGE_BARRIER_BIT);

        apply_auto_exposure(camera);
    }

    // composite
    {
        m_command_buffer->set_depth_func(compare_operation::LESS);
        m_command_buffer->set_cull_face(polygon_face::FACE_BACK);
        m_command_buffer->bind_framebuffer(m_backbuffer); // bind backbuffer.
        m_command_buffer->bind_shader_program(m_composing_pass);
        camera.camera_info->physical.aperture      = glm::clamp(camera.camera_info->physical.aperture, min_aperture, max_aperture);
        camera.camera_info->physical.shutter_speed = glm::clamp(camera.camera_info->physical.shutter_speed, min_shutter_speed, max_shutter_speed);
        camera.camera_info->physical.iso           = glm::clamp(camera.camera_info->physical.iso, min_iso, max_iso);
        float ape                                  = camera.camera_info->physical.aperture;
        float shu                                  = camera.camera_info->physical.shutter_speed;
        float iso                                  = camera.camera_info->physical.iso;
        float ev100                                = glm::log2((ape * ape) / shu * 100.0f / iso);
        float camera_exposure                      = 1.0f / (1.2f * glm::exp2(ev100));
        m_command_buffer->bind_single_uniform(1, &camera_exposure, sizeof(camera_exposure));
        int32 b_val = m_lp_uniforms.debug.value();
        m_command_buffer->bind_single_uniform(2, &(b_val), sizeof(b_val));

        m_command_buffer->bind_texture(0, m_hdr_buffer->get_attachment(framebuffer_attachment::COLOR_ATTACHMENT0), 0);

        // TODO Paul: Check if the binding is better for performance or not.
        m_command_buffer->bind_vertex_array(default_vao);

        m_command_buffer->draw_arrays(primitive_topology::POINTS, 0, 1);
        m_hardware_stats.last_frame.draw_calls++; // TODO Paul: This measurements should be done, on glCalls.
    }

    m_command_buffer->lock_buffer(m_frame_uniform_buffer);

    m_command_buffer->bind_framebuffer(nullptr);
    m_command_buffer->bind_vertex_array(nullptr);
    // We need to unbind the program so we can make changes to the textures.
    m_command_buffer->bind_shader_program(nullptr);

    // We try to reset the default state as possible, without crashing all optimizations.

    {
        GL_NAMED_PROFILE_ZONE("Deferred Renderer");
        m_command_buffer->execute();
    }

    m_frame_uniform_offset = 0;
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

void deferred_pbr_render_system::set_model_info(const glm::mat4& model_matrix, bool has_normals, bool has_tangents)
{
    PROFILE_ZONE;
    class set_model_info_cmd : public command
    {
      public:
        buffer_ptr m_uniform_buffer;
        int32 m_offset;
        set_model_info_cmd(buffer_ptr uniform_buffer, int32 offset)
            : m_uniform_buffer(uniform_buffer)
            , m_offset(offset)
        {
        }

        void execute(graphics_state&) override
        {
            GL_NAMED_PROFILE_ZONE("Set Model Info");
            MANGO_ASSERT(m_uniform_buffer, "Uniforms do not exist anymore.");
            m_uniform_buffer->bind(buffer_target::UNIFORM_BUFFER, 0, m_offset, sizeof(scene_vertex_uniforms));
        }
    };

    scene_vertex_uniforms u{ std140_mat4(model_matrix), std140_mat3(glm::transpose(glm::inverse(model_matrix))), std140_bool(has_normals), std140_bool(has_tangents), 0, 0 };

    MANGO_ASSERT(m_frame_uniform_offset < uniform_buffer_size - static_cast<int32>(sizeof(scene_vertex_uniforms)), "Uniform buffer size is too small.");
    memcpy(static_cast<g_byte*>(m_mapped_uniform_memory) + m_frame_uniform_offset, &u, sizeof(scene_vertex_uniforms));

    m_render_queue->submit<set_model_info_cmd>(m_frame_uniform_buffer, m_frame_uniform_offset);
    if (m_pipeline_steps[mango::render_step::shadow_map])
        std::static_pointer_cast<shadow_map_step>(m_pipeline_steps[mango::render_step::shadow_map])->get_caster_queue()->submit<set_model_info_cmd>(m_frame_uniform_buffer, m_frame_uniform_offset);
    m_frame_uniform_offset += m_uniform_buffer_alignment;
    m_hardware_stats.last_frame.meshes++;
}

void deferred_pbr_render_system::draw_mesh(const vertex_array_ptr& vertex_array, const material_ptr& mat, primitive_topology topology, int32 first, int32 count, index_type type, int32 instance_count)
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
    class push_material_cmd : public command
    {
      public:
        buffer_ptr m_uniform_buffer;
        int32 m_offset;
        push_material_cmd(buffer_ptr uniform_buffer, int32 offset)
            : m_uniform_buffer(uniform_buffer)
            , m_offset(offset)
        {
        }

        void execute(graphics_state&) override
        {
            GL_NAMED_PROFILE_ZONE("Push Material");
            MANGO_ASSERT(m_uniform_buffer, "Uniforms do not exist anymore.");
            m_uniform_buffer->bind(buffer_target::UNIFORM_BUFFER, 1, m_offset, sizeof(scene_material_uniforms));
        }
    };

    scene_material_uniforms u;

    u.base_color     = std140_vec4(mat->base_color);
    u.emissive_color = std140_vec3(mat->emissive_color);
    u.metallic       = (g_float)mat->metallic;
    u.roughness      = (g_float)mat->roughness;
    if (mat->use_base_color_texture)
    {
        u.base_color_texture = std140_bool(true);
        m_render_queue->bind_texture(0, mat->base_color_texture, 1);
    }
    else
    {
        u.base_color_texture = std140_bool(false);
        m_render_queue->bind_texture(0, default_texture, 1);
    }
    if (mat->use_roughness_metallic_texture)
    {
        u.roughness_metallic_texture = std140_bool(true);
        m_render_queue->bind_texture(1, mat->roughness_metallic_texture, 2);
    }
    else
    {
        u.roughness_metallic_texture = std140_bool(false);
        m_render_queue->bind_texture(1, default_texture, 2);
    }
    if (mat->use_occlusion_texture)
    {
        u.occlusion_texture = std140_bool(true);
        m_render_queue->bind_texture(2, mat->occlusion_texture, 3);
        u.packed_occlusion = std140_bool(false);
    }
    else
    {
        u.occlusion_texture = std140_bool(false);
        m_render_queue->bind_texture(2, default_texture, 3);
        // eventually it is packed
        u.packed_occlusion = std140_bool(mat->packed_occlusion && mat->use_packed_occlusion);
    }
    if (mat->use_normal_texture)
    {
        u.normal_texture = std140_bool(true);
        m_render_queue->bind_texture(3, mat->normal_texture, 4);
    }
    else
    {
        u.normal_texture = std140_bool(false);
        m_render_queue->bind_texture(3, default_texture, 4);
    }
    if (mat->use_emissive_color_texture)
    {
        u.emissive_color_texture = std140_bool(true);
        m_render_queue->bind_texture(4, mat->emissive_color_texture, 5);
    }
    else
    {
        u.emissive_color_texture = std140_bool(false);
        m_render_queue->bind_texture(4, default_texture, 5);
    }

    u.alpha_mode   = static_cast<g_int>(mat->alpha_rendering);
    u.alpha_cutoff = mat->alpha_cutoff;

    MANGO_ASSERT(m_frame_uniform_offset < uniform_buffer_size - static_cast<int32>(sizeof(scene_material_uniforms)), "Uniform buffer size is too small.");
    memcpy(static_cast<g_byte*>(m_mapped_uniform_memory) + m_frame_uniform_offset, &u, sizeof(scene_material_uniforms));

    m_render_queue->submit<push_material_cmd>(m_frame_uniform_buffer, m_frame_uniform_offset);
    m_frame_uniform_offset += m_uniform_buffer_alignment;

    if (mat->double_sided)
    {
        m_render_queue->set_face_culling(false);
        if (caster_queue)
            caster_queue->set_face_culling(false);
    }

    if (type == index_type::NONE)
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
    if (caster_queue)
    {
        caster_queue->set_face_culling(true);
        caster_queue->bind_vertex_array(nullptr);
    }

    // This needs to be done.
    // TODO Paul: State synchronization is not perfect.
    m_render_queue->bind_texture(0, nullptr, 0);
    m_render_queue->bind_texture(1, nullptr, 1);
    m_render_queue->bind_texture(2, nullptr, 2);
    m_render_queue->bind_texture(3, nullptr, 3);
    m_render_queue->bind_texture(4, nullptr, 4);

    if (caster_queue)
    {
        caster_queue->bind_texture(0, nullptr, 0);
        caster_queue->bind_texture(1, nullptr, 1);
        caster_queue->bind_texture(2, nullptr, 2);
        caster_queue->bind_texture(3, nullptr, 3);
        caster_queue->bind_texture(4, nullptr, 4);
    }
}

void deferred_pbr_render_system::set_view_projection_matrix(const glm::mat4& view_projection)
{
    PROFILE_ZONE;
    m_command_buffer->bind_single_uniform(0, &const_cast<glm::mat4&>(view_projection), sizeof(glm::mat4));
}

void deferred_pbr_render_system::set_environment_texture(const texture_ptr& hdr_texture)
{
    PROFILE_ZONE;
    if (m_pipeline_steps[mango::render_step::ibl])
    {
        auto ibl = std::static_pointer_cast<ibl_step>(m_pipeline_steps[mango::render_step::ibl]);
        ibl->load_from_hdr(hdr_texture);
        ibl->set_render_level(0.0f);
        ibl->set_intensity(mango::default_environment_intensity);
    }
}

void deferred_pbr_render_system::set_environment_settings(float render_level, float intensity)
{
    PROFILE_ZONE;
    if (m_pipeline_steps[mango::render_step::ibl])
    {
        auto ibl = std::static_pointer_cast<ibl_step>(m_pipeline_steps[mango::render_step::ibl]);
        ibl->set_render_level(render_level);
        ibl->set_intensity(intensity);
    }
}

void deferred_pbr_render_system::submit_light(light_type type, light_data* data)
{
    PROFILE_ZONE;
    // view projection for directional lights
    if (type == light_type::directional) // currently always true
    {
        auto directional_data               = static_cast<directional_light_data*>(data);
        m_lp_uniforms.directional.direction = directional_data->direction;
        m_lp_uniforms.directional.color     = std140_vec3(directional_data->light_color);
        m_lp_uniforms.directional.intensity = directional_data->intensity;
        glm::mat4 projection;
        glm::mat4 view;
        projection                                = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, -10.0f, 10.0f);
        auto front                                = glm::normalize(-directional_data->direction);
        auto right                                = glm::normalize(glm::cross(GLOBAL_UP, front));
        auto up                                   = glm::normalize(glm::cross(front, right));
        view                                      = glm::lookAt(glm::normalize(directional_data->direction), glm::vec3(0, 0, 0), up);
        m_lp_uniforms.directional.view_projection = projection * view;
    }
}

void deferred_pbr_render_system::bind_lighting_pass_uniform_buffer()
{
    PROFILE_ZONE;
    class set_lighting_pass_uniforms_cmd : public command
    {
      public:
        buffer_ptr m_uniform_buffer;
        int32 m_offset;
        set_lighting_pass_uniforms_cmd(buffer_ptr uniform_buffer, int32 offset)
            : m_uniform_buffer(uniform_buffer)
            , m_offset(offset)
        {
        }

        void execute(graphics_state&) override
        {
            GL_NAMED_PROFILE_ZONE("Bind Lighting Pass Uniforms");
            MANGO_ASSERT(m_uniform_buffer, "Uniforms do not exist anymore.");
            m_uniform_buffer->bind(buffer_target::UNIFORM_BUFFER, 0, m_offset, sizeof(lighting_pass_uniforms));
        }
    };

    auto scene  = m_shared_context->get_current_scene();
    auto camera = scene->get_active_camera_data();
    if (camera.camera_info && camera.transform)
    {
        m_lp_uniforms.inverse_view_projection = glm::inverse(camera.camera_info->view_projection);
        m_lp_uniforms.camera_position         = std140_vec3(camera.transform->world_transformation_matrix[3]);
        m_lp_uniforms.camera_params           = std140_vec4(glm::vec4(camera.camera_info->z_near, camera.camera_info->z_far, 0.0f, 0.0f));
    }
    else
    {
        MANGO_LOG_ERROR("Lighting pass uniforms can not be set!");
    }
    m_lp_uniforms.directional.cast_shadows = (m_pipeline_steps[mango::render_step::shadow_map] != nullptr);

    MANGO_ASSERT(m_frame_uniform_offset < uniform_buffer_size - static_cast<int32>(sizeof(lighting_pass_uniforms)), "Uniform buffer size is too small.");
    memcpy(static_cast<g_byte*>(m_mapped_uniform_memory) + m_frame_uniform_offset, &m_lp_uniforms, sizeof(lighting_pass_uniforms));

    m_command_buffer->submit<set_lighting_pass_uniforms_cmd>(m_frame_uniform_buffer, m_frame_uniform_offset);
    m_frame_uniform_offset += m_uniform_buffer_alignment;

    // clean up m_lp_uniforms.
    m_lp_uniforms.directional.intensity = 0.0f;
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
    float target_ev      = glm::log2(avg_luminance * 100.0f / K);

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
    ImGui::Checkbox("Wireframe##deferred_pbr", &m_wireframe);
    if (ImGui::CollapsingHeader("Steps##deferred_pbr"))
    {
        static bool has_ibl = m_pipeline_steps[mango::render_step::ibl] != nullptr;
        change_flag         = has_ibl;
        ImGui::Checkbox("IBL##deferred_pbr", &has_ibl);
        if (has_ibl != change_flag)
        {
            if (has_ibl)
            {
                auto step_ibl = std::make_shared<ibl_step>();
                step_ibl->create();
                m_pipeline_steps[mango::render_step::ibl] = std::static_pointer_cast<pipeline_step>(step_ibl);
            }
            else
            {
                m_pipeline_steps[mango::render_step::ibl] = nullptr;
            }
        }
        static bool has_shadow_map = m_pipeline_steps[mango::render_step::shadow_map] != nullptr;
        change_flag                = has_shadow_map;
        ImGui::Checkbox("Shadow Map##deferred_pbr", &has_shadow_map);
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
    }
    if (ImGui::CollapsingHeader("Debug##deferred_pbr"))
    {
        memset(m_lp_uniforms.debug_views.debug, 0, sizeof(m_lp_uniforms.debug_views.debug));
        m_lp_uniforms.debug = std140_bool(false);
        ImGui::Combo("Views##deferred_pbr", &current_debug, debug);
        if (current_debug)
        {
            m_lp_uniforms.debug_views.debug[current_debug - 1] = std140_bool(true);
            m_lp_uniforms.debug                                = std140_bool(true);
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
