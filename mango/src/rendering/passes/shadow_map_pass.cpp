//! \file      shadow_map_pass.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2022
//! \copyright Apache License 2.0

#include <mango/imgui_helper.hpp>
#include <mango/profile.hpp>
#include <rendering/passes/shadow_map_pass.hpp>
#include <rendering/renderer_bindings.hpp>
#include <rendering/renderer_impl.hpp>
#include <resources/resources_impl.hpp>
#include <scene/scene_impl.hpp>
#include <util/helpers.hpp>

using namespace mango;

shadow_map_pass::shadow_map_pass(const shadow_settings& settings)
    : m_settings(settings)
{
    PROFILE_ZONE;

    m_shadow_data.shadow_resolution                  = settings.get_resolution();
    m_shadow_data.shadow_sample_count                = settings.get_sample_count();
    m_shadow_data.shadow_width                       = settings.get_shadow_width();
    m_shadow_map_offset                              = settings.get_offset();
    m_shadow_data.shadow_cascade_count               = settings.get_cascade_count();
    m_shadow_data.shadow_slope_bias                  = settings.get_slope_bias();
    m_shadow_data.shadow_normal_bias                 = settings.get_normal_bias();
    m_shadow_data.shadow_filter_mode                 = static_cast<int32>(settings.get_filter_mode());
    m_cascade_data.lambda                            = settings.get_split_lambda();
    m_shadow_data.shadow_cascade_interpolation_range = settings.get_cascade_interpolation_range();
    m_shadow_data.shadow_light_size                  = settings.get_light_size();
    MANGO_ASSERT(m_shadow_data.shadow_resolution % 2 == 0, "Shadow Map Resolution has to be a multiple of 2!");
    MANGO_ASSERT(m_shadow_data.shadow_sample_count >= 8 && m_shadow_data.shadow_sample_count <= 64, "Sample count is not in valid range 8 - 64!");
    MANGO_ASSERT(m_shadow_data.shadow_cascade_count > 0 && m_shadow_data.shadow_cascade_count < 5, "Cascade count has to be between 1 and 4!");
    MANGO_ASSERT(m_cascade_data.lambda > 0.0f && m_cascade_data.lambda < 1.0f, "Lambda has to be between 0.0 and 1.0!");
}

void shadow_map_pass::setup(const shared_ptr<renderer_pipeline_cache>& pipeline_cache, const shared_ptr<debug_drawer>& dbg_drawer)
{
    m_pipeline_cache = pipeline_cache;
    m_debug_drawer   = dbg_drawer;
}

shadow_map_pass::~shadow_map_pass() {}

