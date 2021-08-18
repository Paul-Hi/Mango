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
#include <scene/scene_impl.hpp>
#include <ui/dear_imgui/icons_font_awesome_5.hpp>
#include <ui/dear_imgui/imgui_glfw.hpp>

using namespace mango;

static int32 get_attrib_component_count_from_tinygltf_types(int32 type);
static gfx_sampler_filter get_texture_filter_from_tinygltf(int32 filter);
static gfx_sampler_edge_wrap get_texture_wrap_from_tinygltf(int32 wrap);

scene_impl::scene_impl(const string& name, const shared_ptr<context_impl>& context)
    : m_shared_context(context)
    , m_scene_textures()
    , m_scene_materials()
    , m_scene_buffers()
    , m_scene_buffer_views()
    , m_scene_meshes()
    , m_scene_primitives()
    , m_scene_cameras()
    , m_scene_lights()
    , m_scene_transforms()
    , m_scene_nodes()
    , m_scene_scenarios()
    , m_scene_models()
{
    PROFILE_ZONE;
    MANGO_UNUSED(name);

    scene_node root_node;
    m_root_node                = sid::create(m_scene_nodes.emplace(root_node), scene_structure_type::scene_structure_node);
    scene_node& nd             = m_scene_nodes.back();
    nd.public_data             = node();
    nd.public_data.instance_id = m_root_node;
    nd.public_data.name        = "Root";

    sid transform_id                                      = sid::create(m_scene_transforms.emplace(), scene_structure_type::scene_structure_transform);
    m_scene_transforms.back().public_data.instance_id     = transform_id;
    m_scene_transforms.back().public_data.containing_node = m_root_node;

    nd.node_transform = transform_id;

    m_scene_graph_root          = mango::make_unique<hierarchy_node>();
    m_scene_graph_root->node_id = m_root_node;
}

scene_impl::~scene_impl() {}

sid scene_impl::add_node(node& new_node)
{
    PROFILE_ZONE;

    sid node_id                        = sid::create(m_scene_nodes.emplace(), scene_structure_type::scene_structure_node);
    scene_node& nd                     = m_scene_nodes.back();
    nd.public_data                     = new_node;
    nd.public_data.instance_id         = node_id;
    nd.public_data.containing_scenario = invalid_sid; // no scenario

    if (!nd.public_data.parent_node.is_valid())
    {
        attach(nd.public_data.instance_id, m_root_node);
    }
    else
    {
        attach(nd.public_data.instance_id, nd.public_data.parent_node);
    }

    return node_id;
}

sid scene_impl::add_perspective_camera(perspective_camera& new_perspective_camera, sid containing_node_id)
{
    PROFILE_ZONE;
    packed_freelist_id node = containing_node_id.id();

    if (!m_scene_nodes.contains(node))
    {
        MANGO_LOG_WARN("Containing node with ID {0} does not exist! Can not add perspective camera!", containing_node_id.id().get());
        return invalid_sid;
    }

    sid camera_id                                   = sid::create(m_scene_cameras.emplace(), scene_structure_type::scene_structure_perspective_camera);
    scene_camera& cam                               = m_scene_cameras.back();
    cam.public_data_as_perspective                  = new_perspective_camera;
    cam.public_data_as_perspective->instance_id     = camera_id;
    cam.public_data_as_perspective->containing_node = containing_node_id;
    cam.type                                        = camera_type::perspective;

    scene_node& nd = m_scene_nodes.at(node);
    nd.camera_id   = camera_id;
    nd.type |= node_type::camera;

    if (!m_main_camera_node.is_valid())
    {
        m_main_camera_node = containing_node_id;
    }

    return containing_node_id;
}

sid scene_impl::add_orthographic_camera(orthographic_camera& new_orthographic_camera, sid containing_node_id)
{
    PROFILE_ZONE;
    packed_freelist_id node = containing_node_id.id();

    if (!m_scene_nodes.contains(node))
    {
        MANGO_LOG_WARN("Containing node with ID {0} does not exist! Can not add orthographic camera!", containing_node_id.id().get());
        return invalid_sid;
    }

    sid camera_id                                    = sid::create(m_scene_cameras.emplace(), scene_structure_type::scene_structure_orthographic_camera);
    scene_camera& cam                                = m_scene_cameras.back();
    cam.public_data_as_orthographic                  = new_orthographic_camera;
    cam.public_data_as_orthographic->instance_id     = camera_id;
    cam.public_data_as_orthographic->containing_node = containing_node_id;
    cam.type                                         = camera_type::orthographic;

    scene_node& nd = m_scene_nodes.at(node);
    nd.camera_id   = camera_id;
    nd.type |= node_type::camera;

    if (!m_main_camera_node.is_valid())
    {
        m_main_camera_node = containing_node_id;
    }

    return containing_node_id;
}

sid scene_impl::add_directional_light(directional_light& new_directional_light, sid containing_node_id)
{
    PROFILE_ZONE;
    packed_freelist_id node = containing_node_id.id();

    if (!m_scene_nodes.contains(node))
    {
        MANGO_LOG_WARN("Containing node with ID {0} does not exist! Can not add directional light!", containing_node_id.id().get());
        return invalid_sid;
    }

    sid light_id                                        = sid::create(m_scene_lights.emplace(), scene_structure_type::scene_structure_directional_light);
    scene_light& l                                      = m_scene_lights.back();
    l.public_data_as_directional_light                  = new_directional_light;
    l.public_data_as_directional_light->instance_id     = light_id;
    l.public_data_as_directional_light->containing_node = containing_node_id;
    l.type                                              = light_type::directional;

    scene_node& nd                                            = m_scene_nodes.at(node);
    nd.light_ids[static_cast<uint8>(light_type::directional)] = light_id;
    nd.type |= node_type::light;

    return containing_node_id;
}

sid scene_impl::add_skylight(skylight& new_skylight, sid containing_node_id)
{
    PROFILE_ZONE;
    packed_freelist_id node = containing_node_id.id();

    if (!m_scene_nodes.contains(node))
    {
        MANGO_LOG_WARN("Containing node with ID {0} does not exist! Can not add skylight!", containing_node_id.id().get());
        return invalid_sid;
    }

    sid skylight_id                                        = add_skylight_structure(new_skylight, containing_node_id);
    scene_node& nd                                         = m_scene_nodes.at(node);
    nd.light_ids[static_cast<uint8>(light_type::skylight)] = skylight_id;
    nd.type |= node_type::light;

    return containing_node_id;
}

