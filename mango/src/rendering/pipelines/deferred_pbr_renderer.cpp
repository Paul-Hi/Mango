//! \file      deferred_pbr_renderer.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2022
//! \copyright Apache License 2.0

#include <glad/glad.h>
#include <mango/imgui_helper.hpp>
#include <mango/profile.hpp>
#include <rendering/pipelines/deferred_pbr_renderer.hpp>
#include <rendering/steps/environment_display_step.hpp>
#include <rendering/steps/fxaa_step.hpp>
#include <rendering/steps/shadow_map_step.hpp>
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
    , m_pipeline_cache(context)
    , m_frame_context(nullptr)
    , m_debug_drawer(context)
    , m_debug_bounds(false)
    , m_graphics_device(m_shared_context->get_graphics_device())
{
    PROFILE_ZONE;

    m_frame_context = m_graphics_device->create_graphics_device_context();

    if (!create_renderer_resources())
    {
        MANGO_LOG_ERROR("Resource Creation Failed! Renderer is not available!");
        return;
    }

    m_renderer_data.shadow_step_enabled         = false;
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

    // additional render steps
    const bool* render_steps = configuration.get_render_steps();
    if (render_steps[mango::render_pipeline_step::environment_display])
    {
        // create an extra object that is capable to render environment cubemaps.
        auto environment_display = std::make_shared<environment_display_step>(configuration.get_environment_display_settings());
        environment_display->attach(m_shared_context);
        m_pipeline_steps[mango::render_pipeline_step::environment_display] = std::static_pointer_cast<render_step>(environment_display);
    }
    if (configuration.get_render_steps()[mango::render_pipeline_step::shadow_map])
    {
        auto step_shadow_map = std::make_shared<shadow_map_step>(configuration.get_shadow_settings());
        step_shadow_map->attach(m_shared_context);
        m_pipeline_cache.set_shadow_base(step_shadow_map->get_shadow_pass_pipeline_base());
        m_pipeline_steps[mango::render_pipeline_step::shadow_map] = std::static_pointer_cast<render_step>(step_shadow_map);
        m_renderer_data.shadow_step_enabled                       = true;
    }
    if (configuration.get_render_steps()[mango::render_pipeline_step::fxaa])
    {
        auto step_fxaa = std::make_shared<fxaa_step>(configuration.get_fxaa_settings());
        step_fxaa->attach(m_shared_context);
        m_pipeline_steps[mango::render_pipeline_step::fxaa] = std::static_pointer_cast<render_step>(step_fxaa);
    }
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

    // Shaders
    if (!create_shader_stages())
        return false;

    // Pipeline prebuild stuff
    if (!create_pipeline_resources())
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

    auto fxaa_pass = std::static_pointer_cast<fxaa_step>(m_pipeline_steps[mango::render_pipeline_step::fxaa]);
    if (fxaa_pass)
        fxaa_pass->set_output_targets(m_output_target, m_ouput_depth_target);

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

    buffer_info.buffer_target = gfx_buffer_target::buffer_target_shader_storage;
    buffer_info.buffer_access = gfx_buffer_access::buffer_access_mapped_access_read_write;
    buffer_info.size          = sizeof(luminance_data);
    m_luminance_data_buffer   = m_graphics_device->create_buffer(buffer_info);
    if (!check_creation(m_luminance_data_buffer.get(), "luminance data buffer"))
        return false;
    m_luminance_data_mapping                      = nullptr;
    graphics_device_context_handle device_context = m_graphics_device->create_graphics_device_context();
    device_context->begin();
    m_luminance_data_mapping = static_cast<luminance_data*>(device_context->map_buffer_data(m_luminance_data_buffer, 0, sizeof(luminance_data)));
    device_context->end();
    device_context->submit();
    if (!check_mapping(m_luminance_data_mapping, "luminance data buffer"))
        return false;

    memset(m_luminance_data_mapping, 0, sizeof(luminance_data));
    m_luminance_data_mapping->luminance = 1.0f;

    return true;
}

