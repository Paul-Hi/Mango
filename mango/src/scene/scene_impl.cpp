//! \file      scene_impl.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

//! \cond NO_COND
#define GLM_FORCE_SILENT_WARNINGS 1
//! \endcond
#include <core/context_impl.hpp>
#include <glad/glad.h>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>
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
    , m_models()
    , m_scenarios()
    , m_light_gpu_data()
    , m_nodes()
    , m_transforms()
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
{
    PROFILE_ZONE;
    MANGO_UNUSED(name);

    node root("Root");
    root.transform_id = m_transforms.emplace();
    root.type         = node_type::hierarchy;
    m_root_node       = m_nodes.emplace(root);
}

scene_impl::~scene_impl() {}

uid scene_impl::add_node(const string& name, uid parent_node)
{
    PROFILE_ZONE;

    node new_node(name);
    new_node.transform_id = m_transforms.emplace();
    new_node.type         = node_type::hierarchy;

    uid node_id = m_nodes.emplace(new_node);

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

    const vec3& camera_position = m_transforms.at(nd.transform_id).position;

    mat4 view, projection;
    view_projection_perspective_camera(new_perspective_camera, camera_position, view, projection);
    const mat4 view_projection = projection * view;

    data.per_camera_data.view_matrix             = view;
    data.per_camera_data.projection_matrix       = projection;
    data.per_camera_data.view_projection_matrix  = view_projection;
    data.per_camera_data.inverse_view_projection = glm::inverse(view_projection);
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

    const vec3& camera_position = m_transforms.at(nd.transform_id).position;

    mat4 view, projection;
    view_projection_orthographic_camera(new_orthographic_camera, camera_position, view, projection);
    const mat4 view_projection = projection * view;

    data.per_camera_data.view_matrix             = view;
    data.per_camera_data.projection_matrix       = projection;
    data.per_camera_data.view_projection_matrix  = view_projection;
    data.per_camera_data.inverse_view_projection = glm::inverse(view_projection);
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

    data.per_material_data.base_color                 = new_material.base_color;
    data.per_material_data.emissive_color             = new_material.emissive_color;
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

    const model& m        = m_models.at(model_to_add);
    const scenario& scen  = m_scenarios.at(scenario_id);
    node& containing_node = m_nodes.at(node_id);

    auto found = std::find(m.scenarios.begin(), m.scenarios.end(), scenario_id);
    if (found == m.scenarios.end())
    {
        MANGO_LOG_WARN("Model to add does not contain scenario to add! Can not add model to scene!");
        return;
    }

    if ((containing_node.type & node_type::instantiable) != node_type::hierarchy)
    {
        // TODO Paul: This should be possible later on.
        MANGO_LOG_WARN("Node with ID {0} is already instanced! Can not add model here!", node_id.get());
        return;
    }

    containing_node.children.push_back(scenario_id);
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

    switch (to_remove.type)
    {
    case node_type::instantiable:
        return;
    case node_type::hierarchy:
    case node_type::mesh:
        remove_mesh(to_remove.mesh_id);
    case node_type::perspective_camera:
        remove_perspective_camera(to_remove.camera_ids[static_cast<uint8>(camera_type::perspective)]);
    case node_type::orthographic_camera:
        remove_orthographic_camera(to_remove.camera_ids[static_cast<uint8>(camera_type::orthographic)]);
    case node_type::directional_light:
        remove_directional_light(to_remove.light_ids[static_cast<uint8>(light_type::directional)]);
    case node_type::skylight:
        remove_skylight(to_remove.light_ids[static_cast<uint8>(light_type::skylight)]);
    case node_type::atmospheric_light:
        remove_atmospheric_light(to_remove.light_ids[static_cast<uint8>(light_type::atmospheric)]);
    default:
        break;
    }

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
    MANGO_ASSERT(m_camera_gpu_data.contains(gpu_data_id), "Mesh gpu data for mesh does not exist!");

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
        remove_texture_gpu_data(to_remove.hdr_texture);
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
            remove_instantiable_node(node);
        }

        uid lights_gpu_data = scen.lights_gpu_data;
        MANGO_ASSERT(m_light_gpu_data.contains(lights_gpu_data), "Light gpu data for scenario does not exist!");

        m_light_gpu_data.erase(lights_gpu_data);
        m_scenarios.erase(sc);
    }

    m_models.erase(model_id);
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
        return;
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
        return;
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
        return;
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
        return;
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
        return;
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
        return nd.camera_ids[static_cast<uint8>(camera_type::perspective)];
    }
    if ((nd.type & node_type::orthographic_camera) != node_type::hierarchy)
    {
        return nd.camera_ids[static_cast<uint8>(camera_type::orthographic)];
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
    }
    if ((nd.type & node_type::orthographic_camera) != node_type::hierarchy)
    {
        m_main_camera_node = node_id;
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
    if (parent_node == m_root_node)
    {
        MANGO_LOG_WARN("Can not detach from root node - only removable would be possible!");
        return;
    }

    const node& child = m_nodes.at(child_node);
    node& parent = m_nodes.at(parent_node);

    if ((child.type & node_type::instantiable) != node_type::hierarchy)
    {
        MANGO_LOG_WARN("Child is instantiated! Can not detach!");
        return;
    }
    if ((parent.type & node_type::instantiable) != node_type::hierarchy)
    {
        MANGO_LOG_WARN("Parent is instantiated! Can not detach!");
        return;
    }

    auto found = std::find(parent.children.begin(), parent.children.end(), child_node);
    if (found == parent.children.end())
    {
        MANGO_LOG_WARN("Child is not attached to parent! Can not detach!");
        return;
    }

    parent.children.erase(found);
    node& root = m_nodes.at(m_root_node);
    root.children.push_back(child_node);
}

