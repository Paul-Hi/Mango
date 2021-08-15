//! \file      gl_graphics_resources.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#include <glad/glad.h>
#include <graphics/opengl/gl_graphics_resources.hpp>
#include <mango/profile.hpp>

using namespace mango;

gl_shader_stage::gl_shader_stage(const shader_stage_create_info& info)
    : m_info(info)
{
    create_shader_from_source();

    set_uid(get_uid_low(), m_shader_stage_gl_handle);
}

void gl_shader_stage::create_shader_from_source()
{
    m_shader_stage_gl_handle = glCreateShader(gfx_shader_stage_type_to_gl(m_info.stage));
    glShaderSource(m_shader_stage_gl_handle, 1, &m_info.shader_source.source, &m_info.shader_source.size);
    MANGO_LOG_INFO("Entry point specification is currently not supported and is \"main\"!"); // TODO Paul
    glCompileShader(m_shader_stage_gl_handle);
    int32 status = 0;
    glGetShaderiv(m_shader_stage_gl_handle, GL_COMPILE_STATUS, &status);
    if (GL_FALSE == status)
    {
        int32 log_size = 0;
        glGetShaderiv(m_shader_stage_gl_handle, GL_INFO_LOG_LENGTH, &log_size);
        std::vector<char> info_log(log_size);
        glGetShaderInfoLog(m_shader_stage_gl_handle, log_size, &log_size, info_log.data());
        glDeleteShader(m_shader_stage_gl_handle);
        m_shader_stage_gl_handle = 0;

        MANGO_LOG_ERROR("Shader compilation failed: {0} !", info_log.data());
        return;
    }
}

gl_shader_stage::~gl_shader_stage()
{
    glDeleteShader(m_shader_stage_gl_handle);
}

gl_buffer::gl_buffer(const buffer_create_info& info)
    : m_info(info)
{
    glCreateBuffers(1, &m_buffer_gl_handle);
    glNamedBufferStorage(m_buffer_gl_handle, m_info.size, nullptr, gfx_buffer_access_to_gl(m_info.buffer_access));

    set_uid(get_uid_low(), m_buffer_gl_handle);
}

gl_buffer gl_buffer::dummy()
{
    return gl_buffer();
}

gl_buffer::~gl_buffer()
{
    glDeleteBuffers(1, &m_buffer_gl_handle);
}

gl_texture::gl_texture(const texture_create_info& info)
    : m_info(info)
{
    glCreateTextures(gfx_texture_type_to_gl(m_info.texture_type), 1, &m_texture_gl_handle);

    gl_enum internal_format = gfx_format_to_gl(m_info.texture_format);

    m_info.miplevels = glm::min(m_info.miplevels, gfx_calculate_max_miplevels(m_info.width, m_info.height));

    switch (m_info.texture_type)
    {
    case gfx_texture_type::texture_type_1d:
        glTextureStorage1D(m_texture_gl_handle, m_info.miplevels, internal_format, m_info.width);
        break;
    case gfx_texture_type::texture_type_2d:
    case gfx_texture_type::texture_type_rectangle:
        glTextureStorage2D(m_texture_gl_handle, m_info.miplevels, internal_format, m_info.width, m_info.height);
        break;
    case gfx_texture_type::texture_type_cube_map:
        glTextureStorage2D(m_texture_gl_handle, m_info.miplevels, internal_format, m_info.width, m_info.height);
        break;
    case gfx_texture_type::texture_type_1d_array:
        glTextureStorage2D(m_texture_gl_handle, m_info.miplevels, internal_format, m_info.width, m_info.array_layers);
        break;
    case gfx_texture_type::texture_type_3d:
    case gfx_texture_type::texture_type_2d_array:
    case gfx_texture_type::texture_type_cube_map_array:
        glTextureStorage3D(m_texture_gl_handle, m_info.miplevels, internal_format, m_info.width, m_info.height, m_info.array_layers);
        break;
    default:
        MANGO_LOG_ERROR("Unknown texture_type!");
    }
    // Binding reqiured since we only use glBindTextures later // TODO Paul: Check!
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(gfx_texture_type_to_gl(m_info.texture_type), m_texture_gl_handle);
    glBindTexture(gfx_texture_type_to_gl(m_info.texture_type), 0);

    set_uid(get_uid_low(), m_texture_gl_handle);
}

gl_texture gl_texture::dummy()
{
    return gl_texture();
}

