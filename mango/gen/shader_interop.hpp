//! \file      shader_interop.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2022
//! \copyright Apache License 2.0

#ifndef MANGO_SHADER_INTEROP_HPP
#define MANGO_SHADER_INTEROP_HPP

#include <mango/types.hpp>
#include <mango/assert.hpp>

namespace mango
{

// This file is generated. Do NOT change until you want to loose it!

using std140_uint32 = uint32;
using std140_int32  = int32;
using std140_float  = float;
using std140_double = double;
using std140_vec2   = vec2;
using std140_vec3   = vec3;
using std140_vec4   = vec4;
using std140_ivec2  = ivec2;
using std140_ivec3  = ivec3;
using std140_ivec4  = ivec4;
using std140_uvec2  = uvec2;
using std140_uvec3  = uvec3;
using std140_uvec4  = uvec4;
using std140_dvec2  = dvec2;
using std140_dvec3  = dvec3;
using std140_dvec4  = dvec4;
using std140_mat2   = mat2;
using std140_mat4   = mat4;
using std140_dmat2  = dmat2;
using std140_dmat4  = dmat4;

struct std140_bool
{
    std140_bool(const bool& b)
    {
        pad = 0;
        v   = b;
    }
    std140_bool()
    {
        pad = 0;
        v   = false;
    }
    operator bool&()
    {
        return v;
    }
    void operator=(const bool& o)
    {
        pad = 0;
        v   = o;
    }

  private:
    union
    {
        bool v;
        uint32 pad;
    };
};

using std140_bvec2 = Eigen::Vector2<std140_bool>;
using std140_bvec3 = Eigen::Vector3<std140_bool>;
using std140_bvec4 = Eigen::Vector4<std140_bool>;

template <typename T>
struct std140_matrix3
{
    std140_matrix3(const Eigen::Matrix3<T>& mat)
        : m(Eigen::Matrix3<T>::Zero())
    {
        m.block<0, 0>(3, 3) = mat;
    }
    std140_matrix3()
        : m(Eigen::Matrix<T, 4, 3>::Zero())
    {
    }
    void operator=(const Eigen::Matrix3<T>& o)
    {
        m.block<3, 3>(0, 0) = o;
    }
    operator Eigen::Matrix3<T>&()
    {
        return m.block<3, 3>(0, 0);
    }

  private:
    Eigen::Matrix<T, 4, 3> m;
};

using std140_mat3  = std140_matrix3<float>;
using std140_dmat3 = std140_matrix3<double>;
template <typename T, ptr_size N>
struct std140_array
{
    static_assert(
                  std::is_same<std140_uint32, T>::value ||
                  std::is_same<std140_int32, T>::value ||
                  std::is_same<std140_float, T>::value ||
                  std::is_same<std140_double, T>::value ||
                  std::is_same<std140_bool, T>::value ||
                  std::is_same<std140_vec2, T>::value ||
                  std::is_same<std140_vec3, T>::value ||
                  std::is_same<std140_vec4, T>::value ||
                  std::is_same<std140_ivec2, T>::value ||
                  std::is_same<std140_ivec3, T>::value ||
                  std::is_same<std140_ivec4, T>::value ||
                  std::is_same<std140_uvec2, T>::value ||
                  std::is_same<std140_uvec3, T>::value ||
                  std::is_same<std140_uvec4, T>::value ||
                  std::is_same<std140_dvec2, T>::value ||
                  std::is_same<std140_dvec3, T>::value ||
                  std::is_same<std140_dvec4, T>::value ||
                  std::is_same<std140_bvec2, T>::value ||
                  std::is_same<std140_bvec3, T>::value ||
                  std::is_same<std140_bvec4, T>::value ||
                  std::is_same<std140_mat2, T>::value ||
                  std::is_same<std140_mat3, T>::value ||
                  std::is_same<std140_mat4, T>::value ||
                  std::is_same<std140_dmat2, T>::value ||
                  std::is_same<std140_dmat3, T>::value ||
                  std::is_same<std140_dmat4, T>::value ||
                  0
                  , "Type is not allowed.");
};

template <ptr_size N>
struct std140_array<std140_uint32, N>
{
    void fill_from_list(const uint32* list, uint32 count)
    {
        MANGO_ASSERT(count == N, "List size not correct!");
        for (uint32 i = 0; i < N; ++i)
        {
            data[i] = internal_element(list[i]);
        }
    }

