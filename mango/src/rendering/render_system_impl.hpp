//! \file      render_system_impl.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_RENDER_SYSTEM_IMPL_HPP
#define MANGO_RENDER_SYSTEM_IMPL_HPP

#include <core/context_impl.hpp>
#include <glm/glm.hpp>
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

        //! \brief Does all the setup and has to be called before rendering the scene.
        //! \details Adds setup and clear calls to the \a command_buffer.
        virtual void begin_render();

        //! \brief Renders the current frame.
        //! \details Calls the execute() function of the \a command_buffer, after doing some other things to it.
        //! This includes for example extra framebuffers and passes.
        virtual void finish_render();

        //! \brief Sets the viewport.
        //! \details  Should be called on resizing events, instead of scheduling a viewport command directly.
        //! This manages the resizing of eventually created \a framebuffers internally and schedules the \a command as well.
        //! \param[in] x The x start coordinate.
        //! \param[in] y The y start coordinate.
        //! \param[in] width The width of the viewport after this call.
        //! \param[in] height The height of the viewport after this call.
        virtual void set_viewport(uint32 x, uint32 y, uint32 width, uint32 height);

        virtual void update(float dt) override;
        virtual void destroy() override;

        //! \brief Retrieves and returns the base \a render_pipeline of the real implementation of the \a render_system.
        //! \details This needs to be overriden by th real \a render_system_impl.
        //! \return The current set base \a render_pipeline of the \a render_system.
        virtual render_pipeline get_base_render_pipeline();

        //! \brief Sets the model matrix for the next draw calls.
        //! \param[in] model_matrix The model matrix for the next draw calls.
        virtual void set_model_matrix(const glm::mat4& model_matrix);

        //! \brief Sets the \a material for the next draw call.
        //! \param[in] mat The \a material for the next draw call.
        virtual void push_material(const material_ptr& mat);

        //! \brief Sets the view projection matrix for the next draw calls.
        //! \param[in] view_projection The view projection for the next draw calls.
        virtual void set_view_projection_matrix(const glm::mat4& view_projection);

        //! \brief Sets the \a texture for a environment.
        //! \param[in] hdr_texture The pointer to the hdr \a texture to use as an environment.
        virtual void set_environment_texture(const texture_ptr& hdr_texture);

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