bool deferred_pbr_renderer::create_shader_stages()
{
    auto& internal_resources = m_shared_context->get_internal_resources();
    shader_stage_create_info shader_info;
    shader_resource_resource_description res_resource_desc;
    shader_source_description source_desc;

    // Geometry Pass Vertex Stage
    {
        res_resource_desc.path = "res/shader/forward/v_scene_gltf.glsl";
        res_resource_desc.defines.push_back({ "VERTEX", "" });
        const shader_resource* source = internal_resources->acquire(res_resource_desc);

        source_desc.entry_point = "main";
        source_desc.source      = source->source.c_str();
        source_desc.size        = static_cast<int32>(source->source.size());

        shader_info.stage         = gfx_shader_stage_type::shader_stage_vertex;
        shader_info.shader_source = source_desc;

        shader_info.resource_count = 2;

        shader_info.resources = { {
            { gfx_shader_stage_type::shader_stage_vertex, CAMERA_DATA_BUFFER_BINDING_POINT, "camera_data", gfx_shader_resource_type::shader_resource_constant_buffer, 1 },
            { gfx_shader_stage_type::shader_stage_vertex, MODEL_DATA_BUFFER_BINDING_POINT, "model_data", gfx_shader_resource_type::shader_resource_constant_buffer, 1 },
        } };

        m_geometry_pass_vertex = m_graphics_device->create_shader_stage(shader_info);
        if (!check_creation(m_geometry_pass_vertex.get(), "geometry pass vertex shader"))
            return false;

        res_resource_desc.defines.clear();
    }
    // Geometry Pass Fragement Stage
    {
        res_resource_desc.path = "res/shader/forward/f_scene_gltf.glsl";
        res_resource_desc.defines.push_back({ "GBUFFER_FRAGMENT", "" });
        const shader_resource* source = internal_resources->acquire(res_resource_desc);

        source_desc.entry_point = "main";
        source_desc.source      = source->source.c_str();
        source_desc.size        = static_cast<int32>(source->source.size());

        shader_info.stage         = gfx_shader_stage_type::shader_stage_fragment;
        shader_info.shader_source = source_desc;

        shader_info.resource_count = 11;

        shader_info.resources = { {
            { gfx_shader_stage_type::shader_stage_fragment, MATERIAL_DATA_BUFFER_BINDING_POINT, "material_data", gfx_shader_resource_type::shader_resource_constant_buffer, 1 },

            { gfx_shader_stage_type::shader_stage_fragment, GEOMETRY_TEXTURE_SAMPLER_BASE_COLOR, "texture_base_color", gfx_shader_resource_type::shader_resource_input_attachment, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, GEOMETRY_TEXTURE_SAMPLER_BASE_COLOR, "sampler_base_color", gfx_shader_resource_type::shader_resource_sampler, 1 },

            { gfx_shader_stage_type::shader_stage_fragment, GEOMETRY_TEXTURE_SAMPLER_ROUGHNESS_METALLIC, "texture_roughness_metallic", gfx_shader_resource_type::shader_resource_input_attachment, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, GEOMETRY_TEXTURE_SAMPLER_ROUGHNESS_METALLIC, "sampler_roughness_metallic", gfx_shader_resource_type::shader_resource_sampler, 1 },

            { gfx_shader_stage_type::shader_stage_fragment, GEOMETRY_TEXTURE_SAMPLER_OCCLUSION, "texture_occlusion", gfx_shader_resource_type::shader_resource_input_attachment, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, GEOMETRY_TEXTURE_SAMPLER_OCCLUSION, "sampler_occlusion", gfx_shader_resource_type::shader_resource_sampler, 1 },

            { gfx_shader_stage_type::shader_stage_fragment, GEOMETRY_TEXTURE_SAMPLER_NORMAL, "texture_normal", gfx_shader_resource_type::shader_resource_input_attachment, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, GEOMETRY_TEXTURE_SAMPLER_NORMAL, "sampler_normal", gfx_shader_resource_type::shader_resource_sampler, 1 },

            { gfx_shader_stage_type::shader_stage_fragment, GEOMETRY_TEXTURE_SAMPLER_EMISSIVE_COLOR, "texture_emissive_color", gfx_shader_resource_type::shader_resource_input_attachment, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, GEOMETRY_TEXTURE_SAMPLER_EMISSIVE_COLOR, "sampler_emissive_color", gfx_shader_resource_type::shader_resource_sampler, 1 },
        } };

        m_geometry_pass_fragment = m_graphics_device->create_shader_stage(shader_info);
        if (!check_creation(m_geometry_pass_fragment.get(), "geometry pass fragment shader"))
            return false;

        res_resource_desc.defines.clear();
    }
    // Transparent Forward Lighting Fragment Stage
    {
        res_resource_desc.path = "res/shader/forward/f_scene_transparent_gltf.glsl";
        res_resource_desc.defines.push_back({ "FORWARD_LIGHTING_FRAGMENT", "" });
        const shader_resource* source = internal_resources->acquire(res_resource_desc);

        source_desc.entry_point = "main";
        source_desc.source      = source->source.c_str();
        source_desc.size        = static_cast<int32>(source->source.size());

        shader_info.stage         = gfx_shader_stage_type::shader_stage_fragment;
        shader_info.shader_source = source_desc;

        shader_info.resource_count = 26;

        shader_info.resources = { {
            { gfx_shader_stage_type::shader_stage_fragment, MATERIAL_DATA_BUFFER_BINDING_POINT, "material_data", gfx_shader_resource_type::shader_resource_constant_buffer, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, CAMERA_DATA_BUFFER_BINDING_POINT, "camera_data", gfx_shader_resource_type::shader_resource_constant_buffer, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, MODEL_DATA_BUFFER_BINDING_POINT, "model_data", gfx_shader_resource_type::shader_resource_constant_buffer, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, RENDERER_DATA_BUFFER_BINDING_POINT, "renderer_data", gfx_shader_resource_type::shader_resource_constant_buffer, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, LIGHT_DATA_BUFFER_BINDING_POINT, "light_data", gfx_shader_resource_type::shader_resource_constant_buffer, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, SHADOW_DATA_BUFFER_BINDING_POINT, "shadow_data", gfx_shader_resource_type::shader_resource_constant_buffer, 1 },

            { gfx_shader_stage_type::shader_stage_fragment, GEOMETRY_TEXTURE_SAMPLER_BASE_COLOR, "texture_base_color", gfx_shader_resource_type::shader_resource_input_attachment, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, GEOMETRY_TEXTURE_SAMPLER_BASE_COLOR, "sampler_base_color", gfx_shader_resource_type::shader_resource_sampler, 1 },

            { gfx_shader_stage_type::shader_stage_fragment, GEOMETRY_TEXTURE_SAMPLER_ROUGHNESS_METALLIC, "texture_roughness_metallic", gfx_shader_resource_type::shader_resource_input_attachment, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, GEOMETRY_TEXTURE_SAMPLER_ROUGHNESS_METALLIC, "sampler_roughness_metallic", gfx_shader_resource_type::shader_resource_sampler, 1 },

            { gfx_shader_stage_type::shader_stage_fragment, GEOMETRY_TEXTURE_SAMPLER_OCCLUSION, "texture_occlusion", gfx_shader_resource_type::shader_resource_input_attachment, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, GEOMETRY_TEXTURE_SAMPLER_OCCLUSION, "sampler_occlusion", gfx_shader_resource_type::shader_resource_sampler, 1 },

            { gfx_shader_stage_type::shader_stage_fragment, GEOMETRY_TEXTURE_SAMPLER_NORMAL, "texture_normal", gfx_shader_resource_type::shader_resource_input_attachment, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, GEOMETRY_TEXTURE_SAMPLER_NORMAL, "sampler_normal", gfx_shader_resource_type::shader_resource_sampler, 1 },

            { gfx_shader_stage_type::shader_stage_fragment, GEOMETRY_TEXTURE_SAMPLER_EMISSIVE_COLOR, "texture_emissive_color", gfx_shader_resource_type::shader_resource_input_attachment, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, GEOMETRY_TEXTURE_SAMPLER_EMISSIVE_COLOR, "sampler_emissive_color", gfx_shader_resource_type::shader_resource_sampler, 1 },

            { gfx_shader_stage_type::shader_stage_fragment, IBL_SAMPLER_IRRADIANCE_MAP, "texture_irradiance_map", gfx_shader_resource_type::shader_resource_input_attachment, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, IBL_SAMPLER_IRRADIANCE_MAP, "sampler_irradiance_map", gfx_shader_resource_type::shader_resource_sampler, 1 },

            { gfx_shader_stage_type::shader_stage_fragment, IBL_SAMPLER_RADIANCE_MAP, "texture_radiance_map", gfx_shader_resource_type::shader_resource_input_attachment, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, IBL_SAMPLER_RADIANCE_MAP, "sampler_radiance_map", gfx_shader_resource_type::shader_resource_sampler, 1 },

            { gfx_shader_stage_type::shader_stage_fragment, IBL_SAMPLER_LOOKUP, "texture_brdf_integration_lut", gfx_shader_resource_type::shader_resource_input_attachment, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, IBL_SAMPLER_LOOKUP, "sampler_brdf_integration_lut", gfx_shader_resource_type::shader_resource_sampler, 1 },

            { gfx_shader_stage_type::shader_stage_fragment, SAMPLER_SHADOW_SHADOW_MAP, "texture_shadow_map_comp", gfx_shader_resource_type::shader_resource_input_attachment, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, SAMPLER_SHADOW_SHADOW_MAP, "sampler_shadow_shadow_map", gfx_shader_resource_type::shader_resource_sampler, 1 },

            { gfx_shader_stage_type::shader_stage_fragment, SAMPLER_SHADOW_MAP, "texture_shadow_map", gfx_shader_resource_type::shader_resource_input_attachment, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, SAMPLER_SHADOW_MAP, "sampler_shadow_map", gfx_shader_resource_type::shader_resource_sampler, 1 },
        } };

        m_transparent_pass_fragment = m_graphics_device->create_shader_stage(shader_info);
        if (!check_creation(m_transparent_pass_fragment.get(), "transparent pass fragment shader"))
            return false;

        res_resource_desc.defines.clear();
    }
    // Screen Space Quad for Compositing
    {
        res_resource_desc.path        = "res/shader/v_screen_space_triangle.glsl";
        const shader_resource* source = internal_resources->acquire(res_resource_desc);

        source_desc.entry_point = "main";
        source_desc.source      = source->source.c_str();
        source_desc.size        = static_cast<int32>(source->source.size());

        shader_info.stage         = gfx_shader_stage_type::shader_stage_vertex;
        shader_info.shader_source = source_desc;

        shader_info.resource_count = 0;

        m_screen_space_triangle_vertex = m_graphics_device->create_shader_stage(shader_info);
        if (!check_creation(m_screen_space_triangle_vertex.get(), "screen space triangle vertex shader"))
            return false;

        res_resource_desc.defines.clear();
    }
    // Composing Pass Fragment Stage
    {
        res_resource_desc.path = "res/shader/post/f_composing.glsl";
        res_resource_desc.defines.push_back({ "COMPOSING", "" });
        const shader_resource* source = internal_resources->acquire(res_resource_desc);

        source_desc.entry_point = "main";
        source_desc.source      = source->source.c_str();
        source_desc.size        = static_cast<int32>(source->source.size());

        shader_info.stage         = gfx_shader_stage_type::shader_stage_fragment;
        shader_info.shader_source = source_desc;

        shader_info.resource_count = 6;

        shader_info.resources = { {
            { gfx_shader_stage_type::shader_stage_fragment, CAMERA_DATA_BUFFER_BINDING_POINT, "camera_data", gfx_shader_resource_type::shader_resource_constant_buffer, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, RENDERER_DATA_BUFFER_BINDING_POINT, "renderer_data", gfx_shader_resource_type::shader_resource_constant_buffer, 1 },

            { gfx_shader_stage_type::shader_stage_fragment, COMPOSING_HDR_SAMPLER, "texture_hdr_input", gfx_shader_resource_type::shader_resource_input_attachment, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, COMPOSING_HDR_SAMPLER, "sampler_hdr_input", gfx_shader_resource_type::shader_resource_sampler, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, COMPOSING_DEPTH_SAMPLER, "texture_geometry_depth_input", gfx_shader_resource_type::shader_resource_input_attachment, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, COMPOSING_DEPTH_SAMPLER, "sampler_geometry_depth_input", gfx_shader_resource_type::shader_resource_sampler, 1 },
        } };

        m_composing_pass_fragment = m_graphics_device->create_shader_stage(shader_info);
        if (!check_creation(m_composing_pass_fragment.get(), "composing pass fragment shader"))
            return false;

        res_resource_desc.defines.clear();
    }
    // Luminance Construction Compute Stage
    {
        res_resource_desc.path = "res/shader/luminance_compute/c_construct_luminance_buffer.glsl";
        res_resource_desc.defines.push_back({ "COMPUTE", "" });
        const shader_resource* source = internal_resources->acquire(res_resource_desc);

        source_desc.entry_point = "main";
        source_desc.source      = source->source.c_str();
        source_desc.size        = static_cast<int32>(source->source.size());

        shader_info.stage         = gfx_shader_stage_type::shader_stage_compute;
        shader_info.shader_source = source_desc;

        shader_info.resource_count = 2;

        shader_info.resources = { {
            { gfx_shader_stage_type::shader_stage_compute, LUMINANCE_DATA_BUFFER_BINDING_POINT, "luminance_data", gfx_shader_resource_type::shader_resource_buffer_storage, 1 },
            { gfx_shader_stage_type::shader_stage_compute, HDR_IMAGE_LUMINANCE_COMPUTE, "image_hdr_color", gfx_shader_resource_type::shader_resource_image_storage, 1 },
        } };

        m_luminance_construction_compute = m_graphics_device->create_shader_stage(shader_info);
        if (!check_creation(m_luminance_construction_compute.get(), "luminance construction compute shader"))
            return false;

        res_resource_desc.defines.clear();
    }
    // Luminance Reduction Compute Stage
    {
        res_resource_desc.path = "res/shader/luminance_compute/c_luminance_buffer_reduction.glsl";
        res_resource_desc.defines.push_back({ "COMPUTE", "" });
        const shader_resource* source = internal_resources->acquire(res_resource_desc);

        source_desc.entry_point = "main";
        source_desc.source      = source->source.c_str();
        source_desc.size        = static_cast<int32>(source->source.size());

        shader_info.stage         = gfx_shader_stage_type::shader_stage_compute;
        shader_info.shader_source = source_desc;

        shader_info.resource_count = 1;

        shader_info.resources = { {
            { gfx_shader_stage_type::shader_stage_compute, LUMINANCE_DATA_BUFFER_BINDING_POINT, "luminance_data", gfx_shader_resource_type::shader_resource_buffer_storage, 1 },
        } };

        m_luminance_reduction_compute = m_graphics_device->create_shader_stage(shader_info);
        if (!check_creation(m_luminance_reduction_compute.get(), "luminance reduction compute shader"))
            return false;

        res_resource_desc.defines.clear();
    }

    return true;
}