gl_texture::~gl_texture()
{
    glDeleteTextures(1, &m_texture_gl_handle);
}

gl_sampler::gl_sampler(const sampler_create_info& info)
    : m_info(info)
{
    glCreateSamplers(1, &m_sampler_gl_handle);
    glSamplerParameteri(m_sampler_gl_handle, GL_TEXTURE_MIN_FILTER, gfx_sampler_filter_to_gl(m_info.sampler_min_filter));
    glSamplerParameteri(m_sampler_gl_handle, GL_TEXTURE_MAG_FILTER, gfx_sampler_filter_to_gl(m_info.sampler_max_filter));

    gl_enum wrap_s = gfx_sampler_edge_wrap_to_gl(m_info.edge_value_wrap_u);
    gl_enum wrap_t = gfx_sampler_edge_wrap_to_gl(m_info.edge_value_wrap_v);
    gl_enum wrap_r = gfx_sampler_edge_wrap_to_gl(m_info.edge_value_wrap_w);
    glSamplerParameteri(m_sampler_gl_handle, GL_TEXTURE_WRAP_S, wrap_s);
    glSamplerParameteri(m_sampler_gl_handle, GL_TEXTURE_WRAP_T, wrap_t);
    glSamplerParameteri(m_sampler_gl_handle, GL_TEXTURE_WRAP_R, wrap_r);

    // TODO Paul: Additional checks for valid configurations?
    glSamplerParameteri(m_sampler_gl_handle, GL_TEXTURE_COMPARE_MODE, m_info.enable_comparison_mode ? GL_COMPARE_REF_TO_TEXTURE : GL_NONE);
    glSamplerParameteri(m_sampler_gl_handle, GL_TEXTURE_COMPARE_FUNC, gfx_compare_operator_to_gl(m_info.comparison_operator));

    glGetSamplerParameterfv(m_sampler_gl_handle, GL_TEXTURE_BORDER_COLOR, m_info.border_color.data());

#ifdef MANGO_DEBUG
    if (m_info.enable_seamless_cubemap)
        MANGO_LOG_WARN("Can not enable seamless cubemaps per texture, enable it globally!"); // TODO Paul
#endif                                                                                       // MANGO_DEBUG

    set_uid(get_uid_low(), m_sampler_gl_handle);
}

gl_sampler gl_sampler::dummy()
{
    return gl_sampler();
}

gl_sampler::~gl_sampler()
{
    glDeleteSamplers(1, &m_sampler_gl_handle);
}

gl_semaphore::gl_semaphore(const semaphore_create_info& info)
    : m_info(info)
{
    m_semaphore_gl_handle = static_cast<gl_sync>(glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0));

    set_uid(get_uid_low(), static_cast<uint32>(reinterpret_cast<ptr_size>(m_semaphore_gl_handle))); // TODO Paul: Is that cast okay?
}

gl_semaphore::~gl_semaphore()
{
    GLsync sync_object = static_cast<GLsync>(m_semaphore_gl_handle);

    if (glIsSync(sync_object))
        glDeleteSync(sync_object); // TODO Paul: Does this work?
}

