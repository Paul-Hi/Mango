//! \file      scene_impl.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2022
//! \copyright Apache License 2.0

#include <core/context_impl.hpp>
#include <glad/glad.h>
#include <mango/profile.hpp>
#include <mango/resources.hpp>
#include <scene/scene_helper.hpp>
#include <scene/scene_impl.hpp>
#include <ui/dear_imgui/icons_font_awesome_5.hpp>
#include <ui/dear_imgui/imgui_glfw.hpp>

using namespace mango;

static int32 get_attrib_component_count_from_tinygltf_types(int32 type);
static gfx_sampler_filter get_texture_filter_from_tinygltf(int32 filter);
static gfx_sampler_edge_wrap get_texture_wrap_from_tinygltf(int32 wrap);

scene_impl::scene_impl(const string& name, const shared_ptr<context_impl>& context)
    : m_shared_context(context)
    , m_light_stack()
    , m_light_gpu_data()
    , m_models()
    , m_scenarios()
    , m_nodes()
    , m_transforms()
    , m_global_transformation_matrices()
    , m_meshes()
    , m_mesh_gpu_data()
    , m_primitives()
    , m_primitive_gpu_data()
    , m_materials()
    , m_material_gpu_data()
    , m_textures()
    , m_texture_gpu_data()
    , m_perspective_cameras()
    , m_orthographic_cameras()
    , m_camera_gpu_data()
    , m_directional_lights()
    , m_skylights()
    , m_atmospheric_lights()
    , m_buffer_views()
    , m_scene_graphics_device(m_shared_context->get_graphics_device())
{
    PROFILE_ZONE;
    MANGO_UNUSED(name);

    // create light stack
    m_light_stack.init(m_shared_context);

    node root("Root");
    root.transform_id     = m_transforms.emplace();
    root.type             = node_type::hierarchy;
    root.global_matrix_id = m_global_transformation_matrices.emplace(mat4::Identity());
    m_root_node           = m_nodes.emplace(root);

    transform& tr    = m_transforms.at(root.transform_id);
    tr.position      = make_vec3(0.0f);
    tr.rotation      = quat(1.0f, 0.0f, 0.0f, 0.0f);
    tr.scale         = make_vec3(1.0f);
    tr.rotation_hint = rad_to_deg(tr.rotation.toRotationMatrix().eulerAngles(1, 2, 0));
}

scene_impl::~scene_impl() {}

uid scene_impl::add_node(const string& name, uid parent_node)
{
    PROFILE_ZONE;

    node new_node(name);
    new_node.transform_id     = m_transforms.emplace();
    new_node.type             = node_type::hierarchy;
    new_node.global_matrix_id = m_global_transformation_matrices.emplace(mat4::Identity());
    uid node_id               = m_nodes.emplace(new_node);

    transform& tr    = m_transforms.at(new_node.transform_id);
    tr.position      = make_vec3(0.0f);
    tr.rotation      = quat(1.0f, 0.0f, 0.0f, 0.0f);
    tr.scale         = make_vec3(1.0f);
    tr.rotation_hint = rad_to_deg(tr.rotation.toRotationMatrix().eulerAngles(1, 2, 0));

    if (!parent_node.is_valid())
    {
        attach(node_id, m_root_node);
    }
    else
    {
        attach(node_id, parent_node);
    }

    return node_id;
}

uid scene_impl::add_perspective_camera(perspective_camera& new_perspective_camera, uid node_id)
{
    PROFILE_ZONE;

    if (!m_nodes.contains(node_id))
    {
        MANGO_LOG_WARN("Node with ID {0} does not exist! Can not add perspective camera!", node_id.get());
        return invalid_uid;
    }

    node& nd = m_nodes.at(node_id);

    buffer_create_info buffer_info;
    buffer_info.buffer_target = gfx_buffer_target::buffer_target_uniform;
    buffer_info.buffer_access = gfx_buffer_access::buffer_access_dynamic_storage;
    buffer_info.size          = sizeof(camera_data);
    camera_gpu_data data;
    data.camera_data_buffer = m_scene_graphics_device->create_buffer(buffer_info);
    if (!check_creation(data.camera_data_buffer.get(), "camera data buffer"))
        return invalid_uid;

    const vec3& camera_position    = m_transforms.at(nd.transform_id).position;
    new_perspective_camera.node_id = node_id;

    mat4 view, projection;
    view_projection_perspective_camera(new_perspective_camera, camera_position, view, projection);
    const mat4 view_projection = projection * view;

    data.per_camera_data.view_matrix             = view;
    data.per_camera_data.projection_matrix       = projection;
    data.per_camera_data.view_projection_matrix  = view_projection;
    data.per_camera_data.inverse_view_projection = view_projection.inverse();
    data.per_camera_data.camera_position         = camera_position;
    data.per_camera_data.camera_near             = new_perspective_camera.z_near;
    data.per_camera_data.camera_far              = new_perspective_camera.z_far;

    if (new_perspective_camera.adaptive_exposure) // Has to be calculated each frame if enabled.
        data.per_camera_data.camera_exposure = 1.0f;
    else
    {
        float ape = new_perspective_camera.physical.aperture;
        float shu = new_perspective_camera.physical.shutter_speed;
        float iso = new_perspective_camera.physical.iso;

        float e                              = ((ape * ape) * 100.0f) / (shu * iso);
        data.per_camera_data.camera_exposure = 1.0f / (1.2f * e);
    }

    auto device_context = m_scene_graphics_device->create_graphics_device_context();
    device_context->begin();
    device_context->set_buffer_data(data.camera_data_buffer, 0, sizeof(camera_data), const_cast<void*>((void*)(&(data.per_camera_data))));
    device_context->end();
    device_context->submit();

    new_perspective_camera.changed  = false;
    new_perspective_camera.gpu_data = m_camera_gpu_data.emplace(data);

    uid camera_id = m_perspective_cameras.emplace(new_perspective_camera);

    if (!m_main_camera_node.is_valid())
    {
        m_main_camera_node = node_id;
    }

    nd.camera_ids[static_cast<uint8>(camera_type::perspective)] = camera_id;
    nd.type |= node_type::perspective_camera;

    return camera_id;
}

uid scene_impl::add_orthographic_camera(orthographic_camera& new_orthographic_camera, uid node_id)
{
    PROFILE_ZONE;

    if (!m_nodes.contains(node_id))
    {
        MANGO_LOG_WARN("Node with ID {0} does not exist! Can not add orthographic camera!", node_id.get());
        return invalid_uid;
    }

    node& nd = m_nodes.at(node_id);

    buffer_create_info buffer_info;
    buffer_info.buffer_target = gfx_buffer_target::buffer_target_uniform;
    buffer_info.buffer_access = gfx_buffer_access::buffer_access_dynamic_storage;
    buffer_info.size          = sizeof(camera_data);
    camera_gpu_data data;
    data.camera_data_buffer = m_scene_graphics_device->create_buffer(buffer_info);
    if (!check_creation(data.camera_data_buffer.get(), "camera data buffer"))
        return invalid_uid;

    const vec3& camera_position     = m_transforms.at(nd.transform_id).position;
    new_orthographic_camera.node_id = node_id;

    mat4 view, projection;
    view_projection_orthographic_camera(new_orthographic_camera, camera_position, view, projection);
    const mat4 view_projection = projection * view;

    data.per_camera_data.view_matrix             = view;
    data.per_camera_data.projection_matrix       = projection;
    data.per_camera_data.view_projection_matrix  = view_projection;
    data.per_camera_data.inverse_view_projection = view_projection.inverse();
    data.per_camera_data.camera_position         = camera_position;
    data.per_camera_data.camera_near             = new_orthographic_camera.z_near;
    data.per_camera_data.camera_far              = new_orthographic_camera.z_far;

    if (new_orthographic_camera.adaptive_exposure) // Has to be calculated each frame if enabled.
        data.per_camera_data.camera_exposure = 1.0f;
    else
    {
        float ape = new_orthographic_camera.physical.aperture;
        float shu = new_orthographic_camera.physical.shutter_speed;
        float iso = new_orthographic_camera.physical.iso;

        float e                              = ((ape * ape) * 100.0f) / (shu * iso);
        data.per_camera_data.camera_exposure = 1.0f / (1.2f * e);
    }

    auto device_context = m_scene_graphics_device->create_graphics_device_context();
    device_context->begin();
    device_context->set_buffer_data(data.camera_data_buffer, 0, sizeof(camera_data), const_cast<void*>((void*)(&(data.per_camera_data))));
    device_context->end();
    device_context->submit();

    new_orthographic_camera.changed  = false;
    new_orthographic_camera.gpu_data = m_camera_gpu_data.emplace(data);

    uid camera_id = m_orthographic_cameras.emplace(new_orthographic_camera);

    if (!m_main_camera_node.is_valid())
    {
        m_main_camera_node = node_id;
    }

    nd.camera_ids[static_cast<uint8>(camera_type::orthographic)] = camera_id;
    nd.type |= node_type::orthographic_camera;

    return camera_id;
}

uid scene_impl::add_directional_light(directional_light& new_directional_light, uid node_id)
{
    PROFILE_ZONE;

    if (!m_nodes.contains(node_id))
    {
        MANGO_LOG_WARN("Node with ID {0} does not exist! Can not add directional light!", node_id.get());
        return invalid_uid;
    }

    uid light_id = m_directional_lights.emplace(new_directional_light);

    node& nd = m_nodes.at(node_id);

    nd.light_ids[static_cast<uint8>(light_type::directional)] = light_id;
    nd.type |= node_type::directional_light;

    return light_id;
}

uid scene_impl::add_skylight(skylight& new_skylight, uid node_id)
{
    PROFILE_ZONE;

    if (!m_nodes.contains(node_id))
    {
        MANGO_LOG_WARN("Node with ID {0} does not exist! Can not add skylight!", node_id.get());
        return invalid_uid;
    }

    uid light_id = m_skylights.emplace(new_skylight);

    node& nd = m_nodes.at(node_id);

    nd.light_ids[static_cast<uint8>(light_type::skylight)] = light_id;
    nd.type |= node_type::skylight;

    return light_id;
}

uid scene_impl::add_atmospheric_light(atmospheric_light& new_atmospheric_light, uid node_id)
{
    PROFILE_ZONE;

    if (!m_nodes.contains(node_id))
    {
        MANGO_LOG_WARN("Node with ID {0} does not exist! Can not add atmospheric light!", node_id.get());
        return invalid_uid;
    }

    uid light_id = m_atmospheric_lights.emplace(new_atmospheric_light);

    node& nd = m_nodes.at(node_id);

    nd.light_ids[static_cast<uint8>(light_type::atmospheric)] = light_id;
    nd.type |= node_type::atmospheric_light;

    return light_id;
}

uid scene_impl::build_material(material& new_material)
{
    PROFILE_ZONE;

    buffer_create_info buffer_info;
    buffer_info.buffer_target = gfx_buffer_target::buffer_target_uniform;
    buffer_info.buffer_access = gfx_buffer_access::buffer_access_dynamic_storage;
    buffer_info.size          = sizeof(material_data);
    material_gpu_data data;
    data.material_data_buffer = m_scene_graphics_device->create_buffer(buffer_info);
    if (!check_creation(data.material_data_buffer.get(), "material data buffer"))
        return invalid_uid;

    data.per_material_data.base_color                 = new_material.base_color.as_vec4();
    data.per_material_data.emissive_color             = new_material.emissive_color.as_vec3();
    data.per_material_data.metallic                   = new_material.metallic;
    data.per_material_data.roughness                  = new_material.roughness;
    data.per_material_data.base_color_texture         = new_material.base_color_texture.is_valid();
    data.per_material_data.roughness_metallic_texture = new_material.metallic_roughness_texture.is_valid();
    data.per_material_data.occlusion_texture          = new_material.occlusion_texture.is_valid();
    data.per_material_data.packed_occlusion           = new_material.packed_occlusion;
    data.per_material_data.normal_texture             = new_material.normal_texture.is_valid();
    data.per_material_data.emissive_color_texture     = new_material.emissive_texture.is_valid();
    data.per_material_data.emissive_intensity         = new_material.emissive_intensity;
    data.per_material_data.alpha_mode                 = static_cast<uint8>(new_material.alpha_mode);
    data.per_material_data.alpha_cutoff               = new_material.alpha_cutoff;

    auto device_context = m_scene_graphics_device->create_graphics_device_context();
    device_context->begin();
    device_context->set_buffer_data(data.material_data_buffer, 0, sizeof(material_data), const_cast<void*>((void*)(&(data.per_material_data))));
    device_context->end();
    device_context->submit();

    new_material.changed  = false;
    new_material.gpu_data = m_material_gpu_data.emplace(data);

    uid material_id = m_materials.emplace(new_material);

    return material_id;
}

