//! \file      deferred_pbr_renderer.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2022
//! \copyright Apache License 2.0

#include <glad/glad.h>
#include <mango/imgui_helper.hpp>
#include <mango/profile.hpp>
#include <rendering/passes/bloom_pass.hpp>
#include <rendering/passes/environment_display_pass.hpp>
#include <rendering/passes/fxaa_pass.hpp>
#include <rendering/passes/gtao_pass.hpp>
#include <rendering/passes/shadow_map_pass.hpp>
#include <rendering/pipelines/deferred_pbr_renderer.hpp>
#include <resources/resources_impl.hpp>
#include <scene/scene_impl.hpp>
#include <util/helpers.hpp>

using namespace mango;

//! \brief Default 2D \a gfx_texture to bind when no other texture is available.
gfx_handle<const gfx_texture> default_texture_2D;
//! \brief Default cube \a gfx_texture to bind when no other texture is available.
gfx_handle<const gfx_texture> default_texture_cube;
//! \brief Default array \a gfx_texture to bind when no other texture is available.
gfx_handle<const gfx_texture> default_texture_array;

deferred_pbr_renderer::deferred_pbr_renderer(const renderer_configuration& configuration, const shared_ptr<context_impl>& context)
    : renderer_impl(configuration, context)
    , m_pipeline_cache(std::make_shared<renderer_pipeline_cache>(context))
    , m_frame_context(nullptr)
    , m_debug_drawer(std::make_shared<debug_drawer>(context))
    , m_debug_bounds(false)
    , m_graphics_device(m_shared_context->get_graphics_device())
{
    PROFILE_ZONE;

    m_frame_context = m_graphics_device->create_graphics_device_context();

    m_renderer_data.shadow_pass_enabled         = false;
    m_renderer_data.debug_view_enabled          = false;
    m_renderer_data.position_debug_view         = false;
    m_renderer_data.normal_debug_view           = false;
    m_renderer_data.depth_debug_view            = false;
    m_renderer_data.base_color_debug_view       = false;
    m_renderer_data.reflection_color_debug_view = false;
    m_renderer_data.emission_debug_view         = false;
    m_renderer_data.occlusion_debug_view        = false;
    m_renderer_data.roughness_debug_view        = false;
    m_renderer_data.metallic_debug_view         = false;
    m_renderer_data.show_cascades               = false;

    if (!create_renderer_resources())
    {
        MANGO_LOG_ERROR("Resource Creation Failed! Renderer is not available!");
        return;
    }

    m_vsync           = configuration.is_vsync_enabled();
    m_wireframe       = configuration.should_draw_wireframe();
    m_frustum_culling = configuration.is_frustum_culling_enabled();
    m_debug_bounds    = configuration.should_draw_debug_bounds();

    auto device_context = m_graphics_device->create_graphics_device_context();
    device_context->begin();
    device_context->set_buffer_data(m_renderer_data_buffer, 0, sizeof(renderer_data), &m_renderer_data);
    device_context->set_swap_interval(m_vsync ? 1 : 0);
    device_context->end();
    device_context->submit();
}

deferred_pbr_renderer::~deferred_pbr_renderer() {}

bool deferred_pbr_renderer::create_renderer_resources()
{
    auto display = m_shared_context->get_display();
    int32 w      = display->get_width();
    int32 h      = display->get_height();

    m_renderer_info.canvas.x      = 0;
    m_renderer_info.canvas.y      = 0;
    m_renderer_info.canvas.width  = w;
    m_renderer_info.canvas.height = h;

    // Textures and Samplers
    if (!create_textures_and_samplers())
        return false;

    // Uniforms and Uniform/ShaderStorage/Image Buffers
    if (!create_buffers())
        return false;

    // Passes
    if (!create_passes())
        return false;

    return true;
}

