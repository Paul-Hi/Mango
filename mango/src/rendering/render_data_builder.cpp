//! \file      render_data_builder.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#include <mango/profile.hpp>
#include <rendering/render_data_builder.hpp>
#include <resources/resources_impl.hpp>
#include <scene/scene_impl.hpp>
#include <util/helpers.hpp>

using namespace mango;

bool skylight_builder::init(const shared_ptr<context_impl>& context)
{
    m_shared_context = context;

    auto& graphics_device    = m_shared_context->get_graphics_device();
    auto& internal_resources = m_shared_context->get_internal_resources();
    shader_stage_create_info shader_info;
    shader_resource_resource_description res_resource_desc;
    shader_source_description source_desc;

    {
        res_resource_desc.path        = "res/shader/c_equi_to_cubemap.glsl";
        const shader_resource* source = internal_resources->acquire(res_resource_desc);

        source_desc.entry_point = "main";
        source_desc.source      = source->source.c_str();
        source_desc.size        = static_cast<int32>(source->source.size());

        shader_info.stage         = gfx_shader_stage_type::shader_stage_compute;
        shader_info.shader_source = source_desc;

        shader_info.resource_count = 4;

        shader_info.resources = { {
            { gfx_shader_stage_type::shader_stage_compute, 0, "texture_hdr_in", gfx_shader_resource_type::shader_resource_texture, 1 },
            { gfx_shader_stage_type::shader_stage_compute, 0, "sampler_hdr_in", gfx_shader_resource_type::shader_resource_sampler, 1 },
            { gfx_shader_stage_type::shader_stage_compute, 1, "cubemap_out", gfx_shader_resource_type::shader_resource_image_storage, 1 },
            { gfx_shader_stage_type::shader_stage_compute, 3, "ibl_generation_data", gfx_shader_resource_type::shader_resource_buffer_storage, 1 },
        } };

        m_equi_to_cubemap = graphics_device->create_shader_stage(shader_info);
        if (!check_creation(m_equi_to_cubemap.get(), "cubemap compute shader"))
            return false;

        compute_pipeline_create_info cubemap_compute_pass_info = graphics_device->provide_compute_pipeline_create_info();
        auto cubemap_compute_pass_pipeline_layout              = graphics_device->create_pipeline_resource_layout({
            { gfx_shader_stage_type::shader_stage_compute, 0, gfx_shader_resource_type::shader_resource_texture, gfx_shader_resource_access::shader_access_dynamic },
            { gfx_shader_stage_type::shader_stage_compute, 0, gfx_shader_resource_type::shader_resource_sampler, gfx_shader_resource_access::shader_access_dynamic },
            { gfx_shader_stage_type::shader_stage_compute, 1, gfx_shader_resource_type::shader_resource_image_storage, gfx_shader_resource_access::shader_access_dynamic },
            { gfx_shader_stage_type::shader_stage_compute, 3, gfx_shader_resource_type::shader_resource_buffer_storage, gfx_shader_resource_access::shader_access_dynamic },
        });

        cubemap_compute_pass_info.pipeline_layout = cubemap_compute_pass_pipeline_layout;

        cubemap_compute_pass_info.shader_stage_descriptor.compute_shader_stage = m_equi_to_cubemap;

        m_equi_to_cubemap_pipeline = graphics_device->create_compute_pipeline(cubemap_compute_pass_info);
    }

    {
        res_resource_desc.path        = "res/shader/atmospheric_scattering/c_atmospheric_scattering_cubemap.glsl";
        const shader_resource* source = internal_resources->acquire(res_resource_desc);

        source_desc.entry_point = "main";
        source_desc.source      = source->source.c_str();
        source_desc.size        = static_cast<int32>(source->source.size());

        shader_info.stage         = gfx_shader_stage_type::shader_stage_compute;
        shader_info.shader_source = source_desc;

        shader_info.resource_count = 3;

        shader_info.resources = { {
            { gfx_shader_stage_type::shader_stage_compute, 0, "cubemap_out", gfx_shader_resource_type::shader_resource_image_storage, 1 },
            { gfx_shader_stage_type::shader_stage_compute, 3, "ibl_generation_data", gfx_shader_resource_type::shader_resource_buffer_storage, 1 },
            { gfx_shader_stage_type::shader_stage_compute, 4, "atmosphere_ub_data", gfx_shader_resource_type::shader_resource_buffer_storage, 1 },
        } };

        m_atmospheric_cubemap = graphics_device->create_shader_stage(shader_info);
        if (!check_creation(m_atmospheric_cubemap.get(), "atmospheric cubemap compute shader"))
            return false;

        compute_pipeline_create_info atmospheric_cubemap_compute_pass_info = graphics_device->provide_compute_pipeline_create_info();
        auto atmospheric_cubemap_compute_pass_pipeline_layout              = graphics_device->create_pipeline_resource_layout({
            { gfx_shader_stage_type::shader_stage_compute, 0, gfx_shader_resource_type::shader_resource_image_storage, gfx_shader_resource_access::shader_access_static },
            { gfx_shader_stage_type::shader_stage_compute, 3, gfx_shader_resource_type::shader_resource_buffer_storage, gfx_shader_resource_access::shader_access_static },
            { gfx_shader_stage_type::shader_stage_compute, 3, gfx_shader_resource_type::shader_resource_buffer_storage, gfx_shader_resource_access::shader_access_static },
        });

        atmospheric_cubemap_compute_pass_info.pipeline_layout = atmospheric_cubemap_compute_pass_pipeline_layout;

        atmospheric_cubemap_compute_pass_info.shader_stage_descriptor.compute_shader_stage = m_atmospheric_cubemap;

        m_generate_atmospheric_cubemap_pipeline = graphics_device->create_compute_pipeline(atmospheric_cubemap_compute_pass_info);
    }

    {
        res_resource_desc.path        = "res/shader/pbr_compute/c_irradiance_map.glsl";
        const shader_resource* source = internal_resources->acquire(res_resource_desc);

        source_desc.entry_point = "main";
        source_desc.source      = source->source.c_str();
        source_desc.size        = static_cast<int32>(source->source.size());

        shader_info.stage         = gfx_shader_stage_type::shader_stage_compute;
        shader_info.shader_source = source_desc;

        shader_info.resource_count = 4;

        shader_info.resources = { {
            { gfx_shader_stage_type::shader_stage_compute, 0, "texture_cubemap_in", gfx_shader_resource_type::shader_resource_texture, 1 },
            { gfx_shader_stage_type::shader_stage_compute, 0, "sampler_cubemap_in", gfx_shader_resource_type::shader_resource_sampler, 1 },
            { gfx_shader_stage_type::shader_stage_compute, 1, "irradiance_map_out", gfx_shader_resource_type::shader_resource_image_storage, 1 },
            { gfx_shader_stage_type::shader_stage_compute, 3, "ibl_generation_data", gfx_shader_resource_type::shader_resource_buffer_storage, 1 },
        } };

        m_build_irradiance_map = graphics_device->create_shader_stage(shader_info);
        if (!check_creation(m_build_irradiance_map.get(), "irradiance map compute shader"))
            return false;

        compute_pipeline_create_info irradiance_map_compute_pass_info = graphics_device->provide_compute_pipeline_create_info();
        auto irradiance_map_compute_pass_pipeline_layout              = graphics_device->create_pipeline_resource_layout({
            { gfx_shader_stage_type::shader_stage_compute, 0, gfx_shader_resource_type::shader_resource_texture, gfx_shader_resource_access::shader_access_dynamic },
            { gfx_shader_stage_type::shader_stage_compute, 0, gfx_shader_resource_type::shader_resource_sampler, gfx_shader_resource_access::shader_access_dynamic },
            { gfx_shader_stage_type::shader_stage_compute, 1, gfx_shader_resource_type::shader_resource_image_storage, gfx_shader_resource_access::shader_access_dynamic },
            { gfx_shader_stage_type::shader_stage_compute, 3, gfx_shader_resource_type::shader_resource_buffer_storage, gfx_shader_resource_access::shader_access_dynamic },
        });

        irradiance_map_compute_pass_info.pipeline_layout = irradiance_map_compute_pass_pipeline_layout;

        irradiance_map_compute_pass_info.shader_stage_descriptor.compute_shader_stage = m_build_irradiance_map;

        m_build_irradiance_map_pipeline = graphics_device->create_compute_pipeline(irradiance_map_compute_pass_info);
    }

    {
        res_resource_desc.path        = "res/shader/pbr_compute/c_prefilter_specular_map.glsl";
        const shader_resource* source = internal_resources->acquire(res_resource_desc);

        source_desc.entry_point = "main";
        source_desc.source      = source->source.c_str();
        source_desc.size        = static_cast<int32>(source->source.size());

        shader_info.stage         = gfx_shader_stage_type::shader_stage_compute;
        shader_info.shader_source = source_desc;

        shader_info.resource_count = 4;

        shader_info.resources = { {
            { gfx_shader_stage_type::shader_stage_compute, 0, "texture_cubemap_in", gfx_shader_resource_type::shader_resource_texture, 1 },
            { gfx_shader_stage_type::shader_stage_compute, 0, "sampler_cubemap_in", gfx_shader_resource_type::shader_resource_sampler, 1 },
            { gfx_shader_stage_type::shader_stage_compute, 1, "prefiltered_spec_out", gfx_shader_resource_type::shader_resource_image_storage, 1 },
            { gfx_shader_stage_type::shader_stage_compute, 3, "ibl_generation_data", gfx_shader_resource_type::shader_resource_buffer_storage, 1 },
        } };

        m_build_specular_prefiltered_map = graphics_device->create_shader_stage(shader_info);
        if (!check_creation(m_build_specular_prefiltered_map.get(), "prefilter specular cubemap compute shader"))
            return false;

        compute_pipeline_create_info spec_prefiltered_map_compute_pass_info = graphics_device->provide_compute_pipeline_create_info();
        auto spec_prefiltered_map_compute_pass_pipeline_layout              = graphics_device->create_pipeline_resource_layout({
            { gfx_shader_stage_type::shader_stage_compute, 0, gfx_shader_resource_type::shader_resource_texture, gfx_shader_resource_access::shader_access_dynamic },
            { gfx_shader_stage_type::shader_stage_compute, 0, gfx_shader_resource_type::shader_resource_sampler, gfx_shader_resource_access::shader_access_dynamic },
            { gfx_shader_stage_type::shader_stage_compute, 1, gfx_shader_resource_type::shader_resource_image_storage, gfx_shader_resource_access::shader_access_dynamic },
            { gfx_shader_stage_type::shader_stage_compute, 3, gfx_shader_resource_type::shader_resource_buffer_storage, gfx_shader_resource_access::shader_access_dynamic },
        });

        spec_prefiltered_map_compute_pass_info.pipeline_layout = spec_prefiltered_map_compute_pass_pipeline_layout;

        spec_prefiltered_map_compute_pass_info.shader_stage_descriptor.compute_shader_stage = m_build_specular_prefiltered_map;

        m_build_specular_prefiltered_map_pipeline = graphics_device->create_compute_pipeline(spec_prefiltered_map_compute_pass_info);
    }

    // Lookup for all skylights
    create_brdf_lookup();
    if (!m_brdf_integration_lut)
        return false;

    return true;
}

