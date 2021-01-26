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

resource_system::resource_system(const shared_ptr<context_impl>& context)
    : m_shared_context(context)
    , m_allocator(1073741824) // 1 GiB TODO Paul: Size???
{
    m_allocator.init();
}

resource_system::~resource_system() {}

bool resource_system::create()
{
    return true;
}

void resource_system::update(float)
{
    // release unused resources (ref count == 0).
    for (auto it = m_resource_cache.begin(); it != m_resource_cache.end();)
    {
        auto res = static_cast<resource_base*>(it->second);
        if (!res->reference_count)
        {
            it = m_resource_cache.erase(it);
        }
        else
            it++;
    }
}

void resource_system::destroy()
{
    for (auto it = m_resource_cache.begin(); it != m_resource_cache.end();)
    {
        it = m_resource_cache.erase(it);
    }
    m_allocator.reset();
}

const image_resource* resource_system::acquire(const image_resource_configuration& configuration)
{
    PROFILE_ZONE;
    resource_id res_id = resource_hash::get_id(configuration);
    auto cached        = m_resource_cache.find(res_id);
    if (cached == m_resource_cache.end())
    {
        image_resource* img = load_image_from_file(configuration);
        if (!img)
            return nullptr;
        m_resource_cache.insert({ res_id, img });
        img->reference_count = 1;
        return img;
    }
    else
    {
        image_resource* img = static_cast<image_resource*>(cached->second);
        img->reference_count++;
        return img;
    }
}

void resource_system::release(const image_resource* resource)
{
    PROFILE_ZONE;
    auto cached = std::find_if(std::begin(m_resource_cache), std::end(m_resource_cache), [&resource](std::pair<resource_id, void*>&& r) { return r.second == (void*)resource; });
    if (cached == m_resource_cache.end())
    {
        return;
    }
    else
    {
        image_resource* img = static_cast<image_resource*>(cached->second);
        img->reference_count--;
        if (img->reference_count <= 0)
        {
            m_allocator.free_memory(static_cast<void*>(img->data));
            m_allocator.free_memory(static_cast<void*>(img));
            img->reference_count = 0;
        }
    }
}

const model_resource* resource_system::acquire(const model_resource_configuration& configuration)
{
    PROFILE_ZONE;
    resource_id res_id = resource_hash::get_id(configuration);
    auto cached        = m_resource_cache.find(res_id);
    if (cached == m_resource_cache.end())
    {
        model_resource* m = load_model_from_file(configuration);
        if (!m)
            return nullptr;
        m_resource_cache.insert({ res_id, m });
        m->reference_count = 1;
        return m;
    }
    else
    {
        model_resource* m = static_cast<model_resource*>(cached->second);
        m->reference_count++;
        return m;
    }
}

void resource_system::release(const model_resource* resource)
{
    PROFILE_ZONE;
    auto cached = std::find_if(std::begin(m_resource_cache), std::end(m_resource_cache), [&resource](std::pair<resource_id, void*>&& r) { return r.second == (void*)resource; });
    if (cached == m_resource_cache.end())
    {
        return;
    }
    else
    {
        model_resource* m = static_cast<model_resource*>(cached->second);
        m->reference_count--;
        if (m->reference_count <= 0)
        {
            m_allocator.free_memory(static_cast<void*>(m));
            m->reference_count = 0;
        }
    }
}

image_resource* resource_system::load_image_from_file(const image_resource_configuration& configuration)
{
    PROFILE_ZONE;

    image_resource* img = static_cast<image_resource*>(m_allocator.allocate(sizeof(image_resource)));
    // stbi_set_flip_vertically_on_load(true); // This is usually needed for OpenGl

    int width = 0, height = 0, components = 0;
    int64 img_len = sizeof(uint8);
    if (!configuration.is_hdr)
    {
        img->bits           = 8;
        unsigned char* data = nullptr;

        if (stbi_is_16_bit(configuration.path))
        {
            data = reinterpret_cast<unsigned char*>(stbi_load_16(configuration.path, &width, &height, &components, 0));
            if (data)
            {
                img->bits = 16;
                img_len *= 2;
            }
        }
        if (!data)
            data = stbi_load(configuration.path, &width, &height, &components, 0);

        img_len *= width * height * components;

        if (!data)
        {
            MANGO_LOG_ERROR("Could not load image from path '{0}! Image resource not valid!", configuration.path);
            return nullptr;
        }

        img->data = m_allocator.allocate(img_len);
        memcpy(img->data, (void*)data, img_len);
        stbi_image_free(data);
    }
    else
    {
        float* data = stbi_loadf(configuration.path, &width, &height, &components, 0);

        if (!data)
        {
            MANGO_LOG_ERROR("Could not load image from path '{0}! Image resource not valid!", configuration.path);
            return nullptr;
        }

        img_len = width * height * components * sizeof(float);
        img->data     = m_allocator.allocate(img_len);
        memcpy(img->data, (void*)data, img_len);
        stbi_image_free(data);
    }

    img->width             = width;
    img->height            = height;
    img->number_components = components;
    img->configuration     = configuration;

    return img;
}

model_resource* resource_system::load_model_from_file(const model_resource_configuration& configuration)
{
    PROFILE_ZONE;

    void* mem         = m_allocator.allocate(sizeof(model_resource));
    model_resource* m = new (mem) model_resource;

    tinygltf::TinyGLTF loader;
    string err;
    string warn;
    auto ext = string(configuration.path).substr(string(configuration.path).find_last_of(".") + 1);
    bool ret = false;
    if (ext == "gltf")
        ret = loader.LoadASCIIFromFile(&(m->gltf_model), &err, &warn, configuration.path);
    else if (ext == "glb")
        ret = loader.LoadBinaryFromFile(&(m->gltf_model), &err, &warn, configuration.path);

    if (!warn.empty())
    {
        MANGO_LOG_WARN("Warning on loading gltf file {0}:\n {1}", configuration.path, warn);
    }

    if (!err.empty())
    {
        MANGO_LOG_ERROR("Error on loading gltf file {0}:\n {1}", configuration.path, err);
        return nullptr;
    }

    if (!ret)
    {
        MANGO_LOG_ERROR("Failed parsing gltf! Model is not valid!");
        return nullptr;
    }

    return m;
}