uid scene_impl::load_texture_from_image(const string& path, bool standard_color_space, bool high_dynamic_range)
{
    PROFILE_ZONE;

    texture tex;
    tex.file_path            = path;
    tex.standard_color_space = standard_color_space;
    tex.high_dynamic_range   = high_dynamic_range;

    // TODO Paul: We probably want more exposed settings here!
    sampler_create_info sampler_info;
    sampler_info.sampler_min_filter      = gfx_sampler_filter::sampler_filter_linear_mipmap_linear;
    sampler_info.sampler_max_filter      = gfx_sampler_filter::sampler_filter_linear;
    sampler_info.enable_comparison_mode  = false;
    sampler_info.comparison_operator     = gfx_compare_operator::compare_operator_always;
    sampler_info.edge_value_wrap_u       = gfx_sampler_edge_wrap::sampler_edge_wrap_repeat;
    sampler_info.edge_value_wrap_v       = gfx_sampler_edge_wrap::sampler_edge_wrap_repeat;
    sampler_info.edge_value_wrap_w       = gfx_sampler_edge_wrap::sampler_edge_wrap_repeat;
    sampler_info.border_color[0]         = 0;
    sampler_info.border_color[1]         = 0;
    sampler_info.border_color[2]         = 0;
    sampler_info.border_color[3]         = 0;
    sampler_info.enable_seamless_cubemap = false;

    auto texture_sampler_pair = create_gfx_texture_and_sampler(path, standard_color_space, high_dynamic_range, sampler_info);

    texture_gpu_data data;
    data.graphics_texture = texture_sampler_pair.first;
    data.graphics_sampler = texture_sampler_pair.second;
    tex.gpu_data          = m_texture_gpu_data.emplace(data);

    uid texture_id = m_textures.emplace(tex);

    return texture_id;
}

uid scene_impl::load_model_from_gltf(const string& path)
{
    PROFILE_ZONE;

    model mod;
    mod.file_path = path;
    mod.scenarios = load_model_from_file(path, mod.default_scenario);

    uid model_id = m_models.emplace(mod);

    return model_id;
}

uid scene_impl::add_skylight_from_hdr(const string& path, uid node_id)
{
    PROFILE_ZONE;

    if (!m_nodes.contains(node_id))
    {
        MANGO_LOG_WARN("Node with ID {0} does not exist! Can not add skylight!", node_id.get());
        return invalid_uid;
    }

    // texture
    uid texture_id = load_texture_from_image(path, false, true);

    // skylight
    skylight new_skylight;
    new_skylight.hdr_texture = texture_id;
    new_skylight.use_texture = true;

    uid light_id = m_skylights.emplace(new_skylight);

    node& nd = m_nodes.at(node_id);

    nd.light_ids[static_cast<uint8>(light_type::skylight)] = light_id;
    nd.type |= node_type::skylight;

    return light_id;
}

void scene_impl::add_model_to_scene(uid model_to_add, uid scenario_id, uid node_id)
{
    PROFILE_ZONE;

    if (!m_nodes.contains(node_id))
    {
        MANGO_LOG_WARN("Node with ID {0} does not exist! Can not add model to scene!", node_id.get());
        return;
    }

    if (!model_to_add.is_valid() || !scenario_id.is_valid())
    {
        MANGO_LOG_WARN("Model or scenario are not valid! Can not add model to scene!");
        return;
    }

    if (!m_models.contains(model_to_add))
    {
        MANGO_LOG_WARN("Model with ID {0} does not exist! Can not add model to scene!", model_to_add.get());
        return;
    }
    if (!m_scenarios.contains(scenario_id))
    {
        MANGO_LOG_WARN("Scenario with ID {0} does not exist! Can not add model to scene!", scenario_id.get());
        return;
    }

    const model& m = m_models.at(model_to_add);

    auto found = std::find(m.scenarios.begin(), m.scenarios.end(), scenario_id);
    if (found == m.scenarios.end())
    {
        MANGO_LOG_WARN("Model to add does not contain scenario to add! Can not add model to scene!");
        return;
    }

    const scenario& sc = m_scenarios.at(scenario_id);
    for (auto nd : sc.root_nodes)
    {
        instantiate_model_scene(nd, node_id);
    }
}

void scene_impl::remove_node(uid node_id)
{
    PROFILE_ZONE;

    if (!m_nodes.contains(node_id))
    {
        MANGO_LOG_WARN("Node with ID {0} does not exist! Can not remove node!", node_id.get());
        return;
    }
    if (node_id == m_root_node)
    {
        MANGO_LOG_WARN("Can not remove root node!");
        return;
    }

    const node& to_remove = m_nodes.at(node_id);

    if ((to_remove.type & node_type::mesh) != node_type::hierarchy)
        remove_mesh(node_id);
    if ((to_remove.type & node_type::perspective_camera) != node_type::hierarchy)
        remove_perspective_camera(node_id);
    if ((to_remove.type & node_type::orthographic_camera) != node_type::hierarchy)
        remove_orthographic_camera(node_id);
    if ((to_remove.type & node_type::directional_light) != node_type::hierarchy)
        remove_directional_light(node_id);
    if ((to_remove.type & node_type::skylight) != node_type::hierarchy)
        remove_skylight(node_id);
    if ((to_remove.type & node_type::atmospheric_light) != node_type::hierarchy)
        remove_atmospheric_light(node_id);

    for (uid c : to_remove.children)
    {
        remove_node(c);
    }

    m_nodes.erase(node_id);
}

void scene_impl::remove_perspective_camera(uid node_id)
{
    PROFILE_ZONE;

    if (!m_nodes.contains(node_id))
    {
        MANGO_LOG_WARN("Node with ID {0} does not exist! Can not remove perspective camera!", node_id.get());
        return;
    }

    node& node = m_nodes.at(node_id);

    if ((node.type & node_type::perspective_camera) == node_type::hierarchy)
    {
        MANGO_LOG_WARN("Node with ID {0} does not contain a perspective camera! Can not remove perspective camera!", node_id.get());
        return;
    }

    uid camera_id = node.camera_ids[static_cast<uint8>(camera_type::perspective)];

    if (!m_perspective_cameras.contains(camera_id))
    {
        MANGO_LOG_WARN("Perspective camera with ID {0} does not exist! Can not remove perspective camera!", camera_id.get());
        return;
    }

    const perspective_camera& to_remove = m_perspective_cameras.at(camera_id);

    uid gpu_data_id = to_remove.gpu_data;
    MANGO_ASSERT(m_camera_gpu_data.contains(gpu_data_id), "Camera gpu data for perspective camera does not exist!");

    node.type &= ~node_type::perspective_camera;
    node.camera_ids[static_cast<uint8>(camera_type::perspective)] = invalid_uid;

    m_camera_gpu_data.erase(gpu_data_id);
    m_perspective_cameras.erase(camera_id);
}

void scene_impl::remove_orthographic_camera(uid node_id)
{
    PROFILE_ZONE;

    if (!m_nodes.contains(node_id))
    {
        MANGO_LOG_WARN("Node with ID {0} does not exist! Can not remove orthographic camera!", node_id.get());
        return;
    }

    node& node = m_nodes.at(node_id);

    if ((node.type & node_type::orthographic_camera) == node_type::hierarchy)
    {
        MANGO_LOG_WARN("Node with ID {0} does not contain a orthographic camera! Can not remove orthographic camera!", node_id.get());
        return;
    }

    uid camera_id = node.camera_ids[static_cast<uint8>(camera_type::orthographic)];

    if (!m_orthographic_cameras.contains(camera_id))
    {
        MANGO_LOG_WARN("Orthographic camera with ID {0} does not exist! Can not remove orthographic camera!", camera_id.get());
        return;
    }

    const orthographic_camera& to_remove = m_orthographic_cameras.at(camera_id);

    uid gpu_data_id = to_remove.gpu_data;
    MANGO_ASSERT(m_camera_gpu_data.contains(gpu_data_id), "Camera gpu data for orthographic camera does not exist!");

    node.type &= ~node_type::orthographic_camera;
    node.camera_ids[static_cast<uint8>(camera_type::orthographic)] = invalid_uid;

    m_camera_gpu_data.erase(gpu_data_id);
    m_orthographic_cameras.erase(camera_id);
}

void scene_impl::remove_mesh(uid node_id)
{
    PROFILE_ZONE;

    if (!m_nodes.contains(node_id))
    {
        MANGO_LOG_WARN("Node with ID {0} does not exist! Can not remove mesh!", node_id.get());
        return;
    }

    node& node = m_nodes.at(node_id);

    if ((node.type & node_type::mesh) == node_type::hierarchy)
    {
        MANGO_LOG_WARN("Node with ID {0} does not contain a mesh! Can not remove mesh!", node_id.get());
        return;
    }

    uid mesh_id = node.mesh_id;

    if (!m_meshes.contains(mesh_id))
    {
        MANGO_LOG_WARN("Mesh with ID {0} does not exist! Can not remove mesh!", mesh_id.get());
        return;
    }

    const mesh& to_remove = m_meshes.at(mesh_id);

    uid gpu_data_id = to_remove.gpu_data;
    MANGO_ASSERT(m_mesh_gpu_data.contains(gpu_data_id), "Mesh gpu data for mesh does not exist!");

    node.type &= ~node_type::mesh;
    node.mesh_id = invalid_uid;

    m_mesh_gpu_data.erase(gpu_data_id);
    m_meshes.erase(mesh_id);
}

void scene_impl::remove_directional_light(uid node_id)
{
    PROFILE_ZONE;

    if (!m_nodes.contains(node_id))
    {
        MANGO_LOG_WARN("Node with ID {0} does not exist! Can not remove directional light!", node_id.get());
        return;
    }

    node& node = m_nodes.at(node_id);

    if ((node.type & node_type::directional_light) == node_type::hierarchy)
    {
        MANGO_LOG_WARN("Node with ID {0} does not contain a directional light! Can not remove directional light!", node_id.get());
        return;
    }

    uid light_id = node.light_ids[static_cast<uint8>(light_type::directional)];

    if (!m_directional_lights.contains(light_id))
    {
        MANGO_LOG_WARN("Directional light with ID {0} does not exist! Can not remove directional light!", light_id.get());
        return;
    }

    node.type &= ~node_type::directional_light;
    node.camera_ids[static_cast<uint8>(light_type::directional)] = invalid_uid;

    m_directional_lights.erase(light_id);
}

void scene_impl::remove_skylight(uid node_id)
{
    PROFILE_ZONE;

    if (!m_nodes.contains(node_id))
    {
        MANGO_LOG_WARN("Node with ID {0} does not exist! Can not remove skylight!", node_id.get());
        return;
    }

    node& node = m_nodes.at(node_id);

    if ((node.type & node_type::skylight) == node_type::hierarchy)
    {
        MANGO_LOG_WARN("Node with ID {0} does not contain a skylight! Can not remove skylight!", node_id.get());
        return;
    }

    uid light_id = node.light_ids[static_cast<uint8>(light_type::skylight)];

    if (!m_skylights.contains(light_id))
    {
        MANGO_LOG_WARN("Skylight with ID {0} does not exist! Can not remove skylight!", light_id.get());
        return;
    }

    const skylight& to_remove = m_skylights.at(light_id);

    if (to_remove.hdr_texture.is_valid())
    {
        remove_texture(to_remove.hdr_texture);
    }

    node.type &= ~node_type::skylight;
    node.camera_ids[static_cast<uint8>(light_type::skylight)] = invalid_uid;

    m_skylights.erase(light_id);
}

void scene_impl::remove_atmospheric_light(uid node_id)
{
    PROFILE_ZONE;

    if (!m_nodes.contains(node_id))
    {
        MANGO_LOG_WARN("Node with ID {0} does not exist! Can not remove atmospheric light!", node_id.get());
        return;
    }

    node& node = m_nodes.at(node_id);

    if ((node.type & node_type::atmospheric_light) == node_type::hierarchy)
    {
        MANGO_LOG_WARN("Node with ID {0} does not contain a atmospheric light! Can not remove atmospheric light!", node_id.get());
        return;
    }

    uid light_id = node.light_ids[static_cast<uint8>(light_type::atmospheric)];

    if (!m_atmospheric_lights.contains(light_id))
    {
        MANGO_LOG_WARN("Atmospheric light with ID {0} does not exist! Can not remove atmospheric light!", light_id.get());
        return;
    }

    node.type &= ~node_type::atmospheric_light;
    node.camera_ids[static_cast<uint8>(light_type::atmospheric)] = invalid_uid;

    m_atmospheric_lights.erase(light_id);
}

void scene_impl::unload_gltf_model(uid model_id)
{
    PROFILE_ZONE;
    MANGO_ASSERT(false, "Currently not supported!");

    if (!m_models.contains(model_id))
    {
        MANGO_LOG_WARN("Model with ID {0} does not exist! Can not unload model!", model_id.get());
        return;
    }

    const model& m = m_models.at(model_id);

    for (auto sc : m.scenarios)
    {
        if (!m_scenarios.contains(sc))
        {
            MANGO_LOG_WARN("Scenario with ID {0} does not exist! Can not unload model!", sc.get());
            return;
        }

        const scenario& scen = m_scenarios.at(sc);
        for (auto node : scen.root_nodes)
        {
            remove_model_node(node);
        }

        m_scenarios.erase(sc);
    }

    m_models.erase(model_id);
}