bool deferred_pbr_renderer::create_pipeline_resources()
{
    // Geometry Pass Pipeline
    {
        graphics_pipeline_create_info geometry_pass_info = m_graphics_device->provide_graphics_pipeline_create_info();
        auto geometry_pass_pipeline_layout               = m_graphics_device->create_pipeline_resource_layout({
                          { gfx_shader_stage_type::shader_stage_vertex, CAMERA_DATA_BUFFER_BINDING_POINT, gfx_shader_resource_type::shader_resource_constant_buffer,
                            gfx_shader_resource_access::shader_access_dynamic },
                          { gfx_shader_stage_type::shader_stage_vertex, MODEL_DATA_BUFFER_BINDING_POINT, gfx_shader_resource_type::shader_resource_constant_buffer,
                            gfx_shader_resource_access::shader_access_dynamic },

                          { gfx_shader_stage_type::shader_stage_fragment, MATERIAL_DATA_BUFFER_BINDING_POINT, gfx_shader_resource_type::shader_resource_constant_buffer,
                            gfx_shader_resource_access::shader_access_dynamic },

                          { gfx_shader_stage_type::shader_stage_fragment, GEOMETRY_TEXTURE_SAMPLER_BASE_COLOR, gfx_shader_resource_type::shader_resource_input_attachment,
                            gfx_shader_resource_access::shader_access_dynamic },
                          { gfx_shader_stage_type::shader_stage_fragment, GEOMETRY_TEXTURE_SAMPLER_BASE_COLOR, gfx_shader_resource_type::shader_resource_sampler, gfx_shader_resource_access::shader_access_dynamic },

                          { gfx_shader_stage_type::shader_stage_fragment, GEOMETRY_TEXTURE_SAMPLER_ROUGHNESS_METALLIC, gfx_shader_resource_type::shader_resource_input_attachment,
                            gfx_shader_resource_access::shader_access_dynamic },
                          { gfx_shader_stage_type::shader_stage_fragment, GEOMETRY_TEXTURE_SAMPLER_ROUGHNESS_METALLIC, gfx_shader_resource_type::shader_resource_sampler,
                            gfx_shader_resource_access::shader_access_dynamic },

                          { gfx_shader_stage_type::shader_stage_fragment, GEOMETRY_TEXTURE_SAMPLER_OCCLUSION, gfx_shader_resource_type::shader_resource_input_attachment,
                            gfx_shader_resource_access::shader_access_dynamic },
                          { gfx_shader_stage_type::shader_stage_fragment, GEOMETRY_TEXTURE_SAMPLER_OCCLUSION, gfx_shader_resource_type::shader_resource_sampler, gfx_shader_resource_access::shader_access_dynamic },

                          { gfx_shader_stage_type::shader_stage_fragment, GEOMETRY_TEXTURE_SAMPLER_NORMAL, gfx_shader_resource_type::shader_resource_input_attachment,
                            gfx_shader_resource_access::shader_access_dynamic },
                          { gfx_shader_stage_type::shader_stage_fragment, GEOMETRY_TEXTURE_SAMPLER_NORMAL, gfx_shader_resource_type::shader_resource_sampler, gfx_shader_resource_access::shader_access_dynamic },

                          { gfx_shader_stage_type::shader_stage_fragment, GEOMETRY_TEXTURE_SAMPLER_EMISSIVE_COLOR, gfx_shader_resource_type::shader_resource_input_attachment,
                            gfx_shader_resource_access::shader_access_dynamic },
                          { gfx_shader_stage_type::shader_stage_fragment, GEOMETRY_TEXTURE_SAMPLER_EMISSIVE_COLOR, gfx_shader_resource_type::shader_resource_sampler,
                            gfx_shader_resource_access::shader_access_dynamic },
        });

        geometry_pass_info.pipeline_layout = geometry_pass_pipeline_layout;

        geometry_pass_info.shader_stage_descriptor.vertex_shader_stage   = m_geometry_pass_vertex;
        geometry_pass_info.shader_stage_descriptor.fragment_shader_stage = m_geometry_pass_fragment;

        // vertex_input_descriptor comes from the mesh to render.
        // input_assembly_descriptor comes from the mesh to render.

        // viewport_descriptor is dynamic

        // rasterization_state -> keep default
        // depth_stencil_state -> keep default
        // blend_state -> keep default

        geometry_pass_info.dynamic_state.dynamic_states = gfx_dynamic_state_flag_bits::dynamic_state_viewport | gfx_dynamic_state_flag_bits::dynamic_state_scissor;

        m_pipeline_cache.set_opaque_base(geometry_pass_info);
    }
    // Transparent Pass Pipeline
    {
        graphics_pipeline_create_info transparent_pass_info = m_graphics_device->provide_graphics_pipeline_create_info();
        auto transparent_pass_pipeline_layout               = m_graphics_device->create_pipeline_resource_layout({
                          { gfx_shader_stage_type::shader_stage_vertex, CAMERA_DATA_BUFFER_BINDING_POINT, gfx_shader_resource_type::shader_resource_constant_buffer,
                            gfx_shader_resource_access::shader_access_dynamic },
                          { gfx_shader_stage_type::shader_stage_vertex, MODEL_DATA_BUFFER_BINDING_POINT, gfx_shader_resource_type::shader_resource_constant_buffer,
                            gfx_shader_resource_access::shader_access_dynamic },

                          { gfx_shader_stage_type::shader_stage_fragment, MATERIAL_DATA_BUFFER_BINDING_POINT, gfx_shader_resource_type::shader_resource_constant_buffer,
                            gfx_shader_resource_access::shader_access_dynamic },
                          { gfx_shader_stage_type::shader_stage_fragment, CAMERA_DATA_BUFFER_BINDING_POINT, gfx_shader_resource_type::shader_resource_constant_buffer,
                            gfx_shader_resource_access::shader_access_dynamic },
                          { gfx_shader_stage_type::shader_stage_fragment, MODEL_DATA_BUFFER_BINDING_POINT, gfx_shader_resource_type::shader_resource_constant_buffer,
                            gfx_shader_resource_access::shader_access_dynamic },
                          { gfx_shader_stage_type::shader_stage_fragment, RENDERER_DATA_BUFFER_BINDING_POINT, gfx_shader_resource_type::shader_resource_constant_buffer,
                            gfx_shader_resource_access::shader_access_dynamic },
                          { gfx_shader_stage_type::shader_stage_fragment, LIGHT_DATA_BUFFER_BINDING_POINT, gfx_shader_resource_type::shader_resource_constant_buffer,
                            gfx_shader_resource_access::shader_access_dynamic },
                          { gfx_shader_stage_type::shader_stage_fragment, SHADOW_DATA_BUFFER_BINDING_POINT, gfx_shader_resource_type::shader_resource_constant_buffer,
                            gfx_shader_resource_access::shader_access_dynamic },

                          { gfx_shader_stage_type::shader_stage_fragment, GEOMETRY_TEXTURE_SAMPLER_BASE_COLOR, gfx_shader_resource_type::shader_resource_input_attachment,
                            gfx_shader_resource_access::shader_access_dynamic },
                          { gfx_shader_stage_type::shader_stage_fragment, GEOMETRY_TEXTURE_SAMPLER_BASE_COLOR, gfx_shader_resource_type::shader_resource_sampler, gfx_shader_resource_access::shader_access_dynamic },

                          { gfx_shader_stage_type::shader_stage_fragment, GEOMETRY_TEXTURE_SAMPLER_ROUGHNESS_METALLIC, gfx_shader_resource_type::shader_resource_input_attachment,
                            gfx_shader_resource_access::shader_access_dynamic },
                          { gfx_shader_stage_type::shader_stage_fragment, GEOMETRY_TEXTURE_SAMPLER_ROUGHNESS_METALLIC, gfx_shader_resource_type::shader_resource_sampler,
                            gfx_shader_resource_access::shader_access_dynamic },

                          { gfx_shader_stage_type::shader_stage_fragment, GEOMETRY_TEXTURE_SAMPLER_OCCLUSION, gfx_shader_resource_type::shader_resource_input_attachment,
                            gfx_shader_resource_access::shader_access_dynamic },
                          { gfx_shader_stage_type::shader_stage_fragment, GEOMETRY_TEXTURE_SAMPLER_OCCLUSION, gfx_shader_resource_type::shader_resource_sampler, gfx_shader_resource_access::shader_access_dynamic },

                          { gfx_shader_stage_type::shader_stage_fragment, GEOMETRY_TEXTURE_SAMPLER_NORMAL, gfx_shader_resource_type::shader_resource_input_attachment,
                            gfx_shader_resource_access::shader_access_dynamic },
                          { gfx_shader_stage_type::shader_stage_fragment, GEOMETRY_TEXTURE_SAMPLER_NORMAL, gfx_shader_resource_type::shader_resource_sampler, gfx_shader_resource_access::shader_access_dynamic },

                          { gfx_shader_stage_type::shader_stage_fragment, GEOMETRY_TEXTURE_SAMPLER_EMISSIVE_COLOR, gfx_shader_resource_type::shader_resource_input_attachment,
                            gfx_shader_resource_access::shader_access_dynamic },
                          { gfx_shader_stage_type::shader_stage_fragment, GEOMETRY_TEXTURE_SAMPLER_EMISSIVE_COLOR, gfx_shader_resource_type::shader_resource_sampler,
                            gfx_shader_resource_access::shader_access_dynamic },

                          { gfx_shader_stage_type::shader_stage_fragment, IBL_SAMPLER_IRRADIANCE_MAP, gfx_shader_resource_type::shader_resource_input_attachment, gfx_shader_resource_access::shader_access_dynamic },
                          { gfx_shader_stage_type::shader_stage_fragment, IBL_SAMPLER_IRRADIANCE_MAP, gfx_shader_resource_type::shader_resource_sampler, gfx_shader_resource_access::shader_access_dynamic },

                          { gfx_shader_stage_type::shader_stage_fragment, IBL_SAMPLER_RADIANCE_MAP, gfx_shader_resource_type::shader_resource_input_attachment, gfx_shader_resource_access::shader_access_dynamic },
                          { gfx_shader_stage_type::shader_stage_fragment, IBL_SAMPLER_RADIANCE_MAP, gfx_shader_resource_type::shader_resource_sampler, gfx_shader_resource_access::shader_access_dynamic },

                          { gfx_shader_stage_type::shader_stage_fragment, IBL_SAMPLER_LOOKUP, gfx_shader_resource_type::shader_resource_input_attachment, gfx_shader_resource_access::shader_access_dynamic },
                          { gfx_shader_stage_type::shader_stage_fragment, IBL_SAMPLER_LOOKUP, gfx_shader_resource_type::shader_resource_sampler, gfx_shader_resource_access::shader_access_dynamic },

                          { gfx_shader_stage_type::shader_stage_fragment, SAMPLER_SHADOW_SHADOW_MAP, gfx_shader_resource_type::shader_resource_input_attachment, gfx_shader_resource_access::shader_access_dynamic },
                          { gfx_shader_stage_type::shader_stage_fragment, SAMPLER_SHADOW_SHADOW_MAP, gfx_shader_resource_type::shader_resource_sampler, gfx_shader_resource_access::shader_access_dynamic },

                          { gfx_shader_stage_type::shader_stage_fragment, SAMPLER_SHADOW_MAP, gfx_shader_resource_type::shader_resource_input_attachment, gfx_shader_resource_access::shader_access_dynamic },
                          { gfx_shader_stage_type::shader_stage_fragment, SAMPLER_SHADOW_MAP, gfx_shader_resource_type::shader_resource_sampler, gfx_shader_resource_access::shader_access_dynamic },
        });

        transparent_pass_info.pipeline_layout = transparent_pass_pipeline_layout;

        transparent_pass_info.shader_stage_descriptor.vertex_shader_stage   = m_geometry_pass_vertex;
        transparent_pass_info.shader_stage_descriptor.fragment_shader_stage = m_transparent_pass_fragment;

        // vertex_input_descriptor comes from the mesh to render.
        // input_assembly_descriptor comes from the mesh to render.

        // viewport_descriptor is dynamic

        // rasterization_state -> keep default
        // depth_stencil_state -> keep default
        transparent_pass_info.rasterization_state.cull_mode                       = gfx_cull_mode_flag_bits::mode_back; // TODO Paul: For now this is fine.
        transparent_pass_info.blend_state.blend_description.enable_blend          = true;
        transparent_pass_info.blend_state.blend_description.color_blend_operation = gfx_blend_operation::blend_operation_add;
        transparent_pass_info.blend_state.blend_description.color_blend_operation = gfx_blend_operation::blend_operation_add;
        transparent_pass_info.blend_state.blend_description.alpha_blend_operation = gfx_blend_operation::blend_operation_add;

        transparent_pass_info.blend_state.blend_description.src_color_blend_factor = gfx_blend_factor::blend_factor_src_alpha;
        transparent_pass_info.blend_state.blend_description.dst_color_blend_factor = gfx_blend_factor::blend_factor_one_minus_src_alpha;
        transparent_pass_info.blend_state.blend_description.src_alpha_blend_factor = gfx_blend_factor::blend_factor_one;
        transparent_pass_info.blend_state.blend_description.dst_alpha_blend_factor = gfx_blend_factor::blend_factor_one_minus_src_alpha;

        transparent_pass_info.dynamic_state.dynamic_states = gfx_dynamic_state_flag_bits::dynamic_state_viewport | gfx_dynamic_state_flag_bits::dynamic_state_scissor;

        m_pipeline_cache.set_transparent_base(transparent_pass_info);
    }
    // Composing Pass Pipeline
    {
        graphics_pipeline_create_info composing_pass_info = m_graphics_device->provide_graphics_pipeline_create_info();
        auto composing_pass_pipeline_layout               = m_graphics_device->create_pipeline_resource_layout({
                          { gfx_shader_stage_type::shader_stage_fragment, CAMERA_DATA_BUFFER_BINDING_POINT, gfx_shader_resource_type::shader_resource_constant_buffer,
                            gfx_shader_resource_access::shader_access_dynamic },
                          { gfx_shader_stage_type::shader_stage_fragment, RENDERER_DATA_BUFFER_BINDING_POINT, gfx_shader_resource_type::shader_resource_constant_buffer,
                            gfx_shader_resource_access::shader_access_dynamic },

                          { gfx_shader_stage_type::shader_stage_fragment, COMPOSING_HDR_SAMPLER, gfx_shader_resource_type::shader_resource_input_attachment, gfx_shader_resource_access::shader_access_dynamic },
                          { gfx_shader_stage_type::shader_stage_fragment, COMPOSING_HDR_SAMPLER, gfx_shader_resource_type::shader_resource_sampler, gfx_shader_resource_access::shader_access_dynamic },
                          { gfx_shader_stage_type::shader_stage_fragment, COMPOSING_DEPTH_SAMPLER, gfx_shader_resource_type::shader_resource_input_attachment, gfx_shader_resource_access::shader_access_dynamic },
                          { gfx_shader_stage_type::shader_stage_fragment, COMPOSING_DEPTH_SAMPLER, gfx_shader_resource_type::shader_resource_sampler, gfx_shader_resource_access::shader_access_dynamic },
        });

        composing_pass_info.pipeline_layout = composing_pass_pipeline_layout;

        composing_pass_info.shader_stage_descriptor.vertex_shader_stage   = m_screen_space_triangle_vertex;
        composing_pass_info.shader_stage_descriptor.fragment_shader_stage = m_composing_pass_fragment;

        composing_pass_info.vertex_input_state.attribute_description_count = 0;
        composing_pass_info.vertex_input_state.binding_description_count   = 0;

        composing_pass_info.input_assembly_state.topology = gfx_primitive_topology::primitive_topology_triangle_list; // Not relevant.

        // viewport_descriptor is dynamic

        // rasterization_state -> keep default
        composing_pass_info.depth_stencil_state.depth_compare_operator = gfx_compare_operator::compare_operator_always; // Do not disable since it writes the depth in the fragment shader.
        // blend_state -> keep default

        composing_pass_info.dynamic_state.dynamic_states = gfx_dynamic_state_flag_bits::dynamic_state_viewport | gfx_dynamic_state_flag_bits::dynamic_state_scissor;

        m_composing_pass_pipeline = m_graphics_device->create_graphics_pipeline(composing_pass_info);
    }
    // Luminance Construction Pipeline
    {
        compute_pipeline_create_info construction_pass_info = m_graphics_device->provide_compute_pipeline_create_info();
        auto construction_pass_pipeline_layout              = m_graphics_device->create_pipeline_resource_layout({
                         { gfx_shader_stage_type::shader_stage_compute, LUMINANCE_DATA_BUFFER_BINDING_POINT, gfx_shader_resource_type::shader_resource_buffer_storage,
                           gfx_shader_resource_access::shader_access_dynamic },

                         { gfx_shader_stage_type::shader_stage_compute, HDR_IMAGE_LUMINANCE_COMPUTE, gfx_shader_resource_type::shader_resource_image_storage, gfx_shader_resource_access::shader_access_dynamic },
        });

        construction_pass_info.pipeline_layout = construction_pass_pipeline_layout;

        construction_pass_info.shader_stage_descriptor.compute_shader_stage = m_luminance_construction_compute;

        m_luminance_construction_pipeline = m_graphics_device->create_compute_pipeline(construction_pass_info);
    }
    // Luminance Reduction Pipeline
    {
        compute_pipeline_create_info reduction_pass_info = m_graphics_device->provide_compute_pipeline_create_info();
        auto reduction_pass_pipeline_layout              = m_graphics_device->create_pipeline_resource_layout({
                         { gfx_shader_stage_type::shader_stage_compute, LUMINANCE_DATA_BUFFER_BINDING_POINT, gfx_shader_resource_type::shader_resource_buffer_storage,
                           gfx_shader_resource_access::shader_access_dynamic },
        });

        reduction_pass_info.pipeline_layout = reduction_pass_pipeline_layout;

        reduction_pass_info.shader_stage_descriptor.compute_shader_stage = m_luminance_reduction_compute;

        m_luminance_reduction_pipeline = m_graphics_device->create_compute_pipeline(reduction_pass_info);
    }

    return true; // TODO Paul: This is always true atm.
}

