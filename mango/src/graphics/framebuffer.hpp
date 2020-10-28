//! \file      framebuffer.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_FRAMEBUFFER_HPP
#define MANGO_FRAMEBUFFER_HPP

#include <graphics/graphics_object.hpp>

namespace mango
{
    //! \brief A configuration for \a framebuffers.
    class framebuffer_configuration : public graphics_configuration
    {
      public:
        //! \brief Constructs a new \a framebuffer_configuration with default values.
        framebuffer_configuration()
            : graphics_configuration()
        {
        }

        //! \brief Constructs a new \a framebuffer_configuration.
        //! \param[in] width The width for the \a framebuffer to create. Has to be a positive value.
        //! \param[in] height The height for the \a framebuffer to create. Has to be a positive value.
        //! \param[in] color_attachment0 The \a texture_ptr to the \a texture bound as the \a framebuffers color attachment at position 0.
        //! \param[in] color_attachment1 The \a texture_ptr to the \a texture bound as the \a framebuffers color attachment at position 1.
        //! \param[in] color_attachment2 The \a texture_ptr to the \a texture bound as the \a framebuffers color attachment at position 2.
        //! \param[in] color_attachment3 The \a texture_ptr to the \a texture bound as the \a framebuffers color attachment at position 3.
        //! \param[in] depth_attachment The \a texture_ptr to the \a texture bound as the \a framebuffers depth attachment.
        //! \param[in] stencil_attachment The \a texture_ptr to the \a texture bound as the \a framebuffers stencil attachment.
        //! \param[in] depth_stencil_attachment The \a texture_ptr to the \a texture bound as the \a framebuffers depth stencil attachment.
        framebuffer_configuration(int32 width, int32 height, texture_ptr color_attachment0, texture_ptr color_attachment1, texture_ptr color_attachment2, texture_ptr color_attachment3, texture_ptr depth_attachment,
                                  texture_ptr stencil_attachment, texture_ptr depth_stencil_attachment)
            : graphics_configuration()
            , m_width(width)
            , m_height(height)
            , m_color_attachment0(color_attachment0)
            , m_color_attachment1(color_attachment1)
            , m_color_attachment2(color_attachment2)
            , m_color_attachment3(color_attachment3)
            , m_depth_attachment(depth_attachment)
            , m_stencil_attachment(stencil_attachment)
            , m_depth_stencil_attachment(depth_stencil_attachment)
        {
        }

        //! \brief The width of the \a framebuffer.
        int32 m_width;
        //! \brief The height of the \a framebuffer.
        int32 m_height;
        //! \brief The first color attachment.
        texture_ptr m_color_attachment0;
        //! \brief The second color attachment.
        texture_ptr m_color_attachment1;
        //! \brief The third color attachment.
        texture_ptr m_color_attachment2;
        //! \brief The fourth color attachment.
        texture_ptr m_color_attachment3;
        //! \brief The depth attachment.
        texture_ptr m_depth_attachment;
        //! \brief The stencil attachment.
        texture_ptr m_stencil_attachment;
        //! \brief The depth and stencil attachment.
        texture_ptr m_depth_stencil_attachment;

        bool is_valid() const
        {
            if (nullptr != m_depth_attachment && nullptr != m_depth_stencil_attachment)
                return false;
            if (nullptr != m_stencil_attachment && nullptr != m_depth_stencil_attachment)
                return false;
            return m_width > 0 && m_height > 0 && nullptr != m_color_attachment0;
        }
    };

    //! \brief A object with \a texture attachments for drawing into.
    //! \details Used to share \a image data between cpu and gpu devices.
    //! Can be bound for sampling in the \a shaders.
    class framebuffer : public graphics_object
    {
      public:
        //! \brief Creates a new \a framebuffer and returns a pointer to it.
        //! \param[in] configuration The \a framebuffer_configuration for the new \a framebuffer.
        //! \return A pointer to the new \a framebuffer.
        static framebuffer_ptr create(const framebuffer_configuration& configuration);

        //! \brief Returns the width of the \a framebuffer in pixels.
        //! \return Width of the \a framebuffer in pixels.
        virtual int32 get_width() = 0;

        //! \brief Returns the height of the \a framebuffer in pixels.
        //! \return Height of the \a framebuffer in pixels.
        virtual int32 get_height() = 0;

        //! \brief Resizes the \a framebuffer and all its attachments.
        //! \param[in] width The new width of the \a framebuffer in pixels. Has to be a positive value.
        //! \param[in] height The new height of the \a framebuffer in pixels. Has to be a positive value.
        virtual void resize(int32 width, int32 height) = 0;

        //! \brief Returns a specific attachment of the \a framebuffer.
        //! \param[in] attachment The specificiation of the attachment \a texture to retrieve.
        //! \return Attachment \a texture of the \a framebuffer specified.
        virtual texture_ptr get_attachment(framebuffer_attachment attachment) = 0;

      protected:
        framebuffer()  = default;
        ~framebuffer() = default;
    };
} // namespace mango

#endif // MANGO_FRAMEBUFFER_HPP
