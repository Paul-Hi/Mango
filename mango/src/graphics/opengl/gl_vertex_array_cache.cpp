//! \file      gl_vertex_array_cache.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#include <graphics/opengl/gl_vertex_array_cache.hpp>

using namespace mango;

//! \brief A \a gl_handle of an empty vertex array.
gl_handle empty_vao;

gl_vertex_array_cache::gl_vertex_array_cache()
{
    glCreateVertexArrays(1, &empty_vao);
}

gl_vertex_array_cache::~gl_vertex_array_cache()
{
    for (auto vao_handle : cache)
    {
        glDeleteVertexArrays(1, &vao_handle.second);
    }
    cache.clear();
    glDeleteVertexArrays(1, &empty_vao);
}

gl_handle gl_vertex_array_cache::get_vertex_array(const vertex_array_data_descriptor& desc)
{
    vertex_array_key key;
    vao_create_info create_info;

    if (desc.index_count)
    {
        MANGO_ASSERT(desc.index_buffer, "Index count > 0, but index buffer not provided!");
        MANGO_ASSERT(std::dynamic_pointer_cast<const gl_buffer>(*desc.index_buffer), "Buffer is not a gl_buffer!");
        auto ib          = static_gfx_handle_cast<const gl_buffer>(*desc.index_buffer);
        key.index_buffer = ib->get_uid();

        create_info.index_buffer_handle = ib->m_buffer_gl_handle;
    }

    for (int32 i = 0; i < desc.vertex_buffer_count; ++i)
    {
        MANGO_ASSERT(desc.vertex_buffers[i].buffer, "Vertex buffer not provided!");
        MANGO_ASSERT(std::dynamic_pointer_cast<const gl_buffer>(desc.vertex_buffers[i].buffer), "Buffer is not a gl_buffer!");
        auto vb                      = static_gfx_handle_cast<const gl_buffer>(desc.vertex_buffers[i].buffer);
        key.vertex_buffers[i].uid    = vb->get_uid();
        key.vertex_buffers[i].offset = desc.vertex_buffers[i].offset;
        key.binding_bitmask |= 1ULL << desc.vertex_buffers[i].binding; // TODO Paul: Check that.

        create_info.vertex_buffers[desc.vertex_buffers[i].binding].handle = vb->m_buffer_gl_handle;
        create_info.vertex_buffers[desc.vertex_buffers[i].binding].offset = desc.vertex_buffers[i].offset;
    }

    auto result = cache.find(key);

    if (result != cache.end())
        return result->second;

    MANGO_ASSERT(desc.vertex_buffer_count == desc.input_descriptor->binding_description_count, "Binding description and vertex buffer count are not equal!");

    for (int32 i = 0; i < desc.input_descriptor->binding_description_count; ++i)
    {
        create_info.vertex_buffers[desc.input_descriptor->binding_descriptions[i].binding].stride     = desc.input_descriptor->binding_descriptions[i].stride;
        create_info.vertex_buffers[desc.input_descriptor->binding_descriptions[i].binding].input_rate = desc.input_descriptor->binding_descriptions[i].input_rate;
    }

    gl_handle created = create(create_info, desc.input_descriptor);

    cache.insert({ key, created });

    return created;
}

gl_handle gl_vertex_array_cache::get_empty_vertex_array()
{
    return empty_vao;
}

gl_handle gl_vertex_array_cache::create(const vao_create_info& create_info, const vertex_input_descriptor* input_descriptor)
{
    gl_handle vertex_array;
    glCreateVertexArrays(1, &vertex_array);

    if (create_info.index_buffer_handle)
    {
        glVertexArrayElementBuffer(vertex_array, create_info.index_buffer_handle);
    }

    for (int32 i = 0; i < max_attached_vertex_buffers; ++i)
    {
        if (!create_info.vertex_buffers[i].handle)
            continue;

        glVertexArrayVertexBuffer(vertex_array, i, create_info.vertex_buffers[i].handle, create_info.vertex_buffers[i].offset, create_info.vertex_buffers[i].stride);

        glVertexArrayBindingDivisor(vertex_array, i, create_info.vertex_buffers[i].input_rate == gfx_vertex_input_rate::per_instance ? 1 : 0);
    }

    for (int32 i = 0; i < input_descriptor->attribute_description_count; ++i)
    {
        const int32& buffer_index          = input_descriptor->attribute_descriptions[i].binding;
        const int32& attribute_index       = input_descriptor->attribute_descriptions[i].location;
        const int32& relative_offset       = input_descriptor->attribute_descriptions[i].offset;
        const gfx_format& attribute_format = input_descriptor->attribute_descriptions[i].attribute_format;

        gl_enum type           = 0;
        int32 number_of_values = 0;
        bool normalized        = 0;
        gfx_format_to_gl_attribute_data(attribute_format, type, number_of_values, normalized);

        glEnableVertexArrayAttrib(vertex_array, attribute_index);
        glVertexArrayAttribFormat(vertex_array, attribute_index, number_of_values, type, normalized, relative_offset);
        glVertexArrayAttribBinding(vertex_array, attribute_index, buffer_index);
    }

    return vertex_array;
}
