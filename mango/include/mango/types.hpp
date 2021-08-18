//! \file      types.hpp
//! This file has different typedefs mostly for convenience and some perfect forwarding helper functions for standard types as well as some helpful macro definitions.
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_TYPES_HPP
#define MANGO_TYPES_HPP

//! \cond NO_COND
#define GLM_FORCE_SILENT_WARNINGS 1
//! \endcond
#include <functional>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <limits>
#include <mango/assert.hpp>
#include <memory>
#include <stdint.h>
#include <string>
#include <tl/optional.hpp>

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

//! \brief The maximum size of a int64.
#define MAX_INT64 std::numeric_limits<int64>::max()

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

    //! \brief Type alias for an iintptr_t.
    using intptr = ::intptr_t;

    //! \brief Type alias for an uintptr_t.
    using uintptr = ::uintptr_t;

    //! \brief Type alias for a std::string.
    using string = std::string;

    //! \brief Type alias for a glm::ivec2.
    using ivec2 = glm::ivec2;

    //! \brief Type alias for a glm::ivec3.
    using ivec3 = glm::ivec3;

    //! \brief Type alias for a glm::ivec4.
    using ivec4 = glm::ivec4;

    //! \brief Type alias for a glm::vec2.
    using vec2 = glm::vec2;

    //! \brief Type alias for a glm::vec3.
    using vec3 = glm::vec3;

    //! \brief Type alias for a glm::vec4.
    using vec4 = glm::vec4;

    //! \brief Type alias for a glm::quat.
    using quat = glm::quat;

    //! \brief Type alias for a glm::dvec2.
    using dvec2 = glm::dvec2;

    //! \brief Type alias for a glm::dvec3.
    using dvec3 = glm::dvec3;

    //! \brief Type alias for a glm::dvec4.
    using dvec4 = glm::dvec4;

    //! \brief Type alias for a glm::mat3.
    using mat3 = glm::mat3;

    //! \brief Type alias for a glm::mat4.
    using mat4 = glm::mat4;

    //! \brief Type alias for a std::shared_ptr.
    template <typename T>
    using shared_ptr = std::shared_ptr<T>;

    //! \brief Type alias for a std::weak_ptr.
    template <typename T>
    using weak_ptr = std::weak_ptr<T>;

    //! \brief Type alias for a std::unique_ptr.
    template <typename T>
    using unique_ptr = std::unique_ptr<T>;

    //! \brief Type alias for a tl::optional.
    template <typename T>
    using optional = tl::optional<T>;

//! \brief Define for a tl::nullopt.
#define NULL_OPTION tl::nullopt

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

