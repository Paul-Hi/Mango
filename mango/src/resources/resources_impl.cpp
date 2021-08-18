//! \file      resources_impl.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#include <mango/log.hpp>
#include <mango/profile.hpp>
#include <resources/resources_impl.hpp>
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE
#include <tiny_gltf.h>

using namespace mango;

resources_impl::resources_impl()
    : m_allocator(1073741824) // 1 GiB TODO Paul: Size???
{
    m_allocator.init();
}

resources_impl::~resources_impl()
{
    for (auto it = m_resource_cache.begin(); it != m_resource_cache.end();)
    {
        it = m_resource_cache.erase(it);
    }
    m_allocator.reset();
}

void resources_impl::update(float)
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

    // TODO Paul: Update resources on demand?
}

const image_resource* resources_impl::acquire(const image_resource_description& description)
{
    PROFILE_ZONE;
    resource_id res_id = resource_hash::get_id(description);
    auto cached        = m_resource_cache.find(res_id);
    if (cached == m_resource_cache.end())
    {
        image_resource* img = load_image_from_file(description);
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

void resources_impl::release(const image_resource* resource)
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
            m_resource_cache.erase(cached);
        }
    }
}

const model_resource* resources_impl::acquire(const model_resource_description& description)
{
    PROFILE_ZONE;
    resource_id res_id = resource_hash::get_id(description);
    auto cached        = m_resource_cache.find(res_id);
    if (cached == m_resource_cache.end())
    {
        model_resource* m = load_model_from_file(description);
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

void resources_impl::release(const model_resource* resource)
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
            m_resource_cache.erase(cached);
        }
    }
}

const shader_resource* resources_impl::acquire(const shader_resource_resource_description& description)
{
    PROFILE_ZONE;
    resource_id res_id = resource_hash::get_id(description);
    auto cached        = m_resource_cache.find(res_id);
    if (cached == m_resource_cache.end())
    {
        shader_resource* s = load_shader_from_file(description);
        if (!s)
            return nullptr;
        m_resource_cache.insert({ res_id, s });
        s->reference_count = 1;
        return s;
    }
    else
    {
        shader_resource* s = static_cast<shader_resource*>(cached->second);
        s->reference_count++;
        return s;
    }
}

void resources_impl::release(const shader_resource* resource)
{
    PROFILE_ZONE;
    auto cached = std::find_if(std::begin(m_resource_cache), std::end(m_resource_cache), [&resource](std::pair<resource_id, void*>&& r) { return r.second == (void*)resource; });
    if (cached == m_resource_cache.end())
    {
        return;
    }
    else
    {
        shader_resource* s = static_cast<shader_resource*>(cached->second);
        s->reference_count--;
        if (s->reference_count <= 0)
        {
            m_allocator.free_memory(static_cast<void*>(s));
            s->reference_count = 0;
            m_resource_cache.erase(cached);
        }
    }
}

image_resource* resources_impl::load_image_from_file(const image_resource_description& description)
{
    PROFILE_ZONE;

    image_resource* img = static_cast<image_resource*>(m_allocator.allocate(sizeof(image_resource)));

    int width = 0, height = 0, components = 0;
    int64 img_len = sizeof(uint8);
    if (!description.is_hdr)
    {
        img->bits           = 8;
        unsigned char* data = nullptr;

        if (stbi_is_16_bit(description.path))
        {
            data = reinterpret_cast<unsigned char*>(stbi_load_16(description.path, &width, &height, &components, 0));
            if (data)
            {
                img->bits = 16;
                img_len *= 2;
            }
        }
        if (!data)
            data = stbi_load(description.path, &width, &height, &components, 0);

        if (!data)
        {
            MANGO_LOG_ERROR("Could not load image from path '{0}! Image resource not valid!", description.path);
            return nullptr;
        }

        img_len *= width * height * components;
        img->data = m_allocator.allocate(img_len);
        std::copy(data, data + img_len, static_cast<uint8*>(img->data));
        stbi_image_free(data);
    }
    else
    {
        float* data = stbi_loadf(description.path, &width, &height, &components, 0);

        if (!data)
        {
            MANGO_LOG_ERROR("Could not load image from path '{0}! Image resource not valid!", description.path);
            return nullptr;
        }

        img_len   = width * height * components;
        img->bits = stbi_is_16_bit(description.path) ? 16 : 32;
        img->data = m_allocator.allocate(img_len);
        std::copy(data, data + img_len, static_cast<float*>(img->data));
        stbi_image_free(data);
    }

    img->width             = width;
    img->height            = height;
    img->number_components = components;
    img->description       = description;

    return img;
}

model_resource* resources_impl::load_model_from_file(const model_resource_description& description)
{
    PROFILE_ZONE;

    void* mem         = m_allocator.allocate(sizeof(model_resource));
    model_resource* m = new (mem) model_resource;

    tinygltf::TinyGLTF loader;
    string err;
    string warn;
    auto ext = string(description.path).substr(string(description.path).find_last_of(".") + 1);
    bool ret = false;
    if (ext == "gltf")
        ret = loader.LoadASCIIFromFile(&(m->gltf_model), &err, &warn, description.path);
    else if (ext == "glb")
        ret = loader.LoadBinaryFromFile(&(m->gltf_model), &err, &warn, description.path);

    if (!warn.empty())
    {
        MANGO_LOG_WARN("Warning on loading gltf file {0}:\n {1}", description.path, warn);
    }

    if (!err.empty())
    {
        MANGO_LOG_ERROR("Error on loading gltf file {0}:\n {1}", description.path, err);
        return nullptr;
    }

    if (!ret)
    {
        MANGO_LOG_ERROR("Failed parsing gltf! Model is not valid!");
        return nullptr;
    }

    return m;
}

shader_resource* resources_impl::load_shader_from_file(const shader_resource_resource_description& description)
{
    PROFILE_ZONE;

    void* mem          = m_allocator.allocate(sizeof(shader_resource));
    shader_resource* s = new (mem) shader_resource;

    s->description = description;

    s->source = "";

    s->source += "#version 430 core\n"; // version in first line is important!
    // insert defines
    for (shader_define def : description.defines)
    {
        s->source += "#define ";
        s->source += def.name;
        s->source += " ";
        s->source += def.value;
        s->source += "\n";
    }
    // reset line count
    s->source += "#line 1\n";

    s->source += load_shader_string_from_file(description.path, false);

    return s;
}

string resources_impl::load_shader_string_from_file(const string path, bool recursive)
{
    string source_string = "";

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

                // TODO Paul: Line count for included shaders is okay, but in error messages the compiled shader is shown and not the included one.
                // reset line count
                source_string += "#line 0\n";
                source_string += load_shader_string_from_file(new_path, true);
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
