//! \file      linux_window_system.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_LINUX_WINDOW_SYSTEM_HPP
#define MANGO_LINUX_WINDOW_SYSTEM_HPP

#include <core/window_system_impl.hpp>

namespace mango
{
    //! \brief The \a window_system for the linux platform.
    class linux_window_system : public window_system_impl
    {
      public:
        //! \brief Constructs the \a linux_window_system.
        //! \param[in] context The internally shared context of mango.
        linux_window_system(const shared_ptr<context_impl>& context);
        ~linux_window_system();
        bool create() override;
        void configure(const window_configuration& configuration) override;

        inline int32 get_width() override
        {
            return m_window_configuration.get_width();
        }
        inline int32 get_height() override
        {
            return m_window_configuration.get_height();
        }
        inline bool vsync() override
        {
            return m_vsync;
        }
        void set_size(int32 width, int32 height) override;

        void swap_buffers() override;
        void update(float dt) override;
        void poll_events() override;
        bool should_close() override;
        void destroy() override;
        void set_vsync(bool enabled) override;
        void make_window_context_current() override;

        inline shared_ptr<platform_data> get_platform_data() override
        {
            return m_platform_data;
        }

      private:
        //! \brief The \a window_configuration for the \a linux_window_system.
        //! \details This holds the information that is needed to create a window.
        window_configuration m_window_configuration;

        //! \brief The platform data holds the window handle that is needed to identify the window after creation.
        //! \details This is important, because without it destruction, update and input polling would fail.
        shared_ptr<platform_data> m_platform_data;

        bool create_window() override;
    };

} // namespace mango

#endif // MANGO_LINUX_WINDOW_SYSTEM_HPP
