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

//! \cond NO_COND
using sl_uint32 = uint32;
using sl_int32  = int32;
using sl_float  = float;
using sl_double = double;
using sl_vec2   = vec2;
using sl_vec3   = vec3;
using sl_vec4   = vec4;
using sl_ivec2  = ivec2;
using sl_ivec3  = ivec3;
using sl_ivec4  = ivec4;
using sl_uvec2  = uvec2;
using sl_uvec3  = uvec3;
using sl_uvec4  = uvec4;
using sl_dvec2  = dvec2;
using sl_dvec3  = dvec3;
using sl_dvec4  = dvec4;
using sl_mat2   = mat2;
using sl_mat4   = mat4;
using sl_dmat2  = dmat2;
using sl_dmat4  = dmat4;

struct sl_bool
{
    sl_bool(const bool& b)
    {
        pad = 0;
        v   = b;
    }
    sl_bool()
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

using sl_bvec2 = Eigen::Vector2<sl_bool>;
using sl_bvec3 = Eigen::Vector3<sl_bool>;
using sl_bvec4 = Eigen::Vector4<sl_bool>;

template <typename T>
struct sl_matrix3
{
    sl_matrix3(const Eigen::Matrix3<T>& mat)
        : m(Eigen::Matrix<T, 4, 3>::Zero())
    {
        m.template block<3, 3>(0, 0) = mat;
    }
    sl_matrix3()
        : m(Eigen::Matrix<T, 4, 3>::Zero())
    {
    }
    void operator=(const Eigen::Matrix3<T>& o)
    {
        m.template block<3, 3>(0, 0) = o;
    }
    operator Eigen::Matrix3<T>&()
    {
        return m.template block<3, 3>(0, 0);
    }

  private:
    Eigen::Matrix<T, 4, 3> m;
};

using sl_mat3  = sl_matrix3<float>;
using sl_dmat3 = sl_matrix3<double>;
template <ptr_size N>
struct sl_uint32_array_std140
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

