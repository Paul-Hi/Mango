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
#include <functional>

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

    //! \brief Type definition of a callback for drag'n'drop.
    using drag_n_drop_callback = std::function<void(int count, const char** paths)>;

    //! \brief Describes the topology of primitives used for rendering and interpreting geometry data.
    //! \details Same as OpenGL.
    enum class primitive_topology : uint8
    {
        POINTS,
        LINES,
        LINE_LOOP,
        LINE_STRIP,
        TRIANGLES,
        TRIANGLE_STRIP,
        TRIANGLE_FAN,
        QUADS
    };

    //! \brief The data type in index buffers.
    enum class index_type : uint32
    {
        UBYTE  = 0x1401,
        USHORT = 0x1403,
        UINT   = 0x1405
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
#define MANGO_ENABLE_BITMASK_OPERATIONS(e)   \
    template <>                              \
    struct bit_mask_operations<e>            \
    {                                        \
        static constexpr bool enable = true; \
    };

} // namespace mango

#endif // MANGO_TYPES_HPP