bool gl_shader_resource_mapping::set(const string variable_name, gfx_handle<const gfx_device_object> resource)
{
    auto query = m_name_to_binding_pair.find(variable_name);
    if (query == m_name_to_binding_pair.end())
    {
        MANGO_LOG_ERROR("Mapping for {0} does not exist!", variable_name);
        return false;
    }

    int32 binding               = query->second.first;
    gfx_shader_resource_type tp = query->second.second;

    switch (tp)
    {
    case gfx_shader_resource_type::shader_resource_constant_buffer:
    case gfx_shader_resource_type::shader_resource_buffer_storage:
    {
        auto& device_pair = m_buffers.at(binding);
        if (device_pair.second > 2)
        {
            MANGO_LOG_ERROR("Mapping for static binding {0} already set!", binding);
            return false;
        }
        if (resource->get_type_id() != device_pair.first->get_type_id())
        {
            MANGO_LOG_ERROR("Type {0} does not fit for shader resource with type {1}!", resource->get_type_id(), tp);
            return false;
        }
        if (device_pair.second == 2)
            device_pair.second = 3;

        device_pair.first = static_gfx_handle_cast<const gl_buffer>(resource);

        break;
    }
    case gfx_shader_resource_type::shader_resource_image_storage:
    {
        auto& device_pair = m_texture_images.at(binding);
        if (device_pair.second > 2)
        {
            MANGO_LOG_ERROR("Mapping for static binding {0} already set!", binding);
            return false;
        }
        if (resource->get_type_id() != device_pair.first->get_type_id())
        {
            MANGO_LOG_ERROR("Type {0} does not fit for shader resource with type {1}!", resource->get_type_id(), tp);
            return false;
        }
        if (device_pair.second == 2)
            device_pair.second = 3;

        device_pair.first = static_gfx_handle_cast<const gl_image_texture_view>(resource);

        break;
    }
    case gfx_shader_resource_type::shader_resource_texture:
    case gfx_shader_resource_type::shader_resource_input_attachment:
    {
        auto& device_pair = m_textures.at(binding);
        if (device_pair.second > 2)
        {
            MANGO_LOG_ERROR("Mapping for static binding {0} already set!", binding);
            return false;
        }
        if (resource->get_type_id() != device_pair.first->get_type_id())
        {
            MANGO_LOG_ERROR("Type {0} does not fit for shader resource with type {1}!", resource->get_type_id(), tp);
            return false;
        }
        if (device_pair.second == 2)
            device_pair.second = 3;

        device_pair.first = static_gfx_handle_cast<const gl_texture>(resource);

        break;
    }
    case gfx_shader_resource_type::shader_resource_sampler:
    {
        auto& device_pair = m_samplers.at(binding);
        if (device_pair.second > 2)
        {
            MANGO_LOG_ERROR("Mapping for static binding {0} already set!", binding);
            return false;
        }
        if (resource->get_type_id() != device_pair.first->get_type_id())
        {
            MANGO_LOG_ERROR("Type {0} does not fit for shader resource with type {1}!", resource->get_type_id(), tp);
            return false;
        }
        if (device_pair.second == 2)
            device_pair.second = 3;

        device_pair.first = static_gfx_handle_cast<const gl_sampler>(resource);

        break;
    }
    default:
        MANGO_ASSERT(false, "Shader resource type is unknown!", tp);
        break;
    }

    return true;
}

gl_pipeline_resource_layout::gl_pipeline_resource_layout(std::initializer_list<shader_resource_binding> bindings)
    : m_bindings(std::forward<std::initializer_list<shader_resource_binding>>(bindings))
{
}

