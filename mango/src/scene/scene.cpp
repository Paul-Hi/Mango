//! \file      scene.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <core/context_impl.hpp>
#include <glad/glad.h>
//! \cond NO_COND
#define GLM_FORCE_SILENT_WARNINGS 1
//! \endcond
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <graphics/buffer.hpp>
#include <graphics/texture.hpp>
#include <graphics/vertex_array.hpp>
#include <mango/profile.hpp>
#include <mango/scene.hpp>
#include <mango/scene_ecs.hpp>
#include <rendering/render_system_impl.hpp>
#include <resources/resource_system.hpp>
#include <scene/ecs_internal.hpp>

using namespace mango;

//! \brief The internal \a ecsystem for transformation updates.
transformation_update_system transformation_update;
//! \brief The internal \a ecsystem for scene graph updates.
scene_graph_update_system scene_graph_update;
//! \brief The internal \a ecsystem for camera updates.
camera_update_system camera_update;

static void update_scene_boundaries(glm::mat4& trafo, tinygltf::Model& m, tinygltf::Mesh& mesh, glm::vec3& min, glm::vec3& max);

static void render_meshes(shared_ptr<render_system_impl> rs, scene_component_pool<mesh_component>& meshes, scene_component_pool<transform_component>& transformations);

scene::scene(const string& name)
    : m_nodes()
    , m_transformations()
    , m_meshes()
    , m_cameras()
{
    PROFILE_ZONE;
    MANGO_UNUSED(name);
    m_active_camera        = invalid_entity;
    m_scene_boundaries.max = glm::vec3(-3.402823e+38f);
    m_scene_boundaries.min = glm::vec3(3.402823e+38f);

    for (uint32 i = 1; i <= max_entities; ++i)
        m_free_entities.push(i);
}

scene::~scene() {}

entity scene::create_empty()
{
    PROFILE_ZONE;
    MANGO_ASSERT(!m_free_entities.empty(), "Reached maximum number of entities!");
    entity new_entity = m_free_entities.front();
    m_free_entities.pop();
    MANGO_LOG_DEBUG("Created entity {0}, {1} left", new_entity, m_free_entities.size());
    return new_entity;
}

void scene::remove_entity(entity e)
{
    PROFILE_ZONE;
    if (e == invalid_entity)
        return;
    detach(e);
    m_transformations.remove_component_from(e);
    m_meshes.remove_component_from(e);
    m_cameras.remove_component_from(e);
    m_environments.remove_component_from(e);
    m_free_entities.push(e);
    MANGO_LOG_DEBUG("Removed entity {0}, {1} left", e, m_free_entities.size());
}

entity scene::create_default_camera()
{
    PROFILE_ZONE;
    entity camera_entity      = create_empty();
    auto& camera_component    = m_cameras.create_component_for(camera_entity);
    auto& transform_component = m_transformations.create_component_for(camera_entity);

    // default parameters
    camera_component.cam_type               = camera_type::perspective_camera;
    camera_component.aspect                 = 16.0f / 9.0f;
    camera_component.z_near                 = 0.015f;
    camera_component.z_far                  = 15.0f;
    camera_component.vertical_field_of_view = glm::radians(45.0f);
    camera_component.up                     = glm::vec3(0.0f, 1.0f, 0.0f);
    camera_component.target                 = glm::vec3(0.0f, 0.0f, 0.0f);

    glm::vec3 position = glm::vec3(0.0f, 0.0f, 1.5f);

    transform_component.position = position;

    camera_component.view            = glm::lookAt(position, camera_component.target, camera_component.up);
    camera_component.projection      = glm::perspective(camera_component.vertical_field_of_view, camera_component.aspect, camera_component.z_near, camera_component.z_far);
    camera_component.view_projection = camera_component.projection * camera_component.view;

    // Currently the only camera is the active one.
    m_active_camera = camera_entity;

    return camera_entity;
}