sid scene_impl::add_atmospheric_light(atmospheric_light& new_atmospheric_light, sid containing_node_id)
{
    PROFILE_ZONE;
    packed_freelist_id node = containing_node_id.id();

    if (!m_scene_nodes.contains(node))
    {
        MANGO_LOG_WARN("Containing node with ID {0} does not exist! Can not add atmospheric light!", containing_node_id.id().get());
        return invalid_sid;
    }

    sid light_id                                        = sid::create(m_scene_lights.emplace(), scene_structure_type::scene_structure_atmospheric_light);
    scene_light& l                                      = m_scene_lights.back();
    l.public_data_as_atmospheric_light                  = new_atmospheric_light;
    l.public_data_as_atmospheric_light->instance_id     = light_id;
    l.public_data_as_atmospheric_light->containing_node = containing_node_id;
    l.type                                              = light_type::atmospheric;

    scene_node& nd                                            = m_scene_nodes.at(node);
    nd.light_ids[static_cast<uint8>(light_type::atmospheric)] = light_id;
    nd.type |= node_type::light;

    return containing_node_id;
}

sid scene_impl::build_material(material& new_material)
{
    PROFILE_ZONE;

    sid material_id     = sid::create(m_scene_materials.emplace(), scene_structure_type::scene_structure_material);
    scene_material& mat = m_scene_materials.back();
    mat.public_data     = new_material;

    return material_id;
}

sid scene_impl::load_texture_from_image(const string& path, bool standard_color_space, bool high_dynamic_range)
{
    PROFILE_ZONE;

    texture tex;
    tex.file_path            = path;
    tex.standard_color_space = standard_color_space;
    tex.high_dynamic_range   = high_dynamic_range;

    sid texture_id  = sid::create(m_scene_textures.emplace(), scene_structure_type::scene_structure_texture);
    tex.instance_id = texture_id;

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

    auto texture_sampler_pair = create_gfx_texture_and_sampler(path, standard_color_space, high_dynamic_range, sampler_info);

    scene_texture& st   = m_scene_textures.back();
    st.graphics_texture = texture_sampler_pair.first;
    st.graphics_sampler = texture_sampler_pair.second;
    st.public_data      = tex;

    return texture_id;
}

sid scene_impl::load_model_from_gltf(const string& path)
{
    PROFILE_ZONE;

    model mod;
    mod.file_path = path;

    sid model_id    = sid::create(m_scene_models.emplace(), scene_structure_type::scene_structure_model);
    mod.instance_id = model_id;

    mod.scenarios = load_model_from_file(path, mod.default_scenario);

    scene_model& md = m_scene_models.back();
    md.public_data  = mod;

    return model_id;
}

sid scene_impl::add_skylight_from_hdr(const string& path, sid containing_node_id)
{
    PROFILE_ZONE;
    packed_freelist_id node = containing_node_id.id();

    if (!m_scene_nodes.contains(node))
    {
        MANGO_LOG_WARN("Containing node with ID {0} does not exist! Can not add skylight!", containing_node_id.id().get());
        return invalid_sid;
    }

    // texture
    sid texture_id = load_texture_from_image(path, false, true);

    // skylight
    skylight sl;
    sl.hdr_texture  = texture_id;
    sl.use_texture  = true;
    sid skylight_id = add_skylight_structure(sl, containing_node_id);

    scene_node& nd                                         = m_scene_nodes.at(node);
    nd.light_ids[static_cast<uint8>(light_type::skylight)] = skylight_id;
    nd.type |= node_type::light;

    return containing_node_id;
}

sid scene_impl::add_model_to_scene(sid model_to_add, sid scenario_id, sid containing_node_id)
{
    PROFILE_ZONE;
    if (model_to_add == invalid_sid || scenario_id == invalid_sid)
    {
        if (!m_scene_nodes.contains(containing_node_id.id()))
        {
            MANGO_LOG_WARN("Containing node with ID {0} does not exist! Can not add model!", containing_node_id.id().get());
            return invalid_sid;
        }
        return invalid_sid;
    }

    packed_freelist_id model_id = model_to_add.id();
    packed_freelist_id scenario = scenario_id.id();

    if (!m_scene_models.contains(model_id))
    {
        MANGO_LOG_WARN("Model with ID {0} does not exist! Can not add model to scene!", model_to_add.id().get());
        return invalid_sid;
    }
    if (!m_scene_scenarios.contains(scenario)) // TODO Scenarios.
    {
        MANGO_LOG_WARN("Scenario with ID {0} does not exist! Can not add model to scene!", scenario_id.id().get());
        return invalid_sid;
    }

    scene_model& m       = m_scene_models.at(model_id);
    scene_scenario& scen = m_scene_scenarios.at(scenario);

    packed_freelist_id node = containing_node_id.id();

    if (!m_scene_nodes.contains(node))
    {
        MANGO_LOG_WARN("Node with ID {0} does not exist! Can not add model to scene!", model_to_add.id().get());
        return invalid_sid;
    }

    scene_node& nd_containing = m_scene_nodes.at(node);
    nd_containing.type |= node_type::is_parent;

    auto found = std::find(m.public_data.scenarios.begin(), m.public_data.scenarios.end(), scenario_id);
    if (found == m.public_data.scenarios.end())
    {
        MANGO_LOG_WARN("Model to add does not contain scenario to add.");
        return invalid_sid;
    }

    hierarchy_node* containing_entry;
    if (sg_bfs_node(containing_node_id, containing_entry))
    {
        std::unordered_map<sid, hierarchy_node*, sid_hash> previous_added; // Should make the insertion faster, since, we don't need to bfs each time
        for (sid node_id : scen.nodes)
        {
            packed_freelist_id node_pf = node_id.id();
            MANGO_ASSERT(m_scene_nodes.contains(node_pf), "Something went wrong while loading the model! Can not add model to scene!");

            scene_node& nd = m_scene_nodes.at(node_pf);

            unique_ptr<hierarchy_node> current = mango::make_unique<hierarchy_node>();
            current->node_id                   = node_id;

            previous_added.insert({ node_id, current.get() });
            if (nd.public_data.parent_node.is_valid())
            {
                auto parent_hierarchy = previous_added.find(nd.public_data.parent_node);
                MANGO_ASSERT(parent_hierarchy != previous_added.end(), "Something went wrong while loading the model scenario! Can not add model to scene!");
                parent_hierarchy->second->children.emplace_back(std::move(current));
            }
            else // first
            {
                containing_entry->children.emplace_back(std::move(current));
                nd.public_data.parent_node = containing_node_id; // TODO Paul: This has to be reset or something like that?
                nd_containing.children++;
            }
        }

        return containing_node_id;
    }
    else
    {
        MANGO_LOG_WARN("Node with ID {0} is not actively in the scene! Can not add model to scene!", containing_node_id.id().get());
        return invalid_sid;
    }
}

