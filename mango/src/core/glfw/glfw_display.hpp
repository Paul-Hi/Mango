//! \file      glfw_display.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_GLFW_DISPLAY_HPP
#define MANGO_GLFW_DISPLAY_HPP

#include <core/display_impl.hpp>
#include <util/helpers.hpp>
#define GLFW_INCLUDE_NONE // Do not include gl headers, will be done by ourselfs later on.
#include <GLFW/glfw3.h>

namespace mango
{
    //! \brief A \a display_impl using glfw to create the window and manage the input.
    class glfw_display : public display_impl
    {
        MANGO_DISABLE_COPY_AND_ASSIGNMENT(glfw_display)
      public:
        //! \brief Constructor.
        //! \param[in] create_info The \a display_info to use to construct the \a glfw_display.
        glfw_display(const display_info& create_info);
        ~glfw_display();
        void change_size(int32 width, int32 height) override;
        void quit() override;
        bool is_initialized() const override;
        int32 get_x_position() const override;
        int32 get_y_position() const override;
        int32 get_width() const override;
        int32 get_height() const override;
        const char* get_title() const override;
        bool is_decorated() const override;
        display_configuration::native_renderer_type get_native_renderer_type() const override;
        void poll_events() const override;
        bool should_close() const override;
        display_impl::native_window_handle native_handle() const override;

      private:
        //! \brief Initializes all necessary data and creates a display.
        //! \return True on success, else false.
        bool initialize();

        //! \brief Creates a display with opengl backend.
        //! \return True on success, else false.
        bool create_glfw_opengl();

        //! \brief Internal data.
        struct glfw_display_data
        {
            //! \brief The native handle to the GLFWWindow.
            GLFWwindow* native_handle;
            //! \brief The stored \a display_info.
            //! \details Gets updated on callbacks.
            display_info info;

            //! \brief True if \a display is already initialized, else false.
            bool initialized;
        } m_glfw_display_data; //!< The internal data for the display.

        //! \brief Number of native GLFWWindows created in the class.
        static int32 s_glfw_windows;
    };
} // namespace mango

#endif // MANGO_GLFW_DISPLAY_HPP