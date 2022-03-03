//! \file      types.hpp
//! This file has different typedefs mostly for convenience and some perfect forwarding helper functions for standard types as well as some helpful macro definitions.
//! \author    Paul Himmler
//! \version   1.0
//! \date      2022
//! \copyright Apache License 2.0

#ifndef MANGO_TYPES_HPP
#define MANGO_TYPES_HPP

#include <Eigen/Dense>
#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <mango/assert.hpp>
#include <memory>
#include <stdint.h>
#include <string>
#include <tl/optional.hpp>
#include <type_traits>

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

    //! \brief Type alias for a Eigen::Vector2i.
    using ivec2 = Eigen::Vector2i;

    //! \brief Type alias for a  Eigen::Vector3i.
    using ivec3 = Eigen::Vector3i;

    //! \brief Type alias for a Eigen::Vector4i.
    using ivec4 = Eigen::Vector4i;

    //! \brief Type alias for a Eigen::Vector2f.
    using vec2 = Eigen::Vector2f;

    //! \brief Type alias for a Eigen::Vector3f.
    using vec3 = Eigen::Vector3f;

    //! \brief Type alias for a Eigen::Vector4f.
    using vec4 = Eigen::Vector4f;

    //! \brief Type alias for a Eigen::Vector2d.
    using dvec2 = Eigen::Vector2d;

    //! \brief Type alias for a Eigen::Vector3d.
    using dvec3 = Eigen::Vector3d;

    //! \brief Type alias for a Eigen::Vector4d.
    using dvec4 = Eigen::Vector4d;

    //! \brief Type alias for a Eigen::Quaternionf.
    using quat = Eigen::Quaternionf;

    //! \brief Type alias for a Eigen::Quaterniond.
    using dquat = Eigen::Quaterniond;

    //! \brief Type alias for a Eigen::Matrix2f.
    using mat2 = Eigen::Matrix2f;

    //! \brief Type alias for a Eigen::Matrix3f.
    using mat3 = Eigen::Matrix3f;

    //! \brief Type alias for a Eigen::Matrix4f.
    using mat4 = Eigen::Matrix4f;

    //! \brief Type alias for a Eigen::Matrix2d.
    using dmat2 = Eigen::Matrix2d;

    //! \brief Type alias for a Eigen::Matrix3d.
    using dmat3 = Eigen::Matrix3d;

    //! \brief Type alias for a Eigen::Matrix4d.
    using dmat4 = Eigen::Matrix4d;

    //! \brief Create a \a vec3 from one value.
    //! \param[in] value The value to fill the \a vec3 with.
    //! \return The created \a vec3.
    inline vec3 make_vec3(const float& value)
    {
        vec3 v;
        v << value, value, value;
        return v;
    }

    //! \brief Create a \a vec4 from one value.
    //! \param[in] value The value to fill the \a vec4 with.
    //! \return The created \a vec4.
    inline vec4 make_vec4(const float& value)
    {
        vec4 v;
        v << value, value, value, value;
        return v;
    }

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
#define PI 3.1415926535897932384626433832795
//! \brief Pi times two.
#define TWO_PI 2.0 * PI
//! \brief Define for the global up vector.
#define GLOBAL_UP vec3(0.0f, 1.0f, 0.0f)
//! \brief Define for the global right vector.
#define GLOBAL_RIGHT vec3(1.0f, 0.0f, 0.0f)
//! \brief Define for the global forward vector.
#define GLOBAL_FORWARD vec3(0.0f, 0.0f, -1.0f)
//! \brief Define for the global unit vector.
#define GLOBAL_UNIT vec3(1.0f)

    //! \brief An uid used to index everything.
    struct uid
    {
        //! \brief Retrieves the internal lookup id.
        //! \return A constant reference to the internal lookup id.
        inline const uint32& get() const
        {
            return lookup_id;
        }

        uid()
            : lookup_id(0)
            , valid(false){};
        ~uid() = default;
        //! \brief Copy constructor.
        uid(const uid&) = default;
        //! \brief Move constructor.
        uid(uid&&) = default;
        //! \brief Assignment operator.
        uid& operator=(const uid&) = default;
        //! \brief Move assignment operator.
        uid& operator=(uid&&) = default;

        //! \brief Comparison operator equal.
        //! \param other The other \a uid.
        //! \return True if other \a uid is equal to the current one, else false.
        bool operator==(const uid& other) const
        {
            return lookup_id == other.lookup_id;
        }

        //! \brief Comparison operator not equal.
        //! \param other The other \a uid.
        //! \return True if other \a uid is not equal to the current one, else false.
        bool operator!=(const uid& other) const
        {
            return lookup_id != other.lookup_id;
        }

        //! \brief Comparison operator less.
        //! \param other The other \a uid.
        //! \return True if other \a uid is less then the current one, else false.
        bool operator<(const uid& other) const
        {
            return lookup_id < other.lookup_id;
        }

        //! \brief Comparison operator less or equal.
        //! \param other The other \a uid.
        //! \return True if other \a uid is less or equal then the current one, else false.
        bool operator<=(const uid& other) const
        {
            return lookup_id <= other.lookup_id;
        }

        //! \brief Returns if the \a uid is valid or not.
        //! \return True if the \a uid is valid else false.
        const bool is_valid() const
        {
            return valid;
        }

      private:
        template <typename T, ptr_size C>
        friend class packed_freelist;

        //! \brief Id of the lookup.
        //! \details 16 least significant bits = index of this lookup in a packed freelist lookup array / 16 most significant bits = usage count of this lookup.
        uint32 lookup_id;

        //! \brief True if \a uid is valid, else false.
        bool valid;
    };

    //! \brief Invalid \a uid.
    static const uid invalid_uid;

    //! \brief Hash for the \a uid structure.
    struct uid_hash
    {
        //! \brief Function call operator.
        //! \details Hashes the \a uid.
        //! \param[in] k The \a uid to hash.
        //! \return The hash for the given \a uid.
        std::size_t operator()(const uid& k) const
        {
            // https://stackoverflow.com/questions/1646807/quick-and-simple-hash-code-combinations/

            size_t res = 17;
            res        = res * 31 + std::hash<uint32>()(k.get());

            return res;
        };
    };

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
        normalized_float(const normalized_float& other)
        {
            MANGO_ASSERT(other.value >= 0.0f && other.value <= 1.0f, "Value is not normalized (between 0.0f and 1.0f)!");
            value = other.value;
        }
        normalized_float(normalized_float&& other)
        {
            MANGO_ASSERT(other.value >= 0.0f && other.value <= 1.0f, "Value is not normalized (between 0.0f and 1.0f)!");
            value = other.value;
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
        color_rgb(vec3 v = make_vec3(0.0f))
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
            values = make_vec3(v);
        }
        color_rgb(float r, float g, float b)
        {
            values = vec3(r, g, b);
        }
        vec3& as_vec3()
        {
            return values;
        }
        const vec3& as_vec3() const
        {
            return values;
        }
        operator float*()
        {
            return values.data();
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

        float& r()
        {
            return values.x();
        }

        float& g()
        {
            return values.y();
        }

        float& b()
        {
            return values.z();
        }

      private:
        vec3 values;
        //! \endcond
    };

    //! \brief A 4d vector type used to describe rgba color properties. Values should be between 0.0 and 1.0 unless color is a hdr one.
    struct color_rgba
    {
        //! \cond NO_COND
        color_rgba(vec4 v = make_vec4(0.0f))
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
            values = make_vec4(v);
        }
        color_rgba(float r, float g, float b, float a)
        {
            values = vec4(r, g, b, a);
        }
        vec4& as_vec4()
        {
            return values;
        }
        const vec4& as_vec4() const
        {
            return values;
        }
        operator float*()
        {
            return values.data();
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

        float& r()
        {
            return values.x();
        }

        float& g()
        {
            return values.y();
        }

        float& b()
        {
            return values.z();
        }

        float& a()
        {
            return values.w();
        }

      private:
        vec4 values;
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

    // Utility functions

    // TODO Paul: We might move such utility functions.
    inline float deg_to_rad(const float& degrees)
    {
        return degrees * (static_cast<float>(PI) / 180.0f);
    }

    inline float rad_to_deg(const float& radians)
    {
        return radians * (180.0f / static_cast<float>(PI));
    }

    inline vec3 deg_to_rad(const vec3& degrees)
    {
        return degrees * (PI / 180.0f);
    }

    inline vec3 rad_to_deg(const vec3& radians)
    {
        return radians * (180.0f / PI);
    }

    inline vec4 deg_to_rad(const vec4& degrees)
    {
        return degrees * (PI / 180.0f);
    }

    inline vec4 rad_to_deg(const vec4& radians)
    {
        return radians * (180.0f / PI);
    }

    template <typename T>
    constexpr const T& clamp(const T& v, const T& lo, const T& hi)
    {
        return clamp(v, lo, hi, std::less<>());
    }

    template <typename T, typename Compare>
    constexpr const T& clamp(const T& v, const T& lo, const T& hi, Compare comp)
    {
        return comp(v, lo) ? lo : comp(hi, v) ? hi : v;
    }

    inline vec3 abs(const vec3& v)
    {
        return vec3(std::abs(v.x()), std::abs(v.y()), std::abs(v.z()));
    }

    inline vec4 abs(const vec4& v)
    {
        return vec4(std::abs(v.x()), std::abs(v.y()), std::abs(v.z()), std::abs(v.w()));
    }

    template <typename T, typename U>
    typename std::common_type<T, U>::type min(const T& a, const U& b)
    {
        return (a < b) ? a : b;
    }

    template <typename T, typename U>
    typename std::common_type<T, U>::type max(const T& a, const U& b)
    {
        return (a > b) ? a : b;
    }

    template <>
    inline vec3 min<vec3, vec3>(const vec3& a, const vec3& b)
    {
        return vec3(mango::min(a.x(), b.x()), mango::min(a.y(), b.y()), mango::min(a.z(), b.z()));
    }

    template <>
    inline vec3 max<vec3, vec3>(const vec3& a, const vec3& b)
    {
        return vec3(mango::max(a.x(), b.x()), mango::max(a.y(), b.y()), mango::max(a.z(), b.z()));
    }

    template <>
    inline vec4 min<vec4, vec4>(const vec4& a, const vec4& b)
    {
        return vec4(mango::min(a.x(), b.x()), mango::min(a.y(), b.y()), mango::min(a.z(), b.z()), mango::min(a.w(), b.w()));
    }

    template <>
    inline vec4 max<vec4, vec4>(const vec4& a, const vec4& b)
    {
        return vec4(mango::max(a.x(), b.x()), mango::max(a.y(), b.y()), mango::max(a.z(), b.z()), mango::max(a.w(), b.w()));
    }

    template <typename Scalar>
    Eigen::Matrix<Scalar, 4, 4> perspective(Scalar fovy, Scalar aspect, Scalar zNear, Scalar zFar)
    {
        Eigen::Transform<Scalar, 3, Eigen::Projective> tr;
        tr.matrix().setZero();
        assert(aspect > 0);
        assert(zFar > zNear);
        assert(zNear > 0);
        Scalar tan_half_fovy = std::tan(fovy / static_cast<Scalar>(2));
        tr(0, 0)             = static_cast<Scalar>(1) / (aspect * tan_half_fovy);
        tr(1, 1)             = static_cast<Scalar>(1) / (tan_half_fovy);
        tr(2, 2)             = -(zFar + zNear) / (zFar - zNear);
        tr(3, 2)             = -static_cast<Scalar>(1);
        tr(2, 3)             = -(static_cast<Scalar>(2) * zFar * zNear) / (zFar - zNear);
        return tr.matrix();
    }

    template <typename Scalar>
    Eigen::Matrix<Scalar, 4, 4> ortho(const Scalar& left, const Scalar& right, const Scalar& bottom, const Scalar& top, const Scalar& zNear, const Scalar& zFar)
    {
        Eigen::Matrix<Scalar, 4, 4> mat = Eigen::Matrix<Scalar, 4, 4>::Identity();
        mat(0, 0)                       = static_cast<Scalar>(2) / (right - left);
        mat(1, 1)                       = static_cast<Scalar>(2) / (top - bottom);
        mat(2, 2)                       = -static_cast<Scalar>(2) / (zFar - zNear);
        mat(0, 3)                       = -(right + left) / (right - left);
        mat(1, 3)                       = -(top + bottom) / (top - bottom);
        mat(2, 3)                       = -(zFar + zNear) / (zFar - zNear);
        return mat;
    }

    template <typename Derived>
    Eigen::Matrix<typename Derived::Scalar, 4, 4> lookAt(const Derived& eye, const Derived& center, const Derived& up)
    {
        typedef Eigen::Matrix<typename Derived::Scalar, 4, 4> Matrix4;
        typedef Eigen::Matrix<typename Derived::Scalar, 3, 1> Vector3;
        Vector3 f   = (center - eye).normalized();
        Vector3 u   = up.normalized();
        Vector3 s   = f.cross(u).normalized();
        u           = s.cross(f);
        Matrix4 mat = Matrix4::Zero();
        mat(0, 0)   = s.x();
        mat(0, 1)   = s.y();
        mat(0, 2)   = s.z();
        mat(1, 0)   = u.x();
        mat(1, 1)   = u.y();
        mat(1, 2)   = u.z();
        mat(2, 0)   = -f.x();
        mat(2, 1)   = -f.y();
        mat(2, 2)   = -f.z();
        mat(0, 3)   = -s.dot(eye);
        mat(1, 3)   = -u.dot(eye);
        mat(2, 3)   = f.dot(eye);
        mat.row(3) << 0, 0, 0, 1;
        return mat;
    }

    template <typename Scalar>
    Eigen::Matrix<Scalar, 4, 4> scale(Scalar x, Scalar y, Scalar z)
    {
        Eigen::Transform<Scalar, 3, Eigen::Affine> tr;
        tr.matrix().setZero();
        tr(0, 0) = x;
        tr(1, 1) = y;
        tr(2, 2) = z;
        tr(3, 3) = 1;
        return tr.matrix();
    }

    template <typename Scalar>
    Eigen::Matrix<Scalar, 4, 4> translate(Scalar x, Scalar y, Scalar z)
    {
        Eigen::Transform<Scalar, 3, Eigen::Affine> tr;
        tr.matrix().setIdentity();
        tr(0, 3) = x;
        tr(1, 3) = y;
        tr(2, 3) = z;
        return tr.matrix();
    }

    template <typename Scalar>
    Eigen::Matrix<Scalar, 4, 4> scale(const Eigen::Vector3<Scalar>& scale)
    {
        Eigen::Transform<Scalar, 3, Eigen::Affine> tr;
        tr.matrix().setZero();
        tr(0, 0) = scale.x();
        tr(1, 1) = scale.y();
        tr(2, 2) = scale.z();
        tr(3, 3) = 1;
        return tr.matrix();
    }

    template <typename Scalar>
    Eigen::Matrix<Scalar, 4, 4> translate(const Eigen::Vector3<Scalar>& translation)
    {
        Eigen::Transform<Scalar, 3, Eigen::Affine> tr;
        tr.matrix().setIdentity();
        tr(0, 3) = translation.x();
        tr(1, 3) = translation.y();
        tr(2, 3) = translation.z();
        return tr.matrix();
    }

    inline mat4 quaternion_to_mat4(const quat& rotation)
    {
        mat4 m              = Eigen::Matrix4f::Identity();
        m.block(0, 0, 3, 3) = rotation.toRotationMatrix();
        return m;
    }

    inline void decompose_transformation(const mat4& input, vec3& out_scale, quat& out_rotation, vec3& out_translation)
    {
        out_translation = input.col(3).head<3>();
        mat3 rot        = input.block(0, 0, 3, 3);
        out_scale.x()   = rot.col(0).norm();
        out_scale.y()   = rot.col(1).norm();
        out_scale.z()   = (rot.determinant() < 0.0 ? -1 : 1) * rot.col(2).norm();

        rot.col(0) /= out_scale.x();
        rot.col(1) /= out_scale.y();
        rot.col(2) /= out_scale.z();

        out_rotation = quat(rot);
    }
} // namespace mango

#endif // MANGO_TYPES_HPP