void scene_impl::remove_node(sid node_id)
{
    PROFILE_ZONE;
    packed_freelist_id node = node_id.id();

    if (!m_scene_nodes.contains(node))
    {
        MANGO_LOG_WARN("Node with ID {0} does not exist! Can not remove node!", node_id.id().get());
        return;
    }

    scene_node& to_remove = m_scene_nodes.at(node);

    if (to_remove.type != node_type::empty_leaf)
    {
        MANGO_LOG_WARN("Node with ID {0} has children or holds another object! Will not remove node!", node_id.id().get());
        return;
    }

    if (to_remove.public_data.parent_node.is_valid())
    {
        detach(node_id);
        // afterwards th subtree is attached to the root
        // so first detach from root
        scene_node& root = m_scene_nodes.at(m_root_node.id());
        root.children--;
        if (root.children <= 0)
        {
            root.type &= ~node_type::is_parent;
            root.children = 0;
        }
    }

    m_scene_nodes.erase(node);

    // scene graph
    m_scene_graph_root->children.pop_back(); // just destroy it :D
}

void scene_impl::remove_perspective_camera(sid node_id)
{
    PROFILE_ZONE;
    packed_freelist_id node_pf = node_id.id();

    if (!m_scene_nodes.contains(node_pf))
    {
        MANGO_LOG_WARN("Node with ID {0} does not exist! Can not remove perspective camera!", node_id.id().get());
        return;
    }

    scene_node& node = m_scene_nodes.at(node_pf);

    if ((node.type & node_type::camera) == node_type::empty_leaf)
    {
        MANGO_LOG_WARN("Node with ID {0} does not contain a camera! Can not remove perspective camera!", node_id.id().get());
        return;
    }

    packed_freelist_id cam = node.camera_id.id();

    if (!m_scene_cameras.contains(cam))
    {
        MANGO_LOG_WARN("Camera with ID {0} does not exist! Can not remove perspective camera!", cam.get());
        return;
    }

    scene_camera& to_remove = m_scene_cameras.at(cam);

    sid containing_node = to_remove.public_data_as_perspective->containing_node;

    if (containing_node.is_valid())
    {
        packed_freelist_id cn = containing_node.id();
        MANGO_ASSERT(m_scene_nodes.contains(cn), "Containing node does not exist!"); // TODO Is this assertion right?
        scene_node& nd = m_scene_nodes.at(cn);
        nd.camera_id   = invalid_sid;
        nd.type &= ~node_type::camera;
    }

    m_scene_cameras.erase(cam);
}

void scene_impl::remove_orthographic_camera(sid node_id)
{
    PROFILE_ZONE;
    packed_freelist_id node_pf = node_id.id();

    if (!m_scene_nodes.contains(node_pf))
    {
        MANGO_LOG_WARN("Node with ID {0} does not exist! Can not remove orthographic camera!", node_id.id().get());
        return;
    }

    scene_node& node = m_scene_nodes.at(node_pf);

    if ((node.type & node_type::camera) == node_type::empty_leaf)
    {
        MANGO_LOG_WARN("Node with ID {0} does not contain a camera! Can not remove orthographic camera!", node_id.id().get());
        return;
    }

    packed_freelist_id cam = node.camera_id.id();

    if (!m_scene_cameras.contains(cam))
    {
        MANGO_LOG_WARN("Camera with ID {0} does not exist! Can not remove orthographic camera!", cam.get());
        return;
    }

    scene_camera& to_remove = m_scene_cameras.at(cam);

    sid containing_node = to_remove.public_data_as_orthographic->containing_node;

    if (containing_node.is_valid())
    {
        packed_freelist_id cn = containing_node.id();
        MANGO_ASSERT(m_scene_nodes.contains(cn), "Containing node does not exist!"); // TODO Is this assertion right?
        scene_node& nd = m_scene_nodes.at(cn);
        nd.camera_id   = invalid_sid;
        nd.type &= ~node_type::camera;
    }

    m_scene_cameras.erase(cam);
}

void scene_impl::remove_mesh(sid node_id)
{
    PROFILE_ZONE;
    packed_freelist_id node_pf = node_id.id();

    if (!m_scene_nodes.contains(node_pf))
    {
        MANGO_LOG_WARN("Node with ID {0} does not exist! Can not remove mesh!", node_id.id().get());
        return;
    }

    scene_node& node = m_scene_nodes.at(node_pf);

    if ((node.type & node_type::mesh) == node_type::empty_leaf)
    {
        MANGO_LOG_WARN("Node with ID {0} does not contain a mesh! Can not remove mesh!", node_id.id().get());
        return;
    }

    packed_freelist_id m = node.mesh_id.id();

    if (!m_scene_meshes.contains(m))
    {
        MANGO_LOG_WARN("Mesh with ID {0} does not exist! Can not remove mesh!", m.get());
        return;
    }

    scene_mesh& to_remove = m_scene_meshes.at(m);

    sid containing_node = to_remove.public_data.containing_node;

    if (containing_node.is_valid())
    {
        packed_freelist_id cn = containing_node.id();
        MANGO_ASSERT(m_scene_nodes.contains(cn), "Containing node does not exist!"); // TODO Is this assertion right?
        scene_node& nd                                            = m_scene_nodes.at(cn);
        nd.light_ids[static_cast<uint8>(light_type::directional)] = invalid_sid;
        nd.type &= ~node_type::mesh;
    }

    m_scene_meshes.erase(m);
}

void scene_impl::remove_directional_light(sid node_id)
{
    PROFILE_ZONE;
    packed_freelist_id node_pf = node_id.id();

    if (!m_scene_nodes.contains(node_pf))
    {
        MANGO_LOG_WARN("Node with ID {0} does not exist! Can not remove directional light!", node_id.id().get());
        return;
    }

    scene_node& node = m_scene_nodes.at(node_pf);

    if ((node.type & node_type::light) == node_type::empty_leaf)
    {
        MANGO_LOG_WARN("Node with ID {0} does not contain a light! Can not remove directional light!", node_id.id().get());
        return;
    }

    packed_freelist_id light = node.light_ids[static_cast<uint8>(light_type::directional)].id();

    if (!m_scene_lights.contains(light))
    {
        MANGO_LOG_WARN("Light with ID {0} does not exist! Can not remove directional light!", light.get());
        return;
    }

    scene_light& to_remove = m_scene_lights.at(light);

    sid containing_node = to_remove.public_data_as_directional_light->containing_node;

    if (containing_node.is_valid())
    {
        packed_freelist_id cn = containing_node.id();
        MANGO_ASSERT(m_scene_nodes.contains(cn), "Containing node does not exist!"); // TODO Is this assertion right?
        scene_node& nd                                            = m_scene_nodes.at(cn);
        nd.light_ids[static_cast<uint8>(light_type::directional)] = invalid_sid;
        nd.type &= ~node_type::light;
    }

    m_scene_lights.erase(light);
}