void scene_impl::remove_texture_gpu_data(uid texture_id)
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

    m_texture_gpu_data.erase(tex.gpu_data);
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

optional<light_gpu_data&> scene_impl::get_light_gpu_data(uid instance_id)
{
    PROFILE_ZONE;

    if (!m_light_gpu_data.contains(instance_id))
    {
        MANGO_LOG_WARN("Light gpu data with ID {0} does not exist! Can not retrieve light gpu data!", instance_id.get());
        return NULL_OPTION;
    }

    return m_light_gpu_data.at(instance_id);
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

    uid active_camera_uid = get_active_camera_uid();

    if(!active_camera_uid.is_valid())
    {
        MANGO_LOG_WARN("Active camera id is not valid! Can not retrieve active camera gpu data!");
        return NULL_OPTION;
    }

    if (!m_camera_gpu_data.contains(active_camera_uid))
    {
        MANGO_LOG_WARN("Camera gpu data with ID {0} does not exist! Can not retrieve camera gpu data!", active_camera_uid.get());
        return NULL_OPTION;
    }

    return m_camera_gpu_data.at(active_camera_uid);
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
        MANGO_LOG_DEBUG("The gltf model has {0} scenarios. At the moment only the default one is loaded!", m.scenes.size());
    }

    // load buffers
    std::vector<uid> buffer_ids(m.buffers.size());
    for (int32 i = 0; i < static_cast<int32>(m.buffers.size()); ++i)
    {
        const tinygltf::Buffer& t_buffer = m.buffers[i];

        uid buffer_object_id = uid::create(m_scene_buffers.emplace(), scene_structure_type::scene_structure_internal_buffer);
        scene_buffer& buf    = m_scene_buffers.back();
        buf.instance_id      = buffer_object_id;
        buf.name             = t_buffer.name;
        buf.data             = t_buffer.data;

        buffer_ids[i] = buffer_object_id;
    }
    // load buffer views
    std::vector<uid> buffer_view_ids(m.bufferViews.size());
    for (int32 i = 0; i < static_cast<int32>(m.bufferViews.size()); ++i)
    {
        const tinygltf::BufferView& buffer_view = m.bufferViews[i];
        if (buffer_view.target == 0)
        {
            MANGO_LOG_DEBUG("Buffer view target is zero!"); // We can continue here.
        }

        const tinygltf::Buffer& t_buffer = m.buffers[buffer_view.buffer];

        uid buffer_view_object_id = uid::create(m_scene_buffer_views.emplace(), scene_structure_type::scene_structure_internal_buffer_view);
        scene_buffer_view& view   = m_scene_buffer_views.back();
        view.instance_id          = buffer_view_object_id;
        view.offset               = 0; // buffer_view.byteOffset; -> Is done on upload.
        view.size                 = static_cast<int32>(buffer_view.byteLength);
        view.stride               = static_cast<int32>(buffer_view.byteStride);
        view.buffer               = buffer_ids[buffer_view.buffer];

        buffer_create_info buffer_info;
        buffer_info.buffer_access = gfx_buffer_access::buffer_access_dynamic_storage;
        buffer_info.buffer_target = (buffer_view.target == 0 || buffer_view.target == GL_ARRAY_BUFFER) ? gfx_buffer_target::buffer_target_vertex : gfx_buffer_target::buffer_target_index;
        buffer_info.size          = buffer_view.byteLength;

        view.graphics_buffer = graphics_device->create_buffer(buffer_info);

        // upload data
        auto device_context = graphics_device->create_graphics_device_context();
        device_context->begin();
        const unsigned char* buffer_start = t_buffer.data.data() + buffer_view.byteOffset;
        const void* buffer_data           = static_cast<const void*>(buffer_start);
        device_context->set_buffer_data(view.graphics_buffer, 0, view.size, const_cast<void*>(buffer_data));
        device_context->end();
        device_context->submit();
        // TODO Paul: Are interleaved buffers loaded multiple times?

        buffer_view_ids[i] = buffer_view_object_id;
    }

    int32 scene_id                 = m.defaultScene > -1 ? m.defaultScene : 0;
    const tinygltf::Scene& t_scene = m.scenes[scene_id];

    scenario scen;
    default_scenario = scene_id;

    uid scenario_id  = uid::create(m_scene_scenarios.emplace(), scene_structure_type::scene_structure_scenario);
    scen.instance_id = scenario_id;

    scene_scenario& sc = m_scene_scenarios.back();
    sc.public_data     = scen;

    /*
     * We store all nodes in the scenario as well. Since we iterate top down here, we can later add it top down,
     * to the scene graph without breaking anything regarding the transformations.
     */
    for (int32 i = 0; i < static_cast<int32>(t_scene.nodes.size()); ++i)
    {
        build_model_node(m, m.nodes.at(t_scene.nodes.at(i)), buffer_view_ids, sc.nodes, invalid_uid, scenario_id);
    }

    std::vector<uid> result;
    result.push_back(scenario_id);

    return result;
}

