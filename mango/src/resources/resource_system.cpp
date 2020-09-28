//! \file      resource_system.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <mango/log.hpp>
#include <mango/profile.hpp>
#include <resources/resource_system.hpp>
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE
#include <tiny_gltf.h>

using namespace mango;

static image load_image_from_file(const string& path, const image_configuration& configuration);

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

const shared_ptr<image> resource_system::get_image(const string& path, const image_configuration& configuration)
{
    PROFILE_ZONE;
    image img = load_image_from_file(path, configuration);
    return std::shared_ptr<image>(new image(img), [](image* img) {
        stbi_image_free(img->data);
        delete img;
    });
}

const shared_ptr<model> resource_system::get_gltf_model(const string& path, const model_configuration& configuration)
{
    PROFILE_ZONE;
    resource_handle handle = { configuration.name };

    model m;
    tinygltf::TinyGLTF loader;
    string err;
    string warn;
    auto ext = path.substr(path.find_last_of(".") + 1);
    bool ret = false;
    if (ext == "gltf")
        ret = loader.LoadASCIIFromFile(&m.gltf_model, &err, &warn, path);
    else if (ext == "glb")
        ret = loader.LoadBinaryFromFile(&m.gltf_model, &err, &warn, path);

    if (!warn.empty())
    {
        MANGO_LOG_WARN("Warning on loading gltf file {0}:\n {1}", path, warn);
    }

    if (!err.empty())
    {
        MANGO_LOG_ERROR("Error on loading gltf file {0}:\n {1}", path, err);
        return nullptr;
    }

    if (!ret)
    {
        MANGO_LOG_ERROR("Failed parsing gltf! Model is not valid!");
        return nullptr;
    }

    return std::make_shared<model>(m);
}

static image load_image_from_file(const string& path, const image_configuration& configuration)
{
    PROFILE_ZONE;
    image img;
    // stbi_set_flip_vertically_on_load(true); // This is usually needed for OpenGl

    int width = 0, height = 0, components = 0;
    if (!configuration.is_hdr)
    {
        img.bits            = 8;
        unsigned char* data = nullptr;

        if (stbi_is_16_bit(path.c_str()))
        {
            data = reinterpret_cast<unsigned char *>(stbi_load_16(path.c_str(), &width, &height, &components, 0));
            if (data)
            {
                img.bits = 16;
            }
        }
        if (!data)
            data = stbi_load(path.c_str(), &width, &height, &components, 0);

        if (!data)
        {
            MANGO_LOG_ERROR("Could not load image from path '{0}! Image resource not valid!", path);
            return img;
        }
        img.data = static_cast<void*>(data);
    }
    else
    {
        float* data = stbi_loadf(path.c_str(), &width, &height, &components, 0);

        if (!data)
        {
            MANGO_LOG_ERROR("Could not load image from path '{0}! Image resource not valid!", path);
            return img;
        }
        img.data = static_cast<void*>(data);
    }

    img.width             = width;
    img.height            = height;
    img.number_components = components;
    img.configuration     = configuration;

    return img;
}