std::vector<entity> scene::create_entities_from_model(const string& path)
{
    PROFILE_ZONE;
    std::vector<entity> scene_entities;
    entity scene_root = create_empty();
    auto& transform   = m_transformations.create_component_for(scene_root);
    scene_entities.push_back(scene_root);
    shared_ptr<resource_system> rs = m_shared_context->get_resource_system_internal().lock();
    MANGO_ASSERT(rs, "Resource System is invalid!");
    auto start                     = path.find_last_of("\\/") + 1;
    auto name                      = path.substr(start, path.find_last_of(".") - start);
    model_configuration config     = { name };
    const shared_ptr<model> loaded = rs->get_gltf_model(path, config);
    tinygltf::Model& m             = loaded->gltf_model;

    // load the default scene or the first one.
    glm::vec3 max_backup   = m_scene_boundaries.max;
    glm::vec3 min_backup   = m_scene_boundaries.min;
    m_scene_boundaries.max = glm::vec3(-3.402823e+38f);
    m_scene_boundaries.min = glm::vec3(3.402823e+38f);
    MANGO_ASSERT(m.scenes.size() > 0, "No scenes in the gltf model found!");

    // load all model buffer views into buffers.
    std::map<int, buffer_ptr> index_to_buffer_data;

    for (int32 i = 0; i < static_cast<int32>(m.bufferViews.size()); ++i)
    {
        const tinygltf::BufferView& buffer_view = m.bufferViews[i];
        if (buffer_view.target == 0)
        {
            MANGO_LOG_WARN("Buffer view target is zero!"); // We can continue here.
        }

        const tinygltf::Buffer& t_buffer = m.buffers[buffer_view.buffer];

        buffer_configuration buffer_config;
        buffer_config.m_access = buffer_access::NONE;
        buffer_config.m_size   = buffer_view.byteLength;
        buffer_config.m_target = (buffer_view.target == 0 || buffer_view.target == GL_ARRAY_BUFFER) ? buffer_target::VERTEX_BUFFER : buffer_target::INDEX_BUFFER;

        const unsigned char* buffer_start = t_buffer.data.data() + buffer_view.byteOffset;
        const void* buffer_data           = static_cast<const void*>(buffer_start);
        buffer_config.m_data              = buffer_data;
        buffer_ptr buf                    = buffer::create(buffer_config);

        index_to_buffer_data.insert({ i, buf });
    }

    int scene_id                 = m.defaultScene > -1 ? m.defaultScene : 0;
    const tinygltf::Scene& scene = m.scenes[scene_id];
    for (int32 i = 0; i < static_cast<int32>(scene.nodes.size()); ++i)
    {
        entity node = build_model_node(scene_entities, m, m.nodes.at(scene.nodes.at(i)), glm::mat4(1.0), index_to_buffer_data);

        attach(node, scene_root);
    }

    // normalize scale
    const glm::vec3 scale = glm::vec3(1.0f / (glm::compMax(m_scene_boundaries.max) - glm::compMin(m_scene_boundaries.min)));
    transform.scale       = scale;

    if (m_active_camera == invalid_entity)
    {
        // We have at least one default camera in each scene and at the moment the first camera is the active one everytime.
        create_default_camera();
    }

    m_cameras.get_component_for_entity(m_active_camera)->target = (m_scene_boundaries.max + m_scene_boundaries.min) * 0.5f * scale;

    m_scene_boundaries.max =
        glm::max(m_scene_boundaries.max, max_backup); // TODO Paul: This is just in case all other assets are still here, we need to do the calculation with all still existing entities.
    m_scene_boundaries.min =
        glm::min(m_scene_boundaries.min, min_backup); // TODO Paul: This is just in case all other assets are still here, we need to do the calculation with all still existing entities.

    return scene_entities;
}

