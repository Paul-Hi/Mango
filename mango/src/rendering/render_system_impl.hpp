//! \file      render_system_impl.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_RENDER_SYSTEM_IMPL_HPP
#define MANGO_RENDER_SYSTEM_IMPL_HPP

#include <core/context_impl.hpp>
#include <graphics/command_buffer.hpp>
#include <mango/render_system.hpp>
#include <queue>

namespace mango
{
    //! \brief The implementation of the \a render_system.
    //! \details This class only manages the configuration of the base \a render_system and forwards everything else to the real implementation of the specific configured one.
    class render_system_impl : public render_system
    {
      public:
        //! \brief Constructs the \a render_system_impl.
        //! \param[in] context The internally shared context of mango.
        render_system_impl(const shared_ptr<context_impl>& context);
        ~render_system_impl();

        virtual bool create() override;
        virtual void configure(const render_configuration& configuration) override;

        //! \brief Retrieves the \a command_buffer of a \a render_system.
        //! \details The \a command_buffer should be created and destroyed by the \a render_system.
        //! Also this specific \a command_buffer should not be executed externally. This would lead to undefined behavior.
        //! \return The \a command_buffer of the \a render_system.
        inline command_buffer_ptr get_command_buffer()
        {
            return m_command_buffer;
        }

        //! \brief Renders the current frame.
        //! \details Calls the execute() function of the \a command_buffer, after doing some other things to it.
        //! This includes for example extra framebuffers and passes.
        virtual void render();

        virtual void update(float dt) override;
        virtual void destroy() override;

        //! \brief Retrieves and returns the base \a render_pipeline of the real implementation of the \a render_system.
        //! \details This needs to be overriden by th real \a render_system_impl.
        //! \return The current set base \a render_pipeline of the \a render_system.
        virtual render_pipeline get_base_render_pipeline();

      protected:
        //! \brief Mangos internal context for shared usage in all \a render_systems.
        shared_ptr<context_impl> m_shared_context;

        //! \brief The \a command_buffer for the \a render_system.
        command_buffer_ptr m_command_buffer;

      private:
        //! \brief A shared pointer to the currently used internal \a render_system.
        //! \details This is used to make runtime switching of different \a render_systems possible.
        shared_ptr<render_system_impl> m_current_render_system;
    };

} // namespace mango

#endif // #define MANGO_RENDER_SYSTEM_IMPL_HPP