void skylight_builder::create_brdf_lookup()
{
    auto& graphics_device = m_shared_context->get_graphics_device();

    texture_create_info texture_info;
    texture_info.texture_type   = gfx_texture_type::texture_type_2d;
    texture_info.width          = brdf_lut_size;
    texture_info.height         = brdf_lut_size;
    texture_info.miplevels      = 1;
    texture_info.array_layers   = 1;
    texture_info.texture_format = gfx_format::rgba16f;

    m_brdf_integration_lut = graphics_device->create_texture(texture_info);
    if (!check_creation(m_brdf_integration_lut.get(), "brdf integration lookup texture"))
        return;

    buffer_create_info buffer_info;
    buffer_info.buffer_target = gfx_buffer_target::buffer_target_uniform;
    buffer_info.buffer_access = gfx_buffer_access::buffer_access_dynamic_storage;

    buffer_info.size            = sizeof(ibl_generator_data);
    m_ibl_generator_data_buffer = graphics_device->create_buffer(buffer_info);
    if (!check_creation(m_ibl_generator_data_buffer.get(), "ibl generator data buffer"))
        return;

    auto& internal_resources = m_shared_context->get_internal_resources();
    shader_stage_create_info shader_info;
    shader_resource_resource_description res_resource_desc;
    shader_source_description source_desc;

    res_resource_desc.path        = "res/shader/pbr_compute/c_brdf_integration.glsl";
    const shader_resource* source = internal_resources->acquire(res_resource_desc);

    source_desc.entry_point = "main";
    source_desc.source      = source->source.c_str();
    source_desc.size        = static_cast<int32>(source->source.size());

    shader_info.stage         = gfx_shader_stage_type::shader_stage_compute;
    shader_info.shader_source = source_desc;

    shader_info.resource_count = 2;

    shader_info.resources = { {
        { gfx_shader_stage_type::shader_stage_compute, 0, "integration_lut_out", gfx_shader_resource_type::shader_resource_image_storage, 1 },
        { gfx_shader_stage_type::shader_stage_compute, 3, "ibl_generation_data", gfx_shader_resource_type::shader_resource_buffer_storage, 1 },
    } };

    m_brdf_lookup_generation_compute = graphics_device->create_shader_stage(shader_info);
    if (!check_creation(m_brdf_lookup_generation_compute.get(), "brdf lookup generation compute shader"))
        return;

    compute_pipeline_create_info brdf_lookup_pass_info = graphics_device->provide_compute_pipeline_create_info();
    auto brdf_lookup_pass_pipeline_layout              = graphics_device->create_pipeline_resource_layout({
        { gfx_shader_stage_type::shader_stage_compute, 0, gfx_shader_resource_type::shader_resource_image_storage, gfx_shader_resource_access::shader_access_static },
        { gfx_shader_stage_type::shader_stage_compute, 3, gfx_shader_resource_type::shader_resource_buffer_storage, gfx_shader_resource_access::shader_access_static },
    });

    brdf_lookup_pass_info.pipeline_layout = brdf_lookup_pass_pipeline_layout;

    brdf_lookup_pass_info.shader_stage_descriptor.compute_shader_stage = m_brdf_lookup_generation_compute;

    m_brdf_integration_lut_pipeline = graphics_device->create_compute_pipeline(brdf_lookup_pass_info);

    graphics_device_context_handle device_context = graphics_device->create_graphics_device_context();

    device_context->begin();
    GL_NAMED_PROFILE_ZONE("Generating brdf lookup");

    device_context->bind_pipeline(m_brdf_integration_lut_pipeline);
    m_current_ibl_generator_data.out_size = vec2(static_cast<float>(brdf_lut_size));
    m_current_ibl_generator_data.data     = vec2(0.0f); // unused here
    device_context->set_buffer_data(m_ibl_generator_data_buffer, 0, sizeof(ibl_generator_data), &m_current_ibl_generator_data);

    auto lut_view = graphics_device->create_image_texture_view(m_brdf_integration_lut);
    m_brdf_integration_lut_pipeline->get_resource_mapping()->set("integration_lut_out", lut_view);
    m_brdf_integration_lut_pipeline->get_resource_mapping()->set("ibl_generation_data", m_ibl_generator_data_buffer);

    device_context->submit_pipeline_state_resources();

    device_context->dispatch(brdf_lut_size / 8, brdf_lut_size / 8, 1);

    barrier_description bd;
    bd.barrier_bit = gfx_barrier_bit::shader_image_access_barrier_bit;
    device_context->barrier(bd);
    device_context->end();
    device_context->submit();
}

