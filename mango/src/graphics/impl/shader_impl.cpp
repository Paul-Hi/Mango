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
    : m_path(configuration.path)
    , m_type(configuration.type)
    , m_defines(configuration.defines)
{
    string source_string = load_from_file(m_path, false);

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

        MANGO_LOG_ERROR("Shader link failure : {0} with {1} !", m_path, info_log.data());
        MANGO_LOG_DEBUG(source_c_string);
        return;
    }
}

string shader_impl::load_from_file(const string path, bool recursive)
{
    // load the shader from the source file
    string source_string = "";

    if (!recursive)
    {
        source_string += "#version 430 core\n"; // version in first line is important!
        // insert defines
        for (shader_define def : m_defines)
        {
            source_string += "#define ";
            source_string += def.name;
            source_string += " ";
            source_string += def.value;
            source_string += "\n";
        }
        // reset line count
        source_string += "#line 1\n";
    }

    // incl. recursive includes
    string include_id = "#include <";
    std::ifstream input_stream;
    input_stream.open(path, std::ios::in | std::ios::binary);

    // retrieving the current folder path, because include is relative.
    auto path_end      = path.find_last_of("/\\");
    string folder_path = path.substr(0, path_end + 1);

    if (input_stream.is_open())
    {
        string line;
        int32 line_nr = 1;
        while (getline(input_stream, line))
        {
            auto offset = line.find(include_id);
            if (offset != string::npos)
            {
                auto include_end = line.find_first_of(">");
                if (include_end == string::npos)
                {
                    MANGO_LOG_ERROR("Including shader file failed: {0} !", line);
                    return source_string;
                }

                string new_path = folder_path + line.substr(offset + include_id.size(), include_end - (offset + include_id.size()));

                // reset line count
                source_string += "#line 0\n";
                source_string += load_from_file(new_path, true);
                // reset line count
                source_string += "#line " + std::to_string(++line_nr) + "\n";

                continue;
            }

            source_string += line + "\n";
            line_nr++;
        }

        if (!recursive)
            source_string += "\0";

        input_stream.close();
        return source_string;
    }
    else
    {
        MANGO_LOG_ERROR("Opening shader file failed: {0} !", path);
        return source_string;
    }
}

shader_impl::~shader_impl()
{
    glDeleteShader(m_name);
}