void scene_impl::instantiate_model_scene(uid node_id, uid parent_id)
{
    if (!m_nodes.contains(node_id))
    {
        MANGO_LOG_WARN("Node with ID {0} does not exist! Can not instantiate!", node_id.get());
        return;
    }

    node& nd = m_nodes.at(node_id);

    uid instance_id = add_node(nd.name, parent_id);

    node& instance = m_nodes.at(instance_id);

    instance.type = nd.type;

    if (nd.mesh_id.is_valid())
    {
        mesh copy               = m_meshes.at(nd.mesh_id);
        mesh_gpu_data data_copy = m_mesh_gpu_data.at(copy.gpu_data);

        buffer_create_info buffer_info;
        buffer_info.buffer_target   = gfx_buffer_target::buffer_target_uniform;
        buffer_info.buffer_access   = gfx_buffer_access::buffer_access_dynamic_storage;
        buffer_info.size            = sizeof(model_data);
        data_copy.model_data_buffer = nullptr;
        data_copy.model_data_buffer = m_scene_graphics_device->create_buffer(buffer_info);
        if (!check_creation(data_copy.model_data_buffer.get(), "model data buffer"))
            return;

        copy.gpu_data    = m_mesh_gpu_data.emplace(data_copy);
        copy.node_id     = instance_id;
        copy.changed     = true;
        instance.mesh_id = m_meshes.emplace(copy);
    }

    if (nd.camera_ids[0].is_valid())
    {
        perspective_camera copy   = m_perspective_cameras.at(nd.camera_ids[0]);
        camera_gpu_data data_copy = m_camera_gpu_data.at(copy.gpu_data);

        buffer_create_info buffer_info;
        buffer_info.buffer_target    = gfx_buffer_target::buffer_target_uniform;
        buffer_info.buffer_access    = gfx_buffer_access::buffer_access_dynamic_storage;
        buffer_info.size             = sizeof(camera_data);
        data_copy.camera_data_buffer = nullptr;
        data_copy.camera_data_buffer = m_scene_graphics_device->create_buffer(buffer_info);
        if (!check_creation(data_copy.camera_data_buffer.get(), "camera data buffer"))
            return;

        copy.gpu_data          = m_camera_gpu_data.emplace(data_copy);
        copy.node_id           = instance_id;
        copy.changed           = true;
        instance.camera_ids[0] = m_perspective_cameras.emplace(copy);
    }

    if (nd.camera_ids[1].is_valid())
    {
        orthographic_camera copy  = m_orthographic_cameras.at(nd.camera_ids[1]);
        camera_gpu_data data_copy = m_camera_gpu_data.at(copy.gpu_data);

        buffer_create_info buffer_info;
        buffer_info.buffer_target    = gfx_buffer_target::buffer_target_uniform;
        buffer_info.buffer_access    = gfx_buffer_access::buffer_access_dynamic_storage;
        buffer_info.size             = sizeof(camera_data);
        data_copy.camera_data_buffer = nullptr;
        data_copy.camera_data_buffer = m_scene_graphics_device->create_buffer(buffer_info);
        if (!check_creation(data_copy.camera_data_buffer.get(), "camera data buffer"))
            return;

        copy.gpu_data          = m_camera_gpu_data.emplace(data_copy);
        copy.node_id           = instance_id;
        copy.changed           = true;
        instance.camera_ids[1] = m_orthographic_cameras.emplace(copy);
    }

    if (nd.light_ids[0].is_valid())
    {
        directional_light copy = m_directional_lights.at(nd.light_ids[0]);
        copy.changed           = true;
        instance.light_ids[0]  = m_directional_lights.emplace(copy);
    }

    if (nd.light_ids[1].is_valid())
    {
        skylight copy         = m_skylights.at(nd.light_ids[1]);
        copy.changed          = true;
        instance.light_ids[1] = m_skylights.emplace(copy);
    }

    if (nd.light_ids[2].is_valid())
    {
        atmospheric_light& to_copy = m_atmospheric_lights.at(nd.light_ids[2]);
        atmospheric_light copy     = to_copy;
        copy.changed               = true;
        instance.light_ids[2]      = m_atmospheric_lights.emplace(copy);
    }

    transform& tr             = m_transforms.at(nd.transform_id);
    transform& instance_tr    = m_transforms.at(instance.transform_id);
    instance_tr.position      = tr.position;
    instance_tr.rotation      = tr.rotation;
    instance_tr.scale         = tr.scale;
    instance_tr.rotation_hint = tr.rotation_hint;
    instance_tr.changed       = true;

    for (uid c : nd.children)
    {
        instantiate_model_scene(c, instance_id);
    }
}

void scene_impl::remove_model_node(uid node_id)
{
    MANGO_UNUSED(node_id);
    MANGO_ASSERT(false, "Not implemented!");
}

optional<node&> scene_impl::get_node(uid node_id)
{
    PROFILE_ZONE;

    if (!m_nodes.contains(node_id))
    {
        MANGO_LOG_WARN("Node with ID {0} does not exist! Can not retrieve node!", node_id.get());
        return NULL_OPTION;
    }

    return m_nodes.at(node_id);
}

optional<transform&> scene_impl::get_transform(uid node_id)
{
    PROFILE_ZONE;

    if (!m_nodes.contains(node_id))
    {
        MANGO_LOG_WARN("Node with ID {0} does not exist! Can not retrieve transform!", node_id.get());
        return NULL_OPTION;
    }

    const node& nd = m_nodes.at(node_id);

    if (!m_transforms.contains(nd.transform_id))
    {
        MANGO_LOG_WARN("Transform with ID {0} does not exist! Can not retrieve transform!", nd.transform_id.get());
        return NULL_OPTION;
    }

    return m_transforms.at(nd.transform_id);
}

optional<perspective_camera&> scene_impl::get_perspective_camera(uid node_id)
{
    PROFILE_ZONE;

    if (!m_nodes.contains(node_id))
    {
        MANGO_LOG_WARN("Node with ID {0} does not exist! Can not retrieve perspective camera!", node_id.get());
        return NULL_OPTION;
    }

    const node& nd = m_nodes.at(node_id);

    if ((nd.type & node_type::perspective_camera) == node_type::hierarchy)
    {
        MANGO_LOG_WARN("Node with ID {0} does not contain a perspective camera! Can not retrieve perspective camera!", node_id.get());
        return NULL_OPTION;
    }

    uid camera_id = nd.camera_ids[static_cast<uint8>(camera_type::perspective)];

    if (!m_perspective_cameras.contains(camera_id))
    {
        MANGO_LOG_WARN("Perspective camera with ID {0} does not exist! Can not retrieve perspective camera!", camera_id.get());
        return NULL_OPTION;
    }

    return m_perspective_cameras.at(camera_id);
}

optional<orthographic_camera&> scene_impl::get_orthographic_camera(uid node_id)
{
    PROFILE_ZONE;

    if (!m_nodes.contains(node_id))
    {
        MANGO_LOG_WARN("Node with ID {0} does not exist! Can not retrieve orthographic camera!", node_id.get());
        return NULL_OPTION;
    }

    const node& nd = m_nodes.at(node_id);

    if ((nd.type & node_type::orthographic_camera) == node_type::hierarchy)
    {
        MANGO_LOG_WARN("Node with ID {0} does not contain a orthographic camera! Can not retrieve orthographic camera!", node_id.get());
        return NULL_OPTION;
    }

    uid camera_id = nd.camera_ids[static_cast<uint8>(camera_type::orthographic)];

    if (!m_orthographic_cameras.contains(camera_id))
    {
        MANGO_LOG_WARN("Orthographic camera with ID {0} does not exist! Can not retrieve orthographic camera!", camera_id.get());
        return NULL_OPTION;
    }

    return m_orthographic_cameras.at(camera_id);
}

optional<directional_light&> scene_impl::get_directional_light(uid node_id)
{
    PROFILE_ZONE;

    if (!m_nodes.contains(node_id))
    {
        MANGO_LOG_WARN("Node with ID {0} does not exist! Can not retrieve directional light!", node_id.get());
        return NULL_OPTION;
    }

    const node& nd = m_nodes.at(node_id);

    if ((nd.type & node_type::directional_light) == node_type::hierarchy)
    {
        MANGO_LOG_WARN("Node with ID {0} does not contain a directional light! Can not retrieve directional light!", node_id.get());
        return NULL_OPTION;
    }

    uid light_id = nd.light_ids[static_cast<uint8>(light_type::directional)];

    if (!m_directional_lights.contains(light_id))
    {
        MANGO_LOG_WARN("Directional light with ID {0} does not exist! Can not retrieve directional light!", light_id.get());
        return NULL_OPTION;
    }

    return m_directional_lights.at(light_id);
}

optional<skylight&> scene_impl::get_skylight(uid node_id)
{
    PROFILE_ZONE;

    if (!m_nodes.contains(node_id))
    {
        MANGO_LOG_WARN("Node with ID {0} does not exist! Can not retrieve skylight!", node_id.get());
        return NULL_OPTION;
    }

    const node& nd = m_nodes.at(node_id);

    if ((nd.type & node_type::skylight) == node_type::hierarchy)
    {
        MANGO_LOG_WARN("Node with ID {0} does not contain a skylight! Can not retrieve skylight!", node_id.get());
        return NULL_OPTION;
    }

    uid light_id = nd.light_ids[static_cast<uint8>(light_type::skylight)];

    if (!m_skylights.contains(light_id))
    {
        MANGO_LOG_WARN("Skylight with ID {0} does not exist! Can not retrieve skylight!", light_id.get());
        return NULL_OPTION;
    }

    return m_skylights.at(light_id);
}

optional<atmospheric_light&> scene_impl::get_atmospheric_light(uid node_id)
{
    PROFILE_ZONE;

    if (!m_nodes.contains(node_id))
    {
        MANGO_LOG_WARN("Node with ID {0} does not exist! Can not retrieve atmospheric light!", node_id.get());
        return NULL_OPTION;
    }

    const node& nd = m_nodes.at(node_id);

    if ((nd.type & node_type::atmospheric_light) == node_type::hierarchy)
    {
        MANGO_LOG_WARN("Node with ID {0} does not contain a atmospheric light! Can not retrieve atmospheric light!", node_id.get());
        return NULL_OPTION;
    }

    uid light_id = nd.light_ids[static_cast<uint8>(light_type::atmospheric)];

    if (!m_atmospheric_lights.contains(light_id))
    {
        MANGO_LOG_WARN("Atmospheric light with ID {0} does not exist! Can not retrieve atmospheric light!", light_id.get());
        return NULL_OPTION;
    }

    return m_atmospheric_lights.at(light_id);
}

optional<model&> scene_impl::get_model(uid instance_id)
{
    PROFILE_ZONE;

    if (!m_models.contains(instance_id))
    {
        MANGO_LOG_WARN("Model with ID {0} does not exist! Can not retrieve model!", instance_id.get());
        return NULL_OPTION;
    }

    return m_models.at(instance_id);
}

optional<mesh&> scene_impl::get_mesh(uid instance_id)
{
    PROFILE_ZONE;

    if (!m_meshes.contains(instance_id))
    {
        MANGO_LOG_WARN("Mesh with ID {0} does not exist! Can not retrieve mesh!", instance_id.get());
        return NULL_OPTION;
    }

    return m_meshes.at(instance_id);
}

optional<material&> scene_impl::get_material(uid instance_id)
{
    PROFILE_ZONE;

    if (!m_materials.contains(instance_id))
    {
        MANGO_LOG_WARN("Material with ID {0} does not exist! Can not retrieve material!", instance_id.get());
        return NULL_OPTION;
    }

    return m_materials.at(instance_id);
}

optional<texture&> scene_impl::get_texture(uid instance_id)
{
    PROFILE_ZONE;

    if (!m_textures.contains(instance_id))
    {
        MANGO_LOG_WARN("Texture with ID {0} does not exist! Can not retrieve texture!", instance_id.get());
        return NULL_OPTION;
    }

    return m_textures.at(instance_id);
}

uid scene_impl::get_root_node()
{
    return m_root_node;
}

uid scene_impl::get_active_camera_uid()
{
    PROFILE_ZONE;

    if (!m_nodes.contains(m_main_camera_node))
    {
        MANGO_LOG_WARN("Active camera node with ID {0} does not exist! Can not retrieve active camera data!", m_main_camera_node.get());
        return invalid_uid;
    }

    const node& nd = m_nodes.at(m_main_camera_node);

    if ((nd.type & node_type::perspective_camera) != node_type::hierarchy)
    {
        return m_main_camera_node;
    }
    if ((nd.type & node_type::orthographic_camera) != node_type::hierarchy)
    {
        return m_main_camera_node;
    }

    MANGO_LOG_WARN("Active camera node with ID {0} does not contain any camera! Can not retrieve active camera data!", m_main_camera_node.get());
    return invalid_uid;
}

void scene_impl::set_main_camera(uid node_id)
{
    if (!m_nodes.contains(node_id))
    {
        MANGO_LOG_WARN("Node with ID {0} does not exist! Can not set as active camera!", node_id.get());
        return;
    }

    const node& nd = m_nodes.at(node_id);

    if ((nd.type & node_type::perspective_camera) != node_type::hierarchy)
    {
        m_main_camera_node = node_id;
        return;
    }
    if ((nd.type & node_type::orthographic_camera) != node_type::hierarchy)
    {
        m_main_camera_node = node_id;
        return;
    }

    MANGO_LOG_WARN("Node with ID {0} does not contain any camera! Can not set as active camera!", node_id.get());
}

void scene_impl::attach(uid child_node, uid parent_node)
{
    PROFILE_ZONE;

    if (!m_nodes.contains(child_node))
    {
        MANGO_LOG_WARN("Child node with ID {0} does not exist! Can not attach!", child_node.get());
        return;
    }
    if (!m_nodes.contains(parent_node))
    {
        MANGO_LOG_WARN("Parent node with ID {0} does not exist! Can not attach!", parent_node.get());
        return;
    }

    node& child = m_nodes.at(child_node);

    auto found = std::find(child.children.begin(), child.children.end(), parent_node);
    if (found != child.children.end())
    {
        MANGO_LOG_ERROR("Parent is attached to child! Can not attach in a circle!"); // TODO Paul: We need to check the whole subtree!
        return;
    }

    node& parent = m_nodes.at(parent_node);

    parent.children.push_back(child_node);
}

void scene_impl::detach(uid child_node, uid parent_node)
{
    PROFILE_ZONE;

    if (!m_nodes.contains(child_node))
    {
        MANGO_LOG_WARN("Child node with ID {0} does not exist! Can not detach!", child_node.get());
        return;
    }
    if (!m_nodes.contains(parent_node))
    {
        MANGO_LOG_WARN("Parent node with ID {0} does not exist! Can not detach!", parent_node.get());
        return;
    }

    node& parent = m_nodes.at(parent_node);

    auto found = std::find(parent.children.begin(), parent.children.end(), child_node);
    if (found == parent.children.end())
    {
        MANGO_LOG_WARN("Child is not attached to parent! Can not detach!");
        return;
    }

    parent.children.erase(found);
}

