//! \file      types.hpp
//! This file has different typedefs mostly for convenience and some perfect forwarding helper functions for standard types as well as some helpful macro definitions.
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_TYPES_HPP
#define MANGO_TYPES_HPP

#include <limits>
#include <memory>
#include <stdint.h>
#include <string>

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

    //! \brief Type alias for a size_t.
    using ptr_size = ::size_t;

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

    //! \brief  Cast an object that is owned by a unique_ptr to an other object type.
    //! \param[in]  old  unique_ptr of type \a F to cast from.
    //! \return A unique_ptr holding the object pointer casted from \a F to \a T.
    template <typename T, typename F>
    unique_ptr<T> static_unique_pointer_cast(unique_ptr<F>&& old)
    {
        return unique_ptr<T>{ static_cast<T*>(old.release()) };
    }

    //! \brief The pointer to the procedure address loading function type for opengl.
    typedef void* (*mango_gl_load_proc)(const char*);

    //! \brief The type of the resource used in a shader program by the gpu.
    enum gpu_resource_type
    {
        gpu_unknown,              //!< The representation of an unknown type on the gpu.
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

    //! \cond NO_COND

    template <typename e>
    struct bit_mask_operations
    {
        static const bool enable = false;
    };

    template <typename e>
    typename std::enable_if<bit_mask_operations<e>::enable, e>::type operator|(e lhs, e rhs)
    {
        typedef typename std::underlying_type<e>::type underlying;
        return static_cast<e>(static_cast<underlying>(lhs) | static_cast<underlying>(rhs));
    }

    template <typename e>
    typename std::enable_if<bit_mask_operations<e>::enable, e>::type operator&(e lhs, e rhs)
    {
        typedef typename std::underlying_type<e>::type underlying;
        return static_cast<e>(static_cast<underlying>(lhs) & static_cast<underlying>(rhs));
    }

    template <typename e>
    typename std::enable_if<bit_mask_operations<e>::enable, e>::type operator^(e lhs, e rhs)
    {
        typedef typename std::underlying_type<e>::type underlying;
        return static_cast<e>(static_cast<underlying>(lhs) ^ static_cast<underlying>(rhs));
    }

    template <typename e>
    typename std::enable_if<bit_mask_operations<e>::enable, e>::type operator~(e lhs)
    {
        typedef typename std::underlying_type<e>::type underlying;
        return static_cast<e>(~static_cast<underlying>(lhs));
    }

    template <typename e>
    typename std::enable_if<bit_mask_operations<e>::enable, e&>::type operator|=(e& lhs, e rhs)
    {
        typedef typename std::underlying_type<e>::type underlying;
        lhs = static_cast<e>(static_cast<underlying>(lhs) | static_cast<underlying>(rhs));
        return lhs;
    }

    template <typename e>
    typename std::enable_if<bit_mask_operations<e>::enable, e&>::type operator&=(e& lhs, e rhs)
    {
        typedef typename std::underlying_type<e>::type underlying;
        lhs = static_cast<e>(static_cast<underlying>(lhs) & static_cast<underlying>(rhs));
        return lhs;
    }

    template <typename e>
    typename std::enable_if<bit_mask_operations<e>::enable, e&>::type operator^=(e& lhs, e rhs)
    {
        typedef typename std::underlying_type<e>::type underlying;
        lhs = static_cast<e>(static_cast<underlying>(lhs) ^ static_cast<underlying>(rhs));
        return lhs;
    }

    //! \endcond

//! \brief Macro used to enable safe bitmask operations on enum classes.
#define MANGO_ENABLE_BITMASK_OPERATIONS(e)    \
    template <>                              \
    struct bit_mask_operations<e>            \
    {                                        \
        static constexpr bool enable = true; \
    };

} // namespace mango

#endif // MANGO_TYPES_HPP