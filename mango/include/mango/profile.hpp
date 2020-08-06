//! \file      profile.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_PROFILE_HPP
#define MANGO_PROFILE_HPP

#ifdef MANGO_PROFILE
#include <Tracy.hpp>
#include <glad/glad.h>
#include <TracyOpenGL.hpp>
#endif

namespace mango
{
#ifdef MANGO_PROFILE
    #define PROFILE_ZONE ZoneScoped
    #define NAMED_PROFILE_ZONE(name) ZoneScopedN(name)

    #define GL_PROFILED_CONTEXT TracyGpuContext
    #define GL_PROFILE_COLLECT TracyGpuCollect
    #define GL_NAMED_PROFILE_ZONE(name) TracyGpuZone(name)
#else
    #define PROFILE_ZONE()
    #define NAMED_PROFILE_ZONE(name)
    #define GL_PROFILED_CONTEXT
    #define GL_NAMED_PROFILE_ZONE(name)
    #define GL_PROFILE_COLLECT
#endif
} // namespace mango

#endif // MANGO_PROFILE_HPP