void scene_impl::remove_skylight(sid node_id)
{
    PROFILE_ZONE;
    packed_freelist_id node_pf = node_id.id();

    if (!m_scene_nodes.contains(node_pf))
    {
        MANGO_LOG_WARN("Node with ID {0} does not exist! Can not remove skylight!", node_id.id().get());
        return;
    }

    scene_node& node = m_scene_nodes.at(node_pf);

    if ((node.type & node_type::light) == node_type::empty_leaf)
    {
        MANGO_LOG_WARN("Node with ID {0} does not contain a light! Can not remove skylight!", node_id.id().get());
        return;
    }

    packed_freelist_id light = node.light_ids[static_cast<uint8>(light_type::skylight)].id();

    if (!m_scene_lights.contains(light))
    {
        MANGO_LOG_WARN("Light with ID {0} does not exist! Can not remove skylight!", light.get());
        return;
    }

    scene_light& to_remove = m_scene_lights.at(light);

    sid containing_node = to_remove.public_data_as_skylight->containing_node;

    if (containing_node.is_valid())
    {
        packed_freelist_id cn = containing_node.id();
        MANGO_ASSERT(m_scene_nodes.contains(cn), "Containing node does not exist!"); // TODO Is this assertion right?
        scene_node& nd                                         = m_scene_nodes.at(cn);
        nd.light_ids[static_cast<uint8>(light_type::skylight)] = invalid_sid;
        nd.type &= ~node_type::light;
    }

    m_scene_lights.erase(light);
}

void scene_impl::remove_atmospheric_light(sid node_id)
{
    PROFILE_ZONE;
    packed_freelist_id node_pf = node_id.id();

    if (!m_scene_nodes.contains(node_pf))
    {
        MANGO_LOG_WARN("Node with ID {0} does not exist! Can not remove atmospheric light!", node_id.id().get());
        return;
    }

    scene_node& node = m_scene_nodes.at(node_pf);

    if ((node.type & node_type::light) == node_type::empty_leaf)
    {
        MANGO_LOG_WARN("Node with ID {0} does not contain a light! Can not remove atmospheric light!", node_id.id().get());
        return;
    }

    packed_freelist_id light = node.light_ids[static_cast<uint8>(light_type::atmospheric)].id();

    if (!m_scene_lights.contains(light))
    {
        MANGO_LOG_WARN("Light with ID {0} does not exist! Can not remove atmospheric light!", light.get());
        return;
    }

    scene_light& to_remove = m_scene_lights.at(light);

    sid containing_node = to_remove.public_data_as_atmospheric_light->containing_node;

    if (containing_node.is_valid())
    {
        packed_freelist_id cn = containing_node.id();
        MANGO_ASSERT(m_scene_nodes.contains(cn), "Containing node does not exist!"); // TODO Is this assertion right?
        scene_node& nd                                            = m_scene_nodes.at(cn);
        nd.light_ids[static_cast<uint8>(light_type::atmospheric)] = invalid_sid;
        nd.type &= ~node_type::light;
    }

    m_scene_lights.erase(light);
}

optional<node&> scene_impl::get_node(sid node_id)
{
    PROFILE_ZONE;
    packed_freelist_id node = node_id.id();

    if (!m_scene_nodes.contains(node))
    {
        MANGO_LOG_WARN("Node with ID {0} does not exist!", node_id.id().get());
        return NULL_OPTION;
    }

    return m_scene_nodes.at(node).public_data;
}

optional<transform&> scene_impl::get_transform(sid node_id)
{
    PROFILE_ZONE;
    packed_freelist_id node_pf = node_id.id();

    if (!m_scene_nodes.contains(node_pf))
    {
        MANGO_LOG_WARN("Node with ID {0} does not exist! Can not retrieve transform!", node_id.id().get());
        return NULL_OPTION;
    }

    scene_node& node = m_scene_nodes.at(node_pf);

    packed_freelist_id tr = node.node_transform.id();

    if (!m_scene_transforms.contains(tr))
    {
        MANGO_LOG_WARN("Transform with ID {0} does not exist! Can not retrieve transform!", tr.get());
        return NULL_OPTION;
    }

    return m_scene_transforms.at(tr).public_data;
}

optional<perspective_camera&> scene_impl::get_perspective_camera(sid node_id)
{
    PROFILE_ZONE;
    packed_freelist_id node_pf = node_id.id();

    if (!m_scene_nodes.contains(node_pf))
    {
        MANGO_LOG_WARN("Node with ID {0} does not exist! Can not retrieve perspective camera!", node_id.id().get());
        return NULL_OPTION;
    }

    scene_node& node = m_scene_nodes.at(node_pf);

    if ((node.type & node_type::camera) == node_type::empty_leaf)
    {
        MANGO_LOG_WARN("Node with ID {0} does not contain a camera! Can not retrieve perspective camera!", node_id.id().get());
        return NULL_OPTION;
    }

    packed_freelist_id cam = node.camera_id.id();

    if (!m_scene_cameras.contains(cam))
    {
        MANGO_LOG_WARN("Camera with ID {0} does not exist! Can not retrieve perspective camera!", cam.get());
        return NULL_OPTION;
    }
    if (m_scene_cameras.at(cam).type != camera_type::perspective)
    {
        MANGO_LOG_WARN("Camera with ID {0} is not a perspective camera! Can not retrieve perspective camera!", node_id.id().get());
        return NULL_OPTION;
    }

    return m_scene_cameras.at(cam).public_data_as_perspective.value();
}

optional<orthographic_camera&> scene_impl::get_orthographic_camera(sid node_id)
{
    PROFILE_ZONE;
    packed_freelist_id node_pf = node_id.id();

    if (!m_scene_nodes.contains(node_pf))
    {
        MANGO_LOG_WARN("Node with ID {0} does not exist! Can not retrieve orthographic camera!", node_id.id().get());
        return NULL_OPTION;
    }

    scene_node& node = m_scene_nodes.at(node_pf);

    if ((node.type & node_type::camera) == node_type::empty_leaf)
    {
        MANGO_LOG_WARN("Node with ID {0} does not contain a camera! Can not retrieve orthographic camera!", node_id.id().get());
        return NULL_OPTION;
    }

    packed_freelist_id cam = node.camera_id.id();

    if (!m_scene_cameras.contains(cam))
    {
        MANGO_LOG_WARN("Camera with ID {0} does not exist! Can not retrieve orthographic camera!", cam.get());
        return NULL_OPTION;
    }
    if (m_scene_cameras.at(cam).type != camera_type::orthographic)
    {
        MANGO_LOG_WARN("Camera with ID {0} is not a orthographic camera! Can not retrieve orthographic camera!", node_id.id().get());
        return NULL_OPTION;
    }

    return m_scene_cameras.at(cam).public_data_as_orthographic.value();
}