    sl_uint32_array_std140() = default;
    ~sl_uint32_array_std140() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_uint32& v)
        {
            value = v;
        };
        sl_uint32 value;
        sl_uint32 pad0 = 0;
        sl_uint32 pad1 = 0;
        sl_uint32 pad2 = 0;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_int32_array_std140
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

    sl_int32_array_std140() = default;
    ~sl_int32_array_std140() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_int32& v)
        {
            value = v;
        };
        sl_int32 value;
        sl_uint32 pad0 = 0;
        sl_uint32 pad1 = 0;
        sl_uint32 pad2 = 0;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_float_array_std140
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

    sl_float_array_std140() = default;
    ~sl_float_array_std140() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_float& v)
        {
            value = v;
        };
        sl_float value;
        sl_uint32 pad0 = 0;
        sl_uint32 pad1 = 0;
        sl_uint32 pad2 = 0;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_double_array_std140
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

    sl_double_array_std140() = default;
    ~sl_double_array_std140() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_double& v)
        {
            value = v;
        };
        sl_double value;
        sl_uint32 pad0 = 0;
        sl_uint32 pad1 = 0;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_bool_array_std140
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

    sl_bool_array_std140() = default;
    ~sl_bool_array_std140() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_bool& v)
        {
            value = v;
        };
        sl_bool value;
        sl_uint32 pad0 = 0;
        sl_uint32 pad1 = 0;
        sl_uint32 pad2 = 0;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_vec2_array_std140
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

    sl_vec2_array_std140() = default;
    ~sl_vec2_array_std140() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_vec2& v)
        {
            value = v;
        };
        sl_vec2 value;
        sl_uint32 pad0 = 0;
        sl_uint32 pad1 = 0;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_vec3_array_std140
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

    sl_vec3_array_std140() = default;
    ~sl_vec3_array_std140() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_vec3& v)
        {
            value = v;
        };
        sl_vec3 value;
        sl_uint32 pad0 = 0;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_vec4_array_std140
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

    sl_vec4_array_std140() = default;
    ~sl_vec4_array_std140() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_vec4& v)
        {
            value = v;
        };
        sl_vec4 value;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_ivec2_array_std140
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

    sl_ivec2_array_std140() = default;
    ~sl_ivec2_array_std140() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_ivec2& v)
        {
            value = v;
        };
        sl_ivec2 value;
        sl_uint32 pad0 = 0;
        sl_uint32 pad1 = 0;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_ivec3_array_std140
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

    sl_ivec3_array_std140() = default;
    ~sl_ivec3_array_std140() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_ivec3& v)
        {
            value = v;
        };
        sl_ivec3 value;
        sl_uint32 pad0 = 0;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_ivec4_array_std140
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

    sl_ivec4_array_std140() = default;
    ~sl_ivec4_array_std140() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_ivec4& v)
        {
            value = v;
        };
        sl_ivec4 value;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_uvec2_array_std140
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

    sl_uvec2_array_std140() = default;
    ~sl_uvec2_array_std140() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_uvec2& v)
        {
            value = v;
        };
        sl_uvec2 value;
        sl_uint32 pad0 = 0;
        sl_uint32 pad1 = 0;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_uvec3_array_std140
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

    sl_uvec3_array_std140() = default;
    ~sl_uvec3_array_std140() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_uvec3& v)
        {
            value = v;
        };
        sl_uvec3 value;
        sl_uint32 pad0 = 0;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_uvec4_array_std140
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

    sl_uvec4_array_std140() = default;
    ~sl_uvec4_array_std140() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_uvec4& v)
        {
            value = v;
        };
        sl_uvec4 value;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_dvec2_array_std140
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

    sl_dvec2_array_std140() = default;
    ~sl_dvec2_array_std140() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_dvec2& v)
        {
            value = v;
        };
        sl_dvec2 value;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_dvec3_array_std140
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

    sl_dvec3_array_std140() = default;
    ~sl_dvec3_array_std140() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_dvec3& v)
        {
            value = v;
        };
        sl_dvec3 value;
        sl_uint32 pad0 = 0;
        sl_uint32 pad1 = 0;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_dvec4_array_std140
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

    sl_dvec4_array_std140() = default;
    ~sl_dvec4_array_std140() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_dvec4& v)
        {
            value = v;
        };
        sl_dvec4 value;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_bvec2_array_std140
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

    sl_bvec2_array_std140() = default;
    ~sl_bvec2_array_std140() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_bvec2& v)
        {
            value = v;
        };
        sl_bvec2 value;
        sl_uint32 pad0 = 0;
        sl_uint32 pad1 = 0;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_bvec3_array_std140
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

    sl_bvec3_array_std140() = default;
    ~sl_bvec3_array_std140() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_bvec3& v)
        {
            value = v;
        };
        sl_bvec3 value;
        sl_uint32 pad0 = 0;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_bvec4_array_std140
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

    sl_bvec4_array_std140() = default;
    ~sl_bvec4_array_std140() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_bvec4& v)
        {
            value = v;
        };
        sl_bvec4 value;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_mat2_array_std140
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

    sl_mat2_array_std140() = default;
    ~sl_mat2_array_std140() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_mat2& v)
        {
            value = v;
        };
        sl_mat2 value;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_mat3_array_std140
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

    sl_mat3_array_std140() = default;
    ~sl_mat3_array_std140() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_mat3& v)
        {
            value = v;
        };
        sl_mat3 value;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_mat4_array_std140
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

    sl_mat4_array_std140() = default;
    ~sl_mat4_array_std140() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_mat4& v)
        {
            value = v;
        };
        sl_mat4 value;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_dmat2_array_std140
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

    sl_dmat2_array_std140() = default;
    ~sl_dmat2_array_std140() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_dmat2& v)
        {
            value = v;
        };
        sl_dmat2 value;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_dmat3_array_std140
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

    sl_dmat3_array_std140() = default;
    ~sl_dmat3_array_std140() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_dmat3& v)
        {
            value = v;
        };
        sl_dmat3 value;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_dmat4_array_std140
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

    sl_dmat4_array_std140() = default;
    ~sl_dmat4_array_std140() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_dmat4& v)
        {
            value = v;
        };
        sl_dmat4 value;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_uint32_array_std430
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

    sl_uint32_array_std430() = default;
    ~sl_uint32_array_std430() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_uint32& v)
        {
            value = v;
        };
        sl_uint32 value;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_int32_array_std430
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

    sl_int32_array_std430() = default;
    ~sl_int32_array_std430() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_int32& v)
        {
            value = v;
        };
        sl_int32 value;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_float_array_std430
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

    sl_float_array_std430() = default;
    ~sl_float_array_std430() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_float& v)
        {
            value = v;
        };
        sl_float value;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_double_array_std430
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

    sl_double_array_std430() = default;
    ~sl_double_array_std430() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_double& v)
        {
            value = v;
        };
        sl_double value;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_bool_array_std430
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

    sl_bool_array_std430() = default;
    ~sl_bool_array_std430() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_bool& v)
        {
            value = v;
        };
        sl_bool value;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_vec2_array_std430
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

    sl_vec2_array_std430() = default;
    ~sl_vec2_array_std430() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_vec2& v)
        {
            value = v;
        };
        sl_vec2 value;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_vec3_array_std430
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

    sl_vec3_array_std430() = default;
    ~sl_vec3_array_std430() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_vec3& v)
        {
            value = v;
        };
        sl_vec3 value;
        sl_uint32 pad0 = 0;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_vec4_array_std430
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

    sl_vec4_array_std430() = default;
    ~sl_vec4_array_std430() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_vec4& v)
        {
            value = v;
        };
        sl_vec4 value;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_ivec2_array_std430
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

    sl_ivec2_array_std430() = default;
    ~sl_ivec2_array_std430() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_ivec2& v)
        {
            value = v;
        };
        sl_ivec2 value;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_ivec3_array_std430
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

    sl_ivec3_array_std430() = default;
    ~sl_ivec3_array_std430() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_ivec3& v)
        {
            value = v;
        };
        sl_ivec3 value;
        sl_uint32 pad0 = 0;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_ivec4_array_std430
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

    sl_ivec4_array_std430() = default;
    ~sl_ivec4_array_std430() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_ivec4& v)
        {
            value = v;
        };
        sl_ivec4 value;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_uvec2_array_std430
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

    sl_uvec2_array_std430() = default;
    ~sl_uvec2_array_std430() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_uvec2& v)
        {
            value = v;
        };
        sl_uvec2 value;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_uvec3_array_std430
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

    sl_uvec3_array_std430() = default;
    ~sl_uvec3_array_std430() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_uvec3& v)
        {
            value = v;
        };
        sl_uvec3 value;
        sl_uint32 pad0 = 0;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_uvec4_array_std430
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

    sl_uvec4_array_std430() = default;
    ~sl_uvec4_array_std430() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_uvec4& v)
        {
            value = v;
        };
        sl_uvec4 value;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_dvec2_array_std430
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

    sl_dvec2_array_std430() = default;
    ~sl_dvec2_array_std430() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_dvec2& v)
        {
            value = v;
        };
        sl_dvec2 value;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_dvec3_array_std430
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

    sl_dvec3_array_std430() = default;
    ~sl_dvec3_array_std430() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_dvec3& v)
        {
            value = v;
        };
        sl_dvec3 value;
        sl_uint32 pad0 = 0;
        sl_uint32 pad1 = 0;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_dvec4_array_std430
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

    sl_dvec4_array_std430() = default;
    ~sl_dvec4_array_std430() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_dvec4& v)
        {
            value = v;
        };
        sl_dvec4 value;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_bvec2_array_std430
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

    sl_bvec2_array_std430() = default;
    ~sl_bvec2_array_std430() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_bvec2& v)
        {
            value = v;
        };
        sl_bvec2 value;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_bvec3_array_std430
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

    sl_bvec3_array_std430() = default;
    ~sl_bvec3_array_std430() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_bvec3& v)
        {
            value = v;
        };
        sl_bvec3 value;
        sl_uint32 pad0 = 0;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_bvec4_array_std430
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

    sl_bvec4_array_std430() = default;
    ~sl_bvec4_array_std430() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_bvec4& v)
        {
            value = v;
        };
        sl_bvec4 value;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_mat2_array_std430
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

    sl_mat2_array_std430() = default;
    ~sl_mat2_array_std430() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_mat2& v)
        {
            value = v;
        };
        sl_mat2 value;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_mat3_array_std430
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

    sl_mat3_array_std430() = default;
    ~sl_mat3_array_std430() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_mat3& v)
        {
            value = v;
        };
        sl_mat3 value;
        sl_uint32 pad0 = 0;
        sl_uint32 pad1 = 0;
        sl_uint32 pad2 = 0;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_mat4_array_std430
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

    sl_mat4_array_std430() = default;
    ~sl_mat4_array_std430() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_mat4& v)
        {
            value = v;
        };
        sl_mat4 value;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_dmat2_array_std430
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

    sl_dmat2_array_std430() = default;
    ~sl_dmat2_array_std430() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_dmat2& v)
        {
            value = v;
        };
        sl_dmat2 value;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_dmat3_array_std430
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

    sl_dmat3_array_std430() = default;
    ~sl_dmat3_array_std430() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_dmat3& v)
        {
            value = v;
        };
        sl_dmat3 value;
        sl_uint32 pad0 = 0;
        sl_uint32 pad1 = 0;
        sl_uint32 pad2 = 0;
        sl_uint32 pad3 = 0;
        sl_uint32 pad4 = 0;
        sl_uint32 pad5 = 0;
    };

    internal_element data[N];
};