void scene_impl::build_model_node(tinygltf::Model& m, tinygltf::Node& n, const std::vector<uid>& buffer_view_ids, std::vector<uid>& scenario_nodes, uid parent_node_id, uid scenario_id)
{
    PROFILE_ZONE;

    uid node_id                        = uid::create(m_scene_nodes.emplace(), scene_structure_type::scene_structure_node);
    scene_node& nd                     = m_scene_nodes.back();
    nd.public_data                     = node(n.name);
    nd.public_data.instance_id         = node_id;
    nd.public_data.containing_scenario = scenario_id;
    nd.public_data.parent_node         = parent_node_id;

    // add to scenario
    scenario_nodes.push_back(node_id);

    uid transform_id    = uid::create(m_scene_transforms.emplace(), scene_structure_type::scene_structure_transform);
    scene_transform& tr = m_scene_transforms.back();

    nd.node_transform = transform_id;

    if (n.matrix.size() == 16)
    {
        mat4 input = glm::make_mat4(n.matrix.data());
        vec3 s;
        vec4 p;
        glm::decompose(input, tr.public_data.scale, tr.public_data.rotation, tr.public_data.position, s, p);
    }
    else
    {
        if (n.translation.size() == 3)
        {
            tr.public_data.position = vec3(n.translation[0], n.translation[1], n.translation[2]);
        }
        else
        {
            tr.public_data.position = vec3(0.0f);
        }
        if (n.rotation.size() == 4)
        {
            tr.public_data.rotation = quat(static_cast<float>(n.rotation[3]), static_cast<float>(n.rotation[0]), static_cast<float>(n.rotation[1]), static_cast<float>(n.rotation[2]));
        }
        else
        {
            tr.public_data.rotation = quat(1.0f, 0.0f, 0.0f, 0.0f);
        }
        if (n.scale.size() == 3)
        {
            tr.public_data.scale = vec3(n.scale[0], n.scale[1], n.scale[2]);
        }
        else
        {
            tr.public_data.scale = vec3(1.0f);
        }
    }

    tr.rotation_hint = glm::degrees(glm::eulerAngles(tr.public_data.rotation));

    tr.public_data.update();

    if (n.mesh > -1)
    {
        MANGO_ASSERT(n.mesh < static_cast<int32>(m.meshes.size()), "Invalid gltf mesh!");
        MANGO_LOG_DEBUG("Node contains a mesh!");
        nd.mesh_id = build_model_mesh(m, m.meshes.at(n.mesh), buffer_view_ids, node_id);
        nd.type |= node_type::mesh;
    }

    if (n.camera > -1)
    {
        MANGO_ASSERT(n.camera < static_cast<int32>(m.cameras.size()), "Invalid gltf camera!");
        MANGO_LOG_DEBUG("Node contains a camera!");
        nd.camera_id = build_model_camera(m.cameras.at(n.camera), node_id);
        nd.type |= node_type::camera;
    }

    nd.children = static_cast<int32>(n.children.size());
    if (nd.children)
        nd.type |= node_type::is_parent;

    // build child nodes
    for (int32 i = 0; i < static_cast<int32>(n.children.size()); ++i)
    {
        MANGO_ASSERT(n.children[i] < static_cast<int32>(m.nodes.size()), "Invalid gltf node!");

        build_model_node(m, m.nodes.at(n.children.at(i)), buffer_view_ids, scenario_nodes, node_id, scenario_id);
    }
}