void scene_impl::remove_texture(uid texture_id)
{
    PROFILE_ZONE;

    if (!m_textures.contains(texture_id))
    {
        MANGO_LOG_WARN("Texture with ID {0} does not exist! Can not remove texture gpu data!", texture_id.get());
        return;
    }

    const texture& tex = m_textures.at(texture_id);

    if (!m_texture_gpu_data.contains(tex.gpu_data))
    {
        MANGO_LOG_WARN("Texture gpu data with ID {0} does not exist! Can not remove texture gpu data!", tex.gpu_data.get());
        return;
    }

    m_textures.erase(texture_id);
    m_texture_gpu_data.erase(tex.gpu_data);
}

optional<primitive&> scene_impl::get_primitive(uid instance_id)
{
    PROFILE_ZONE;

    if (!m_primitives.contains(instance_id))
    {
        MANGO_LOG_WARN("Primitive with ID {0} does not exist! Can not retrieve primitive!", instance_id.get());
        return NULL_OPTION;
    }

    return m_primitives.at(instance_id);
}

optional<mat4&> scene_impl::get_global_transformation_matrix(uid instance_id)
{
    PROFILE_ZONE;

    if (!m_global_transformation_matrices.contains(instance_id))
    {
        MANGO_LOG_WARN("Global transformation matrix with ID {0} does not exist! Can not retrieve global transformation matrix!", instance_id.get());
        return NULL_OPTION;
    }

    return m_global_transformation_matrices.at(instance_id);
}

optional<texture_gpu_data&> scene_impl::get_texture_gpu_data(uid instance_id)
{
    PROFILE_ZONE;

    if (!m_texture_gpu_data.contains(instance_id))
    {
        MANGO_LOG_WARN("Texture gpu data with ID {0} does not exist! Can not retrieve texture gpu data!", instance_id.get());
        return NULL_OPTION;
    }

    return m_texture_gpu_data.at(instance_id);
}

optional<material_gpu_data&> scene_impl::get_material_gpu_data(uid instance_id)
{
    PROFILE_ZONE;

    if (!m_material_gpu_data.contains(instance_id))
    {
        MANGO_LOG_WARN("Material gpu data with ID {0} does not exist! Can not retrieve material gpu data!", instance_id.get());
        return NULL_OPTION;
    }

    return m_material_gpu_data.at(instance_id);
}

optional<primitive_gpu_data&> scene_impl::get_primitive_gpu_data(uid instance_id)
{
    PROFILE_ZONE;

    if (!m_primitive_gpu_data.contains(instance_id))
    {
        MANGO_LOG_WARN("Primitive gpu data with ID {0} does not exist! Can not retrieve primitive gpu data!", instance_id.get());
        return NULL_OPTION;
    }

    return m_primitive_gpu_data.at(instance_id);
}

optional<mesh_gpu_data&> scene_impl::get_mesh_gpu_data(uid instance_id)
{
    PROFILE_ZONE;

    if (!m_mesh_gpu_data.contains(instance_id))
    {
        MANGO_LOG_WARN("Mesh gpu data with ID {0} does not exist! Can not retrieve mesh gpu data!", instance_id.get());
        return NULL_OPTION;
    }

    return m_mesh_gpu_data.at(instance_id);
}

optional<camera_gpu_data&> scene_impl::get_camera_gpu_data(uid instance_id)
{
    PROFILE_ZONE;

    if (!m_camera_gpu_data.contains(instance_id))
    {
        MANGO_LOG_WARN("Canera gpu data with ID {0} does not exist! Can not retrieve camera gpu data!", instance_id.get());
        return NULL_OPTION;
    }

    return m_camera_gpu_data.at(instance_id);
}

const light_gpu_data& scene_impl::get_light_gpu_data()
{
    return m_light_gpu_data;
}

optional<buffer_view&> scene_impl::get_buffer_view(uid instance_id)
{
    PROFILE_ZONE;

    if (!m_buffer_views.contains(instance_id))
    {
        MANGO_LOG_WARN("Buffer view with ID {0} does not exist! Can not retrieve buffer view!", instance_id.get());
        return NULL_OPTION;
    }

    return m_buffer_views.at(instance_id);
}

optional<camera_gpu_data&> scene_impl::get_active_camera_gpu_data()
{
    PROFILE_ZONE;

    if (!m_nodes.contains(m_main_camera_node))
    {
        MANGO_LOG_WARN("Active camera node with ID {0} does not exist! Can not retrieve active camera gpu data!", m_main_camera_node.get());
        return NULL_OPTION;
    }

    const node& nd = m_nodes.at(m_main_camera_node);

    uid camera_data_id = invalid_uid;

    if ((nd.type & node_type::perspective_camera) != node_type::hierarchy)
    {
        uid camera_id = nd.camera_ids[static_cast<uint8>(camera_type::perspective)];

        if (!m_perspective_cameras.contains(camera_id))
        {
            MANGO_LOG_WARN("Perspective camera with ID {0} does not exist! Can not retrieve camera gpu data!", camera_id.get());
            return NULL_OPTION;
        }
        const perspective_camera& cam = m_perspective_cameras.at(camera_id);
        camera_data_id                = cam.gpu_data;
    }
    if ((nd.type & node_type::orthographic_camera) != node_type::hierarchy)
    {
        uid camera_id = nd.camera_ids[static_cast<uint8>(camera_type::orthographic)];

        if (!m_orthographic_cameras.contains(camera_id))
        {
            MANGO_LOG_WARN("Perspective camera with ID {0} does not exist! Can not retrieve camera gpu data!", camera_id.get());
            return NULL_OPTION;
        }
        const orthographic_camera& cam = m_orthographic_cameras.at(camera_id);
        camera_data_id                 = cam.gpu_data;
    }

    if (!camera_data_id.is_valid() || !m_camera_gpu_data.contains(camera_data_id))
    {
        MANGO_LOG_WARN("Camera gpu data with ID {0} does not exist! Can not retrieve camera gpu data!", camera_data_id.get());
        return NULL_OPTION;
    }

    return m_camera_gpu_data.at(camera_data_id);
}

std::pair<gfx_handle<const gfx_texture>, gfx_handle<const gfx_sampler>> scene_impl::create_gfx_texture_and_sampler(const string& path, bool standard_color_space, bool high_dynamic_range,
                                                                                                                   const sampler_create_info& sampler_info)
{
    auto& graphics_device = m_shared_context->get_graphics_device();

    image_resource_description desc;
    desc.path                    = path.c_str();
    desc.is_standard_color_space = standard_color_space;
    desc.is_hdr                  = high_dynamic_range;

    auto res                  = m_shared_context->get_resources();
    const image_resource* img = res->acquire(desc);
    gfx_format internal       = gfx_format::invalid;
    gfx_format pixel_format   = gfx_format::invalid;
    gfx_format component_type = gfx_format::invalid;
    graphics::get_formats_for_image(img->number_components, img->bits, desc.is_standard_color_space, desc.is_hdr, internal, pixel_format, component_type);

    // TODO Paul: We probably want more exposed settings here!
    texture_create_info tex_info;
    tex_info.texture_type   = gfx_texture_type::texture_type_2d; // TODO Is it?
    tex_info.texture_format = internal;
    tex_info.width          = img->width;
    tex_info.height         = img->height;
    tex_info.miplevels      = graphics::calculate_mip_count(img->width, img->height);
    tex_info.array_layers   = 1;

    std::pair<gfx_handle<const gfx_texture>, gfx_handle<const gfx_sampler>> result;
    result.first  = graphics_device->create_texture(tex_info);
    result.second = graphics_device->create_sampler(sampler_info);

    // upload data
    texture_set_description set_desc;
    set_desc.level          = 0;
    set_desc.x_offset       = 0;
    set_desc.y_offset       = 0;
    set_desc.z_offset       = 0;
    set_desc.width          = img->width;
    set_desc.height         = img->height;
    set_desc.depth          = 1; // TODO Paul: Is it?
    set_desc.pixel_format   = pixel_format;
    set_desc.component_type = component_type;

    auto device_context = graphics_device->create_graphics_device_context();
    device_context->begin();
    device_context->set_texture_data(result.first, set_desc, img->data);
    device_context->calculate_mipmaps(result.first);
    device_context->end();
    device_context->submit();

    return result;
}

std::pair<gfx_handle<const gfx_texture>, gfx_handle<const gfx_sampler>> scene_impl::create_gfx_texture_and_sampler(const image_resource& img, bool standard_color_space, bool high_dynamic_range,
                                                                                                                   const sampler_create_info& sampler_info)
{
    auto& graphics_device = m_shared_context->get_graphics_device();

    gfx_format internal       = gfx_format::invalid;
    gfx_format pixel_format   = gfx_format::invalid;
    gfx_format component_type = gfx_format::invalid;
    graphics::get_formats_for_image(img.number_components, img.bits, standard_color_space, high_dynamic_range, internal, pixel_format, component_type);

    // TODO Paul: We probably want more exposed settings here!
    texture_create_info tex_info;
    tex_info.texture_type   = gfx_texture_type::texture_type_2d; // TODO Is it?
    tex_info.texture_format = internal;
    tex_info.width          = img.width;
    tex_info.height         = img.height;
    tex_info.miplevels      = graphics::calculate_mip_count(img.width, img.height);
    tex_info.array_layers   = 1;

    std::pair<gfx_handle<const gfx_texture>, gfx_handle<const gfx_sampler>> result;
    result.first  = graphics_device->create_texture(tex_info);
    result.second = graphics_device->create_sampler(sampler_info);

    // upload data
    texture_set_description set_desc;
    set_desc.level          = 0;
    set_desc.x_offset       = 0;
    set_desc.y_offset       = 0;
    set_desc.z_offset       = 0;
    set_desc.width          = img.width;
    set_desc.height         = img.height;
    set_desc.depth          = 1; // TODO Paul: Is it?
    set_desc.pixel_format   = pixel_format;
    set_desc.component_type = component_type;

    auto device_context = graphics_device->create_graphics_device_context();
    device_context->begin();
    device_context->set_texture_data(result.first, set_desc, img.data);
    device_context->calculate_mipmaps(result.first);
    device_context->end();
    device_context->submit();

    return result;
}

std::vector<uid> scene_impl::load_model_from_file(const string& path, int32& default_scenario)
{
    auto& graphics_device = m_shared_context->get_graphics_device();

    model_resource_description desc;
    desc.path = path.c_str();

    auto res                 = m_shared_context->get_resources();
    const model_resource* mr = res->acquire(desc);

    tinygltf::Model& m = const_cast<model_resource*>(mr)->gltf_model;

    if (m.scenes.size() <= 0)
    {
        MANGO_LOG_DEBUG("No scenarios in the gltf model found! Can not load invalid gltf.");
        return std::vector<uid>();
    }
    else
    {
        MANGO_LOG_DEBUG("The gltf model has {0} scenarios.", m.scenes.size());
    }

    // load buffer views and data
    std::vector<uid> buffer_view_ids(m.bufferViews.size());
    for (int32 i = 0; i < static_cast<int32>(m.bufferViews.size()); ++i)
    {
        const tinygltf::BufferView& bv = m.bufferViews[i];
        if (bv.target == 0)
        {
            MANGO_LOG_DEBUG("Buffer view target is zero!"); // We can continue here.
        }

        buffer_view view;
        view.offset = 0; // bv.byteOffset; -> Is done on upload.
        view.size   = static_cast<int32>(bv.byteLength);
        view.stride = static_cast<int32>(bv.byteStride);

        buffer_create_info buffer_info;
        buffer_info.buffer_access = gfx_buffer_access::buffer_access_dynamic_storage;
        buffer_info.buffer_target = (bv.target == 0 || bv.target == GL_ARRAY_BUFFER) ? gfx_buffer_target::buffer_target_vertex : gfx_buffer_target::buffer_target_index;
        buffer_info.size          = bv.byteLength;

        view.graphics_buffer = graphics_device->create_buffer(buffer_info);

        // upload data
        const tinygltf::Buffer& buffer = m.buffers[bv.buffer];
        auto device_context            = graphics_device->create_graphics_device_context();
        device_context->begin();
        const unsigned char* buffer_start = buffer.data.data() + bv.byteOffset;
        const void* buffer_data           = static_cast<const void*>(buffer_start);
        device_context->set_buffer_data(view.graphics_buffer, 0, view.size, const_cast<void*>(buffer_data));
        device_context->end();
        device_context->submit();
        // TODO Paul: Are interleaved buffers loaded multiple times? Could we just preprocess them?

        buffer_view_ids[i] = m_buffer_views.emplace(view);
    }

    default_scenario = m.defaultScene > -1 ? m.defaultScene : 0;

    std::vector<uid> all_scenarios;

    buffer_create_info buffer_info;
    buffer_info.buffer_target = gfx_buffer_target::buffer_target_uniform;
    buffer_info.buffer_access = gfx_buffer_access::buffer_access_dynamic_storage;
    buffer_info.size          = sizeof(light_data);
    // light_data is filled by the light_stack
    m_light_gpu_data.light_data_buffer = m_scene_graphics_device->create_buffer(buffer_info);
    if (!check_creation(m_light_gpu_data.light_data_buffer.get(), "light data buffer"))
        return std::vector<uid>();

    for (const tinygltf::Scene& t_scene : m.scenes)
    {
        scenario scen;

        for (int32 i = 0; i < static_cast<int32>(t_scene.nodes.size()); ++i)
        {
            scen.root_nodes.push_back(build_model_node(m, m.nodes.at(t_scene.nodes.at(i)), buffer_view_ids));
        }

        uid scenario_id = m_scenarios.emplace(scen);
        all_scenarios.push_back(scenario_id);
    }

    return all_scenarios;
}