    uint32& operator[](uint32 i)
    {
        MANGO_ASSERT(i < N, "Index out of bounds!");
        return data[i].value;
    }

    std140_array() = default;
    ~std140_array() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const std140_uint32& v)
        {
            value = v;
        };
        std140_uint32 value;
        std140_uint32 pad0 = 0;
        std140_uint32 pad1 = 0;
        std140_uint32 pad2 = 0;
    };

    internal_element data[N];
};

template <ptr_size N>
struct std140_array<std140_int32, N>
{
    void fill_from_list(const int32* list, uint32 count)
    {
        MANGO_ASSERT(count == N, "List size not correct!");
        for (uint32 i = 0; i < N; ++i)
        {
            data[i] = internal_element(list[i]);
        }
    }

    int32& operator[](uint32 i)
    {
        MANGO_ASSERT(i < N, "Index out of bounds!");
        return data[i].value;
    }

    std140_array() = default;
    ~std140_array() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const std140_int32& v)
        {
            value = v;
        };
        std140_int32 value;
        std140_uint32 pad0 = 0;
        std140_uint32 pad1 = 0;
        std140_uint32 pad2 = 0;
    };

    internal_element data[N];
};

template <ptr_size N>
struct std140_array<std140_float, N>
{
    void fill_from_list(const float* list, uint32 count)
    {
        MANGO_ASSERT(count == N, "List size not correct!");
        for (uint32 i = 0; i < N; ++i)
        {
            data[i] = internal_element(list[i]);
        }
    }

    float& operator[](uint32 i)
    {
        MANGO_ASSERT(i < N, "Index out of bounds!");
        return data[i].value;
    }

    std140_array() = default;
    ~std140_array() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const std140_float& v)
        {
            value = v;
        };
        std140_float value;
        std140_uint32 pad0 = 0;
        std140_uint32 pad1 = 0;
        std140_uint32 pad2 = 0;
    };

    internal_element data[N];
};

template <ptr_size N>
struct std140_array<std140_double, N>
{
    void fill_from_list(const double* list, uint32 count)
    {
        MANGO_ASSERT(count == N, "List size not correct!");
        for (uint32 i = 0; i < N; ++i)
        {
            data[i] = internal_element(list[i]);
        }
    }

    double& operator[](uint32 i)
    {
        MANGO_ASSERT(i < N, "Index out of bounds!");
        return data[i].value;
    }

    std140_array() = default;
    ~std140_array() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const std140_double& v)
        {
            value = v;
        };
        std140_double value;
        std140_uint32 pad0 = 0;
        std140_uint32 pad1 = 0;
    };

    internal_element data[N];
};

template <ptr_size N>
struct std140_array<std140_bool, N>
{
    void fill_from_list(const bool* list, uint32 count)
    {
        MANGO_ASSERT(count == N, "List size not correct!");
        for (uint32 i = 0; i < N; ++i)
        {
            data[i] = internal_element(list[i]);
        }
    }

    bool& operator[](uint32 i)
    {
        MANGO_ASSERT(i < N, "Index out of bounds!");
        return data[i].value;
    }

    std140_array() = default;
    ~std140_array() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const std140_bool& v)
        {
            value = v;
        };
        std140_bool value;
        std140_uint32 pad0 = 0;
        std140_uint32 pad1 = 0;
        std140_uint32 pad2 = 0;
    };

    internal_element data[N];
};

template <ptr_size N>
struct std140_array<std140_vec2, N>
{
    void fill_from_list(const vec2* list, uint32 count)
    {
        MANGO_ASSERT(count == N, "List size not correct!");
        for (uint32 i = 0; i < N; ++i)
        {
            data[i] = internal_element(list[i]);
        }
    }

    vec2& operator[](uint32 i)
    {
        MANGO_ASSERT(i < N, "Index out of bounds!");
        return data[i].value;
    }

    std140_array() = default;
    ~std140_array() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const std140_vec2& v)
        {
            value = v;
        };
        std140_vec2 value;
        std140_uint32 pad0 = 0;
        std140_uint32 pad1 = 0;
    };

    internal_element data[N];
};

