//! \file      shader_system.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <fstream>
#include <glad/glad.h>
#include <mango/assert.hpp>
#include <mango/log.hpp>
#include <resources/shader_system.hpp>
#include <sstream>

using namespace mango;

static const string load_shader_source(const string& path);
static GLenum map_shader_type(const shader_type& type);
static gpu_resource_type get_uniform_type(const string& shader_name);

shader_system::shader_system(const shared_ptr<context_impl>& context)
    : m_shared_context(context)
{
}

shader_system::~shader_system() {}

bool shader_system::create()
{
    return true;
}

void shader_system::update(float dt)
{
    MANGO_UNUSED(dt);
}

void shader_system::destroy() {}

const shared_ptr<shader_program> shader_system::get_shader_program(const shader_program_configuration& configuration)
{
    // check if shader_program is cached
    auto it = m_shader_program_cache.find(configuration);
    if (it != m_shader_program_cache.end())
        return it->second;

    uint32 program_handle = glCreateProgram();
    shader_program program;
    program.handle = program_handle;
    // get all the shader_data
    uint32 shader_handles[8]; // TODO Paul: More than 8 shaders possible???
    uint32 shader_handle = 0;
    for (uint32 i = 0; i < configuration.pipeline_steps; ++i)
    {
        shader_configuration shader_config = configuration.shader_configs[i];
        const shared_ptr<shader_data> data = get_shader_data(shader_config);
        GLenum gl_type                     = map_shader_type(data->configuration.type);
        if (gl_type == GL_INVALID_ENUM)
        {
            MANGO_LOG_ERROR("Shader type is unknown. Can not create shader program!");
            return nullptr;
        }
        uint32 shader                 = glCreateShader(gl_type);
        const GLchar* source_c_string = data->source.c_str();
        glShaderSource(shader, 1, &source_c_string, 0);
        glCompileShader(shader);
        GLint compile_status = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);
        if (compile_status == GL_FALSE)
        {
            GLint max_log = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &max_log);

            std::vector<GLchar> info_log(max_log);
            glGetShaderInfoLog(shader, max_log, &max_log, &info_log[0]);

            glDeleteShader(shader);

            MANGO_LOG_ERROR("Shader link failure (idx: {0}):\n {1} Can not create shader program!", i, info_log.data());
            return nullptr;
        }

        glAttachShader(program_handle, shader);
        shader_handles[shader_handle++] = shader;
    }

    glLinkProgram(program_handle);

    GLint link_status = 0;
    glGetProgramiv(program_handle, GL_LINK_STATUS, &link_status);
    if (link_status == GL_FALSE)
    {
        GLint max_log = 0;
        glGetProgramiv(program_handle, GL_INFO_LOG_LENGTH, &max_log);

        std::vector<GLchar> info_log(max_log);
        glGetProgramInfoLog(program_handle, max_log, &max_log, &info_log[0]);

        glDeleteProgram(program_handle);

        for (uint32 i = 0; i < shader_handle; ++i)
            glDeleteShader(shader_handles[i]);

        MANGO_LOG_ERROR("Program link failure:\n {0} Can not create shader program!", info_log.data());
        return nullptr;
    }

    for (uint32 i = 0; i < shader_handle; ++i)
    {
        glDeleteShader(shader_handles[i]);

        shader_configuration shader_config = configuration.shader_configs[i];
        const shared_ptr<shader_data> data = get_shader_data(shader_config);
        populate_binding_data(program.binding_data, data->source, program_handle);
    }

    m_shader_program_cache.insert({ configuration, std::make_shared<shader_program>(program) });

    return m_shader_program_cache.at(configuration);
}

const shared_ptr<shader_data> shader_system::get_shader_data(const shader_configuration& configuration)
{
    // check if shader is cached
    auto it = m_shader_cache.find(configuration);
    if (it != m_shader_cache.end())
        return it->second;

    // load shader source
    shader_data data;
    data.configuration = configuration;
    data.source        = load_shader_source(configuration.path);
    m_shader_cache.insert({ configuration, std::make_shared<shader_data>(data) });

    return m_shader_cache.at(configuration);
}

void shader_system::populate_binding_data(std::unordered_map<string, std::pair<gpu_resource_type, uint32>>& binding_data, const string& source, uint32 program)
{
    // basic uniforms
    auto next = source.find("uniform");
    while (next != source.npos)
    {
        next += 7;
        string info = source.substr(next, source.find(";", next) - next);
        std::istringstream iss(info);
        std::vector<std::string> parts((std::istream_iterator<std::string>(iss)), std::istream_iterator<std::string>());
        MANGO_ASSERT(parts.size() >= 2, "Invalid uniform!");
        gpu_resource_type resource_type = get_uniform_type(parts.at(0));
        if (resource_type == gpu_unknown)
        {
            next = source.find("uniform", next);
            continue;
        }
        int32 location = glGetUniformLocation(program, parts.at(1).c_str());
        if (location < 0)
        {
            next = source.find("uniform", next);
            continue;
        }
        MANGO_LOG_DEBUG("Uniform {0} of type {1} at location {2}.", parts.at(1), parts.at(0), location);
        binding_data.insert({ parts.at(1), { resource_type, static_cast<uint32>(location) } });
        next = source.find("uniform", next);
    }
}

static const string load_shader_source(const string& path)
{
    string text = "";
    std::ifstream input_stream(path, std::ios::in | std::ios::binary);
    if (input_stream)
    {
        input_stream.seekg(0, std::ios::end);
        text.resize(static_cast<uint32_t>(input_stream.tellg()));
        input_stream.seekg(0, std::ios::beg);
        input_stream.read(&text[0], text.size());
        input_stream.close();
    }
    return text;
}

static GLenum map_shader_type(const shader_type& type)
{
    switch (type)
    {
    case vertex_shader:
        return GL_VERTEX_SHADER;
    case fragment_shader:
        return GL_FRAGMENT_SHADER;
    case geometry_shader:
        return GL_GEOMETRY_SHADER;
    default:
        return GL_INVALID_ENUM;
    }
}

static gpu_resource_type get_uniform_type(const string& shader_name)
{
    if (shader_name == "float")
        return gpu_float;
    if (shader_name == "vec2")
        return gpu_vec2;
    if (shader_name == "vec3")
        return gpu_vec3;
    if (shader_name == "vec4")
        return gpu_vec4;
    if (shader_name == "int")
        return gpu_int;
    if (shader_name == "ivec2")
        return gpu_ivec2;
    if (shader_name == "ivec3")
        return gpu_ivec3;
    if (shader_name == "ivec4")
        return gpu_ivec4;
    if (shader_name == "mat3")
        return gpu_mat3;
    if (shader_name == "mat4")
        return gpu_mat4;
    if (shader_name == "sampler2D")
        return gpu_sampler_texture_2d;
    if (shader_name == "samplerCube")
        return gpu_sampler_texture_cube;

    MANGO_LOG_ERROR("Unknown uniform type: {0}.", shader_name);
    return gpu_unknown;
}
