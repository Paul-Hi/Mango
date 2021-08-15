//! \file      gl_shader_program_cache.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#include <graphics/opengl/gl_shader_program_cache.hpp>

using namespace mango;

gl_shader_program_cache::gl_shader_program_cache() {}

gl_shader_program_cache::~gl_shader_program_cache()
{
    for (auto sp_handle : cache)
    {
        glDeleteProgram(sp_handle.second);
    }
    cache.clear();
}

gl_handle gl_shader_program_cache::get_shader_program(const graphics_shader_stage_descriptor& desc)
{
    shader_program_key key;

    key.stage_count = 0;
    gl_handle handles[3];
    handles[0] = handles[1] = handles[2] = 0;

    if (desc.vertex_shader_stage)
    {
        MANGO_ASSERT(std::dynamic_pointer_cast<const gl_shader_stage>(desc.vertex_shader_stage), "Shader stage is not a gl_shader_stage!");
        auto vertex_shader                     = static_gfx_handle_cast<const gl_shader_stage>(desc.vertex_shader_stage);
        key.shader_stage_uids[key.stage_count] = vertex_shader->get_uid();
        key.stage_types[key.stage_count]       = gfx_shader_stage_type::shader_stage_vertex;
        key.stage_count++;

        handles[0] = vertex_shader->m_shader_stage_gl_handle;
    }

    if (desc.geometry_shader_stage)
    {
        MANGO_ASSERT(std::dynamic_pointer_cast<const gl_shader_stage>(desc.geometry_shader_stage), "Shader stage is not a gl_shader_stage!");
        auto geometry_shader                   = static_gfx_handle_cast<const gl_shader_stage>(desc.geometry_shader_stage);
        key.shader_stage_uids[key.stage_count] = geometry_shader->get_uid();
        key.stage_types[key.stage_count]       = gfx_shader_stage_type::shader_stage_geometry;
        key.stage_count++;

        handles[1] = geometry_shader->m_shader_stage_gl_handle;
    }

    if (desc.fragment_shader_stage)
    {
        MANGO_ASSERT(std::dynamic_pointer_cast<const gl_shader_stage>(desc.fragment_shader_stage), "Shader stage is not a gl_shader_stage!");
        auto fragment_shader                   = static_gfx_handle_cast<const gl_shader_stage>(desc.fragment_shader_stage);
        key.shader_stage_uids[key.stage_count] = fragment_shader->get_uid();
        key.stage_types[key.stage_count]       = gfx_shader_stage_type::shader_stage_fragment;
        key.stage_count++;

        handles[2] = fragment_shader->m_shader_stage_gl_handle;
    }

    // TODO Paul: Check these!
    MANGO_ASSERT(desc.vertex_shader_stage || desc.geometry_shader_stage, "Vertex or Geometry shader has to exist in a graphics pipeline!");
    MANGO_ASSERT(desc.fragment_shader_stage, "Fragment shader has to exist in a graphics pipeline!");

    auto result = cache.find(key);

    if (result != cache.end())
        return result->second;

    gl_handle created = create(key.stage_count, handles);

    cache.insert({ key, created });

    return created;
}

gl_handle gl_shader_program_cache::get_shader_program(const compute_shader_stage_descriptor& desc)
{
    shader_program_key key;

    key.stage_count = 0;
    gl_handle handle;

    if (desc.compute_shader_stage)
    {
        MANGO_ASSERT(std::dynamic_pointer_cast<const gl_shader_stage>(desc.compute_shader_stage), "Shader stage is not a gl_shader_stage!");
        auto compute_shader                    = static_gfx_handle_cast<const gl_shader_stage>(desc.compute_shader_stage);
        key.shader_stage_uids[key.stage_count] = compute_shader->get_uid();
        key.stage_types[key.stage_count]       = gfx_shader_stage_type::shader_stage_compute;
        key.stage_count++;

        handle = compute_shader->m_shader_stage_gl_handle;
    }

    MANGO_ASSERT(desc.compute_shader_stage, "Compute pipeline needs a compute shader stage!");

    auto result = cache.find(key);

    if (result != cache.end())
        return result->second;

    gl_handle created = create(key.stage_count, &handle);

    cache.insert({ key, created });

    return created;
}

gl_handle gl_shader_program_cache::create(int32 stage_count, gl_handle handles[max_shader_stages])
{
    gl_handle program = glCreateProgram();

    int32 limit = stage_count;

    for (int32 i = 0; i < limit; ++i)
    {
        if (handles[i] > 0)
            glAttachShader(program, handles[i]);
        else
            limit++;
    }

    glLinkProgram(program);

    int32 status = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (GL_FALSE == status)
    {
        int32 log_length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
        std::vector<char> info_log(log_length);
        glGetProgramInfoLog(program, log_length, &log_length, info_log.data());

        glDeleteProgram(program);

        MANGO_LOG_ERROR("Program link failure : {0} !", info_log.data());
        return 0;
    }

    return program;
}