uid scene_impl::build_model_node(tinygltf::Model& m, tinygltf::Node& n, const std::vector<uid>& buffer_view_ids)
{
    PROFILE_ZONE;

    node model_node(n.name);
    model_node.transform_id     = m_transforms.emplace();
    model_node.type             = node_type::hierarchy;
    model_node.global_matrix_id = invalid_uid; // We do not need this here, since we add it when instantiated.
    uid node_id                 = m_nodes.emplace(model_node);
    node& node_ref              = m_nodes.back();

    transform& tr = m_transforms.at(model_node.transform_id);

    if (n.matrix.size() == 16)
    {
        dmat4 dinput = Eigen::Map<Eigen::Matrix<double, 4, 4>>(n.matrix.data());
        mat4 input   = dinput.cast<float>();
        vec3 s;
        vec4 p;
        decompose_transformation(input, tr.scale, tr.rotation, tr.position);
    }
    else
    {
        if (n.translation.size() == 3)
        {
            tr.position = vec3(n.translation[0], n.translation[1], n.translation[2]);
        }
        else
        {
            tr.position = make_vec3(0.0f);
        }
        if (n.rotation.size() == 4)
        {
            tr.rotation = quat(static_cast<float>(n.rotation[3]), static_cast<float>(n.rotation[0]), static_cast<float>(n.rotation[1]), static_cast<float>(n.rotation[2]));
        }
        else
        {
            tr.rotation = quat(1.0f, 0.0f, 0.0f, 0.0f);
        }
        if (n.scale.size() == 3)
        {
            tr.scale = vec3(n.scale[0], n.scale[1], n.scale[2]);
        }
        else
        {
            tr.scale = make_vec3(1.0f);
        }
    }

    tr.rotation_hint = rad_to_deg(tr.rotation.toRotationMatrix().eulerAngles(1, 2, 0));

    if (n.mesh > -1)
    {
        MANGO_ASSERT(n.mesh < static_cast<int32>(m.meshes.size()), "Invalid gltf mesh!");
        MANGO_LOG_DEBUG("Node contains a mesh!");
        node_ref.mesh_id = build_model_mesh(m, m.meshes.at(n.mesh), node_id, buffer_view_ids);
        node_ref.type |= node_type::mesh;
    }

    if (n.camera > -1)
    {
        MANGO_ASSERT(n.camera < static_cast<int32>(m.cameras.size()), "Invalid gltf camera!");
        MANGO_LOG_DEBUG("Node contains a camera!");

        camera_type out_type;
        vec3 target   = tr.rotation * GLOBAL_FORWARD; // TODO Paul: Check that!
        uid camera_id = build_model_camera(m.cameras.at(n.camera), node_id, target, out_type);

        node_ref.camera_ids[static_cast<uint8>(out_type)] = camera_id;
        node_ref.type |= ((out_type == camera_type::perspective) ? node_type::perspective_camera : node_type::orthographic_camera);
    }

    // build child nodes
    for (int32 i = 0; i < static_cast<int32>(n.children.size()); ++i)
    {
        MANGO_ASSERT(n.children[i] < static_cast<int32>(m.nodes.size()), "Invalid gltf node!");

        uid child_id = build_model_node(m, m.nodes.at(n.children.at(i)), buffer_view_ids);
        node_ref.children.push_back(child_id);
    }

    return node_id;
}

uid scene_impl::build_model_camera(tinygltf::Camera& t_camera, uid node_id, const vec3& target, camera_type& out_type)
{
    PROFILE_ZONE;

    if (t_camera.type == "perspective")
    {
        out_type = camera_type::perspective;
        perspective_camera cam;
        cam.z_near                 = static_cast<float>(t_camera.perspective.znear);
        cam.z_far                  = t_camera.perspective.zfar > 0.0 ? static_cast<float>(t_camera.perspective.zfar) : 10000.0f; // Infinite?
        cam.vertical_field_of_view = static_cast<float>(t_camera.perspective.yfov);
        cam.aspect                 = t_camera.perspective.aspectRatio > 0.0 ? static_cast<float>(t_camera.perspective.aspectRatio) : 16.0f / 9.0f;

        cam.physical.aperture      = default_camera_aperture;
        cam.physical.shutter_speed = default_camera_shutter_speed;
        cam.physical.iso           = default_camera_iso;

        cam.target = target;

        cam.node_id = node_id;
        cam.changed = false; // No update needed, since this is only storage.

        cam.gpu_data = m_camera_gpu_data.emplace(camera_gpu_data()); // gpu data is filled on instantiation

        return m_perspective_cameras.emplace(cam);
    }
    if (t_camera.type == "orthographic")
    {
        out_type = camera_type::orthographic;
        orthographic_camera cam;
        cam.z_near = static_cast<float>(t_camera.orthographic.znear);
        cam.z_far  = t_camera.perspective.zfar > 0.0 ? static_cast<float>(t_camera.perspective.zfar) : 10000.0f; // Infinite?
        cam.x_mag  = static_cast<float>(t_camera.orthographic.xmag);
        cam.y_mag  = static_cast<float>(t_camera.orthographic.ymag);

        cam.physical.aperture      = default_camera_aperture;
        cam.physical.shutter_speed = default_camera_shutter_speed;
        cam.physical.iso           = default_camera_iso;

        cam.target  = target;
        cam.changed = false; // No update needed, since this is only storage.

        cam.node_id = node_id;
        cam.changed = false; // No update needed, since this is only storage.

        cam.gpu_data = m_camera_gpu_data.emplace(camera_gpu_data()); // gpu data is filled on instantiation

        return m_orthographic_cameras.emplace(cam);
    }

    return invalid_uid;
}

uid scene_impl::build_model_mesh(tinygltf::Model& m, tinygltf::Mesh& t_mesh, uid node_id, const std::vector<uid>& buffer_view_ids)
{
    PROFILE_ZONE;
    mesh mesh;
    mesh.name    = t_mesh.name;
    mesh.node_id = node_id;
    mesh_gpu_data data;
    data.per_mesh_data.has_normals  = true;
    data.per_mesh_data.has_tangents = true;

    for (int32 i = 0; i < static_cast<int32>(t_mesh.primitives.size()); ++i)
    {
        const tinygltf::Primitive& t_primitive = t_mesh.primitives[i];

        primitive prim;
        primitive_gpu_data prim_gpu_data;

        prim_gpu_data.draw_call_desc.vertex_count   = 0;
        prim_gpu_data.draw_call_desc.instance_count = 1;
        prim_gpu_data.draw_call_desc.base_instance  = 0;
        prim_gpu_data.draw_call_desc.base_vertex    = 0;
        prim_gpu_data.draw_call_desc.index_offset   = 0;

        prim_gpu_data.input_assembly.topology = static_cast<gfx_primitive_topology>(t_primitive.mode + 1); // cast should be okay

        if (t_primitive.indices >= 0)
        {
            const tinygltf::Accessor& index_accessor = m.accessors[t_primitive.indices];

            uid view_id                               = buffer_view_ids[index_accessor.bufferView];
            prim_gpu_data.index_buffer_view           = m_buffer_views.at(view_id);
            prim_gpu_data.index_type                  = static_cast<gfx_format>(index_accessor.componentType); // cast should be okay
            prim_gpu_data.draw_call_desc.index_count  = static_cast<int32>(index_accessor.count);
            prim_gpu_data.draw_call_desc.index_offset = static_cast<int32>(index_accessor.byteOffset);
        }
        else
        {
            prim_gpu_data.draw_call_desc.index_count = 0; // Has to be set!!!
            prim_gpu_data.index_type                 = gfx_format::invalid;
            // vertex_count has to be set later.
        }

        uid material_id;
        if (t_primitive.material >= 0)
            material_id = load_material(m.materials[t_primitive.material], m);
        else
            material_id = default_material();

        prim.material = material_id;

        int32 vertex_buffer_binding = 0;
        int32 description_index     = 0;

        for (auto& attrib : t_primitive.attributes)
        {
            vertex_input_binding_description binding_desc;
            vertex_input_attribute_description attrib_desc;

            const tinygltf::Accessor& accessor = m.accessors[attrib.second];
            if (accessor.sparse.isSparse)
            {
                MANGO_LOG_ERROR("Models with sparse accessors are currently not supported! Undefined behavior!");
                return uid();
            }

            int32 attrib_location = -1;
            if (attrib.first.compare("POSITION") == 0)
                attrib_location = 0;
            else if (attrib.first.compare("NORMAL") == 0)
            {
                attrib_location  = 1;
                prim.has_normals = true;
            }
            else if (attrib.first.compare("TEXCOORD_0") == 0)
                attrib_location = 2;
            else if (attrib.first.compare("TANGENT") == 0)
            {
                attrib_location   = 3;
                prim.has_tangents = true;
            }

            if (attrib_location > -1)
            {
                uid view_id   = buffer_view_ids[accessor.bufferView];
                auto buffer_v = m_buffer_views.at(view_id);
                buffer_v.offset += static_cast<int32>(accessor.byteOffset);
                prim_gpu_data.vertex_buffer_views.emplace_back(buffer_v);

                binding_desc.binding    = vertex_buffer_binding;
                binding_desc.stride     = accessor.ByteStride(m.bufferViews[accessor.bufferView]);
                binding_desc.input_rate = gfx_vertex_input_rate::per_vertex; // TODO Paul: This will probably change later.

                attrib_desc.binding = vertex_buffer_binding;
                attrib_desc.offset  = 0; // TODO Paul: Does that work with interleaved buffers?
                // TODO Paul: Does this work with matrix types?
                attrib_desc.attribute_format =
                    graphics::get_attribute_format_for_component_info(static_cast<gfx_format>(accessor.componentType), get_attrib_component_count_from_tinygltf_types(accessor.type));
                attrib_desc.location = attrib_location;

                if (attrib_location == 0 && prim_gpu_data.index_type == gfx_format::invalid)
                {
                    prim_gpu_data.draw_call_desc.vertex_count = static_cast<int32>(accessor.count);
                }

                vertex_buffer_binding++;
                prim_gpu_data.vertex_layout.binding_descriptions[description_index]   = binding_desc;
                prim_gpu_data.vertex_layout.attribute_descriptions[description_index] = attrib_desc;
                description_index++;

                // AABB
                if (attrib_location == 0)
                {
                    prim.bounding_box = axis_aligned_bounding_box::from_min_max(vec3(accessor.minValues[0], accessor.minValues[1], accessor.minValues[2]),
                                                                                vec3(accessor.maxValues[0], accessor.maxValues[1], accessor.maxValues[2]));
                }
            }
            else
            {
                MANGO_LOG_DEBUG("Vertex attribute is ignored: {0}!", attrib.first);
                continue;
            }
        }
        prim_gpu_data.vertex_layout.binding_description_count   = description_index;
        prim_gpu_data.vertex_layout.attribute_description_count = description_index;

        uid prim_gpu_data_id = m_primitive_gpu_data.emplace(prim_gpu_data);
        prim.gpu_data        = prim_gpu_data_id;
        uid prim_id          = m_primitives.emplace(prim);
        mesh.primitives.push_back(prim_id);

        data.per_mesh_data.has_normals &= prim.has_normals;
        data.per_mesh_data.has_tangents &= prim.has_tangents;
    }

    // data.per_mesh_data.model_matrix is updated in update()
    // data.per_mesh_data.normal_matrix is updated in update()

    mesh.gpu_data = m_mesh_gpu_data.emplace(data);

    mesh.changed = false; // No update needed, since this is only storage.

    return m_meshes.emplace(mesh);
}

