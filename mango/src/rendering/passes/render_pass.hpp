//! \file      render_pass.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2022
//! \copyright Apache License 2.0

#ifndef MANGO_RENDER_PASS_HPP
#define MANGO_RENDER_PASS_HPP

#include <mango/types.hpp>
#include <core/context_impl.hpp>
#include <graphics/graphics_device_context.hpp>

namespace mango
{
    struct render_pass_execution_info
    {
        int32 draw_calls; //!< The number of draw calls.
        int32 vertices;   //!< The number of vertices.
    };

    //! \brief Base class for all render passes in \a renderers.
    class render_pass
    {
      public:
        //! \brief Attaches the pass to the current active \a renderer.
        //! \details After creation this function has to be called. Does all the setup.
        //! \param[in] context The internally shared context of mango.
        virtual void attach(const shared_ptr<context_impl>& context) = 0;

        //! \brief Executes the \a render_pass.
        //! \param[in] device_context The \a graphics_device_context_handle to submit commands to.
        virtual void execute(graphics_device_context_handle& device_context) = 0;

        virtual render_pass_execution_info get_info() = 0;

      protected:
        //! \brief Does create pass resources for the \a render_pass.
        //! \return True on success, else false.
        virtual bool create_pass_resources() = 0;

        //! \brief Mangos shared context.
        shared_ptr<context_impl> m_shared_context;
    };
} // namespace mango

#endif // MANGO_RENDER_PASS_HPP