uid scene_impl::build_model_camera(tinygltf::Camera& camera, uid node_id)
{
    PROFILE_ZONE;
    uid camera_id;

    if (camera.type == "perspective")
    {
        camera_id         = uid::create(m_scene_cameras.emplace(), scene_structure_type::scene_structure_perspective_camera);
        scene_camera& cam = m_scene_cameras.back();
        cam.type          = camera_type::perspective;
        perspective_camera cam_data;
        cam_data.z_near                 = static_cast<float>(camera.perspective.znear);
        cam_data.z_far                  = camera.perspective.zfar > 0.0 ? static_cast<float>(camera.perspective.zfar) : 10000.0f; // Infinite?
        cam_data.vertical_field_of_view = static_cast<float>(camera.perspective.yfov);
        cam_data.aspect                 = camera.perspective.aspectRatio > 0.0 ? static_cast<float>(camera.perspective.aspectRatio) : 16.0f / 9.0f;

        cam_data.physical.aperture      = default_camera_aperture;
        cam_data.physical.shutter_speed = default_camera_shutter_speed;
        cam_data.physical.iso           = default_camera_iso;

        cam.public_data_as_perspective                  = cam_data;
        cam.public_data_as_perspective->instance_id     = camera_id;
        cam.public_data_as_perspective->containing_node = node_id;
    }
    else // orthographic
    {
        camera_id         = uid::create(m_scene_cameras.emplace(), scene_structure_type::scene_structure_orthographic_camera);
        scene_camera& cam = m_scene_cameras.back();
        cam.type          = camera_type::orthographic;
        orthographic_camera cam_data;

        cam_data.z_near = static_cast<float>(camera.orthographic.znear);
        cam_data.z_far  = camera.perspective.zfar > 0.0 ? static_cast<float>(camera.perspective.zfar) : 10000.0f; // Infinite?
        cam_data.x_mag  = static_cast<float>(camera.orthographic.xmag);
        cam_data.y_mag  = static_cast<float>(camera.orthographic.ymag);

        cam_data.physical.aperture      = default_camera_aperture;
        cam_data.physical.shutter_speed = default_camera_shutter_speed;
        cam_data.physical.iso           = default_camera_iso;

        cam.public_data_as_orthographic                  = cam_data;
        cam.public_data_as_orthographic->instance_id     = camera_id;
        cam.public_data_as_orthographic->containing_node = node_id;
    }

    return camera_id;
}

uid scene_impl::build_model_mesh(tinygltf::Model& m, tinygltf::Mesh& mesh, const std::vector<uid>& buffer_view_ids, uid node_id)
{
    PROFILE_ZONE;
    uid mesh_id                     = uid::create(m_scene_meshes.emplace(), scene_structure_type::scene_structure_mesh);
    scene_mesh& msh                 = m_scene_meshes.back();
    msh.public_data.instance_id     = mesh_id;
    msh.public_data.containing_node = node_id;
    msh.public_data.name            = mesh.name.empty() ? "Unnamed" : mesh.name;

    for (int32 i = 0; i < static_cast<int32>(mesh.primitives.size()); ++i)
    {
        const tinygltf::Primitive& primitive = mesh.primitives[i];

        uid primitive_id           = uid::create(m_scene_primitives.emplace(), scene_structure_type::scene_structure_primitive);
        scene_primitive& sp        = m_scene_primitives.back();
        sp.public_data.instance_id = primitive_id;
        sp.public_data.type        = primitive_type::custom;

        sp.draw_call_desc.vertex_count   = 0;
        sp.draw_call_desc.instance_count = 1;
        sp.draw_call_desc.base_instance  = 0;
        sp.draw_call_desc.base_vertex    = 0;
        sp.draw_call_desc.index_offset   = 0;

        sp.input_assembly.topology = static_cast<gfx_primitive_topology>(primitive.mode + 1); // cast should be okay

        if (primitive.indices >= 0)
        {
            const tinygltf::Accessor& index_accessor = m.accessors[primitive.indices];

            packed_freelist_id view_id     = buffer_view_ids[index_accessor.bufferView].id(); // TODO Paul: Do we need to check the index?
            sp.index_buffer_view           = m_scene_buffer_views.at(view_id);
            sp.index_type                  = static_cast<gfx_format>(index_accessor.componentType); // cast should be okay
            sp.draw_call_desc.index_count  = static_cast<int32>(index_accessor.count);
            sp.draw_call_desc.index_offset = static_cast<int32>(index_accessor.byteOffset);
        }
        else
        {
            sp.draw_call_desc.index_count = 0; // Has to be set!!!
            sp.index_type                 = gfx_format::invalid;
            // vertex_count has to be set later.
        }

        uid material_id     = uid::create(m_scene_materials.emplace(), scene_structure_type::scene_structure_material);
        scene_material& mat = m_scene_materials.back();

        // Some defaults
        mat.public_data.instance_id = material_id;
        mat.public_data.base_color  = vec4(vec3(0.9f), 1.0f);
        mat.public_data.metallic    = 0.0f;
        mat.public_data.roughness   = 1.0f;

        if (primitive.material >= 0)
            load_material(mat.public_data, m.materials[primitive.material], m);

        sp.public_data.material = material_id;

        int32 vertex_buffer_binding = 0;
        int32 description_index     = 0;

        for (auto& attrib : primitive.attributes)
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
                attrib_location            = 1;
                sp.public_data.has_normals = true;
            }
            else if (attrib.first.compare("TEXCOORD_0") == 0)
                attrib_location = 2;
            else if (attrib.first.compare("TANGENT") == 0)
            {
                attrib_location             = 3;
                sp.public_data.has_tangents = true;
            }

            if (attrib_location > -1)
            {
                packed_freelist_id view_id = buffer_view_ids[accessor.bufferView].id(); // TODO Paul: Do we need to check the index?
                auto buffer_v              = m_scene_buffer_views.at(view_id);
                buffer_v.offset += static_cast<int32>(accessor.byteOffset);
                sp.vertex_buffer_views.emplace_back(buffer_v);

                binding_desc.binding    = vertex_buffer_binding;
                binding_desc.stride     = accessor.ByteStride(m.bufferViews[accessor.bufferView]);
                binding_desc.input_rate = gfx_vertex_input_rate::per_vertex; // TODO Paul: This will probably change later.

                attrib_desc.binding = vertex_buffer_binding;
                attrib_desc.offset  = 0; // TODO Paul: Does that work with interleaved buffers?
                // TODO Paul: Does this work with matrix types?
                attrib_desc.attribute_format =
                    graphics::get_attribute_format_for_component_info(static_cast<gfx_format>(accessor.componentType), get_attrib_component_count_from_tinygltf_types(accessor.type));
                attrib_desc.location = attrib_location;

                if (attrib_location == 0 && sp.index_type == gfx_format::invalid)
                {
                    sp.draw_call_desc.vertex_count = static_cast<int32>(accessor.count);
                }

                vertex_buffer_binding++;
                sp.vertex_layout.binding_descriptions[description_index]   = binding_desc;
                sp.vertex_layout.attribute_descriptions[description_index] = attrib_desc;
                description_index++;

                // AABB
                if (attrib_location == 0)
                {
                    sp.bounding_box = axis_aligned_bounding_box::from_min_max(vec3(accessor.minValues[0], accessor.minValues[1], accessor.minValues[2]),
                                                                              vec3(accessor.maxValues[0], accessor.maxValues[1], accessor.maxValues[2]));
                }
            }
            else
            {
                MANGO_LOG_DEBUG("Vertex attribute is ignored: {0}!", attrib.first);
                continue;
            }
        }
        sp.vertex_layout.binding_description_count   = description_index;
        sp.vertex_layout.attribute_description_count = description_index;
        msh.scene_primitives.push_back(sp);
    }

    return mesh_id;
}

