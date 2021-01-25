//! \file      types.hpp
//! This file has different typedefs mostly for convenience and some perfect forwarding helper functions for standard types as well as some helpful macro definitions.
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_TYPES_HPP
#define MANGO_TYPES_HPP

#include <functional>
//! \cond NO_COND
#define GLM_FORCE_SILENT_WARNINGS 1
//! \endcond
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <limits>
#include <mango/assert.hpp>
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

    //! \brief Type alias for an uintptr_t.
    using uintptr = ::uintptr_t;

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

    //! \brief Describes the topology of primitives used for rendering and interpreting geometry data.
    //! \details Same as OpenGL.
    enum class primitive_topology : uint8
    {
        points,
        lines,
        line_loop,
        line_strip,
        triangles,
        triangle_strip,
        triangle_fan,
        quads
    };

    //! \brief The data type in index buffers.
    enum class index_type : uint32
    {
        none   = 0x0000,
        ubyte  = 0x1401,
        ushort = 0x1403,
        uint   = 0x1405
    };

//! \brief Pi.
#define PI 3.1415926535897932384626433832795f
//! \brief Pi times two.
#define TWO_PI 2.0f * PI
//! \brief Define for the global up vector.
#define GLOBAL_UP glm::vec3(0.0f, 1.0f, 0.0f)
//! \brief Define for the global right vector.
#define GLOBAL_RIGHT glm::vec3(1.0f, 0.0f, 0.0f)
//! \brief Define for the global forward vector.
#define GLOBAL_FORWARD glm::vec3(0.0f, 0.0f, -1.0f)
//! \brief Define for the global unit vector.
#define GLOBAL_UNIT glm::vec3(1.0f)

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

    //! \brief A 3d vector type used to describe rgb color properties.
    struct color_rgb
    {
        //! \cond NO_COND
        color_rgb(glm::vec3 v = glm::vec3(0.0f))
        {
            // MANGO_ASSERT(v.x >= 0.0f && v.x <= 1.0f, "r value is not normalized (between 0.0f and 1.0f)!");
            // MANGO_ASSERT(v.y >= 0.0f && v.y <= 1.0f, "g value is not normalized (between 0.0f and 1.0f)!");
            // MANGO_ASSERT(v.z >= 0.0f && v.z <= 1.0f, "b value is not normalized (between 0.0f and 1.0f)!");
            values = v;
        }
        color_rgb(float v)
        {
            // MANGO_ASSERT(v.x >= 0.0f && v.x <= 1.0f, "r value is not normalized (between 0.0f and 1.0f)!");
            // MANGO_ASSERT(v.y >= 0.0f && v.y <= 1.0f, "g value is not normalized (between 0.0f and 1.0f)!");
            // MANGO_ASSERT(v.z >= 0.0f && v.z <= 1.0f, "b value is not normalized (between 0.0f and 1.0f)!");
            values = glm::vec3(v);
        }
        operator glm::vec3()
        {
            return values;
        }
        operator float*()
        {
            return glm::value_ptr(values);
        }
        inline float& r()
        {
            return values.r;
        }
        inline float& g()
        {
            return values.g;
        }
        inline float& b()
        {
            return values.b;
        }
        color_rgb& operator=(const color_rgb& other)
        {
            // MANGO_ASSERT(other.values.x >= 0.0f && other.values.x <= 1.0f, "r value is not normalized (between 0.0f and 1.0f)!");
            // MANGO_ASSERT(other.values.y >= 0.0f && other.values.y <= 1.0f, "g value is not normalized (between 0.0f and 1.0f)!");
            // MANGO_ASSERT(other.values.z >= 0.0f && other.values.z <= 1.0f, "b value is not normalized (between 0.0f and 1.0f)!");
            if (this != &other)
                values = other.values;
            return *this;
        }
        color_rgb& operator=(color_rgb&& other)
        {
            // MANGO_ASSERT(other.values.x >= 0.0f && other.values.x <= 1.0f, "r value is not normalized (between 0.0f and 1.0f)!");
            // MANGO_ASSERT(other.values.y >= 0.0f && other.values.y <= 1.0f, "g value is not normalized (between 0.0f and 1.0f)!");
            // MANGO_ASSERT(other.values.z >= 0.0f && other.values.z <= 1.0f, "b value is not normalized (between 0.0f and 1.0f)!");
            if (this != &other)
                values = other.values;
            return *this;
        }

      private:
        glm::vec3 values;
        //! \endcond
    };

    //! \brief A 4d vector type used to describe rgba color properties.
    struct color_rgba
    {
        //! \cond NO_COND
        color_rgba(glm::vec4 v = glm::vec4(0.0f))
        {
            // MANGO_ASSERT(v.x >= 0.0f && v.x <= 1.0f, "r value is not normalized (between 0.0f and 1.0f)!");
            // MANGO_ASSERT(v.y >= 0.0f && v.y <= 1.0f, "g value is not normalized (between 0.0f and 1.0f)!");
            // MANGO_ASSERT(v.z >= 0.0f && v.z <= 1.0f, "b value is not normalized (between 0.0f and 1.0f)!");
            // MANGO_ASSERT(v.z >= 0.0f && v.z <= 1.0f, "a value is not normalized (between 0.0f and 1.0f)!");
            values = v;
        }
        color_rgba(float v)
        {
            // MANGO_ASSERT(v.x >= 0.0f && v.x <= 1.0f, "r value is not normalized (between 0.0f and 1.0f)!");
            // MANGO_ASSERT(v.y >= 0.0f && v.y <= 1.0f, "g value is not normalized (between 0.0f and 1.0f)!");
            // MANGO_ASSERT(v.z >= 0.0f && v.z <= 1.0f, "b value is not normalized (between 0.0f and 1.0f)!");
            // MANGO_ASSERT(v.z >= 0.0f && v.z <= 1.0f, "a value is not normalized (between 0.0f and 1.0f)!");
            values = glm::vec4(v);
        }
        operator glm::vec4()
        {
            return values;
        }
        operator float*()
        {
            return glm::value_ptr(values);
        }
        inline float& r()
        {
            return values.r;
        }
        inline float& g()
        {
            return values.g;
        }
        inline float& b()
        {
            return values.b;
        }
        inline float& a()
        {
            return values.a;
        }
        color_rgba& operator=(const color_rgba& other)
        {
            // MANGO_ASSERT(other.values.x >= 0.0f && other.values.x <= 1.0f, "r value is not normalized (between 0.0f and 1.0f)!");
            // MANGO_ASSERT(other.values.y >= 0.0f && other.values.y <= 1.0f, "g value is not normalized (between 0.0f and 1.0f)!");
            // MANGO_ASSERT(other.values.z >= 0.0f && other.values.z <= 1.0f, "b value is not normalized (between 0.0f and 1.0f)!");
            // MANGO_ASSERT(other.values.z >= 0.0f && other.values.z <= 1.0f, "a value is not normalized (between 0.0f and 1.0f)!");
            if (this != &other)
                values = other.values;
            return *this;
        }
        color_rgba& operator=(color_rgba&& other)
        {
            // MANGO_ASSERT(other.values.x >= 0.0f && other.values.x <= 1.0f, "r value is not normalized (between 0.0f and 1.0f)!");
            // MANGO_ASSERT(other.values.y >= 0.0f && other.values.y <= 1.0f, "g value is not normalized (between 0.0f and 1.0f)!");
            // MANGO_ASSERT(other.values.z >= 0.0f && other.values.z <= 1.0f, "b value is not normalized (between 0.0f and 1.0f)!");
            // MANGO_ASSERT(other.values.z >= 0.0f && other.values.z <= 1.0f, "a value is not normalized (between 0.0f and 1.0f)!");
            if (this != &other)
                values = other.values;
            return *this;
        }

      private:
        glm::vec4 values;
        //! \endcond
    };

    //! \brief Platform data holding the window handle.
    struct platform_data
    {
        void* native_window_handle; //!< The window handle. Platform dependent.
    };

    //! \brief The default intensity of a directional light. Is approx. the intensity of the sun.
    const float default_directional_intensity = 110000.0f;
    //! \brief The default intensity of a skylight. Is approx. the intensity of a sunny sky.
    const float default_skylight_intensity = 30000.0f;

    //! \brief Model type to identify lights.
    enum class light_model : uint8
    {
        directional, //!< Simple directional light type.
        skylight,    //!< Skylight type.
        atmosphere   //!< Atmospherical light type.
    };

    //! \brief Base class for all lights in mango.
    struct mango_light
    {
        //! \brief Model of the light.
        light_model model;
        mango_light() = delete;

      protected:
        //! \brief Constructs a base light with a type.
        //! \param[in] model The model type to use.
        mango_light(light_model model)
            : model(model)
        {
        }
    };

    //! \brief Directional light class.
    struct directional_light : mango_light
    {
        directional_light()
            : mango_light(light_model::directional)
            , direction(1.0f)
            , light_color(1.0f)
            , intensity(default_directional_intensity)
            , cast_shadows(false)
            , atmospherical(false)
        {
        }

        glm::vec3 direction;   //!< The light direction.
        color_rgb light_color; //!< The light color. Will get multiplied by the intensity.
        float intensity;       //!< The intensity of the light in lumen (111000 would f.e. be a basic sun)
        bool cast_shadows;     //!< True if the light should cast shadows.
        bool atmospherical;    //!< True if the light should contribute to atmosphere light.
    };

    class texture;
    //! \brief Sklight class.
    struct skylight : mango_light
    {
        skylight()
            : mango_light(light_model::skylight)
            , hdr_texture(0)
            , intensity(default_skylight_intensity)
            , use_texture(false)
            , dynamic(false)
            , local(false)
        {
        }

        shared_ptr<texture> hdr_texture; //!< The optional hdr texture.
        float intensity;                 //!< The intensity in lux (cd/m^2).
        bool use_texture;                //!< True if texture should be used, else false.
        bool dynamic;                    //!< True if the skylight should get automatic updates (reflection capture).
        bool local;                      //!< True if the skylight influences only a local area.
    };

    //! \brief Atmospherical light class.
    struct atmosphere_light : mango_light
    {
        atmosphere_light()
            : mango_light(light_model::atmosphere){};
        /*
        atmosphere_light()
            : mango_light(light_model::atmosphere)
            , intensity_multiplier(1.0f)
            , scatter_points(32)
            , scatter_points_second_ray(8)
            , rayleigh_scattering_coefficients(glm::vec3(5.8e-6f, 13.5e-6f, 33.1e-6f))
            , mie_scattering_coefficient(21e-6f)
            , density_multiplier(glm::vec2(8e3f, 1.2e3f))
            , ground_radius(6360e3f)
            , atmosphere_radius(6420e3f)
            , view_height(1e3f)
            , mie_preferred_scattering_dir(0.758f)

        {
        }

        float intensity_multiplier;
        // scattering parameters -> will be extended if necessary
        int32 scatter_points;
        int32 scatter_points_second_ray;
        glm::vec3 rayleigh_scattering_coefficients;
        float mie_scattering_coefficient;
        glm::vec2 density_multiplier;
        float ground_radius;
        float atmosphere_radius;
        float view_height;
        float mie_preferred_scattering_dir;
        */
    };

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