uid scene_impl::load_material(const tinygltf::Material& primitive_material, tinygltf::Model& m)
{
    PROFILE_ZONE;

    material new_material;
    new_material.name = "Unnamed";
    if (!primitive_material.name.empty())
    {
        MANGO_LOG_DEBUG("Loading material: {0}", primitive_material.name.c_str());
        new_material.name = primitive_material.name;

        // check if material with that name is alread loaded
        auto cached = material_name_to_uid.find(new_material.name);
        if (cached != material_name_to_uid.end())
        {
            return cached->second;
        }
    }

    new_material.double_sided = primitive_material.doubleSided;

    auto& pbr = primitive_material.pbrMetallicRoughness;

    // TODO Paul: Better structure?!

    sampler_create_info sampler_info;
    sampler_info.sampler_min_filter      = gfx_sampler_filter::sampler_filter_linear_mipmap_linear;
    sampler_info.sampler_max_filter      = gfx_sampler_filter::sampler_filter_linear;
    sampler_info.enable_comparison_mode  = false;
    sampler_info.comparison_operator     = gfx_compare_operator::compare_operator_always;
    sampler_info.edge_value_wrap_u       = gfx_sampler_edge_wrap::sampler_edge_wrap_repeat;
    sampler_info.edge_value_wrap_v       = gfx_sampler_edge_wrap::sampler_edge_wrap_repeat;
    sampler_info.edge_value_wrap_w       = gfx_sampler_edge_wrap::sampler_edge_wrap_repeat;
    sampler_info.border_color[0]         = 0;
    sampler_info.border_color[1]         = 0;
    sampler_info.border_color[2]         = 0;
    sampler_info.border_color[3]         = 0;
    sampler_info.enable_seamless_cubemap = false;

    bool high_dynamic_range = false;

    texture tex;
    tex.file_path = "from_gltf"; // TODO Paul: This could be problematic, when user tries to reload this one.

    if (pbr.baseColorTexture.index < 0)
    {
        auto col                = pbr.baseColorFactor;
        new_material.base_color = color_rgba((float)col[0], (float)col[1], (float)col[2], (float)col[3]);
    }
    else
    {
        // base color
        const tinygltf::Texture& base_col = m.textures.at(pbr.baseColorTexture.index);

        if (base_col.source < 0)
            return invalid_uid;

        const tinygltf::Image& image = m.images[base_col.source];

        if (base_col.sampler >= 0)
        {
            const tinygltf::Sampler& sampler = m.samplers[base_col.sampler];
            sampler_info.sampler_min_filter  = get_texture_filter_from_tinygltf(sampler.minFilter);
            sampler_info.sampler_max_filter  = get_texture_filter_from_tinygltf(sampler.magFilter);
            sampler_info.edge_value_wrap_u   = get_texture_wrap_from_tinygltf(sampler.wrapS);
            sampler_info.edge_value_wrap_v   = get_texture_wrap_from_tinygltf(sampler.wrapT);
            // extension: sampler_info.edge_value_wrap_w   = get_texture_wrap_from_tinygltf(sampler.wrapR);
            sampler_info.sampler_min_filter =
                sampler_info.sampler_min_filter != gfx_sampler_filter::sampler_filter_unknown ? sampler_info.sampler_min_filter : gfx_sampler_filter::sampler_filter_linear_mipmap_linear;
            sampler_info.sampler_max_filter =
                sampler_info.sampler_max_filter != gfx_sampler_filter::sampler_filter_unknown ? sampler_info.sampler_max_filter : gfx_sampler_filter::sampler_filter_linear;
            sampler_info.edge_value_wrap_w = gfx_sampler_edge_wrap::sampler_edge_wrap_repeat;
        }
        else
        {
            sampler_info.sampler_min_filter = gfx_sampler_filter::sampler_filter_linear_mipmap_linear;
            sampler_info.sampler_max_filter = gfx_sampler_filter::sampler_filter_linear;
            sampler_info.edge_value_wrap_u  = gfx_sampler_edge_wrap::sampler_edge_wrap_repeat;
            sampler_info.edge_value_wrap_v  = gfx_sampler_edge_wrap::sampler_edge_wrap_repeat;
            sampler_info.edge_value_wrap_w  = gfx_sampler_edge_wrap::sampler_edge_wrap_repeat;
        }

        bool standard_color_space = true;

        texture tex;
        tex.file_path            = image.uri;
        tex.standard_color_space = standard_color_space;
        tex.high_dynamic_range   = high_dynamic_range;

        image_resource img;
        img.data                                = const_cast<void*>(static_cast<const void*>(image.image.data()));
        img.width                               = image.width;
        img.height                              = image.height;
        img.bits                                = image.bits;
        img.number_components                   = image.component;
        img.description.is_standard_color_space = standard_color_space;
        img.description.is_hdr                  = high_dynamic_range;
        img.description.path                    = image.uri.c_str();

        auto texture_sampler_pair = create_gfx_texture_and_sampler(img, standard_color_space, high_dynamic_range, sampler_info);

        texture_gpu_data data;
        data.graphics_texture = texture_sampler_pair.first;
        data.graphics_sampler = texture_sampler_pair.second;
        tex.gpu_data          = m_texture_gpu_data.emplace(data);

        new_material.base_color_texture          = m_textures.emplace(tex);
        new_material.base_color_texture_gpu_data = tex.gpu_data;
    }

    // metallic / roughness
    if (pbr.metallicRoughnessTexture.index < 0)
    {
        new_material.metallic  = static_cast<float>(pbr.metallicFactor);
        new_material.roughness = static_cast<float>(pbr.roughnessFactor);
    }
    else
    {
        const tinygltf::Texture& o_r_m_t = m.textures.at(pbr.metallicRoughnessTexture.index);

        if (o_r_m_t.source < 0)
            return invalid_uid;

        const tinygltf::Image& image = m.images[o_r_m_t.source];

        if (o_r_m_t.sampler >= 0)
        {
            const tinygltf::Sampler& sampler = m.samplers[o_r_m_t.sampler];
            sampler_info.sampler_min_filter  = get_texture_filter_from_tinygltf(sampler.minFilter);
            sampler_info.sampler_max_filter  = get_texture_filter_from_tinygltf(sampler.magFilter);
            sampler_info.edge_value_wrap_u   = get_texture_wrap_from_tinygltf(sampler.wrapS);
            sampler_info.edge_value_wrap_v   = get_texture_wrap_from_tinygltf(sampler.wrapT);
            // extension: sampler_info.edge_value_wrap_w   = get_texture_wrap_from_tinygltf(sampler.wrapR);
            sampler_info.sampler_min_filter =
                sampler_info.sampler_min_filter != gfx_sampler_filter::sampler_filter_unknown ? sampler_info.sampler_min_filter : gfx_sampler_filter::sampler_filter_linear_mipmap_linear;
            sampler_info.sampler_max_filter =
                sampler_info.sampler_max_filter != gfx_sampler_filter::sampler_filter_unknown ? sampler_info.sampler_max_filter : gfx_sampler_filter::sampler_filter_linear;
            sampler_info.edge_value_wrap_w = gfx_sampler_edge_wrap::sampler_edge_wrap_repeat;
        }
        else
        {
            sampler_info.sampler_min_filter = gfx_sampler_filter::sampler_filter_linear_mipmap_linear;
            sampler_info.sampler_max_filter = gfx_sampler_filter::sampler_filter_linear;
            sampler_info.edge_value_wrap_u  = gfx_sampler_edge_wrap::sampler_edge_wrap_repeat;
            sampler_info.edge_value_wrap_v  = gfx_sampler_edge_wrap::sampler_edge_wrap_repeat;
            sampler_info.edge_value_wrap_w  = gfx_sampler_edge_wrap::sampler_edge_wrap_repeat;
        }

        bool standard_color_space = false;

        texture tex;
        tex.file_path            = image.uri;
        tex.standard_color_space = standard_color_space;
        tex.high_dynamic_range   = high_dynamic_range;

        image_resource img;
        img.data                                = const_cast<void*>(static_cast<const void*>(image.image.data()));
        img.width                               = image.width;
        img.height                              = image.height;
        img.bits                                = image.bits;
        img.number_components                   = image.component;
        img.description.is_standard_color_space = standard_color_space;
        img.description.is_hdr                  = high_dynamic_range;
        img.description.path                    = image.uri.c_str();

        auto texture_sampler_pair = create_gfx_texture_and_sampler(img, standard_color_space, high_dynamic_range, sampler_info);

        texture_gpu_data data;
        data.graphics_texture = texture_sampler_pair.first;
        data.graphics_sampler = texture_sampler_pair.second;
        tex.gpu_data          = m_texture_gpu_data.emplace(data);

        new_material.metallic_roughness_texture          = m_textures.emplace(tex);
        new_material.metallic_roughness_texture_gpu_data = tex.gpu_data;
    }

    // occlusion
    if (primitive_material.occlusionTexture.index >= 0)
    {
        if (pbr.metallicRoughnessTexture.index == primitive_material.occlusionTexture.index)
        {
            // occlusion packed into r channel of the roughness and metallic texture.
            new_material.packed_occlusion = true;
        }
        else
        {
            new_material.packed_occlusion = false;

            const tinygltf::Texture& occ = m.textures.at(primitive_material.occlusionTexture.index);
            if (occ.source < 0)
                return invalid_uid;

            const tinygltf::Image& image = m.images[occ.source];

            if (occ.sampler >= 0)
            {
                const tinygltf::Sampler& sampler = m.samplers[occ.sampler];
                sampler_info.sampler_min_filter  = get_texture_filter_from_tinygltf(sampler.minFilter);
                sampler_info.sampler_max_filter  = get_texture_filter_from_tinygltf(sampler.magFilter);
                sampler_info.edge_value_wrap_u   = get_texture_wrap_from_tinygltf(sampler.wrapS);
                sampler_info.edge_value_wrap_v   = get_texture_wrap_from_tinygltf(sampler.wrapT);
                // extension: sampler_info.edge_value_wrap_w   = get_texture_wrap_from_tinygltf(sampler.wrapR);
                sampler_info.sampler_min_filter =
                    sampler_info.sampler_min_filter != gfx_sampler_filter::sampler_filter_unknown ? sampler_info.sampler_min_filter : gfx_sampler_filter::sampler_filter_linear_mipmap_linear;
                sampler_info.sampler_max_filter =
                    sampler_info.sampler_max_filter != gfx_sampler_filter::sampler_filter_unknown ? sampler_info.sampler_max_filter : gfx_sampler_filter::sampler_filter_linear;
                sampler_info.edge_value_wrap_w = gfx_sampler_edge_wrap::sampler_edge_wrap_repeat;
            }
            else
            {
                sampler_info.sampler_min_filter = gfx_sampler_filter::sampler_filter_linear_mipmap_linear;
                sampler_info.sampler_max_filter = gfx_sampler_filter::sampler_filter_linear;
                sampler_info.edge_value_wrap_u  = gfx_sampler_edge_wrap::sampler_edge_wrap_repeat;
                sampler_info.edge_value_wrap_v  = gfx_sampler_edge_wrap::sampler_edge_wrap_repeat;
                sampler_info.edge_value_wrap_w  = gfx_sampler_edge_wrap::sampler_edge_wrap_repeat;
            }

            bool standard_color_space = false;

            texture tex;
            tex.file_path            = image.uri;
            tex.standard_color_space = standard_color_space;
            tex.high_dynamic_range   = high_dynamic_range;

            image_resource img;
            img.data                                = const_cast<void*>(static_cast<const void*>(image.image.data()));
            img.width                               = image.width;
            img.height                              = image.height;
            img.bits                                = image.bits;
            img.number_components                   = image.component;
            img.description.is_standard_color_space = standard_color_space;
            img.description.is_hdr                  = high_dynamic_range;
            img.description.path                    = image.uri.c_str();

            auto texture_sampler_pair = create_gfx_texture_and_sampler(img, standard_color_space, high_dynamic_range, sampler_info);

            texture_gpu_data data;
            data.graphics_texture = texture_sampler_pair.first;
            data.graphics_sampler = texture_sampler_pair.second;
            tex.gpu_data          = m_texture_gpu_data.emplace(data);

            new_material.occlusion_texture          = m_textures.emplace(tex);
            new_material.occlusion_texture_gpu_data = tex.gpu_data;
        }
    }

    // normal
    if (primitive_material.normalTexture.index >= 0)
    {
        const tinygltf::Texture& norm = m.textures.at(primitive_material.normalTexture.index);

        if (norm.source < 0)
            return invalid_uid;

        const tinygltf::Image& image = m.images[norm.source];

        if (norm.sampler >= 0)
        {
            const tinygltf::Sampler& sampler = m.samplers[norm.sampler];
            sampler_info.sampler_min_filter  = get_texture_filter_from_tinygltf(sampler.minFilter);
            sampler_info.sampler_max_filter  = get_texture_filter_from_tinygltf(sampler.magFilter);
            sampler_info.edge_value_wrap_u   = get_texture_wrap_from_tinygltf(sampler.wrapS);
            sampler_info.edge_value_wrap_v   = get_texture_wrap_from_tinygltf(sampler.wrapT);
            // extension: sampler_info.edge_value_wrap_w   = get_texture_wrap_from_tinygltf(sampler.wrapR);
            sampler_info.sampler_min_filter =
                sampler_info.sampler_min_filter != gfx_sampler_filter::sampler_filter_unknown ? sampler_info.sampler_min_filter : gfx_sampler_filter::sampler_filter_linear_mipmap_linear;
            sampler_info.sampler_max_filter =
                sampler_info.sampler_max_filter != gfx_sampler_filter::sampler_filter_unknown ? sampler_info.sampler_max_filter : gfx_sampler_filter::sampler_filter_linear;
            sampler_info.edge_value_wrap_w = gfx_sampler_edge_wrap::sampler_edge_wrap_repeat;
        }
        else
        {
            sampler_info.sampler_min_filter = gfx_sampler_filter::sampler_filter_linear_mipmap_linear;
            sampler_info.sampler_max_filter = gfx_sampler_filter::sampler_filter_linear;
            sampler_info.edge_value_wrap_u  = gfx_sampler_edge_wrap::sampler_edge_wrap_repeat;
            sampler_info.edge_value_wrap_v  = gfx_sampler_edge_wrap::sampler_edge_wrap_repeat;
            sampler_info.edge_value_wrap_w  = gfx_sampler_edge_wrap::sampler_edge_wrap_repeat;
        }

        bool standard_color_space = false;

        texture tex;
        tex.file_path            = image.uri;
        tex.standard_color_space = standard_color_space;
        tex.high_dynamic_range   = high_dynamic_range;

        image_resource img;
        img.data                                = const_cast<void*>(static_cast<const void*>(image.image.data()));
        img.width                               = image.width;
        img.height                              = image.height;
        img.bits                                = image.bits;
        img.number_components                   = image.component;
        img.description.is_standard_color_space = standard_color_space;
        img.description.is_hdr                  = high_dynamic_range;
        img.description.path                    = image.uri.c_str();

        auto texture_sampler_pair = create_gfx_texture_and_sampler(img, standard_color_space, high_dynamic_range, sampler_info);

        texture_gpu_data data;
        data.graphics_texture = texture_sampler_pair.first;
        data.graphics_sampler = texture_sampler_pair.second;
        tex.gpu_data          = m_texture_gpu_data.emplace(data);

        new_material.normal_texture          = m_textures.emplace(tex);
        new_material.normal_texture_gpu_data = tex.gpu_data;
    }

    // emissive
    if (primitive_material.emissiveTexture.index < 0)
    {
        auto col                    = primitive_material.emissiveFactor;
        new_material.emissive_color = color_rgb((float)col[0], (float)col[1], (float)col[2]);
    }
    else
    {
        const tinygltf::Texture& emissive = m.textures.at(primitive_material.emissiveTexture.index);

        if (emissive.source < 0)
            return invalid_uid;

        const tinygltf::Image& image = m.images[emissive.source];

        if (emissive.sampler >= 0)
        {
            const tinygltf::Sampler& sampler = m.samplers[emissive.sampler];
            sampler_info.sampler_min_filter  = get_texture_filter_from_tinygltf(sampler.minFilter);
            sampler_info.sampler_max_filter  = get_texture_filter_from_tinygltf(sampler.magFilter);
            sampler_info.edge_value_wrap_u   = get_texture_wrap_from_tinygltf(sampler.wrapS);
            sampler_info.edge_value_wrap_v   = get_texture_wrap_from_tinygltf(sampler.wrapT);
            // extension: sampler_info.edge_value_wrap_w   = get_texture_wrap_from_tinygltf(sampler.wrapR);
            sampler_info.sampler_min_filter =
                sampler_info.sampler_min_filter != gfx_sampler_filter::sampler_filter_unknown ? sampler_info.sampler_min_filter : gfx_sampler_filter::sampler_filter_linear_mipmap_linear;
            sampler_info.sampler_max_filter =
                sampler_info.sampler_max_filter != gfx_sampler_filter::sampler_filter_unknown ? sampler_info.sampler_max_filter : gfx_sampler_filter::sampler_filter_linear;
            sampler_info.edge_value_wrap_w = gfx_sampler_edge_wrap::sampler_edge_wrap_repeat;
        }
        else
        {
            sampler_info.sampler_min_filter = gfx_sampler_filter::sampler_filter_linear_mipmap_linear;
            sampler_info.sampler_max_filter = gfx_sampler_filter::sampler_filter_linear;
            sampler_info.edge_value_wrap_u  = gfx_sampler_edge_wrap::sampler_edge_wrap_repeat;
            sampler_info.edge_value_wrap_v  = gfx_sampler_edge_wrap::sampler_edge_wrap_repeat;
            sampler_info.edge_value_wrap_w  = gfx_sampler_edge_wrap::sampler_edge_wrap_repeat;
        }

        bool standard_color_space = true;

        texture tex;
        tex.file_path            = image.uri;
        tex.standard_color_space = standard_color_space;
        tex.high_dynamic_range   = high_dynamic_range;

        image_resource img;
        img.data                                = const_cast<void*>(static_cast<const void*>(image.image.data()));
        img.width                               = image.width;
        img.height                              = image.height;
        img.bits                                = image.bits;
        img.number_components                   = image.component;
        img.description.is_standard_color_space = standard_color_space;
        img.description.is_hdr                  = high_dynamic_range;
        img.description.path                    = image.uri.c_str();

        auto texture_sampler_pair = create_gfx_texture_and_sampler(img, standard_color_space, high_dynamic_range, sampler_info);

        texture_gpu_data data;
        data.graphics_texture = texture_sampler_pair.first;
        data.graphics_sampler = texture_sampler_pair.second;
        tex.gpu_data          = m_texture_gpu_data.emplace(data);

        new_material.emissive_texture          = m_textures.emplace(tex);
        new_material.emissive_texture_gpu_data = tex.gpu_data;
    }

    new_material.emissive_intensity = default_emissive_intensity;

    // transparency
    if (primitive_material.alphaMode.compare("OPAQUE") == 0)
    {
        new_material.alpha_mode   = material_alpha_mode::mode_opaque;
        new_material.alpha_cutoff = 1.0f;
    }
    else if (primitive_material.alphaMode.compare("MASK") == 0)
    {
        new_material.alpha_mode   = material_alpha_mode::mode_mask;
        new_material.alpha_cutoff = static_cast<float>(primitive_material.alphaCutoff);
    }
    else if (primitive_material.alphaMode.compare("BLEND") == 0)
    {
        new_material.alpha_mode   = material_alpha_mode::mode_blend;
        new_material.alpha_cutoff = 1.0f;
    }

    uid material_uid = build_material(new_material);

    if (!primitive_material.name.empty())
    {
        material_name_to_uid.insert({ new_material.name, material_uid });
    }

    return material_uid;
}