bool shadow_map_pass::create_pass_resources()
{
    PROFILE_ZONE;
    auto& graphics_device = m_shared_context->get_graphics_device();
    // buffers
    buffer_create_info buffer_info;
    buffer_info.buffer_target = gfx_buffer_target::buffer_target_uniform;
    buffer_info.buffer_access = gfx_buffer_access::buffer_access_dynamic_storage;
    buffer_info.size          = sizeof(shadow_data);

    m_shadow_data_buffer = graphics_device->create_buffer(buffer_info);
    if (!check_creation(m_shadow_data_buffer.get(), "shadow m_shadow_data buffer"))
        return false;

    // textures
    if (!create_shadow_map())
        return false;

    // shader stages
    auto& internal_resources = m_shared_context->get_internal_resources();
    shader_stage_create_info shader_info;
    shader_resource_resource_description res_resource_desc;
    shader_source_description source_desc;

    // vertex stage
    {
        res_resource_desc.path        = "res/shader/shadow/v_shadow_pass.glsl";
        const shader_resource* source = internal_resources->acquire(res_resource_desc);

        source_desc.entry_point = "main";
        source_desc.source      = source->source.c_str();
        source_desc.size        = static_cast<int32>(source->source.size());

        shader_info.stage         = gfx_shader_stage_type::shader_stage_vertex;
        shader_info.shader_source = source_desc;

        shader_info.resource_count = 1;

        shader_info.resources = { { { gfx_shader_stage_type::shader_stage_vertex, MODEL_DATA_BUFFER_BINDING_POINT, "model_data", gfx_shader_resource_type::shader_resource_constant_buffer, 1 } } };

        m_shadow_pass_vertex = graphics_device->create_shader_stage(shader_info);
        if (!check_creation(m_shadow_pass_vertex.get(), "shadow pass vertex shader"))
            return false;

        res_resource_desc.defines.clear();
    }
    // geometry stage
    {
        res_resource_desc.path        = "res/shader/shadow/g_shadow_pass.glsl";
        const shader_resource* source = internal_resources->acquire(res_resource_desc);

        source_desc.entry_point = "main";
        source_desc.source      = source->source.c_str();
        source_desc.size        = static_cast<int32>(source->source.size());

        shader_info.stage         = gfx_shader_stage_type::shader_stage_geometry;
        shader_info.shader_source = source_desc;

        shader_info.resource_count = 1;

        shader_info.resources = { { { gfx_shader_stage_type::shader_stage_geometry, SHADOW_DATA_BUFFER_BINDING_POINT, "shadow_data", gfx_shader_resource_type::shader_resource_constant_buffer, 1 } } };

        m_shadow_pass_geometry = graphics_device->create_shader_stage(shader_info);
        if (!check_creation(m_shadow_pass_geometry.get(), "shadow pass geometry shader"))
            return false;

        res_resource_desc.defines.clear();
    }
    // fragment stage
    {
        res_resource_desc.path        = "res/shader/shadow/f_shadow_pass.glsl";
        const shader_resource* source = internal_resources->acquire(res_resource_desc);

        source_desc.entry_point = "main";
        source_desc.source      = source->source.c_str();
        source_desc.size        = static_cast<int32>(source->source.size());

        shader_info.stage         = gfx_shader_stage_type::shader_stage_fragment;
        shader_info.shader_source = source_desc;

        shader_info.resource_count = 3;

        shader_info.resources = { {
            { gfx_shader_stage_type::shader_stage_fragment, MATERIAL_DATA_BUFFER_BINDING_POINT, "material_data", gfx_shader_resource_type::shader_resource_constant_buffer, 1 },

            { gfx_shader_stage_type::shader_stage_fragment, GEOMETRY_TEXTURE_SAMPLER_BASE_COLOR, "texture_base_color", gfx_shader_resource_type::shader_resource_input_attachment, 1 },
            { gfx_shader_stage_type::shader_stage_fragment, GEOMETRY_TEXTURE_SAMPLER_BASE_COLOR, "sampler_base_color", gfx_shader_resource_type::shader_resource_sampler, 1 },
        } };

        m_shadow_pass_fragment = graphics_device->create_shader_stage(shader_info);
        if (!check_creation(m_shadow_pass_fragment.get(), "shadow pass fragment shader"))
            return false;

        res_resource_desc.defines.clear();
    }
    // Pass Pipeline Base
    {
        auto shadow_pass_info            = graphics_device->provide_graphics_pipeline_create_info();
        auto shadow_pass_pipeline_layout = graphics_device->create_pipeline_resource_layout({
            { gfx_shader_stage_type::shader_stage_vertex, MODEL_DATA_BUFFER_BINDING_POINT, gfx_shader_resource_type::shader_resource_constant_buffer,
              gfx_shader_resource_access::shader_access_dynamic },

            { gfx_shader_stage_type::shader_stage_geometry, SHADOW_DATA_BUFFER_BINDING_POINT, gfx_shader_resource_type::shader_resource_constant_buffer,
              gfx_shader_resource_access::shader_access_dynamic },

            { gfx_shader_stage_type::shader_stage_fragment, 0, gfx_shader_resource_type::shader_resource_input_attachment, gfx_shader_resource_access::shader_access_dynamic },
            { gfx_shader_stage_type::shader_stage_fragment, 0, gfx_shader_resource_type::shader_resource_sampler, gfx_shader_resource_access::shader_access_dynamic },

            { gfx_shader_stage_type::shader_stage_fragment, MATERIAL_DATA_BUFFER_BINDING_POINT, gfx_shader_resource_type::shader_resource_constant_buffer,
              gfx_shader_resource_access::shader_access_dynamic },
        });

        shadow_pass_info.pipeline_layout = shadow_pass_pipeline_layout;

        shadow_pass_info.shader_stage_descriptor.vertex_shader_stage   = m_shadow_pass_vertex;
        shadow_pass_info.shader_stage_descriptor.geometry_shader_stage = m_shadow_pass_geometry;
        shadow_pass_info.shader_stage_descriptor.fragment_shader_stage = m_shadow_pass_fragment;

        // vertex_input_descriptor comes from the mesh to render.
        // input_assembly_descriptor comes from the mesh to render.

        // viewport_descriptor is dynamic

        // rasterization_state -> keep default
        shadow_pass_info.rasterization_state.cull_mode               = gfx_cull_mode_flag_bits::mode_none;
        shadow_pass_info.rasterization_state.enable_depth_bias       = true;
        shadow_pass_info.rasterization_state.depth_bias_slope_factor = 1.1f;
        shadow_pass_info.rasterization_state.constant_depth_bias     = 4.0f;
        // depth_stencil_state -> keep default
        shadow_pass_info.blend_state.blend_description.color_write_mask = gfx_color_component_flag_bits::component_none;

        shadow_pass_info.dynamic_state.dynamic_states = gfx_dynamic_state_flag_bits::dynamic_state_viewport | gfx_dynamic_state_flag_bits::dynamic_state_scissor;

        m_pipeline_cache->set_shadow_base(shadow_pass_info);
    }

    return true;
}