optional<directional_light&> scene_impl::get_directional_light(sid node_id)
{
    PROFILE_ZONE;
    packed_freelist_id node_pf = node_id.id();

    if (!m_scene_nodes.contains(node_pf))
    {
        MANGO_LOG_WARN("Node with ID {0} does not exist! Can not retrieve directional light!", node_id.id().get());
        return NULL_OPTION;
    }

    scene_node& node = m_scene_nodes.at(node_pf);

    if ((node.type & node_type::light) == node_type::empty_leaf)
    {
        MANGO_LOG_WARN("Node with ID {0} does not contain a light! Can not retrieve directional light!", node_id.id().get());
        return NULL_OPTION;
    }

    packed_freelist_id light = node.light_ids[static_cast<uint8>(light_type::directional)].id();

    if (!m_scene_lights.contains(light))
    {
        MANGO_LOG_WARN("Light with ID {0} does not exist! Can not retrieve directional light!", node_id.id().get());
        return NULL_OPTION;
    }
    if (m_scene_lights.at(light).type != light_type::directional)
    {
        MANGO_LOG_WARN("Light with ID {0} is not a directional light! Can not retrieve directional light!", node_id.id().get());
        return NULL_OPTION;
    }

    return m_scene_lights.at(light).public_data_as_directional_light.value();
}

optional<skylight&> scene_impl::get_skylight(sid node_id)
{
    PROFILE_ZONE;
    packed_freelist_id node_pf = node_id.id();

    if (!m_scene_nodes.contains(node_pf))
    {
        MANGO_LOG_WARN("Node with ID {0} does not exist! Can not retrieve skylight!", node_id.id().get());
        return NULL_OPTION;
    }

    scene_node& node = m_scene_nodes.at(node_pf);

    if ((node.type & node_type::light) == node_type::empty_leaf)
    {
        MANGO_LOG_WARN("Node with ID {0} does not contain a light! Can not retrieve skylight!", node_id.id().get());
        return NULL_OPTION;
    }

    packed_freelist_id light = node.light_ids[static_cast<uint8>(light_type::skylight)].id();

    if (!m_scene_lights.contains(light))
    {
        MANGO_LOG_WARN("Light with ID {0} does not exist! Can not retrieve skylight!", node_id.id().get());
        return NULL_OPTION;
    }
    if (m_scene_lights.at(light).type != light_type::skylight)
    {
        MANGO_LOG_WARN("Light with ID {0} is not a skylight! Can not retrieve skylight!", node_id.id().get());
        return NULL_OPTION;
    }

    return m_scene_lights.at(light).public_data_as_skylight.value();
}

optional<atmospheric_light&> scene_impl::get_atmospheric_light(sid node_id)
{
    PROFILE_ZONE;
    packed_freelist_id node_pf = node_id.id();

    if (!m_scene_nodes.contains(node_pf))
    {
        MANGO_LOG_WARN("Node with ID {0} does not exist! Can not retrieve atmospheric light!", node_id.id().get());
        return NULL_OPTION;
    }

    scene_node& node = m_scene_nodes.at(node_pf);

    if ((node.type & node_type::light) == node_type::empty_leaf)
    {
        MANGO_LOG_WARN("Node with ID {0} does not contain a light! Can not retrieve atmospheric light!", node_id.id().get());
        return NULL_OPTION;
    }

    packed_freelist_id light = node.light_ids[static_cast<uint8>(light_type::atmospheric)].id();

    if (!m_scene_lights.contains(light))
    {
        MANGO_LOG_WARN("Light with ID {0} does not exist! Can not retrieve atmospheric light!", node_id.id().get());
        return NULL_OPTION;
    }
    if (m_scene_lights.at(light).type != light_type::atmospheric)
    {
        MANGO_LOG_WARN("Light with ID {0} is not a atmospheric light! Can not retrieve atmospheric light!", node_id.id().get());
        return NULL_OPTION;
    }

    return m_scene_lights.at(light).public_data_as_atmospheric_light.value();
}

optional<model&> scene_impl::get_model(sid instance_id)
{
    PROFILE_ZONE;
    packed_freelist_id m = instance_id.id();

    if (!m_scene_models.contains(m))
    {
        MANGO_LOG_WARN("Model with ID {0} does not exist!", instance_id.id().get());
        return NULL_OPTION;
    }

    return m_scene_models.at(m).public_data;
}

optional<mesh&> scene_impl::get_mesh(sid instance_id)
{
    PROFILE_ZONE;
    packed_freelist_id m = instance_id.id();

    if (!m_scene_meshes.contains(m))
    {
        MANGO_LOG_WARN("Mesh with ID {0} does not exist!", instance_id.id().get());
        return NULL_OPTION;
    }

    return m_scene_meshes.at(m).public_data;
}

optional<material&> scene_impl::get_material(sid instance_id)
{
    PROFILE_ZONE;
    packed_freelist_id mat = instance_id.id();

    if (!m_scene_materials.contains(mat))
    {
        MANGO_LOG_WARN("Material with ID {0} does not exist!", instance_id.id().get());
        return NULL_OPTION;
    }

    return m_scene_materials.at(mat).public_data;
}

optional<texture&> scene_impl::get_texture(sid instance_id)
{
    PROFILE_ZONE;
    packed_freelist_id tex = instance_id.id();

    if (!m_scene_textures.contains(tex))
    {
        MANGO_LOG_WARN("Texture with ID {0} does not exist!", instance_id.id().get());
        return NULL_OPTION;
    }

    return m_scene_textures.at(tex).public_data;
}

optional<scene_node&> scene_impl::get_scene_node(sid instance_id)
{
    PROFILE_ZONE;
    packed_freelist_id node = instance_id.id();

    if (!m_scene_nodes.contains(node))
    {
        MANGO_LOG_WARN("Node with ID {0} does not exist!", instance_id.id().get());
        return NULL_OPTION;
    }

    return m_scene_nodes.at(node);
}

optional<scene_transform&> scene_impl::get_scene_transform(sid instance_id)
{
    PROFILE_ZONE;
    packed_freelist_id tr = instance_id.id();

    if (!m_scene_transforms.contains(tr))
    {
        MANGO_LOG_WARN("Tranform with ID {0} does not exist!", instance_id.id().get());
        return NULL_OPTION;
    }

    return m_scene_transforms.at(tr);
}