gl_graphics_pipeline::gl_graphics_pipeline(const graphics_pipeline_create_info& info)
    : m_info(info)
{
    NAMED_PROFILE_ZONE("Create Graphics Pipeline Ressource Mapping");
    // Create mapping from layout.
    gfx_handle<const gl_pipeline_resource_layout> gl_layout = static_gfx_handle_cast<const gl_pipeline_resource_layout>(info.pipeline_layout);

    graphics_shader_stage_descriptor shader_stages   = info.shader_stage_descriptor;
    gfx_handle<const gl_shader_stage> vertex_stage   = static_gfx_handle_cast<const gl_shader_stage>(shader_stages.vertex_shader_stage);
    gfx_handle<const gl_shader_stage> geometry_stage = static_gfx_handle_cast<const gl_shader_stage>(shader_stages.geometry_shader_stage);
    gfx_handle<const gl_shader_stage> fragment_stage = static_gfx_handle_cast<const gl_shader_stage>(shader_stages.fragment_shader_stage);

    m_mapping = make_gfx_handle<gl_shader_resource_mapping>();
    for (const shader_resource_binding b : gl_layout->m_bindings)
    {
        gl_shader_resource_mapping::status_bit status;

        if (b.access == gfx_shader_resource_access::shader_access_dynamic)
            status = 1;
        else if (b.access == gfx_shader_resource_access::shader_access_static)
            status = 2;
        else
        {
            MANGO_LOG_WARN("Shader resource access unspecified, making it static!");
            status = 2;
        }

        const char* name     = nullptr;
        int32 array_size_out = 0;

        switch (b.stage)
        {
        case gfx_shader_stage_type::shader_stage_vertex:
        {
            MANGO_ASSERT(vertex_stage->m_info.stage == b.stage, "Shader type does not fit type of shader info.");
            name = query_shader_info(b.binding, vertex_stage->m_info, b.type, array_size_out);
            break;
        }
        case gfx_shader_stage_type::shader_stage_geometry:
        {
            MANGO_ASSERT(geometry_stage->m_info.stage == b.stage, "Shader type does not fit type of shader info.");
            name = query_shader_info(b.binding, geometry_stage->m_info, b.type, array_size_out);
            break;
        }
        case gfx_shader_stage_type::shader_stage_fragment:
        {
            MANGO_ASSERT(fragment_stage->m_info.stage == b.stage, "Shader type does not fit type of shader info.");
            name = query_shader_info(b.binding, fragment_stage->m_info, b.type, array_size_out);
            break;
        }
        case gfx_shader_stage_type::shader_stage_compute:
        {
            MANGO_ASSERT(false, "Compute Stage in Graphics Pipeline!");
            break;
        }
        default:
            MANGO_ASSERT(false, "Stage is currently not supported!");
            break;
        }

        switch (b.type)
        {
        case gfx_shader_resource_type::shader_resource_constant_buffer:
        case gfx_shader_resource_type::shader_resource_buffer_storage:
        {
            gl_shader_resource_mapping::resource_pair<gl_buffer> device_pair;
            device_pair.first  = make_gfx_handle<gl_buffer>(gl_buffer::dummy());
            device_pair.second = status;
            if (static_cast<int32>(m_mapping->m_buffers.size()) < b.binding + array_size_out)
                m_mapping->m_buffers.resize(b.binding + array_size_out);
            if (array_size_out == 1)
            {
                m_mapping->m_name_to_binding_pair.insert({ name, { b.binding, b.type } });
                m_mapping->m_buffers.at(b.binding) = device_pair;
            }
            else
            {
                string indexed_name;
                for (int32 c = 0; c < array_size_out; ++c)
                {
                    indexed_name = string(name) + "[" + std::to_string(c) + "]";
                    m_mapping->m_name_to_binding_pair.insert({ indexed_name.c_str(), { b.binding + c, b.type } });
                    m_mapping->m_buffers.at(b.binding + c) = device_pair;
                }
            }
            break;
        }
        case gfx_shader_resource_type::shader_resource_image_storage:
        {
            gl_shader_resource_mapping::resource_pair<gl_image_texture_view> device_pair;
            device_pair.first  = make_gfx_handle<gl_image_texture_view>(gl_image_texture_view(make_gfx_handle<gl_texture>(gl_texture::dummy()), 0));
            device_pair.second = status;
            if (static_cast<int32>(m_mapping->m_texture_images.size()) < b.binding + array_size_out)
                m_mapping->m_texture_images.resize(b.binding + array_size_out);
            if (array_size_out == 1)
            {
                m_mapping->m_name_to_binding_pair.insert({ name, { b.binding, b.type } });
                m_mapping->m_texture_images.at(b.binding) = device_pair;
            }
            else
            {
                string indexed_name;
                for (int32 c = 0; c < array_size_out; ++c)
                {
                    indexed_name = string(name) + "[" + std::to_string(c) + "]";
                    m_mapping->m_name_to_binding_pair.insert({ indexed_name.c_str(), { b.binding + c, b.type } });
                    m_mapping->m_texture_images.at(b.binding + c) = device_pair;
                }
            }
            break;
        }
        case gfx_shader_resource_type::shader_resource_texture:
        case gfx_shader_resource_type::shader_resource_input_attachment:
        {
            gl_shader_resource_mapping::resource_pair<gl_texture> device_pair;
            device_pair.first  = make_gfx_handle<gl_texture>(gl_texture::dummy());
            device_pair.second = status;
            if (static_cast<int32>(m_mapping->m_textures.size()) < b.binding + array_size_out)
                m_mapping->m_textures.resize(b.binding + array_size_out);
            if (array_size_out == 1)
            {
                m_mapping->m_name_to_binding_pair.insert({ name, { b.binding, b.type } });
                m_mapping->m_textures.at(b.binding) = device_pair;
            }
            else
            {
                string indexed_name;
                for (int32 c = 0; c < array_size_out; ++c)
                {
                    indexed_name = string(name) + "[" + std::to_string(c) + "]";
                    m_mapping->m_name_to_binding_pair.insert({ indexed_name.c_str(), { b.binding + c, b.type } });
                    m_mapping->m_textures.at(b.binding + c) = device_pair;
                }
            }
            break;
        }
        case gfx_shader_resource_type::shader_resource_sampler:
        {
            gl_shader_resource_mapping::resource_pair<gl_sampler> device_pair;
            device_pair.first  = make_gfx_handle<gl_sampler>(gl_sampler::dummy());
            device_pair.second = status;
            if (static_cast<int32>(m_mapping->m_samplers.size()) < b.binding + array_size_out)
                m_mapping->m_samplers.resize(b.binding + array_size_out);
            if (array_size_out == 1)
            {
                m_mapping->m_name_to_binding_pair.insert({ name, { b.binding, b.type } });
                m_mapping->m_samplers.at(b.binding) = device_pair;
            }
            else
            {
                string indexed_name;
                for (int32 c = 0; c < array_size_out; ++c)
                {
                    indexed_name = string(name) + "[" + std::to_string(c) + "]";
                    m_mapping->m_name_to_binding_pair.insert({ indexed_name.c_str(), { b.binding + c, b.type } });
                    m_mapping->m_samplers.at(b.binding + c) = device_pair;
                }
            }
            break;
        }
        default:
            MANGO_ASSERT(false, "Unknown shader resource type!");
            break;
        }
    }
}

