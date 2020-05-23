//! \file      resource_system.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <mango/log.hpp>
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

void resource_system::destroy()
{
    for (auto sp : m_image_storage)
    {
        stbi_image_free(sp.second->data);
    }
    m_image_storage.clear();
    m_model_storage.clear();
}

const shared_ptr<image> resource_system::load_image(const string& path, const image_configuration& configuration)
{
    resource_handle handle = { configuration.name };
    // check if image is already loaded
    auto it = m_image_storage.find(handle);
    if (it != m_image_storage.end())
    {
        MANGO_LOG_INFO("Image '{0}' is already loaded!", configuration.name);
        return it->second;
    }

    image img = load_image_from_file(path, configuration);
    m_image_storage.insert({ handle, std::make_shared<image>(img) });
    return m_image_storage.at(handle);
}

const shared_ptr<image> resource_system::get_image(const string& name)
{
    resource_handle handle = { name };
    // check if image is loaded
    auto it = m_image_storage.find(handle);
    if (it != m_image_storage.end())
        return it->second;

    MANGO_LOG_ERROR("Image '{0}' is not loaded!", name);
    return nullptr;
}

const shared_ptr<model> resource_system::load_gltf(const string& path, const model_configuration& configuration)
{
    resource_handle handle = { configuration.name };
    // check if model is already loaded
    auto it = m_model_storage.find(handle);
    if (it != m_model_storage.end())
    {
        MANGO_LOG_INFO("Model '{0}' is already loaded!", configuration.name);
        return it->second;
    }

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
    }

    if (!ret)
    {
        MANGO_LOG_ERROR("Failed parsing gltf! Model is not valid!");
        return nullptr;
    }

    m.configuration = configuration;

    m_model_storage.insert({ handle, std::make_shared<model>(m) });

    return m_model_storage.at(handle);
}

const shared_ptr<model> resource_system::get_gltf_model(const string& name)
{
    resource_handle handle = { name };
    // check if model is loaded
    auto it = m_model_storage.find(handle);
    if (it != m_model_storage.end())
        return it->second;

    MANGO_LOG_ERROR("Model '{0}' is not loaded!", name);
    return nullptr;
}

static image load_image_from_file(const string& path, const image_configuration& configuration)
{
    image img;
    // stbi_set_flip_vertically_on_load(true); // This is usually needed for OpenGl

    int width = 0, height = 0, components = 0;
    if (!configuration.is_hdr)
    {
        unsigned char* data = stbi_load(path.c_str(), &width, &height, &components, STBI_rgb_alpha);

        if (!data)
        {
            MANGO_LOG_ERROR("Could not load image from path '{0}! Image resource not valid!", path);
            return img;
        }
        img.data = static_cast<void*>(data);
    }
    else
    {
        float* data = stbi_loadf(path.c_str(), &width, &height, &components, STBI_rgb_alpha);

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