optional<scene_camera&> scene_impl::get_scene_camera(sid instance_id)
{
    PROFILE_ZONE;
    packed_freelist_id cam = instance_id.id();

    if (!m_scene_cameras.contains(cam))
    {
        MANGO_LOG_WARN("Camera with ID {0} does not exist!", instance_id.id().get());
        return NULL_OPTION;
    }

    return m_scene_cameras.at(cam);
}

optional<scene_light&> scene_impl::get_scene_light(sid instance_id)
{
    PROFILE_ZONE;
    packed_freelist_id l = instance_id.id();

    if (!m_scene_lights.contains(l))
    {
        MANGO_LOG_WARN("Light with ID {0} does not exist!", instance_id.id().get());
        return NULL_OPTION;
    }

    return m_scene_lights.at(l);
}

optional<scene_material&> scene_impl::get_scene_material(sid instance_id)
{
    PROFILE_ZONE;
    packed_freelist_id mat = instance_id.id();

    if (!m_scene_materials.contains(mat))
    {
        MANGO_LOG_WARN("Material with ID {0} does not exist!", instance_id.id().get());
        return NULL_OPTION;
    }

    return m_scene_materials.at(mat);
}

optional<scene_mesh&> scene_impl::get_scene_mesh(sid instance_id)
{
    PROFILE_ZONE;
    packed_freelist_id m = instance_id.id();

    if (!m_scene_meshes.contains(m))
    {
        MANGO_LOG_WARN("Mesh with ID {0} does not exist!", instance_id.id().get());
        return NULL_OPTION;
    }

    return m_scene_meshes.at(m);
}

optional<scene_model&> scene_impl::get_scene_model(sid instance_id)
{
    PROFILE_ZONE;
    packed_freelist_id m = instance_id.id();

    if (!m_scene_models.contains(m))
    {
        MANGO_LOG_WARN("Model with ID {0} does not exist!", instance_id.id().get());
        return NULL_OPTION;
    }

    return m_scene_models.at(m);
}

optional<scene_primitive&> scene_impl::get_scene_primitive(sid instance_id)
{
    PROFILE_ZONE;
    packed_freelist_id prim = instance_id.id();

    if (!m_scene_primitives.contains(prim))
    {
        MANGO_LOG_WARN("Primitive with ID {0} does not exist!", instance_id.id().get());
        return NULL_OPTION;
    }

    return m_scene_primitives.at(prim);
}

optional<scene_texture&> scene_impl::get_scene_texture(sid instance_id)
{
    PROFILE_ZONE;
    packed_freelist_id tex = instance_id.id();

    if (!m_scene_textures.contains(tex))
    {
        MANGO_LOG_WARN("Texture with ID {0} does not exist!", instance_id.id().get());
        return NULL_OPTION;
    }

    return m_scene_textures.at(tex);
}

optional<scene_camera&> scene_impl::get_active_scene_camera(vec3& position)
{
    PROFILE_ZONE;
    if (!m_main_camera_node.is_valid())
        return NULL_OPTION;

    packed_freelist_id node_pf = m_main_camera_node.id();

    if (!m_scene_nodes.contains(node_pf))
    {
        MANGO_LOG_WARN("Node with ID {0} does not exist! Can not get active camera data!", m_main_camera_node.id().get());
        return NULL_OPTION;
    }

    scene_node& node = m_scene_nodes.at(node_pf);

    if ((node.type & node_type::camera) == node_type::empty_leaf)
    {
        MANGO_LOG_WARN("Node with ID {0} does not contain a camera! Can not get active camera data!", m_main_camera_node.id().get());
        return NULL_OPTION;
    }

    packed_freelist_id cam = node.camera_id.id();

    if (!m_scene_cameras.contains(cam))
    {
        MANGO_LOG_WARN("Camera with ID {0} does not exist! Can not get active camera data!", cam.get());
        return NULL_OPTION;
    }

    packed_freelist_id tr = node.node_transform.id();

    if (!m_scene_transforms.contains(tr))
    {
        MANGO_LOG_WARN("Transform with ID {0} does not exist! Can not get active camera data!", tr.get());
        return NULL_OPTION;
    }

    position = m_scene_transforms.at(tr).public_data.position;

    return m_scene_cameras.at(cam);
}

sid scene_impl::get_active_camera_sid()
{
    packed_freelist_id node_pf = m_main_camera_node.id();
    if (!m_scene_nodes.contains(node_pf))
    {
        MANGO_LOG_WARN("Node with ID {0} does not exist! Can not get active camera id!", m_main_camera_node.id().get());
        return invalid_sid;
    }

    scene_node& node = m_scene_nodes.at(node_pf);
    return node.camera_id;
}

sid scene_impl::get_active_scene_camera_node_sid()
{
    return m_main_camera_node;
}

sid scene_impl::get_root_node()
{
    return m_root_node; // TODO Paul!
}

void scene_impl::set_main_camera(sid node_id)
{
    if (node_id == invalid_sid)
    {
        MANGO_ASSERT(false, "Can set active camera to invalid at the moment.");
    }

    packed_freelist_id node_pf = node_id.id();

    if (!m_scene_nodes.contains(node_pf))
    {
        MANGO_LOG_WARN("Node with ID {0} does not exist! Can not set as active camera!", node_id.id().get());
        return;
    }

    scene_node& node = m_scene_nodes.at(node_pf);

    if ((node.type & node_type::camera) == node_type::empty_leaf)
    {
        MANGO_LOG_WARN("Node with ID {0} does not contain a camera! Can not set as active camera!", node_id.id().get());
        return;
    }

    packed_freelist_id cam = node.camera_id.id();

    if (!m_scene_cameras.contains(cam))
    {
        MANGO_LOG_WARN("Camera with ID {0} does not exist! Can not set as active camera!", cam.get());
        return;
    }

    m_main_camera_node = node_id;
}

void scene_impl::attach(sid child_node, sid parent_node)
{
    PROFILE_ZONE;
    packed_freelist_id child  = child_node.id();
    packed_freelist_id parent = parent_node.id();

    if (!m_scene_nodes.contains(child))
    {
        MANGO_LOG_WARN("Child node with ID {0} does not exist! Can not attach to parent!", child_node.id().get());
        return;
    }

    if (!m_scene_nodes.contains(parent))
    {
        MANGO_LOG_WARN("Parent node with ID {0} does not exist! Can not attach to child!", parent_node.id().get());
        return;
    }

    scene_node& c = m_scene_nodes.at(child);

    if (!m_scene_transforms.contains(child))
    {
        sid transform_id                                      = sid::create(m_scene_transforms.emplace(), scene_structure_type::scene_structure_transform);
        m_scene_transforms.back().public_data.instance_id     = transform_id;
        m_scene_transforms.back().public_data.containing_node = child_node;
        c.node_transform                                      = transform_id;
    }

    scene_node& p = m_scene_nodes.at(parent);

    if (!m_scene_transforms.contains(parent))
    {
        sid transform_id                                      = sid::create(m_scene_transforms.emplace(), scene_structure_type::scene_structure_transform);
        m_scene_transforms.back().public_data.instance_id     = transform_id;
        m_scene_transforms.back().public_data.containing_node = parent_node;
        p.node_transform                                      = transform_id;
    }

    unique_ptr<hierarchy_node> child_h = nullptr;
    if (c.public_data.parent_node.is_valid())
    {
        child_h = std::move(detach_and_get(child_node));
    }

    c.public_data.parent_node = parent_node;

    p.children++;
    p.type |= node_type::is_parent;

    // scene graph

    if (!child_h)
    {
        child_h          = mango::make_unique<hierarchy_node>();
        child_h->node_id = child_node;
    }

    hierarchy_node* parent_h;
    bool res = sg_bfs_node(parent_node, parent_h);
    MANGO_ASSERT(res, "Scene Graph is corrupted!");
    parent_h->children.emplace_back(std::move(child_h));
}

