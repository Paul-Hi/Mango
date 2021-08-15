//! \file      display.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_DISPLAY_HPP
#define MANGO_DISPLAY_HPP

#include <mango/types.hpp>

namespace mango
{
    //! \brief The configuration for a \a display.
    class display_configuration
    {
      public:
        //! \brief The native renderer to setup dependencies in \a display.
        enum class native_renderer_type : uint8
        {
            opengl,
            // vulkan,
            // dx11,
            // dx12,
            // metal,
        };

        display_configuration()
            : m_x(0)
            , m_y(0)
            , m_width(0)
            , m_height(0)
            , m_title("")
            , m_decorated(true)
            , m_native_renderer(native_renderer_type::opengl)
        {
        }

        //! \brief Sets or changes the hint for horizontal position of the \a display.
        //! \details This is a hint and does not guarantee the position.
        //! \param[in] x The horizontal pixel position. Has to be a positive value.
        //! \return A reference to the modified \a display_configuration.
        inline display_configuration& set_x_position_hint(int32 x)
        {
            m_x = x;
            return *this;
        }

        //! \brief Sets or changes the hint for vertical position of the \a display.
        //! \details This is a hint and does not guarantee the position.
        //! \param[in] y The vertical pixel position. Has to be a positive value.
        //! \return A reference to the modified \a display_configuration.
        inline display_configuration& set_y_position_hint(int32 y)
        {
            m_y = y;
            return *this;
        }

        //! \brief Sets or changes the width for the \a display.
        //! \param[in] width The width to set. Has to be a positive value.
        //! \return A reference to the modified \a display_configuration.
        inline display_configuration& set_width(int32 width)
        {
            m_width = width;
            return *this;
        }

        //! \brief Sets or changes the height for the \a display.
        //! \param[in] height The height to set. Has to be a positive value.
        //! \return A reference to the modified \a display_configuration.
        inline display_configuration& set_height(int32 height)
        {
            m_height = height;
            return *this;
        }

        //! \brief Sets or changes the title for the \a display.
        //! \param[in] title The \a display title.
        //! \return A reference to the modified \a display_configuration.
        inline display_configuration& set_title(const char* title)
        {
            m_title = title;
            return *this;
        }

        //! \brief Sets or changes the decoration for the \a display.
        //! \param[in] decoration True if window should be decorated, else false.
        //! \return A reference to the modified \a display_configuration.
        inline display_configuration& set_decoration(bool decoration)
        {
            m_decorated = decoration;
            return *this;
        }

        //! \brief Sets or changes the \a native_renderer_type for the \a display.
        //! \brief This is mandatory to setup up the correct hardware requirements.
        //! \param[in] renderer_type The \a native_renderer_type.
        //! \return A reference to the modified \a display_configuration.
        inline display_configuration& set_native_renderer_type(native_renderer_type renderer_type)
        {
            m_native_renderer = renderer_type;
            return *this;
        }

        //! \brief Retrieves and returns the horizontal position hint.
        //! \return The current horizontal position hint.
        inline int32 get_x_position_hint() const
        {
            return m_x;
        }

        //! \brief Retrieves and returns the vertical position hint.
        //! \return The current vertical position hint.
        inline int32 get_y_position_hint() const
        {
            return m_y;
        }

        //! \brief Retrieves and returns the width.
        //! \return The current width.
        inline int32 get_width() const
        {
            return m_width;
        }

        //! \brief Retrieves and returns the height.
        //! \return The current height.
        inline int32 get_height() const
        {
            return m_height;
        }

        //! \brief Retrieves and returns the title.
        //! \return The current title.
        inline const char* get_title() const
        {
            return m_title;
        }

        //! \brief Retrieves and returns the decoration setup.
        //! \return True if \a display will be decorated, else false.
        inline bool is_decorated() const
        {
            return m_decorated;
        }

        //! \brief Retrieves and returns the \a native_renderer_type.
        //! \return The current \a native_renderer_type.
        inline native_renderer_type get_native_renderer_type() const
        {
            return m_native_renderer;
        }

      private:
        //! \brief Horizontal screen position.
        int32 m_x;
        //! \brief Vertical screen position.
        int32 m_y;
        //! \brief Display width.
        int32 m_width;
        //! \brief Display height.
        int32 m_height;
        //! \brief Display title.
        const char* m_title;
        //! \brief Defines if \a display should be decorated.
        bool m_decorated;
        //! \brief Native renderer for the \a display.
        native_renderer_type m_native_renderer;
    };

    //! \brief Interface for \a display.
    //! \details Defines an interface for all platform specific \a display instances.
    class display
    {
      public:
        virtual ~display() = default;

        //! \brief Sets the size of the \a display in pixels.
        //! \param[in] width of the \a display in pixels. Has to be a positive value.
        //! \param[in] height of the \a display in pixels. Has to be a positive value.
        virtual void change_size(int32 width, int32 height) = 0;

        //! \brief Forces a \a display close.
        virtual void quit() = 0;

        //! \brief Determines whether the \a display is initialized.
        //! \return True if initialized, else false.
        virtual bool is_initialized() const = 0;

        //! \brief Retrieves and returns the horizontal \a display position.
        //! \return The horizontal position.
        virtual int32 get_x_position() const = 0;

        //! \brief Retrieves and returns the vertical \a display position.
        //! \return The vertical position hint.
        virtual int32 get_y_position() const = 0;

        //! \brief Retrieves and returns the \a display width.
        //! \return The width.
        virtual int32 get_width() const = 0;

        //! \brief Retrieves and returns the \a display height.
        //! \return The height.
        virtual int32 get_height() const = 0;

        //! \brief Retrieves and returns the \a display title.
        //! \return The title.
        virtual const char* get_title() const = 0;

        //! \brief Determines whether the \a display is decorated.
        //! \return True if decorated, else false.
        virtual bool is_decorated() const = 0;

        //! \brief Retrieves and returns the \a native_renderer_type.
        //! \return The \a native_renderer_type.
        virtual display_configuration::native_renderer_type get_native_renderer_type() const = 0;
    };

    //! \brief A unique pointer holding a \a display.
    using display_ptr    = std::unique_ptr<display>;

    //! \brief A constant pointer pointing to a \a display.
    using display_handle = const display*;
} // namespace mango

#endif // MANGO_DISPLAY_HPP