bool deferred_pbr_renderer::create_passes()
{
    m_deferred_lighting_pass.attach(m_shared_context);

    return update_passes();
}

bool deferred_pbr_renderer::update_passes()
{
    gfx_viewport window_viewport{ static_cast<float>(m_renderer_info.canvas.x), static_cast<float>(m_renderer_info.canvas.y), static_cast<float>(m_renderer_info.canvas.width),
                                  static_cast<float>(m_renderer_info.canvas.height) };

    m_deferred_lighting_pass.set_viewport(window_viewport);
    m_deferred_lighting_pass.set_render_targets(m_hdr_buffer_render_targets);
    m_deferred_lighting_pass.set_gbuffer(m_gbuffer_render_targets, m_linear_sampler);
    m_deferred_lighting_pass.set_renderer_data_buffer(m_renderer_data_buffer);
    m_deferred_lighting_pass.set_irradiance_map_sampler(m_mipmapped_linear_sampler);
    m_deferred_lighting_pass.set_radiance_map_sampler(m_mipmapped_linear_sampler);
    m_deferred_lighting_pass.set_brdf_integration_lut_sampler(m_linear_sampler);
    m_deferred_lighting_pass.set_shadow_map_sampler(m_linear_sampler);
    m_deferred_lighting_pass.set_shadow_map_compare_sampler(m_linear_compare_sampler);

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
    m_renderer_info.last_frame.triangles  = 0;
    m_renderer_info.last_frame.meshes     = 0;
    m_renderer_info.last_frame.primitives = 0;
    m_renderer_info.last_frame.materials  = 0;

    m_frame_context->begin();
    // m_frame_context->client_wait(m_frame_semaphore);
    float clear_color[4] = { 0.1f, 0.1f, 0.1f, 1.0f }; // TODO Paul: member or dynamic?
    auto swap_buffer     = m_graphics_device->get_swap_chain_render_target();

    auto shadow_pass = std::static_pointer_cast<shadow_map_step>(m_pipeline_steps[mango::render_pipeline_step::shadow_map]);

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

    struct draw_key
    {
        key primitive_gpu_data_id;
        key mesh_gpu_data_id;
        handle<material> material_hnd;
        float view_depth;
        bool transparent;
        axis_aligned_bounding_box bounding_box; // Does not contribute to order.

        bool operator<(const draw_key& other) const
        {
            if (transparent < other.transparent)
                return true;
            if (other.transparent < transparent)
                return false;

            if (view_depth > other.view_depth)
                return true;
            if (other.view_depth > view_depth)
                return false;

            if (material_hnd < other.material_hnd)
                return true;
            if (other.material_hnd < material_hnd)
                return false;

            if (primitive_gpu_data_id < other.primitive_gpu_data_id)
                return true;
            if (other.primitive_gpu_data_id < primitive_gpu_data_id)
                return false;

            return false;
        }
    };

    auto active_camera_data = scene->get_active_camera_gpu_data();
    if (!active_camera_data.has_value())
        return;

    std::vector<draw_key> draws;
    int32 opaque_count = 0;
    auto instances     = scene->get_render_instances();
    if (m_debug_bounds)
        m_debug_drawer.clear();
    bounding_frustum camera_frustum;
    if (m_frustum_culling)
    {
        camera_frustum = bounding_frustum(active_camera_data->per_camera_data.view_matrix, active_camera_data->per_camera_data.projection_matrix);
        if (m_debug_bounds)
        {
            auto corners = bounding_frustum::get_corners(active_camera_data->per_camera_data.view_projection_matrix);
            m_debug_drawer.set_color(color_rgb(0.0f, 1.0f, 0.0f));
            m_debug_drawer.add(corners[0], corners[1]);
            m_debug_drawer.add(corners[1], corners[3]);
            m_debug_drawer.add(corners[3], corners[2]);
            m_debug_drawer.add(corners[2], corners[6]);
            m_debug_drawer.add(corners[6], corners[4]);
            m_debug_drawer.add(corners[4], corners[0]);
            m_debug_drawer.add(corners[0], corners[2]);

            m_debug_drawer.add(corners[5], corners[4]);
            m_debug_drawer.add(corners[4], corners[6]);
            m_debug_drawer.add(corners[6], corners[7]);
            m_debug_drawer.add(corners[7], corners[3]);
            m_debug_drawer.add(corners[3], corners[1]);
            m_debug_drawer.add(corners[1], corners[5]);
            m_debug_drawer.add(corners[5], corners[7]);
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

            m_renderer_info.last_frame.meshes++;

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

                draws.push_back(a_draw);
            }
        }
    }

    std::sort(begin(draws), end(draws));

    auto warn_missing_draw = [](string what) { MANGO_LOG_WARN("{0} missing for draw. Skipping DrawCall!", what); };

    const light_stack& ls = scene->get_light_stack();
    auto light_data       = scene->get_light_gpu_data();

    // shadow pass
    // draw objects
    {
        GL_NAMED_PROFILE_ZONE("Shadow Pass");
        NAMED_PROFILE_ZONE("Shadow Pass");
        auto shadow_casters = ls.get_shadow_casters(); // currently only directional.

        if (shadow_pass && !m_renderer_data.debug_view_enabled && !shadow_casters.empty())
        {
            for (auto sc : shadow_casters)
            {
                shadow_pass->update_cascades(dt, active_camera_data->per_camera_data.camera_near, active_camera_data->per_camera_data.camera_far,
                                             active_camera_data->per_camera_data.view_projection_matrix, sc.direction);
                auto& shadow_data_buffer = shadow_pass->get_shadow_data_buffer();
                m_frame_context->set_render_targets(0, nullptr, shadow_pass->get_shadow_maps_texture());
                auto& data = shadow_pass->get_shadow_data();
                for (int32 casc = 0; casc < data.shadow_cascade_count; ++casc)
                {
                    auto& cascade_frustum = shadow_pass->get_cascade_frustum(casc);

                    data.shadow_cascade = casc;

                    if (m_debug_bounds)
                    {
                        auto corners = bounding_frustum::get_corners(data.shadow_view_projection_matrices[casc]);
                        m_debug_drawer.set_color(color_rgb(0.5f));
                        m_debug_drawer.add(corners[0], corners[1]);
                        m_debug_drawer.add(corners[1], corners[3]);
                        m_debug_drawer.add(corners[3], corners[2]);
                        m_debug_drawer.add(corners[2], corners[6]);
                        m_debug_drawer.add(corners[6], corners[4]);
                        m_debug_drawer.add(corners[4], corners[0]);
                        m_debug_drawer.add(corners[0], corners[2]);

                        m_debug_drawer.add(corners[5], corners[4]);
                        m_debug_drawer.add(corners[4], corners[6]);
                        m_debug_drawer.add(corners[6], corners[7]);
                        m_debug_drawer.add(corners[7], corners[3]);
                        m_debug_drawer.add(corners[3], corners[1]);
                        m_debug_drawer.add(corners[1], corners[5]);
                        m_debug_drawer.add(corners[5], corners[7]);
                    }

                    for (uint32 c = 0; c < draws.size(); ++c)
                    {
                        auto& dc = draws[c];

                        if (m_frustum_culling)
                        {
                            auto& bb = dc.bounding_box;
                            if (!cascade_frustum.intersects(bb))
                                continue;
                        }

                        optional<primitive_gpu_data&> prim_gpu_data = scene->get_primitive_gpu_data(dc.primitive_gpu_data_id);
                        if (!prim_gpu_data)
                        {
                            warn_missing_draw("Primitive gpu data");
                            continue;
                        }
                        optional<mesh_gpu_data&> m_gpu_data = scene->get_mesh_gpu_data(dc.mesh_gpu_data_id);
                        if (!m_gpu_data)
                        {
                            warn_missing_draw("Mesh gpu data");
                            continue;
                        }
                        optional<material&> mat                   = scene->get_material(dc.material_hnd);
                        optional<material_gpu_data&> mat_gpu_data = scene->get_material_gpu_data(mat->gpu_data);
                        if (!mat || !mat_gpu_data)
                        {
                            warn_missing_draw("Material");
                            continue;
                        }

                        gfx_handle<const gfx_pipeline> dc_pipeline = m_pipeline_cache.get_shadow(prim_gpu_data->vertex_layout, prim_gpu_data->input_assembly, mat->double_sided);

                        m_frame_context->bind_pipeline(dc_pipeline);
                        gfx_viewport shadow_viewport{ 0.0f, 0.0f, static_cast<float>(shadow_pass->resolution()), static_cast<float>(shadow_pass->resolution()) };
                        m_frame_context->set_viewport(0, 1, &shadow_viewport);

                        m_frame_context->set_buffer_data(shadow_data_buffer, 0, sizeof(shadow_data), &(data));
                        dc_pipeline->get_resource_mapping()->set("shadow_data", shadow_data_buffer);

                        dc_pipeline->get_resource_mapping()->set("model_data", m_gpu_data->model_data_buffer);

                        if (mat_gpu_data->per_material_data.alpha_mode > 1)
                            continue; // TODO Paul: Transparent shadows?!

                        dc_pipeline->get_resource_mapping()->set("material_data", mat_gpu_data->material_data_buffer);

                        if (mat_gpu_data->per_material_data.base_color_texture)
                        {
                            MANGO_ASSERT(mat->base_color_texture_gpu_data.has_value(), "Texture has no gpu data!");
                            optional<texture_gpu_data&> tex = scene->get_texture_gpu_data(mat->base_color_texture_gpu_data.value());
                            if (!tex)
                            {
                                warn_missing_draw("Base Color Texture");
                                continue;
                            }
                            dc_pipeline->get_resource_mapping()->set("texture_base_color", tex->graphics_texture);
                            dc_pipeline->get_resource_mapping()->set("sampler_base_color", tex->graphics_sampler);
                        }
                        else
                        {
                            dc_pipeline->get_resource_mapping()->set("texture_base_color", default_texture_2D);
                        }

                        m_frame_context->submit_pipeline_state_resources();

                        m_frame_context->set_index_buffer(prim_gpu_data->index_buffer_view.graphics_buffer, prim_gpu_data->index_type);

                        std::vector<gfx_handle<const gfx_buffer>> vbs;
                        vbs.reserve(prim_gpu_data->vertex_buffer_views.size());
                        std::vector<int32> bindings;
                        bindings.reserve(prim_gpu_data->vertex_buffer_views.size());
                        std::vector<int32> offsets;
                        offsets.reserve(prim_gpu_data->vertex_buffer_views.size());
                        int32 idx = 0;
                        for (auto vbv : prim_gpu_data->vertex_buffer_views)
                        {
                            vbs.push_back(vbv.graphics_buffer);
                            bindings.push_back(idx++);
                            offsets.push_back(vbv.offset);
                        }

                        m_frame_context->set_vertex_buffers(static_cast<int32>(prim_gpu_data->vertex_buffer_views.size()), vbs.data(), bindings.data(), offsets.data());

                        m_renderer_info.last_frame.draw_calls++;
                        m_renderer_info.last_frame.primitives++;
                        m_renderer_info.last_frame.vertices += std::max(prim_gpu_data->draw_call_desc.vertex_count, prim_gpu_data->draw_call_desc.index_count);
                        m_frame_context->draw(prim_gpu_data->draw_call_desc.vertex_count, prim_gpu_data->draw_call_desc.index_count, prim_gpu_data->draw_call_desc.instance_count,
                                              prim_gpu_data->draw_call_desc.base_vertex, prim_gpu_data->draw_call_desc.base_instance, prim_gpu_data->draw_call_desc.index_offset);
                    }
                }
            }
        }
    }

    // gbuffer pass
    // draw objects
    {
        GL_NAMED_PROFILE_ZONE("GBuffer Pass");
        NAMED_PROFILE_ZONE("GBuffer Pass");
        m_frame_context->set_render_targets(static_cast<int32>(m_gbuffer_render_targets.size()) - 1, m_gbuffer_render_targets.data(), m_gbuffer_render_targets.back());
        for (int32 c = 0; c < opaque_count; ++c)
        {
            auto& dc = draws[c];

            if (m_frustum_culling)
            {
                auto& bb = dc.bounding_box;
                if (!camera_frustum.intersects(bb))
                    continue;
            }
            if (m_debug_bounds)
            {
                auto& bb     = dc.bounding_box;
                auto corners = bb.get_corners();
                m_debug_drawer.set_color(color_rgb(1.0f, 0.0f, 0.0f));
                m_debug_drawer.add(corners[0], corners[1]);
                m_debug_drawer.add(corners[1], corners[3]);
                m_debug_drawer.add(corners[3], corners[2]);
                m_debug_drawer.add(corners[2], corners[6]);
                m_debug_drawer.add(corners[6], corners[4]);
                m_debug_drawer.add(corners[4], corners[0]);
                m_debug_drawer.add(corners[0], corners[2]);

                m_debug_drawer.add(corners[5], corners[4]);
                m_debug_drawer.add(corners[4], corners[6]);
                m_debug_drawer.add(corners[6], corners[7]);
                m_debug_drawer.add(corners[7], corners[3]);
                m_debug_drawer.add(corners[3], corners[1]);
                m_debug_drawer.add(corners[1], corners[5]);
                m_debug_drawer.add(corners[5], corners[7]);
            }

            optional<primitive_gpu_data&> prim_gpu_data = scene->get_primitive_gpu_data(dc.primitive_gpu_data_id);
            if (!prim_gpu_data)
            {
                warn_missing_draw("Primitive gpu data");
                continue;
            }
            optional<mesh_gpu_data&> m_gpu_data = scene->get_mesh_gpu_data(dc.mesh_gpu_data_id);
            if (!m_gpu_data)
            {
                warn_missing_draw("Mesh gpu data");
                continue;
            }
            optional<material&> mat                   = scene->get_material(dc.material_hnd);
            optional<material_gpu_data&> mat_gpu_data = scene->get_material_gpu_data(mat->gpu_data);
            if (!mat || !mat_gpu_data)
            {
                warn_missing_draw("Material");
                continue;
            }

            gfx_handle<const gfx_pipeline> dc_pipeline = m_pipeline_cache.get_opaque(prim_gpu_data->vertex_layout, prim_gpu_data->input_assembly, m_wireframe, mat->double_sided);

            m_frame_context->bind_pipeline(dc_pipeline);
            gfx_viewport window_viewport{ static_cast<float>(m_renderer_info.canvas.x), static_cast<float>(m_renderer_info.canvas.y), static_cast<float>(m_renderer_info.canvas.width),
                                          static_cast<float>(m_renderer_info.canvas.height) };
            m_frame_context->set_viewport(0, 1, &window_viewport);

            dc_pipeline->get_resource_mapping()->set("model_data", m_gpu_data->model_data_buffer);
            dc_pipeline->get_resource_mapping()->set("camera_data", active_camera_data->camera_data_buffer);
            dc_pipeline->get_resource_mapping()->set("material_data", mat_gpu_data->material_data_buffer);

            if (mat_gpu_data->per_material_data.base_color_texture)
            {
                MANGO_ASSERT(mat->base_color_texture_gpu_data.has_value(), "Texture has no gpu data!");
                optional<texture_gpu_data&> tex = scene->get_texture_gpu_data(mat->base_color_texture_gpu_data.value());
                if (!tex)
                {
                    warn_missing_draw("Base Color Texture");
                    continue;
                }
                dc_pipeline->get_resource_mapping()->set("texture_base_color", tex->graphics_texture);
                dc_pipeline->get_resource_mapping()->set("sampler_base_color", tex->graphics_sampler);
            }
            else
            {
                dc_pipeline->get_resource_mapping()->set("texture_base_color", default_texture_2D);
            }
            if (mat_gpu_data->per_material_data.roughness_metallic_texture)
            {
                MANGO_ASSERT(mat->metallic_roughness_texture_gpu_data.has_value(), "Texture has no gpu data!");
                optional<texture_gpu_data&> tex = scene->get_texture_gpu_data(mat->metallic_roughness_texture_gpu_data.value());
                if (!tex)
                {
                    warn_missing_draw("Roughness Metallic Texture");
                    continue;
                }
                dc_pipeline->get_resource_mapping()->set("texture_roughness_metallic", tex->graphics_texture);
                dc_pipeline->get_resource_mapping()->set("sampler_roughness_metallic", tex->graphics_sampler);
            }
            else
            {
                dc_pipeline->get_resource_mapping()->set("texture_roughness_metallic", default_texture_2D);
            }
            if (mat_gpu_data->per_material_data.occlusion_texture)
            {
                MANGO_ASSERT(mat->occlusion_texture_gpu_data.has_value(), "Texture has no gpu data!");
                optional<texture_gpu_data&> tex = scene->get_texture_gpu_data(mat->occlusion_texture_gpu_data.value());
                if (!tex)
                {
                    warn_missing_draw("Occlusion Texture");
                    continue;
                }
                dc_pipeline->get_resource_mapping()->set("texture_occlusion", tex->graphics_texture);
                dc_pipeline->get_resource_mapping()->set("sampler_occlusion", tex->graphics_sampler);
            }
            else
            {
                dc_pipeline->get_resource_mapping()->set("texture_occlusion", default_texture_2D);
            }
            if (mat_gpu_data->per_material_data.normal_texture)
            {
                MANGO_ASSERT(mat->normal_texture_gpu_data.has_value(), "Texture has no gpu data!");
                optional<texture_gpu_data&> tex = scene->get_texture_gpu_data(mat->normal_texture_gpu_data.value());
                if (!tex)
                {
                    warn_missing_draw("Normal Texture");
                    continue;
                }
                dc_pipeline->get_resource_mapping()->set("texture_normal", tex->graphics_texture);
                dc_pipeline->get_resource_mapping()->set("sampler_normal", tex->graphics_sampler);
            }
            else
            {
                dc_pipeline->get_resource_mapping()->set("texture_normal", default_texture_2D);
            }
            if (mat_gpu_data->per_material_data.emissive_color_texture)
            {
                MANGO_ASSERT(mat->emissive_texture_gpu_data.has_value(), "Texture has no gpu data!");
                optional<texture_gpu_data&> tex = scene->get_texture_gpu_data(mat->emissive_texture_gpu_data.value());
                if (!tex)
                {
                    warn_missing_draw("Emissive Color Texture");
                    continue;
                }
                dc_pipeline->get_resource_mapping()->set("texture_emissive_color", tex->graphics_texture);
                dc_pipeline->get_resource_mapping()->set("sampler_emissive_color", tex->graphics_sampler);
            }
            else
            {
                dc_pipeline->get_resource_mapping()->set("texture_emissive_color", default_texture_2D);
            }

            m_frame_context->submit_pipeline_state_resources();

            m_frame_context->set_index_buffer(prim_gpu_data->index_buffer_view.graphics_buffer, prim_gpu_data->index_type);

            std::vector<gfx_handle<const gfx_buffer>> vbs;
            vbs.reserve(prim_gpu_data->vertex_buffer_views.size());
            std::vector<int32> bindings;
            bindings.reserve(prim_gpu_data->vertex_buffer_views.size());
            std::vector<int32> offsets;
            offsets.reserve(prim_gpu_data->vertex_buffer_views.size());
            int32 idx = 0;
            for (auto vbv : prim_gpu_data->vertex_buffer_views)
            {
                vbs.push_back(vbv.graphics_buffer);
                bindings.push_back(idx++);
                offsets.push_back(vbv.offset);
            }

            m_frame_context->set_vertex_buffers(static_cast<int32>(prim_gpu_data->vertex_buffer_views.size()), vbs.data(), bindings.data(), offsets.data());

            // m_frame_context->set_render_targets(m_gbuffer_render_targets.size() - 1, m_gbuffer_render_targets.data(), m_gbuffer_render_targets.back());

            m_renderer_info.last_frame.draw_calls++;
            m_renderer_info.last_frame.primitives++;
            m_renderer_info.last_frame.vertices += std::max(prim_gpu_data->draw_call_desc.vertex_count, prim_gpu_data->draw_call_desc.index_count);
            m_frame_context->draw(prim_gpu_data->draw_call_desc.vertex_count, prim_gpu_data->draw_call_desc.index_count, prim_gpu_data->draw_call_desc.instance_count,
                                  prim_gpu_data->draw_call_desc.base_vertex, prim_gpu_data->draw_call_desc.base_instance, prim_gpu_data->draw_call_desc.index_offset);
        }
    }

    auto irradiance = ls.get_skylight_irradiance_map();
    auto specular   = ls.get_skylight_specular_prefilter_map();
    auto brdf_lut   = ls.get_skylight_brdf_lookup();
    // lighting pass
    {
        m_deferred_lighting_pass.set_camera_data_buffer(active_camera_data->camera_data_buffer);
        m_deferred_lighting_pass.set_light_data_buffer(light_data.light_data_buffer);
        m_deferred_lighting_pass.set_shadow_data_buffer(shadow_pass ? shadow_pass->get_shadow_data_buffer() : nullptr);

        m_deferred_lighting_pass.set_irradiance_map(irradiance ? irradiance : default_texture_cube);
        m_deferred_lighting_pass.set_radiance_map(specular ? specular : default_texture_cube);
        m_deferred_lighting_pass.set_brdf_integration_lut(brdf_lut ? brdf_lut : default_texture_2D);

        m_deferred_lighting_pass.set_shadow_map(shadow_pass ? shadow_pass->get_shadow_maps_texture() : default_texture_array);

        m_deferred_lighting_pass.execute(m_frame_context);

        auto pass_info = m_deferred_lighting_pass.get_info();
        m_renderer_info.last_frame.draw_calls += pass_info.draw_calls;
        m_renderer_info.last_frame.vertices += pass_info.vertices;
    }

    // cubemap pass
    if (!m_renderer_data.debug_view_enabled)
    {
        GL_NAMED_PROFILE_ZONE("Environment Display Pass");
        NAMED_PROFILE_ZONE("Environment Display Pass");
        auto environment_display_pass = std::static_pointer_cast<environment_display_step>(m_pipeline_steps[mango::render_pipeline_step::environment_display]);
        if (environment_display_pass && specular)
        {
            environment_display_pass->set_cubemap(specular);
            m_renderer_info.last_frame.draw_calls++;
            m_renderer_info.last_frame.vertices += 18;
            environment_display_pass->execute();
        }
    }

    // transparent pass
    {
        GL_NAMED_PROFILE_ZONE("Transparent Pass");
        NAMED_PROFILE_ZONE("Transparent Pass");
        for (uint32 c = opaque_count; c < draws.size(); ++c)
        {
            auto& dc = draws[c];

            if (m_frustum_culling)
            {
                auto& bb = dc.bounding_box;
                if (!camera_frustum.intersects(bb))
                    continue;
            }
            if (m_debug_bounds)
            {
                auto& bb     = dc.bounding_box;
                auto corners = bb.get_corners();
                m_debug_drawer.set_color(color_rgb(1.0f, 0.0f, 0.0f));
                m_debug_drawer.add(corners[0], corners[1]);
                m_debug_drawer.add(corners[1], corners[3]);
                m_debug_drawer.add(corners[3], corners[2]);
                m_debug_drawer.add(corners[2], corners[6]);
                m_debug_drawer.add(corners[6], corners[4]);
                m_debug_drawer.add(corners[4], corners[0]);
                m_debug_drawer.add(corners[0], corners[2]);

                m_debug_drawer.add(corners[5], corners[4]);
                m_debug_drawer.add(corners[4], corners[6]);
                m_debug_drawer.add(corners[6], corners[7]);
                m_debug_drawer.add(corners[7], corners[3]);
                m_debug_drawer.add(corners[3], corners[1]);
                m_debug_drawer.add(corners[1], corners[5]);
                m_debug_drawer.add(corners[5], corners[7]);
            }

            optional<primitive_gpu_data&> prim_gpu_data = scene->get_primitive_gpu_data(dc.primitive_gpu_data_id);
            if (!prim_gpu_data)
            {
                warn_missing_draw("Primitive gpu data");
                continue;
            }
            optional<mesh_gpu_data&> m_gpu_data = scene->get_mesh_gpu_data(dc.mesh_gpu_data_id);
            if (!m_gpu_data)
            {
                warn_missing_draw("Mesh gpu data");
                continue;
            }
            optional<material&> mat                   = scene->get_material(dc.material_hnd);
            optional<material_gpu_data&> mat_gpu_data = scene->get_material_gpu_data(mat->gpu_data);
            if (!mat || !mat_gpu_data)
            {
                warn_missing_draw("Material");
                continue;
            }

            gfx_handle<const gfx_pipeline> dc_pipeline = m_pipeline_cache.get_transparent(prim_gpu_data->vertex_layout, prim_gpu_data->input_assembly, m_wireframe, mat->double_sided);

            m_frame_context->bind_pipeline(dc_pipeline);
            gfx_viewport window_viewport{ static_cast<float>(m_renderer_info.canvas.x), static_cast<float>(m_renderer_info.canvas.y), static_cast<float>(m_renderer_info.canvas.width),
                                          static_cast<float>(m_renderer_info.canvas.height) };
            m_frame_context->set_viewport(0, 1, &window_viewport);

            dc_pipeline->get_resource_mapping()->set("camera_data", active_camera_data->camera_data_buffer);
            dc_pipeline->get_resource_mapping()->set("light_data", light_data.light_data_buffer);
            dc_pipeline->get_resource_mapping()->set("renderer_data", m_renderer_data_buffer);
            dc_pipeline->get_resource_mapping()->set("model_data", m_gpu_data->model_data_buffer);
            dc_pipeline->get_resource_mapping()->set("material_data", mat_gpu_data->material_data_buffer);

            if (mat_gpu_data->per_material_data.base_color_texture)
            {
                MANGO_ASSERT(mat->base_color_texture_gpu_data.has_value(), "Texture has no gpu data!");
                optional<texture_gpu_data&> tex = scene->get_texture_gpu_data(mat->base_color_texture_gpu_data.value());
                if (!tex)
                {
                    warn_missing_draw("Base Color Texture");
                    continue;
                }
                dc_pipeline->get_resource_mapping()->set("texture_base_color", tex->graphics_texture);
                dc_pipeline->get_resource_mapping()->set("sampler_base_color", tex->graphics_sampler);
            }
            else
            {
                dc_pipeline->get_resource_mapping()->set("texture_base_color", default_texture_2D);
            }
            if (mat_gpu_data->per_material_data.roughness_metallic_texture)
            {
                MANGO_ASSERT(mat->metallic_roughness_texture_gpu_data.has_value(), "Texture has no gpu data!");
                optional<texture_gpu_data&> tex = scene->get_texture_gpu_data(mat->metallic_roughness_texture_gpu_data.value());
                if (!tex)
                {
                    warn_missing_draw("Roughness Metallic Texture");
                    continue;
                }
                dc_pipeline->get_resource_mapping()->set("texture_roughness_metallic", tex->graphics_texture);
                dc_pipeline->get_resource_mapping()->set("sampler_roughness_metallic", tex->graphics_sampler);
            }
            else
            {
                dc_pipeline->get_resource_mapping()->set("texture_roughness_metallic", default_texture_2D);
            }
            if (mat_gpu_data->per_material_data.occlusion_texture)
            {
                MANGO_ASSERT(mat->occlusion_texture_gpu_data.has_value(), "Texture has no gpu data!");
                optional<texture_gpu_data&> tex = scene->get_texture_gpu_data(mat->occlusion_texture_gpu_data.value());
                if (!tex)
                {
                    warn_missing_draw("Occlusion Texture");
                    continue;
                }
                dc_pipeline->get_resource_mapping()->set("texture_occlusion", tex->graphics_texture);
                dc_pipeline->get_resource_mapping()->set("sampler_occlusion", tex->graphics_sampler);
            }
            else
            {
                dc_pipeline->get_resource_mapping()->set("texture_occlusion", default_texture_2D);
            }
            if (mat_gpu_data->per_material_data.normal_texture)
            {
                MANGO_ASSERT(mat->normal_texture_gpu_data.has_value(), "Texture has no gpu data!");
                optional<texture_gpu_data&> tex = scene->get_texture_gpu_data(mat->normal_texture_gpu_data.value());
                if (!tex)
                {
                    warn_missing_draw("Normal Texture");
                    continue;
                }
                dc_pipeline->get_resource_mapping()->set("texture_normal", tex->graphics_texture);
                dc_pipeline->get_resource_mapping()->set("sampler_normal", tex->graphics_sampler);
            }
            else
            {
                dc_pipeline->get_resource_mapping()->set("texture_normal", default_texture_2D);
            }
            if (mat_gpu_data->per_material_data.emissive_color_texture)
            {
                MANGO_ASSERT(mat->emissive_texture_gpu_data.has_value(), "Texture has no gpu data!");
                optional<texture_gpu_data&> tex = scene->get_texture_gpu_data(mat->emissive_texture_gpu_data.value());
                if (!tex)
                {
                    warn_missing_draw("Emissive Color Texture");
                    continue;
                }
                dc_pipeline->get_resource_mapping()->set("texture_emissive_color", tex->graphics_texture);
                dc_pipeline->get_resource_mapping()->set("sampler_emissive_color", tex->graphics_sampler);
            }
            else
            {
                dc_pipeline->get_resource_mapping()->set("texture_emissive_color", default_texture_2D);
            }

            if (irradiance && specular) // If this exists the rest has to exist too
            {
                dc_pipeline->get_resource_mapping()->set("texture_irradiance_map", irradiance);
                dc_pipeline->get_resource_mapping()->set("sampler_irradiance_map", m_mipmapped_linear_sampler);
                dc_pipeline->get_resource_mapping()->set("texture_radiance_map", specular);
                dc_pipeline->get_resource_mapping()->set("sampler_radiance_map", m_mipmapped_linear_sampler);
                dc_pipeline->get_resource_mapping()->set("texture_brdf_integration_lut", ls.get_skylight_brdf_lookup());
                dc_pipeline->get_resource_mapping()->set("sampler_brdf_integration_lut", m_linear_sampler);
            }
            else
            {
                dc_pipeline->get_resource_mapping()->set("texture_irradiance_map", default_texture_cube);
                dc_pipeline->get_resource_mapping()->set("texture_radiance_map", default_texture_cube);
                dc_pipeline->get_resource_mapping()->set("texture_brdf_integration_lut", default_texture_2D);
            }

            if (shadow_pass)
            {
                dc_pipeline->get_resource_mapping()->set("texture_shadow_map_comp", shadow_pass->get_shadow_maps_texture());
                dc_pipeline->get_resource_mapping()->set("texture_shadow_map", shadow_pass->get_shadow_maps_texture());
                dc_pipeline->get_resource_mapping()->set("sampler_shadow_shadow_map", m_linear_compare_sampler);
                dc_pipeline->get_resource_mapping()->set("sampler_shadow_map", m_linear_sampler);
            }
            else
            {
                dc_pipeline->get_resource_mapping()->set("texture_shadow_map_comp", default_texture_array);
                dc_pipeline->get_resource_mapping()->set("texture_shadow_map", default_texture_array);
            }

            m_frame_context->submit_pipeline_state_resources();

            m_frame_context->set_index_buffer(prim_gpu_data->index_buffer_view.graphics_buffer, prim_gpu_data->index_type);

            std::vector<gfx_handle<const gfx_buffer>> vbs;
            vbs.reserve(prim_gpu_data->vertex_buffer_views.size());
            std::vector<int32> bindings;
            bindings.reserve(prim_gpu_data->vertex_buffer_views.size());
            std::vector<int32> offsets;
            offsets.reserve(prim_gpu_data->vertex_buffer_views.size());
            int32 idx = 0;
            for (auto vbv : prim_gpu_data->vertex_buffer_views)
            {
                vbs.push_back(vbv.graphics_buffer);
                bindings.push_back(idx++);
                offsets.push_back(vbv.offset);
            }

            m_frame_context->set_vertex_buffers(static_cast<int32>(prim_gpu_data->vertex_buffer_views.size()), vbs.data(), bindings.data(), offsets.data());

            // m_frame_context->set_render_targets(m_gbuffer_render_targets.size() - 1, m_gbuffer_render_targets.data(), m_gbuffer_render_targets.back());

            m_renderer_info.last_frame.draw_calls++;
            m_renderer_info.last_frame.primitives++;
            m_renderer_info.last_frame.vertices += std::max(prim_gpu_data->draw_call_desc.vertex_count, prim_gpu_data->draw_call_desc.index_count);
            m_frame_context->draw(prim_gpu_data->draw_call_desc.vertex_count, prim_gpu_data->draw_call_desc.index_count, prim_gpu_data->draw_call_desc.instance_count,
                                  prim_gpu_data->draw_call_desc.base_vertex, prim_gpu_data->draw_call_desc.base_instance, prim_gpu_data->draw_call_desc.index_offset);
        }
    }

    m_debug_drawer.update_buffer();

    // auto exposure
    if (scene->calculate_auto_exposure())
    {
        GL_NAMED_PROFILE_ZONE("Auto Exposure Calculation");
        NAMED_PROFILE_ZONE("Auto Exposure Calculation");
        m_frame_context->bind_pipeline(m_luminance_construction_pipeline);

        m_frame_context->calculate_mipmaps(m_hdr_buffer_render_targets[0]);

        int32 mip_level = 0;
        int32 hr_width  = m_renderer_info.canvas.width;
        int32 hr_height = m_renderer_info.canvas.height;
        while (hr_width >> mip_level > 512 && hr_height >> mip_level > 512) // we can make it smaller, when we have some better focussing.
        {
            ++mip_level;
        }
        hr_width >>= mip_level;
        hr_height >>= mip_level;

        barrier_description bd;
        bd.barrier_bit = gfx_barrier_bit::shader_image_access_barrier_bit;
        m_frame_context->barrier(bd);

        auto hdr_view = m_graphics_device->create_image_texture_view(m_hdr_buffer_render_targets[0], mip_level);

        // time coefficient with tau = 1.1;
        float tau                        = 1.1f;
        float time_coefficient           = 1.0f - expf(-dt * tau);
        m_luminance_data_mapping->params = vec4(-8.0f, 1.0f / 31.0f, time_coefficient, hr_width * hr_height); // min -8.0, max +23.0

        m_luminance_construction_pipeline->get_resource_mapping()->set("image_hdr_color", hdr_view);
        m_luminance_construction_pipeline->get_resource_mapping()->set("luminance_data", m_luminance_data_buffer);
        m_frame_context->submit_pipeline_state_resources();

        m_frame_context->dispatch(hr_width / 16, hr_height / 16, 1);

        bd.barrier_bit = gfx_barrier_bit::shader_storage_barrier_bit;
        m_frame_context->barrier(bd);

        m_frame_context->bind_pipeline(m_luminance_reduction_pipeline);

        m_luminance_reduction_pipeline->get_resource_mapping()->set("luminance_data", m_luminance_data_buffer);
        m_frame_context->submit_pipeline_state_resources();

        m_frame_context->dispatch(1, 1, 1);

        bd.barrier_bit = gfx_barrier_bit::buffer_update_barrier_bit;
        m_frame_context->barrier(bd);
    }

    auto fxaa_pass             = std::static_pointer_cast<fxaa_step>(m_pipeline_steps[mango::render_pipeline_step::fxaa]);
    bool postprocessing_buffer = fxaa_pass != nullptr;

    // composing pass
    {
        GL_NAMED_PROFILE_ZONE("Composing Pass");
        NAMED_PROFILE_ZONE("Composing Pass");
        m_frame_context->bind_pipeline(m_composing_pass_pipeline);

        gfx_viewport window_viewport{ static_cast<float>(m_renderer_info.canvas.x), static_cast<float>(m_renderer_info.canvas.y), static_cast<float>(m_renderer_info.canvas.width),
                                      static_cast<float>(m_renderer_info.canvas.height) };
        m_frame_context->set_viewport(0, 1, &window_viewport);

        if (postprocessing_buffer)
            m_frame_context->set_render_targets(static_cast<int32>(m_post_render_targets.size()) - 1, m_post_render_targets.data(), m_post_render_targets.back());
        else
            m_frame_context->set_render_targets(1, &m_output_target, m_ouput_depth_target);

        m_composing_pass_pipeline->get_resource_mapping()->set("camera_data", active_camera_data->camera_data_buffer);
        m_composing_pass_pipeline->get_resource_mapping()->set("renderer_data", m_renderer_data_buffer);
        m_composing_pass_pipeline->get_resource_mapping()->set("texture_hdr_input", m_hdr_buffer_render_targets[0]);
        m_composing_pass_pipeline->get_resource_mapping()->set("sampler_hdr_input", m_nearest_sampler);
        m_composing_pass_pipeline->get_resource_mapping()->set("texture_geometry_depth_input", m_hdr_buffer_render_targets.back());
        m_composing_pass_pipeline->get_resource_mapping()->set("sampler_geometry_depth_input", m_nearest_sampler);

        m_frame_context->submit_pipeline_state_resources();

        m_frame_context->set_index_buffer(nullptr, gfx_format::invalid);
        m_frame_context->set_vertex_buffers(0, nullptr, nullptr, nullptr);

        m_renderer_info.last_frame.draw_calls++;
        m_renderer_info.last_frame.vertices += 3;
        m_frame_context->draw(3, 0, 1, 0, 0, 0); // Triangle gets created in geometry shader.
    }

    // debug lines
    if (m_debug_bounds)
    {
        m_renderer_info.last_frame.draw_calls++;
        m_renderer_info.last_frame.vertices += m_debug_drawer.vertex_count();
        // use the already set render targets ...
        m_debug_drawer.execute();
    }

    // fxaa
    {
        GL_NAMED_PROFILE_ZONE("Fxaa Pass");
        NAMED_PROFILE_ZONE("Fxaa Pass");
        if (fxaa_pass)
        {
            fxaa_pass->set_input_texture(m_post_render_targets[0]); // TODO Paul Hardcoded post targets -> meh.
            m_renderer_info.last_frame.draw_calls++;
            m_renderer_info.last_frame.vertices += 3;
            fxaa_pass->execute();
        }
    }

    m_frame_context->bind_pipeline(nullptr);
    // TODO Paul: Is the renderer in charge here?
    m_frame_context->set_render_targets(1, &swap_buffer, m_graphics_device->get_swap_chain_depth_stencil_target());
}