void scene_impl::detach(sid child_node)
{
    PROFILE_ZONE;
    packed_freelist_id child = child_node.id();

    if (!m_scene_nodes.contains(child))
    {
        MANGO_LOG_WARN("Child node with ID {0} does not exist! Can not detach from parent!", child_node.id().get());
        return;
    }

    scene_node& c = m_scene_nodes.at(child);

    if (c.public_data.parent_node == m_root_node)
    {
        MANGO_LOG_DEBUG("Can not detach from root!"); // But we still have to move it to the end -.- ...
        for (auto it = m_scene_graph_root->children.begin(); it != m_scene_graph_root->children.end();)
        {
            if (it->get()->node_id == child_node)
            {
                std::swap(*it, m_scene_graph_root->children.back());
                return;
            }
            else
                it++;
        }
    }

    packed_freelist_id parent = c.public_data.parent_node.id();

    if (!m_scene_nodes.contains(parent))
    {
        MANGO_LOG_WARN("Child node with ID {0} has no parent! Can not detach from parent!", child_node.id().get());
        return;
    }

    scene_node& p = m_scene_nodes.at(parent);

    p.children--;
    if (p.children <= 0)
    {
        p.type &= ~node_type::is_parent;
        p.children = 0;
    }

    // scene graph

    hierarchy_node child_h;
    child_h.node_id = child_node;

    hierarchy_node* parent_h;
    bool res = sg_bfs_node(c.public_data.parent_node, parent_h);
    MANGO_ASSERT(res, "Scene Graph is corrupted!");
    for (auto it = parent_h->children.begin(); it != parent_h->children.end();)
    {
        if (it->get()->node_id == child_node)
        {
            // attach to root so we can not loose any child relations
            scene_node& root = m_scene_nodes.at(m_root_node.id());
            root.children++;
            root.type |= node_type::is_parent;
            m_scene_graph_root->children.emplace_back(std::move(*it));
            parent_h->children.erase(it);
            break;
        }
        else
            it++;
    }
    p.children = static_cast<int32>(parent_h->children.size());

    c.public_data.parent_node = sid();
}

unique_ptr<scene_impl::hierarchy_node> scene_impl::detach_and_get(sid child_node)
{
    PROFILE_ZONE;
    packed_freelist_id child = child_node.id();

    if (!m_scene_nodes.contains(child))
    {
        MANGO_LOG_WARN("Child node with ID {0} does not exist! Can not detach from parent!", child_node.id().get());
        return nullptr;
    }

    scene_node& c = m_scene_nodes.at(child);

    packed_freelist_id parent = c.public_data.parent_node.id();

    if (!m_scene_nodes.contains(parent))
    {
        MANGO_LOG_WARN("Child node with ID {0} has no parent! Can not detach from parent!", child_node.id().get());
        return nullptr;
    }

    scene_node& p = m_scene_nodes.at(parent);

    p.children--;
    if (p.children <= 0)
    {
        p.type &= ~node_type::is_parent;
        p.children = 0;
    }

    // scene graph

    hierarchy_node child_h;
    child_h.node_id = child_node;

    hierarchy_node* parent_h;
    std::unique_ptr<hierarchy_node> detached = nullptr;
    bool res                                 = sg_bfs_node(c.public_data.parent_node, parent_h);
    MANGO_ASSERT(res, "Scene Graph is corrupted!");
    for (auto it = parent_h->children.begin(); it != parent_h->children.end();)
    {
        if (it->get()->node_id == child_node)
        {
            detached = std::move(*it);
            parent_h->children.erase(it);
            break;
        }
        else
            it++;
    }
    p.children = static_cast<int32>(parent_h->children.size());

    c.public_data.parent_node = sid();

    return detached;
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

    // TODO Paul: We probably want more exposed settings here - at least in public_data!
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

    // TODO Paul: We probably want more exposed settings here - at least in public_data!
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

std::vector<sid> scene_impl::load_model_from_file(const string& path, int32& default_scenario)
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
        return std::vector<sid>();
    }
    else
    {
        MANGO_LOG_DEBUG("The gltf model has {0} scenarios. At the moment only the default one is loaded!", m.scenes.size());
    }

    // load buffers
    std::vector<sid> buffer_ids(m.buffers.size());
    for (int32 i = 0; i < static_cast<int32>(m.buffers.size()); ++i)
    {
        const tinygltf::Buffer& t_buffer = m.buffers[i];

        sid buffer_object_id = sid::create(m_scene_buffers.emplace(), scene_structure_type::scene_structure_internal_buffer);
        scene_buffer& buf    = m_scene_buffers.back();
        buf.instance_id      = buffer_object_id;
        buf.name             = t_buffer.name;
        buf.data             = t_buffer.data;

        buffer_ids[i] = buffer_object_id;
    }
    // load buffer views
    std::vector<sid> buffer_view_ids(m.bufferViews.size());
    for (int32 i = 0; i < static_cast<int32>(m.bufferViews.size()); ++i)
    {
        const tinygltf::BufferView& buffer_view = m.bufferViews[i];
        if (buffer_view.target == 0)
        {
            MANGO_LOG_DEBUG("Buffer view target is zero!"); // We can continue here.
        }

        const tinygltf::Buffer& t_buffer = m.buffers[buffer_view.buffer];

        sid buffer_view_object_id = sid::create(m_scene_buffer_views.emplace(), scene_structure_type::scene_structure_internal_buffer_view);
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

    sid scenario_id  = sid::create(m_scene_scenarios.emplace(), scene_structure_type::scene_structure_scenario);
    scen.instance_id = scenario_id;

    scene_scenario& sc = m_scene_scenarios.back();
    sc.public_data     = scen;

    /*
     * We store all nodes in the scenario as well. Since we iterate top down here, we can later add it top down,
     * to the scene graph without breaking anything regarding the transformations.
     */
    for (int32 i = 0; i < static_cast<int32>(t_scene.nodes.size()); ++i)
    {
        build_model_node(m, m.nodes.at(t_scene.nodes.at(i)), buffer_view_ids, sc.nodes, invalid_sid, scenario_id);
    }

    std::vector<sid> result;
    result.push_back(scenario_id);

    return result;
}