template <ptr_size N>
struct sl_dmat4_array_std430
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

    sl_dmat4_array_std430() = default;
    ~sl_dmat4_array_std430() = default;

private:
    struct internal_element
    {
        internal_element() = default;
        internal_element(const sl_dmat4& v)
        {
            value = v;
        };
        sl_dmat4 value;
    };

    internal_element data[N];
};


struct camera_data
{
    sl_mat4 view_matrix;
    sl_mat4 projection_matrix;
    sl_mat4 inverse_view_matrix;
    sl_mat4 inverse_projection_matrix;
    sl_mat4 view_projection_matrix;
    sl_mat4 inverse_view_projection_matrix;
    sl_vec3 camera_position;
    sl_float camera_near;
    sl_float camera_far;
    sl_float camera_exposure;
};

struct light_data
{
    sl_vec3 directional_light_direction;
    sl_uint32 pad0;
    sl_vec3 directional_light_color;
    sl_float directional_light_intensity;
    sl_bool directional_light_cast_shadows;
    sl_bool directional_light_valid;
    sl_float skylight_intensity;
    sl_bool skylight_valid;
};

struct material_data
{
    sl_vec4 base_color;
    sl_vec3 emissive_color;
    sl_float metallic;
    sl_float roughness;
    sl_bool base_color_texture;
    sl_bool roughness_metallic_texture;
    sl_bool occlusion_texture;
    sl_bool packed_occlusion;
    sl_bool normal_texture;
    sl_bool emissive_color_texture;
    sl_float emissive_intensity;
    sl_int32 alpha_mode;
    sl_float alpha_cutoff;
};