entity scene::create_environment_from_hdr(const string& path, float rendered_mip_level)
{
    PROFILE_ZONE;
    entity environment_entity = create_empty();
    auto& environment         = m_environments.create_component_for(environment_entity);

    // default rotation and scale
    environment.rotation_scale_matrix = glm::mat3(1.0f);

    // load image and texture
    shared_ptr<resource_system> res = m_shared_context->get_resource_system_internal().lock();
    MANGO_ASSERT(res, "Resource System is expired!");

    image_configuration img_config;
    img_config.name                    = path.substr(path.find_last_of("/") + 1, path.find_last_of("."));
    img_config.is_standard_color_space = false;
    img_config.is_hdr                  = true;

    auto hdr_image = res->get_image(path, img_config);

    texture_configuration tex_config;
    tex_config.m_generate_mipmaps        = 1;
    tex_config.m_is_standard_color_space = false;
    tex_config.m_texture_min_filter      = texture_parameter::FILTER_LINEAR;
    tex_config.m_texture_mag_filter      = texture_parameter::FILTER_LINEAR;
    tex_config.m_texture_wrap_s          = texture_parameter::WRAP_CLAMP_TO_EDGE;
    tex_config.m_texture_wrap_t          = texture_parameter::WRAP_CLAMP_TO_EDGE;

    texture_ptr hdr_texture = texture::create(tex_config);

    format f        = format::RGBA;
    format internal = format::RGBA32F;
    format type     = format::FLOAT;

    hdr_texture->set_data(internal, hdr_image->width, hdr_image->height, f, type, hdr_image->data);

    environment.hdr_texture = hdr_texture;

    shared_ptr<render_system_impl> rs = m_shared_context->get_render_system_internal().lock();
    MANGO_ASSERT(rs, "Render System is expired!");
    rs->set_environment_texture(environment.hdr_texture, rendered_mip_level); // TODO Paul: Transformation?

    return environment_entity;
}

void scene::update(float dt)
{
    PROFILE_ZONE;
    MANGO_UNUSED(dt);
    transformation_update.update(dt, m_transformations);
    scene_graph_update.update(dt, m_nodes, m_transformations);
    camera_update.update(dt, m_cameras, m_transformations);
}

void scene::render()
{
    PROFILE_ZONE;
    shared_ptr<render_system_impl> rs = m_shared_context->get_render_system_internal().lock();
    MANGO_ASSERT(rs, "Render System is expired!");

    render_meshes(rs, m_meshes, m_transformations);
}

void scene::attach(entity child, entity parent)
{
    PROFILE_ZONE;
    if (m_nodes.contains(child))
    {
        detach(child);
    }

    m_nodes.create_component_for(child).parent_entity = parent;

    // reorder subtrees if necessary
    if (m_nodes.size() > 1)
    {
        NAMED_PROFILE_ZONE("Reordering on entity attachment");
        m_nodes.for_each(
            [this](node_component, int32& index) {
                entity possible_parent = m_nodes.entity_at(index);
                for (int32 j = 0; j < index; ++j)
                {
                    const node_component& possible_child = m_nodes.component_at(j);

                    if (possible_child.parent_entity == possible_parent)
                    {
                        m_nodes.move(index, j);
                        ++index;
                        break;
                    }
                }
            },
            true);
    }

    transform_component* parent_transform = m_transformations.get_component_for_entity(parent);
    if (nullptr == parent_transform)
    {
        // create transform component for parent if non-existent
        parent_transform = &m_transformations.create_component_for(parent);
    }

    transform_component* child_transform = m_transformations.get_component_for_entity(child);
    if (nullptr == child_transform)
    {
        // create transform component for child if non-existent
        child_transform = &m_transformations.create_component_for(child);
    }
}

void scene::detach(entity child)
{
    PROFILE_ZONE;
    node_component* parent_component = m_nodes.get_component_for_entity(child);

    if (nullptr == parent_component)
    {
        MANGO_LOG_DEBUG("Entity has no parent!");
        return;
    }

    transform_component* child_transform = m_transformations.get_component_for_entity(child);

    if (nullptr != child_transform)
    {
        // Add transformation from parent before removing the node hierarchy
        child_transform->local_transformation_matrix = child_transform->world_transformation_matrix;
    }

    // We want to remove it without crashing the order. In that way, we don't need to sort it again.
    m_nodes.sort_remove_component_from(child);
}

