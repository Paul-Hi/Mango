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
#include <mango/imgui_helper.hpp>
#include <mango/profile.hpp>
#include <mango/scene.hpp>
#include <rendering/pipelines/deferred_pbr_render_system.hpp>
#include <util/helpers.hpp>

using namespace mango;

#ifdef MANGO_DEBUG

static void GLAPIENTRY debugCallback(g_enum source, g_enum type, g_uint id, g_enum severity, g_sizei length, const g_char* message, const void* userParam);
#endif // MANGO_DEBUG

//! \brief Default vertex array object for second pass with geometry shader generated geometry.
vertex_array_ptr default_vao;
//! \brief Default texture that is bound to every texture unit not in use to prevent warnings.
texture_ptr default_texture;
//! \brief Default cubemap texture that is bound to every texture unit not in use to prevent warnings.
texture_ptr default_cube_texture;
//! \brief Default texture array that is bound to every texture unit not in use to prevent warnings.
texture_ptr default_texture_array;
//! \brief Default material.
material_ptr default_material;

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
    m_renderer_info.api_version = "OpenGL ";
    m_renderer_info.api_version.append(string((const char*)glGetString(GL_VERSION)));
    MANGO_LOG_INFO("Using: {0}", m_renderer_info.api_version);
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

    if (!create_renderer_resources())
    {
        MANGO_LOG_ERROR("Resource Creation Failed! Render System is not available!");
        return false;
    }

    // create light stack
    m_light_stack.init();

    for (int32 i = 0; i < 9; ++i)
        m_lighting_pass_data.debug_views.debug[i] = false;

    m_lighting_pass_data.debug_view_enabled             = false;
    m_lighting_pass_data.debug_options.show_cascades    = false;
    m_lighting_pass_data.debug_options.draw_shadow_maps = false;

    return true;
}

bool deferred_pbr_render_system::create_renderer_resources()
{
    shared_ptr<window_system_impl> ws = m_shared_context->get_window_system_internal().lock();
    MANGO_ASSERT(ws, "Window System is expired!");
    int32 w = ws->get_width();
    int32 h = ws->get_height();

    m_renderer_info.canvas.x      = 0;
    m_renderer_info.canvas.y      = 0;
    m_renderer_info.canvas.width  = w;
    m_renderer_info.canvas.height = h;

    m_begin_render_commands   = command_buffer<min_key>::create(512);
    m_global_binding_commands = command_buffer<min_key>::create(256);
    m_gbuffer_commands        = command_buffer<max_key>::create(524288 * 2); // 1.0 MiB?
    m_transparent_commands    = command_buffer<max_key>::create(524288 * 2); // 1.0 MiB?
    m_lighting_pass_commands  = command_buffer<min_key>::create(512);
    m_exposure_commands       = command_buffer<min_key>::create(512);
    m_composite_commands      = command_buffer<min_key>::create(256);
    m_finish_render_commands  = command_buffer<min_key>::create(256);

    texture_configuration attachment_config;
    attachment_config.generate_mipmaps        = 1;
    attachment_config.is_standard_color_space = false;
    attachment_config.texture_min_filter      = texture_parameter::filter_nearest;
    attachment_config.texture_mag_filter      = texture_parameter::filter_nearest;
    attachment_config.texture_wrap_s          = texture_parameter::wrap_clamp_to_edge;
    attachment_config.texture_wrap_t          = texture_parameter::wrap_clamp_to_edge;

    framebuffer_configuration gbuffer_config;
    gbuffer_config.color_attachment0 = texture::create(attachment_config);
    gbuffer_config.color_attachment0->set_data(format::rgba8, w, h, format::rgba, format::t_unsigned_int_8_8_8_8, nullptr);
    gbuffer_config.color_attachment1 = texture::create(attachment_config);
    gbuffer_config.color_attachment1->set_data(format::rgb10_a2, w, h, format::rgba, format::t_unsigned_int_10_10_10_2, nullptr);
    gbuffer_config.color_attachment2 = texture::create(attachment_config);
    gbuffer_config.color_attachment2->set_data(format::rgba8, w, h, format::rgba, format::t_unsigned_int_8_8_8_8, nullptr);
    gbuffer_config.color_attachment3 = texture::create(attachment_config);
    gbuffer_config.color_attachment3->set_data(format::rgba8, w, h, format::rgba, format::t_unsigned_int_8_8_8_8, nullptr);
    gbuffer_config.depth_attachment = texture::create(attachment_config);
    gbuffer_config.depth_attachment->set_data(format::depth_component32f, w, h, format::depth_component, format::t_float, nullptr);

    gbuffer_config.width  = w;
    gbuffer_config.height = h;

    m_gbuffer = framebuffer::create(gbuffer_config);

    if (!check_creation(m_gbuffer.get(), "gbuffer"))
        return false;

    // HDR for auto exposure
    framebuffer_configuration hdr_buffer_config;
    attachment_config.generate_mipmaps  = calculate_mip_count(w, h);
    hdr_buffer_config.color_attachment0 = texture::create(attachment_config);
    hdr_buffer_config.color_attachment0->set_data(format::rgba32f, w, h, format::rgba, format::t_float, nullptr);
    attachment_config.generate_mipmaps = 1;
    hdr_buffer_config.depth_attachment = texture::create(attachment_config);
    hdr_buffer_config.depth_attachment->set_data(format::depth_component32f, w, h, format::depth_component, format::t_float, nullptr);

    hdr_buffer_config.width  = w;
    hdr_buffer_config.height = h;

    m_hdr_buffer = framebuffer::create(hdr_buffer_config);

    if (!check_creation(m_hdr_buffer.get(), "hdr buffer"))
        return false;

    // backbuffer

    framebuffer_configuration backbuffer_config;
    backbuffer_config.color_attachment0 = texture::create(attachment_config);
    backbuffer_config.color_attachment0->set_data(format::rgb8, w, h, format::rgb, format::t_unsigned_int, nullptr);
    backbuffer_config.depth_attachment = texture::create(attachment_config);
    backbuffer_config.depth_attachment->set_data(format::depth_component32f, w, h, format::depth_component, format::t_float, nullptr);

    backbuffer_config.width  = w;
    backbuffer_config.height = h;

    m_backbuffer = framebuffer::create(backbuffer_config);

    if (!check_creation(m_backbuffer.get(), "backbuffer"))
        return false;

    // postprocessing buffer is the same as an backbuffer
    backbuffer_config.depth_attachment = texture::create(attachment_config);
    backbuffer_config.depth_attachment->set_data(format::depth_component32f, w, h, format::depth_component, format::t_float, nullptr);
    // need linear here
    attachment_config.texture_min_filter = texture_parameter::filter_linear;
    attachment_config.texture_mag_filter = texture_parameter::filter_linear;
    backbuffer_config.color_attachment0  = texture::create(attachment_config);
    backbuffer_config.color_attachment0->set_data(format::rgb8, w, h, format::rgb, format::t_unsigned_int, nullptr);
    m_post_buffer = framebuffer::create(backbuffer_config);

    if (!check_creation(m_post_buffer.get(), "postprocessing buffer"))
        return false;

    // frame uniform buffer
    m_frame_uniform_buffer = gpu_buffer::create();
    if (!check_creation(m_frame_uniform_buffer.get(), "frame uniform buffer"))
        return false;

    if (!m_frame_uniform_buffer->init(524288 * 2, buffer_technique::triple_buffering)) // Triple Buffering with 1 MiB per Frame.
        return false;

    // scene geometry pass
    shader_configuration shader_config;
    shader_config.path = "res/shader/forward/v_scene_gltf.glsl";
    shader_config.type = shader_type::vertex_shader;
    shader_config.defines.push_back({ "GBUFFER_PREPASS", "" });
    shader_config.defines.push_back({ "VERTEX", "" });
    shader_ptr d_vertex = shader::create(shader_config);
    shader_config.defines.clear();
    if (!check_creation(d_vertex.get(), "geometry pass vertex shader"))
        return false;

    shader_config.path = "res/shader/forward/f_scene_gltf.glsl";
    shader_config.type = shader_type::fragment_shader;
    shader_config.defines.push_back({ "GBUFFER_PREPASS", "" });
    shader_config.defines.push_back({ "FRAGMENT", "" });
    shader_ptr d_fragment = shader::create(shader_config);
    shader_config.defines.clear();
    if (!check_creation(d_fragment.get(), "geometry pass fragment shader"))
        return false;

    m_scene_geometry_pass = shader_program::create_graphics_pipeline(d_vertex, nullptr, nullptr, nullptr, d_fragment);
    if (!check_creation(m_scene_geometry_pass.get(), "geometry pass shader program"))
        return false;

    // transparent pass
    shader_config.path = "res/shader/forward/f_scene_transparent_gltf.glsl";
    shader_config.type = shader_type::fragment_shader;
    shader_config.defines.push_back({ "LIGHTING", "" });
    shader_config.defines.push_back({ "FORWARD", "" });
    d_fragment = shader::create(shader_config);
    shader_config.defines.clear();
    if (!check_creation(d_fragment.get(), "transparent pass fragment shader"))
        return false;

    m_transparent_pass = shader_program::create_graphics_pipeline(d_vertex, nullptr, nullptr, nullptr, d_fragment);
    if (!check_creation(m_transparent_pass.get(), "transparent pass shader program"))
        return false;

    // lighting pass
    shader_config.path = "res/shader/v_screen_space_triangle.glsl";
    shader_config.type = shader_type::vertex_shader;
    d_vertex           = shader::create(shader_config);
    if (!check_creation(d_vertex.get(), "screen space triangle vertex shader"))
        return false;

    shader_config.path = "res/shader/deferred/f_deferred_lighting.glsl";
    shader_config.type = shader_type::fragment_shader;
    shader_config.defines.push_back({ "LIGHTING", "" });
    shader_config.defines.push_back({ "DEFERRED", "" });
    d_fragment = shader::create(shader_config);
    shader_config.defines.clear();
    if (!check_creation(d_fragment.get(), "lighting pass fragment shader"))
        return false;

    m_lighting_pass = shader_program::create_graphics_pipeline(d_vertex, nullptr, nullptr, nullptr, d_fragment);
    if (!check_creation(m_lighting_pass.get(), "lighting pass shader program"))
        return false;

    // composing pass
    shader_config.path = "res/shader/post/f_composing.glsl";
    shader_config.type = shader_type::fragment_shader;
    d_fragment         = shader::create(shader_config);
    if (!check_creation(d_fragment.get(), "composing pass fragment shader"))
        return false;

    m_composing_pass = shader_program::create_graphics_pipeline(d_vertex, nullptr, nullptr, nullptr, d_fragment);
    if (!check_creation(m_composing_pass.get(), "composing pass shader program"))
        return false;

    // luminance compute for auto exposure
    shader_config.path                    = "res/shader/luminance_compute/c_construct_luminance_buffer.glsl";
    shader_config.type                    = shader_type::compute_shader;
    shader_ptr construct_luminance_buffer = shader::create(shader_config);
    if (!check_creation(construct_luminance_buffer.get(), "luminance construction compute shader"))
        return false;

    m_construct_luminance_buffer = shader_program::create_compute_pipeline(construct_luminance_buffer);
    if (!check_creation(m_construct_luminance_buffer.get(), "luminance construction compute shader program"))
        return false;

    shader_config.path                 = "res/shader/luminance_compute/c_luminance_buffer_reduction.glsl";
    shader_config.type                 = shader_type::compute_shader;
    shader_ptr reduce_luminance_buffer = shader::create(shader_config);
    if (!check_creation(reduce_luminance_buffer.get(), "luminance reduction compute shader"))
        return false;

    m_reduce_luminance_buffer = shader_program::create_compute_pipeline(reduce_luminance_buffer);
    if (!check_creation(m_reduce_luminance_buffer.get(), "luminance reduction compute shader program"))
        return false;

    buffer_configuration b_config;
    b_config.access              = buffer_access::mapped_access_read_write;
    b_config.size                = 256 * sizeof(uint32) + sizeof(float);
    b_config.target              = buffer_target::shader_storage_buffer;
    m_luminance_histogram_buffer = buffer::create(b_config);

    m_luminance_data_mapping = static_cast<luminance_data*>(m_luminance_histogram_buffer->map(0, b_config.size, buffer_access::mapped_access_write));
    if (!check_mapping(m_luminance_data_mapping, "luminance data"))
        return false;

    memset(&m_luminance_data_mapping->histogram[0], 0, 256 * sizeof(uint32));
    m_luminance_data_mapping->luminance = 1.0f;

    // default vao needed
    default_vao = vertex_array::create();
    if (!check_creation(default_vao.get(), "default vertex array object"))
        return false;
    // default textures needed
    default_texture = texture::create(attachment_config);
    if (!check_creation(default_texture.get(), "default texture"))
        return false;
    g_ubyte albedo[4] = { 1, 1, 1, 255 };
    default_texture->set_data(format::rgba8, 1, 1, format::rgba, format::t_unsigned_byte, albedo);
    attachment_config.is_cubemap = true;
    default_cube_texture         = texture::create(attachment_config);
    if (!check_creation(default_cube_texture.get(), "default cube texture"))
        return false;
    default_cube_texture->set_data(format::rgba8, 1, 1, format::rgba, format::t_unsigned_byte, albedo);
    attachment_config.layers     = 3;
    attachment_config.is_cubemap = false;
    default_texture_array        = texture::create(attachment_config);
    if (!check_creation(default_texture_array.get(), "default texture array"))
        return false;
    default_texture_array->set_data(format::rgb8, 1, 1, format::rgb, format::t_unsigned_byte, albedo);

    default_material             = std::make_shared<material>();
    default_material->base_color = glm::vec4(glm::vec3(0.75f), 1.0f);
    default_material->metallic   = 0.0f;
    default_material->roughness  = 1.0f;

    return true;
}