void scene_impl::load_material(material& mat, const tinygltf::Material& primitive_material, tinygltf::Model& m)
{
    PROFILE_ZONE;

    if (!primitive_material.name.empty())
    {
        MANGO_LOG_DEBUG("Loading material: {0}", primitive_material.name.c_str());
    }

    mat.name         = primitive_material.name.empty() ? "Unnamed" : primitive_material.name;
    mat.double_uided = primitive_material.doubleuided;

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
        auto col       = pbr.baseColorFactor;
        mat.base_color = color_rgba((float)col[0], (float)col[1], (float)col[2], (float)col[3]);
    }
    else
    {
        // base color
        const tinygltf::Texture& base_col = m.textures.at(pbr.baseColorTexture.index);

        if (base_col.source < 0)
            return;

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

        uid texture_id  = uid::create(m_scene_textures.emplace(), scene_structure_type::scene_structure_texture);
        tex.instance_id = texture_id;

        image_resource img;
        img.data                                = const_cast<void*>(static_cast<const void*>(image.image.data()));
        img.width                               = image.width;
        img.height                              = image.height;
        img.bits                                = image.bits;
        img.number_components                   = image.component;
        img.description.is_standard_color_space = standard_color_space;
        img.description.is_hdr                  = high_dynamic_range;
        img.description.path                    = "from_gltf";

        auto texture_sampler_pair = create_gfx_texture_and_sampler(img, standard_color_space, high_dynamic_range, sampler_info);

        scene_texture& st   = m_scene_textures.back();
        st.graphics_texture = texture_sampler_pair.first;
        st.graphics_sampler = texture_sampler_pair.second;
        st.public_data      = tex;

        mat.base_color_texture = texture_id;
    }

    // metallic / roughness
    if (pbr.metallicRoughnessTexture.index < 0)
    {
        mat.metallic  = static_cast<float>(pbr.metallicFactor);
        mat.roughness = static_cast<float>(pbr.roughnessFactor);
    }
    else
    {
        const tinygltf::Texture& o_r_m_t = m.textures.at(pbr.metallicRoughnessTexture.index);

        if (o_r_m_t.source < 0)
            return;

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

        uid texture_id  = uid::create(m_scene_textures.emplace(), scene_structure_type::scene_structure_texture);
        tex.instance_id = texture_id;

        image_resource img;
        img.data                                = const_cast<void*>(static_cast<const void*>(image.image.data()));
        img.width                               = image.width;
        img.height                              = image.height;
        img.bits                                = image.bits;
        img.number_components                   = image.component;
        img.description.is_standard_color_space = standard_color_space;
        img.description.is_hdr                  = high_dynamic_range;
        img.description.path                    = "from_gltf";

        auto texture_sampler_pair = create_gfx_texture_and_sampler(img, standard_color_space, high_dynamic_range, sampler_info);

        scene_texture& st   = m_scene_textures.back();
        st.graphics_texture = texture_sampler_pair.first;
        st.graphics_sampler = texture_sampler_pair.second;
        st.public_data      = tex;

        mat.metallic_roughness_texture = texture_id;
    }

    // occlusion
    if (primitive_material.occlusionTexture.index >= 0)
    {
        if (pbr.metallicRoughnessTexture.index == primitive_material.occlusionTexture.index)
        {
            // occlusion packed into r channel of the roughness and metallic texture.
            mat.packed_occlusion = true;
        }
        else
        {
            mat.packed_occlusion = false;

            const tinygltf::Texture& occ = m.textures.at(primitive_material.occlusionTexture.index);
            if (occ.source < 0)
                return;

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

            uid texture_id  = uid::create(m_scene_textures.emplace(), scene_structure_type::scene_structure_texture);
            tex.instance_id = texture_id;

            image_resource img;
            img.data                                = const_cast<void*>(static_cast<const void*>(image.image.data()));
            img.width                               = image.width;
            img.height                              = image.height;
            img.bits                                = image.bits;
            img.number_components                   = image.component;
            img.description.is_standard_color_space = standard_color_space;
            img.description.is_hdr                  = high_dynamic_range;
            img.description.path                    = "from_gltf";

            auto texture_sampler_pair = create_gfx_texture_and_sampler(img, standard_color_space, high_dynamic_range, sampler_info);

            scene_texture& st   = m_scene_textures.back();
            st.graphics_texture = texture_sampler_pair.first;
            st.graphics_sampler = texture_sampler_pair.second;
            st.public_data      = tex;

            mat.occlusion_texture = texture_id;
        }
    }

    // normal
    if (primitive_material.normalTexture.index >= 0)
    {
        const tinygltf::Texture& norm = m.textures.at(primitive_material.normalTexture.index);

        if (norm.source < 0)
            return;

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

        uid texture_id  = uid::create(m_scene_textures.emplace(), scene_structure_type::scene_structure_texture);
        tex.instance_id = texture_id;

        image_resource img;
        img.data                                = const_cast<void*>(static_cast<const void*>(image.image.data()));
        img.width                               = image.width;
        img.height                              = image.height;
        img.bits                                = image.bits;
        img.number_components                   = image.component;
        img.description.is_standard_color_space = standard_color_space;
        img.description.is_hdr                  = high_dynamic_range;
        img.description.path                    = "from_gltf";

        auto texture_sampler_pair = create_gfx_texture_and_sampler(img, standard_color_space, high_dynamic_range, sampler_info);

        scene_texture& st   = m_scene_textures.back();
        st.graphics_texture = texture_sampler_pair.first;
        st.graphics_sampler = texture_sampler_pair.second;
        st.public_data      = tex;

        mat.normal_texture = texture_id;
    }

    // emissive
    if (primitive_material.emissiveTexture.index < 0)
    {
        auto col           = primitive_material.emissiveFactor;
        mat.emissive_color = color_rgb((float)col[0], (float)col[1], (float)col[2]);
    }
    else
    {
        const tinygltf::Texture& emissive = m.textures.at(primitive_material.emissiveTexture.index);

        if (emissive.source < 0)
            return;

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

        uid texture_id  = uid::create(m_scene_textures.emplace(), scene_structure_type::scene_structure_texture);
        tex.instance_id = texture_id;

        image_resource img;
        img.data                                = const_cast<void*>(static_cast<const void*>(image.image.data()));
        img.width                               = image.width;
        img.height                              = image.height;
        img.bits                                = image.bits;
        img.number_components                   = image.component;
        img.description.is_standard_color_space = standard_color_space;
        img.description.is_hdr                  = high_dynamic_range;
        img.description.path                    = "from_gltf";

        auto texture_sampler_pair = create_gfx_texture_and_sampler(img, standard_color_space, high_dynamic_range, sampler_info);

        scene_texture& st   = m_scene_textures.back();
        st.graphics_texture = texture_sampler_pair.first;
        st.graphics_sampler = texture_sampler_pair.second;
        st.public_data      = tex;

        mat.emissive_texture = texture_id;
    }

    // transparency
    if (primitive_material.alphaMode.compare("OPAQUE") == 0)
    {
        mat.alpha_mode   = material_alpha_mode::mode_opaque;
        mat.alpha_cutoff = 1.0f;
    }
    else if (primitive_material.alphaMode.compare("MASK") == 0)
    {
        mat.alpha_mode   = material_alpha_mode::mode_mask;
        mat.alpha_cutoff = static_cast<float>(primitive_material.alphaCutoff);
    }
    else if (primitive_material.alphaMode.compare("BLEND") == 0)
    {
        mat.alpha_mode   = material_alpha_mode::mode_blend;
        mat.alpha_cutoff = 1.0f;
    }
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

