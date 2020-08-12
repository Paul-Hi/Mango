//! \file      window_system.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_WINDOW_SYSTEM_HPP
#define MANGO_WINDOW_SYSTEM_HPP

#include <mango/assert.hpp>
#include <mango/system.hpp>

namespace mango
{
    //! \brief The configuration for the \a window_system.
    //! \details Should be used to configure the \a window_system in the \a application create() method.
    class window_configuration
    {
      public:
        //! \brief Default constructor to set some default values before the user application configures the \a window_system.
        window_configuration()
            : m_width(256)
            , m_height(128)
            , m_title("Mango")
        {
        }

        //! \brief Constructs a \a window_configuration with specific values.
        //! \param[in] width The configurated width for the \a window_system to create a window. Has to be a positive value.
        //! \param[in] height The configurated height for the \a window_system to create a window. Has to be a positive value.
        //! \param[in] title The configurated title for the \a window_system to create a window.
        window_configuration(int32 width, int32 height, const char* title)
            : m_width(width)
            , m_height(height)
            , m_title(title)
        {
            MANGO_ASSERT(width > 0, "Invalid window width!");
            MANGO_ASSERT(height > 0, "Invalid window height!");
        }

        //! \brief Sets or changes the width in the \a window_configuration.
        //! \param[in] width The configurated width for the \a window_system to create a window. Has to be a positive value.
        //! \return A reference to the modified \a window_configuration.
        inline window_configuration& set_width(int32 width)
        {
            MANGO_ASSERT(width > 0, "Invalid window width!");
            m_width = width;
            return *this;
        }

        //! \brief Sets or changes the height in the \a window_configuration.
        //! \param[in] height The configurated height for the \a window_system to create a window. Has to be a positive value.
        //! \return A reference to the modified \a window_configuration.
        inline window_configuration& set_height(int32 height)
        {
            MANGO_ASSERT(height > 0, "Invalid window height!");
            m_height = height;
            return *this;
        }

        //! \brief Sets or changes the title in the \a window_configuration.
        //! \param[in] title The configurated title for the \a window_system to create a window.
        //! \return A reference to the modified \a window_configuration.
        inline window_configuration& set_title(const char* title)
        {
            m_title = title;
            return *this;
        }

        //! \brief Retrieves and returns the width of the \a window_configuration.
        //! \return The current configurated width.
        inline int32 get_width()
        {
            return m_width;
        }

        //! \brief Retrieves and returns the height of the \a window_configuration.
        //! \return The current configurated height.
        inline int32 get_height()
        {
            return m_height;
        }

        //! \brief Retrieves and returns the title of the \a window_configuration.
        //! \return The current configurated title.
        inline const char* get_title()
        {
            return m_title;
        }

      private:
        //! \brief The configurated width of the \a window_configuration.
        int32 m_width;
        //! \brief The configurated height of the \a window_configuration.
        int32 m_height;
        //! \brief The configurated title of the \a window_configuration.
        const char* m_title;
    };

    //! \brief A system for window creation and handling.
    //! \details The \a window_system manages the handle of the window, swaps buffers after rendering and polls for input.
    class window_system : public system
    {
      public:
        //! \brief Does the configuration of the \a window_system.
        //! \details After creation this function should be called.
        //! Changes the configuration in the \a window_system to \a configuration.
        //! \param[in] configuration The \a window_configuration to use for the window.
        virtual void configure(const window_configuration& configuration) = 0;

        //! \brief Returns the width of the \a window in pixels.
        //! \return Width of the \a window in pixels.
        virtual int32 get_width() = 0;
        //! \brief Returns the height of the \a window in pixels.
        //! \return Height of the \a window in pixels.
        virtual int32 get_height() = 0;
        //! \brief Sets the size of the \a window in pixels.
        //! \param[in] width of the \a window in pixels. Has to be a positive value.
        //! \param[in] height of the \a window in pixels. Has to be a positive value.
        virtual void set_size(int32 width, int32 height) = 0;

      protected:
        virtual bool create()         = 0;
        virtual void update(float dt) = 0;
        virtual void destroy()        = 0;
    };

} // namespace mango

#endif // MANGO_WINDOW_SYSTEM_HPP