bool shadow_map_pass::create_shadow_map()
{
    auto& graphics_device = m_shared_context->get_graphics_device();

    texture_create_info shadow_map_info;
    shadow_map_info.texture_type   = gfx_texture_type::texture_type_2d_array;
    shadow_map_info.width          = m_shadow_data.shadow_resolution;
    shadow_map_info.height         = m_shadow_data.shadow_resolution;
    shadow_map_info.miplevels      = 1;
    shadow_map_info.array_layers   = max_shadow_mapping_cascades;
    shadow_map_info.texture_format = gfx_format::depth_component32;

    m_shadow_map = graphics_device->create_texture(shadow_map_info);
    if (!check_creation(m_shadow_map.get(), "shadow map texture"))
        return false;

    return true;
}

void shadow_map_pass::attach(const shared_ptr<context_impl>& context)
{
    m_shared_context = context;

    create_pass_resources();
}

void shadow_map_pass::execute(graphics_device_context_handle& device_context)
{
    auto warn_missing_draw = [](string what) { MANGO_LOG_WARN("{0} missing for draw. Skipping DrawCall!", what); };

    m_rpei.draw_calls = 0;
    m_rpei.vertices   = 0;

    GL_NAMED_PROFILE_ZONE("Shadow Pass");
    NAMED_PROFILE_ZONE("Shadow Pass");

    if (!m_debug_view_enabled && !m_shadow_casters.empty())
    {
        for (auto& sc : m_shadow_casters)
        {
            update_cascades(sc.direction);
            device_context->set_render_targets(0, nullptr, m_shadow_map);

            for (int32 casc = 0; casc < m_shadow_data.shadow_cascade_count; ++casc)
            {
                auto& cascade_frustum = m_cascade_data.frusta[casc];

                m_shadow_data.shadow_cascade = casc;

                if (m_debug_bounds)
                {
                    auto corners = bounding_frustum::get_corners(m_shadow_data.shadow_view_projection_matrices[casc]);
                    m_debug_drawer->set_color(color_rgb(0.5f));
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

                for (uint32 c = 0; c < m_draws->size(); ++c)
                {
                    auto& dc = m_draws->operator[](c);

                    if (m_frustum_culling)
                    {
                        auto& bb = dc.bounding_box;
                        if (!cascade_frustum.intersects(bb))
                            continue;
                    }

                    optional<primitive_gpu_data&> prim_gpu_data = m_scene->get_primitive_gpu_data(dc.primitive_gpu_data_id);
                    if (!prim_gpu_data)
                    {
                        warn_missing_draw("Primitive gpu m_shadow_data");
                        continue;
                    }
                    optional<mesh_gpu_data&> m_gpu_data = m_scene->get_mesh_gpu_data(dc.mesh_gpu_data_id);
                    if (!m_gpu_data)
                    {
                        warn_missing_draw("Mesh gpu m_shadow_data");
                        continue;
                    }
                    optional<material&> mat                   = m_scene->get_material(dc.material_hnd);
                    optional<material_gpu_data&> mat_gpu_data = m_scene->get_material_gpu_data(mat->gpu_data);
                    if (!mat || !mat_gpu_data)
                    {
                        warn_missing_draw("Material");
                        continue;
                    }

                    gfx_handle<const gfx_pipeline> dc_pipeline = m_pipeline_cache->get_shadow(prim_gpu_data->vertex_layout, prim_gpu_data->input_assembly, mat->double_sided);

                    device_context->bind_pipeline(dc_pipeline);
                    gfx_viewport shadow_viewport{ 0.0f, 0.0f, static_cast<float>(m_shadow_data.shadow_resolution), static_cast<float>(m_shadow_data.shadow_resolution) };
                    device_context->set_viewport(0, 1, &shadow_viewport);

                    device_context->set_buffer_data(m_shadow_data_buffer, 0, sizeof(shadow_data), &(m_shadow_data));
                    dc_pipeline->get_resource_mapping()->set("shadow_data", m_shadow_data_buffer);

                    dc_pipeline->get_resource_mapping()->set("model_data", m_gpu_data->model_data_buffer);

                    if (mat_gpu_data->per_material_data.alpha_mode > 1)
                        continue; // TODO Paul: Transparent shadows?!

                    dc_pipeline->get_resource_mapping()->set("material_data", mat_gpu_data->material_data_buffer);

                    if (mat_gpu_data->per_material_data.base_color_texture)
                    {
                        MANGO_ASSERT(mat->base_color_texture_gpu_data.has_value(), "Texture has no gpu m_shadow_data!");
                        optional<texture_gpu_data&> tex = m_scene->get_texture_gpu_data(mat->base_color_texture_gpu_data.value());
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
                        dc_pipeline->get_resource_mapping()->set("texture_base_color", m_default_texture_2D);
                    }

                    device_context->submit_pipeline_state_resources();

                    device_context->set_index_buffer(prim_gpu_data->index_buffer_view.graphics_buffer, prim_gpu_data->index_type);

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

                    device_context->set_vertex_buffers(static_cast<int32>(prim_gpu_data->vertex_buffer_views.size()), vbs.data(), bindings.data(), offsets.data());

                    m_rpei.draw_calls++;
                    m_rpei.vertices += std::max(prim_gpu_data->draw_call_desc.vertex_count, prim_gpu_data->draw_call_desc.index_count);
                    device_context->draw(prim_gpu_data->draw_call_desc.vertex_count, prim_gpu_data->draw_call_desc.index_count, prim_gpu_data->draw_call_desc.instance_count,
                                         prim_gpu_data->draw_call_desc.base_vertex, prim_gpu_data->draw_call_desc.base_instance, prim_gpu_data->draw_call_desc.index_offset);
                }
            }
        }
    }
}

void shadow_map_pass::update_cascades(const vec3& directional_light_direction)
{
    // TODO Paul: Think of a better way to limit and handle the updates...
    // Update only with 30 fps
    static float fps_lock = 0.0f;
    fps_lock += m_dt;
    if (fps_lock * 1000.0f < 1.0f / 30.0f)
        return;
    fps_lock -= 1.0f / 30.0f;

    m_cascade_data.camera_near                 = m_camera_near;
    m_cascade_data.camera_far                  = m_camera_far;
    m_cascade_data.directional_light_direction = directional_light_direction;

    const float& clip_near  = m_camera_near;
    const float& clip_far   = m_camera_far;
    const float& clip_range = clip_far - clip_near;
    const float& min_z      = clip_near;
    const float& max_z      = min_z + clip_range;
    const float& ratio      = max_z / min_z;
    const float& range      = max_z - min_z;

    float cascade_splits[max_shadow_mapping_cascades];

    // Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
    for (int32 i = 0; i < m_shadow_data.shadow_cascade_count; ++i)
    {
        float p           = static_cast<float>(i + 1) / static_cast<float>(m_shadow_data.shadow_cascade_count);
        float log         = min_z * std::pow(std::abs(ratio), p);
        float uniform     = min_z + range * p;
        float d           = m_cascade_data.lambda * (log - uniform) + uniform;
        cascade_splits[i] = (d - clip_near) / clip_range;
    }

    // calculate camera frustum in world space
    // TODO Paul: Could we extract this to bounding_frustum?
    vec3 frustum_corners[8] = {
        vec3(-1.0f, 1.0f, -1.0f), vec3(1.0f, 1.0f, -1.0f), vec3(1.0f, -1.0f, -1.0f), vec3(-1.0f, -1.0f, -1.0f),
        vec3(-1.0f, 1.0f, 1.0f),  vec3(1.0f, 1.0f, 1.0f),  vec3(1.0f, -1.0f, 1.0f),  vec3(-1.0f, -1.0f, 1.0f),
    };

    mat4 cam_inv_vp = m_inverse_camera_view_projection;
    for (int32 i = 0; i < 8; ++i)
    {
        vec4 inv           = cam_inv_vp * vec4(frustum_corners[i].x(), frustum_corners[i].y(), frustum_corners[i].z(), 1.0f);
        frustum_corners[i] = inv.head<3>() / inv.w();
    }

    float interpolation   = (m_shadow_data.shadow_cascade_interpolation_range - clip_near) / clip_range;
    float last_split_dist = 0.0f;
    for (int32 casc = 0; casc < m_shadow_data.shadow_cascade_count; ++casc)
    {
        vec3 center = make_vec3(0.0f);
        vec3 current_frustum_corners[8];
        float split_dist = cascade_splits[casc] - interpolation;
        for (int32 i = 0; i < 4; ++i)
        {
            vec3 dist                      = frustum_corners[i + 4] - frustum_corners[i];
            current_frustum_corners[i + 4] = frustum_corners[i] + (dist * split_dist);
            current_frustum_corners[i]     = frustum_corners[i] + (dist * last_split_dist);
            center += current_frustum_corners[i + 4];
            center += current_frustum_corners[i];
        }
        center /= 8.0f;
        last_split_dist = split_dist;

        float radius = 0.0f;
        for (int32 i = 0; i < 8; ++i)
        {
            float distance = (current_frustum_corners[i] - center).norm();
            radius         = max(radius, distance);
        }
        radius = std::ceil(radius * 16.0f) / 16.0f;

        vec3 max_extends = make_vec3(radius);
        vec3 min_extends = -max_extends;

        // calculate view projection

        mat4 projection;
        mat4 view;
        vec3 up             = GLOBAL_UP;
        vec3 light_to_point = -m_cascade_data.directional_light_direction.normalized();
        if (up.dot(light_to_point) < 1e-5f)
            up = GLOBAL_RIGHT;
        view                                  = lookAt(vec3(center - light_to_point * (-min_extends.z() + m_shadow_map_offset)), center, up);
        projection                            = ortho(min_extends.x(), max_extends.x(), min_extends.y(), max_extends.y(), 0.0f, (max_extends.z() - min_extends.z()) + m_shadow_map_offset);
        m_shadow_data.shadow_far_planes[casc] = (max_extends.z() - min_extends.z()) + m_shadow_map_offset;

        mat4 shadow_matrix = projection * view;
        vec4 origin        = vec4(0.0f, 0.0f, 0.0f, 1.0f);
        origin             = shadow_matrix * origin;
        origin *= static_cast<float>(m_shadow_data.shadow_resolution) * 0.5f;

        vec4 rounded;
        rounded.x() = round(origin.x());
        rounded.y() = round(origin.y());
        rounded.z() = round(origin.z());
        rounded.w() = round(origin.w());
        vec4 offset = rounded - origin;
        offset *= 2.0f / static_cast<float>(m_shadow_data.shadow_resolution);
        offset.z() = 0.0f;
        offset.w() = 0.0f;
        projection.col(3) += offset;

        m_shadow_data.shadow_split_depth[casc]              = (clip_near + split_dist * clip_range);
        m_shadow_data.shadow_view_projection_matrices[casc] = projection * view;
        m_cascade_data.frusta[casc]                         = bounding_frustum(view, projection);
    }
}

void shadow_map_pass::on_ui_widget()
{
    ImGui::PushID("shadow_pass");
    // Resolution 512, 1024, 2048, 4096
    const char* resolutions[4] = { "512", "1024", "2048", "4096" };
    int32 r                    = m_shadow_data.shadow_resolution;
    int32 current              = r > 2048 ? 3 : (r > 1024 ? 2 : (r > 512 ? 1 : 0));
    combo("Shadow Map Resolution", resolutions, 4, current, 2);
    m_shadow_data.shadow_resolution = 512 * static_cast<int32>(pow(2, current));
    if (m_shadow_data.shadow_resolution != r)
        create_shadow_map();

    // Filter Type
    const char* filter[3] = { "Hard Shadows", "Soft Shadows", "PCCF Shadows" };
    int32& current_filter = m_shadow_data.shadow_filter_mode;
    combo("Shadow Filter Mode", filter, 3, current_filter, 1);

    int32 default_ivalue[1] = { 16 };
    float default_value[1]  = { 4.0f };

    if (m_shadow_data.shadow_filter_mode == 1)
    {
        int32& sample_count = m_shadow_data.shadow_sample_count;
        slider_int_n("Sample Count", &sample_count, 1, default_ivalue, 8, 64);
        float& width = m_shadow_data.shadow_width;
        slider_float_n("Shadow Width (px)", &width, 1, default_value, 1.0f, 16.0f);
    }

    if (m_shadow_data.shadow_filter_mode == 2)
    {
        int32& sample_count = m_shadow_data.shadow_sample_count;
        slider_int_n("Sample Count", &sample_count, 1, default_ivalue, 8, 64);
        float& l_size = m_shadow_data.shadow_light_size;
        slider_float_n("Light Size PCFF", &l_size, 1, default_value, 1.0f, 16.0f);
    }

    // Offset 0.0 - 100.0
    slider_float_n("Shadow Map Offset", &m_shadow_map_offset, 1, default_value, 0.0f, 100.0f);

    float& s_bias    = m_shadow_data.shadow_slope_bias;
    default_value[0] = 0.005f;
    drag_float_n("Shadow Map Slope Bias", &s_bias, 1, default_value, 0.001f, 0.0f, 0.5f);

    float& n_bias    = m_shadow_data.shadow_normal_bias;
    default_value[0] = 0.01f;
    drag_float_n("Shadow Map Normal Bias", &n_bias, 1, default_value, 0.001f, 0.0f, 0.5f);

    // Cascades 1, 2, 3, 4
    int32& shadow_cascades = m_shadow_data.shadow_cascade_count;
    default_ivalue[0]      = 3;
    slider_int_n("Number Of Shadow Cascades", &shadow_cascades, 1, default_ivalue, 1, 4);
    float& interpolation_range = m_shadow_data.shadow_cascade_interpolation_range;
    default_value[0]           = 0.5f;
    slider_float_n("Cascade Interpolation Range", &interpolation_range, 1, default_value, 0.0f, 10.0f);
    slider_float_n("Cascade Splits Lambda", &m_cascade_data.lambda, 1, default_value, 0.0f, 1.0f);
    ImGui::PopID();
}