template <ptr_size N>
struct std140_array<std140_vec3, N>
{
    void fill_from_list(const vec3* list, uint32 count)
    {
        MANGO_ASSERT(count == N, "List size not correct!");
        for (uint32 i = 0; i < N; ++i)
        {
            data[i] = internal_element(list[i]);
        }
    }

    vec3& operator[](uint32 i)
    {
        MANGO_ASSERT(i < N, "Index out of bounds!");
        return data[i].value;
    }

    std140_array() = default;
    ~std140_array() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const std140_vec3& v)
        {
            value = v;
        };
        std140_vec3 value;
        std140_uint32 pad0 = 0;
    };

    internal_element data[N];
};

template <ptr_size N>
struct std140_array<std140_vec4, N>
{
    void fill_from_list(const vec4* list, uint32 count)
    {
        MANGO_ASSERT(count == N, "List size not correct!");
        for (uint32 i = 0; i < N; ++i)
        {
            data[i] = internal_element(list[i]);
        }
    }

    vec4& operator[](uint32 i)
    {
        MANGO_ASSERT(i < N, "Index out of bounds!");
        return data[i].value;
    }

    std140_array() = default;
    ~std140_array() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const std140_vec4& v)
        {
            value = v;
        };
        std140_vec4 value;
    };

    internal_element data[N];
};

template <ptr_size N>
struct std140_array<std140_ivec2, N>
{
    void fill_from_list(const ivec2* list, uint32 count)
    {
        MANGO_ASSERT(count == N, "List size not correct!");
        for (uint32 i = 0; i < N; ++i)
        {
            data[i] = internal_element(list[i]);
        }
    }

    ivec2& operator[](uint32 i)
    {
        MANGO_ASSERT(i < N, "Index out of bounds!");
        return data[i].value;
    }

    std140_array() = default;
    ~std140_array() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const std140_ivec2& v)
        {
            value = v;
        };
        std140_ivec2 value;
        std140_uint32 pad0 = 0;
        std140_uint32 pad1 = 0;
    };

    internal_element data[N];
};

template <ptr_size N>
struct std140_array<std140_ivec3, N>
{
    void fill_from_list(const ivec3* list, uint32 count)
    {
        MANGO_ASSERT(count == N, "List size not correct!");
        for (uint32 i = 0; i < N; ++i)
        {
            data[i] = internal_element(list[i]);
        }
    }

    ivec3& operator[](uint32 i)
    {
        MANGO_ASSERT(i < N, "Index out of bounds!");
        return data[i].value;
    }

    std140_array() = default;
    ~std140_array() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const std140_ivec3& v)
        {
            value = v;
        };
        std140_ivec3 value;
        std140_uint32 pad0 = 0;
    };

    internal_element data[N];
};

template <ptr_size N>
struct std140_array<std140_ivec4, N>
{
    void fill_from_list(const ivec4* list, uint32 count)
    {
        MANGO_ASSERT(count == N, "List size not correct!");
        for (uint32 i = 0; i < N; ++i)
        {
            data[i] = internal_element(list[i]);
        }
    }

    ivec4& operator[](uint32 i)
    {
        MANGO_ASSERT(i < N, "Index out of bounds!");
        return data[i].value;
    }

    std140_array() = default;
    ~std140_array() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const std140_ivec4& v)
        {
            value = v;
        };
        std140_ivec4 value;
    };

    internal_element data[N];
};

template <ptr_size N>
struct std140_array<std140_uvec2, N>
{
    void fill_from_list(const uvec2* list, uint32 count)
    {
        MANGO_ASSERT(count == N, "List size not correct!");
        for (uint32 i = 0; i < N; ++i)
        {
            data[i] = internal_element(list[i]);
        }
    }

    uvec2& operator[](uint32 i)
    {
        MANGO_ASSERT(i < N, "Index out of bounds!");
        return data[i].value;
    }

    std140_array() = default;
    ~std140_array() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const std140_uvec2& v)
        {
            value = v;
        };
        std140_uvec2 value;
        std140_uint32 pad0 = 0;
        std140_uint32 pad1 = 0;
    };

    internal_element data[N];
};