void scene_impl::build_model_node(tinygltf::Model& m, tinygltf::Node& n, const std::vector<sid>& buffer_view_ids, std::vector<sid>& scenario_nodes, sid parent_node_id, sid scenario_id)
{
    PROFILE_ZONE;

    sid node_id                        = sid::create(m_scene_nodes.emplace(), scene_structure_type::scene_structure_node);
    scene_node& nd                     = m_scene_nodes.back();
    nd.public_data                     = node(n.name);
    nd.public_data.instance_id         = node_id;
    nd.public_data.containing_scenario = scenario_id;
    nd.public_data.parent_node         = parent_node_id;

    // add to scenario
    scenario_nodes.push_back(node_id);

    sid transform_id    = sid::create(m_scene_transforms.emplace(), scene_structure_type::scene_structure_transform);
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

sid scene_impl::build_model_camera(tinygltf::Camera& camera, sid containing_node_id)
{
    PROFILE_ZONE;
    sid camera_id;

    if (camera.type == "perspective")
    {
        camera_id         = sid::create(m_scene_cameras.emplace(), scene_structure_type::scene_structure_perspective_camera);
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
        cam.public_data_as_perspective->containing_node = containing_node_id;
    }
    else // orthographic
    {
        camera_id         = sid::create(m_scene_cameras.emplace(), scene_structure_type::scene_structure_orthographic_camera);
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
        cam.public_data_as_orthographic->containing_node = containing_node_id;
    }

    return camera_id;
}

sid scene_impl::build_model_mesh(tinygltf::Model& m, tinygltf::Mesh& mesh, const std::vector<sid>& buffer_view_ids, sid containing_node_id)
{
    PROFILE_ZONE;
    sid mesh_id                     = sid::create(m_scene_meshes.emplace(), scene_structure_type::scene_structure_mesh);
    scene_mesh& msh                 = m_scene_meshes.back();
    msh.public_data.instance_id     = mesh_id;
    msh.public_data.containing_node = containing_node_id;
    msh.public_data.name            = mesh.name.empty() ? "Unnamed" : mesh.name;

    for (int32 i = 0; i < static_cast<int32>(mesh.primitives.size()); ++i)
    {
        const tinygltf::Primitive& primitive = mesh.primitives[i];

        sid primitive_id           = sid::create(m_scene_primitives.emplace(), scene_structure_type::scene_structure_primitive);
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

        sid material_id     = sid::create(m_scene_materials.emplace(), scene_structure_type::scene_structure_material);
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
                return sid();
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
    mat.double_sided = primitive_material.doubleSided;

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

        sid texture_id  = sid::create(m_scene_textures.emplace(), scene_structure_type::scene_structure_texture);
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

        sid texture_id  = sid::create(m_scene_textures.emplace(), scene_structure_type::scene_structure_texture);
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

            sid texture_id  = sid::create(m_scene_textures.emplace(), scene_structure_type::scene_structure_texture);
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

        sid texture_id  = sid::create(m_scene_textures.emplace(), scene_structure_type::scene_structure_texture);
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

        sid texture_id  = sid::create(m_scene_textures.emplace(), scene_structure_type::scene_structure_texture);
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

sid scene_impl::add_skylight_structure(const skylight& new_skylight, sid containing_node_id)
{
    PROFILE_ZONE;

    sid light_id                               = sid::create(m_scene_lights.emplace(), scene_structure_type::scene_structure_skylight);
    scene_light& l                             = m_scene_lights.back();
    l.public_data_as_skylight                  = new_skylight;
    l.public_data_as_skylight->instance_id     = light_id;
    l.public_data_as_skylight->containing_node = containing_node_id;
    l.type                                     = light_type::skylight;

    scene_node& nd                                         = m_scene_nodes.at(containing_node_id.id());
    nd.light_ids[static_cast<uint8>(light_type::skylight)] = light_id;
    nd.type |= node_type::light;

    return light_id;
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

void scene_impl::draw_scene_hierarchy(sid& selected)
{
    std::vector<sid> to_remove = draw_scene_hierarchy_internal(m_scene_graph_root.get(), selected);
    for (auto n : to_remove)
        remove_node(n);
}

std::vector<sid> scene_impl::draw_scene_hierarchy_internal(hierarchy_node* current, sid& selected)
{
    optional<scene_node&> sc_node = get_scene_node(current->node_id);
    MANGO_ASSERT(sc_node, "Something is broken - Can not draw hierarchy for a non existing node!");

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 5));
    const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_FramePadding |
                                     ImGuiTreeNodeFlags_AllowItemOverlap | ((m_ui_selected_sid == current->node_id) ? ImGuiTreeNodeFlags_Selected : 0) |
                                     ((current->children.empty()) ? ImGuiTreeNodeFlags_Leaf : 0);

    ImGui::PushID(current->node_id.id().get());

    string display_name = get_display_name(current->node_id);
    bool open           = ImGui::TreeNodeEx(display_name.c_str(), flags, "%s", display_name.c_str());
    bool removed        = false;
    ImGui::PopStyleVar();
    if (ImGui::IsItemClicked(0))
    {
        m_ui_selected_sid = current->node_id;
    }
    if (ImGui::IsItemClicked(1) && !ImGui::IsPopupOpen(("##entity_menu" + std::to_string(current->node_id.id().get())).c_str()))
    {
        m_ui_selected_sid = current->node_id;
        ImGui::OpenPopup(("##entity_menu" + std::to_string(current->node_id.id().get())).c_str());
    }

    std::vector<sid> to_remove;
    if (ImGui::BeginPopup(("##entity_menu" + std::to_string(current->node_id.id().get())).c_str()))
    {
        if (ImGui::Selectable(("Add Entity##entity_menu" + std::to_string(current->node_id.id().get())).c_str()))
        {
            node nd;
            nd.parent_node    = current->node_id;
            m_ui_selected_sid = add_node(nd);
        }
        if (!(m_root_node == current->node_id) && ImGui::Selectable(("Remove Entity##entity_menu" + std::to_string(current->node_id.id().get())).c_str()))
        {
            m_ui_selected_sid = invalid_sid;
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
    selected = m_ui_selected_sid;
    return to_remove;
}

string scene_impl::get_display_name(sid object) // TODO Paul: Make that better!
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
