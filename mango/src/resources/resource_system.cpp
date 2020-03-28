//! \file      resource_system.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <glad/glad.h>
#include <mango/log.hpp>
#include <resources/resource_system.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace mango;

static uint32 load_texture_from_file(const string& path, const texture_configuration& configuration);

resource_system::resource_system(const shared_ptr<context_impl>& context)
    : m_shared_context(context)
{
}

resource_system::~resource_system() {}

bool resource_system::create()
{
    return true;
}

void resource_system::update(float dt)
{
    MANGO_UNUSED(dt);
}

void resource_system::destroy() {}

const texture* resource_system::load_texture(const string& path, const texture_configuration& configuration)
{
    resource_configuration config = { configuration.name };
    // check if texture is already loaded
    auto it = m_texture_storage.find(config);
    if (it != m_texture_storage.end())
    {
        MANGO_LOG_INFO("Texture '{0}' is already loaded!", configuration.name);
        return &(it->second);
    }

    texture tex;
    tex.handle = load_texture_from_file(path, configuration);
    std::pair<resource_configuration, texture> pair(config, tex);
    m_texture_storage.insert(pair);
    it = m_texture_storage.find(config);
    return &(it->second);
}

const texture* resource_system::get_texture(const string& name)
{
    resource_configuration config = { name };
    // check if texture is loaded
    auto it = m_texture_storage.find(config);
    if (it != m_texture_storage.end())
        return &(it->second);

    MANGO_LOG_ERROR("Texture '{0}' is not loaded!", name);
    return nullptr;
}

static int wrap_parameter(const texture_parameter& wrapping)
{
    switch (wrapping)
    {
    case wrap_repeat:
        return GL_REPEAT;
    case wrap_clamp_to_edge:
        return GL_CLAMP_TO_EDGE;
    case wrap_clamp_to_border:
        return GL_CLAMP_TO_BORDER;
    default:
        MANGO_LOG_ERROR("Unknown texture wrap parameter.");
    }
    return -1;
}

static int filter_parameter(const texture_parameter& filtering)
{
    switch (filtering)
    {
    case filter_nearest:
        return GL_NEAREST;
    case filter_linear:
        return GL_LINEAR;
    case filter_nearest_mipmap_nearest:
        return GL_NEAREST_MIPMAP_NEAREST;
    case filter_linear_mipmap_nearest:
        return GL_LINEAR_MIPMAP_NEAREST;
    case filter_nearest_mipmap_linear:
        return GL_NEAREST_MIPMAP_LINEAR;
    case filter_linear_mipmap_linear:
        return GL_LINEAR_MIPMAP_LINEAR;
    default:
        MANGO_LOG_ERROR("Unknown texture filter parameter.");
    }
    return -1;
}

static uint32 load_texture_from_file(const string& path, const texture_configuration& configuration)
{
    stbi_set_flip_vertically_on_load(true); // This is usually needed for OpenGl

    int width = 0, height = 0, components = 0;

    unsigned char* data = stbi_load(path.c_str(), &width, &height, &components, STBI_rgb);

    if (!data)
    {
        MANGO_LOG_ERROR("Could not load texture from path '{0}! Texture not valid!", path);
        return 0;
    }

    uint32 handle;
    glGenTextures(1, &handle);
    glBindTexture(GL_TEXTURE_2D, handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_parameter(configuration.texture_wrap_s));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_parameter(configuration.texture_wrap_t));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter_parameter(configuration.texture_min_filter));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter_parameter(configuration.texture_mag_filter));
    uint32 internal_format = 0;
    uint32 format          = GL_RGB;

    if (components == 1)
    {
        internal_format = GL_R8;
    }
    else if (components == 3)
    {
        internal_format = configuration.is_standard_color_space ? GL_SRGB8 : GL_RGB8;
    }
    else if (components == 4)
    {
        internal_format = configuration.is_standard_color_space ? GL_SRGB8_ALPHA8 : GL_RGBA8;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    if (configuration.generate_mipmaps)
        glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);

    glBindTexture(GL_TEXTURE_2D, 0);

    return handle;
}