void deferred_pbr_render_system::configure(const render_configuration& configuration)
{
    PROFILE_ZONE;
    m_vsync = configuration.is_vsync_enabled();
    auto ws = m_shared_context->get_window_system_internal().lock();
    MANGO_ASSERT(ws, "Window System is expired!");
    ws->set_vsync(m_vsync);

    // additional render steps
    if (configuration.get_render_steps()[mango::render_step::cubemap])
    {
        // create an extra object that is capable to create cubemaps from equirectangular hdr, preprocess everything and
        // do all the rendering for the environment.
        auto step_cubemap = std::make_shared<cubemap_step>();
        step_cubemap->create();
        m_pipeline_steps[mango::render_step::cubemap] = std::static_pointer_cast<pipeline_step>(step_cubemap);
    }
    if (configuration.get_render_steps()[mango::render_step::shadow_map])
    {
        auto step_shadow_map = std::make_shared<shadow_map_step>();
        step_shadow_map->create();
        m_pipeline_steps[mango::render_step::shadow_map] = std::static_pointer_cast<pipeline_step>(step_shadow_map);
    }
    if (configuration.get_render_steps()[mango::render_step::fxaa])
    {
        auto step_fxaa = std::make_shared<fxaa_step>();
        step_fxaa->create();
        m_pipeline_steps[mango::render_step::fxaa] = std::static_pointer_cast<pipeline_step>(step_fxaa);
    }
}