bool skylight_builder::needs_rebuild()
{
    if (old_dependencies.size() != new_dependencies.size())
        return true;

    // order change is important here
    for (int32 i = 0; i < static_cast<int32>(old_dependencies.size()); ++i)
    {
        if (old_dependencies.at(i) != new_dependencies.at(i))
            return true;
    }
    return false;
}

/*
void skylight_builder::add_atmosphere_influence(atmosphere_light* light)
{
    new_dependencies.push_back(light);
}
*/

void skylight_builder::build(scene_impl* scene, const skylight& light, skylight_cache* render_data)
{
    PROFILE_ZONE;
    old_dependencies = new_dependencies;
    new_dependencies.clear();

    // HDR Texture
    if (light.use_texture)
    {
        if (light.hdr_texture == invalid_sid)
        {
            clear(render_data);
            return;
        }
        load_from_hdr(scene, light, render_data);
    }
    else // TODO Paul: capture ... will be done soon....
    {
        // capture(compute_commands, render_data);
    }
}

/*
void skylight_builder::capture(const command_buffer_ptr<min_key>& compute_commands, skylight_cache* render_data)
{
    PROFILE_ZONE;
    texture_configuration texture_config;
    texture_config.generate_mipmaps        = calculate_mip_count(global_cubemap_size, global_cubemap_size);
    texture_config.is_standard_color_space = false;
    texture_config.is_cubemap              = true;
    texture_config.texture_min_filter      = texture_parameter::filter_linear_mipmap_linear;
    texture_config.texture_mag_filter      = texture_parameter::filter_linear;
    texture_config.texture_wrap_s          = texture_parameter::wrap_clamp_to_edge;
    texture_config.texture_wrap_t          = texture_parameter::wrap_clamp_to_edge;

    if (render_data->cubemap)
        render_data->cubemap->release();
    render_data->cubemap = texture::create(texture_config);
    if (!check_creation(render_data->cubemap.get(), "environment cubemap texture"))
        return;

    render_data->cubemap->set_data(format::rgba16f, global_cubemap_size, global_cubemap_size, format::rgba, format::t_float, nullptr);

    // creating a temporal command buffer for compute shader execution.

    // capturing shader (we can try to do it in one call like with the shadows)
    // bind_shader_program_command* bsp = compute_commands->create<bind_shader_program_command>(command_keys::no_sort);
    // bsp->shader_program_name         = m_capture_scene->get_name();

    // bind output cubemap
    bind_image_texture_command* bit = compute_commands->create<bind_image_texture_command>(command_keys::no_sort);
    bit->binding                    = 1;
    bit->texture_name               = render_data->cubemap->get_name();
    bit->level                      = 0;
    bit->layered                    = true;
    bit->layer                      = 0;
    bit->access                     = base_access::write_only;
    bit->element_format             = format::rgba16f;

    // bind uniforms
    vec2 out                    = vec2(render_data->cubemap->get_width(), render_data->cubemap->get_height());
    bind_single_uniform_command* bsu = compute_commands->create<bind_single_uniform_command>(command_keys::no_sort, sizeof(out));
    bsu->count                       = 1;
    bsu->location                    = 1;
    bsu->type                        = shader_resource_type::fvec2;
    bsu->uniform_value               = compute_commands->map_spare<bind_single_uniform_command>();
    memcpy(bsu->uniform_value, &out, sizeof(out));

    // execute draw calls
    if (m_draw_commands)
    {
        m_draw_commands->execute();
        m_draw_commands->invalidate();
    }

    // We need to recalculate mipmaps
    calculate_mipmaps_command* cm = compute_commands->create<calculate_mipmaps_command>(command_keys::no_sort);
    cm->texture_name              = render_data->cubemap->get_name();

    add_memory_barrier_command* amb = compute_commands->create<add_memory_barrier_command>(command_keys::no_sort);
    amb->barrier_bit                = memory_barrier_bit::shader_image_access_barrier_bit;

    calculate_ibl_maps(compute_commands, render_data);
}
*/
void skylight_builder::load_from_hdr(scene_impl* scene, const skylight& light, skylight_cache* render_data)
{
    PROFILE_ZONE;
    auto& graphics_device = m_shared_context->get_graphics_device();

    texture_create_info texture_info;
    texture_info.texture_type   = gfx_texture_type::texture_type_cube_map;
    texture_info.width          = global_cubemap_size;
    texture_info.height         = global_cubemap_size;
    texture_info.miplevels      = graphics::calculate_mip_count(global_cubemap_size, global_cubemap_size);
    texture_info.array_layers   = 1;
    texture_info.texture_format = gfx_format::rgba16f;

    render_data->cubemap = graphics_device->create_texture(texture_info);
    if (!check_creation(render_data->cubemap.get(), "environment cubemap texture"))
        return;

    graphics_device_context_handle device_context = graphics_device->create_graphics_device_context();

    device_context->begin();
    GL_NAMED_PROFILE_ZONE("Generating IBL Cubemap");
    // equirectangular to cubemap
    device_context->bind_pipeline(m_equi_to_cubemap_pipeline);
    auto input_hdr = scene->get_scene_texture(light.hdr_texture);
    if (!input_hdr)
    {
        MANGO_LOG_WARN("Hdr texture to build ibl does not exist.");
        return;
    }
    m_current_ibl_generator_data.out_size = vec2(static_cast<float>(global_cubemap_size));
    m_current_ibl_generator_data.data     = vec2(0.0f); // unused here
    device_context->set_buffer_data(m_ibl_generator_data_buffer, 0, sizeof(ibl_generator_data), &m_current_ibl_generator_data);

    m_equi_to_cubemap_pipeline->get_resource_mapping()->set("texture_hdr_in", input_hdr->graphics_texture);
    m_equi_to_cubemap_pipeline->get_resource_mapping()->set("sampler_hdr_in", input_hdr->graphics_sampler);
    auto cubemap_view = graphics_device->create_image_texture_view(render_data->cubemap);
    m_equi_to_cubemap_pipeline->get_resource_mapping()->set("cubemap_out", cubemap_view);
    m_equi_to_cubemap_pipeline->get_resource_mapping()->set("ibl_generation_data", m_ibl_generator_data_buffer);

    device_context->submit_pipeline_state_resources();

    device_context->dispatch(global_cubemap_size / 32, global_cubemap_size / 32, 6);

    barrier_description bd;
    bd.barrier_bit = gfx_barrier_bit::shader_image_access_barrier_bit;
    device_context->barrier(bd);

    device_context->calculate_mipmaps(render_data->cubemap);

    device_context->end();
    device_context->submit();

    calculate_ibl_maps(render_data);
}