bool deferred_pbr_renderer::create_textures_and_samplers()
{
    const int32& w = m_renderer_info.canvas.width;
    const int32& h = m_renderer_info.canvas.height;

    texture_create_info attachment_info;
    attachment_info.texture_type   = gfx_texture_type::texture_type_2d;
    attachment_info.width          = 1;
    attachment_info.height         = 1;
    attachment_info.miplevels      = 1;
    attachment_info.array_layers   = 1;
    attachment_info.texture_format = gfx_format::r8;
    default_texture_2D             = m_graphics_device->create_texture(attachment_info);
    if (!check_creation(default_texture_2D.get(), "default texture 2D"))
        return false;
    attachment_info.texture_type = gfx_texture_type::texture_type_cube_map;
    default_texture_cube         = m_graphics_device->create_texture(attachment_info);
    if (!check_creation(default_texture_cube.get(), "default texture cube"))
        return false;
    attachment_info.texture_type = gfx_texture_type::texture_type_2d_array;
    attachment_info.array_layers = 3;
    default_texture_array        = m_graphics_device->create_texture(attachment_info);
    if (!check_creation(default_texture_array.get(), "default texture array"))
        return false;

    texture_set_description set_desc;
    set_desc.level          = 0;
    set_desc.x_offset       = 0;
    set_desc.y_offset       = 0;
    set_desc.z_offset       = 0;
    set_desc.width          = 1;
    set_desc.height         = 1;
    set_desc.depth          = 1;
    set_desc.pixel_format   = gfx_format::rgba;
    set_desc.component_type = gfx_format::t_unsigned_byte;

    uint8 albedo[4] = { 1, 1, 1, 255 };

    auto device_context = m_graphics_device->create_graphics_device_context();
    device_context->begin();
    device_context->set_texture_data(default_texture_2D, set_desc, albedo);
    device_context->set_texture_data(default_texture_cube, set_desc, albedo);
    set_desc.pixel_format = gfx_format::rgb;
    set_desc.depth        = 3;
    device_context->set_texture_data(default_texture_array, set_desc, albedo);
    device_context->end();
    device_context->submit();

    attachment_info.width        = w;
    attachment_info.height       = h;
    attachment_info.array_layers = 1;
    attachment_info.texture_type = gfx_texture_type::texture_type_2d;

    m_gbuffer_render_targets.clear();
    attachment_info.texture_format = gfx_format::rgba8;
    m_gbuffer_render_targets.push_back(m_graphics_device->create_texture(attachment_info));
    attachment_info.texture_format = gfx_format::rgb10_a2;
    m_gbuffer_render_targets.push_back(m_graphics_device->create_texture(attachment_info));
    attachment_info.texture_format = gfx_format::rgba32f;
    m_gbuffer_render_targets.push_back(m_graphics_device->create_texture(attachment_info));
    attachment_info.texture_format = gfx_format::rgba8;
    m_gbuffer_render_targets.push_back(m_graphics_device->create_texture(attachment_info));
    attachment_info.texture_format = gfx_format::depth_component32f;
    m_gbuffer_render_targets.push_back(m_graphics_device->create_texture(attachment_info));

    for (auto rt : m_gbuffer_render_targets)
    {
        if (!check_creation(rt.get(), "gbuffer render targets"))
            return false;
    }

    // HDR for auto exposure
    m_hdr_buffer_render_targets.clear();
    attachment_info.miplevels      = graphics::calculate_mip_count(w, h);
    attachment_info.texture_format = gfx_format::rgba32f;
    m_hdr_buffer_render_targets.push_back(m_graphics_device->create_texture(attachment_info));
    attachment_info.miplevels      = 1;
    attachment_info.texture_format = gfx_format::depth_component32f;
    m_hdr_buffer_render_targets.push_back(m_graphics_device->create_texture(attachment_info));

    for (auto rt : m_hdr_buffer_render_targets)
    {
        if (!check_creation(rt.get(), "hdr buffer render targets"))
            return false;
    }

    // output
    attachment_info.miplevels      = 1;
    attachment_info.texture_format = gfx_format::rgba8;
    m_output_target                = m_graphics_device->create_texture(attachment_info);
    attachment_info.texture_format = gfx_format::depth_component32f;
    m_ouput_depth_target           = m_graphics_device->create_texture(attachment_info);

    if (!check_creation(m_output_target.get(), "output target"))
        return false;
    if (!check_creation(m_ouput_depth_target.get(), "output depth target"))
        return false;

    auto antialiasing = std::static_pointer_cast<fxaa_pass>(m_pipeline_extensions[mango::render_pipeline_extension::fxaa]);
    if (antialiasing)
        antialiasing->set_output_targets(m_output_target, m_ouput_depth_target);

    // postprocessing render targets
    m_post_render_targets.clear();
    attachment_info.miplevels      = 1;
    attachment_info.texture_format = gfx_format::rgba8;
    m_post_render_targets.push_back(m_graphics_device->create_texture(attachment_info));
    attachment_info.texture_format = gfx_format::depth_component32f;
    m_post_render_targets.push_back(m_graphics_device->create_texture(attachment_info));

    for (auto rt : m_post_render_targets)
    {
        if (!check_creation(rt.get(), "postprocessing buffer render targets"))
            return false;
    }

    sampler_create_info sampler_info;
    sampler_info.sampler_min_filter      = gfx_sampler_filter::sampler_filter_nearest;
    sampler_info.sampler_max_filter      = gfx_sampler_filter::sampler_filter_nearest;
    sampler_info.enable_comparison_mode  = false;
    sampler_info.comparison_operator     = gfx_compare_operator::compare_operator_always;
    sampler_info.edge_value_wrap_u       = gfx_sampler_edge_wrap::sampler_edge_wrap_clamp_to_edge;
    sampler_info.edge_value_wrap_v       = gfx_sampler_edge_wrap::sampler_edge_wrap_clamp_to_edge;
    sampler_info.edge_value_wrap_w       = gfx_sampler_edge_wrap::sampler_edge_wrap_clamp_to_edge;
    sampler_info.border_color[0]         = 0;
    sampler_info.border_color[1]         = 0;
    sampler_info.border_color[2]         = 0;
    sampler_info.border_color[3]         = 0;
    sampler_info.enable_seamless_cubemap = false;

    m_nearest_sampler = m_graphics_device->create_sampler(sampler_info);

    sampler_info.sampler_min_filter = gfx_sampler_filter::sampler_filter_linear;
    sampler_info.sampler_max_filter = gfx_sampler_filter::sampler_filter_linear;
    m_linear_sampler                = m_graphics_device->create_sampler(sampler_info);

    sampler_info.enable_comparison_mode = true;
    sampler_info.comparison_operator    = gfx_compare_operator::compare_operator_less_equal;
    m_linear_compare_sampler            = m_graphics_device->create_sampler(sampler_info);

    sampler_info.enable_comparison_mode = false;
    sampler_info.sampler_min_filter     = gfx_sampler_filter::sampler_filter_linear_mipmap_linear;
    sampler_info.sampler_max_filter     = gfx_sampler_filter::sampler_filter_linear;
    m_mipmapped_linear_sampler          = m_graphics_device->create_sampler(sampler_info);

    if (!check_creation(m_nearest_sampler.get(), "nearest sampler"))
        return false;
    if (!check_creation(m_linear_sampler.get(), "linear sampler"))
        return false;
    if (!check_creation(m_mipmapped_linear_sampler.get(), "mipmapped linear sampler"))
        return false;

    return true;
}

bool deferred_pbr_renderer::create_buffers()
{
    buffer_create_info buffer_info;
    buffer_info.buffer_target = gfx_buffer_target::buffer_target_uniform;
    buffer_info.buffer_access = gfx_buffer_access::buffer_access_dynamic_storage;

    buffer_info.size       = sizeof(renderer_data);
    m_renderer_data_buffer = m_graphics_device->create_buffer(buffer_info);
    if (!check_creation(m_renderer_data_buffer.get(), "renderer data buffer"))
        return false;

    return true;
}