template <ptr_size N>
struct std140_array<std140_uvec3, N>
{
    void fill_from_list(const uvec3* list, uint32 count)
    {
        MANGO_ASSERT(count == N, "List size not correct!");
        for (uint32 i = 0; i < N; ++i)
        {
            data[i] = internal_element(list[i]);
        }
    }

    uvec3& operator[](uint32 i)
    {
        MANGO_ASSERT(i < N, "Index out of bounds!");
        return data[i].value;
    }

    std140_array() = default;
    ~std140_array() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const std140_uvec3& v)
        {
            value = v;
        };
        std140_uvec3 value;
        std140_uint32 pad0 = 0;
    };

    internal_element data[N];
};

template <ptr_size N>
struct std140_array<std140_uvec4, N>
{
    void fill_from_list(const uvec4* list, uint32 count)
    {
        MANGO_ASSERT(count == N, "List size not correct!");
        for (uint32 i = 0; i < N; ++i)
        {
            data[i] = internal_element(list[i]);
        }
    }

    uvec4& operator[](uint32 i)
    {
        MANGO_ASSERT(i < N, "Index out of bounds!");
        return data[i].value;
    }

    std140_array() = default;
    ~std140_array() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const std140_uvec4& v)
        {
            value = v;
        };
        std140_uvec4 value;
    };

    internal_element data[N];
};

template <ptr_size N>
struct std140_array<std140_dvec2, N>
{
    void fill_from_list(const dvec2* list, uint32 count)
    {
        MANGO_ASSERT(count == N, "List size not correct!");
        for (uint32 i = 0; i < N; ++i)
        {
            data[i] = internal_element(list[i]);
        }
    }

    dvec2& operator[](uint32 i)
    {
        MANGO_ASSERT(i < N, "Index out of bounds!");
        return data[i].value;
    }

    std140_array() = default;
    ~std140_array() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const std140_dvec2& v)
        {
            value = v;
        };
        std140_dvec2 value;
    };

    internal_element data[N];
};

template <ptr_size N>
struct std140_array<std140_dvec3, N>
{
    void fill_from_list(const dvec3* list, uint32 count)
    {
        MANGO_ASSERT(count == N, "List size not correct!");
        for (uint32 i = 0; i < N; ++i)
        {
            data[i] = internal_element(list[i]);
        }
    }

    dvec3& operator[](uint32 i)
    {
        MANGO_ASSERT(i < N, "Index out of bounds!");
        return data[i].value;
    }

    std140_array() = default;
    ~std140_array() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const std140_dvec3& v)
        {
            value = v;
        };
        std140_dvec3 value;
        std140_uint32 pad0 = 0;
        std140_uint32 pad1 = 0;
    };

    internal_element data[N];
};

template <ptr_size N>
struct std140_array<std140_dvec4, N>
{
    void fill_from_list(const dvec4* list, uint32 count)
    {
        MANGO_ASSERT(count == N, "List size not correct!");
        for (uint32 i = 0; i < N; ++i)
        {
            data[i] = internal_element(list[i]);
        }
    }

    dvec4& operator[](uint32 i)
    {
        MANGO_ASSERT(i < N, "Index out of bounds!");
        return data[i].value;
    }

    std140_array() = default;
    ~std140_array() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const std140_dvec4& v)
        {
            value = v;
        };
        std140_dvec4 value;
    };

    internal_element data[N];
};

template <ptr_size N>
struct std140_array<std140_bvec2, N>
{
    void fill_from_list(const bvec2* list, uint32 count)
    {
        MANGO_ASSERT(count == N, "List size not correct!");
        for (uint32 i = 0; i < N; ++i)
        {
            data[i] = internal_element(list[i]);
        }
    }

    bvec2& operator[](uint32 i)
    {
        MANGO_ASSERT(i < N, "Index out of bounds!");
        return data[i].value;
    }

    std140_array() = default;
    ~std140_array() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const std140_bvec2& v)
        {
            value = v;
        };
        std140_bvec2 value;
        std140_uint32 pad0 = 0;
        std140_uint32 pad1 = 0;
    };

    internal_element data[N];
};

template <ptr_size N>
struct std140_array<std140_bvec3, N>
{
    void fill_from_list(const bvec3* list, uint32 count)
    {
        MANGO_ASSERT(count == N, "List size not correct!");
        for (uint32 i = 0; i < N; ++i)
        {
            data[i] = internal_element(list[i]);
        }
    }

