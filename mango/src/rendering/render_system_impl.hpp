//! \file      render_system_impl.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_RENDER_SYSTEM_IMPL_HPP
#define MANGO_RENDER_SYSTEM_IMPL_HPP

#include <core/context_impl.hpp>
#include <mango/render_system.hpp>
#include <queue>
#include <rendering/render_data.hpp>

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

        //! \brief Starts a new frame to render.
        //! \details This has to be called before submiting \a render_commands.
        //! The internal \a render_states get reset and the queue is cleared.
        virtual void start_frame();

        //! \brief Pushes a \a render_command to the end of the queue.
        //! \details A \a render_command must include the type of the \a render_command and should also provide all relevant data.
        //! The ownership of the data in the \a render_command is still with the caller, so the caller has to guarantee that the pointer is valid for the whole frame.
        //! In return the \a render_system will not delete or corrupt the memory the data pointer is pointing to.
        //! \param[in] command The \a render_command to submit to the \a render_system.
        virtual void submit(const render_command& command);

        //! \brief Finishes the current frame.
        //! \details This has to be called after submiting \a render_commands and before calling \a render().
        //! The queue is sorted in here and the frame is prepared for rendering.
        virtual void finish_frame();

        //! \brief Renders the current frame.
        //! \details This can be called after calling \a finish_frame().
        //! If a frame is in the queue and this function is not called the frame will be lost after the next call to \a start_frame().
        virtual void render();

        virtual void update(float dt) override;
        virtual void destroy() override;

        //! \brief Retrieves and returns the base \a render_pipeline of the real implementation of the \a render_system.
        //! \details This needs to be overriden by th real \a render_system_impl.
        //! \return The current set base \a render_pipeline of the \a render_system.
        virtual render_pipeline get_base_render_pipeline();

      protected:
        //! \brief The queue for the \a render_commands.
        //! \details This is cleared before each new frame in \a start_frame(). Is sorted in \a finish_frame().
        std::queue<render_command> m_command_queue;

        //! \brief Mangos internal context for shared usage in all \a render_systems.
        shared_ptr<context_impl> m_shared_context;

        //! \brief The current internal \a render_state of the \a render_system.
        render_state m_render_state;

        //! \brief Updates the current \a render_state.
        //! \param[in] state The \a render_state to copy from.
        virtual void updateState(const render_state& state);

      private:
        //! \brief A shared pointer to the currently used internal \a render_system.
        //! \details This is used to make runtime switching of different \a render_systems possible.
        shared_ptr<render_system_impl> m_current_render_system;
    };

} // namespace mango

#endif // #define MANGO_RENDER_SYSTEM_IMPL_HPP