void deferred_pbr_renderer::present()
{
    m_frame_context->present();
    // m_frame_semaphore = m_frame_context->fence(semaphore_create_info());
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
    checkbox("Frustum Culling", &m_frustum_culling, true);
    ImGui::Separator();
    bool has_environment_display = m_pipeline_steps[mango::render_pipeline_step::environment_display] != nullptr;
    bool has_shadow_map          = m_pipeline_steps[mango::render_pipeline_step::shadow_map] != nullptr;
    bool has_fxaa                = m_pipeline_steps[mango::render_pipeline_step::fxaa] != nullptr;
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
                auto environment_display = std::make_shared<environment_display_step>(environment_display_settings(0.0f)); // TODO Paul: Settings?
                environment_display->attach(m_shared_context);
                m_pipeline_steps[mango::render_pipeline_step::environment_display] = std::static_pointer_cast<render_step>(environment_display);
            }
            else
            {
                m_pipeline_steps[mango::render_pipeline_step::environment_display] = nullptr;
            }
        }
        if (has_environment_display && open)
        {
            m_pipeline_steps[mango::render_pipeline_step::environment_display]->on_ui_widget();
        }

        open = ImGui::CollapsingHeader("Shadow Step", flags | ImGuiTreeNodeFlags_AllowItemOverlap | (!has_shadow_map ? ImGuiTreeNodeFlags_Leaf : 0));
        ImGui::SameLine(ImGui::GetContentRegionAvail().x);
        ImGui::PushID("enable_shadow_step");
        value_changed = ImGui::Checkbox("", &has_shadow_map);
        ImGui::PopID();
        if (value_changed)
        {
            if (has_shadow_map)
            {
                auto step_shadow_map = std::make_shared<shadow_map_step>(shadow_settings());
                step_shadow_map->attach(m_shared_context);
                m_pipeline_cache.set_shadow_base(step_shadow_map->get_shadow_pass_pipeline_base());
                m_pipeline_steps[mango::render_pipeline_step::shadow_map] = std::static_pointer_cast<render_step>(step_shadow_map);
                m_renderer_data.shadow_step_enabled                       = true;
            }
            else
            {
                m_pipeline_steps[mango::render_pipeline_step::shadow_map] = nullptr;
                m_renderer_data.shadow_step_enabled                       = false;
            }
        }
        if (has_shadow_map && open)
        {
            m_pipeline_steps[mango::render_pipeline_step::shadow_map]->on_ui_widget();
        }
        open = ImGui::CollapsingHeader("FXAA Step", flags | ImGuiTreeNodeFlags_AllowItemOverlap | (!has_fxaa ? ImGuiTreeNodeFlags_Leaf : 0));
        ImGui::SameLine(ImGui::GetContentRegionAvail().x);
        ImGui::PushID("enable_fxaa_step");
        value_changed = ImGui::Checkbox("", &has_fxaa);
        ImGui::PopID();
        if (value_changed)
        {
            if (has_fxaa)
            {
                auto step_fxaa = std::make_shared<fxaa_step>(fxaa_settings(0.75f));
                step_fxaa->attach(m_shared_context);
                step_fxaa->set_output_targets(m_output_target, m_ouput_depth_target);
                m_pipeline_steps[mango::render_pipeline_step::fxaa] = std::static_pointer_cast<render_step>(step_fxaa);
            }
            else
            {
                m_pipeline_steps[mango::render_pipeline_step::fxaa] = nullptr;
            }
        }
        if (has_fxaa && open)
        {
            m_pipeline_steps[mango::render_pipeline_step::fxaa]->on_ui_widget();
        }
        ImGui::TreePop();
    }
    const char* debug[10]      = { "Default", "Position", "Normal", "Depth", "Base Color", "Reflection Color", "Emission", "Occlusion", "Roughness", "Metallic" };
    static int32 current_debug = 0;
    if (ImGui::CollapsingHeader("Debug", flags))
    {
        checkbox("Render Wireframe", &m_wireframe, false);
        checkbox("Debug Bounds", &m_debug_bounds, false);

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
}

float deferred_pbr_renderer::get_average_luminance() const
{
    return m_luminance_data_mapping->luminance;
}