//! \brief Pi.
#define PI 3.1415926535897932384626433832795f
//! \brief Pi times two.
#define TWO_PI 2.0f * PI
//! \brief Define for the global up vector.
#define GLOBAL_UP vec3(0.0f, 1.0f, 0.0f)
//! \brief Define for the global right vector.
#define GLOBAL_RIGHT vec3(1.0f, 0.0f, 0.0f)
//! \brief Define for the global forward vector.
#define GLOBAL_FORWARD vec3(0.0f, 0.0f, -1.0f)
//! \brief Define for the global unit vector.
#define GLOBAL_UNIT vec3(1.0f)

    //! \brief A floating point type used to describe properties with a 0 to 1 range.
    struct normalized_float
    {
        //! \cond NO_COND
        normalized_float(float v = 0.0f)
        {
            MANGO_ASSERT(v >= 0.0f && v <= 1.0f, "Value is not normalized (between 0.0f and 1.0f)!");
            value = v;
        }
        operator float()
        {
            MANGO_ASSERT(value >= 0.0f && value <= 1.0f, "Value is not normalized (between 0.0f and 1.0f)!");
            return value;
        }
        float* type_data()
        {
            return &value;
        }
        normalized_float& operator=(const normalized_float& other)
        {
            MANGO_ASSERT(other.value >= 0.0f && other.value <= 1.0f, "Value is not normalized (between 0.0f and 1.0f)!");
            if (this != &other)
                value = other.value;
            return *this;
        }
        normalized_float& operator=(normalized_float&& other)
        {
            MANGO_ASSERT(other.value >= 0.0f && other.value <= 1.0f, "Value is not normalized (between 0.0f and 1.0f)!");
            if (this != &other)
                value = other.value;
            return *this;
        }

      private:
        float value;
        //! \endcond
    };

    //! \brief A 3d vector type used to describe rgb color properties. Values should be between 0.0 and 1.0 unless color is a hdr one.
    struct color_rgb
    {
        //! \cond NO_COND
        color_rgb(vec3 v = vec3(0.0f))
        {
            values = v;
        }
        color_rgb(const color_rgb& other)
        {
            if (this != &other)
                values = other.values;
        }
        color_rgb(float v)
        {
            MANGO_ASSERT(v >= 0.0f && v <= 1.0f, "Value is not normalized (between 0.0f and 1.0f)!");
            this->r = v;
            this->g = v;
            this->b = v;
        }
        color_rgb(float r, float g, float b)
        {
            this->r = r;
            this->g = g;
            this->b = b;
        }
        operator vec3()
        {
            return values;
        }
        operator float*()
        {
            return glm::value_ptr(values);
        }
        color_rgb& operator=(const color_rgb& other)
        {
            if (this != &other)
                values = other.values;
            return *this;
        }
        color_rgb& operator=(color_rgb&& other)
        {
            if (this != &other)
                values = other.values;
            return *this;
        }

#pragma warning(push)
#pragma warning(disable : 4201) // nonstandard extension used : nameless struct/union
        union
        {
            struct
            {
                float r, g, b;
            };

            vec3 values;
        };
#pragma warning(pop)
        //! \endcond
    };

    //! \brief A 4d vector type used to describe rgba color properties. Values should be between 0.0 and 1.0 unless color is a hdr one.
    struct color_rgba
    {
        //! \cond NO_COND
        color_rgba(vec4 v = vec4(0.0f))
        {
            values = v;
        }
        color_rgba(const color_rgba& other)
        {
            if (this != &other)
                values = other.values;
        }
        color_rgba(float v)
        {
            this->r = v;
            this->g = v;
            this->b = v;
            this->a = v;
        }
        color_rgba(float r, float g, float b, float a)
        {
            this->r = r;
            this->g = g;
            this->b = b;
            this->a = a;
        }
        operator glm::vec4()
        {
            return values;
        }
        operator float*()
        {
            return glm::value_ptr(values);
        }
        color_rgba& operator=(const color_rgba& other)
        {
            if (this != &other)
                values = other.values;
            return *this;
        }
        color_rgba& operator=(color_rgba&& other)
        {
            if (this != &other)
                values = other.values;
            return *this;
        }

#pragma warning(push)
#pragma warning(disable : 4201) // nonstandard extension used : nameless struct/union
        union
        {
            struct
            {
                float r, g, b, a;
            };

            vec4 values;
        };
#pragma warning(pop)
        //! \endcond
    };

    //! \brief The default intensity of a directional light. Is approx. the intensity of the sun.
    const float default_directional_intensity = 110000.0f;
    //! \brief The default intensity of a skylight. Is approx. the intensity of a sunny sky.
    const float default_skylight_intensity = 30000.0f;
    //! \brief The default intensity of a emissive object. // TODO Paul: Make something more meaningful.
    const float default_emissive_intensity = 3000.0f;

    //! \brief The minimum valid value for the camera aperture.
    const float min_camera_aperture = 0.5f;
    //! \brief The default value for the camera aperture.
    const float default_camera_aperture = 16.0f;
    //! \brief The maximum valid value for the camera aperture.
    const float max_camera_aperture = 64.0f;
    //! \brief The minimum valid value for the camera shutter speed.
    const float min_camera_shutter_speed = 1.0f / 25000.0f;
    //! \brief The default value for the camera shutter speed.
    const float default_camera_shutter_speed = 1.0f / 125.0f;
    //! \brief The maximum valid value for the camera shutter speed.
    const float max_camera_shutter_speed = 60.0f;
    //! \brief The minimum valid value for the camera iso.
    const float min_camera_iso = 10.0f;
    //! \brief The default value for the camera iso.
    const float default_camera_iso = 100.0f;
    //! \brief The maximum valid value for the camera iso.
    const float max_camera_iso = 204800.0f;

    //! \cond NO_COND

    template <typename e>
    struct bit_mask_operations
    {
        static const bool enable = false;
    };

//! \brief Macro used to enable safe bitmask operations on enum classes.
#define MANGO_ENABLE_BITMASK_OPERATIONS(e)   \
    template <>                              \
    struct bit_mask_operations<e>            \
    {                                        \
        static constexpr bool enable = true; \
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

} // namespace mango

#endif // MANGO_TYPES_HPP