entity scene::build_model_node(std::vector<entity>& entities, tinygltf::Model& m, tinygltf::Node& n, const glm::mat4& parent_world, const std::map<int, buffer_ptr>& buffer_map)
{
    PROFILE_ZONE;
    entity node     = create_empty();
    auto& transform = m_transformations.create_component_for(node);
    if (n.matrix.size() == 16)
    {
        glm::mat4 input = glm::make_mat4(n.matrix.data());
        glm::quat orient;
        glm::vec3 s;
        glm::vec4 p;
        glm::decompose(input, transform.scale, orient, transform.position, s, p); // TODO Paul: Use quaternions.
        transform.rotation = glm::vec4(glm::angle(orient), glm::axis(orient));
    }
    else
    {
        if (n.translation.size() == 3)
        {
            transform.position = glm::vec3(n.translation[0], n.translation[1], n.translation[2]);
        }
        if (n.rotation.size() == 4)
        {
            glm::quat orient =
                glm::quat(static_cast<float>(n.rotation[3]), static_cast<float>(n.rotation[0]), static_cast<float>(n.rotation[1]), static_cast<float>(n.rotation[2])); // TODO Paul: Use quaternions.
            transform.rotation = glm::vec4(glm::angle(orient), glm::axis(orient));
        }
        if (n.scale.size() == 3)
        {
            transform.scale = glm::vec3(n.scale[0], n.scale[1], n.scale[2]);
        }
    }

    glm::mat4 trafo = glm::translate(glm::mat4(1.0), transform.position);
    trafo           = glm::rotate(trafo, transform.rotation.x, glm::vec3(transform.rotation.y, transform.rotation.z, transform.rotation.w));
    trafo           = glm::scale(trafo, transform.scale);

    trafo = parent_world * trafo;

    if (n.mesh > -1)
    {
        MANGO_ASSERT(n.mesh < static_cast<int32>(m.meshes.size()), "Invalid gltf mesh!");
        build_model_mesh(node, m, m.meshes.at(n.mesh), buffer_map);
        update_scene_boundaries(trafo, m, m.meshes.at(n.mesh), m_scene_boundaries.min, m_scene_boundaries.max);
    }

    entities.push_back(node);

    // build child nodes.
    for (int32 i = 0; i < static_cast<int32>(n.children.size()); ++i)
    {
        MANGO_ASSERT(n.children[i] < static_cast<int32>(m.nodes.size()), "Invalid gltf node!");

        entity child = build_model_node(entities, m, m.nodes.at(n.children.at(i)), trafo, buffer_map);
        attach(child, node);
    }

    return node;
}

