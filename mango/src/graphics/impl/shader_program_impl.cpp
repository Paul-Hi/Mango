//! \file      shader_program_impl.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <graphics/impl/shader_program_impl.hpp>
#include <graphics/shader.hpp>

using namespace mango;

shader_program_impl::shader_program_impl()
{
    m_binding_data.listed_data.clear();
    m_name = glCreateProgram();
}

shader_program_impl::~shader_program_impl()
{
    glDeleteProgram(m_name);
    m_binding_data.listed_data.clear();
}

const uniform_binding_data& shader_program_impl::get_single_bindings()
{
    MANGO_ASSERT(is_created(), "Shader program not created!");
    if (!m_binding_data.listed_data.empty())
    {
        return m_binding_data;
    }

    g_int uniform_count = 0;
    glGetProgramiv(m_name, GL_ACTIVE_UNIFORMS, &uniform_count);

    if (uniform_count > 0)
    {
        g_int max_name_len = 0;
        g_sizei length     = 0;
        g_int size         = 0;
        g_enum type        = GL_NONE;
        glGetProgramiv(m_name, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max_name_len);

        auto uniform_name = std::vector<char>(max_name_len);

        for (g_int i = 0; i < uniform_count; ++i)
        {
            glGetActiveUniform(m_name, i, max_name_len, &length, &size, &type, uniform_name.data());

            uniform_binding_data::uniform u;
            g_uint location = glGetUniformLocation(m_name, uniform_name.data());
            u.type          = shader_resource_type_from_gl(type);

            m_binding_data.listed_data.insert({ location, u });
        }
    }

    return m_binding_data;
}

void shader_program_impl::create_graphics_pipeline_impl(shader_ptr vertex_shader, shader_ptr tess_control_shader, shader_ptr tess_eval_shader, shader_ptr geometry_shader, shader_ptr fragment_shader)
{
    MANGO_ASSERT(is_created(), "Shader program not created!");
    MANGO_ASSERT(vertex_shader, "Vertex shader is mandatory for a graphics pipeline!");
    MANGO_ASSERT(fragment_shader, "Fragment shader is mandatory for a graphics pipeline!");

    glAttachShader(m_name, vertex_shader->get_name());
    m_shaders.push_back(vertex_shader);

    if (tess_control_shader)
    {
        glAttachShader(m_name, tess_control_shader->get_name());
        m_shaders.push_back(tess_control_shader);
    }
    if (tess_eval_shader)
    {
        glAttachShader(m_name, tess_eval_shader->get_name());
        m_shaders.push_back(tess_eval_shader);
    }
    if (geometry_shader)
    {
        glAttachShader(m_name, geometry_shader->get_name());
        m_shaders.push_back(geometry_shader);
    }

    glAttachShader(m_name, fragment_shader->get_name());
    m_shaders.push_back(fragment_shader);

    link_program();
}

void shader_program_impl::create_compute_pipeline_impl(shader_ptr compute_shader)
{
    MANGO_ASSERT(is_created(), "Shader program not created!");
    MANGO_ASSERT(compute_shader, "Compute shader is mandatory for a compute pipeline!");

    glAttachShader(m_name, compute_shader->get_name());
    m_shaders.push_back(compute_shader);

    link_program();
}

void shader_program_impl::link_program()
{
    MANGO_ASSERT(is_created(), "Shader program not created and can not be linked!");

    glLinkProgram(m_name);

    g_int status = 0;
    glGetProgramiv(m_name, GL_LINK_STATUS, &status);
    if (GL_FALSE == status)
    {
        g_int log_length = 0;
        glGetProgramiv(m_name, GL_INFO_LOG_LENGTH, &log_length);
        std::vector<g_char> info_log(log_length);
        glGetProgramInfoLog(m_name, log_length, &log_length, info_log.data());

        glDeleteProgram(m_name);
        m_name = 0; // This is done, because we check if it is != 0 to make sure it is valid.

        MANGO_LOG_ERROR("Program link failure : {0} !", info_log.data());
        return;
    }
}
