//! \file      framebuffer_impl.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <glad/glad.h>
#include <graphics/impl/framebuffer_impl.hpp>
#include <graphics/texture.hpp>

using namespace mango;

framebuffer_impl::framebuffer_impl(const framebuffer_configuration& configuration)
    : m_width(configuration.m_width)
    , m_height(configuration.m_height)
    , m_color_attachment0(configuration.m_color_attachment0)
    , m_color_attachment1(configuration.m_color_attachment1)
    , m_color_attachment2(configuration.m_color_attachment2)
    , m_color_attachment3(configuration.m_color_attachment3)
    , m_depth_attachment(configuration.m_depth_attachment)
    , m_stencil_attachment(configuration.m_stencil_attachment)
    , m_depth_stencil_attachment(configuration.m_depth_stencil_attachment)
{
    glCreateFramebuffers(1, &m_name);
    m_draw_buffers.clear();

    if (nullptr != m_color_attachment0)
    {
        glNamedFramebufferTexture(m_name, GL_COLOR_ATTACHMENT0, m_color_attachment0->get_name(), 0);
        m_draw_buffers.push_back(GL_COLOR_ATTACHMENT0);
    }
    if (nullptr != m_color_attachment1)
    {
        glNamedFramebufferTexture(m_name, GL_COLOR_ATTACHMENT1, m_color_attachment1->get_name(), 0);
        m_draw_buffers.push_back(GL_COLOR_ATTACHMENT1);
    }
    if (nullptr != m_color_attachment2)
    {
        glNamedFramebufferTexture(m_name, GL_COLOR_ATTACHMENT2, m_color_attachment2->get_name(), 0);
        m_draw_buffers.push_back(GL_COLOR_ATTACHMENT2);
    }
    if (nullptr != m_color_attachment3)
    {
        glNamedFramebufferTexture(m_name, GL_COLOR_ATTACHMENT3, m_color_attachment3->get_name(), 0);
        m_draw_buffers.push_back(GL_COLOR_ATTACHMENT3);
    }
    if (nullptr != m_depth_attachment)
    {
        glNamedFramebufferTexture(m_name, GL_DEPTH_ATTACHMENT, m_depth_attachment->get_name(), 0);
    }
    if (nullptr != m_stencil_attachment)
    {
        glNamedFramebufferTexture(m_name, GL_STENCIL_ATTACHMENT, m_stencil_attachment->get_name(), 0);
    }
    if (nullptr != m_depth_stencil_attachment)
    {
        glNamedFramebufferTexture(m_name, GL_DEPTH_STENCIL_ATTACHMENT, m_depth_stencil_attachment->get_name(), 0);
    }

    glNamedFramebufferDrawBuffers(m_name, static_cast<g_sizei>(m_draw_buffers.size()), m_draw_buffers.data());

    if (g_enum status = glCheckNamedFramebufferStatus(m_name, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        MANGO_LOG_ERROR("Framebuffer {0} is incomplete! Status: {1}.", m_name, status);
        MANGO_UNUSED(status);
    }
}

framebuffer_impl::~framebuffer_impl()
{
    MANGO_ASSERT(is_created(), "Framebuffer not created!");
    glDeleteFramebuffers(1, &m_name);
}

void framebuffer_impl::resize(int32 width, int32 height)
{
    MANGO_ASSERT(width > 0, "Invalid framebuffer width!");
    MANGO_ASSERT(height > 0, "Invalid framebuffer height!");
    MANGO_ASSERT(is_created(), "Framebuffer not created!");
    m_width  = width;
    m_height = height;
    // TODO Paul: Is there a cleaner way to do that?
    texture_configuration config;
    if (nullptr != m_color_attachment0)
    {
        config.m_generate_mipmaps        = m_color_attachment0->mipmaps();
        config.m_is_standard_color_space = m_color_attachment0->is_in_standard_color_space();
        config.m_texture_min_filter      = m_color_attachment0->min_filter();
        config.m_texture_mag_filter      = m_color_attachment0->mag_filter();
        config.m_texture_wrap_s          = m_color_attachment0->wrap_s();
        config.m_texture_wrap_t          = m_color_attachment0->wrap_t();
        config.m_layers                  = m_color_attachment0->layers();
        auto internal                    = m_color_attachment0->get_internal_format();
        auto form                        = m_color_attachment0->get_format();
        auto c_type                      = m_color_attachment0->component_type();
        m_color_attachment0->release();
        m_color_attachment0 = texture::create(config);
        m_color_attachment0->set_data(internal, width, height, form, c_type, nullptr);
        glNamedFramebufferTexture(m_name, GL_COLOR_ATTACHMENT0, m_color_attachment0->get_name(), 0);
    }
    if (nullptr != m_color_attachment1)
    {
        config.m_generate_mipmaps        = m_color_attachment1->mipmaps();
        config.m_is_standard_color_space = m_color_attachment1->is_in_standard_color_space();
        config.m_texture_min_filter      = m_color_attachment1->min_filter();
        config.m_texture_mag_filter      = m_color_attachment1->mag_filter();
        config.m_texture_wrap_s          = m_color_attachment1->wrap_s();
        config.m_texture_wrap_t          = m_color_attachment1->wrap_t();
        config.m_layers                  = m_color_attachment1->layers();
        auto internal                    = m_color_attachment1->get_internal_format();
        auto form                        = m_color_attachment1->get_format();
        auto c_type                      = m_color_attachment1->component_type();
        m_color_attachment1->release();
        m_color_attachment1 = texture::create(config);
        m_color_attachment1->set_data(internal, width, height, form, c_type, nullptr);
        glNamedFramebufferTexture(m_name, GL_COLOR_ATTACHMENT1, m_color_attachment1->get_name(), 0);
    }
    if (nullptr != m_color_attachment2)
    {
        config.m_generate_mipmaps        = m_color_attachment2->mipmaps();
        config.m_is_standard_color_space = m_color_attachment2->is_in_standard_color_space();
        config.m_texture_min_filter      = m_color_attachment2->min_filter();
        config.m_texture_mag_filter      = m_color_attachment2->mag_filter();
        config.m_texture_wrap_s          = m_color_attachment2->wrap_s();
        config.m_texture_wrap_t          = m_color_attachment2->wrap_t();
        config.m_layers                  = m_color_attachment2->layers();
        auto internal                    = m_color_attachment2->get_internal_format();
        auto form                        = m_color_attachment2->get_format();
        auto c_type                      = m_color_attachment2->component_type();
        m_color_attachment2->release();
        m_color_attachment2 = texture::create(config);
        m_color_attachment2->set_data(internal, width, height, form, c_type, nullptr);
        glNamedFramebufferTexture(m_name, GL_COLOR_ATTACHMENT2, m_color_attachment2->get_name(), 0);
    }
    if (nullptr != m_color_attachment3)
    {
        config.m_generate_mipmaps        = m_color_attachment3->mipmaps();
        config.m_is_standard_color_space = m_color_attachment3->is_in_standard_color_space();
        config.m_texture_min_filter      = m_color_attachment3->min_filter();
        config.m_texture_mag_filter      = m_color_attachment3->mag_filter();
        config.m_texture_wrap_s          = m_color_attachment3->wrap_s();
        config.m_texture_wrap_t          = m_color_attachment3->wrap_t();
        config.m_layers                  = m_color_attachment3->layers();
        auto internal                    = m_color_attachment3->get_internal_format();
        auto form                        = m_color_attachment3->get_format();
        auto c_type                      = m_color_attachment3->component_type();
        m_color_attachment3->release();
        m_color_attachment3 = texture::create(config);
        m_color_attachment3->set_data(internal, width, height, form, c_type, nullptr);
        glNamedFramebufferTexture(m_name, GL_COLOR_ATTACHMENT3, m_color_attachment3->get_name(), 0);
    }
    if (nullptr != m_depth_attachment)
    {
        config.m_generate_mipmaps        = m_depth_attachment->mipmaps();
        config.m_is_standard_color_space = m_depth_attachment->is_in_standard_color_space();
        config.m_texture_min_filter      = m_depth_attachment->min_filter();
        config.m_texture_mag_filter      = m_depth_attachment->mag_filter();
        config.m_texture_wrap_s          = m_depth_attachment->wrap_s();
        config.m_texture_wrap_t          = m_depth_attachment->wrap_t();
        config.m_layers                  = m_depth_attachment->layers();
        auto internal                    = m_depth_attachment->get_internal_format();
        auto form                        = m_depth_attachment->get_format();
        auto c_type                      = m_depth_attachment->component_type();
        m_depth_attachment->release();
        m_depth_attachment = texture::create(config);
        m_depth_attachment->set_data(internal, width, height, form, c_type, nullptr);
        glNamedFramebufferTexture(m_name, GL_DEPTH_ATTACHMENT, m_depth_attachment->get_name(), 0);
    }
    if (nullptr != m_stencil_attachment)
    {
        config.m_generate_mipmaps        = m_stencil_attachment->mipmaps();
        config.m_is_standard_color_space = m_stencil_attachment->is_in_standard_color_space();
        config.m_texture_min_filter      = m_stencil_attachment->min_filter();
        config.m_texture_mag_filter      = m_stencil_attachment->mag_filter();
        config.m_texture_wrap_s          = m_stencil_attachment->wrap_s();
        config.m_texture_wrap_t          = m_stencil_attachment->wrap_t();
        config.m_layers                  = m_stencil_attachment->layers();
        auto internal                    = m_stencil_attachment->get_internal_format();
        auto form                        = m_stencil_attachment->get_format();
        auto c_type                      = m_stencil_attachment->component_type();
        m_stencil_attachment->release();
        m_stencil_attachment = texture::create(config);
        m_stencil_attachment->set_data(internal, width, height, form, c_type, nullptr);
        glNamedFramebufferTexture(m_name, GL_STENCIL_ATTACHMENT, m_stencil_attachment->get_name(), 0);
    }
    if (nullptr != m_depth_stencil_attachment)
    {
        config.m_generate_mipmaps        = m_depth_stencil_attachment->mipmaps();
        config.m_is_standard_color_space = m_depth_stencil_attachment->is_in_standard_color_space();
        config.m_texture_min_filter      = m_depth_stencil_attachment->min_filter();
        config.m_texture_mag_filter      = m_depth_stencil_attachment->mag_filter();
        config.m_texture_wrap_s          = m_depth_stencil_attachment->wrap_s();
        config.m_texture_wrap_t          = m_depth_stencil_attachment->wrap_t();
        config.m_layers                  = m_depth_stencil_attachment->layers();
        auto internal                    = m_depth_stencil_attachment->get_internal_format();
        auto form                        = m_depth_stencil_attachment->get_format();
        auto c_type                      = m_depth_stencil_attachment->component_type();
        m_depth_stencil_attachment->release();
        m_depth_stencil_attachment = texture::create(config);
        m_depth_stencil_attachment->set_data(internal, width, height, form, c_type, nullptr);
        glNamedFramebufferTexture(m_name, GL_DEPTH_STENCIL_ATTACHMENT, m_depth_stencil_attachment->get_name(), 0);
    }

    if (g_enum status = glCheckNamedFramebufferStatus(m_name, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        MANGO_LOG_ERROR("Framebuffer {0} is incomplete! Status: {1}.", m_name, status);
        MANGO_UNUSED(status);
    }
}

texture_ptr framebuffer_impl::get_attachment(framebuffer_attachment attachment)
{
    MANGO_ASSERT(is_created(), "Framebuffer not created!");
    switch (attachment)
    {
    case framebuffer_attachment::color_attachment0:
        return m_color_attachment0;
    case framebuffer_attachment::color_attachment1:
        return m_color_attachment1;
    case framebuffer_attachment::color_attachment2:
        return m_color_attachment2;
    case framebuffer_attachment::color_attachment3:
        return m_color_attachment3;
    case framebuffer_attachment::depth_attachment:
        return m_depth_attachment;
    case framebuffer_attachment::stencil_attachment:
        return m_stencil_attachment;
    case framebuffer_attachment::depth_stencil_attachment:
        return m_depth_stencil_attachment;
    default:
        MANGO_LOG_ERROR("Framebuffer attachment type {0} is unknown!", attachment);
        return nullptr;
    }
}
