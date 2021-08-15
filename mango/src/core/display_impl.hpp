//! \file      display_impl.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_DISPLAY_IMPL_HPP
#define MANGO_DISPLAY_IMPL_HPP

#include <mango/display_event_handler.hpp>
#include <mango/display.hpp>

namespace mango
{
    //! \brief The info to setup a \a display.
    class display_info
    {
      public:
        //! \brief The pixel formats possible for the hardware buffer.
        enum class pixel_format : uint8
        {
            rgba8888, //!< 4 channels with 8 bits
            rgb888    //!< 3 channels with 8 bits
        };

        //! \brief The depth stencil formats possible for the hardware buffer.
        enum class depth_stencil_format : uint8
        {
            depth16,          //!< 16 bit depth, no stencil
            depth24,          //!< 24 bit depth, no stencil
            depth16_stencil8, //!< 16 bit depth, 8 bit stencil
            depth24_stencil8  //!< 24 bit depth, 8 bit stencil
        };

        display_info()
            : x(0)
            , y(0)
            , width(0)
            , height(0)
            , title("")
            , decorated(true)
            , pixel_format(pixel_format::rgba8888)
            , depth_stencil_format(depth_stencil_format::depth16)
            , native_renderer(display_configuration::native_renderer_type::opengl)
            , display_event_handler(nullptr)
        {
        }

        //! \brief Horizontal screen position.
        int32 x;
        //! \brief Vertical screen position.
        int32 y;
        //! \brief Display width.
        int32 width;
        //! \brief Display height.
        int32 height;
        //! \brief Display title.
        const char* title;
        //! \brief Defines if display should be decorated.
        bool decorated;

        //! \brief Pixel format for the display buffer.
        pixel_format pixel_format;
        //! \brief Depth stencil format for the display buffer.
        depth_stencil_format depth_stencil_format;
        //! \brief Native renderer for the display.
        display_configuration::native_renderer_type native_renderer;
        //! \brief Pointer to handler to route events to mango.
        display_event_handler_ptr display_event_handler;
    };

    //! \brief The internal display interface.
    class display_impl : public display
    {
      public:
        virtual ~display_impl()                                                              = default;
        virtual void change_size(int32 width, int32 height)                                  = 0;
        virtual void quit()                                                                  = 0;
        virtual bool is_initialized() const                                                  = 0;
        virtual int32 get_x_position() const                                                 = 0;
        virtual int32 get_y_position() const                                                 = 0;
        virtual int32 get_width() const                                                      = 0;
        virtual int32 get_height() const                                                     = 0;
        virtual const char* get_title() const                                                = 0;
        virtual bool is_decorated() const                                                    = 0;
        virtual display_configuration::native_renderer_type get_native_renderer_type() const = 0;

        //! \brief Polls \a display events.
        //! \details The call is necessary to receive events from the operating system.
        virtual void poll_events() const = 0;

        //! \brief Determines if the \a display should close.
        //! \return True if the \a display should close, else false.
        virtual bool should_close() const = 0;

        //! \brief Type alias describing the native handle for any platform window.
        using native_window_handle = void*;

        //! \brief Retrieves and returns the underlying handle to the operating system window.
        //! \return The operating system window handle.
        virtual native_window_handle native_handle() const = 0;
    };
} // namespace mango

#endif // MANGO_DISPLAY_IMPL_HPP