gl_compute_pipeline::gl_compute_pipeline(const compute_pipeline_create_info& info)
    : m_info(info)
{
    NAMED_PROFILE_ZONE("Create Compute Pipeline Ressource Mapping");
    // Create mapping from layout.
    gfx_handle<const gl_pipeline_resource_layout> gl_layout = static_gfx_handle_cast<const gl_pipeline_resource_layout>(info.pipeline_layout);

    compute_shader_stage_descriptor shader_stages   = info.shader_stage_descriptor;
    gfx_handle<const gl_shader_stage> compute_stage = static_gfx_handle_cast<const gl_shader_stage>(shader_stages.compute_shader_stage);

    m_mapping = make_gfx_handle<gl_shader_resource_mapping>();
    for (const shader_resource_binding b : gl_layout->m_bindings)
    {
        gl_shader_resource_mapping::status_bit status;

        if (b.access == gfx_shader_resource_access::shader_access_dynamic)
            status = 1;
        else if (b.access == gfx_shader_resource_access::shader_access_static)
            status = 2;
        else
        {
            MANGO_LOG_WARN("Shader resource access unspecified, making it static!");
            status = 2;
        }

        const char* name     = nullptr;
        int32 array_size_out = 0;

        switch (b.stage)
        {
        case gfx_shader_stage_type::shader_stage_vertex:
        case gfx_shader_stage_type::shader_stage_geometry:
        case gfx_shader_stage_type::shader_stage_fragment:
        {
            MANGO_ASSERT(false, "Graphics Stage in Compute Pipeline!");
            break;
        }
        case gfx_shader_stage_type::shader_stage_compute:
        {
            MANGO_ASSERT(compute_stage->m_info.stage == b.stage, "Shader type does not fit type of shader info.");
            name = query_shader_info(b.binding, compute_stage->m_info, b.type, array_size_out);
            break;
        }
        default:
            MANGO_ASSERT(false, "Stage is currently not supported!");
            break;
        }

        switch (b.type)
        {
        case gfx_shader_resource_type::shader_resource_constant_buffer:
        case gfx_shader_resource_type::shader_resource_buffer_storage:
        {
            gl_shader_resource_mapping::resource_pair<gl_buffer> device_pair;
            device_pair.first  = make_gfx_handle<gl_buffer>(gl_buffer::dummy());
            device_pair.second = status;
            if (static_cast<int32>(m_mapping->m_buffers.size()) < b.binding + array_size_out)
                m_mapping->m_buffers.resize(b.binding + array_size_out);
            if (array_size_out == 1)
            {
                m_mapping->m_name_to_binding_pair.insert({ name, { b.binding, b.type } });
                m_mapping->m_buffers.at(b.binding) = device_pair;
            }
            else
            {
                string indexed_name;
                for (int32 c = 0; c < array_size_out; ++c)
                {
                    indexed_name = string(name) + "[" + std::to_string(c) + "]";
                    m_mapping->m_name_to_binding_pair.insert({ indexed_name.c_str(), { b.binding + c, b.type } });
                    m_mapping->m_buffers.at(b.binding + c) = device_pair;
                }
            }
            break;
        }
        case gfx_shader_resource_type::shader_resource_image_storage:
        {
            gl_shader_resource_mapping::resource_pair<gl_image_texture_view> device_pair;
            device_pair.first  = make_gfx_handle<gl_image_texture_view>(gl_image_texture_view(make_gfx_handle<gl_texture>(gl_texture::dummy()), 0));
            device_pair.second = status;
            if (static_cast<int32>(m_mapping->m_texture_images.size()) < b.binding + array_size_out)
                m_mapping->m_texture_images.resize(b.binding + array_size_out);
            if (array_size_out == 1)
            {
                m_mapping->m_name_to_binding_pair.insert({ name, { b.binding, b.type } });
                m_mapping->m_texture_images.at(b.binding) = device_pair;
            }
            else
            {
                string indexed_name;
                for (int32 c = 0; c < array_size_out; ++c)
                {
                    indexed_name = string(name) + "[" + std::to_string(c) + "]";
                    m_mapping->m_name_to_binding_pair.insert({ indexed_name.c_str(), { b.binding + c, b.type } });
                    m_mapping->m_texture_images.at(b.binding + c) = device_pair;
                }
            }
            break;
        }
        case gfx_shader_resource_type::shader_resource_texture:
        case gfx_shader_resource_type::shader_resource_input_attachment:
        {
            gl_shader_resource_mapping::resource_pair<gl_texture> device_pair;
            device_pair.first  = make_gfx_handle<gl_texture>(gl_texture::dummy());
            device_pair.second = status;
            if (static_cast<int32>(m_mapping->m_textures.size()) < b.binding + array_size_out)
                m_mapping->m_textures.resize(b.binding + array_size_out);
            if (array_size_out == 1)
            {
                m_mapping->m_name_to_binding_pair.insert({ name, { b.binding, b.type } });
                m_mapping->m_textures.at(b.binding) = device_pair;
            }
            else
            {
                string indexed_name;
                for (int32 c = 0; c < array_size_out; ++c)
                {
                    indexed_name = string(name) + "[" + std::to_string(c) + "]";
                    m_mapping->m_name_to_binding_pair.insert({ indexed_name.c_str(), { b.binding + c, b.type } });
                    m_mapping->m_textures.at(b.binding + c) = device_pair;
                }
            }
            break;
        }
        case gfx_shader_resource_type::shader_resource_sampler:
        {
            gl_shader_resource_mapping::resource_pair<gl_sampler> device_pair;
            device_pair.first  = make_gfx_handle<gl_sampler>(gl_sampler::dummy());
            device_pair.second = status;
            if (static_cast<int32>(m_mapping->m_samplers.size()) < b.binding + array_size_out)
                m_mapping->m_samplers.resize(b.binding + array_size_out);
            if (array_size_out == 1)
            {
                m_mapping->m_name_to_binding_pair.insert({ name, { b.binding, b.type } });
                m_mapping->m_samplers.at(b.binding) = device_pair;
            }
            else
            {
                string indexed_name;
                for (int32 c = 0; c < array_size_out; ++c)
                {
                    indexed_name = string(name) + "[" + std::to_string(c) + "]";
                    m_mapping->m_name_to_binding_pair.insert({ indexed_name.c_str(), { b.binding + c, b.type } });
                    m_mapping->m_samplers.at(b.binding + c) = device_pair;
                }
            }
            break;
        }
        default:
            MANGO_ASSERT(false, "Unknown shader resource type!");
            break;
        }
    }
}

