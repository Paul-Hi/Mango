//! \file      geometry_pass.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2022
//! \copyright Apache License 2.0

#include <mango/profile.hpp>
#include <rendering/passes/geometry_pass.hpp>
#include <rendering/renderer_bindings.hpp>
#include <resources/resources_impl.hpp>
#include <scene/scene_impl.hpp>

using namespace mango;

void geometry_pass::setup(const shared_ptr<renderer_pipeline_cache>& pipeline_cache, const shared_ptr<debug_drawer>& dbg_drawer)
{
    m_pipeline_cache = pipeline_cache;
    m_debug_drawer   = dbg_drawer;
}

void geometry_pass::attach(const shared_ptr<context_impl>& context)
{
    m_shared_context = context;

    MANGO_ASSERT(m_pipeline_cache, "Setup not called! Pipeline Cache is null!");
    MANGO_ASSERT(m_debug_drawer, "Setup not called! Debug Drawer is null!");

    create_pass_resources();
}

void geometry_pass::execute(graphics_device_context_handle& device_context)
{
    auto warn_missing_draw = [](string what) { MANGO_LOG_WARN("{0} missing for draw. Skipping DrawCall!", what); };

    m_rpei.draw_calls = 0;
    m_rpei.vertices   = 0;
    // gbuffer pass
    // draw objects
    GL_NAMED_PROFILE_ZONE("GBuffer Pass");
    NAMED_PROFILE_ZONE("GBuffer Pass");
    device_context->set_render_targets(static_cast<int32>(m_render_targets.size()) - 1, m_render_targets.data(), m_render_targets.back());
    for (int32 c = 0; c < m_opaque_count; ++c)
    {
        auto& dc = m_draws->operator[](c);

        if (m_frustum_culling)
        {
            auto& bb = dc.bounding_box;
            if (!m_camera_frustum.intersects(bb))
                continue;
        }
        if (m_debug_bounds)
        {
            auto& bb     = dc.bounding_box;
            auto corners = bb.get_corners();
            m_debug_drawer->set_color(color_rgb(1.0f, 0.0f, 0.0f));
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

        optional<primitive_gpu_data&> prim_gpu_data = m_scene->get_primitive_gpu_data(dc.primitive_gpu_data_id);
        if (!prim_gpu_data)
        {
            warn_missing_draw("Primitive gpu data");
            continue;
        }
        optional<mesh_gpu_data&> m_gpu_data = m_scene->get_mesh_gpu_data(dc.mesh_gpu_data_id);
        if (!m_gpu_data)
        {
            warn_missing_draw("Mesh gpu data");
            continue;
        }
        optional<material&> mat                   = m_scene->get_material(dc.material_hnd);
        optional<material_gpu_data&> mat_gpu_data = m_scene->get_material_gpu_data(mat->gpu_data);
        if (!mat || !mat_gpu_data)
        {
            warn_missing_draw("Material");
            continue;
        }

        gfx_handle<const gfx_pipeline> dc_pipeline = m_pipeline_cache->get_opaque(prim_gpu_data->vertex_layout, prim_gpu_data->input_assembly, m_wireframe, mat->double_sided);

        device_context->bind_pipeline(dc_pipeline);
        device_context->set_viewport(0, 1, &m_viewport);

        dc_pipeline->get_resource_mapping()->set("model_data", m_gpu_data->model_data_buffer);
        dc_pipeline->get_resource_mapping()->set("camera_data", m_camera_data_buffer);
        dc_pipeline->get_resource_mapping()->set("material_data", mat_gpu_data->material_data_buffer);

        if (mat_gpu_data->per_material_data.base_color_texture)
        {
            MANGO_ASSERT(mat->base_color_texture_gpu_data.has_value(), "Texture has no gpu data!");
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
        if (mat_gpu_data->per_material_data.roughness_metallic_texture)
        {
            MANGO_ASSERT(mat->metallic_roughness_texture_gpu_data.has_value(), "Texture has no gpu data!");
            optional<texture_gpu_data&> tex = m_scene->get_texture_gpu_data(mat->metallic_roughness_texture_gpu_data.value());
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
            dc_pipeline->get_resource_mapping()->set("texture_roughness_metallic", m_default_texture_2D);
        }
        if (mat_gpu_data->per_material_data.occlusion_texture)
        {
            MANGO_ASSERT(mat->occlusion_texture_gpu_data.has_value(), "Texture has no gpu data!");
            optional<texture_gpu_data&> tex = m_scene->get_texture_gpu_data(mat->occlusion_texture_gpu_data.value());
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
            dc_pipeline->get_resource_mapping()->set("texture_occlusion", m_default_texture_2D);
        }
        if (mat_gpu_data->per_material_data.normal_texture)
        {
            MANGO_ASSERT(mat->normal_texture_gpu_data.has_value(), "Texture has no gpu data!");
            optional<texture_gpu_data&> tex = m_scene->get_texture_gpu_data(mat->normal_texture_gpu_data.value());
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
            dc_pipeline->get_resource_mapping()->set("texture_normal", m_default_texture_2D);
        }
        if (mat_gpu_data->per_material_data.emissive_color_texture)
        {
            MANGO_ASSERT(mat->emissive_texture_gpu_data.has_value(), "Texture has no gpu data!");
            optional<texture_gpu_data&> tex = m_scene->get_texture_gpu_data(mat->emissive_texture_gpu_data.value());
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
            dc_pipeline->get_resource_mapping()->set("texture_emissive_color", m_default_texture_2D);
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

bool geometry_pass::create_pass_resources()
{
    PROFILE_ZONE;
    auto& graphics_device    = m_shared_context->get_graphics_device();
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

        m_geometry_pass_vertex = graphics_device->create_shader_stage(shader_info);
        if (!check_creation(m_geometry_pass_vertex.get(), "Geometry pass vertex shader"))
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

        m_geometry_pass_fragment = graphics_device->create_shader_stage(shader_info);
        if (!check_creation(m_geometry_pass_fragment.get(), "Geometry pass fragment shader"))
            return false;

        res_resource_desc.defines.clear();
    }

    graphics_pipeline_create_info geometry_pass_info = graphics_device->provide_graphics_pipeline_create_info();
    auto geometry_pass_pipeline_layout               = graphics_device->create_pipeline_resource_layout({
                      { gfx_shader_stage_type::shader_stage_vertex, CAMERA_DATA_BUFFER_BINDING_POINT, gfx_shader_resource_type::shader_resource_constant_buffer, gfx_shader_resource_access::shader_access_dynamic },
                      { gfx_shader_stage_type::shader_stage_vertex, MODEL_DATA_BUFFER_BINDING_POINT, gfx_shader_resource_type::shader_resource_constant_buffer, gfx_shader_resource_access::shader_access_dynamic },

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
                      { gfx_shader_stage_type::shader_stage_fragment, GEOMETRY_TEXTURE_SAMPLER_EMISSIVE_COLOR, gfx_shader_resource_type::shader_resource_sampler, gfx_shader_resource_access::shader_access_dynamic },
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

    m_pipeline_cache->set_opaque_base(geometry_pass_info);

    return true;
}
