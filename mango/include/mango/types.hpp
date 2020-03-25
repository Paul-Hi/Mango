//! \file      types.hpp
//! This file has different typedefs mostly for convenience and some perfect forwarding helper functions for standard types as well as some helpful macro definitions.
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_TYPES_HPP
#define MANGO_TYPES_HPP

#include <memory>
#include <stdint.h>
#include <string>
#include <unordered_map>

namespace mango
{
//! \brief A macro to avoid warnings because of unused variables.
#define MANGO_UNUSED(var) (void)(var)

    //! \brief Type alias for an 8 bit integer.
    using int8 = ::int8_t;
    //! \brief Type alias for a 16 bit integer.
    using int16 = ::int16_t;
    //! \brief Type alias for a 32 bit integer.
    using int32 = ::int32_t;
    //! \brief Type alias for a 64 bit integer.
    using int64 = ::int64_t;

    //! \brief Type alias for an 8 bit unsigned integer.
    using uint8 = ::uint8_t;
    //! \brief Type alias for a 16 bit unsigned integer.
    using uint16 = ::uint16_t;
    //! \brief Type alias for a 32 bit unsigned integer.
    using uint32 = ::uint32_t;
    //! \brief Type alias for a 64 bit unsigned integer.
    using uint64 = ::uint64_t;

    //! \brief Type alias for a std::string.
    using string = std::string;

    //! \brief Type alias for a std::shared_ptr.
    template <typename T>
    using shared_ptr = std::shared_ptr<T>;

    //! \brief Type alias for a std::weak_ptr.
    template <typename T>
    using weak_ptr = std::weak_ptr<T>;

    //! \brief Type alias for a std::unique_ptr.
    template <typename T>
    using unique_ptr = std::unique_ptr<T>;

    //! \brief  Create an object that is owned by a unique_ptr.
    //! \param[in]  args  Arguments for the \a T object's constructor.
    //! \return A unique_ptr that owns the newly created object.
    template <typename T, typename... Args>
    unique_ptr<T> make_unique(Args&&... args)
    {
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }

    //! \brief The pointer to the procedure address loading function type for opengl.
    typedef void* (*mango_gl_load_proc)(const char*);

    //! \brief The type of the resource used in a shader program by the gpu.
    enum gpu_resource_type
    {
        gpu_float,                //!< The representation of a float on the gpu.
        gpu_vec2,                 //!< The representation of a float vec2 on the gpu.
        gpu_vec3,                 //!< The representation of a float vec3 on the gpu.
        gpu_vec4,                 //!< The representation of a float vec4 on the gpu.
        gpu_int,                  //!< The representation of a int on the gpu.
        gpu_ivec2,                //!< The representation of a int vec2 on the gpu.
        gpu_ivec3,                //!< The representation of a int vec3 on the gpu.
        gpu_ivec4,                //!< The representation of a int vec4 on the gpu.
        gpu_mat3,                 //!< The representation of a float mat3 on the gpu.
        gpu_mat4,                 //!< The representation of a float mat4 on the gpu.
        gpu_sampler_texture_2d,   //!< A texture sampler with two dimensions.
        gpu_sampler_texture_cube, //!< A cube texture sampler with six faces and two dimensions each.
        gpu_framebuffer           //!< A framebuffer resource which can be used e.g. as an output.
    };

} // namespace mango

#endif // MANGO_TYPES_HPP