void scene_impl::update(float dt)
{
    PROFILE_ZONE;
    MANGO_UNUSED(dt);

    m_render_instances.clear();

    sg_bfs_for_each(
        [this](hierarchy_node& hn)
        {
            packed_freelist_id node_id      = hn.node_id.id();
            scene_node& nd                  = m_scene_nodes.at(node_id);
            packed_freelist_id transform_id = nd.node_transform.id();
            scene_transform& tr             = m_scene_transforms.at(transform_id);

            if (tr.public_data.dirty())
            {
                // recalculate node matrices
                nd.local_transformation_matrix = glm::translate(mat4(1.0), tr.public_data.position);
                nd.local_transformation_matrix = nd.local_transformation_matrix * glm::mat4_cast(tr.public_data.rotation);
                nd.local_transformation_matrix = glm::scale(nd.local_transformation_matrix, tr.public_data.scale);

                nd.global_transformation_matrix = nd.local_transformation_matrix;

                tr.changes_handled();
            }

            if (nd.public_data.parent_node.is_valid())
            {
                packed_freelist_id parent_id    = nd.public_data.parent_node.id();
                auto& parent_transformation     = m_scene_nodes.at(parent_id).global_transformation_matrix;
                nd.global_transformation_matrix = parent_transformation * nd.local_transformation_matrix;
            }

            if ((nd.type & node_type::camera) != node_type::empty_leaf)
            {
                // update camera targets - matrices should be calculated by the renderer on demand
                packed_freelist_id camera_id = nd.camera_id.id();
                scene_camera& cam            = m_scene_cameras.at(camera_id);

                vec3& target = (cam.type == camera_type::perspective) ? cam.public_data_as_perspective->target : cam.public_data_as_orthographic->target;

                vec3 front = target - vec3(nd.global_transformation_matrix[3]);

                if (glm::length(front) > 1e-5)
                    front = glm::normalize(front);
                else
                {
                    front = GLOBAL_FORWARD;
                }
            }

            // add to render instances
            m_render_instances.push_back(scene_render_instance(hn.node_id));

            return true;
        });

    for (const packed_freelist_id& id : m_scene_textures)
    {
        scene_texture& tex = m_scene_textures.at(id);
        if (tex.public_data.dirty())
        {
            // TODO Paul: This does crash when we do this for textures from a model -.-...
            if (tex.public_data.file_path == "from_gltf")
                continue;
            // just replacing the texture should work, but is not the fancy way. Also we reload the file even though it is not required.

            // TODO Paul: We probably want more exposed settings here - at least in public_data!
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

            auto texture_sampler_pair = create_gfx_texture_and_sampler(tex.public_data.file_path, tex.public_data.standard_color_space, tex.public_data.high_dynamic_range, sampler_info);

            tex.graphics_texture = texture_sampler_pair.first;
            tex.graphics_sampler = texture_sampler_pair.second;

            tex.changes_handled();
        }
    }
}