bool deferred_pbr_renderer::create_passes()
{
    m_opaque_geometry_pass.setup(m_pipeline_cache, m_debug_drawer);
    m_opaque_geometry_pass.attach(m_shared_context);
    m_deferred_lighting_pass.attach(m_shared_context);
    m_transparent_pass.setup(m_pipeline_cache, m_debug_drawer);
    m_transparent_pass.attach(m_shared_context);
    m_composing_pass.attach(m_shared_context);
    m_auto_luminance_pass.attach(m_shared_context);
    m_hi_z_pass.attach(m_shared_context);

    // optional passes
    const bool* render_passes = m_configuration.get_render_extensions();
    if (render_passes[mango::render_pipeline_extension::environment_display])
    {
        // create an extra object that is capable to render environment cubemaps.
        auto environment_display = std::make_shared<environment_display_pass>(m_configuration.get_environment_display_settings());
        environment_display->attach(m_shared_context);
        m_pipeline_extensions[mango::render_pipeline_extension::environment_display] = std::static_pointer_cast<render_pass>(environment_display);
    }
    if (m_configuration.get_render_extensions()[mango::render_pipeline_extension::shadow_map])
    {
        auto pass_shadow_map = std::make_shared<shadow_map_pass>(m_configuration.get_shadow_settings());
        pass_shadow_map->setup(m_pipeline_cache, m_debug_drawer);
        pass_shadow_map->attach(m_shared_context);
        m_pipeline_extensions[mango::render_pipeline_extension::shadow_map] = std::static_pointer_cast<render_pass>(pass_shadow_map);
        m_renderer_data.shadow_pass_enabled                                 = true;
    }
    if (m_configuration.get_render_extensions()[mango::render_pipeline_extension::fxaa])
    {
        auto pass_fxaa = std::make_shared<fxaa_pass>(m_configuration.get_fxaa_settings());
        pass_fxaa->attach(m_shared_context);
        m_pipeline_extensions[mango::render_pipeline_extension::fxaa] = std::static_pointer_cast<render_pass>(pass_fxaa);
    }
    if (m_configuration.get_render_extensions()[mango::render_pipeline_extension::gtao])
    {
        auto pass_gtao = std::make_shared<gtao_pass>(m_configuration.get_gtao_settings());
        pass_gtao->attach(m_shared_context);
        m_pipeline_extensions[mango::render_pipeline_extension::gtao] = std::static_pointer_cast<render_pass>(pass_gtao);
    }
    if (m_configuration.get_render_extensions()[mango::render_pipeline_extension::bloom])
    {
        auto pass_bloom = std::make_shared<bloom_pass>(m_configuration.get_bloom_settings());
        pass_bloom->attach(m_shared_context);
        m_pipeline_extensions[mango::render_pipeline_extension::bloom] = std::static_pointer_cast<render_pass>(pass_bloom);
    }

    return update_passes();
}

bool deferred_pbr_renderer::update_passes()
{
    gfx_viewport window_viewport{ static_cast<float>(m_renderer_info.canvas.x), static_cast<float>(m_renderer_info.canvas.y), static_cast<float>(m_renderer_info.canvas.width),
                                  static_cast<float>(m_renderer_info.canvas.height) };

    m_opaque_geometry_pass.set_viewport(window_viewport);
    m_opaque_geometry_pass.set_render_targets(m_gbuffer_render_targets);
    m_opaque_geometry_pass.set_debug_bounds(m_debug_bounds);
    m_opaque_geometry_pass.set_frustum_culling(m_frustum_culling);
    m_opaque_geometry_pass.set_wireframe(m_wireframe);
    m_opaque_geometry_pass.set_default_texture_2D(default_texture_2D);

    m_deferred_lighting_pass.set_viewport(window_viewport);
    m_deferred_lighting_pass.set_render_targets(m_hdr_buffer_render_targets);
    m_deferred_lighting_pass.set_gbuffer(m_gbuffer_render_targets, m_linear_sampler);
    m_deferred_lighting_pass.set_renderer_data_buffer(m_renderer_data_buffer);
    m_deferred_lighting_pass.set_irradiance_map_sampler(m_mipmapped_linear_sampler);
    m_deferred_lighting_pass.set_radiance_map_sampler(m_mipmapped_linear_sampler);
    m_deferred_lighting_pass.set_brdf_integration_lut_sampler(m_linear_sampler);
    m_deferred_lighting_pass.set_shadow_map_sampler(m_linear_sampler);

    m_transparent_pass.set_viewport(window_viewport);
    m_transparent_pass.set_render_targets(m_hdr_buffer_render_targets);
    m_transparent_pass.set_debug_bounds(m_debug_bounds);
    m_transparent_pass.set_frustum_culling(m_frustum_culling);
    m_transparent_pass.set_wireframe(m_wireframe);
    m_transparent_pass.set_default_texture_2D(default_texture_2D);
    m_transparent_pass.set_renderer_data_buffer(m_renderer_data_buffer);
    m_transparent_pass.set_irradiance_map_sampler(m_mipmapped_linear_sampler);
    m_transparent_pass.set_radiance_map_sampler(m_mipmapped_linear_sampler);
    m_transparent_pass.set_brdf_integration_lut_sampler(m_linear_sampler);
    m_transparent_pass.set_shadow_map_sampler(m_linear_sampler);

    m_composing_pass.set_viewport(window_viewport);
    m_composing_pass.set_renderer_data_buffer(m_renderer_data_buffer);
    m_composing_pass.set_hdr_input(m_hdr_buffer_render_targets[0]);
    m_composing_pass.set_hdr_input_sampler(m_nearest_sampler);
    m_composing_pass.set_depth_input(m_hdr_buffer_render_targets.back());
    m_composing_pass.set_depth_input_sampler(m_nearest_sampler);

    m_auto_luminance_pass.set_hdr_input(m_hdr_buffer_render_targets[0]);
    m_auto_luminance_pass.set_input_size(m_renderer_info.canvas.width, m_renderer_info.canvas.height);

    m_hi_z_pass.set_depth_texture(m_gbuffer_render_targets.back());
    m_hi_z_pass.set_depth_size(m_renderer_info.canvas.width, m_renderer_info.canvas.height);
    m_hi_z_pass.set_nearest_sampler(m_nearest_sampler);

    // optional
    auto environment_display = std::static_pointer_cast<environment_display_pass>(m_pipeline_extensions[mango::render_pipeline_extension::environment_display]);
    if (environment_display)
        environment_display->set_renderer_data_buffer(m_renderer_data_buffer);

    auto shadow_pass = std::static_pointer_cast<shadow_map_pass>(m_pipeline_extensions[mango::render_pipeline_extension::shadow_map]);
    if (shadow_pass)
    {
        shadow_pass->set_frustum_culling(m_frustum_culling);
        shadow_pass->set_debug_bounds(m_debug_bounds);
        shadow_pass->set_wireframe(m_wireframe);
        shadow_pass->set_debug_view_enabled(m_renderer_data.debug_view_enabled);
        shadow_pass->set_default_texture_2D(default_texture_2D);
    }

    auto pass_fxaa = std::static_pointer_cast<fxaa_pass>(m_pipeline_extensions[mango::render_pipeline_extension::fxaa]);
    if (pass_fxaa)
        pass_fxaa->set_output_targets(m_output_target, m_ouput_depth_target);

    auto pass_gtao = std::static_pointer_cast<gtao_pass>(m_pipeline_extensions[mango::render_pipeline_extension::gtao]);
    if (pass_gtao)
    {
        pass_gtao->set_gbuffer_normal_texture(m_gbuffer_render_targets[1]);
        pass_gtao->set_gbuffer_orm_texture(m_gbuffer_render_targets[3]);
        pass_gtao->set_full_res_depth_texture(m_gbuffer_render_targets.back());
        pass_gtao->set_nearest_sampler(m_nearest_sampler);
        pass_gtao->set_linear_sampler(m_linear_sampler);
        pass_gtao->set_viewport(window_viewport);
    }

    auto pass_bloom = std::static_pointer_cast<bloom_pass>(m_pipeline_extensions[mango::render_pipeline_extension::bloom]);
    if (pass_bloom)
    {
        pass_bloom->set_hdr_texture(m_hdr_buffer_render_targets[0]);
        pass_bloom->set_mipmapped_linear_sampler(m_mipmapped_linear_sampler);
        pass_bloom->set_viewport(window_viewport);
        pass_bloom->set_default_texture_2D(default_texture_2D);
    }

    return true; // TODO Paul: This is always true atm.
}