    bvec3& operator[](uint32 i)
    {
        MANGO_ASSERT(i < N, "Index out of bounds!");
        return data[i].value;
    }

    std140_array() = default;
    ~std140_array() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const std140_bvec3& v)
        {
            value = v;
        };
        std140_bvec3 value;
        std140_uint32 pad0 = 0;
    };

    internal_element data[N];
};

template <ptr_size N>
struct std140_array<std140_bvec4, N>
{
    void fill_from_list(const bvec4* list, uint32 count)
    {
        MANGO_ASSERT(count == N, "List size not correct!");
        for (uint32 i = 0; i < N; ++i)
        {
            data[i] = internal_element(list[i]);
        }
    }

    bvec4& operator[](uint32 i)
    {
        MANGO_ASSERT(i < N, "Index out of bounds!");
        return data[i].value;
    }

    std140_array() = default;
    ~std140_array() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const std140_bvec4& v)
        {
            value = v;
        };
        std140_bvec4 value;
    };

    internal_element data[N];
};

template <ptr_size N>
struct std140_array<std140_mat2, N>
{
    void fill_from_list(const mat2* list, uint32 count)
    {
        MANGO_ASSERT(count == N, "List size not correct!");
        for (uint32 i = 0; i < N; ++i)
        {
            data[i] = internal_element(list[i]);
        }
    }

    mat2& operator[](uint32 i)
    {
        MANGO_ASSERT(i < N, "Index out of bounds!");
        return data[i].value;
    }

    std140_array() = default;
    ~std140_array() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const std140_mat2& v)
        {
            value = v;
        };
        std140_mat2 value;
    };

    internal_element data[N];
};

template <ptr_size N>
struct std140_array<std140_mat3, N>
{
    void fill_from_list(const mat3* list, uint32 count)
    {
        MANGO_ASSERT(count == N, "List size not correct!");
        for (uint32 i = 0; i < N; ++i)
        {
            data[i] = internal_element(list[i]);
        }
    }

    mat3& operator[](uint32 i)
    {
        MANGO_ASSERT(i < N, "Index out of bounds!");
        return data[i].value;
    }

    std140_array() = default;
    ~std140_array() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const std140_mat3& v)
        {
            value = v;
        };
        std140_mat3 value;
        std140_uint32 pad0 = 0;
        std140_uint32 pad1 = 0;
        std140_uint32 pad2 = 0;
        std140_uint32 pad3 = 0;
        std140_uint32 pad4 = 0;
        std140_uint32 pad5 = 0;
        std140_uint32 pad6 = 0;
    };

    internal_element data[N];
};

template <ptr_size N>
struct std140_array<std140_mat4, N>
{
    void fill_from_list(const mat4* list, uint32 count)
    {
        MANGO_ASSERT(count == N, "List size not correct!");
        for (uint32 i = 0; i < N; ++i)
        {
            data[i] = internal_element(list[i]);
        }
    }

    mat4& operator[](uint32 i)
    {
        MANGO_ASSERT(i < N, "Index out of bounds!");
        return data[i].value;
    }

    std140_array() = default;
    ~std140_array() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const std140_mat4& v)
        {
            value = v;
        };
        std140_mat4 value;
    };

    internal_element data[N];
};

template <ptr_size N>
struct std140_array<std140_dmat2, N>
{
    void fill_from_list(const dmat2* list, uint32 count)
    {
        MANGO_ASSERT(count == N, "List size not correct!");
        for (uint32 i = 0; i < N; ++i)
        {
            data[i] = internal_element(list[i]);
        }
    }

    dmat2& operator[](uint32 i)
    {
        MANGO_ASSERT(i < N, "Index out of bounds!");
        return data[i].value;
    }

    std140_array() = default;
    ~std140_array() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const std140_dmat2& v)
        {
            value = v;
        };
        std140_dmat2 value;
    };

    internal_element data[N];
};

template <ptr_size N>
struct std140_array<std140_dmat3, N>
{
    void fill_from_list(const dmat3* list, uint32 count)
    {
        MANGO_ASSERT(count == N, "List size not correct!");
        for (uint32 i = 0; i < N; ++i)
        {
            data[i] = internal_element(list[i]);
        }
    }

