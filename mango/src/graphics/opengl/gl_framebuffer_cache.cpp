//! \file      gl_framebuffer_cache.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#include <graphics/opengl/gl_framebuffer_cache.hpp>

using namespace mango;

gl_framebuffer_cache::gl_framebuffer_cache() {}

gl_framebuffer_cache::~gl_framebuffer_cache()
{
    for (auto fb_handle : cache)
    {
        glDeleteFramebuffers(1, &fb_handle.second);
    }
    cache.clear();
}

gl_handle gl_framebuffer_cache::get_framebuffer(int32 count, gfx_handle<const gfx_texture>* render_targets, gfx_handle<const gfx_texture> depth_stencil_target)
{
    framebuffer_key key;

    framebuffer_create_info create_info;

    key.attachment_count          = count + (depth_stencil_target != nullptr);
    create_info.color_attachments = count;

    for (int32 i = 0; i < count; ++i)
    {
        MANGO_ASSERT(std::dynamic_pointer_cast<const gl_texture>(render_targets[i]), "Texture is not a gl_texture!");
        auto tex            = static_gfx_handle_cast<const gl_texture>(render_targets[i]);
        key.texture_uids[i] = tex->get_uid();

        create_info.handles[i] = tex->m_texture_gl_handle;
    }

    if (depth_stencil_target != nullptr)
    {
        MANGO_ASSERT(std::dynamic_pointer_cast<const gl_texture>(depth_stencil_target), "Texture is not a gl_texture!");
        auto tex                = static_gfx_handle_cast<const gl_texture>(depth_stencil_target);
        key.texture_uids[count] = tex->get_uid();

        // early check
        auto result = cache.find(key);

        if (result != cache.end())
            return result->second;

        create_info.handles[count] = tex->m_texture_gl_handle;

        const gfx_format& internal = tex->m_info.texture_format;

        // TODO Paul: Pure stencil not supported.

        switch (internal)
        {
        case gfx_format::depth24_stencil8:
        case gfx_format::depth32f_stencil8:
            create_info.depth_stencil_attachment++;
            break;
        case gfx_format::depth_component32f:
        case gfx_format::depth_component16:
        case gfx_format::depth_component24:
        case gfx_format::depth_component32:
            create_info.depth_attachment++;
            break;
        default:
            MANGO_ASSERT(false, "Depth Stencil Target has no valid format!");
            break;
        }
    }
    else
    {
        auto result = cache.find(key);

        if (result != cache.end())
            return result->second;
    }

    gl_handle created = create(create_info);

    cache.insert({ key, created });

    return created;
}

gl_handle gl_framebuffer_cache::create(const framebuffer_create_info& create_info)
{
    gl_handle framebuffer;
    glCreateFramebuffers(1, &framebuffer);

    gl_enum draw_buffers[max_render_targets];

    for (int32 i = 0; i < create_info.color_attachments; ++i)
    {
        glNamedFramebufferTexture(framebuffer, GL_COLOR_ATTACHMENT0 + i, create_info.handles[i], 0);
        draw_buffers[i] = GL_COLOR_ATTACHMENT0 + i;
    }

    // TODO Paul: Assuming correct order here.
    if (create_info.depth_stencil_attachment)
    {
        glNamedFramebufferTexture(framebuffer, GL_DEPTH_STENCIL_ATTACHMENT, create_info.handles[create_info.color_attachments], 0);
    }

    if (create_info.depth_attachment)
    {
        glNamedFramebufferTexture(framebuffer, GL_DEPTH_ATTACHMENT, create_info.handles[create_info.color_attachments], 0);
    }

    // TODO Paul: Pure stencil not supported.
    // if (create_info.stencil_attachment)
    // {
    //     glNamedFramebufferTexture(framebuffer, GL_STENCIL_ATTACHMENT, m_stencil_attachment->get_name(), 0);
    // }

    glNamedFramebufferDrawBuffers(framebuffer, create_info.color_attachments, &draw_buffers[0]);

    if (gl_enum status = glCheckNamedFramebufferStatus(framebuffer, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        MANGO_LOG_ERROR("Framebuffer {0} is incomplete! Status: {1}.", framebuffer, status);
        MANGO_UNUSED(status);
    }

    return framebuffer;
}