gfx_handle<shader_resource_mapping> gl_pipeline::get_resource_mapping() const
{
    return m_mapping;
}

void gl_pipeline::submit_pipeline_resources(gfx_handle<gfx_graphics_state> shared_graphics_state) const
{
    NAMED_PROFILE_ZONE("Submit Pipeline Resources");
    GL_NAMED_PROFILE_ZONE("Submit Pipeline Resources");
    int32 buffers_count = static_cast<int32>(m_mapping->m_buffers.size());
    for (int32 b = 0; b < buffers_count; ++b)
    {
        auto& buffer = m_mapping->m_buffers[b];
        if (buffer.second == 0 || buffer.first->m_buffer_gl_handle == 0)
            continue;
        if (shared_graphics_state->is_buffer_bound(buffer.first->m_info.buffer_target, b, buffer.first->native_handle()))
            continue;
        glBindBufferBase(gfx_buffer_target_to_gl(buffer.first->m_info.buffer_target), b, buffer.first->m_buffer_gl_handle);
        shared_graphics_state->record_buffer_binding(buffer.first->m_info.buffer_target, b, buffer.first->native_handle());
    }

    std::vector<gl_handle> gl_handles;
    int32 start_binding = 0;

    int32 textures_count = static_cast<int32>(m_mapping->m_textures.size());
    gl_handles.reserve(textures_count);
    for (int32 b = 0; b < textures_count; ++b)
    {
        auto& texture = m_mapping->m_textures[b];
        if (texture.second == 0)
            continue;
        if (texture.first->m_texture_gl_handle > 0)
            gl_handles.push_back(texture.first->m_texture_gl_handle);
        else
        {
            if (gl_handles.size())
            {
                glBindTextures(start_binding, static_cast<uint32>(gl_handles.size()), gl_handles.data());
                gl_handles.clear();
            }
            start_binding = b + 1;
        }
    }
    if (gl_handles.size())
    {
        glBindTextures(start_binding, static_cast<uint32>(gl_handles.size()), gl_handles.data());
        gl_handles.clear();
    }

    int32 texture_images_count = static_cast<int32>(m_mapping->m_texture_images.size());
    // TODO Paul: Doing this for now, since there seemes to be a problem with cubemaps.
    for (int32 b = 0; b < texture_images_count; ++b)
    {
        auto& image_texture_view = m_mapping->m_texture_images[b];
        if (image_texture_view.second == 0)
            continue;

        auto internal = image_texture_view.first->m_texture->m_info.texture_format;
        glBindImageTexture(b, image_texture_view.first->m_texture->m_texture_gl_handle, image_texture_view.first->m_level, GL_FALSE, 0, GL_READ_WRITE, gfx_format_to_gl(internal));
    }
    /*
    gl_handles.reserve(texture_images_count);
    start_binding = 0;
    for (int32 b = 0; b < texture_images_count; ++b)
    {
        auto& image_texture_view = m_mapping->m_texture_images[b];
        if (image_texture_view.second == 0)
            continue;
        if (image_texture_view.first->m_texture->m_texture_gl_handle > 0 && image_texture_view.first->m_level == 0)
            gl_handles.push_back(image_texture_view.first->m_texture->m_texture_gl_handle);
        else
        {
            if (gl_handles.size())
            {
                glBindImageTextures(start_binding, gl_handles.size(), gl_handles.data());
                gl_handles.clear();
            }
            start_binding = b + 1;
            if (image_texture_view.first->m_texture->m_texture_gl_handle > 0 && image_texture_view.first->m_level > 0)
            {
                auto internal = image_texture_view.first->m_texture->m_info.texture_format;
                glBindImageTexture(b, image_texture_view.first->m_texture->m_texture_gl_handle, image_texture_view.first->m_level, GL_FALSE, 0, GL_READ_WRITE, gfx_format_to_gl(internal));
            }
        }
    }
    if (gl_handles.size())
    {
        glBindImageTextures(start_binding, gl_handles.size(), gl_handles.data());
        gl_handles.clear();
    }*/

    int32 sampler_count = static_cast<int32>(m_mapping->m_samplers.size());
    gl_handles.reserve(sampler_count);
    start_binding = 0;
    for (int32 b = 0; b < sampler_count; ++b)
    {
        auto& sampler = m_mapping->m_samplers[b];
        if (sampler.second == 0)
            continue;
        if (sampler.first->m_sampler_gl_handle > 0)
            gl_handles.push_back(sampler.first->m_sampler_gl_handle);
        else
        {
            if (gl_handles.size())
            {
                glBindSamplers(start_binding, static_cast<uint32>(gl_handles.size()), gl_handles.data());
                gl_handles.clear();
            }
            start_binding = b + 1;
        }
    }
    if (gl_handles.size())
    {
        glBindSamplers(start_binding, static_cast<uint32>(gl_handles.size()), gl_handles.data());
        gl_handles.clear();
    }
}

const char* gl_pipeline::query_shader_info(int32 binding, const shader_stage_create_info& shader_info, gfx_shader_resource_type type, int32& array_size)
{
    if (shader_info.resource_count == 0)
    {
        MANGO_LOG_ERROR("Shader stage does not provide type info for resource.");
        return nullptr;
    }

    for (int32 i = 0; i < shader_info.resource_count; ++i)
    {
        const shader_resource_description& desc = shader_info.resources[i];
        if (binding == desc.binding && type == desc.type)
        {
            array_size = desc.array_size;
            return desc.variable_name;
        }
    }

    MANGO_LOG_ERROR("Shader stage does not provide type info for resource.");
    return nullptr;
}