void scene::build_model_mesh(entity node, tinygltf::Model& m, tinygltf::Mesh& mesh, const std::map<int, buffer_ptr>& buffer_map)
{
    PROFILE_ZONE;
    auto& component_mesh = m_meshes.create_component_for(node);

    for (int32 i = 0; i < static_cast<int32>(mesh.primitives.size()); ++i)
    {
        const tinygltf::Primitive& primitive = mesh.primitives[i];

        primitive_component p;
        p.vertex_array_object = vertex_array::create();
        p.topology            = static_cast<primitive_topology>(primitive.mode); // cast is okay.
        p.instance_count      = 1;
        bool has_indices      = true;

        if (primitive.indices >= 0)
        {
            const tinygltf::Accessor& index_accessor = m.accessors[primitive.indices];

            p.first      = static_cast<int32>(index_accessor.byteOffset); // TODO Paul: Is int32 big enough?
            p.count      = static_cast<int32>(index_accessor.count);      // TODO Paul: Is int32 big enough?
            p.type_index = static_cast<index_type>(index_accessor.componentType);

            auto it = buffer_map.find(index_accessor.bufferView);
            if (it == buffer_map.end())
            {
                MANGO_LOG_ERROR("No buffer data for index bufferView {0}!", index_accessor.bufferView);
                continue;
            }
            p.vertex_array_object->bind_index_buffer(it->second);
        }
        else
        {
            p.first      = 0;
            p.type_index = index_type::NONE;
            // p.count has to be set later.
            has_indices = false;
        }

        material_component mat;
        mat.component_material             = std::make_shared<material>();
        mat.component_material->base_color = glm::vec4(glm::vec3(0.9f), 1.0f);
        mat.component_material->metallic   = 0.0f;
        mat.component_material->roughness  = 1.0f;

        load_material(mat, primitive, m);

        component_mesh.materials.push_back(mat);

        int32 vb_idx                = 0;
        component_mesh.has_normals  = false;
        component_mesh.has_tangents = false;

        for (auto& attrib : primitive.attributes)
        {
            const tinygltf::Accessor& accessor = m.accessors[attrib.second];
            if (accessor.sparse.isSparse)
            {
                MANGO_LOG_ERROR("Models with sparse accessors are currently not supported! Undefined behavior!");
                return;
            }

            format attribute_format = get_attribute_format(static_cast<format>(accessor.componentType), accessor.type % 32); // TODO Paul: Type is not handled properly here.

            int attrib_array = -1;
            if (attrib.first.compare("POSITION") == 0)
                attrib_array = 0;
            if (attrib.first.compare("NORMAL") == 0)
            {
                component_mesh.has_normals = true;
                attrib_array               = 1;
            }
            if (attrib.first.compare("TEXCOORD_0") == 0)
                attrib_array = 2;
            if (attrib.first.compare("TANGENT") == 0)
            {
                component_mesh.has_tangents = true;
                attrib_array                = 3;
            }
            if (attrib_array > -1)
            {
                auto it = buffer_map.find(accessor.bufferView);
                if (it == buffer_map.end())
                {
                    MANGO_LOG_ERROR("No buffer data for bufferView {0}!", accessor.bufferView);
                    continue;
                }
                int32 stride = accessor.ByteStride(m.bufferViews[accessor.bufferView]);
                MANGO_ASSERT(stride > 0, "Broken gltf model! Attribute stride is {0}!", stride);
                p.vertex_array_object->bind_vertex_buffer(vb_idx, it->second, accessor.byteOffset, stride);
                p.vertex_array_object->set_vertex_attribute(attrib_array, vb_idx, attribute_format, 0);

                if (attrib_array == 0 && !has_indices)
                {
                    p.count = static_cast<int32>(accessor.count); // TODO Paul: Is int32 big enough?
                }

                vb_idx++;
            }
            else
            {
                MANGO_LOG_DEBUG("Vertex attribute array is ignored: {0}!", attrib.first);
            }
        }

        component_mesh.primitives.push_back(p);
    }
}