    dmat3& operator[](uint32 i)
    {
        MANGO_ASSERT(i < N, "Index out of bounds!");
        return data[i].value;
    }

    std140_array() = default;
    ~std140_array() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const std140_dmat3& v)
        {
            value = v;
        };
        std140_dmat3 value;
        std140_uint32 pad0 = 0;
        std140_uint32 pad1 = 0;
        std140_uint32 pad2 = 0;
        std140_uint32 pad3 = 0;
        std140_uint32 pad4 = 0;
        std140_uint32 pad5 = 0;
        std140_uint32 pad6 = 0;
        std140_uint32 pad7 = 0;
        std140_uint32 pad8 = 0;
        std140_uint32 pad9 = 0;
        std140_uint32 pad10 = 0;
        std140_uint32 pad11 = 0;
        std140_uint32 pad12 = 0;
        std140_uint32 pad13 = 0;
    };

    internal_element data[N];
};

template <ptr_size N>
struct std140_array<std140_dmat4, N>
{
    void fill_from_list(const dmat4* list, uint32 count)
    {
        MANGO_ASSERT(count == N, "List size not correct!");
        for (uint32 i = 0; i < N; ++i)
        {
            data[i] = internal_element(list[i]);
        }
    }

    dmat4& operator[](uint32 i)
    {
        MANGO_ASSERT(i < N, "Index out of bounds!");
        return data[i].value;
    }

    std140_array() = default;
    ~std140_array() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const std140_dmat4& v)
        {
            value = v;
        };
        std140_dmat4 value;
    };

    internal_element data[N];
};


struct camera_data
{
    std140_mat4 view_matrix;
    std140_mat4 projection_matrix;
    std140_mat4 view_projection_matrix;
    std140_mat4 inverse_view_projection;
    std140_vec3 camera_position;
    std140_float camera_near;
    std140_float camera_far;
    std140_float camera_exposure;
};

struct light_data
{
    std140_vec3 directional_light_direction;
    std140_uint32 pad0;
    std140_vec3 directional_light_color;
    std140_float directional_light_intensity;
    std140_bool directional_light_cast_shadows;
    std140_bool directional_light_valid;
    std140_float skylight_intensity;
    std140_bool skylight_valid;
};

struct material_data
{
    std140_vec4 base_color;
    std140_vec3 emissive_color;
    std140_float metallic;
    std140_float roughness;
    std140_bool base_color_texture;
    std140_bool roughness_metallic_texture;
    std140_bool occlusion_texture;
    std140_bool packed_occlusion;
    std140_bool normal_texture;
    std140_bool emissive_color_texture;
    std140_float emissive_intensity;
    std140_int32 alpha_mode;
    std140_float alpha_cutoff;
};

struct model_data
{
    std140_mat4 model_matrix;
    std140_mat3 normal_matrix;
    std140_bool has_normals;
    std140_bool has_tangents;
};

struct renderer_data
{
    std140_bool shadow_step_enabled;
    std140_bool debug_view_enabled;
    std140_bool position_debug_view;
    std140_bool normal_debug_view;
    std140_bool depth_debug_view;
    std140_bool base_color_debug_view;
    std140_bool reflection_color_debug_view;
    std140_bool emission_debug_view;
    std140_bool occlusion_debug_view;
    std140_bool roughness_debug_view;
    std140_bool metallic_debug_view;
    std140_bool show_cascades;
};

struct shadow_data
{
    std140_array<std140_mat4, 4> shadow_view_projection_matrices;
    std140_array<std140_float, 4> shadow_split_depth;
    std140_vec4 shadow_far_planes;
    std140_int32 shadow_resolution;
    std140_int32 shadow_cascade_count;
    std140_float shadow_cascade_interpolation_range;
    std140_int32 shadow_sample_count;
    std140_float shadow_slope_bias;
    std140_float shadow_normal_bias;
    std140_int32 shadow_filter_mode;
    std140_float shadow_width;
    std140_float shadow_light_size;
    std140_int32 shadow_cascade;
};

struct luminance_data
{
    std140_array<std140_uint32, 256> histogram;
    std140_vec4 params;
    std140_float luminance;
};

} // namespace mango

#endif // MANGO_SHADER_INTEROP_HPP