struct model_data
{
    sl_mat4 model_matrix;
    sl_mat3 normal_matrix;
    sl_bool has_normals;
    sl_bool has_tangents;
};

struct renderer_data
{
    sl_bool shadow_pass_enabled;
    sl_bool debug_view_enabled;
    sl_bool position_debug_view;
    sl_bool normal_debug_view;
    sl_bool depth_debug_view;
    sl_bool base_color_debug_view;
    sl_bool reflection_color_debug_view;
    sl_bool emission_debug_view;
    sl_bool occlusion_debug_view;
    sl_bool roughness_debug_view;
    sl_bool metallic_debug_view;
    sl_bool show_cascades;
};

struct shadow_data
{
    sl_mat4_array_std140<4> shadow_view_projection_matrices;
    sl_float_array_std140<4> shadow_split_depth;
    sl_vec4 shadow_far_planes;
    sl_int32 shadow_resolution;
    sl_int32 shadow_cascade_count;
    sl_float shadow_cascade_interpolation_range;
    sl_int32 shadow_sample_count;
    sl_float shadow_slope_bias;
    sl_float shadow_normal_bias;
    sl_int32 shadow_filter_mode;
    sl_float shadow_width;
    sl_float shadow_light_size;
    sl_int32 shadow_cascade;
};

struct ibl_generation_data
{
    sl_vec2 out_size;
    sl_vec2 data;
};

struct cubemap_data
{
    sl_mat4 model_matrix;
    sl_float render_level;
};

struct fxaa_data
{
    sl_vec2 inverse_screen_size;
    sl_float subpixel_filter;
};

struct luminance_data
{
    sl_uint32_array_std430<256> histogram;
    sl_vec4 params;
    sl_float luminance;
};

struct hi_z_data
{
    sl_vec4 params;
    sl_int32 pass;
};

struct gtao_data
{
    sl_float ao_radius;
    sl_float thin_occluder_compensation;
    sl_int32 slices;
    sl_int32 direction_samples;
    sl_int32 depth_mip_count;
    sl_bool multi_bounce;
    sl_float power;
};

struct bloom_data
{
    sl_int32 filter_radius;
    sl_float power;
    sl_int32 current_mip;
};
//! \encond

} // namespace mango

#endif // MANGO_SHADER_INTEROP_HPP