uid scene_impl::default_material()
{
    if (m_default_material.is_valid())
        return m_default_material;

    material default_material;
    default_material.name = "Default";
    // Some defaults
    default_material.base_color = vec4(0.9f, 0.9f, 0.9f, 1.0f);
    default_material.metallic   = 0.0f;
    default_material.roughness  = 1.0f;

    m_default_material = build_material(default_material);

    return m_default_material;
}

// entity scene_impl::create_atmospheric_environment(const vec3& sun_direction, float sun_intensity) // TODO Paul: More settings needed!
// {
//     PROFILE_ZONE;
//     entity environment_entity = create_empty();
//
//     auto& environment = m_lights.create_component_for(environment_entity);
//
//     environment.type_of_light          = light_type::environment;
//     environment.data                   = std::make_shared<environment_light_data>();
//     auto el_data                       = static_cast<mango::environment_light_data*>(environment.data.get());
//     el_data->intensity                 = default_skylight_intensity;
//     el_data->render_sun_as_directional = true;
//     el_data->create_atmosphere         = true;
//     // sun data as well as scattering parameters are all default initialized.
//     if (sun_intensity > 0.0)
//     {
//         el_data->sun_data.direction = sun_direction;
//         el_data->sun_data.intensity = sun_intensity;
//     }
//
//     el_data->hdr_texture = nullptr;
//
//     return environment_entity;
// }

void scene_impl::update_scene_graph(uid node_id, uid parent_id, bool force_update)
{
    node& nd      = m_nodes.at(node_id);
    transform& tr = m_transforms.at(nd.transform_id);

    if (tr.changed || force_update)
    {
        // recalculate node matrices
        mat4 local_transformation_matrix = mat4::Identity() * translate(tr.position);
        local_transformation_matrix      = local_transformation_matrix * quaternion_to_mat4(tr.rotation);
        local_transformation_matrix      = local_transformation_matrix * scale(tr.scale);

        mat4 parent_transformation_matrix = mat4::Identity();
        if (parent_id.is_valid())
        {
            const node& parent           = m_nodes.at(parent_id);
            parent_transformation_matrix = m_global_transformation_matrices.at(parent.global_matrix_id);
        }

        mat4& global_transformation_matrix = m_global_transformation_matrices.at(nd.global_matrix_id);

        global_transformation_matrix = parent_transformation_matrix * local_transformation_matrix;

        // Set changed flags
        if ((nd.type & node_type::mesh) != node_type::hierarchy)
        {
            mesh& m   = m_meshes.at(nd.mesh_id);
            m.changed = true;
        }
        if ((nd.type & node_type::perspective_camera) != node_type::hierarchy)
        {
            uid camera_id           = nd.camera_ids[static_cast<uint8>(camera_type::perspective)];
            perspective_camera& cam = m_perspective_cameras.at(camera_id);
            cam.changed             = true;
        }
        if ((nd.type & node_type::orthographic_camera) != node_type::hierarchy)
        {
            uid camera_id            = nd.camera_ids[static_cast<uint8>(camera_type::orthographic)];
            orthographic_camera& cam = m_orthographic_cameras.at(camera_id);
            cam.changed              = true;
        }

        tr.changed   = false;
        force_update = true;
    }
    // light changes are handled by the light stack
    if ((nd.type & node_type::directional_light) != node_type::hierarchy)
    {
        uid light_id         = nd.light_ids[static_cast<uint8>(light_type::directional)];
        directional_light& l = m_directional_lights.at(light_id);
        m_light_stack.push(l);
    }
    if ((nd.type & node_type::skylight) != node_type::hierarchy)
    {
        uid light_id = nd.light_ids[static_cast<uint8>(light_type::skylight)];
        skylight& l  = m_skylights.at(light_id);
        m_light_stack.push(l);
    }
    if ((nd.type & node_type::atmospheric_light) != node_type::hierarchy)
    {
        uid light_id         = nd.light_ids[static_cast<uint8>(light_type::atmospheric)];
        atmospheric_light& l = m_atmospheric_lights.at(light_id);
        m_light_stack.push(l);
    }

    // add to render instances
    m_render_instances.push_back(render_instance(node_id));

    for (auto it = nd.children.begin(); it != nd.children.end();)
    {
        uid c = *it;
        if (c.is_valid())
        {
            update_scene_graph(c, node_id, force_update);
            it++;
        }
        else
        {
            it = nd.children.erase(it);
        }
    }
}