void skylight_builder::calculate_ibl_maps(skylight_cache* render_data)
{
    if (!render_data->cubemap)
    {
        MANGO_LOG_ERROR("Can not calculate ibl maps without a cubemap!");
        return; // Should not be possible;
    }

    auto& graphics_device = m_shared_context->get_graphics_device();

    graphics_device_context_handle device_context = graphics_device->create_graphics_device_context();

    texture_create_info texture_info;
    texture_info.texture_type   = gfx_texture_type::texture_type_cube_map;
    texture_info.width          = global_specular_convolution_map_size;
    texture_info.height         = global_specular_convolution_map_size;
    int32 specular_mip_count    = graphics::calculate_mip_count(global_specular_convolution_map_size, global_specular_convolution_map_size);
    texture_info.miplevels      = specular_mip_count;
    texture_info.array_layers   = 1;
    texture_info.texture_format = gfx_format::rgba16f;

    sampler_create_info sampler_info;
    sampler_info.sampler_min_filter      = gfx_sampler_filter::sampler_filter_linear_mipmap_linear;
    sampler_info.sampler_max_filter      = gfx_sampler_filter::sampler_filter_linear;
    sampler_info.enable_comparison_mode  = false;
    sampler_info.comparison_operator     = gfx_compare_operator::compare_operator_always;
    sampler_info.edge_value_wrap_u       = gfx_sampler_edge_wrap::sampler_edge_wrap_clamp_to_edge;
    sampler_info.edge_value_wrap_v       = gfx_sampler_edge_wrap::sampler_edge_wrap_clamp_to_edge;
    sampler_info.edge_value_wrap_w       = gfx_sampler_edge_wrap::sampler_edge_wrap_clamp_to_edge;
    sampler_info.border_color[0]         = 0;
    sampler_info.border_color[1]         = 0;
    sampler_info.border_color[2]         = 0;
    sampler_info.border_color[3]         = 0;
    sampler_info.enable_seamless_cubemap = true;

    auto mipmapped_gen_sampler = graphics_device->create_sampler(sampler_info);

    sampler_info.sampler_min_filter = gfx_sampler_filter::sampler_filter_linear;
    auto gen_sampler                = graphics_device->create_sampler(sampler_info);

    render_data->specular_prefiltered_cubemap = graphics_device->create_texture(texture_info);
    if (!check_creation(render_data->specular_prefiltered_cubemap.get(), "environment specular prefiltered texture"))
        return;

    texture_info.width              = global_irradiance_map_size;
    texture_info.height             = global_irradiance_map_size;
    texture_info.miplevels          = 1;
    render_data->irradiance_cubemap = graphics_device->create_texture(texture_info);
    if (!check_creation(render_data->irradiance_cubemap.get(), "environment irradiance texture"))
        return;

    device_context->begin();
    GL_NAMED_PROFILE_ZONE("Generating IBL Maps");
    // build irradiance map
    device_context->bind_pipeline(m_build_irradiance_map_pipeline);

    m_current_ibl_generator_data.out_size = vec2(static_cast<float>(global_irradiance_map_size));
    m_current_ibl_generator_data.data     = vec2(0.0f); // unused here
    device_context->set_buffer_data(m_ibl_generator_data_buffer, 0, sizeof(ibl_generator_data), &m_current_ibl_generator_data);

    m_build_irradiance_map_pipeline->get_resource_mapping()->set("texture_cubemap_in", render_data->cubemap);
    m_build_irradiance_map_pipeline->get_resource_mapping()->set("sampler_cubemap_in", mipmapped_gen_sampler);
    auto irradiance_view = graphics_device->create_image_texture_view(render_data->irradiance_cubemap);
    m_build_irradiance_map_pipeline->get_resource_mapping()->set("irradiance_map_out", irradiance_view);
    m_build_irradiance_map_pipeline->get_resource_mapping()->set("ibl_generation_data", m_ibl_generator_data_buffer);

    device_context->submit_pipeline_state_resources();

    device_context->dispatch(global_irradiance_map_size / 4, global_irradiance_map_size / 4, 6);

    barrier_description bd;
    bd.barrier_bit = gfx_barrier_bit::shader_image_access_barrier_bit;
    device_context->barrier(bd);

    // build prefiltered specular mipchain
    device_context->bind_pipeline(m_build_specular_prefiltered_map_pipeline);

    m_build_specular_prefiltered_map_pipeline->get_resource_mapping()->set("texture_cubemap_in", render_data->cubemap);
    m_build_specular_prefiltered_map_pipeline->get_resource_mapping()->set("sampler_cubemap_in", mipmapped_gen_sampler);

    for (int32 mip = 0; mip < specular_mip_count; ++mip)
    {
        const uint32 mipmap_width  = global_specular_convolution_map_size >> mip;
        const uint32 mipmap_height = global_specular_convolution_map_size >> mip;
        float roughness            = (float)mip / (float)(specular_mip_count - 1);

        m_current_ibl_generator_data.out_size = vec2(mipmap_width, mipmap_height);
        m_current_ibl_generator_data.data     = vec2(roughness, 0.0f);
        device_context->set_buffer_data(m_ibl_generator_data_buffer, 0, sizeof(ibl_generator_data), &m_current_ibl_generator_data);

        auto mip_view = graphics_device->create_image_texture_view(render_data->specular_prefiltered_cubemap, mip);
        m_build_specular_prefiltered_map_pipeline->get_resource_mapping()->set("prefiltered_spec_out", mip_view);
        m_build_specular_prefiltered_map_pipeline->get_resource_mapping()->set("ibl_generation_data", m_ibl_generator_data_buffer);

        device_context->submit_pipeline_state_resources();

        device_context->dispatch(global_specular_convolution_map_size / 32, global_specular_convolution_map_size / 32, 6);
    }

    device_context->barrier(bd);

    device_context->end();
    device_context->submit();
}