void scene_impl::draw_scene_hierarchy(uid& selected)
{
    std::vector<uid> to_remove = draw_scene_hierarchy_internal(m_scene_graph_root.get(), selected);
    for (auto n : to_remove)
        remove_node(n);
}

std::vector<uid> scene_impl::draw_scene_hierarchy_internal(hierarchy_node* current, uid& selected)
{
    optional<scene_node&> sc_node = get_scene_node(current->node_id);
    MANGO_ASSERT(sc_node, "Something is broken - Can not draw hierarchy for a non existing node!");

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 5));
    const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_FramePadding |
                                     ImGuiTreeNodeFlags_AllowItemOverlap | ((m_ui_selected_uid == current->node_id) ? ImGuiTreeNodeFlags_Selected : 0) |
                                     ((current->children.empty()) ? ImGuiTreeNodeFlags_Leaf : 0);

    ImGui::PushID(current->node_id.id().get());

    string display_name = get_display_name(current->node_id);
    bool open           = ImGui::TreeNodeEx(display_name.c_str(), flags, "%s", display_name.c_str());
    bool removed        = false;
    ImGui::PopStyleVar();
    if (ImGui::IsItemClicked(0))
    {
        m_ui_selected_uid = current->node_id;
    }
    if (ImGui::IsItemClicked(1) && !ImGui::IsPopupOpen(("##object_menu" + std::to_string(current->node_id.id().get())).c_str()))
    {
        m_ui_selected_uid = current->node_id;
        ImGui::OpenPopup(("##object_menu" + std::to_string(current->node_id.id().get())).c_str());
    }

    std::vector<uid> to_remove;
    if (ImGui::BeginPopup(("##object_menu" + std::to_string(current->node_id.id().get())).c_str()))
    {
        if (ImGui::Selectable(("Add Scene Object##object_menu" + std::to_string(current->node_id.id().get())).c_str()))
        {
            node nd;
            nd.parent_node    = current->node_id;
            m_ui_selected_uid = add_node(nd);
        }
        if (!(m_root_node == current->node_id) && ImGui::Selectable(("Remove Scene Object##object_menu" + std::to_string(current->node_id.id().get())).c_str()))
        {
            m_ui_selected_uid = invalid_uid;
            to_remove.push_back(current->node_id);
            removed = true;
        }

        ImGui::EndPopup();
    }
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
    {
        ImGui::SetDragDropPayload("DRAG_DROP_NODE", current, sizeof(hierarchy_node));
        ImGui::EndDragDropSource();
    }
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DRAG_DROP_NODE"))
        {
            IM_ASSERT(payload->DataSize == sizeof(hierarchy_node));
            auto dropped = (const hierarchy_node*)payload->Data;
            attach(dropped->node_id, current->node_id);
        }
        ImGui::EndDragDropTarget();
    }
    if (open)
    {
        if (!removed)
        {
            for (auto it = current->children.begin(); it != current->children.end(); it++)
            {
                auto& child           = *it;
                auto removed_children = draw_scene_hierarchy_internal(child.get(), selected);
                to_remove.insert(to_remove.end(), removed_children.begin(), removed_children.end());
            }
        }
        ImGui::TreePop();
    }

    ImGui::PopID();
    selected = m_ui_selected_uid;
    return to_remove;
}