void deferred_pbr_renderer::update(float dt)
{
    MANGO_UNUSED(dt);
}

void deferred_pbr_renderer::render(scene_impl* scene, float dt)
{
    PROFILE_ZONE;
    m_renderer_info.last_frame.draw_calls = 0;
    m_renderer_info.last_frame.vertices   = 0;

    m_frame_context->begin();
    float clear_color[4] = { 0.1f, 0.1f, 0.1f, 1.0f }; // TODO Paul: member or dynamic?
    auto swap_buffer     = m_graphics_device->get_swap_chain_render_target();

    auto shadow_pass = std::static_pointer_cast<shadow_map_pass>(m_pipeline_extensions[mango::render_pipeline_extension::shadow_map]);

    // clear all framebuffers
    {
        GL_NAMED_PROFILE_ZONE("Clear Framebuffers");
        NAMED_PROFILE_ZONE("Clear Framebuffers");
        if (shadow_pass)
        {
            m_frame_context->set_render_targets(0, nullptr, shadow_pass->get_shadow_maps_texture());
            m_frame_context->clear_depth_stencil(gfx_clear_attachment_flag_bits::clear_flag_depth_buffer, 1.0f, 0);
        }
        m_frame_context->set_render_targets(static_cast<int32>(m_gbuffer_render_targets.size()) - 1, m_gbuffer_render_targets.data(), m_gbuffer_render_targets.back());
        m_frame_context->clear_depth_stencil(gfx_clear_attachment_flag_bits::clear_flag_depth_buffer, 1.0f, 0);
        m_frame_context->clear_render_target(gfx_clear_attachment_flag_bits::clear_flag_all_draw_buffers, clear_color);
        m_frame_context->set_render_targets(static_cast<int32>(m_hdr_buffer_render_targets.size()) - 1, m_hdr_buffer_render_targets.data(), m_hdr_buffer_render_targets.back());
        m_frame_context->clear_depth_stencil(gfx_clear_attachment_flag_bits::clear_flag_depth_buffer, 1.0f, 0);
        m_frame_context->clear_render_target(gfx_clear_attachment_flag_bits::clear_flag_all_draw_buffers, clear_color);
        m_frame_context->set_render_targets(static_cast<int32>(m_post_render_targets.size()) - 1, m_post_render_targets.data(), m_post_render_targets.back());
        m_frame_context->clear_depth_stencil(gfx_clear_attachment_flag_bits::clear_flag_depth_buffer, 1.0f, 0);
        m_frame_context->clear_render_target(gfx_clear_attachment_flag_bits::clear_flag_all_draw_buffers, clear_color);
        m_frame_context->set_render_targets(1, &m_output_target, m_ouput_depth_target);
        m_frame_context->clear_depth_stencil(gfx_clear_attachment_flag_bits::clear_flag_depth_buffer, 1.0f, 0);
        m_frame_context->clear_render_target(gfx_clear_attachment_flag_bits::clear_flag_all_draw_buffers, clear_color);
        // TODO Paul: Is the renderer in charge here?
        m_frame_context->set_render_targets(1, &swap_buffer, m_graphics_device->get_swap_chain_depth_stencil_target());
        m_frame_context->clear_depth_stencil(gfx_clear_attachment_flag_bits::clear_flag_depth_buffer, 1.0f, 0);
        m_frame_context->clear_render_target(gfx_clear_attachment_flag_bits::clear_flag_all_draw_buffers, clear_color);
    }

    m_frame_context->set_buffer_data(m_renderer_data_buffer, 0, sizeof(renderer_data), &m_renderer_data);

    auto active_camera_data = scene->get_active_camera_gpu_data();
    if (!active_camera_data.has_value())
        return;

    shared_ptr<std::vector<draw_key>> draws = std::make_shared<std::vector<draw_key>>();
    int32 opaque_count                      = 0;
    auto instances                          = scene->get_render_instances();
    if (m_debug_bounds)
        m_debug_drawer->clear();
    bounding_frustum camera_frustum;
    if (m_frustum_culling)
    {
        camera_frustum = bounding_frustum(active_camera_data->per_camera_data.view_matrix, active_camera_data->per_camera_data.projection_matrix);
        if (m_debug_bounds)
        {
            auto corners = bounding_frustum::get_corners(active_camera_data->per_camera_data.view_projection_matrix);
            m_debug_drawer->set_color(color_rgb(0.0f, 1.0f, 0.0f));
            m_debug_drawer->add(corners[0], corners[1]);
            m_debug_drawer->add(corners[1], corners[3]);
            m_debug_drawer->add(corners[3], corners[2]);
            m_debug_drawer->add(corners[2], corners[6]);
            m_debug_drawer->add(corners[6], corners[4]);
            m_debug_drawer->add(corners[4], corners[0]);
            m_debug_drawer->add(corners[0], corners[2]);

            m_debug_drawer->add(corners[5], corners[4]);
            m_debug_drawer->add(corners[4], corners[6]);
            m_debug_drawer->add(corners[6], corners[7]);
            m_debug_drawer->add(corners[7], corners[3]);
            m_debug_drawer->add(corners[3], corners[1]);
            m_debug_drawer->add(corners[1], corners[5]);
            m_debug_drawer->add(corners[5], corners[7]);
        }
    }

    for (auto instance : instances)
    {
        // we can assume the stuff exists - we also want to be fast
        optional<node&> node = scene->get_node(instance.node_hnd);
        MANGO_ASSERT(node, "Non existing node in instances!");
        optional<mat4&> global_transformation_matrix = scene->get_global_transformation_matrix(node->global_matrix_hnd);
        MANGO_ASSERT(global_transformation_matrix, "Non existing transformation matrix in instances!");

        if ((node->type & node_type::mesh) != node_type::hierarchy)
        {
            draw_key a_draw;

            MANGO_ASSERT(node->mesh_hnd.valid(), "Node with mesh has no mesh attached!");
            optional<mesh&> mesh = scene->get_mesh(node->mesh_hnd);
            MANGO_ASSERT(mesh, "Non existing mesh in instances!");
            a_draw.mesh_gpu_data_id = mesh->gpu_data;

            for (auto p : mesh->primitives)
            {
                optional<primitive&> prim = scene->get_primitive(p);
                MANGO_ASSERT(prim, "Non existing primitive in instances!");
                optional<material&> mat = scene->get_material(prim->primitive_material);
                MANGO_ASSERT(mat, "Non existing material in instances!");

                a_draw.primitive_gpu_data_id = prim->gpu_data;
                a_draw.material_hnd          = prim->primitive_material;

                a_draw.transparent = mat->alpha_mode > material_alpha_mode::mode_mask;
                opaque_count += a_draw.transparent ? 0 : 1;

                a_draw.bounding_box = prim->bounding_box.get_transformed(global_transformation_matrix.value());

                if (!a_draw.transparent)
                    a_draw.view_depth = active_camera_data->per_camera_data.camera_far + 1.0f;
                else
                    a_draw.view_depth = active_camera_data->per_camera_data.camera_near - 1.0f;

                std::array<vec3, 8> bb_corners = a_draw.bounding_box.get_corners();
                for (const vec3& corner : bb_corners)
                {
                    if (!a_draw.transparent)
                        a_draw.view_depth = min(a_draw.view_depth, (active_camera_data->per_camera_data.view_matrix * vec4(corner.x(), corner.y(), corner.z(), 1.0f)).z());
                    else
                        a_draw.view_depth = max(a_draw.view_depth, (active_camera_data->per_camera_data.view_matrix * vec4(corner.x(), corner.y(), corner.z(), 1.0f)).z());
                }
                // MANGO_LOG_INFO("{0}", a_draw.view_depth);

                draws->push_back(a_draw);
            }
        }
    }

    std::sort(draws->begin(), draws->end());

    auto warn_missing_draw = [](string what) { MANGO_LOG_WARN("{0} missing for draw. Skipping DrawCall!", what); };

    const light_stack& ls = scene->get_light_stack();
    auto light_data       = scene->get_light_gpu_data();

    // shadow pass
    if (shadow_pass)
    {
        shadow_pass->set_camera_data_buffer(active_camera_data->camera_data_buffer);
        shadow_pass->set_scene_pointer(scene);
        shadow_pass->set_camera_frustum(camera_frustum);
        shadow_pass->set_draws(draws);
        shadow_pass->set_delta_time(dt);
        shadow_pass->set_camera_near(active_camera_data->per_camera_data.camera_near);
        shadow_pass->set_camera_far(active_camera_data->per_camera_data.camera_far);
        shadow_pass->set_camera_inverse_view_projection(active_camera_data->per_camera_data.inverse_view_projection_matrix);
        shadow_pass->set_shadow_casters(ls.get_shadow_casters());

        shadow_pass->execute(m_frame_context);

        auto pass_info = shadow_pass->get_info();
        m_renderer_info.last_frame.draw_calls += pass_info.draw_calls;
        m_renderer_info.last_frame.vertices += pass_info.vertices;
    }

    // gbuffer pass
    {
        m_opaque_geometry_pass.set_camera_data_buffer(active_camera_data->camera_data_buffer);
        m_opaque_geometry_pass.set_scene_pointer(scene);
        m_opaque_geometry_pass.set_camera_frustum(camera_frustum);
        m_opaque_geometry_pass.set_draws(draws);
        m_opaque_geometry_pass.set_opaque_count(opaque_count);

        m_opaque_geometry_pass.execute(m_frame_context);

        auto pass_info = m_opaque_geometry_pass.get_info();
        m_renderer_info.last_frame.draw_calls += pass_info.draw_calls;
        m_renderer_info.last_frame.vertices += pass_info.vertices;
    }

    // hi-z
    {
        m_hi_z_pass.execute(m_frame_context);
    }

    // gtao
    auto ao_pass = std::static_pointer_cast<gtao_pass>(m_pipeline_extensions[mango::render_pipeline_extension::gtao]);
    if (ao_pass)
    {
        ao_pass->set_camera_data_buffer(active_camera_data->camera_data_buffer);
        ao_pass->set_hierarchical_depth_texture(m_hi_z_pass.get_hierarchical_depth_buffer());
        ao_pass->set_depth_mip_count(graphics::calculate_mip_count(m_renderer_info.canvas.width, m_renderer_info.canvas.height)); // TODO: Do not calculate again?
        ao_pass->execute(m_frame_context);

        auto pass_info = ao_pass->get_info();
        m_renderer_info.last_frame.draw_calls += pass_info.draw_calls;
        m_renderer_info.last_frame.vertices += pass_info.vertices;
    }

    // lighting pass
    auto irradiance = ls.get_skylight_irradiance_map();
    auto specular   = ls.get_skylight_specular_prefilter_map();
    auto brdf_lut   = ls.get_skylight_brdf_lookup();
    {
        m_deferred_lighting_pass.set_camera_data_buffer(active_camera_data->camera_data_buffer);
        m_deferred_lighting_pass.set_light_data_buffer(light_data.light_data_buffer);
        m_deferred_lighting_pass.set_shadow_data_buffer(shadow_pass ? shadow_pass->get_shadow_data_buffer() : nullptr);

        m_deferred_lighting_pass.set_irradiance_map(irradiance ? irradiance : default_texture_cube);
        m_deferred_lighting_pass.set_radiance_map(specular ? specular : default_texture_cube);
        m_deferred_lighting_pass.set_brdf_integration_lut(brdf_lut ? brdf_lut : default_texture_2D);

        m_deferred_lighting_pass.set_shadow_map(shadow_pass ? shadow_pass->get_shadow_maps_texture() : default_texture_array);
        m_deferred_lighting_pass.set_shadow_map_compare_sampler(shadow_pass ? m_linear_compare_sampler : m_linear_sampler);

        m_deferred_lighting_pass.execute(m_frame_context);

        auto pass_info = m_deferred_lighting_pass.get_info();
        m_renderer_info.last_frame.draw_calls += pass_info.draw_calls;
        m_renderer_info.last_frame.vertices += pass_info.vertices;
    }

    // cubemap pass
    if (!m_renderer_data.debug_view_enabled)
    {
        auto environment_display = std::static_pointer_cast<environment_display_pass>(m_pipeline_extensions[mango::render_pipeline_extension::environment_display]);
        if (environment_display && specular)
        {
            environment_display->set_camera_data_buffer(active_camera_data->camera_data_buffer);
            environment_display->set_cubemap(specular);

            environment_display->execute(m_frame_context);

            auto pass_info = environment_display->get_info();
            m_renderer_info.last_frame.draw_calls += pass_info.draw_calls;
            m_renderer_info.last_frame.vertices += pass_info.vertices;
        }
    }

    // transparent pass
    {
        m_transparent_pass.set_camera_data_buffer(active_camera_data->camera_data_buffer);
        m_transparent_pass.set_light_data_buffer(light_data.light_data_buffer);
        m_transparent_pass.set_shadow_data_buffer(shadow_pass ? shadow_pass->get_shadow_data_buffer() : nullptr);

        m_transparent_pass.set_scene_pointer(scene);
        m_transparent_pass.set_camera_frustum(camera_frustum);
        m_transparent_pass.set_draws(draws);
        m_transparent_pass.set_transparent_start(opaque_count);

        m_transparent_pass.set_irradiance_map(irradiance ? irradiance : default_texture_cube);
        m_transparent_pass.set_radiance_map(specular ? specular : default_texture_cube);
        m_transparent_pass.set_brdf_integration_lut(brdf_lut ? brdf_lut : default_texture_2D);

        m_transparent_pass.set_shadow_map(shadow_pass ? shadow_pass->get_shadow_maps_texture() : default_texture_array);
        m_transparent_pass.set_shadow_map_compare_sampler(shadow_pass ? m_linear_compare_sampler : m_linear_sampler);

        m_transparent_pass.execute(m_frame_context);

        auto pass_info = m_transparent_pass.get_info();
        m_renderer_info.last_frame.draw_calls += pass_info.draw_calls;
        m_renderer_info.last_frame.vertices += pass_info.vertices;
    }

    m_debug_drawer->update_buffer();

    // auto exposure
    if (scene->calculate_auto_exposure())
    {
        m_auto_luminance_pass.set_delta_time(dt);

        m_auto_luminance_pass.execute(m_frame_context);
    }

    // bloom
    auto pass_bloom = std::static_pointer_cast<bloom_pass>(m_pipeline_extensions[mango::render_pipeline_extension::bloom]);
    if (!m_renderer_data.debug_view_enabled && pass_bloom)
    {
        pass_bloom->execute(m_frame_context);

        auto pass_info = pass_bloom->get_info();
        m_renderer_info.last_frame.draw_calls += pass_info.draw_calls;
        m_renderer_info.last_frame.vertices += pass_info.vertices;
    }

    auto antialiasing          = std::static_pointer_cast<fxaa_pass>(m_pipeline_extensions[mango::render_pipeline_extension::fxaa]);
    bool postprocessing_buffer = antialiasing != nullptr;

    // composing pass
    {
        if (postprocessing_buffer)
            m_composing_pass.set_render_targets(m_post_render_targets);
        else
            m_composing_pass.set_render_targets({ m_output_target, m_ouput_depth_target });

        m_composing_pass.set_camera_data_buffer(active_camera_data->camera_data_buffer);

        m_composing_pass.execute(m_frame_context);

        auto pass_info = m_composing_pass.get_info();
        m_renderer_info.last_frame.draw_calls += pass_info.draw_calls;
        m_renderer_info.last_frame.vertices += pass_info.vertices;
    }

    // debug lines
    if (m_debug_bounds)
    {
        m_renderer_info.last_frame.draw_calls++;
        m_renderer_info.last_frame.vertices += m_debug_drawer->vertex_count();
        // use already set last render targets and just add lines on top!
        m_debug_drawer->execute(m_frame_context);
    }

    // fxaa
    if (antialiasing)
    {
        antialiasing->set_input_texture(m_post_render_targets[0]); // TODO Paul Hardcoded post targets -> meh.
        m_renderer_info.last_frame.draw_calls++;
        m_renderer_info.last_frame.vertices += 3;
        antialiasing->execute(m_frame_context);

        auto pass_info = antialiasing->get_info();
        m_renderer_info.last_frame.draw_calls += pass_info.draw_calls;
        m_renderer_info.last_frame.vertices += pass_info.vertices;
    }

    m_frame_context->bind_pipeline(nullptr);
    // TODO Paul: Is the renderer in charge here?
    m_frame_context->set_render_targets(1, &swap_buffer, m_graphics_device->get_swap_chain_depth_stencil_target());
}

