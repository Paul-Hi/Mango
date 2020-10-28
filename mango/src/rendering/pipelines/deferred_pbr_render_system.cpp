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

    m_begin_render_commands   = command_buffer<min_key>::create(512);
    m_global_binding_commands = command_buffer<min_key>::create(128);
    m_gbuffer_commands        = command_buffer<max_key>::create(524288); // 0.5 MiB?
    m_transparent_commands    = command_buffer<max_key>::create(524288); // 0.5 MiB?
    m_lighting_pass_commands  = command_buffer<min_key>::create(512);
    m_exposure_commands       = command_buffer<min_key>::create(512);
    m_composite_commands      = command_buffer<min_key>::create(256);
    m_finish_render_commands  = command_buffer<min_key>::create(256);

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
    m_frame_uniform_buffer = gpu_buffer::create();
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

    // transparent pass
    shader_config.m_path = "res/shader/f_scene_transparent_gltf.glsl";
    shader_config.m_type = shader_type::fragment_shader;
    d_fragment           = shader::create(shader_config);
    if (!check_creation(d_fragment.get(), "transparent pass fragment shader", "Render System"))
        return false;

    m_transparent_pass = shader_program::create_graphics_pipeline(d_vertex, nullptr, nullptr, nullptr, d_fragment);
    if (!check_creation(m_transparent_pass.get(), "transparent pass shader program", "Render System"))
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
    m_active_model.material_id             = 0;
    m_hardware_stats.last_frame.draw_calls = 0;
    m_hardware_stats.last_frame.meshes     = 0;
    m_hardware_stats.last_frame.primitives = 0;
    m_hardware_stats.last_frame.materials  = 0;

    // TODO Paul: This should not be done here, this is pretty bad!
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
    cf->r = cf->g = cf->b = 0.0f;
    cf->a = cf->depth = 1.0f;

    cf                     = m_begin_render_commands->create<clear_framebuffer_command>(command_keys::no_sort);
    cf->framebuffer_name   = m_backbuffer->get_name();
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

    // gbuffer pass setup
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
        sv->x                            = m_hardware_stats.last_frame.canvas_x;
        sv->y                            = m_hardware_stats.last_frame.canvas_y;
        sv->width                        = m_hardware_stats.last_frame.canvas_width;
        sv->height                       = m_hardware_stats.last_frame.canvas_height;
        set_blending_command* bl         = m_gbuffer_commands->append<set_blending_command, set_viewport_command>(sv);
        bl->enabled                      = false;
        if (m_wireframe)
        {
            set_polygon_mode_command* spm = m_gbuffer_commands->append<set_polygon_mode_command, set_blending_command>(bl);
            spm->face                     = polygon_face::face_front_and_back;
            spm->mode                     = polygon_mode::line;
        }
    }

    // lighting pass setup
    if (m_lighting_pass_commands->dirty())
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

    // transparent pass setup
    {
        max_key k = command_keys::create_key<max_key>(command_keys::key_template::max_key_back_to_front);
        command_keys::add_base_mode(k, command_keys::base_mode::to_front);
        set_depth_test_command* sdt      = m_transparent_commands->create<set_depth_test_command>(k);
        sdt->enabled                     = true;
        set_depth_write_command* sdw     = m_transparent_commands->append<set_depth_write_command, set_depth_test_command>(sdt);
        sdw->enabled                     = false;
        set_polygon_offset_command* spo  = m_transparent_commands->append<set_polygon_offset_command, set_depth_write_command>(sdw);
        spo->factor                      = 0.0f;
        spo->units                       = 0.0f;
        bind_framebuffer_command* bf     = m_transparent_commands->append<bind_framebuffer_command, set_polygon_offset_command>(spo);
        bf->framebuffer_name             = m_hdr_buffer->get_name(); // transparent lighting goes into hdr buffer.
        bind_shader_program_command* bsp = m_transparent_commands->append<bind_shader_program_command, bind_framebuffer_command>(bf);
        bsp->shader_program_name         = m_transparent_pass->get_name();
        set_viewport_command* sv         = m_transparent_commands->append<set_viewport_command, bind_shader_program_command>(bsp);
        sv->x                            = m_hardware_stats.last_frame.canvas_x;
        sv->y                            = m_hardware_stats.last_frame.canvas_y;
        sv->width                        = m_hardware_stats.last_frame.canvas_width;
        sv->height                       = m_hardware_stats.last_frame.canvas_height;
        set_blending_command* bl         = m_transparent_commands->append<set_blending_command, set_viewport_command>(sv);
        bl->enabled                      = true;
        set_blend_factors_command* blf   = m_transparent_commands->append<set_blend_factors_command, set_blending_command>(bl);
        blf->source                      = blend_factor::one;
        blf->destination                 = blend_factor::one_minus_src_alpha;

        g_uint irradiance_map_name       = default_texture->get_name();
        g_uint prefiltered_specular_name = default_texture->get_name();
        g_uint brdf_lookup_name          = default_texture->get_name();
        g_uint shadow_map_name           = default_texture_array->get_name();

        auto step_shadow_map = std::static_pointer_cast<shadow_map_step>(m_pipeline_steps[mango::render_step::shadow_map]);
        auto step_ibl        = std::static_pointer_cast<ibl_step>(m_pipeline_steps[mango::render_step::ibl]);

        if (step_ibl)
        {
            irradiance_map_name       = step_ibl->get_irradiance_map()->get_name();
            prefiltered_specular_name = step_ibl->get_prefiltered_specular()->get_name();
            brdf_lookup_name          = step_ibl->get_brdf_lookup()->get_name();
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
}

void deferred_pbr_render_system::finish_render(float dt)
{
    PROFILE_ZONE;
    auto scene           = m_shared_context->get_current_scene();
    auto camera          = scene->get_active_camera_data();
    auto env             = scene->get_active_environment_data();
    auto step_shadow_map = std::static_pointer_cast<shadow_map_step>(m_pipeline_steps[mango::render_step::shadow_map]);
    auto step_ibl        = std::static_pointer_cast<ibl_step>(m_pipeline_steps[mango::render_step::ibl]);
    command_buffer_ptr<max_key> shadow_command_buffer;
    command_buffer_ptr<min_key> ibl_command_buffer;
    if (step_shadow_map)
        shadow_command_buffer = step_shadow_map->get_shadow_commands();
    if (step_ibl)
        ibl_command_buffer = step_ibl->get_ibl_commands();

    if (camera.camera_info && camera.camera_info->physical.adaptive_exposure && !m_lighting_pass_data.debug_view_enabled)
        apply_auto_exposure(camera); // with last frames data.

    float camera_exposure = 1.0f;
    if (camera.camera_info)
    {
        // Calculate the exposure from the physical camera parameters.
        camera.camera_info->physical.aperture      = glm::clamp(camera.camera_info->physical.aperture, min_aperture, max_aperture);
        camera.camera_info->physical.shutter_speed = glm::clamp(camera.camera_info->physical.shutter_speed, min_shutter_speed, max_shutter_speed);
        camera.camera_info->physical.iso           = glm::clamp(camera.camera_info->physical.iso, min_iso, max_iso);
        float ape                                  = camera.camera_info->physical.aperture;
        float shu                                  = camera.camera_info->physical.shutter_speed;
        float iso                                  = camera.camera_info->physical.iso;
        float e                                    = ((ape * ape) * 100.0f) / (shu * iso);
        camera_exposure                            = 1.0f / (1.2f * e);
    }

    // Bind the renderer uniform buffer.
    bind_renderer_data_buffer(camera, camera_exposure);
    // Bind lighting pass uniform buffer.
    bind_lighting_pass_buffer(camera, env);

    if (step_shadow_map)
    {
        if (!m_lighting_pass_data.debug_view_enabled && camera.camera_info && m_lighting_pass_data.directional.cast_shadows)
        {
            step_shadow_map->update_cascades(dt, camera.camera_info->z_near, camera.camera_info->z_far, camera.camera_info->view_projection, m_lighting_pass_data.directional.direction);

            // render shadow maps
            step_shadow_map->execute(m_frame_uniform_buffer);
        }
        else
            shadow_command_buffer->invalidate();
    }

    // lighting pass
    if (m_lighting_pass_commands->dirty())
    {
        // m_lighting_pass_commands
        g_uint irradiance_map_name       = default_texture->get_name();
        g_uint prefiltered_specular_name = default_texture->get_name();
        g_uint brdf_lookup_name          = default_texture->get_name();
        g_uint shadow_map_name           = default_texture_array->get_name();

        if (step_ibl)
        {
            irradiance_map_name       = step_ibl->get_irradiance_map()->get_name();
            prefiltered_specular_name = step_ibl->get_prefiltered_specular()->get_name();
            brdf_lookup_name          = step_ibl->get_brdf_lookup()->get_name();
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

        m_hardware_stats.last_frame.draw_calls++; // TODO Paul: This measurements should be done, on glCalls.
    }

    // environment drawing
    if (step_ibl && !m_lighting_pass_data.debug_view_enabled)
    {
        step_ibl->execute(m_frame_uniform_buffer);
    }

    // auto exposure compute shaders
    if (camera.camera_info && camera.camera_info->physical.adaptive_exposure && !m_lighting_pass_data.debug_view_enabled)
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
        amb->barrier_bit = memory_barrier_bit::shader_image_access_barrier_bit;

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

    // composite
    if (m_composite_commands->dirty())
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
        bf->framebuffer_name             = m_backbuffer->get_name();
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

        m_hardware_stats.last_frame.draw_calls++; // TODO Paul: This measurements should be done, on glCalls.
    }

    bind_framebuffer_command* bf = m_finish_render_commands->create<bind_framebuffer_command>(command_keys::no_sort);
    bf->framebuffer_name         = 0;
#ifdef MANGO_DEBUG
    bind_vertex_array_command* bva   = m_finish_render_commands->create<bind_vertex_array_command>(command_keys::no_sort);
    bva->vertex_array_name           = 0;
    bind_shader_program_command* bsp = m_finish_render_commands->create<bind_shader_program_command>(command_keys::no_sort);
    bsp->shader_program_name         = 0;
#endif // MANGO_DEBUG

    // TODO Paul: Is there a better way?
    g_sync* frame_sync_prepare = m_frame_uniform_buffer->prepare();

    client_wait_sync_command* cws = m_finish_render_commands->create<client_wait_sync_command>(command_keys::no_sort);
    cws->sync                     = frame_sync_prepare;
    // TODO Paul: Is there a better way?
    g_sync* frame_sync_end = m_frame_uniform_buffer->end_frame();
    fence_sync_command* fs = m_finish_render_commands->create<fence_sync_command>(command_keys::no_sort);
    fs->sync               = frame_sync_end;

    m_finish_render_commands->create<end_frame_command>(command_keys::no_sort);

    // execute commands
    {
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
            // ibl_command_buffer->sort(); // They do not need to be sorted atm.
            m_transparent_commands->sort();
            // m_exposure_commands->sort(); // They do not need to be sorted atm.
            // m_composite_commands->sort(); // They do not need to be sorted atm.
            // m_finish_render_commands->sort(); // They do not need to be sorted.
        }
        {
            NAMED_PROFILE_ZONE("Deferred Renderer Begin")
            GL_NAMED_PROFILE_ZONE("Deferred Renderer Begin");
            m_begin_render_commands->execute();
            m_begin_render_commands->invalidate();
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
        }
        if (ibl_command_buffer)
        {
            NAMED_PROFILE_ZONE("IBL Commands Execute")
            GL_NAMED_PROFILE_ZONE("IBL Commands Execute");
            ibl_command_buffer->execute();
            ibl_command_buffer->invalidate();
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
        {
            NAMED_PROFILE_ZONE("Deferred Renderer Finish")
            GL_NAMED_PROFILE_ZONE("Deferred Renderer Finish");
            m_finish_render_commands->execute();
            m_finish_render_commands->invalidate();
        }
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

    model_data d{ model_matrix, std140_mat3(glm::mat3(glm::transpose(glm::inverse(model_matrix)))), has_normals, has_tangents, 0, 0 };

    int64 offset                     = m_frame_uniform_buffer->write_data(sizeof(d), &d);
    m_active_model.model_data_offset = offset;
    m_active_model.position          = glm::vec3(model_matrix[3]);
}

void deferred_pbr_render_system::end_mesh()
{
    m_active_model.model_data_offset    = -1;
    m_active_model.material_data_offset = -1;
    m_hardware_stats.last_frame.meshes++;
}

void deferred_pbr_render_system::use_material(const material_ptr& mat)
{
    PROFILE_ZONE;

    command_buffer_ptr<max_key> shadow_command_buffer;
    if (m_pipeline_steps[mango::render_step::shadow_map])
    {
        shadow_command_buffer = std::static_pointer_cast<shadow_map_step>(m_pipeline_steps[mango::render_step::shadow_map])->get_shadow_commands();
    }

    material_data d;

    d.base_color     = static_cast<glm::vec4>(mat->base_color);
    d.emissive_color = static_cast<glm::vec3>(mat->emissive_color);
    d.metallic       = (g_float)mat->metallic;
    d.roughness      = (g_float)mat->roughness;
    if (mat->use_base_color_texture)
    {
        d.base_color_texture                   = true;
        m_active_model.base_color_texture_name = mat->base_color_texture->get_name();
    }
    else
    {
        d.base_color_texture                   = false;
        m_active_model.base_color_texture_name = default_texture->get_name();
    }
    if (mat->use_roughness_metallic_texture)
    {
        d.roughness_metallic_texture                   = true;
        m_active_model.roughness_metallic_texture_name = mat->roughness_metallic_texture->get_name();
    }
    else
    {
        d.roughness_metallic_texture                   = false;
        m_active_model.roughness_metallic_texture_name = default_texture->get_name();
    }
    if (mat->use_occlusion_texture)
    {
        d.occlusion_texture                   = true;
        m_active_model.occlusion_texture_name = mat->occlusion_texture->get_name();
        d.packed_occlusion                    = false;
    }
    else
    {
        d.occlusion_texture                   = false;
        m_active_model.occlusion_texture_name = default_texture->get_name();
        // eventually it is packed
        d.packed_occlusion = mat->packed_occlusion && mat->use_packed_occlusion;
    }
    if (mat->use_normal_texture)
    {
        d.normal_texture                   = true;
        m_active_model.normal_texture_name = mat->normal_texture->get_name();
    }
    else
    {
        d.normal_texture                   = false;
        m_active_model.normal_texture_name = default_texture->get_name();
    }
    if (mat->use_emissive_color_texture)
    {
        d.emissive_color_texture                   = true;
        m_active_model.emissive_color_texture_name = mat->emissive_color_texture->get_name();
    }
    else
    {
        d.emissive_color_texture                   = false;
        m_active_model.emissive_color_texture_name = default_texture->get_name();
    }

    d.alpha_mode   = static_cast<g_int>(mat->alpha_rendering);
    d.alpha_cutoff = static_cast<g_float>(mat->alpha_cutoff);

    m_active_model.blend        = mat->alpha_rendering == alpha_mode::mode_blend;
    m_active_model.face_culling = !mat->double_sided;

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

    if (m_active_model.blend)
    {
        max_key k      = command_keys::create_key<max_key>(command_keys::key_template::max_key_back_to_front);
        float distance = glm::distance(m_active_model.position, camera.transform->position);
        float depth    = glm::clamp(distance / (camera.camera_info->z_far - camera.camera_info->z_near), 0.0f, 1.0f); // TODO Paul: Do the correct calculation...
        command_keys::add_depth(k, 1.0f - depth, command_keys::key_template::max_key_back_to_front);
        // transparent rendering
        // material data buffer
        bind_buffer_command* bb = m_transparent_commands->create<bind_buffer_command>(k);
        bb->target              = buffer_target::uniform_buffer;
        bb->index               = UB_SLOT_MATERIAL_DATA;
        bb->size                = sizeof(material_data);
        bb->buffer_name         = m_frame_uniform_buffer->buffer_name();
        bb->offset              = m_active_model.material_data_offset;

        // model data buffer
        bb              = m_transparent_commands->append<bind_buffer_command, bind_buffer_command>(bb);
        bb->target      = buffer_target::uniform_buffer;
        bb->index       = UB_SLOT_MODEL_DATA;
        bb->size        = sizeof(model_data);
        bb->buffer_name = m_frame_uniform_buffer->buffer_name();
        bb->offset      = m_active_model.model_data_offset;

        // material textures
        bind_texture_command* bt = m_transparent_commands->append<bind_texture_command, bind_buffer_command>(bb);
        bt->binding = bt->sampler_location = 0;
        bt->texture_name                   = m_active_model.base_color_texture_name;

        bt          = m_transparent_commands->append<bind_texture_command, bind_texture_command>(bt);
        bt->binding = bt->sampler_location = 1;
        bt->texture_name                   = m_active_model.roughness_metallic_texture_name;

        bt          = m_transparent_commands->append<bind_texture_command, bind_texture_command>(bt);
        bt->binding = bt->sampler_location = 2;
        bt->texture_name                   = m_active_model.occlusion_texture_name;

        bt          = m_transparent_commands->append<bind_texture_command, bind_texture_command>(bt);
        bt->binding = bt->sampler_location = 3;
        bt->texture_name                   = m_active_model.normal_texture_name;

        bt          = m_transparent_commands->append<bind_texture_command, bind_texture_command>(bt);
        bt->binding = bt->sampler_location = 4;
        bt->texture_name                   = m_active_model.emissive_color_texture_name;

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
            m_hardware_stats.last_frame.draw_calls++; // TODO Paul: This measurements should be done on glCalls.
            m_hardware_stats.last_frame.primitives++;
            m_hardware_stats.last_frame.materials++;

            if (m_active_model.face_culling)
            {
                scf       = m_transparent_commands->append<set_cull_face_command, draw_arrays_command>(da);
                scf->face = polygon_face::face_back;

                da                 = m_transparent_commands->append<draw_arrays_command, set_cull_face_command>(scf);
                da->topology       = topology;
                da->first          = first;
                da->count          = count;
                da->instance_count = instance_count;
                m_hardware_stats.last_frame.draw_calls++; // TODO Paul: This measurements should be done on glCalls.
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
            m_hardware_stats.last_frame.draw_calls++; // TODO Paul: This measurements should be done on glCalls.
            m_hardware_stats.last_frame.primitives++;
            m_hardware_stats.last_frame.materials++;

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
                m_hardware_stats.last_frame.draw_calls++; // TODO Paul: This measurements should be done on glCalls.
            }
#ifdef MANGO_DEBUG
            bva                    = m_transparent_commands->append<bind_vertex_array_command, draw_elements_command>(de);
            bva->vertex_array_name = 0;
#endif // MANGO_DEBUG
        }

#ifdef MANGO_DEBUG
        // This needs to be done.
        // TODO Paul: State synchronization is not perfect.
        bt          = m_transparent_commands->append<bind_texture_command, bind_vertex_array_command>(bva);
        bt->binding = bt->sampler_location = 0;
        bt->texture_name                   = 0;

        bt          = m_transparent_commands->append<bind_texture_command, bind_texture_command>(bt);
        bt->binding = bt->sampler_location = 1;
        bt->texture_name                   = 0;

        bt          = m_transparent_commands->append<bind_texture_command, bind_texture_command>(bt);
        bt->binding = bt->sampler_location = 2;
        bt->texture_name                   = 0;

        bt          = m_transparent_commands->append<bind_texture_command, bind_texture_command>(bt);
        bt->binding = bt->sampler_location = 3;
        bt->texture_name                   = 0;

        bt          = m_transparent_commands->append<bind_texture_command, bind_texture_command>(bt);
        bt->binding = bt->sampler_location = 4;
        bt->texture_name                   = 0;
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

        // material data buffer
        bind_buffer_command* bb = m_gbuffer_commands->create<bind_buffer_command>(k);
        bb->target              = buffer_target::uniform_buffer;
        bb->index               = UB_SLOT_MATERIAL_DATA;
        bb->size                = sizeof(material_data);
        bb->buffer_name         = m_frame_uniform_buffer->buffer_name();
        bb->offset              = m_active_model.material_data_offset;

        // model data buffer
        bb              = m_gbuffer_commands->append<bind_buffer_command, bind_buffer_command>(bb);
        bb->target      = buffer_target::uniform_buffer;
        bb->index       = UB_SLOT_MODEL_DATA;
        bb->size        = sizeof(model_data);
        bb->buffer_name = m_frame_uniform_buffer->buffer_name();
        bb->offset      = m_active_model.model_data_offset;

        // material textures
        bind_texture_command* bt = m_gbuffer_commands->append<bind_texture_command, bind_buffer_command>(bb);
        bt->binding = bt->sampler_location = 0;
        bt->texture_name                   = m_active_model.base_color_texture_name;

        bt          = m_gbuffer_commands->append<bind_texture_command, bind_texture_command>(bt);
        bt->binding = bt->sampler_location = 1;
        bt->texture_name                   = m_active_model.roughness_metallic_texture_name;

        bt          = m_gbuffer_commands->append<bind_texture_command, bind_texture_command>(bt);
        bt->binding = bt->sampler_location = 2;
        bt->texture_name                   = m_active_model.occlusion_texture_name;

        bt          = m_gbuffer_commands->append<bind_texture_command, bind_texture_command>(bt);
        bt->binding = bt->sampler_location = 3;
        bt->texture_name                   = m_active_model.normal_texture_name;

        bt          = m_gbuffer_commands->append<bind_texture_command, bind_texture_command>(bt);
        bt->binding = bt->sampler_location = 4;
        bt->texture_name                   = m_active_model.emissive_color_texture_name;

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
            m_hardware_stats.last_frame.draw_calls++; // TODO Paul: This measurements should be done on glCalls.
            m_hardware_stats.last_frame.primitives++;
            m_hardware_stats.last_frame.materials++;
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
            m_hardware_stats.last_frame.draw_calls++; // TODO Paul: This measurements should be done on glCalls.
            m_hardware_stats.last_frame.primitives++;
            m_hardware_stats.last_frame.materials++;
#ifdef MANGO_DEBUG
            bva                    = m_gbuffer_commands->append<bind_vertex_array_command, draw_elements_command>(de);
            bva->vertex_array_name = 0;
#endif // MANGO_DEBUG
        }

#ifdef MANGO_DEBUG
        // This needs to be done.
        // TODO Paul: State synchronization is not perfect.
        bt          = m_gbuffer_commands->append<bind_texture_command, bind_vertex_array_command>(bva);
        bt->binding = bt->sampler_location = 0;
        bt->texture_name                   = 0;

        bt          = m_gbuffer_commands->append<bind_texture_command, bind_texture_command>(bt);
        bt->binding = bt->sampler_location = 1;
        bt->texture_name                   = 0;

        bt          = m_gbuffer_commands->append<bind_texture_command, bind_texture_command>(bt);
        bt->binding = bt->sampler_location = 2;
        bt->texture_name                   = 0;

        bt          = m_gbuffer_commands->append<bind_texture_command, bind_texture_command>(bt);
        bt->binding = bt->sampler_location = 3;
        bt->texture_name                   = 0;

        bt          = m_gbuffer_commands->append<bind_texture_command, bind_texture_command>(bt);
        bt->binding = bt->sampler_location = 4;
        bt->texture_name                   = 0;
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
        // material data buffer
        bind_buffer_command* bb = shadow_command_buffer->create<bind_buffer_command>(k);
        bb->target              = buffer_target::uniform_buffer;
        bb->index               = UB_SLOT_MATERIAL_DATA;
        bb->size                = sizeof(material_data);
        bb->buffer_name         = m_frame_uniform_buffer->buffer_name();
        bb->offset              = m_active_model.material_data_offset;

        // model data buffer
        bb              = shadow_command_buffer->append<bind_buffer_command, bind_buffer_command>(bb);
        bb->target      = buffer_target::uniform_buffer;
        bb->index       = UB_SLOT_MODEL_DATA;
        bb->size        = sizeof(model_data);
        bb->buffer_name = m_frame_uniform_buffer->buffer_name();
        bb->offset      = m_active_model.model_data_offset;

        // material textures
        bind_texture_command* bt = shadow_command_buffer->append<bind_texture_command, bind_buffer_command>(bb);
        bt->binding = bt->sampler_location = 0;
        bt->texture_name                   = m_active_model.base_color_texture_name;

        bind_vertex_array_command* bva = shadow_command_buffer->append<bind_vertex_array_command, bind_texture_command>(bt);
        bva->vertex_array_name         = vertex_array->get_name();

        if (type == index_type::none)
        {
            draw_arrays_command* da = shadow_command_buffer->append<draw_arrays_command, bind_vertex_array_command>(bva);
            da->topology            = topology;
            da->first               = first;
            da->count               = count;
            da->instance_count      = instance_count;
            m_hardware_stats.last_frame.draw_calls++; // TODO Paul: This measurements should be done on glCalls.
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
            m_hardware_stats.last_frame.draw_calls++; // TODO Paul: This measurements should be done on glCalls.
#ifdef MANGO_DEBUG
            bva                    = shadow_command_buffer->append<bind_vertex_array_command, draw_elements_command>(de);
            bva->vertex_array_name = 0;
#endif // MANGO_DEBUG
        }

#ifdef MANGO_DEBUG
        bt          = shadow_command_buffer->append<bind_texture_command, bind_vertex_array_command>(bva);
        bt->binding = bt->sampler_location = 0;
        bt->texture_name                   = 0;

        bt          = shadow_command_buffer->append<bind_texture_command, bind_texture_command>(bt);
        bt->binding = bt->sampler_location = 1;
        bt->texture_name                   = 0;

        bt          = shadow_command_buffer->append<bind_texture_command, bind_texture_command>(bt);
        bt->binding = bt->sampler_location = 2;
        bt->texture_name                   = 0;

        bt          = shadow_command_buffer->append<bind_texture_command, bind_texture_command>(bt);
        bt->binding = bt->sampler_location = 3;
        bt->texture_name                   = 0;

        bt          = shadow_command_buffer->append<bind_texture_command, bind_texture_command>(bt);
        bt->binding = bt->sampler_location = 4;
        bt->texture_name                   = 0;
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

    bind_buffer_command* bb = m_global_binding_commands->create<bind_buffer_command>(command_keys::no_sort);
    bb->target              = buffer_target::uniform_buffer;
    bb->index               = UB_SLOT_LIGHTING_PASS_DATA;
    bb->size                = sizeof(lighting_pass_data);
    bb->buffer_name         = m_frame_uniform_buffer->buffer_name();
    bb->offset              = m_frame_uniform_buffer->write_data(sizeof(lighting_pass_data), &m_lighting_pass_data);

    // cleanup lighting uniforms...
    m_lighting_pass_data.directional.intensity = 0.0f;
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
            m_lighting_pass_commands->invalidate();
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