string scene_impl::get_display_name(uid object) // TODO Paul: Make that better!
{
    switch (object.structure_type())
    {
    case scene_structure_type::scene_structure_node:
    {
        optional<scene_node&> node = get_scene_node(object);
        MANGO_ASSERT(node, "Can not get name of non existing node!");
        if ((node->type & node_type::light) != node_type::empty_leaf)
        {
            return node->public_data.name.empty() ? string(ICON_FA_LIGHTBULB) + "Unnamed Light" : string(ICON_FA_LIGHTBULB) + " " + node->public_data.name;
        }
        if ((node->type & node_type::mesh) != node_type::empty_leaf)
        {
            return node->public_data.name.empty() ? get_display_name(node->mesh_id) : string(ICON_FA_DICE_D6) + " " + node->public_data.name;
        }
        if ((node->type & node_type::camera) != node_type::empty_leaf)
        {
            return node->public_data.name.empty() ? get_display_name(node->camera_id) : string(ICON_FA_VIDEO) + " " + node->public_data.name;
        }
        return node->public_data.name.empty() ? get_display_name(node->node_transform) : string(ICON_FA_VECTOR_SQUARE) + " " + node->public_data.name;
    }
    case scene_structure_type::scene_structure_transform:
    {
        return string(ICON_FA_VECTOR_SQUARE) + " Transform";
    }
    case scene_structure_type::scene_structure_model:
    {
        optional<model&> m = get_model(object);
        MANGO_ASSERT(m, "Can not get name of non existing model!");
        return string(ICON_FA_SITEMAP) + " " + m->file_path;
    }
    case scene_structure_type::scene_structure_mesh:
    {
        optional<scene_mesh&> m = get_scene_mesh(object);
        MANGO_ASSERT(m, "Can not get name of non existing mesh!");
        return string(ICON_FA_DICE_D6) + " " + m->public_data.name;
    }
    case scene_structure_type::scene_structure_primitive:
    {
        optional<scene_primitive&> p = get_scene_primitive(object);
        MANGO_ASSERT(p, "Can not get name of non existing primitive!");
        optional<scene_material&> mat = get_scene_material(p->public_data.material);
        MANGO_ASSERT(mat, "Can not get name of non existing material!");
        return string(ICON_FA_DICE_D6) + " " + mat->public_data.name;
    }
    case scene_structure_type::scene_structure_material:
    {
        optional<scene_material&> mat = get_scene_material(object);
        MANGO_ASSERT(mat, "Can not get name of non existing material!");
        return string(ICON_FA_DICE_D6) + " " + mat->public_data.name;
    }
    case scene_structure_type::scene_structure_directional_light:
    {
        optional<scene_light&> l = get_scene_light(object);
        MANGO_ASSERT(l, "Can not get name of non existing light!");
        return string(ICON_FA_LIGHTBULB) + " " + "Directional Light";
    }
    case scene_structure_type::scene_structure_skylight:
    {
        optional<scene_light&> l = get_scene_light(object);
        MANGO_ASSERT(l, "Can not get name of non existing light!");
        return string(ICON_FA_LIGHTBULB) + " " + "Skylight";
    }
    case scene_structure_type::scene_structure_atmospheric_light:
    {
        optional<scene_light&> l = get_scene_light(object);
        MANGO_ASSERT(l, "Can not get name of non existing light!");
        return string(ICON_FA_LIGHTBULB) + " " + "Atmospheric Light";
    }
    case scene_structure_type::scene_structure_perspective_camera:
    {
        optional<scene_camera&> cam = get_scene_camera(object);
        MANGO_ASSERT(cam, "Can not get name of non existing camera!");
        return string(ICON_FA_VIDEO) + " " + "Perspective Camera";
    }
    case scene_structure_type::scene_structure_orthographic_camera:
    {
        optional<scene_camera&> cam = get_scene_camera(object);
        MANGO_ASSERT(cam, "Can not get name of non existing camera!");
        return string(ICON_FA_VIDEO) + " " + "Orthographic Camera";
    }
    }
    return "";
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
