//! \file      framebuffer_impl.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_FRAMEBUFFER_IMPL_HPP
#define MANGO_FRAMEBUFFER_IMPL_HPP

#include <graphics/framebuffer.hpp>

namespace mango
{
    //! \brief The implementation of the \a framebuffer.
    class framebuffer_impl : public framebuffer
    {
      public:
        //! \brief Constructs the \a framebuffer_impl.
        //! \param[in] configuration The \a framebuffer_configuration.
        framebuffer_impl(const framebuffer_configuration& configuration);
        ~framebuffer_impl();

        inline int32 get_width() override
        {
            return m_width;
        }

        inline int32 get_height() override
        {
            return m_height;
        }

        void resize(int32 width, int32 height) override;

        texture_ptr get_attachment(framebuffer_attachment attachment) override;

      private:
        //! \brief The width of the \a framebuffer.
        int32 m_width;
        //! \brief The height of the \a framebuffer.
        int32 m_height;

        //! \brief The attachments set as draw buffers.
        std::vector<g_enum> m_draw_buffers;
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
    };
} // namespace mango

#endif // MANGO_FRAMEBUFFER_IMPL_HPP