void scene::load_material(material_component& material, const tinygltf::Primitive& primitive, tinygltf::Model& m)
{
    PROFILE_ZONE;
    if (primitive.material < 0)
        return;

    const tinygltf::Material& p_m = m.materials[primitive.material];
    if (!p_m.name.empty())
    {
        MANGO_LOG_DEBUG("Loading material: {0}", p_m.name.c_str());
    }

    material.component_material->double_sided = p_m.doubleSided;

    auto& pbr = p_m.pbrMetallicRoughness;

    // TODO Paul: Better structure?!

    texture_configuration config;
    config.m_generate_mipmaps        = 1;
    config.m_is_standard_color_space = true;
    config.m_texture_min_filter      = texture_parameter::FILTER_LINEAR_MIPMAP_LINEAR;
    config.m_texture_mag_filter      = texture_parameter::FILTER_LINEAR;
    config.m_texture_wrap_s          = texture_parameter::WRAP_REPEAT;
    config.m_texture_wrap_t          = texture_parameter::WRAP_REPEAT;

    if (pbr.baseColorTexture.index < 0)
    {
        auto col                                = pbr.baseColorFactor;
        material.component_material->base_color = glm::vec4((float)col[0], (float)col[1], (float)col[2], (float)col[3]);
    }
    else
    {
        // base color
        const tinygltf::Texture& base_col = m.textures.at(pbr.baseColorTexture.index);

        if (base_col.source < 0)
            return;

        const tinygltf::Image& image     = m.images[static_cast<g_enum>(base_col.source)];
        const tinygltf::Sampler& sampler = m.samplers[static_cast<g_enum>(base_col.sampler)];

        if (base_col.sampler >= 0)
        {
            config.m_texture_min_filter = filter_parameter_from_gl(static_cast<g_enum>(sampler.minFilter));
            config.m_texture_mag_filter = filter_parameter_from_gl(static_cast<g_enum>(sampler.magFilter));
            config.m_texture_wrap_s     = wrap_parameter_from_gl(static_cast<g_enum>(sampler.wrapS));
            config.m_texture_wrap_t     = wrap_parameter_from_gl(static_cast<g_enum>(sampler.wrapT));
        }

        config.m_is_standard_color_space = true;
        config.m_generate_mipmaps        = calculate_mip_count(image.width, image.height);
        texture_ptr base_color           = texture::create(config);

        format f        = format::RGBA;
        format internal = format::SRGB8_ALPHA8;

        if (image.component == 1)
        {
            f = format::RED;
        }
        else if (image.component == 2)
        {
            f = format::RG;
        }
        else if (image.component == 3)
        {
            f        = format::RGB;
            internal = format::SRGB8;
        }

        format type = format::UNSIGNED_BYTE;
        if (image.bits == 16)
        {
            type = format::UNSIGNED_SHORT;
        }
        else if (image.bits == 32)
        {
            type = format::UNSIGNED_INT;
        }

        base_color->set_data(internal, image.width, image.height, f, type, &image.image.at(0));
        material.component_material->base_color_texture = base_color;
    }

    // metallic / roughness
    if (pbr.metallicRoughnessTexture.index < 0)
    {
        material.component_material->metallic  = static_cast<float>(pbr.metallicFactor);
        material.component_material->roughness = static_cast<float>(pbr.roughnessFactor);
    }
    else
    {
        const tinygltf::Texture& o_r_m_t = m.textures.at(pbr.metallicRoughnessTexture.index);

        if (o_r_m_t.source < 0)
            return;

        const tinygltf::Image& image     = m.images[o_r_m_t.source];
        const tinygltf::Sampler& sampler = m.samplers[o_r_m_t.sampler];

        if (o_r_m_t.sampler >= 0)
        {
            config.m_texture_min_filter = filter_parameter_from_gl(sampler.minFilter);
            config.m_texture_mag_filter = filter_parameter_from_gl(sampler.magFilter);
            config.m_texture_wrap_s     = wrap_parameter_from_gl(sampler.wrapS);
            config.m_texture_wrap_t     = wrap_parameter_from_gl(sampler.wrapT);
        }

        config.m_is_standard_color_space = false;
        config.m_generate_mipmaps        = calculate_mip_count(image.width, image.height);
        texture_ptr o_r_m                = texture::create(config);

        format f        = format::RGBA;
        format internal = format::RGBA8;

        if (image.component == 1)
        {
            f = format::RED;
        }
        else if (image.component == 2)
        {
            f = format::RG;
        }
        else if (image.component == 3)
        {
            f        = format::RGB;
            internal = format::RGB8;
        }

        format type = format::UNSIGNED_BYTE;
        if (image.bits == 16)
        {
            type = format::UNSIGNED_SHORT;
        }
        else if (image.bits == 32)
        {
            type = format::UNSIGNED_INT;
        }

        o_r_m->set_data(internal, image.width, image.height, f, type, &image.image.at(0));
        material.component_material->roughness_metallic_texture = o_r_m;
    }

    // occlusion
    if (p_m.occlusionTexture.index >= 0)
    {
        if (pbr.metallicRoughnessTexture.index == p_m.occlusionTexture.index)
        {
            // occlusion packed into r channel of the roughness and metallic texture.
            material.component_material->packed_occlusion = true;
        }
        else
        {
            material.component_material->packed_occlusion = false;
            const tinygltf::Texture& occ                  = m.textures.at(p_m.occlusionTexture.index);
            if (occ.source < 0)
                return;

            const tinygltf::Image& image     = m.images[occ.source];
            const tinygltf::Sampler& sampler = m.samplers[occ.sampler];

            if (occ.sampler >= 0)
            {
                config.m_texture_min_filter = filter_parameter_from_gl(sampler.minFilter);
                config.m_texture_mag_filter = filter_parameter_from_gl(sampler.magFilter);
                config.m_texture_wrap_s     = wrap_parameter_from_gl(sampler.wrapS);
                config.m_texture_wrap_t     = wrap_parameter_from_gl(sampler.wrapT);
            }

            config.m_is_standard_color_space = false;
            config.m_generate_mipmaps        = calculate_mip_count(image.width, image.height);
            texture_ptr occlusion            = texture::create(config);

            format f        = format::RGBA;
            format internal = format::RGBA8;

            if (image.component == 1)
            {
                f = format::RED;
            }
            else if (image.component == 2)
            {
                f = format::RG;
            }
            else if (image.component == 3)
            {
                f        = format::RGB;
                internal = format::RGB8;
            }

            format type = format::UNSIGNED_BYTE;
            if (image.bits == 16)
            {
                type = format::UNSIGNED_SHORT;
            }
            else if (image.bits == 32)
            {
                type = format::UNSIGNED_INT;
            }

            occlusion->set_data(internal, image.width, image.height, f, type, &image.image.at(0));
            material.component_material->occlusion_texture = occlusion;
        }
    }

    // normal
    if (p_m.normalTexture.index >= 0)
    {
        const tinygltf::Texture& norm = m.textures.at(p_m.normalTexture.index);

        if (norm.source < 0)
            return;

        const tinygltf::Image& image     = m.images[norm.source];
        const tinygltf::Sampler& sampler = m.samplers[norm.sampler];

        if (norm.sampler >= 0)
        {
            config.m_texture_min_filter = filter_parameter_from_gl(sampler.minFilter);
            config.m_texture_mag_filter = filter_parameter_from_gl(sampler.magFilter);
            config.m_texture_wrap_s     = wrap_parameter_from_gl(sampler.wrapS);
            config.m_texture_wrap_t     = wrap_parameter_from_gl(sampler.wrapT);
        }

        config.m_is_standard_color_space = false;
        config.m_generate_mipmaps        = calculate_mip_count(image.width, image.height);
        texture_ptr normal_t             = texture::create(config);

        format f        = format::RGBA;
        format internal = format::RGBA8;

        if (image.component == 1)
        {
            f = format::RED;
        }
        else if (image.component == 2)
        {
            f = format::RG;
        }
        else if (image.component == 3)
        {
            f        = format::RGB;
            internal = format::RGB8;
        }

        format type = format::UNSIGNED_BYTE;
        if (image.bits == 16)
        {
            type = format::UNSIGNED_SHORT;
        }
        else if (image.bits == 32)
        {
            type = format::UNSIGNED_INT;
        }

        normal_t->set_data(internal, image.width, image.height, f, type, &image.image.at(0));
        material.component_material->normal_texture = normal_t;
    }

    // emissive
    if (p_m.emissiveTexture.index < 0)
    {
        auto col                                    = p_m.emissiveFactor;
        material.component_material->emissive_color = glm::vec3((float)col[0], (float)col[1], (float)col[2]);
    }
    else
    {
        const tinygltf::Texture& emissive = m.textures.at(p_m.emissiveTexture.index);

        if (emissive.source < 0)
            return;

        const tinygltf::Image& image     = m.images[emissive.source];
        const tinygltf::Sampler& sampler = m.samplers[emissive.sampler];

        if (emissive.sampler >= 0)
        {
            config.m_texture_min_filter = filter_parameter_from_gl(sampler.minFilter);
            config.m_texture_mag_filter = filter_parameter_from_gl(sampler.magFilter);
            config.m_texture_wrap_s     = wrap_parameter_from_gl(sampler.wrapS);
            config.m_texture_wrap_t     = wrap_parameter_from_gl(sampler.wrapT);
        }

        config.m_is_standard_color_space = true;
        config.m_generate_mipmaps        = calculate_mip_count(image.width, image.height);
        texture_ptr emissive_color       = texture::create(config);

        format f        = format::RGBA;
        format internal = format::SRGB8_ALPHA8;

        if (image.component == 1)
        {
            f = format::RED;
        }
        else if (image.component == 2)
        {
            f = format::RG;
        }
        else if (image.component == 3)
        {
            f        = format::RGB;
            internal = format::SRGB8;
        }

        format type = format::UNSIGNED_BYTE;
        if (image.bits == 16)
        {
            type = format::UNSIGNED_SHORT;
        }
        else if (image.bits == 32)
        {
            type = format::UNSIGNED_INT;
        }

        emissive_color->set_data(internal, image.width, image.height, f, type, &image.image.at(0));
        material.component_material->emissive_color_texture = emissive_color;
    }

    // transparency
    if (p_m.alphaMode.compare("OPAQUE") == 0)
    {
        material.component_material->alpha_rendering = alpha_mode::MODE_OPAQUE;
        material.component_material->alpha_cutoff    = 1.0f;
    }
    if (p_m.alphaMode.compare("MASK") == 0)
    {
        material.component_material->alpha_rendering = alpha_mode::MODE_MASK;
        material.component_material->alpha_cutoff    = static_cast<float>(p_m.alphaCutoff);
    }
    if (p_m.alphaMode.compare("BLEND") == 0)
    {
        material.component_material->alpha_rendering = alpha_mode::MODE_BLEND;
        material.component_material->alpha_cutoff    = 1.0f;
        MANGO_LOG_WARN("Alpha blending currently not supported!");
    }
}

