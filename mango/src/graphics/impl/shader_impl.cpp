//! \file      shader_impl.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <fstream>
#include <glad/glad.h>
#include <graphics/impl/shader_impl.hpp>

using namespace mango;

shader_impl::shader_impl(const shader_configuration& configuration)
    : m_path(configuration.m_path)
    , m_type(configuration.m_type)
{
    // load the shader from the source file
    string source_string = "";
    std::ifstream input_stream;
    input_stream.open(m_path, std::ios::in | std::ios::binary);

    if (input_stream.is_open())
    {
        input_stream.seekg(0, std::ios::end);
        source_string.resize(static_cast<uint32_t>(input_stream.tellg()));
        input_stream.seekg(0, std::ios::beg);
        input_stream.read(&source_string[0], source_string.size());
        input_stream.close();
    }
    else
    {
        MANGO_LOG_ERROR("Opening shader file failed: {0} !");
    }

    m_name                        = glCreateShader(shader_type_to_gl(m_type));
    const g_char* source_c_string = source_string.c_str();
    // MANGO_LOG_DEBUG(source_c_string);
    glShaderSource(m_name, 1, &source_c_string, 0);
    glCompileShader(m_name);
    g_int status = 0;
    glGetShaderiv(m_name, GL_COMPILE_STATUS, &status);
    if (GL_FALSE == status)
    {
        g_int log_length = 0;
        glGetShaderiv(m_name, GL_INFO_LOG_LENGTH, &log_length);
        std::vector<g_char> info_log(log_length);
        glGetShaderInfoLog(m_name, log_length, &log_length, info_log.data());

        glDeleteShader(m_name);
        m_name = 0; // This is done, because we check if it is != 0 to make sure it is valid.

        MANGO_LOG_ERROR("Shader link failure : {0} !", info_log.data());
        return;
    }
}

shader_impl::~shader_impl()
{
    glDeleteShader(m_name);
}