void deferred_pbr_renderer::present()
{
    m_frame_context->present();
    m_frame_context->end();
    m_frame_context->submit();
}

void deferred_pbr_renderer::set_viewport(int32 x, int32 y, int32 width, int32 height)
{
    MANGO_ASSERT(x >= 0, "Viewport x position has to be positive!");
    MANGO_ASSERT(y >= 0, "Viewport y position has to be positive!");
    MANGO_ASSERT(width >= 0, "Viewport width has to be positive!");
    MANGO_ASSERT(height >= 0, "Viewport height has to be positive!");

    // Resize everything ... Samplers are not required ... But ...
    if (m_renderer_info.canvas.x != x || m_renderer_info.canvas.y != y || m_renderer_info.canvas.width != width || m_renderer_info.canvas.height != height)
    {
        m_renderer_info.canvas.x      = x;
        m_renderer_info.canvas.y      = y;
        m_renderer_info.canvas.width  = width;
        m_renderer_info.canvas.height = height;

        create_textures_and_samplers();
        update_passes();
    }
}

void deferred_pbr_renderer::on_ui_widget()
{
    ImGui::PushID("deferred_pbr");
    const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding;
    custom_info("Renderer:", []() { ImGui::Text("Deferred PBR Renderer"); });
    bool changed = checkbox("VSync", &m_vsync, true);
    if (changed)
    {
        auto device_context = m_graphics_device->create_graphics_device_context();
        device_context->begin();
        device_context->set_swap_interval(m_vsync ? 1 : 0);
        device_context->end();
        device_context->submit();
    }
    changed |= checkbox("Frustum Culling", &m_frustum_culling, true);
    ImGui::Separator();
    bool has_environment_display = m_pipeline_extensions[mango::render_pipeline_extension::environment_display] != nullptr;
    bool has_shadow_map          = m_pipeline_extensions[mango::render_pipeline_extension::shadow_map] != nullptr;
    bool has_fxaa                = m_pipeline_extensions[mango::render_pipeline_extension::fxaa] != nullptr;
    bool has_gtao                = m_pipeline_extensions[mango::render_pipeline_extension::gtao] != nullptr;
    bool has_bloom                = m_pipeline_extensions[mango::render_pipeline_extension::bloom] != nullptr;
    if (ImGui::TreeNodeEx("Steps", flags | ImGuiTreeNodeFlags_Framed))
    {
        bool open = ImGui::CollapsingHeader("Environment Display", flags | ImGuiTreeNodeFlags_AllowItemOverlap | (!has_environment_display ? ImGuiTreeNodeFlags_Leaf : 0));
        ImGui::SameLine(ImGui::GetContentRegionAvail().x);
        ImGui::PushID("enable_environment_display");
        bool value_changed = ImGui::Checkbox("", &has_environment_display);
        ImGui::PopID();
        if (value_changed)
        {
            if (has_environment_display)
            {
                auto environment_display = std::make_shared<environment_display_pass>(environment_display_settings(0.0f)); // TODO Paul: Settings?
                environment_display->attach(m_shared_context);
                m_pipeline_extensions[mango::render_pipeline_extension::environment_display] = std::static_pointer_cast<render_pass>(environment_display);
            }
            else
            {
                m_pipeline_extensions[mango::render_pipeline_extension::environment_display] = nullptr;
            }
        }
        if (has_environment_display && open)
        {
            m_pipeline_extensions[mango::render_pipeline_extension::environment_display]->on_ui_widget();
        }
        changed |= value_changed;

        open = ImGui::CollapsingHeader("Shadow Step", flags | ImGuiTreeNodeFlags_AllowItemOverlap | (!has_shadow_map ? ImGuiTreeNodeFlags_Leaf : 0));
        ImGui::SameLine(ImGui::GetContentRegionAvail().x);
        ImGui::PushID("enable_shadow_pass");
        value_changed = ImGui::Checkbox("", &has_shadow_map);
        ImGui::PopID();
        if (value_changed)
        {
            if (has_shadow_map)
            {
                auto pass_shadow_map = std::make_shared<shadow_map_pass>(shadow_settings());
                pass_shadow_map->setup(m_pipeline_cache, m_debug_drawer);
                pass_shadow_map->attach(m_shared_context);
                m_pipeline_extensions[mango::render_pipeline_extension::shadow_map] = std::static_pointer_cast<render_pass>(pass_shadow_map);
                m_renderer_data.shadow_pass_enabled                                 = true;
            }
            else
            {
                m_pipeline_extensions[mango::render_pipeline_extension::shadow_map] = nullptr;
                m_renderer_data.shadow_pass_enabled                                 = false;
            }
        }
        if (has_shadow_map && open)
        {
            m_pipeline_extensions[mango::render_pipeline_extension::shadow_map]->on_ui_widget();
        }
        changed |= value_changed;

        open = ImGui::CollapsingHeader("FXAA Step", flags | ImGuiTreeNodeFlags_AllowItemOverlap | (!has_fxaa ? ImGuiTreeNodeFlags_Leaf : 0));
        ImGui::SameLine(ImGui::GetContentRegionAvail().x);
        ImGui::PushID("enable_fxaa_pass");
        value_changed = ImGui::Checkbox("", &has_fxaa);
        ImGui::PopID();
        if (value_changed)
        {
            if (has_fxaa)
            {
                auto pass_fxaa = std::make_shared<fxaa_pass>(fxaa_settings(0.75f));
                pass_fxaa->attach(m_shared_context);
                m_pipeline_extensions[mango::render_pipeline_extension::fxaa] = std::static_pointer_cast<render_pass>(pass_fxaa);
            }
            else
            {
                m_pipeline_extensions[mango::render_pipeline_extension::fxaa] = nullptr;
            }
        }
        if (has_fxaa && open)
        {
            m_pipeline_extensions[mango::render_pipeline_extension::fxaa]->on_ui_widget();
        }
        changed |= value_changed;

        open = ImGui::CollapsingHeader("GTAO Step", flags | ImGuiTreeNodeFlags_AllowItemOverlap | (!has_gtao ? ImGuiTreeNodeFlags_Leaf : 0));
        ImGui::SameLine(ImGui::GetContentRegionAvail().x);
        ImGui::PushID("enable_gtao_pass");
        value_changed = ImGui::Checkbox("", &has_gtao);
        ImGui::PopID();
        if (value_changed)
        {
            if (has_gtao)
            {
                auto pass_gtao = std::make_shared<gtao_pass>(gtao_settings());
                pass_gtao->attach(m_shared_context);
                m_pipeline_extensions[mango::render_pipeline_extension::gtao] = std::static_pointer_cast<render_pass>(pass_gtao);
            }
            else
            {
                m_pipeline_extensions[mango::render_pipeline_extension::gtao] = nullptr;
            }
        }
        if (has_gtao && open)
        {
            m_pipeline_extensions[mango::render_pipeline_extension::gtao]->on_ui_widget();
        }
        changed |= value_changed;

        open = ImGui::CollapsingHeader("Bloom Step", flags | ImGuiTreeNodeFlags_AllowItemOverlap | (!has_bloom ? ImGuiTreeNodeFlags_Leaf : 0));
        ImGui::SameLine(ImGui::GetContentRegionAvail().x);
        ImGui::PushID("enable_bloom_pass");
        value_changed = ImGui::Checkbox("", &has_bloom);
        ImGui::PopID();
        if (value_changed)
        {
            if (has_bloom)
            {
                auto pass_bloom = std::make_shared<bloom_pass>(bloom_settings());
                pass_bloom->attach(m_shared_context);
                m_pipeline_extensions[mango::render_pipeline_extension::bloom] = std::static_pointer_cast<render_pass>(pass_bloom);
            }
            else
            {
                m_pipeline_extensions[mango::render_pipeline_extension::bloom] = nullptr;
            }
        }
        if (has_bloom && open)
        {
            m_pipeline_extensions[mango::render_pipeline_extension::bloom]->on_ui_widget();
        }
        changed |= value_changed;

        ImGui::TreePop();
    }
    const char* debug[10]      = { "Default", "Position", "Normal", "Depth", "Base Color", "Reflection Color", "Emission", "Occlusion", "Roughness", "Metallic" };
    static int32 current_debug = 0;
    if (ImGui::CollapsingHeader("Debug", flags))
    {
        changed |= checkbox("Render Wireframe", &m_wireframe, false);
        changed |= checkbox("Debug Bounds", &m_debug_bounds, false);

        m_renderer_data.debug_view_enabled          = false;
        m_renderer_data.position_debug_view         = false;
        m_renderer_data.normal_debug_view           = false;
        m_renderer_data.depth_debug_view            = false;
        m_renderer_data.base_color_debug_view       = false;
        m_renderer_data.reflection_color_debug_view = false;
        m_renderer_data.emission_debug_view         = false;
        m_renderer_data.occlusion_debug_view        = false;
        m_renderer_data.roughness_debug_view        = false;
        m_renderer_data.metallic_debug_view         = false;

        int32 idx = current_debug;
        combo("Debug Views", debug, 10, idx, 0);
        current_debug = idx;

        if (current_debug)
        {
            *(&m_renderer_data.debug_view_enabled + current_debug) = true;
            m_renderer_data.debug_view_enabled                     = true;
        }
        ImGui::Separator();

        if (has_shadow_map)
        {
            bool casc = m_renderer_data.show_cascades;
            checkbox("Show Cascades", &casc, false);
            m_renderer_data.show_cascades = casc;
        }
    }
    ImGui::PopID();

    if (changed)
        update_passes();
}

float deferred_pbr_renderer::get_average_luminance() const
{
    return m_auto_luminance_pass.get_average_luminance();
}