void skylight_builder::clear(skylight_cache* render_data)
{
    PROFILE_ZONE;
    render_data->cubemap                      = nullptr;
    render_data->irradiance_cubemap           = nullptr;
    render_data->specular_prefiltered_cubemap = nullptr;
    return;
}

/*
bool atmosphere_builder::init()
{
    return false;
}

bool atmosphere_builder::needs_rebuild()
{
    return false;
}

void atmosphere_builder::build(atmosphere_light* light, atmosphere_cache* render_data)
{
    MANGO_UNUSED(light);
    MANGO_UNUSED(render_data);
}

// buffer_ptr m_atmosphere_data_buffer;
// struct atmosphere_ub_data
// {
//     std140_vec3 sun_dir;
//     std140_vec3 rayleigh_scattering_coefficients;
//     std140_vec3 ray_origin;
//     std140_vec2 density_multiplier;
//     std140_float sun_intensity;
//     std140_float mie_scattering_coefficient;
//     std140_float ground_radius;
//     std140_float atmosphere_radius;
//     std140_float mie_preferred_scattering_dir;
//     std140_int scatter_points;
//     std140_int scatter_points_second_ray;
// };
// atmosphere_ub_data* m_atmosphere_data_mapping;

// void environment_display_step::create_with_atmosphere(const mango::environment_light_data* el_data)
// {
//     PROFILE_ZONE;
//     m_atmosphere_data_mapping->sun_dir                          = glm::normalize(el_data->sun_data.direction);
//     m_atmosphere_data_mapping->sun_intensity                    = el_data->sun_data.intensity * 0.0025f;
//     m_atmosphere_data_mapping->scatter_points                   = el_data->scatter_points;
//     m_atmosphere_data_mapping->scatter_points_second_ray        = el_data->scatter_points_second_ray;
//     m_atmosphere_data_mapping->rayleigh_scattering_coefficients = el_data->rayleigh_scattering_coefficients;
//     m_atmosphere_data_mapping->mie_scattering_coefficient       = el_data->mie_scattering_coefficient;
//     m_atmosphere_data_mapping->density_multiplier               = el_data->density_multiplier;
//     m_atmosphere_data_mapping->ground_radius                    = el_data->ground_radius;
//     m_atmosphere_data_mapping->atmosphere_radius                = el_data->atmosphere_radius;
//     m_atmosphere_data_mapping->ray_origin                       = vec3(0.0f, el_data->ground_radius + el_data->view_height, 0.0f);
//     m_atmosphere_data_mapping->mie_preferred_scattering_dir     = el_data->mie_preferred_scattering_dir;
//
//     texture_configuration texture_config;
//     texture_config.generate_mipmaps        = calculate_mip_count(m_cube_width, m_cube_height);
//     texture_config.is_standard_color_space = false;
//     texture_config.is_cubemap              = true;
//     texture_config.texture_min_filter      = texture_parameter::filter_linear_mipmap_linear;
//     texture_config.texture_mag_filter      = texture_parameter::filter_linear;
//     texture_config.texture_wrap_s          = texture_parameter::wrap_clamp_to_edge;
//     texture_config.texture_wrap_t          = texture_parameter::wrap_clamp_to_edge;
//
//     if (m_cubemap)
//         m_cubemap->release();
//     m_cubemap = texture::create(texture_config);
//     if (!check_creation(m_cubemap.get(), "environment cubemap texture"))
//         return;
//
//     m_cubemap->set_data(format::rgba16f, m_cube_width, m_cube_height, format::rgba, format::t_float, nullptr);
//
//     // creating a temporal command buffer for compute shader execution.
//     command_buffer_ptr<min_key> compute_commands = command_buffer<min_key>::create(4096);
//
//     // atmospheric scattering to cubemap
//     bind_shader_program_command* bsp = compute_commands->create<bind_shader_program_command>(command_keys::no_sort);
//     bsp->shader_program_name         = m_atmospheric_cubemap->get_name();
//
//     // bind output cubemap
//     bind_image_texture_command* bit = compute_commands->create<bind_image_texture_command>(command_keys::no_sort);
//     bit->binding                    = 0;
//     bit->texture_name               = m_cubemap->get_name();
//     bit->level                      = 0;
//     bit->layered                    = true;
//     bit->layer                      = 0;
//     bit->access                     = base_access::write_only;
//     bit->element_format             = format::rgba16f;
//
//     // bind uniforms
//     vec2 out                    = vec2(m_cubemap->get_width(), m_cubemap->get_height());
//     bind_single_uniform_command* bsu = compute_commands->create<bind_single_uniform_command>(command_keys::no_sort, sizeof(out));
//     bsu->count                       = 1;
//     bsu->location                    = 0;
//     bsu->type                        = shader_resource_type::fvec2;
//     bsu->uniform_value               = compute_commands->map_spare<bind_single_uniform_command>();
//     memcpy(bsu->uniform_value, &out, sizeof(out));
//
//     bind_buffer_command* bb = compute_commands->create<bind_buffer_command>(command_keys::no_sort);
//     bb->index               = UB_SLOT_COMPUTE_DATA;
//     bb->buffer_name         = m_atmosphere_data_buffer->get_name();
//     bb->offset              = 0;
//     bb->target              = buffer_target::uniform_buffer;
//     bb->size                = m_atmosphere_data_buffer->byte_length();
//
//     // execute compute
//     dispatch_compute_command* dp = compute_commands->create<dispatch_compute_command>(command_keys::no_sort);
//     dp->num_x_groups             = m_cubemap->get_width() / 32;
//     dp->num_y_groups             = m_cubemap->get_height() / 32;
//     dp->num_z_groups             = 6;
//
//     // We need to recalculate mipmaps
//     calculate_mipmaps_command* cm = compute_commands->create<calculate_mipmaps_command>(command_keys::no_sort);
//     cm->texture_name              = m_cubemap->get_name();
//
//     add_memory_barrier_command* amb = compute_commands->create<add_memory_barrier_command>(command_keys::no_sort);
//     amb->barrier_bit                = memory_barrier_bit::shader_image_access_barrier_bit;
//
//     calculate_ibl_maps(compute_commands);
//
//     {
//         GL_NAMED_PROFILE_ZONE("Generating IBL");
//         compute_commands->execute();
//     }
// }
*/
