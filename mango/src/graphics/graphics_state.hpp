//! \file      graphics_state.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_GRAPHICS_STATE_HPP
#define MANGO_GRAPHICS_STATE_HPP

#include <graphics/graphics_types.hpp>

namespace mango
{
    //! \brief Interface for all graphics states used for mirroring the gpu state for optimization and tracing.
    struct gfx_graphics_state
    {
        //! \brief Checks if a certain buffer is already bound.
        //! \param[in] target The \a gfx_buffer_target to check.
        //! \param[in] idx The binding index to check.
        //! \param[in] native_handle The native handle of the buffer to check.
        virtual bool is_buffer_bound(gfx_buffer_target target, int32 idx, void* native_handle) = 0;

        //! \brief Records a certain binding of a buffer.
        //! \param[in] target The \a gfx_buffer_target to record.
        //! \param[in] idx The binding index to record.
        //! \param[in] native_handle The native handle of the buffer to record.
        virtual void record_buffer_binding(gfx_buffer_target target, int32 idx, void* native_handle) = 0;

        // TODO Paul: More improvements!
        /*
        virtual bool is_texture_bound()       = 0;
        virtual void record_texture_binding() = 0;

        virtual bool is_image_texture_bound()       = 0;
        virtual void record_image_texture_binding() = 0;

        virtual bool is_sampler_bound()       = 0;
        virtual void record_sampler_binding() = 0;
        */
    };
} // namespace mango

#endif // MANGO_GL_GRAPHICS_STATE_HPP