static void render_meshes(shared_ptr<render_system_impl> rs, scene_component_pool<mesh_component>& meshes, scene_component_pool<transform_component>& transformations)
{
    PROFILE_ZONE;
    meshes.for_each(
        [&rs, &meshes, &transformations](mesh_component& c, int32& index) {
            entity e                       = meshes.entity_at(index);
            transform_component* transform = transformations.get_component_for_entity(e);
            if (transform)
            {
                auto cmdb = rs->get_command_buffer();
                rs->set_model_info(transform->world_transformation_matrix, c.has_normals, c.has_tangents);

                for (int32 i = 0; i < static_cast<int32>(c.primitives.size()); ++i)
                {
                    auto m = c.materials[i];
                    auto p = c.primitives[i];
                    cmdb->bind_vertex_array(p.vertex_array_object);
                    rs->draw_mesh(m.component_material, p.topology, p.first, p.count, p.type_index, p.instance_count);
                }
            }
        },
        false);
}

static void update_scene_boundaries(glm::mat4& trafo, tinygltf::Model& m, tinygltf::Mesh& mesh, glm::vec3& min, glm::vec3& max)
{
    PROFILE_ZONE;
    if (mesh.primitives.empty())
        return;

    glm::vec3 min_a;
    glm::vec3 max_a;
    glm::vec3 center;
    glm::vec3 to_center;
    float radius;

    for (int32 j = 0; j < static_cast<int32>(mesh.primitives.size()); ++j)
    {
        const tinygltf::Primitive& primitive = mesh.primitives[j];

        auto attrib = primitive.attributes.find("POSITION");
        if (attrib == primitive.attributes.end())
            continue;

        const tinygltf::Accessor& accessor = m.accessors[attrib->second];

        max_a.x = (float)accessor.maxValues[0];
        max_a.y = (float)accessor.maxValues[1];
        max_a.z = (float)accessor.maxValues[2];
        min_a.x = (float)accessor.minValues[0];
        min_a.y = (float)accessor.minValues[1];
        min_a.z = (float)accessor.minValues[2];

        max_a = glm::vec3(trafo * glm::vec4(max_a, 1.0f));
        min_a = glm::vec3(trafo * glm::vec4(min_a, 1.0f));

        center    = (max_a + min_a) * 0.5f;
        to_center = max_a - center;
        radius    = glm::length(to_center);

        max_a = center + glm::vec3(radius);
        min_a = center - glm::vec3(radius);

        max = glm::max(max, max_a);
        min = glm::min(min, min_a);
    }
}