void deferred_pbr_render_system::setup_cubemap_step(const cubemap_step_configuration& configuration)
{
    if (m_pipeline_steps[mango::render_step::cubemap])
    {
        auto step = std::static_pointer_cast<cubemap_step>(m_pipeline_steps[mango::render_step::cubemap]);
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

void deferred_pbr_render_system::setup_fxaa_step(const fxaa_step_configuration& configuration)
{
    if (m_pipeline_steps[mango::render_step::fxaa])
    {
        auto step = std::static_pointer_cast<fxaa_step>(m_pipeline_steps[mango::render_step::fxaa]);
        step->configure(configuration);
    }
}

void deferred_pbr_render_system::begin_render()
{
    PROFILE_ZONE;
    m_active_model.material_id            = 0;
    m_renderer_info.last_frame.draw_calls = 0;
    m_renderer_info.last_frame.vertices   = 0;
    m_renderer_info.last_frame.triangles  = 0;
    m_renderer_info.last_frame.meshes     = 0;
    m_renderer_info.last_frame.primitives = 0;
    m_renderer_info.last_frame.materials  = 0;

    clear_framebuffers();
    setup_gbuffer_pass();
    if (m_lighting_pass_commands->dirty())
        setup_lighting_pass();

    // Composite and Light
    m_renderer_info.last_frame.draw_calls += 2;
    m_renderer_info.last_frame.vertices += 6;
    m_renderer_info.last_frame.triangles += 2;
    // Cubemap
}

void deferred_pbr_render_system::clear_framebuffers()
{
    if (!m_begin_render_commands->dirty())
        return;
    // TODO Paul: We may should not be clear the default framebuffer here...
    clear_framebuffer_command* cf = m_begin_render_commands->create<clear_framebuffer_command>(command_keys::no_sort);
    cf->framebuffer_name          = 0; // default framebuffer
    cf->buffer_mask               = clear_buffer_mask::color_and_depth;
    cf->fb_attachment_mask        = attachment_mask::draw_buffer0 | attachment_mask::depth_buffer;
    cf->r = cf->g = cf->b = 0.1f;
    cf->a = cf->depth = 1.0f;
    cf->stencil       = 1;

    cf                     = m_begin_render_commands->create<clear_framebuffer_command>(command_keys::no_sort);
    cf->framebuffer_name   = m_gbuffer->get_name();
    cf->buffer_mask        = clear_buffer_mask::color_and_depth;
    cf->fb_attachment_mask = attachment_mask::all_draw_buffers_and_depth;
    cf->r = cf->g = cf->b = 0.0f;
    cf->a = cf->depth = 1.0f;

    cf                     = m_begin_render_commands->create<clear_framebuffer_command>(command_keys::no_sort);
    cf->framebuffer_name   = m_hdr_buffer->get_name();
    cf->buffer_mask        = clear_buffer_mask::color_and_depth;
    cf->fb_attachment_mask = attachment_mask::draw_buffer0 | attachment_mask::depth_buffer;
    cf->r = cf->g = cf->b = 30000.0f;
    cf->a = cf->depth = 1.0f;

    cf                     = m_begin_render_commands->create<clear_framebuffer_command>(command_keys::no_sort);
    cf->framebuffer_name   = m_backbuffer->get_name();
    cf->buffer_mask        = clear_buffer_mask::color_and_depth;
    cf->fb_attachment_mask = attachment_mask::draw_buffer0 | attachment_mask::depth_buffer;
    cf->r = cf->g = cf->b = 0.0f;
    cf->a = cf->depth = 1.0f;

    cf                     = m_begin_render_commands->create<clear_framebuffer_command>(command_keys::no_sort);
    cf->framebuffer_name   = m_post_buffer->get_name();
    cf->buffer_mask        = clear_buffer_mask::color_and_depth;
    cf->fb_attachment_mask = attachment_mask::draw_buffer0 | attachment_mask::depth_buffer;
    cf->r = cf->g = cf->b = 0.0f;
    cf->a = cf->depth = 1.0f;

    if (m_pipeline_steps[mango::render_step::shadow_map])
    {
        auto step                     = std::static_pointer_cast<shadow_map_step>(m_pipeline_steps[mango::render_step::shadow_map]);
        framebuffer_ptr shadow_buffer = step->get_shadow_buffer();
        cf                            = m_begin_render_commands->create<clear_framebuffer_command>(command_keys::no_sort);
        cf->framebuffer_name          = shadow_buffer->get_name();
        cf->buffer_mask               = clear_buffer_mask::depth_buffer;
        cf->fb_attachment_mask        = attachment_mask::depth_buffer;
        cf->depth                     = 1.0f;
    }
}

void deferred_pbr_render_system::setup_gbuffer_pass()
{
    max_key k = command_keys::create_key<max_key>(command_keys::key_template::max_key_material_front_to_back);
    command_keys::add_base_mode(k, command_keys::base_mode::to_front);
    set_depth_test_command* sdt      = m_gbuffer_commands->create<set_depth_test_command>(k);
    sdt->enabled                     = true;
    set_depth_func_command* sdf      = m_gbuffer_commands->append<set_depth_func_command, set_depth_test_command>(sdt);
    sdf->operation                   = compare_operation::less;
    set_cull_face_command* scf       = m_gbuffer_commands->append<set_cull_face_command, set_depth_func_command>(sdf);
    scf->face                        = polygon_face::face_back;
    set_polygon_offset_command* spo  = m_gbuffer_commands->append<set_polygon_offset_command, set_cull_face_command>(scf);
    spo->factor                      = 0.0f;
    spo->units                       = 0.0f;
    bind_framebuffer_command* bf     = m_gbuffer_commands->append<bind_framebuffer_command, set_polygon_offset_command>(spo);
    bf->framebuffer_name             = m_gbuffer->get_name();
    bind_shader_program_command* bsp = m_gbuffer_commands->append<bind_shader_program_command, bind_framebuffer_command>(bf);
    bsp->shader_program_name         = m_scene_geometry_pass->get_name();
    set_viewport_command* sv         = m_gbuffer_commands->append<set_viewport_command, bind_shader_program_command>(bsp);
    sv->x                            = m_renderer_info.canvas.x;
    sv->y                            = m_renderer_info.canvas.y;
    sv->width                        = m_renderer_info.canvas.width;
    sv->height                       = m_renderer_info.canvas.height;
    set_blending_command* bl         = m_gbuffer_commands->append<set_blending_command, set_viewport_command>(sv);
    bl->enabled                      = false;
    if (m_wireframe)
    {
        set_polygon_mode_command* spm = m_gbuffer_commands->append<set_polygon_mode_command, set_blending_command>(bl);
        spm->face                     = polygon_face::face_front_and_back;
        spm->mode                     = polygon_mode::line;
    }
}

void deferred_pbr_render_system::setup_lighting_pass()
{
    bind_framebuffer_command* bf     = m_lighting_pass_commands->create<bind_framebuffer_command>(command_keys::no_sort);
    bf->framebuffer_name             = m_hdr_buffer->get_name(); // lighting goes into hdr buffer.
    bind_shader_program_command* bsp = m_lighting_pass_commands->create<bind_shader_program_command>(command_keys::no_sort);
    bsp->shader_program_name         = m_lighting_pass->get_name();
    set_polygon_mode_command* spm    = m_lighting_pass_commands->create<set_polygon_mode_command>(command_keys::no_sort);
    spm->face                        = polygon_face::face_front_and_back;
    spm->mode                        = polygon_mode::fill;
    bind_texture_command* bt         = m_lighting_pass_commands->create<bind_texture_command>(command_keys::no_sort);
    bt->binding                      = 0;
    bt->sampler_location             = 0;
    bt->texture_name                 = m_gbuffer->get_attachment(framebuffer_attachment::color_attachment0)->get_name();
    bt                               = m_lighting_pass_commands->create<bind_texture_command>(command_keys::no_sort);
    bt->binding                      = 1;
    bt->sampler_location             = 1;
    bt->texture_name                 = m_gbuffer->get_attachment(framebuffer_attachment::color_attachment1)->get_name();
    bt                               = m_lighting_pass_commands->create<bind_texture_command>(command_keys::no_sort);
    bt->binding                      = 2;
    bt->sampler_location             = 2;
    bt->texture_name                 = m_gbuffer->get_attachment(framebuffer_attachment::color_attachment2)->get_name();
    bt                               = m_lighting_pass_commands->create<bind_texture_command>(command_keys::no_sort);
    bt->binding                      = 3;
    bt->sampler_location             = 3;
    bt->texture_name                 = m_gbuffer->get_attachment(framebuffer_attachment::color_attachment3)->get_name();
    bt                               = m_lighting_pass_commands->create<bind_texture_command>(command_keys::no_sort);
    bt->binding                      = 4;
    bt->sampler_location             = 4;
    bt->texture_name                 = m_gbuffer->get_attachment(framebuffer_attachment::depth_attachment)->get_name();
}

void deferred_pbr_render_system::setup_transparent_pass()
{
    max_key k = command_keys::create_key<max_key>(command_keys::key_template::max_key_back_to_front);
    command_keys::add_base_mode(k, command_keys::base_mode::to_front);
    set_depth_test_command* sdt      = m_transparent_commands->create<set_depth_test_command>(k);
    sdt->enabled                     = true;
    set_depth_write_command* sdw     = m_transparent_commands->append<set_depth_write_command, set_depth_test_command>(sdt);
    sdw->enabled                     = true;
    set_polygon_offset_command* spo  = m_transparent_commands->append<set_polygon_offset_command, set_depth_write_command>(sdw);
    spo->factor                      = 0.0f;
    spo->units                       = 0.0f;
    bind_framebuffer_command* bf     = m_transparent_commands->append<bind_framebuffer_command, set_polygon_offset_command>(spo);
    bf->framebuffer_name             = m_hdr_buffer->get_name(); // transparent lighting goes into hdr buffer.
    bind_shader_program_command* bsp = m_transparent_commands->append<bind_shader_program_command, bind_framebuffer_command>(bf);
    bsp->shader_program_name         = m_transparent_pass->get_name();
    set_viewport_command* sv         = m_transparent_commands->append<set_viewport_command, bind_shader_program_command>(bsp);
    sv->x                            = m_renderer_info.canvas.x;
    sv->y                            = m_renderer_info.canvas.y;
    sv->width                        = m_renderer_info.canvas.width;
    sv->height                       = m_renderer_info.canvas.height;
    set_blending_command* bl         = m_transparent_commands->append<set_blending_command, set_viewport_command>(sv);
    bl->enabled                      = true;
    set_blend_factors_command* blf   = m_transparent_commands->append<set_blend_factors_command, set_blending_command>(bl);
    blf->source                      = blend_factor::one;
    blf->destination                 = blend_factor::one_minus_src_alpha;

    g_uint irradiance_map_name       = default_cube_texture->get_name();
    g_uint prefiltered_specular_name = default_cube_texture->get_name();
    g_uint brdf_lookup_name          = default_texture->get_name();
    g_uint shadow_map_name           = default_texture_array->get_name();

    auto step_shadow_map = std::static_pointer_cast<shadow_map_step>(m_pipeline_steps[mango::render_step::shadow_map]);

    auto irradiance_map = m_light_stack.get_skylight_irradiance_map();
    if (irradiance_map)
    {
        irradiance_map_name       = irradiance_map->get_name();
        prefiltered_specular_name = m_light_stack.get_skylight_specular_prefilter_map()->get_name();
        brdf_lookup_name          = m_light_stack.get_skylight_brdf_lookup()->get_name();
    }

    if (step_shadow_map)
    {
        shadow_map_name = step_shadow_map->get_shadow_buffer()->get_attachment(framebuffer_attachment::depth_attachment)->get_name();
    }
    bind_texture_command* bt = m_transparent_commands->append<bind_texture_command, set_blend_factors_command>(blf);
    bt->binding              = 5;
    bt->sampler_location     = 5;
    bt->texture_name         = irradiance_map_name;
    bt                       = m_transparent_commands->append<bind_texture_command, bind_texture_command>(bt);
    bt->binding              = 6;
    bt->sampler_location     = 6;
    bt->texture_name         = prefiltered_specular_name;
    bt                       = m_transparent_commands->append<bind_texture_command, bind_texture_command>(bt);
    bt->binding              = 7;
    bt->sampler_location     = 7;
    bt->texture_name         = brdf_lookup_name;
    bt                       = m_transparent_commands->append<bind_texture_command, bind_texture_command>(bt);
    bt->binding              = 8;
    bt->sampler_location     = 8;
    bt->texture_name         = shadow_map_name;
    if (m_wireframe)
    {
        set_polygon_mode_command* spm = m_transparent_commands->append<set_polygon_mode_command, bind_texture_command>(bt);
        spm->face                     = polygon_face::face_front_and_back;
        spm->mode                     = polygon_mode::line;
    }
}

void deferred_pbr_render_system::finish_render(float dt)
{
    PROFILE_ZONE;
    auto scene           = m_shared_context->get_current_scene();
    auto camera          = scene->get_active_camera_data();
    auto step_shadow_map = std::static_pointer_cast<shadow_map_step>(m_pipeline_steps[mango::render_step::shadow_map]);
    auto step_cubemap    = std::static_pointer_cast<cubemap_step>(m_pipeline_steps[mango::render_step::cubemap]);
    auto step_fxaa       = std::static_pointer_cast<fxaa_step>(m_pipeline_steps[mango::render_step::fxaa]);
    command_buffer_ptr<max_key> shadow_command_buffer;
    command_buffer_ptr<min_key> cubemap_command_buffer;
    command_buffer_ptr<min_key> fxaa_command_buffer;
    if (step_shadow_map)
        shadow_command_buffer = step_shadow_map->get_shadow_commands();
    if (step_cubemap)
        cubemap_command_buffer = step_cubemap->get_cubemap_commands();
    if (step_fxaa)
        fxaa_command_buffer = step_fxaa->get_fxaa_commands();

    if (camera.active_camera_entity == invalid_entity)
    {
        static float fps_lock = 0.0f;
        fps_lock += dt;
        if (fps_lock >= 1.0f)
        {
            fps_lock -= 1.0f;
            MANGO_LOG_WARN("No active Camera. Can not render!");
        }
        m_begin_render_commands->execute();
        m_global_binding_commands->invalidate();
        if (shadow_command_buffer)
        {
            shadow_command_buffer->invalidate();
        }
        m_gbuffer_commands->invalidate();
        if (cubemap_command_buffer)
        {
            cubemap_command_buffer->invalidate();
        }
        if (fxaa_command_buffer)
        {
            fxaa_command_buffer->invalidate();
        }
        m_transparent_commands->invalidate();
        m_exposure_commands->invalidate();

        g_sync* frame_sync_prepare    = m_frame_uniform_buffer->prepare();
        client_wait_sync_command* cws = m_finish_render_commands->create<client_wait_sync_command>(command_keys::no_sort);
        cws->sync                     = frame_sync_prepare;
        // TODO Paul: Is there a better way?
        g_sync* frame_sync_end = m_frame_uniform_buffer->end_frame();
        fence_sync_command* fs = m_finish_render_commands->create<fence_sync_command>(command_keys::no_sort);
        fs->sync               = frame_sync_end;

        m_finish_render_commands->create<end_frame_command>(command_keys::no_sort);
        m_finish_render_commands->execute();
        m_finish_render_commands->invalidate();

        return;
    }

    static float camera_exposure = 1.0f; // TODO Paul: Does the static variable here make sense?
    if (camera.camera_info && !m_lighting_pass_data.debug_view_enabled)
        camera_exposure = apply_exposure(camera); // with last frames data.

    // Bind the renderer uniform buffer.
    bind_renderer_data_buffer(camera, camera_exposure);
    // Bind lighting pass uniform buffer.
    bind_lighting_pass_buffer(camera);
    // Bind light buffer.
    m_light_stack.bind_light_buffers(m_global_binding_commands, m_frame_uniform_buffer);

    // Shadow step execute.
    if (step_shadow_map)
    {
        auto shadow_casters = m_light_stack.get_shadow_casters(); // currently only directional.
        if (!m_lighting_pass_data.debug_view_enabled && camera.camera_info && !shadow_casters.empty())
        {
            for (auto sc : shadow_casters)
            {
                step_shadow_map->update_cascades(dt, camera.camera_info->z_near, camera.camera_info->z_far, camera.camera_info->view_projection, sc->direction);
                // render shadow maps
                step_shadow_map->execute(m_frame_uniform_buffer);
            }
        }
        else
            shadow_command_buffer->invalidate();
    }

    if (m_lighting_pass_commands->dirty())
        finalize_lighting_pass(step_shadow_map);
    setup_transparent_pass();

    // Cubemap execute ->  drawing.
    if (step_cubemap && !m_lighting_pass_data.debug_view_enabled)
    {
        // Cubemap
        m_renderer_info.last_frame.draw_calls += 1;
        m_renderer_info.last_frame.vertices += 18;
        m_renderer_info.last_frame.triangles += 6;
        step_cubemap->set_cubemap(m_light_stack.get_skylight_specular_prefilter_map());
        step_cubemap->execute(m_frame_uniform_buffer);
    }

    // Auto exposure compute shaders.
    if (camera.camera_info && camera.camera_info->physical.adaptive_exposure && !m_lighting_pass_data.debug_view_enabled)
        calculate_auto_exposure(dt);

    bool fxaa_enabled = step_fxaa != nullptr;
    if (m_composite_commands->dirty())
        composite_pass(fxaa_enabled);

    if (step_fxaa)
    {
        step_fxaa->set_input_texture(m_post_buffer->get_attachment(framebuffer_attachment::color_attachment0));
        step_fxaa->set_output_framebuffer(m_backbuffer);
        step_fxaa->execute(m_frame_uniform_buffer);
    }

    end_frame_and_sync();

    // Execute commands.
    execute_commands(cubemap_command_buffer, shadow_command_buffer, fxaa_command_buffer);
}

void deferred_pbr_render_system::finalize_lighting_pass(const std::shared_ptr<shadow_map_step>& step_shadow_map)
{
    g_uint irradiance_map_name       = default_cube_texture->get_name();
    g_uint prefiltered_specular_name = default_cube_texture->get_name();
    g_uint brdf_lookup_name          = default_texture->get_name();
    g_uint shadow_map_name           = default_texture_array->get_name();

    auto irradiance_map = m_light_stack.get_skylight_irradiance_map();
    if (irradiance_map)
    {
        irradiance_map_name       = irradiance_map->get_name();
        prefiltered_specular_name = m_light_stack.get_skylight_specular_prefilter_map()->get_name();
        brdf_lookup_name          = m_light_stack.get_skylight_brdf_lookup()->get_name();
    }

    if (step_shadow_map)
    {
        shadow_map_name = step_shadow_map->get_shadow_buffer()->get_attachment(framebuffer_attachment::depth_attachment)->get_name();
    }
    bind_texture_command* bt = m_lighting_pass_commands->create<bind_texture_command>(command_keys::no_sort);
    bt->binding              = 5;
    bt->sampler_location     = 5;
    bt->texture_name         = irradiance_map_name;
    bt                       = m_lighting_pass_commands->create<bind_texture_command>(command_keys::no_sort);
    bt->binding              = 6;
    bt->sampler_location     = 6;
    bt->texture_name         = prefiltered_specular_name;
    bt                       = m_lighting_pass_commands->create<bind_texture_command>(command_keys::no_sort);
    bt->binding              = 7;
    bt->sampler_location     = 7;
    bt->texture_name         = brdf_lookup_name;
    bt                       = m_lighting_pass_commands->create<bind_texture_command>(command_keys::no_sort);
    bt->binding              = 8;
    bt->sampler_location     = 8;
    bt->texture_name         = shadow_map_name;

    // TODO Paul: Check if the binding is better for performance or not.
    bind_vertex_array_command* bva = m_lighting_pass_commands->create<bind_vertex_array_command>(command_keys::no_sort);
    bva->vertex_array_name         = default_vao->get_name();

    draw_arrays_command* da = m_lighting_pass_commands->create<draw_arrays_command>(command_keys::no_sort);
    da->topology            = primitive_topology::triangles;
    da->first               = 0;
    da->count               = 3;
    da->instance_count      = 1;
}

void deferred_pbr_render_system::calculate_auto_exposure(float dt)
{
    bind_shader_program_command* bsp = m_exposure_commands->create<bind_shader_program_command>(command_keys::no_sort);
    bsp->shader_program_name         = m_construct_luminance_buffer->get_name();

    texture_ptr hdr_result        = m_hdr_buffer->get_attachment(framebuffer_attachment::color_attachment0);
    calculate_mipmaps_command* cm = m_exposure_commands->create<calculate_mipmaps_command>(command_keys::no_sort);
    cm->texture_name              = hdr_result->get_name();

    add_memory_barrier_command* amb = m_exposure_commands->create<add_memory_barrier_command>(command_keys::no_sort);
    amb->barrier_bit                = memory_barrier_bit::shader_image_access_barrier_bit;

    int32 mip_level = 0;
    int32 hr_width  = hdr_result->get_width();
    int32 hr_height = hdr_result->get_height();
    while (hr_width >> mip_level > 512 && hr_height >> mip_level > 512) // we can make it smaller, when we have some better focussing.
    {
        ++mip_level;
    }
    hr_width >>= mip_level;
    hr_height >>= mip_level;

    bind_image_texture_command* bit = m_exposure_commands->create<bind_image_texture_command>(command_keys::no_sort);
    bit->binding                    = 0;
    bit->texture_name               = hdr_result->get_name();
    bit->level                      = mip_level;
    bit->layered                    = false;
    bit->layer                      = 0;
    bit->access                     = base_access::read_only;
    bit->element_format             = format::rgba32f;

    bind_buffer_command* bb = m_exposure_commands->create<bind_buffer_command>(command_keys::no_sort);
    bb->index               = SSB_SLOT_EXPOSURE;
    bb->buffer_name         = m_luminance_histogram_buffer->get_name();
    bb->offset              = 0;
    bb->target              = buffer_target::shader_storage_buffer;
    bb->size                = m_luminance_histogram_buffer->byte_length();

    glm::vec2 params                 = glm::vec2(-8.0f, 1.0f / 40.0f); // min -8.0, max +32.0
    bind_single_uniform_command* bsu = m_exposure_commands->create<bind_single_uniform_command>(command_keys::no_sort, sizeof(params));
    bsu->count                       = 1;
    bsu->location                    = 1;
    bsu->type                        = shader_resource_type::fvec2;
    bsu->uniform_value               = m_exposure_commands->map_spare<bind_single_uniform_command>();
    memcpy(bsu->uniform_value, &params, sizeof(params));

    dispatch_compute_command* dc = m_exposure_commands->create<dispatch_compute_command>(command_keys::no_sort);
    dc->num_x_groups             = hr_width / 16;
    dc->num_y_groups             = hr_height / 16;
    dc->num_z_groups             = 1;

    amb              = m_exposure_commands->create<add_memory_barrier_command>(command_keys::no_sort);
    amb->barrier_bit = memory_barrier_bit::shader_storage_barrier_bit;

    bsp                      = m_exposure_commands->create<bind_shader_program_command>(command_keys::no_sort);
    bsp->shader_program_name = m_reduce_luminance_buffer->get_name();

    bb              = m_exposure_commands->create<bind_buffer_command>(command_keys::no_sort);
    bb->index       = SSB_SLOT_EXPOSURE;
    bb->buffer_name = m_luminance_histogram_buffer->get_name();
    bb->offset      = 0;
    bb->target      = buffer_target::shader_storage_buffer;
    bb->size        = m_luminance_histogram_buffer->byte_length();

    // time coefficient with tau = 1.1;
    float tau              = 0.75f;
    float time_coefficient = 1.0f - expf(-dt * tau);
    glm::vec4 red_params   = glm::vec4(time_coefficient, hr_width * hr_height, -8.0f, 40.0f); // min -8.0, max +32.0
    bsu                    = m_exposure_commands->create<bind_single_uniform_command>(command_keys::no_sort, sizeof(red_params));
    bsu->count             = 1;
    bsu->location          = 0;
    bsu->type              = shader_resource_type::fvec4;
    bsu->uniform_value     = m_exposure_commands->map_spare<bind_single_uniform_command>();
    memcpy(bsu->uniform_value, &red_params, sizeof(red_params));

    dc               = m_exposure_commands->create<dispatch_compute_command>(command_keys::no_sort);
    dc->num_x_groups = dc->num_y_groups = dc->num_z_groups = 1;

    amb              = m_exposure_commands->create<add_memory_barrier_command>(command_keys::no_sort);
    amb->barrier_bit = memory_barrier_bit::shader_image_access_barrier_bit;
}

void deferred_pbr_render_system::composite_pass(bool render_to_pp)
{
    set_depth_test_command* sdt      = m_composite_commands->create<set_depth_test_command>(command_keys::no_sort);
    sdt->enabled                     = true;
    set_depth_write_command* sdw     = m_composite_commands->create<set_depth_write_command>(command_keys::no_sort);
    sdw->enabled                     = true;
    set_depth_func_command* sdf      = m_composite_commands->create<set_depth_func_command>(command_keys::no_sort);
    sdf->operation                   = compare_operation::less;
    set_polygon_mode_command* spm    = m_composite_commands->create<set_polygon_mode_command>(command_keys::no_sort);
    spm->face                        = polygon_face::face_front_and_back;
    spm->mode                        = polygon_mode::fill;
    set_blending_command* bl         = m_composite_commands->create<set_blending_command>(command_keys::no_sort);
    bl->enabled                      = false;
    set_face_culling_command* sfc    = m_composite_commands->create<set_face_culling_command>(command_keys::no_sort);
    sfc->enabled                     = true;
    set_cull_face_command* scf       = m_composite_commands->create<set_cull_face_command>(command_keys::no_sort);
    scf->face                        = polygon_face::face_back;
    bind_framebuffer_command* bf     = m_composite_commands->create<bind_framebuffer_command>(command_keys::no_sort);
    bf->framebuffer_name             = render_to_pp ? m_post_buffer->get_name() : m_backbuffer->get_name();
    bind_shader_program_command* bsp = m_composite_commands->create<bind_shader_program_command>(command_keys::no_sort);
    bsp->shader_program_name         = m_composing_pass->get_name();

    bind_texture_command* bt = m_composite_commands->create<bind_texture_command>(command_keys::no_sort);
    bt->binding              = 0;
    bt->sampler_location     = 0;
    bt->texture_name         = m_hdr_buffer->get_attachment(framebuffer_attachment::color_attachment0)->get_name();

    // TODO Paul: Check if the binding is better for performance or not.
    bind_vertex_array_command* bva = m_composite_commands->create<bind_vertex_array_command>(command_keys::no_sort);
    bva->vertex_array_name         = default_vao->get_name();

    draw_arrays_command* da = m_composite_commands->create<draw_arrays_command>(command_keys::no_sort);
    da->topology            = primitive_topology::triangles;
    da->first               = 0;
    da->count               = 3;
    da->instance_count      = 1;
}

void deferred_pbr_render_system::end_frame_and_sync()
{
    bind_framebuffer_command* bf = m_finish_render_commands->create<bind_framebuffer_command>(command_keys::no_sort);
    bf->framebuffer_name         = 0;
#ifdef MANGO_DEBUG
    bind_vertex_array_command* bva   = m_finish_render_commands->create<bind_vertex_array_command>(command_keys::no_sort);
    bva->vertex_array_name           = 0;
    bind_shader_program_command* bsp = m_finish_render_commands->create<bind_shader_program_command>(command_keys::no_sort);
    bsp->shader_program_name         = 0;
#endif // MANGO_DEBUG

    // TODO Paul: Is there a better way?
    g_sync* frame_sync_prepare    = m_frame_uniform_buffer->prepare();
    client_wait_sync_command* cws = m_finish_render_commands->create<client_wait_sync_command>(command_keys::no_sort);
    cws->sync                     = frame_sync_prepare;
    // TODO Paul: Is there a better way?
    g_sync* frame_sync_end = m_frame_uniform_buffer->end_frame();
    fence_sync_command* fs = m_finish_render_commands->create<fence_sync_command>(command_keys::no_sort);
    fs->sync               = frame_sync_end;

    m_finish_render_commands->create<end_frame_command>(command_keys::no_sort);
}

void deferred_pbr_render_system::execute_commands(const command_buffer_ptr<min_key>& cubemap_command_buffer, const command_buffer_ptr<max_key>& shadow_command_buffer,
                                                  const command_buffer_ptr<min_key>& fxaa_command_buffer)
{
    NAMED_PROFILE_ZONE("Execute Command Buffers")
    GL_NAMED_PROFILE_ZONE("Execute Command Buffers");
    {
        NAMED_PROFILE_ZONE("Sort Command Buffers")
        // m_begin_render_commands->sort(); // They do not need to be sorted.
        // m_global_binding_commands->sort(); // They do not need to be sorted atm.
        // This has to sort the commands so that the max_key_to_start is executed before the objects get rendered (and these would be perfect from front to back).
        if (shadow_command_buffer)
            shadow_command_buffer->sort();
        // This has to sort the commands so that the max_key_to_start is executed before the objects get rendered (and these would be perfect by material and from front to back).
        m_gbuffer_commands->sort();
        // m_lighting_pass_commands->sort(); // They do not need to be sorted atm.
        // cubemap_command_buffer->sort(); // They do not need to be sorted atm.
        m_transparent_commands->sort();
        // m_exposure_commands->sort(); // They do not need to be sorted atm.
        // m_composite_commands->sort(); // They do not need to be sorted atm.
        // m_finish_render_commands->sort(); // They do not need to be sorted.
    }
    {
        NAMED_PROFILE_ZONE("Deferred Renderer Begin")
        GL_NAMED_PROFILE_ZONE("Deferred Renderer Begin");
        m_begin_render_commands->execute();
    }
    {
        NAMED_PROFILE_ZONE("Global Bindings Commands Execute")
        GL_NAMED_PROFILE_ZONE("Global Bindings Execute");
        m_global_binding_commands->execute();
        m_global_binding_commands->invalidate();
    }
    if (shadow_command_buffer)
    {
        NAMED_PROFILE_ZONE("Shadow Commands Execute")
        GL_NAMED_PROFILE_ZONE("Shadow Commands Execute");
        shadow_command_buffer->execute();
        shadow_command_buffer->invalidate();
    }
    {
        NAMED_PROFILE_ZONE("GBuffer Commands Execute")
        GL_NAMED_PROFILE_ZONE("GBuffer Commands Execute");
        m_gbuffer_commands->execute();
        m_gbuffer_commands->invalidate();
    }
    {
        NAMED_PROFILE_ZONE("Lighting Commands Execute")
        GL_NAMED_PROFILE_ZONE("Lighting Commands Execute");
        m_lighting_pass_commands->execute();
        if (m_light_stack.lighting_dirty())
            m_lighting_pass_commands->invalidate();
    }
    if (cubemap_command_buffer)
    {
        NAMED_PROFILE_ZONE("Cubemap Commands Execute")
        GL_NAMED_PROFILE_ZONE("Cubemap Commands Execute");
        cubemap_command_buffer->execute();
        cubemap_command_buffer->invalidate();
    }
    {
        NAMED_PROFILE_ZONE("Transparent Commands Execute")
        GL_NAMED_PROFILE_ZONE("Transparent Commands Execute");
        m_transparent_commands->execute();
        m_transparent_commands->invalidate();
    }
    {
        NAMED_PROFILE_ZONE("Exposure Commands Execute")
        GL_NAMED_PROFILE_ZONE("Exposure Commands Execute");
        m_exposure_commands->execute();
        m_exposure_commands->invalidate();
    }
    {
        NAMED_PROFILE_ZONE("Composite Commands Execute")
        GL_NAMED_PROFILE_ZONE("Composite Commands Execute");
        m_composite_commands->execute();
        // m_composite_commands->invalidate();
    }
    if (fxaa_command_buffer)
    {
        NAMED_PROFILE_ZONE("FXAA Commands Execute")
        GL_NAMED_PROFILE_ZONE("FXAA Commands Execute");
        fxaa_command_buffer->execute();
    }
    {
        NAMED_PROFILE_ZONE("Deferred Renderer Finish")
        GL_NAMED_PROFILE_ZONE("Deferred Renderer Finish");
        m_finish_render_commands->execute();
        m_finish_render_commands->invalidate();
    }
}

void deferred_pbr_render_system::set_viewport(int32 x, int32 y, int32 width, int32 height)
{
    PROFILE_ZONE;
    MANGO_ASSERT(x >= 0, "Viewport x position has to be positive!");
    MANGO_ASSERT(y >= 0, "Viewport y position has to be positive!");
    MANGO_ASSERT(width >= 0, "Viewport width has to be positive!");
    MANGO_ASSERT(height >= 0, "Viewport height has to be positive!");
    m_gbuffer->resize(width, height);
    m_backbuffer->resize(width, height);
    m_hdr_buffer->resize(width, height);
    m_post_buffer->resize(width, height);
    m_begin_render_commands->invalidate();

    m_renderer_info.canvas.x      = x;
    m_renderer_info.canvas.y      = y;
    m_renderer_info.canvas.width  = width;
    m_renderer_info.canvas.height = height;
}

void deferred_pbr_render_system::update(float dt)
{
    MANGO_UNUSED(dt);
    m_light_stack.update();
}

void deferred_pbr_render_system::destroy() {}

render_pipeline deferred_pbr_render_system::get_base_render_pipeline()
{
    return render_pipeline::deferred_pbr;
}

void deferred_pbr_render_system::begin_mesh(const glm::mat4& model_matrix, bool has_normals, bool has_tangents)
{
    PROFILE_ZONE;

    model_data d{ model_matrix, std140_mat3(glm::mat3(glm::transpose(glm::inverse(model_matrix)))), has_normals, has_tangents, 0, 0 };

    int64 offset                     = m_frame_uniform_buffer->write_data(sizeof(d), &d);
    m_active_model.model_data_offset = offset;
    m_active_model.position          = glm::vec3(model_matrix[3]);
}

void deferred_pbr_render_system::end_mesh()
{
    m_active_model.model_data_offset    = -1;
    m_active_model.material_data_offset = -1;
    m_renderer_info.last_frame.meshes++;
}

void deferred_pbr_render_system::use_material(const material_ptr& mat)
{
    PROFILE_ZONE;

    auto m = mat;

    if (!mat)
    {
        // use default
        m = default_material;
    }

    command_buffer_ptr<max_key> shadow_command_buffer;
    if (m_pipeline_steps[mango::render_step::shadow_map])
    {
        shadow_command_buffer = std::static_pointer_cast<shadow_map_step>(m_pipeline_steps[mango::render_step::shadow_map])->get_shadow_commands();
    }

    material_data d;

    d.base_color     = static_cast<glm::vec4>(m->base_color);
    d.emissive_color = static_cast<glm::vec3>(m->emissive_color);
    d.metallic       = (g_float)m->metallic;
    d.roughness      = (g_float)m->roughness;
    if (m->use_base_color_texture)
    {
        d.base_color_texture                   = true;
        m_active_model.base_color_texture_name = m->base_color_texture->get_name();
    }
    else
    {
        d.base_color_texture                   = false;
        m_active_model.base_color_texture_name = default_texture->get_name();
    }
    if (m->use_roughness_metallic_texture)
    {
        d.roughness_metallic_texture                   = true;
        m_active_model.roughness_metallic_texture_name = m->roughness_metallic_texture->get_name();
    }
    else
    {
        d.roughness_metallic_texture                   = false;
        m_active_model.roughness_metallic_texture_name = default_texture->get_name();
    }
    if (m->use_occlusion_texture)
    {
        d.occlusion_texture                   = true;
        m_active_model.occlusion_texture_name = m->occlusion_texture->get_name();
        d.packed_occlusion                    = false;
    }
    else
    {
        d.occlusion_texture                   = false;
        m_active_model.occlusion_texture_name = default_texture->get_name();
        // eventually it is packed
        d.packed_occlusion = m->packed_occlusion && m->use_packed_occlusion;
    }
    if (m->use_normal_texture)
    {
        d.normal_texture                   = true;
        m_active_model.normal_texture_name = m->normal_texture->get_name();
    }
    else
    {
        d.normal_texture                   = false;
        m_active_model.normal_texture_name = default_texture->get_name();
    }
    if (m->use_emissive_color_texture)
    {
        d.emissive_color_texture                   = true;
        m_active_model.emissive_color_texture_name = m->emissive_color_texture->get_name();
    }
    else
    {
        d.emissive_color_texture                   = false;
        m_active_model.emissive_color_texture_name = default_texture->get_name();
    }

    d.alpha_mode   = static_cast<g_int>(m->alpha_rendering);
    d.alpha_cutoff = static_cast<g_float>(m->alpha_cutoff);

    m_active_model.blend        = m->alpha_rendering == alpha_mode::mode_blend;
    m_active_model.face_culling = !m->double_sided;

    int64 offset                        = m_frame_uniform_buffer->write_data(sizeof(d), &d);
    m_active_model.material_data_offset = offset;

    m_active_model.material_id = m_active_model.create_material_id(d);
}

void deferred_pbr_render_system::draw_mesh(const vertex_array_ptr& vertex_array, primitive_topology topology, int32 first, int32 count, index_type type, int32 instance_count)
{
    PROFILE_ZONE;

    MANGO_ASSERT(first >= 0, "The first index has to be greater than 0!");
    MANGO_ASSERT(count >= 0, "The index count has to be greater than 0!");
    MANGO_ASSERT(instance_count >= 0, "The instance count has to be greater than 0!");

    command_buffer_ptr<max_key> shadow_command_buffer;
    if (m_pipeline_steps[mango::render_step::shadow_map])
    {
        shadow_command_buffer = std::static_pointer_cast<shadow_map_step>(m_pipeline_steps[mango::render_step::shadow_map])->get_shadow_commands();
    }

    if (!m_active_model.valid())
        return;

    auto scene  = m_shared_context->get_current_scene();
    auto camera = scene->get_active_camera_data();
    if (camera.active_camera_entity == invalid_entity)
        return;

    if (m_active_model.blend)
    {
        max_key k      = command_keys::create_key<max_key>(command_keys::key_template::max_key_back_to_front);
        float distance = glm::distance(m_active_model.position, camera.transform->position);
        float depth    = glm::clamp(distance / (camera.camera_info->z_far - camera.camera_info->z_near), 0.0f, 1.0f); // TODO Paul: Do the correct calculation...
        command_keys::add_depth(k, 1.0f - depth, command_keys::key_template::max_key_back_to_front);

        // transparent rendering

        bind_texture_command* bt = begin_mesh_draw(m_transparent_commands, k);

        set_face_culling_command* sfc = m_transparent_commands->append<set_face_culling_command, bind_texture_command>(bt);
        sfc->enabled                  = m_active_model.face_culling;

        set_cull_face_command* scf = m_transparent_commands->append<set_cull_face_command, set_face_culling_command>(sfc);
        scf->face                  = m_active_model.face_culling ? polygon_face::face_front : polygon_face::face_back;

        bind_vertex_array_command* bva = m_transparent_commands->append<bind_vertex_array_command, set_cull_face_command>(scf);
        bva->vertex_array_name         = vertex_array->get_name();

        if (type == index_type::none)
        {
            draw_arrays_command* da = m_transparent_commands->append<draw_arrays_command, bind_vertex_array_command>(bva);
            da->topology            = topology;
            da->first               = first;
            da->count               = count;
            da->instance_count      = instance_count;
            m_renderer_info.last_frame.draw_calls++;
            m_renderer_info.last_frame.vertices += (instance_count * count);
            m_renderer_info.last_frame.triangles += (instance_count * count / 3);
            m_renderer_info.last_frame.primitives++;
            m_renderer_info.last_frame.materials++;

            if (m_active_model.face_culling)
            {
                scf       = m_transparent_commands->append<set_cull_face_command, draw_arrays_command>(da);
                scf->face = polygon_face::face_back;

                da                 = m_transparent_commands->append<draw_arrays_command, set_cull_face_command>(scf);
                da->topology       = topology;
                da->first          = first;
                da->count          = count;
                da->instance_count = instance_count;
                m_renderer_info.last_frame.draw_calls++;
                m_renderer_info.last_frame.vertices += (instance_count * count);
                m_renderer_info.last_frame.triangles += (instance_count * count / 3);
            }
#ifdef MANGO_DEBUG
            bva                    = m_transparent_commands->append<bind_vertex_array_command, draw_arrays_command>(da);
            bva->vertex_array_name = 0;
#endif // MANGO_DEBUG
        }
        else
        {
            draw_elements_command* de = m_transparent_commands->append<draw_elements_command, bind_vertex_array_command>(bva);
            de->topology              = topology;
            de->first                 = first;
            de->count                 = count;
            de->type                  = type;
            de->instance_count        = instance_count;
            m_renderer_info.last_frame.draw_calls++;
            m_renderer_info.last_frame.vertices += (instance_count * count);
            m_renderer_info.last_frame.triangles += (instance_count * count / 3);
            m_renderer_info.last_frame.primitives++;
            m_renderer_info.last_frame.materials++;

            if (m_active_model.face_culling)
            {
                scf       = m_transparent_commands->append<set_cull_face_command, draw_elements_command>(de);
                scf->face = polygon_face::face_back;

                de                 = m_transparent_commands->append<draw_elements_command, set_cull_face_command>(scf);
                de->topology       = topology;
                de->first          = first;
                de->count          = count;
                de->type           = type;
                de->instance_count = instance_count;
                m_renderer_info.last_frame.draw_calls++;
                m_renderer_info.last_frame.vertices += (instance_count * count);
                m_renderer_info.last_frame.triangles += (instance_count * count / 3);
            }
#ifdef MANGO_DEBUG
            bva                    = m_transparent_commands->append<bind_vertex_array_command, draw_elements_command>(de);
            bva->vertex_array_name = 0;
#endif // MANGO_DEBUG
        }

#ifdef MANGO_DEBUG
        cleanup_texture_bindings(m_transparent_commands, bva);
#endif // MANGO_DEBUG
    }
    else
    {
        // Normal GBuffer rendering
        max_key k = command_keys::create_key<max_key>(command_keys::key_template::max_key_material_front_to_back);
        command_keys::add_material(k, m_active_model.material_id);
        float distance = glm::distance(m_active_model.position, camera.transform->position);
        float depth    = glm::clamp(distance / (camera.camera_info->z_far - camera.camera_info->z_near), 0.0f, 1.0f); // TODO Paul: Do the correct calculation...
        command_keys::add_depth(k, depth, command_keys::key_template::max_key_material_front_to_back);

        bind_texture_command* bt = begin_mesh_draw(m_gbuffer_commands, k);

        set_face_culling_command* sfc = m_gbuffer_commands->append<set_face_culling_command, bind_texture_command>(bt);
        sfc->enabled                  = m_active_model.face_culling;

        bind_vertex_array_command* bva = m_gbuffer_commands->append<bind_vertex_array_command, set_face_culling_command>(sfc);
        bva->vertex_array_name         = vertex_array->get_name();

        if (type == index_type::none)
        {
            draw_arrays_command* da = m_gbuffer_commands->append<draw_arrays_command, bind_vertex_array_command>(bva);
            da->topology            = topology;
            da->first               = first;
            da->count               = count;
            da->instance_count      = instance_count;
            m_renderer_info.last_frame.draw_calls++;
            m_renderer_info.last_frame.primitives++;
            m_renderer_info.last_frame.vertices += (instance_count * count);
            m_renderer_info.last_frame.triangles += (instance_count * count / 3);
            m_renderer_info.last_frame.materials++;
#ifdef MANGO_DEBUG
            bva                    = m_gbuffer_commands->append<bind_vertex_array_command, draw_arrays_command>(da);
            bva->vertex_array_name = 0;
#endif // MANGO_DEBUG
        }
        else
        {
            draw_elements_command* de = m_gbuffer_commands->append<draw_elements_command, bind_vertex_array_command>(bva);
            de->topology              = topology;
            de->first                 = first;
            de->count                 = count;
            de->type                  = type;
            de->instance_count        = instance_count;
            m_renderer_info.last_frame.draw_calls++;
            m_renderer_info.last_frame.primitives++;
            m_renderer_info.last_frame.vertices += (instance_count * count);
            m_renderer_info.last_frame.triangles += (instance_count * count / 3);
            m_renderer_info.last_frame.materials++;
#ifdef MANGO_DEBUG
            bva                    = m_gbuffer_commands->append<bind_vertex_array_command, draw_elements_command>(de);
            bva->vertex_array_name = 0;
#endif // MANGO_DEBUG
        }

#ifdef MANGO_DEBUG
        cleanup_texture_bindings(m_gbuffer_commands, bva);
#endif // MANGO_DEBUG
    }

    // Same for shadow buffer
    if (shadow_command_buffer)
    {
        max_key k = command_keys::create_key<max_key>(command_keys::key_template::max_key_material_front_to_back);
        command_keys::add_material(k, m_active_model.material_id);
        float distance = glm::distance(m_active_model.position, camera.transform->position);
        float depth    = glm::clamp(distance / (camera.camera_info->z_far - camera.camera_info->z_near), 0.0f, 1.0f); // TODO Paul: Do the correct calculation...
        command_keys::add_depth(k, depth, command_keys::key_template::max_key_material_front_to_back);

        bind_texture_command* bt = begin_mesh_draw(shadow_command_buffer, k, true);

        bind_vertex_array_command* bva = shadow_command_buffer->append<bind_vertex_array_command, bind_texture_command>(bt);
        bva->vertex_array_name         = vertex_array->get_name();

        if (type == index_type::none)
        {
            draw_arrays_command* da = shadow_command_buffer->append<draw_arrays_command, bind_vertex_array_command>(bva);
            da->topology            = topology;
            da->first               = first;
            da->count               = count;
            da->instance_count      = instance_count;
            m_renderer_info.last_frame.draw_calls++;
            m_renderer_info.last_frame.vertices += (instance_count * count);
            m_renderer_info.last_frame.triangles += (instance_count * count / 3);
#ifdef MANGO_DEBUG
            bva                    = shadow_command_buffer->append<bind_vertex_array_command, draw_arrays_command>(da);
            bva->vertex_array_name = 0;
#endif // MANGO_DEBUG
        }
        else
        {
            draw_elements_command* de = shadow_command_buffer->append<draw_elements_command, bind_vertex_array_command>(bva);
            de->topology              = topology;
            de->first                 = first;
            de->count                 = count;
            de->type                  = type;
            de->instance_count        = instance_count;
            m_renderer_info.last_frame.draw_calls++;
            m_renderer_info.last_frame.vertices += (instance_count * count);
            m_renderer_info.last_frame.triangles += (instance_count * count / 3);
#ifdef MANGO_DEBUG
            bva                    = shadow_command_buffer->append<bind_vertex_array_command, draw_elements_command>(de);
            bva->vertex_array_name = 0;
#endif // MANGO_DEBUG
        }

#ifdef MANGO_DEBUG
        cleanup_texture_bindings(shadow_command_buffer, bva);
#endif // MANGO_DEBUG
    }
}

bind_texture_command* deferred_pbr_render_system::begin_mesh_draw(const command_buffer_ptr<max_key>& draw_buffer, max_key mesh_key, bool simplified)
{
    // material data buffer
    bind_buffer_command* bb = draw_buffer->create<bind_buffer_command>(mesh_key);
    bb->target              = buffer_target::uniform_buffer;
    bb->index               = UB_SLOT_MATERIAL_DATA;
    bb->size                = sizeof(material_data);
    bb->buffer_name         = m_frame_uniform_buffer->buffer_name();
    bb->offset              = m_active_model.material_data_offset;

    // model data buffer
    bb              = draw_buffer->append<bind_buffer_command, bind_buffer_command>(bb);
    bb->target      = buffer_target::uniform_buffer;
    bb->index       = UB_SLOT_MODEL_DATA;
    bb->size        = sizeof(model_data);
    bb->buffer_name = m_frame_uniform_buffer->buffer_name();
    bb->offset      = m_active_model.model_data_offset;

    if (!simplified)
        return bind_material_textures(draw_buffer, bb);
    else
    {
        // shadow does only need base color.
        bind_texture_command* bt = draw_buffer->append<bind_texture_command, bind_buffer_command>(bb);
        bt->binding = bt->sampler_location = 0;
        bt->texture_name                   = m_active_model.base_color_texture_name;
        return bt;
    }
}

bind_texture_command* deferred_pbr_render_system::bind_material_textures(const command_buffer_ptr<max_key>& draw_buffer, bind_buffer_command* last_command)
{
    bind_texture_command* bt = draw_buffer->append<bind_texture_command, bind_buffer_command>(last_command);
    bt->binding = bt->sampler_location = 0;
    bt->texture_name                   = m_active_model.base_color_texture_name;

    bt          = draw_buffer->append<bind_texture_command, bind_texture_command>(bt);
    bt->binding = bt->sampler_location = 1;
    bt->texture_name                   = m_active_model.roughness_metallic_texture_name;

    bt          = draw_buffer->append<bind_texture_command, bind_texture_command>(bt);
    bt->binding = bt->sampler_location = 2;
    bt->texture_name                   = m_active_model.occlusion_texture_name;

    bt          = draw_buffer->append<bind_texture_command, bind_texture_command>(bt);
    bt->binding = bt->sampler_location = 3;
    bt->texture_name                   = m_active_model.normal_texture_name;

    bt          = draw_buffer->append<bind_texture_command, bind_texture_command>(bt);
    bt->binding = bt->sampler_location = 4;
    bt->texture_name                   = m_active_model.emissive_color_texture_name;

    return bt;
}

void deferred_pbr_render_system::submit_light(light_id id, mango_light* light)
{
    PROFILE_ZONE;
    m_light_stack.push(id, light);
    // if (type == light_type::directional)
    // {
    //     m_lighting_pass_data.directional.active       = true;
    //     auto directional_data                         = static_cast<directional_light_data*>(data);
    //     m_lighting_pass_data.directional.color        = static_cast<glm::vec3>(directional_data->light_color);
    //     m_lighting_pass_data.directional.cast_shadows = directional_data->cast_shadows;
    //     m_lighting_pass_data.directional.direction    = directional_data->direction;
    //     m_lighting_pass_data.directional.intensity    = directional_data->intensity;
    //
    //     return;
    // }
    // if (type == light_type::environment)
    // {
    //     auto el_data                               = static_cast<environment_light_data*>(data);
    //     m_lighting_pass_data.environment.intensity = el_data->intensity;
    //     m_lighting_pass_data.environment.active    = true;
    //
    //     if (el_data->render_sun_as_directional)
    //     {
    //         m_lighting_pass_data.directional.active       = true;
    //         m_lighting_pass_data.directional.color        = static_cast<glm::vec3>(el_data->sun_data.light_color);
    //         m_lighting_pass_data.directional.cast_shadows = el_data->sun_data.cast_shadows;
    //     }
    //
    //     auto scene = m_shared_context->get_current_scene();
    //     if (m_pipeline_steps[mango::render_step::cubemap] && (el_data->create_atmosphere || el_data->draw_sun_disc) &&
    //         (glm::vec3(m_lighting_pass_data.directional.direction) != el_data->sun_data.direction || m_lighting_pass_data.directional.intensity != el_data->sun_data.intensity))
    //     {
    //         auto cubemap = std::static_pointer_cast<cubemap_step>(m_pipeline_steps[mango::render_step::cubemap]);
    //         cubemap->create_image_based_light_data(el_data);
    //         m_sun_changed = true;
    //     }
    //     else
    //         m_sun_changed = false;
    //
    //     if (el_data->render_sun_as_directional)
    //     {
    //         m_lighting_pass_data.directional.direction = el_data->sun_data.direction;
    //         m_lighting_pass_data.directional.intensity = el_data->sun_data.intensity;
    //     }
    //
    //     return;
    // }
}

void deferred_pbr_render_system::bind_lighting_pass_buffer(camera_data& camera)
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

    bind_buffer_command* bb = m_global_binding_commands->create<bind_buffer_command>(command_keys::no_sort);
    bb->target              = buffer_target::uniform_buffer;
    bb->index               = UB_SLOT_LIGHTING_PASS_DATA;
    bb->size                = sizeof(lighting_pass_data);
    bb->buffer_name         = m_frame_uniform_buffer->buffer_name();
    bb->offset              = m_frame_uniform_buffer->write_data(sizeof(lighting_pass_data), &m_lighting_pass_data);
}

void deferred_pbr_render_system::bind_renderer_data_buffer(camera_data& camera, float camera_exposure)
{
    PROFILE_ZONE;

    if (camera.camera_info && camera.transform)
    {
        m_renderer_data.view_matrix            = camera.camera_info->view;
        m_renderer_data.projection_matrix      = camera.camera_info->projection;
        m_renderer_data.view_projection_matrix = camera.camera_info->view_projection;
        m_renderer_data.camera_exposure        = camera_exposure;
        m_renderer_data.shadow_step_enabled    = m_pipeline_steps[mango::render_step::shadow_map] != nullptr;
    }
    else
    {
        MANGO_LOG_ERROR("Renderer Data not complete! No active camera! Attempting to use last valid data!");
    }

    bind_buffer_command* bb = m_global_binding_commands->create<bind_buffer_command>(command_keys::no_sort);
    bb->target              = buffer_target::uniform_buffer;
    bb->index               = UB_SLOT_RENDERER_FRAME;
    bb->size                = sizeof(renderer_data);
    bb->buffer_name         = m_frame_uniform_buffer->buffer_name();
    bb->offset              = m_frame_uniform_buffer->write_data(sizeof(renderer_data), &m_renderer_data);
}

float deferred_pbr_render_system::apply_exposure(camera_data& camera)
{
    PROFILE_ZONE;
    float ape = mango::default_aperture;
    float shu = mango::default_shutter_speed;
    float iso = mango::default_iso;
    if (camera.camera_info->physical.adaptive_exposure)
    {
        float avg_luminance = m_luminance_data_mapping->luminance;

        // K is a light meter calibration constant
        static const float K = 12.5f;
        static const float S = 100.0f;
        float target_ev      = glm::log2(avg_luminance * S / K);

        // Compute the resulting ISO if we left both shutter and aperture here
        iso                 = glm::clamp(((ape * ape) * 100.0f) / (shu * glm::exp2(target_ev)), mango::min_iso, mango::max_iso);
        float unclamped_iso = (shu * glm::exp2(target_ev));
        MANGO_UNUSED(unclamped_iso);

        // Apply half the difference in EV to the aperture
        float ev_diff = target_ev - glm::log2(((ape * ape) * 100.0f) / (shu * iso));
        ape           = glm::clamp(ape * glm::pow(glm::sqrt(2.0f), ev_diff * 0.5f), mango::min_aperture, mango::max_aperture);

        // Apply the remaining difference to the shutter speed
        ev_diff = target_ev - glm::log2(((ape * ape) * 100.0f) / (shu * iso));
        shu     = glm::clamp(shu * glm::pow(2.0f, -ev_diff), mango::min_shutter_speed, mango::max_shutter_speed);
    }
    else
    {
        ape = camera.camera_info->physical.aperture;
        shu = camera.camera_info->physical.shutter_speed;
        iso = camera.camera_info->physical.iso;
    }

    // Adapt camera settings.
    camera.camera_info->physical.aperture      = glm::clamp(ape, min_aperture, max_aperture);
    camera.camera_info->physical.shutter_speed = glm::clamp(shu, min_shutter_speed, max_shutter_speed);
    camera.camera_info->physical.iso           = glm::clamp(iso, min_iso, max_iso);

    // Calculate the exposure from the physical camera parameters.
    float e               = ((ape * ape) * 100.0f) / (shu * iso);
    float camera_exposure = 1.0f / (1.2f * e);

    return camera_exposure;
}

void deferred_pbr_render_system::on_ui_widget()
{
    ImGui::PushID("deferred_pbr");
    const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding;
    custom_info("Renderer:", []() { ImGui::Text("Deferred PBR Render System"); });
    bool changed = checkbox("VSync", &m_vsync, true);
    if (changed)
    {
        auto ws = m_shared_context->get_window_system_internal().lock();
        MANGO_ASSERT(ws, "Window System is expired!");
        ws->set_vsync(m_vsync);
    }
    ImGui::Separator();
    bool has_cubemap    = m_pipeline_steps[mango::render_step::cubemap] != nullptr;
    bool has_shadow_map = m_pipeline_steps[mango::render_step::shadow_map] != nullptr;
    bool has_fxaa       = m_pipeline_steps[mango::render_step::fxaa] != nullptr;
    if (ImGui::TreeNodeEx("Steps", flags | ImGuiTreeNodeFlags_Framed))
    {
        bool open = ImGui::CollapsingHeader("Cubemap Step", flags | ImGuiTreeNodeFlags_AllowItemOverlap | (!has_cubemap ? ImGuiTreeNodeFlags_Leaf : 0));
        ImGui::SameLine(ImGui::GetContentRegionAvail().x);
        ImGui::PushID("enable_cubemap_step");
        bool value_changed = ImGui::Checkbox("", &has_cubemap);
        ImGui::PopID();
        if (value_changed)
        {
            m_lighting_pass_commands->invalidate();
            if (has_cubemap)
            {
                auto step_cubemap = std::make_shared<cubemap_step>();
                step_cubemap->create();
                m_pipeline_steps[mango::render_step::cubemap] = std::static_pointer_cast<pipeline_step>(step_cubemap);

                auto scene = m_shared_context->get_current_scene();
            }
            else
            {
                m_pipeline_steps[mango::render_step::cubemap] = nullptr;
            }
        }
        if (has_cubemap && open)
        {
            m_pipeline_steps[mango::render_step::cubemap]->on_ui_widget();
        }

        open = ImGui::CollapsingHeader("Shadow Step", flags | ImGuiTreeNodeFlags_AllowItemOverlap | (!has_shadow_map ? ImGuiTreeNodeFlags_Leaf : 0));
        ImGui::SameLine(ImGui::GetContentRegionAvail().x);
        ImGui::PushID("enable_shadow_step");
        value_changed = ImGui::Checkbox("", &has_shadow_map);
        ImGui::PopID();
        if (value_changed)
        {
            m_begin_render_commands->invalidate();
            m_lighting_pass_commands->invalidate();
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
        if (has_shadow_map && open)
        {
            m_pipeline_steps[mango::render_step::shadow_map]->on_ui_widget();
        }

        open = ImGui::CollapsingHeader("FXAA Step", flags | ImGuiTreeNodeFlags_AllowItemOverlap | (!has_fxaa ? ImGuiTreeNodeFlags_Leaf : 0));
        ImGui::SameLine(ImGui::GetContentRegionAvail().x);
        ImGui::PushID("enable_fxaa_step");
        value_changed = ImGui::Checkbox("", &has_fxaa);
        ImGui::PopID();
        if (value_changed)
        {
            m_composite_commands->invalidate();
            if (has_fxaa)
            {
                auto step_fxaa = std::make_shared<fxaa_step>();
                step_fxaa->create();
                m_pipeline_steps[mango::render_step::fxaa] = std::static_pointer_cast<pipeline_step>(step_fxaa);
            }
            else
            {
                m_pipeline_steps[mango::render_step::fxaa] = nullptr;
            }
        }
        if (has_fxaa && open)
        {
            m_pipeline_steps[mango::render_step::fxaa]->on_ui_widget();
        }
        ImGui::TreePop();
    }
    const char* debug[10]      = { "Default", "Position", "Normal", "Depth", "Base Color", "Reflection Color", "Emission", "Occlusion", "Roughness", "Metallic" };
    static int32 current_debug = 0;
    if (ImGui::CollapsingHeader("Debug", flags))
    {
        float occupancy = m_frame_uniform_buffer->get_occupancy();
        custom_info("Frame Uniform Buffer Occupancy:", [occupancy]() {
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%.3f%%", occupancy);
        });
        checkbox("Render Wireframe", &m_wireframe, false);

        for (int32 i = 0; i < 9; ++i)
            m_lighting_pass_data.debug_views.debug[i] = false;
        m_lighting_pass_data.debug_view_enabled = false;

        int32 idx = current_debug;
        combo("Debug Views", debug, 10, idx, 0);
        current_debug = idx;

        if (current_debug)
        {
            m_lighting_pass_data.debug_views.debug[current_debug - 1] = true;
            m_lighting_pass_data.debug_view_enabled                   = true;
        }

        ImGui::Separator();

        if (has_shadow_map)
        {
            bool casc = m_lighting_pass_data.debug_options.show_cascades;
            checkbox("Show Cascades", &casc, false);
            m_lighting_pass_data.debug_options.show_cascades = casc;

            bool sm = m_lighting_pass_data.debug_options.draw_shadow_maps;
            checkbox("Show Shadow Maps", &sm, false);
            m_lighting_pass_data.debug_options.draw_shadow_maps = sm;
        }
    }
    ImGui::PopID();
}

#ifdef MANGO_DEBUG
void deferred_pbr_render_system::cleanup_texture_bindings(const command_buffer_ptr<max_key>& draw_buffer, bind_vertex_array_command* last_command)
{
    bind_texture_command* bt = draw_buffer->append<bind_texture_command, bind_vertex_array_command>(last_command);
    bt->binding = bt->sampler_location = 0;
    bt->texture_name                   = 0;

    bt          = draw_buffer->append<bind_texture_command, bind_texture_command>(bt);
    bt->binding = bt->sampler_location = 1;
    bt->texture_name                   = 0;

    bt          = draw_buffer->append<bind_texture_command, bind_texture_command>(bt);
    bt->binding = bt->sampler_location = 2;
    bt->texture_name                   = 0;

    bt          = draw_buffer->append<bind_texture_command, bind_texture_command>(bt);
    bt->binding = bt->sampler_location = 3;
    bt->texture_name                   = 0;

    bt          = draw_buffer->append<bind_texture_command, bind_texture_command>(bt);
    bt->binding = bt->sampler_location = 4;
    bt->texture_name                   = 0;
}

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