void scene_impl::update(float dt)
{
    PROFILE_ZONE;
    MANGO_UNUSED(dt);

    m_render_instances.clear();

    update_scene_graph(m_root_node, invalid_uid, false);

    // Everything else can be updated in ecs style for changed stuff
    // TODO Paul: This could probably be done in parallel...
    for (auto id : m_meshes)
    {
        mesh& m = m_meshes.at(id);
        if (m.changed)
        {
            mesh_gpu_data& data = m_mesh_gpu_data.at(m.gpu_data);
            const node& nd      = m_nodes.at(m.node_id);
            const mat4& trafo   = m_global_transformation_matrices.at(nd.global_matrix_id);

            data.per_mesh_data.model_matrix  = trafo;
            data.per_mesh_data.normal_matrix = trafo.block(0, 0, 3, 3).inverse().transpose();

            auto device_context = m_scene_graphics_device->create_graphics_device_context();
            device_context->begin();
            device_context->set_buffer_data(data.model_data_buffer, 0, sizeof(model_data), const_cast<void*>((void*)(&(data.per_mesh_data))));
            device_context->end();
            device_context->submit();

            m.changed = false;
        }
    }
    m_requires_auto_exposure = false;
    for (auto id : m_perspective_cameras)
    {
        perspective_camera& cam = m_perspective_cameras.at(id);
        m_requires_auto_exposure |= cam.adaptive_exposure;
        if (cam.changed)
        {
            camera_gpu_data& data = m_camera_gpu_data.at(cam.gpu_data);
            const node& nd        = m_nodes.at(cam.node_id);
            const mat4& trafo     = m_global_transformation_matrices.at(nd.global_matrix_id);
            vec3 camera_position  = trafo.col(3).head<3>();

            mat4 view, projection;
            view_projection_perspective_camera(cam, camera_position, view, projection);
            const mat4 view_projection = projection * view;

            data.per_camera_data.view_matrix             = view;
            data.per_camera_data.projection_matrix       = projection;
            data.per_camera_data.view_projection_matrix  = view_projection;
            data.per_camera_data.inverse_view_projection = view_projection.inverse();
            data.per_camera_data.camera_position         = camera_position;
            data.per_camera_data.camera_near             = cam.z_near;
            data.per_camera_data.camera_far              = cam.z_far;

            float ape;
            float shu;
            float iso;
            if (cam.adaptive_exposure) // Has to be calculated each frame if enabled.
            {
                ape = default_camera_aperture;
                shu = default_camera_shutter_speed;
                iso = default_camera_iso;

                // K is a light meter calibration constant
                static const float K = 12.5f;
                static const float S = 100.0f;
                float target_ev      = log2(m_average_luminance * S / K);

                // Compute the resulting ISO if we left both shutter and aperture here
                iso                 = clamp(((ape * ape) * 100.0f) / (shu * exp2(target_ev)), min_camera_iso, max_camera_iso);
                float unclamped_iso = (shu * exp2(target_ev));
                MANGO_UNUSED(unclamped_iso);

                // Apply half the difference in EV to the aperture
                float ev_diff = target_ev - log2(((ape * ape) * 100.0f) / (shu * iso));
                ape           = clamp(ape * pow(sqrt(2.0f), ev_diff * 0.5f), min_camera_aperture, max_camera_aperture);

                // Apply the remaining difference to the shutter speed
                ev_diff = target_ev - log2(((ape * ape) * 100.0f) / (shu * iso));
                shu     = clamp(shu * pow(2.0f, -ev_diff), min_camera_shutter_speed, max_camera_shutter_speed);
            }
            else
            {
                ape = cam.physical.aperture;
                shu = cam.physical.shutter_speed;
                iso = cam.physical.iso;
            }
            cam.physical.aperture                = clamp(ape, min_camera_aperture, max_camera_aperture);
            cam.physical.shutter_speed           = clamp(shu, min_camera_shutter_speed, max_camera_shutter_speed);
            cam.physical.iso                     = clamp(iso, min_camera_iso, max_camera_iso);
            float e                              = ((ape * ape) * 100.0f) / (shu * iso);
            data.per_camera_data.camera_exposure = 1.0f / (1.2f * e);

            auto device_context = m_scene_graphics_device->create_graphics_device_context();
            device_context->begin();
            device_context->set_buffer_data(data.camera_data_buffer, 0, sizeof(camera_data), const_cast<void*>((void*)(&(data.per_camera_data))));
            device_context->end();
            device_context->submit();

            cam.changed = false;
        }
    }
    for (auto id : m_orthographic_cameras)
    {
        orthographic_camera& cam = m_orthographic_cameras.at(id);
        m_requires_auto_exposure |= cam.adaptive_exposure;
        if (cam.changed)
        {
            camera_gpu_data& data = m_camera_gpu_data.at(cam.gpu_data);
            const node& nd        = m_nodes.at(cam.node_id);
            const mat4& trafo     = m_global_transformation_matrices.at(nd.global_matrix_id);
            vec3 camera_position  = trafo.col(3).head<3>();

            mat4 view, projection;
            view_projection_orthographic_camera(cam, camera_position, view, projection);
            const mat4 view_projection = projection * view;

            data.per_camera_data.view_matrix             = view;
            data.per_camera_data.projection_matrix       = projection;
            data.per_camera_data.view_projection_matrix  = view_projection;
            data.per_camera_data.inverse_view_projection = view_projection.inverse();
            data.per_camera_data.camera_position         = camera_position;
            data.per_camera_data.camera_near             = cam.z_near;
            data.per_camera_data.camera_far              = cam.z_far;

            float ape;
            float shu;
            float iso;
            if (cam.adaptive_exposure) // Has to be calculated each frame if enabled.
            {
                ape = default_camera_aperture;
                shu = default_camera_shutter_speed;
                iso = default_camera_iso;

                // K is a light meter calibration constant
                static const float K = 12.5f;
                static const float S = 100.0f;
                float target_ev      = log2(m_average_luminance * S / K);

                // Compute the resulting ISO if we left both shutter and aperture here
                iso                 = clamp(((ape * ape) * 100.0f) / (shu * exp2(target_ev)), min_camera_iso, max_camera_iso);
                float unclamped_iso = (shu * exp2(target_ev));
                MANGO_UNUSED(unclamped_iso);

                // Apply half the difference in EV to the aperture
                float ev_diff = target_ev - log2(((ape * ape) * 100.0f) / (shu * iso));
                ape           = clamp(ape * pow(sqrt(2.0f), ev_diff * 0.5f), min_camera_aperture, max_camera_aperture);

                // Apply the remaining difference to the shutter speed
                ev_diff = target_ev - log2(((ape * ape) * 100.0f) / (shu * iso));
                shu     = clamp(shu * pow(2.0f, -ev_diff), min_camera_shutter_speed, max_camera_shutter_speed);
            }
            else
            {
                ape = cam.physical.aperture;
                shu = cam.physical.shutter_speed;
                iso = cam.physical.iso;
            }
            cam.physical.aperture                = clamp(ape, min_camera_aperture, max_camera_aperture);
            cam.physical.shutter_speed           = clamp(shu, min_camera_shutter_speed, max_camera_shutter_speed);
            cam.physical.iso                     = clamp(iso, min_camera_iso, max_camera_iso);
            float e                              = ((ape * ape) * 100.0f) / (shu * iso);
            data.per_camera_data.camera_exposure = 1.0f / (1.2f * e);

            auto device_context = m_scene_graphics_device->create_graphics_device_context();
            device_context->begin();
            device_context->set_buffer_data(data.camera_data_buffer, 0, sizeof(camera_data), const_cast<void*>((void*)(&(data.per_camera_data))));
            device_context->end();
            device_context->submit();

            cam.changed = false;
        }
    }

    // Lights are only updated if they are instantiated in the hierarchy.
    m_light_stack.update(this);
    m_light_gpu_data.scene_light_data = m_light_stack.get_light_data();

    auto device_context = m_scene_graphics_device->create_graphics_device_context();
    device_context->begin();
    device_context->set_buffer_data(m_light_gpu_data.light_data_buffer, 0, sizeof(light_data), const_cast<void*>((void*)(&(m_light_gpu_data.scene_light_data))));
    device_context->end();
    device_context->submit();

    for (auto id : m_materials)
    {
        material& mat = m_materials.at(id);
        if (mat.changed)
        {
            material_gpu_data& data = m_material_gpu_data.at(mat.gpu_data);

            data.per_material_data.base_color                 = mat.base_color.as_vec4();
            data.per_material_data.emissive_color             = mat.emissive_color.as_vec3();
            data.per_material_data.metallic                   = mat.metallic;
            data.per_material_data.roughness                  = mat.roughness;
            data.per_material_data.base_color_texture         = mat.base_color_texture.is_valid();
            data.per_material_data.roughness_metallic_texture = mat.metallic_roughness_texture.is_valid();
            data.per_material_data.occlusion_texture          = mat.occlusion_texture.is_valid();
            data.per_material_data.packed_occlusion           = mat.packed_occlusion;
            data.per_material_data.normal_texture             = mat.normal_texture.is_valid();
            data.per_material_data.emissive_color_texture     = mat.emissive_texture.is_valid();
            data.per_material_data.emissive_intensity         = mat.emissive_intensity;
            data.per_material_data.alpha_mode                 = static_cast<uint8>(mat.alpha_mode);
            data.per_material_data.alpha_cutoff               = mat.alpha_cutoff;

            auto device_context = m_scene_graphics_device->create_graphics_device_context();
            device_context->begin();
            device_context->set_buffer_data(data.material_data_buffer, 0, sizeof(material_data), const_cast<void*>((void*)(&(data.per_material_data))));
            device_context->end();
            device_context->submit();

            mat.changed = false;
        }
    }

    for (auto id : m_textures)
    {
        texture& tex = m_textures.at(id);
        if (tex.changed)
        {
            // TODO Paul: This does crash when we do this for textures from a model -.-...

            // just replacing the texture should work, but is not the fancy way. Also we reload the file even though it is not required.

            // TODO Paul: We probably want more exposed settings here!
            sampler_create_info sampler_info;
            sampler_info.sampler_min_filter      = gfx_sampler_filter::sampler_filter_linear_mipmap_linear;
            sampler_info.sampler_max_filter      = gfx_sampler_filter::sampler_filter_linear;
            sampler_info.enable_comparison_mode  = false;
            sampler_info.comparison_operator     = gfx_compare_operator::compare_operator_always;
            sampler_info.edge_value_wrap_u       = gfx_sampler_edge_wrap::sampler_edge_wrap_repeat;
            sampler_info.edge_value_wrap_v       = gfx_sampler_edge_wrap::sampler_edge_wrap_repeat;
            sampler_info.edge_value_wrap_w       = gfx_sampler_edge_wrap::sampler_edge_wrap_repeat;
            sampler_info.border_color[0]         = 0;
            sampler_info.border_color[1]         = 0;
            sampler_info.border_color[2]         = 0;
            sampler_info.border_color[3]         = 0;
            sampler_info.enable_seamless_cubemap = false;

            auto texture_sampler_pair = create_gfx_texture_and_sampler(tex.file_path, tex.standard_color_space, tex.high_dynamic_range, sampler_info);

            texture_gpu_data data = m_texture_gpu_data.at(tex.gpu_data);

            data.graphics_texture = texture_sampler_pair.first;
            data.graphics_sampler = texture_sampler_pair.second;

            tex.changed = false;
        }
    }
}

void scene_impl::draw_scene_hierarchy(uid& selected)
{
    std::vector<uid> to_remove = draw_scene_hierarchy_internal(m_root_node, invalid_uid, selected);
    for (auto n : to_remove)
        remove_node(n);
}

std::vector<uid> scene_impl::draw_scene_hierarchy_internal(uid current, uid parent, uid& selected)
{
    optional<node&> opt_node = get_node(current);
    MANGO_ASSERT(opt_node, "Something is broken - Can not draw hierarchy for a non existing node!");
    node& nd = opt_node.value();

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 5));
    const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_FramePadding |
                                     ImGuiTreeNodeFlags_AllowItemOverlap | ((m_ui_selected_uid == current) ? ImGuiTreeNodeFlags_Selected : 0) | ((nd.children.empty()) ? ImGuiTreeNodeFlags_Leaf : 0);

    ImGui::PushID(current.get());

    string display_name = get_display_name(nd.type, nd.name);
    bool open           = ImGui::TreeNodeEx(display_name.c_str(), flags, "%s", display_name.c_str());
    bool removed        = false;
    ImGui::PopStyleVar();
    if (ImGui::IsItemClicked(0))
    {
        m_ui_selected_uid = current;
    }
    if (ImGui::IsItemClicked(1) && !ImGui::IsPopupOpen(("##object_menu" + std::to_string(current.get())).c_str()))
    {
        m_ui_selected_uid = current;
        ImGui::OpenPopup(("##object_menu" + std::to_string(current.get())).c_str());
    }

    std::vector<uid> to_remove;
    if (ImGui::BeginPopup(("##object_menu" + std::to_string(current.get())).c_str()))
    {
        if (ImGui::Selectable(("Add Node##object_menu" + std::to_string(current.get())).c_str()))
        {
            m_ui_selected_uid = add_node("Node", current);
        }
        if (m_root_node != current && ImGui::Selectable(("Remove Node##object_menu" + std::to_string(current.get())).c_str()))
        {
            m_ui_selected_uid = invalid_uid;
            to_remove.push_back(current);
            removed = true;
        }

        ImGui::EndPopup();
    }
    if (m_root_node != current && !removed && ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
    {
        uid payload[2] = { current, parent };
        ImGui::SetDragDropPayload("DRAG_DROP_NODE", (void*)payload, sizeof(uid) * 2);
        ImGui::EndDragDropSource();
    }
    if (!removed && ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DRAG_DROP_NODE"))
        {
            IM_ASSERT(payload->DataSize == sizeof(uid) * 2);
            const uid* dropped = (const uid*)payload->Data;
            attach(dropped[0], current);
            detach(dropped[0], dropped[1]);
        }
        ImGui::EndDragDropTarget();
    }
    if (open)
    {
        if (!removed)
        {
            for (auto& c : nd.children)
            {
                auto removed_children = draw_scene_hierarchy_internal(c, current, selected);
                to_remove.insert(to_remove.end(), removed_children.begin(), removed_children.end());
                if (removed_children.size() > 0 && removed_children[0] == c)
                    c = invalid_uid;
            }
        }
        ImGui::TreePop();
    }

    ImGui::PopID();
    selected = m_ui_selected_uid;
    return to_remove;
}

string scene_impl::get_display_name(node_type type, const string name) // TODO Paul: Make that better!
{
    string postfix = "";
    if ((type & node_type::mesh) != node_type::hierarchy)
    {
        return name.empty() ? string(ICON_FA_DICE_D6) + "Unnamed" : string(ICON_FA_DICE_D6) + " " + name + postfix;
    }
    if ((type & node_type::perspective_camera) != node_type::hierarchy)
    {
        return name.empty() ? string(ICON_FA_VIDEO) + "Unnamed" : string(ICON_FA_VIDEO) + " " + name + postfix;
    }
    if ((type & node_type::orthographic_camera) != node_type::hierarchy)
    {
        return name.empty() ? string(ICON_FA_VIDEO) + "Unnamed" : string(ICON_FA_VIDEO) + " " + name + postfix;
    }
    if ((type & node_type::directional_light) != node_type::hierarchy)
    {
        return name.empty() ? string(ICON_FA_LIGHTBULB) + "Unnamed" : string(ICON_FA_LIGHTBULB) + " " + name + postfix;
    }
    if ((type & node_type::skylight) != node_type::hierarchy)
    {
        return name.empty() ? string(ICON_FA_LIGHTBULB) + "Unnamed" : string(ICON_FA_LIGHTBULB) + " " + name + postfix;
    }
    if ((type & node_type::atmospheric_light) != node_type::hierarchy)
    {
        return name.empty() ? string(ICON_FA_LIGHTBULB) + "Unnamed" : string(ICON_FA_LIGHTBULB) + " " + name + postfix;
    }
    return name.empty() ? string(ICON_FA_VECTOR_SQUARE) + "Unnamed" : string(ICON_FA_VECTOR_SQUARE) + " " + name + postfix;
}

static int32 get_attrib_component_count_from_tinygltf_types(int32 type)
{
    switch (type)
    {
    case TINYGLTF_TYPE_SCALAR:
        return 1;
    case TINYGLTF_TYPE_VEC2:
        return 2;
    case TINYGLTF_TYPE_VEC3:
        return 3;
    case TINYGLTF_TYPE_VEC4:
        return 4;
    case TINYGLTF_TYPE_MAT2:
        return 2;
    case TINYGLTF_TYPE_MAT3:
        return 3;
    case TINYGLTF_TYPE_MAT4:
        return 4;
    default:
        MANGO_ASSERT(false, "Unknown filter from tinygltf!");
        return -1;
    }
}

static gfx_sampler_filter get_texture_filter_from_tinygltf(int32 filter)
{
    switch (filter)
    {
    case TINYGLTF_TEXTURE_FILTER_NEAREST:
        return gfx_sampler_filter::sampler_filter_nearest;
    case TINYGLTF_TEXTURE_FILTER_LINEAR:
        return gfx_sampler_filter::sampler_filter_linear;
    case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
        return gfx_sampler_filter::sampler_filter_nearest_mipmap_nearest;
    case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
        return gfx_sampler_filter::sampler_filter_linear_mipmap_nearest;
    case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
        return gfx_sampler_filter::sampler_filter_nearest_mipmap_linear;
    case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
        return gfx_sampler_filter::sampler_filter_linear_mipmap_linear;
    default:
        return gfx_sampler_filter::sampler_filter_unknown;
    }
}

static gfx_sampler_edge_wrap get_texture_wrap_from_tinygltf(int32 wrap)
{
    switch (wrap)
    {
    case TINYGLTF_TEXTURE_WRAP_REPEAT:
        return gfx_sampler_edge_wrap::sampler_edge_wrap_repeat;
    case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
        return gfx_sampler_edge_wrap::sampler_edge_wrap_clamp_to_edge;
    case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
        return gfx_sampler_edge_wrap::sampler_edge_wrap_repeat_mirrored;
    default:
        MANGO_ASSERT(false, "Unknown edge wrap from tinygltf!");
        return gfx_sampler_edge_wrap::sampler_edge_wrap_unknown;
    }
}
