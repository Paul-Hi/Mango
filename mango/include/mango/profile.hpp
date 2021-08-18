//! \file      profile.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_PROFILE_HPP
#define MANGO_PROFILE_HPP

#if defined(MANGO_PROFILE) && defined(TRACY_ENABLE)
#include <Tracy.hpp>
#include <glad/glad.h>
#include <TracyOpenGL.hpp>
#endif

namespace mango
{
#if defined(MANGO_PROFILE) && defined(TRACY_ENABLE)
    //! \brief Profile the next zone.
    #define PROFILE_ZONE ZoneScoped
    //! \brief Profile the next zone with some extra name.
    #define NAMED_PROFILE_ZONE(name) ZoneScopedN(name)
    //! \brief Mark the frame as done.
    #define MARK_FRAME FrameMark

    //! \brief Create a profile context for OpenGL.
    #define GL_PROFILED_CONTEXT TracyGpuContext
    //! \brief Collect profiling for OpenGL.
    #define GL_PROFILE_COLLECT TracyGpuCollect
    //! \brief Profile the next zone with some extra name on the GPU.
    #define GL_NAMED_PROFILE_ZONE(name) TracyGpuZone(name)
#else
    //! \brief Profile the next zone.
    #define PROFILE_ZONE
    //! \brief Profile the next zone with some extra name.
    #define NAMED_PROFILE_ZONE(name)
    //! \brief Mark the frame as done.
    #define MARK_FRAME

    //! \brief Create a profile context for OpenGL.
    #define GL_PROFILED_CONTEXT
    //! \brief Collect profiling for OpenGL.
    #define GL_PROFILE_COLLECT
    //! \brief Profile the next zone with some extra name on the GPU.
    #define GL_NAMED_PROFILE_ZONE(name)
#endif
} // namespace mango

#endif // MANGO_PROFILE_HPP