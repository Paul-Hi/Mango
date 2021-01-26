//! \file      graphics_common.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_GRAPHICS_COMMON_HPP
#define MANGO_GRAPHICS_COMMON_HPP

#include <glad/glad.h>
#include <mango/types.hpp>

namespace mango
{
//! \cond NO_COND
#define MANGO_TEMPLATE_GRAPHICS_OBJECT(name) \
    template <typename K>                    \
    class name;                              \
    template <typename K>                    \
    using name##_ptr = shared_ptr<name<K>>;
#define MANGO_GRAPHICS_OBJECT(name) \
    class name;                     \
    using name##_ptr = shared_ptr<name>;
#define MANGO_GRAPHICS_OBJECT_IMPL(name) class name_impl;
    MANGO_TEMPLATE_GRAPHICS_OBJECT(command_buffer)
    MANGO_GRAPHICS_OBJECT(buffer)
    MANGO_GRAPHICS_OBJECT_IMPL(buffer)
    MANGO_GRAPHICS_OBJECT(texture)
    MANGO_GRAPHICS_OBJECT_IMPL(texture)
    MANGO_GRAPHICS_OBJECT(shader)
    MANGO_GRAPHICS_OBJECT_IMPL(shader)
    MANGO_GRAPHICS_OBJECT(shader_program)
    MANGO_GRAPHICS_OBJECT_IMPL(shader_program)
    MANGO_GRAPHICS_OBJECT(vertex_array)
    MANGO_GRAPHICS_OBJECT_IMPL(vertex_array)
    MANGO_GRAPHICS_OBJECT(framebuffer)
    MANGO_GRAPHICS_OBJECT_IMPL(framebuffer)
    MANGO_GRAPHICS_OBJECT(gpu_buffer)

#undef MANGO_GRAPHICS_OBJECT
#undef MANGO_GRAPHICS_OBJECT_IMPL
    //! \endcond

    //! \brief The alpha mode of a material.
    enum class alpha_mode : uint8
    {
        mode_opaque,
        mode_mask,
        mode_blend,
        mode_dither
    };

    //! \brief Structure to store material properties, textures etc.
    struct material
    {
        color_rgba base_color;      //!< The basic color of the material.
        color_rgb emissive_color;   //!< The emissive color of the material.
        normalized_float metallic;  //!< The metallic value of the material. Between 0 and 1.
        normalized_float roughness; //!< The roughness value of the material. Between 0 and 1.
        bool packed_occlusion;      //!< Specifies if the occlusion value is packed in the roughness_metallic_texture.

        bool use_base_color_texture;         //!< Specifies, if the the base color texture is enabled.
        bool use_roughness_metallic_texture; //!< Specifies, if the component texture is enabled for the metallic value and the roughness value.
        bool use_occlusion_texture;          //!< Specifies, if the component texture is enabled for the occlusion value.
        bool use_packed_occlusion;           //!< Specifies if the packed occlusion value is enabled.
        bool use_normal_texture;             //!< Specifies, if the normal texture is enabled.
        bool use_emissive_color_texture;     //!< Specifies, if the the emissive color texture is enabled.

        texture_ptr base_color_texture;         //!< The component texture for the basic color value.
        texture_ptr roughness_metallic_texture; //!< The component texture for the metallic value and the roughness value and eventually occlusion.
        texture_ptr occlusion_texture;          //!< The component texture for the occlusion value.
        texture_ptr normal_texture;             //!< The texture for normals.
        texture_ptr emissive_color_texture;     //!< The texture for the emissive color value.

        bool double_sided;             //!< Specifies if the material is double sided.
        alpha_mode alpha_rendering;    //!< Specifies the materials alpha mode.
        normalized_float alpha_cutoff; //!< Specifies a cutoff value if alpha_rendering is MASK.
    };

    //! \cond NO_COND
    using material_ptr = shared_ptr<material>;
    //! \endcond

    // TODO Paul: We should query these from OpenGL!
    //! \brief Constant for maximum number of bound vertex buffers.
    const uint32 max_vertex_buffers = 128; // This is estimated.
    //! \brief Constant for maximum number of bound uniforms.
    const uint32 max_uniforms = 16; // This should be minimum possible.
    //! \brief Constant for maximum number of bound textures.
    const uint32 max_textures = 16; // This should be minimum possible.

    //! \brief Type alias for GLboolean.
    using g_bool = GLboolean;
    //! \brief Type alias for GLbyte.
    using g_byte = GLbyte;
    //! \brief Type alias for GLubyte.
    using g_ubyte = GLubyte;
    //! \brief Type alias for GLshort.
    using g_short = GLshort;
    //! \brief Type alias for GLushort.
    using g_ushort = GLushort;
    //! \brief Type alias for GLint.
    using g_int = GLint;
    //! \brief Type alias for GLuint.
    using g_uint = GLuint;
    //! \brief Type alias for GLfixed.
    using g_fixed = GLfixed;
    //! \brief Type alias for GLint64.
    using g_int64 = GLint64;
    //! \brief Type alias for GLuint64.
    using g_uint64 = GLuint64;
    //! \brief Type alias for GLsizei.
    using g_sizei = GLsizei;
    //! \brief Type alias for GLenum.
    using g_enum = GLenum;
    //! \brief Type alias for GLintptr.
    using g_intptr = GLintptr;
    //! \brief Type alias for GLsizeiptr.
    using g_sizeiptr = GLsizeiptr;
    //! \brief Type alias for GLsync.
    using g_sync = GLsync;
    //! \brief Type alias for GLbitfield.
    using g_bitfield = GLbitfield;
    //! \brief Type alias for GLhalf.
    using g_half = GLhalf;
    //! \brief Type alias for GLfloat.
    using g_float = GLfloat;
    //! \brief Type alias for GLclampf.
    using g_clampf = GLclampf;
    //! \brief Type alias for GLdouble.
    using g_double = GLdouble;
    //! \brief Type alias for GLclampd.
    using g_clampd = GLclampd;
    //! \brief Type alias for GLchar.
    using g_char = GLchar;
    //! \brief Type alias for GLsync.
    using g_sync = GLsync;

    //! \brief A boolean in the glsl std140 layout.
    struct std140_bool
    {
        //! \cond NO_COND
        std140_bool(const bool& b)
        {
            pad = 0;
            v   = b ? 1 : 0;
        }
        std140_bool()
            : pad(0)
        {
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
            int32 pad;
        };
        //! \endcond
    };

    //! \brief An integer in the glsl std140 layout.
    struct std140_int
    {
        //! \cond NO_COND
        std140_int(const int& i)
        {
            v = i;
        }
        std140_int()
            : v(0)
        {
        }
        operator int32&()
        {
            return v;
        }
        void operator=(const int& o)
        {
            v = o;
        }

      private:
        int32 v;
        //! \endcond
    };

    //! \brief A float in the glsl std140 layout.
    struct std140_float
    {
        //! \cond NO_COND
        std140_float(const float& f)
        {
            v = f;
        }
        std140_float()
            : v(0)
        {
        }
        operator float&()
        {
            return v;
        }
        void operator=(const float& o)
        {
            v = o;
        }

      private:
        float v;
        //! \endcond
    };

    //! \brief A float in the glsl std140 layout for arrays.
    struct std140_float_array
    {
        //! \cond NO_COND
        std140_float_array(const float& f)
        {
            v = f;
        }
        std140_float_array()
            : v(0)
        {
        }
        operator float&()
        {
            return v;
        }
        void operator=(const float& o)
        {
            v = o;
        }

      private:
        float v;
        float p0 = 0.0f;
        float p1 = 0.0f;
        float p2 = 0.0f;
        //! \endcond
    };

    //! \brief A vec2 in the glsl std140 layout.
    struct std140_vec2
    {
        //! \cond NO_COND
        std140_vec2(const glm::vec2& vec)
            : v(vec)
        {
        }
        std140_vec2()
            : v(glm::vec2(0.0f))
        {
        }
        operator glm::vec2&()
        {
            return v;
        }
        void operator=(const glm::vec2& o)
        {
            v = o;
        }
        float& operator[](int i)
        {
            return v[i];
        }

      private:
        glm::vec2 v;
        //! \endcond
    };

    //! \brief A vec3 in the glsl std140 layout.
    struct std140_vec3
    {
        //! \cond NO_COND
        std140_vec3(const glm::vec3& vec)
            : v(vec)
        {
        }
        std140_vec3()
            : v(glm::vec3(0.0f))
        {
        }
        operator glm::vec3&()
        {
            return v;
        }
        void operator=(const glm::vec3& o)
        {
            v = o;
        }
        float& operator[](int i)
        {
            return v[i];
        }

      private:
        glm::vec3 v;
        float pad = 0.0f;
        //! \endcond
    };

    //! \brief A vec4 in the glsl std140 layout.
    struct std140_vec4
    {
        //! \cond NO_COND
        std140_vec4(const glm::vec4& vec)
            : v(vec)
        {
        }
        std140_vec4()
            : v(glm::vec4(0.0f))
        {
        }
        operator glm::vec4&()
        {
            return v;
        }
        void operator=(const glm::vec4& o)
        {
            v = o;
        }
        float& operator[](int i)
        {
            return v[i];
        }

      private:
        glm::vec4 v;
        //! \endcond
    };

    //! \brief A mat3 in the glsl std140 layout.
    struct std140_mat3
    {
        //! \cond NO_COND
        std140_mat3(const glm::mat3& mat)
            : r0(mat[0])
            , r1(mat[1])
            , r2(mat[2])
        {
        }
        std140_mat3()
            : r0()
            , r1()
            , r2()
        {
        }
        void operator=(const glm::mat3& o)
        {
            r0 = o[0];
            r1 = o[1];
            r2 = o[2];
        }
        glm::vec3& operator[](int i)
        {
            switch (i)
            {
            case 0:
                return r0;
            case 1:
                return r1;
            case 2:
                return r2;
            default:
                MANGO_ASSERT(false, "3D Vector has only 3 components!"); // TODO Paul: Ouch!
            }
        }

      private:
        std140_vec3 r0;
        std140_vec3 r1;
        std140_vec3 r2;
        //! \endcond
    };

    //! \brief A mat4 in the glsl std140 layout.
    struct std140_mat4
    {
        //! \cond NO_COND
        std140_mat4(const glm::mat4& mat)
            : r0(mat[0])
            , r1(mat[1])
            , r2(mat[2])
            , r3(mat[3])
        {
        }
        std140_mat4()
            : r0()
            , r1()
            , r2()
            , r3()
        {
        }
        void operator=(const glm::mat4& o)
        {
            r0 = o[0];
            r1 = o[1];
            r2 = o[2];
            r3 = o[3];
        }
        glm::vec4& operator[](int i)
        {
            switch (i)
            {
            case 0:
                return r0;
            case 1:
                return r1;
            case 2:
                return r2;
            case 3:
                return r3;
            default:
                MANGO_ASSERT(false, "3D Vector has only 3 components!"); // TODO Paul: Ouch!
            }
        }

      private:
        std140_vec4 r0;
        std140_vec4 r1;
        std140_vec4 r2;
        std140_vec4 r3;
        //! \endcond
    };

    //! \brief Calculates the number of mipmap images for a given images size.
    //! \param[in] width The width of the image. Has to be a positive value.
    //! \param[in] height The height of the image. Has to be a positive value.
    //! \return The number of mipmaps that need to be gennerated.
    inline int32 calculate_mip_count(int32 width, int32 height)
    {
        int32 levels = 1;
        while ((width | height) >> levels)
        {
            ++levels;
        }
        return levels;
    }

    //! \brief All kinds of format values.
    //! \details The values are the same as in OpenGl, but sometimes the usage is extended.
    enum class format : uint32 // OpenGL values
    {
        invalid = 0x0,
        // vertex attribute formats and buffer format types
        t_byte                        = 0x1400,
        t_unsigned_byte               = 0x1401,
        t_short                       = 0x1402,
        t_unsigned_short              = 0x1403,
        t_half_float                  = 0x140b,
        t_double                      = 0x140a,
        t_fixed                       = 0x140c,
        t_float                       = 0x1406,
        t_float_vec2                  = 0x8b50,
        t_float_vec3                  = 0x8b51,
        t_float_vec4                  = 0x8b52,
        t_int                         = 0x1404,
        t_int_vec2                    = 0x8b53,
        t_int_vec3                    = 0x8b54,
        t_int_vec4                    = 0x8b55,
        t_unsigned_int                = 0x1405,
        t_unsigned_int_vec2           = 0x8dc6,
        t_unsigned_int_vec3           = 0x8dc7,
        t_unsigned_int_vec4           = 0x8dc8,
        t_unsigned_byte_3_3_2         = 0x8032,
        t_unsigned_byte_2_3_3_rev     = 0x8362,
        t_unsigned_short_5_6_5        = 0x8363,
        t_unsigned_short_5_6_5_rev    = 0x8364,
        t_unsigned_short_4_4_4_4      = 0x8033,
        t_unsigned_short_4_4_4_4_rev  = 0x8365,
        t_unsigned_short_5_5_5_1      = 0x8034,
        t_unsigned_short_1_5_5_5_rev  = 0x8366,
        t_unsigned_int_8_8_8_8        = 0x8035,
        t_unsigned_int_8_8_8_8_rev    = 0x8367,
        t_unsigned_int_10_10_10_2     = 0x8036,
        t_unsigned_int_2_10_10_10_rev = 0x8368,
        t_int_2_10_10_10_rev          = 0x8d9f,
        // internal_formats
        r8                 = 0x8229,
        r16                = 0x822a,
        r16f               = 0x822d,
        r32f               = 0x822e,
        r8i                = 0x8231,
        r16i               = 0x8233,
        r32i               = 0x8235,
        r8ui               = 0x8232,
        r16ui              = 0x8234,
        r32ui              = 0x8236,
        rg8                = 0x822b,
        rg16               = 0x822c,
        rg16f              = 0x822f,
        rg32f              = 0x8230,
        rg8i               = 0x8237,
        rg16i              = 0x8239,
        rg32i              = 0x823b,
        rg8ui              = 0x8238,
        rg16ui             = 0x823a,
        rg32ui             = 0x823c,
        rgb4               = 0x804f,
        rgb5               = 0x8050,
        rgb8               = 0x8051,
        rgb10              = 0x8052,
        rgb12              = 0x8053,
        rgb16              = 0x8054,
        srgb8              = 0x8c41,
        srgb8_alpha8       = 0x8c43,
        rgb8ui             = 0x8d7d,
        rgb8i              = 0x8d8f,
        rgb16f             = 0x881b,
        rgb16ui            = 0x8d77,
        rgb16i             = 0x8d89,
        rgb32f             = 0x8815,
        rgb32i             = 0x8d83,
        rgb32ui            = 0x8d71,
        rgba2              = 0x8055,
        rgba4              = 0x8056,
        rgb5_a1            = 0x8057,
        rgba8              = 0x8058,
        rgb10_a2           = 0x8059,
        rgba12             = 0x805a,
        rgba16             = 0x805b,
        rgba16f            = 0x881a,
        rgba32f            = 0x8814,
        rgba8i             = 0x8d8e,
        rgba16i            = 0x8d88,
        rgba32i            = 0x8d82,
        rgba8ui            = 0x8d7c,
        rgba16ui           = 0x8d76,
        rgba32ui           = 0x8d70,
        depth_component32f = 0x8cac,
        depth_component16  = 0x81a5,
        depth_component24  = 0x81a6,
        depth_component32  = 0x81a7,
        // pixel formats
        depth_component = 0x1902,
        stencil_index   = 0x1901,
        depth_stencil   = 0x84f9,
        red             = 0x1903,
        green           = 0x1904,
        blue            = 0x1905,
        rg              = 0x8227,
        rgb             = 0x1907,
        bgr             = 0x80e0,
        rgba            = 0x1908,
        bgra            = 0x80e1,
        red_integer     = 0x8d94,
        green_integer   = 0x8d95,
        blue_integer    = 0x8d96,
        rg_integer      = 0x8228,
        rgb_integer     = 0x8d98,
        bgr_integer     = 0x8d9a,
        rgba_integer    = 0x8d99,
        bgra_integer    = 0x8d9b,
    };
    MANGO_ENABLE_BITMASK_OPERATIONS(format)

    //! \brief Retrieves the gl format, number of components and normalized status for a specific format type.
    //! \param[in] f The format to get the data for.
    //! \param[out] number_of_components The number of components will be stored in here.
    //! \param[out] normalized It will be stored in here, if the value is normalized or not.
    //! \return The gl format.
    inline g_enum get_gl_vertex_attribute_data(const format& f, g_int& number_of_components, g_bool& normalized)
    {
        switch (f)
        {
        case format::r8:
            number_of_components = 1;
            normalized           = true;
            return GL_UNSIGNED_BYTE;
        case format::r16:
            number_of_components = 1;
            normalized           = true;
            return GL_UNSIGNED_SHORT;
        case format::r16f:
            number_of_components = 1;
            normalized           = false;
            return GL_HALF_FLOAT;
        case format::r32f:
            number_of_components = 1;
            normalized           = false;
            return GL_FLOAT;
        case format::r8i:
            number_of_components = 1;
            normalized           = true;
            return GL_BYTE;
        case format::r16i:
            number_of_components = 1;
            normalized           = true;
            return GL_SHORT;
        case format::r32i:
            number_of_components = 1;
            normalized           = true;
            return GL_INT;
        case format::r8ui:
            number_of_components = 1;
            normalized           = true;
            return GL_UNSIGNED_BYTE;
        case format::r16ui:
            number_of_components = 1;
            normalized           = true;
            return GL_UNSIGNED_SHORT;
        case format::r32ui:
            number_of_components = 1;
            normalized           = true;
            return GL_UNSIGNED_INT;
        case format::rg8:
            number_of_components = 2;
            normalized           = true;
            return GL_UNSIGNED_BYTE;
        case format::rg16:
            number_of_components = 2;
            normalized           = true;
            return GL_UNSIGNED_SHORT;
        case format::rg16f:
            number_of_components = 2;
            normalized           = false;
            return GL_HALF_FLOAT;
        case format::rg32f:
            number_of_components = 2;
            normalized           = false;
            return GL_FLOAT;
        case format::rg8i:
            number_of_components = 2;
            normalized           = true;
            return GL_BYTE;
        case format::rg16i:
            number_of_components = 2;
            normalized           = true;
            return GL_SHORT;
        case format::rg32i:
            number_of_components = 2;
            normalized           = true;
            return GL_INT;
        case format::rg8ui:
            number_of_components = 2;
            normalized           = true;
            return GL_UNSIGNED_BYTE;
        case format::rg16ui:
            number_of_components = 2;
            normalized           = true;
            return GL_UNSIGNED_SHORT;
        case format::rg32ui:
            number_of_components = 2;
            normalized           = true;
            return GL_UNSIGNED_INT;
        case format::rgb8i:
            number_of_components = 3;
            normalized           = true;
            return GL_BYTE;
        case format::rgb8ui:
            number_of_components = 3;
            normalized           = true;
            return GL_UNSIGNED_BYTE;
        case format::rgb16f:
            number_of_components = 3;
            normalized           = false;
            return GL_HALF_FLOAT;
        case format::rgb16i:
            number_of_components = 3;
            normalized           = true;
            return GL_SHORT;
        case format::rgb16ui:
            number_of_components = 3;
            normalized           = true;
            return GL_UNSIGNED_SHORT;
        case format::rgb32f:
            number_of_components = 3;
            normalized           = false;
            return GL_FLOAT;
        case format::rgb32i:
            number_of_components = 3;
            normalized           = true;
            return GL_INT;
        case format::rgb32ui:
            number_of_components = 3;
            normalized           = true;
            return GL_UNSIGNED_INT;
        case format::rgba8:
            number_of_components = 4;
            normalized           = true;
            return GL_UNSIGNED_BYTE;
        case format::rgba16:
            number_of_components = 4;
            normalized           = true;
            return GL_SHORT;
        case format::rgba16f:
            number_of_components = 4;
            normalized           = false;
            return GL_HALF_FLOAT;
        case format::rgba32f:
            number_of_components = 4;
            normalized           = false;
            return GL_FLOAT;
        case format::rgba8i:
            number_of_components = 4;
            normalized           = true;
            return GL_BYTE;
        case format::rgba16i:
            number_of_components = 4;
            normalized           = true;
            return GL_UNSIGNED_BYTE;
        case format::rgba32i:
            number_of_components = 4;
            normalized           = true;
            return GL_UNSIGNED_BYTE;
        case format::rgba8ui:
            number_of_components = 4;
            normalized           = true;
            return GL_UNSIGNED_BYTE;
        case format::rgba16ui:
            number_of_components = 4;
            normalized           = true;
            return GL_UNSIGNED_SHORT;
        case format::rgba32ui:
            number_of_components = 4;
            normalized           = true;
            return GL_UNSIGNED_INT;
        default:
            MANGO_ASSERT(false, "Invalid format! Could also be, that I did not think of adding this here!");
            return GL_NONE;
        }
    }

    //! \brief Creates an attribute type from component type and count.
    //! \param[in] f The format for each component.
    //! \param[in] number_of_components The number of components.
    //! \return The attribute format fitting.
    inline format get_attribute_format(const format& f, g_int number_of_components)
    {
        switch (f)
        {
        case format::t_byte:
            if (number_of_components == 1)
                return format::r8i;
            if (number_of_components == 2)
                return format::rg8i;
            if (number_of_components == 3)
                return format::rgb8i;
            if (number_of_components == 4)
                return format::rgba8i;
            break;
        case format::t_unsigned_byte:
            if (number_of_components == 1)
                return format::r8ui;
            if (number_of_components == 2)
                return format::rg8ui;
            if (number_of_components == 3)
                return format::rgb8ui;
            if (number_of_components == 4)
                return format::rgba8ui;
            break;
        case format::t_short:
            if (number_of_components == 1)
                return format::r16i;
            if (number_of_components == 2)
                return format::rg16i;
            if (number_of_components == 3)
                return format::rgb16i;
            if (number_of_components == 4)
                return format::rgba16i;
            break;
        case format::t_unsigned_short:
            if (number_of_components == 1)
                return format::r16ui;
            if (number_of_components == 2)
                return format::rg16ui;
            if (number_of_components == 3)
                return format::rgb16ui;
            if (number_of_components == 4)
                return format::rgba16ui;
            break;
        case format::t_int:
            if (number_of_components == 1)
                return format::r32i;
            if (number_of_components == 2)
                return format::rg32i;
            if (number_of_components == 3)
                return format::rgb32i;
            if (number_of_components == 4)
                return format::rgba32i;
            break;
        case format::t_unsigned_int:
            if (number_of_components == 1)
                return format::r32ui;
            if (number_of_components == 2)
                return format::rg32ui;
            if (number_of_components == 3)
                return format::rgb32ui;
            if (number_of_components == 4)
                return format::rgba32ui;
            break;
        case format::t_half_float:
            if (number_of_components == 1)
                return format::r16f;
            if (number_of_components == 2)
                return format::rg16f;
            if (number_of_components == 3)
                return format::rgb16f;
            if (number_of_components == 4)
                return format::rgba16f;
            break;
        case format::t_float:
            if (number_of_components == 1)
                return format::r32f;
            if (number_of_components == 2)
                return format::rg32f;
            if (number_of_components == 3)
                return format::rgb32f;
            if (number_of_components == 4)
                return format::rgba32f;
            break;
        default:
            MANGO_ASSERT(false, "Invalid format! Could also be, that I did not think of adding this here!");
            return format::invalid;
        }
        MANGO_ASSERT(false, "Invalid count! Could also be, that I did not think of adding this here!");
        return format::invalid;
    }

    //! \brief Retrieves the number of machine units, or size in bytes for a format.
    //! \param[in] internal_format The format to get the byte size for.
    //! \return The size in bytes.
    inline int64 number_of_basic_machine_units(const format& internal_format)
    {
        switch (internal_format)
        {
        case format::r8:
            return 1 * sizeof(g_ubyte);
        case format::r16:
            return 1 * sizeof(g_ushort);
        case format::r16f:
            return 1 * sizeof(g_half);
        case format::r32f:
            return 1 * sizeof(g_float);
        case format::r8i:
            return 1 * sizeof(g_byte);
        case format::r16i:
            return 1 * sizeof(g_short);
        case format::r32i:
            return 1 * sizeof(g_int);
        case format::r8ui:
            return 1 * sizeof(g_ubyte);
        case format::r16ui:
            return 1 * sizeof(g_ushort);
        case format::r32ui:
            return 1 * sizeof(g_uint);
        case format::rg8:
            return 2 * sizeof(g_ubyte);
        case format::rg16:
            return 2 * sizeof(g_ushort);
        case format::rg16f:
            return 2 * sizeof(g_half);
        case format::rg32f:
            return 2 * sizeof(g_float);
        case format::rg8i:
            return 2 * sizeof(g_byte);
        case format::rg16i:
            return 2 * sizeof(g_ushort);
        case format::rg32i:
            return 2 * sizeof(g_int);
        case format::rg8ui:
            return 2 * sizeof(g_ubyte);
        case format::rg16ui:
            return 2 * sizeof(g_ushort);
        case format::rg32ui:
            return 2 * sizeof(g_uint);
        case format::rgb32f:
            return 3 * sizeof(g_float);
        case format::rgb32i:
            return 3 * sizeof(g_int);
        case format::rgb32ui:
            return 3 * sizeof(g_uint);
        case format::rgba8:
            return 4 * sizeof(g_ubyte);
        case format::rgba16:
            return 4 * sizeof(g_short);
        case format::rgba16f:
            return 4 * sizeof(g_half);
        case format::rgba32f:
            return 4 * sizeof(g_float);
        case format::rgba8i:
            return 4 * sizeof(g_byte);
        case format::rgba16i:
            return 4 * sizeof(g_short);
        case format::rgba32i:
            return 4 * sizeof(g_int);
        case format::rgba8ui:
            return 4 * sizeof(g_ubyte);
        case format::rgba16ui:
            return 4 * sizeof(g_ushort);
        case format::rgba32ui:
            return 4 * sizeof(g_uint);
        default:
            MANGO_ASSERT(false, "Invalid internal format! Could also be, that I did not think of adding this here!");
            return 0;
        }
    }

    //! \brief Returns internal format, format and type for an image depending on a few infos.
    //! \param[in] srgb True if image is in standard color space, else False.
    //! \param[in] components The number of components in the image.
    //! \param[in] bits The number of bits in the image.
    //! \param[out] f The format to choose.
    //! \param[out] internal The internal format to choose.
    //! \param[out] type The type to choose.
    //! \param[in] is_hdr True if image is hdr, else False.
    inline void get_formats_and_types_for_image(bool srgb, int32 components, int32 bits, format& f, format& internal, format& type, bool is_hdr)
    {
        if (is_hdr)
        {
            f        = format::rgb;
            internal = format::rgb32f;
            type     = format::t_float;

            if (components == 4)
            {
                f        = format::rgba;
                internal = format::rgba32f;
            }
            return;
        }

        f        = format::rgba;
        internal = srgb ? format::srgb8_alpha8 : format::rgba8;

        if (components == 1)
        {
            f = format::red;
        }
        else if (components == 2)
        {
            f = format::rg;
        }
        else if (components == 3)
        {
            f        = format::rgb;
            internal = srgb ? format::srgb8 : format::rgb8;
        }

        type = format::t_unsigned_byte;
        if (bits == 16)
        {
            type = format::t_unsigned_short;
        }
        else if (bits == 32)
        {
            type = format::t_unsigned_int;
        }
    }

    //! \brief Compare operation used for depth test and similar things.
    enum class compare_operation : uint8
    {
        never,
        less,
        equal,
        less_equal,
        greater,
        not_equal,
        greater_equal,
        always
    };

    //! \brief Converts a \a compare_operation to an OpenGl enumeration value.
    //! \param[in] op The \a compare_operation to convert.
    //! \return The gl enumeration specifying a \a compare_operation.
    inline g_enum compare_operation_to_gl(const compare_operation& op)
    {
        switch (op)
        {
        case compare_operation::never:
            return GL_NEVER;
        case compare_operation::less:
            return GL_LESS;
        case compare_operation::equal:
            return GL_EQUAL;
        case compare_operation::less_equal:
            return GL_LEQUAL;
        case compare_operation::greater:
            return GL_GREATER;
        case compare_operation::not_equal:
            return GL_NOTEQUAL;
        case compare_operation::greater_equal:
            return GL_GEQUAL;
        case compare_operation::always:
            return GL_ALWAYS;
        default:
            MANGO_ASSERT(false, "Unknown compare operation!");
            return GL_NONE;
        }
    }

    //! \brief Enumeration specifying the face of a polygon. Used for face culling.
    enum class polygon_face : uint8
    {
        face_back           = 1 << 0,
        face_front          = 1 << 1,
        face_front_and_back = face_back | face_front
    };
    MANGO_ENABLE_BITMASK_OPERATIONS(polygon_face)

    //! \brief Converts a \a polygon_face to an OpenGl enumeration value.
    //! \param[face] op The \a polygon_face to convert.
    //! \return The gl enumeration specifying a \a polygon_face.
    inline g_enum polygon_face_to_gl(const polygon_face& face)
    {
        switch (face)
        {
        case polygon_face::face_back:
            return GL_BACK;
        case polygon_face::face_front:
            return GL_FRONT;
        case polygon_face::face_front_and_back:
            return GL_FRONT_AND_BACK;
        default:
            MANGO_ASSERT(false, "Unknown polygon face!");
            return GL_NONE;
        }
    }

    //! \brief Enumeration specifying how a polygon should be drawn. For example used to render wireframes.
    enum class polygon_mode : uint8
    {
        point,
        line,
        fill
    };

    //! \brief Converts a \a polygon_mode to an OpenGl enumeration value.
    //! \param[in] mode The \a polygon_mode to convert.
    //! \return The gl enumeration specifying a \a polygon_mode.
    inline g_enum polygon_mode_to_gl(const polygon_mode& mode)
    {
        switch (mode)
        {
        case polygon_mode::point:
            return GL_POINT;
        case polygon_mode::line:
            return GL_LINE;
        case polygon_mode::fill:
            return GL_FILL;
        default:
            MANGO_ASSERT(false, "Unknown polygon mode!");
            return GL_NONE;
        }
    }

    //! \brief The blend factor used for blending operations.
    enum class blend_factor : uint8
    {
        zero,
        one,
        src_color,
        one_minus_src_color,
        dst_color,
        one_minus_dst_color,
        src_alpha,
        one_minus_src_alpha,
        dst_alpha,
        one_minus_dst_alpha,
        constant_color,
        one_minus_constant_color,
        constant_alpha,
        one_minus_constant_alpha,
        src_alpha_saturate,
        src1_color,
        one_minus_src1_color,
        src1_alpha,
        one_minus_src1_alpha
    };

    //! \brief Converts a \a blend_factor to an OpenGl enumeration value.
    //! \param[in] factor The \a blend_factor to convert.
    //! \return The gl enumeration specifying a \a blend_factor.
    inline g_enum blend_factor_to_gl(const blend_factor& factor)
    {
        switch (factor)
        {
        case blend_factor::zero:
            return GL_ZERO;
        case blend_factor::one:
            return GL_ONE;
        case blend_factor::src_color:
            return GL_SRC_COLOR;
        case blend_factor::one_minus_src_color:
            return GL_ONE_MINUS_SRC_COLOR;
        case blend_factor::dst_color:
            return GL_DST_COLOR;
        case blend_factor::one_minus_dst_color:
            return GL_ONE_MINUS_DST_COLOR;
        case blend_factor::src_alpha:
            return GL_SRC_ALPHA;
        case blend_factor::one_minus_src_alpha:
            return GL_ONE_MINUS_SRC_ALPHA;
        case blend_factor::dst_alpha:
            return GL_DST_ALPHA;
        case blend_factor::one_minus_dst_alpha:
            return GL_ONE_MINUS_DST_ALPHA;
        case blend_factor::constant_color:
            return GL_CONSTANT_COLOR;
        case blend_factor::one_minus_constant_color:
            return GL_ONE_MINUS_CONSTANT_COLOR;
        case blend_factor::constant_alpha:
            return GL_CONSTANT_ALPHA;
        case blend_factor::one_minus_constant_alpha:
            return GL_ONE_MINUS_CONSTANT_ALPHA;
        case blend_factor::src_alpha_saturate:
            return GL_SRC_ALPHA_SATURATE;
        case blend_factor::src1_color:
            return GL_SRC1_COLOR;
        case blend_factor::one_minus_src1_color:
            return GL_ONE_MINUS_SRC1_COLOR;
        case blend_factor::src1_alpha:
            return GL_SRC1_ALPHA;
        case blend_factor::one_minus_src1_alpha:
            return GL_ONE_MINUS_SRC1_ALPHA;
        default:
            MANGO_ASSERT(false, "Unknown blend factor!");
            return GL_NONE;
        }
    }

    //! \brief Mask used to specify buffers that should be cleared.
    enum class clear_buffer_mask : uint8
    {
        color_buffer         = 1 << 0,
        depth_buffer         = 1 << 1,
        stencil_buffer       = 1 << 2,
        depth_stencil_buffer = 1 << 3,

        none                        = 0,
        color_and_depth             = color_buffer | depth_buffer,
        color_and_stencil           = color_buffer | stencil_buffer,
        color_and_depth_and_stencil = color_buffer | depth_buffer | stencil_buffer,
        color_and_depth_stencil     = color_buffer | depth_stencil_buffer
    };
    MANGO_ENABLE_BITMASK_OPERATIONS(clear_buffer_mask)

    //! \brief Mask used to specify attachements that should be cleared.
    enum class attachment_mask : uint8
    {
        draw_buffer0   = 1 << 0,
        draw_buffer1   = 1 << 1,
        draw_buffer2   = 1 << 2,
        draw_buffer3   = 1 << 3,
        draw_buffer4   = 1 << 4,
        draw_buffer5   = 1 << 5,
        depth_buffer   = 1 << 6,
        stencil_buffer = 1 << 7,

        none                         = 0,
        all_draw_buffers             = draw_buffer0 | draw_buffer1 | draw_buffer2 | draw_buffer3 | draw_buffer4 | draw_buffer5,
        all_draw_buffers_and_depth   = all_draw_buffers | depth_buffer,
        all_draw_buffers_and_stencil = all_draw_buffers | stencil_buffer,
        depth_stencil_buffer         = depth_buffer | stencil_buffer,
        all                          = all_draw_buffers | depth_stencil_buffer,
    };
    MANGO_ENABLE_BITMASK_OPERATIONS(attachment_mask)

    //! \brief The targets buffer can be bound to.
    enum class buffer_target : uint8
    {
        none,
        vertex_buffer,
        index_buffer,
        uniform_buffer,
        shader_storage_buffer,
        texture_buffer
    };

    //! \brief Converts a \a buffer_target to an OpenGl enumeration value.
    //! \param[in] target The \a buffer_target to convert.
    //! \return The gl enumeration specifying a \a buffer_target.
    inline g_enum buffer_target_to_gl(const buffer_target& target)
    {
        switch (target)
        {
        case buffer_target::vertex_buffer:
            return GL_ARRAY_BUFFER;
        case buffer_target::index_buffer:
            return GL_ELEMENT_ARRAY_BUFFER;
        case buffer_target::uniform_buffer:
            return GL_UNIFORM_BUFFER;
        case buffer_target::shader_storage_buffer:
            return GL_SHADER_STORAGE_BUFFER;
        case buffer_target::texture_buffer:
            return GL_TEXTURE_BUFFER;
        default:
            MANGO_ASSERT(false, "Unknown buffer target!");
            return GL_NONE;
        }
    }

    //! \brief A set of access bits used for accessing buffers.
    enum class buffer_access : uint8
    {
        none                     = 0,
        dynamic_storage          = 1 << 0,
        mapped_access_read       = 1 << 1, // We want to map in a specific way, so we do not give any other options.
        mapped_access_write      = 1 << 2, // We want to map in a specific way, so we do not give any other options.
        mapped_access_read_write = mapped_access_read | mapped_access_write
    };
    MANGO_ENABLE_BITMASK_OPERATIONS(buffer_access)

    //! \brief A set of access bits used for general access.
    enum class base_access : uint8
    {
        none = 0,
        read_only,
        write_only,
        read_write
    };
    //! \brief Converts a \a base_access to an OpenGl enumeration value.
    //! \param[in] access The \a base_access to convert.
    //! \return The gl enumeration specifying a \a base_access.
    inline g_enum base_access_to_gl(const base_access& access)
    {
        switch (access)
        {
        case base_access::read_only:
            return GL_READ_ONLY;
        case base_access::write_only:
            return GL_WRITE_ONLY;
        case base_access::read_write:
            return GL_READ_WRITE;
        default:
            return GL_NONE;
        }
    }

    //! \brief The type of a \a shader.
    enum class shader_type : uint8
    {
        none = 0,

        vertex_shader,
        tesselation_control_shader,
        tesselation_evaluation_shader,
        geometry_shader,
        fragment_shader,

        compute_shader
    };
    //! \brief Converts a \a shader_type to an OpenGl enumeration value.
    //! \param[in] type The \a shader_type to convert.
    //! \return The gl enumeration specifying a \a shader_type.
    inline g_enum shader_type_to_gl(const shader_type& type)
    {
        switch (type)
        {
        case shader_type::vertex_shader:
            return GL_VERTEX_SHADER;
        case shader_type::tesselation_control_shader:
            return GL_TESS_CONTROL_SHADER;
        case shader_type::tesselation_evaluation_shader:
            return GL_TESS_EVALUATION_SHADER;
        case shader_type::geometry_shader:
            return GL_GEOMETRY_SHADER;
        case shader_type::fragment_shader:
            return GL_FRAGMENT_SHADER;
        case shader_type::compute_shader:
            return GL_COMPUTE_SHADER;
        default:
            MANGO_ASSERT(false, "Unknown shader type!");
            return GL_NONE;
        }
    }

    //! \brief The type of the resource used in a shader program by the gpu.
    //! \details Extend this when needed -> all types are here https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGetActiveUniform.xhtml
    enum class shader_resource_type : uint8
    {
        none,
        fsingle,
        fvec2,
        fvec3,
        fvec4,
        // double,
        // dvec2,
        // dvec3,
        // dvec4,
        isingle,
        ivec2,
        ivec3,
        ivec4,
        // unsigned_int,
        // uvec2,
        // uvec3,
        // uvec4,
        bsingle,
        // bvec2,
        // bvec3,
        // bvec4,
        // mat2,
        mat3,
        mat4,
    };
    //! \brief Converts an OpenGl enumeration value to a \a shader_resource_type.
    //! \param[in] type The g_enum to convert.
    //! \return The \a shader_resource_type.
    inline shader_resource_type shader_resource_type_from_gl(g_enum type)
    {
        switch (type)
        {
        case GL_FLOAT:
            return shader_resource_type::fsingle;
        case GL_FLOAT_VEC2:
            return shader_resource_type::fvec2;
        case GL_FLOAT_VEC3:
            return shader_resource_type::fvec3;
        case GL_FLOAT_VEC4:
            return shader_resource_type::fvec4;
        case GL_INT:
            return shader_resource_type::isingle;
        case GL_INT_VEC2:
            return shader_resource_type::ivec2;
        case GL_INT_VEC3:
            return shader_resource_type::ivec3;
        case GL_INT_VEC4:
            return shader_resource_type::ivec4;
        case GL_BOOL:
            return shader_resource_type::bsingle;
        case GL_FLOAT_MAT3:
            return shader_resource_type::mat3;
        case GL_FLOAT_MAT4:
            return shader_resource_type::mat4;
        case GL_SAMPLER_2D:
        case GL_SAMPLER_2D_ARRAY:
        case GL_SAMPLER_CUBE:
            return shader_resource_type::isingle; // We only need integers, because the binding of the texture is not done with an uniform.
        case GL_IMAGE_2D:
        case GL_IMAGE_2D_ARRAY:
        case GL_IMAGE_CUBE:
            return shader_resource_type::none; // We don't need that, because the binding of the image texture is not done with an uniform.
        default:
            MANGO_LOG_ERROR("GL Uniform type {0} currently not supported!", type);
            return shader_resource_type::none;
        }
    }

    //! \brief Some parameters required for creation of a \a texture on the gpu.
    enum class texture_parameter : uint8
    {
        filter_nearest,
        filter_linear,
        filter_nearest_mipmap_nearest,
        filter_linear_mipmap_nearest,
        filter_nearest_mipmap_linear,
        filter_linear_mipmap_linear,
        wrap_repeat,
        wrap_clamp_to_edge,
        wrap_clamp_to_border
    };
    //! \brief Converts an wrapping \a texture_parameter to an OpenGl enumeration value.
    //! \param[in] wrapping The \a texture_parameter to convert.
    //! \return The g_enum.
    inline g_enum wrap_parameter_to_gl(const texture_parameter& wrapping)
    {
        switch (wrapping)
        {
        case texture_parameter::wrap_repeat:
            return GL_REPEAT;
        case texture_parameter::wrap_clamp_to_edge:
            return GL_CLAMP_TO_EDGE;
        case texture_parameter::wrap_clamp_to_border:
            return GL_CLAMP_TO_BORDER;
        default:
            MANGO_LOG_ERROR("Unknown texture wrap parameter.");
            return GL_NONE;
        }
    }
    //! \brief Converts a filter \a texture_parameter to an OpenGl enumeration value.
    //! \param[in] filtering The \a texture_parameter to convert.
    //! \return The g_enum.
    inline g_enum filter_parameter_to_gl(const texture_parameter& filtering)
    {
        switch (filtering)
        {
        case texture_parameter::filter_nearest:
            return GL_NEAREST;
        case texture_parameter::filter_linear:
            return GL_LINEAR;
        case texture_parameter::filter_nearest_mipmap_nearest:
            return GL_NEAREST_MIPMAP_NEAREST;
        case texture_parameter::filter_linear_mipmap_nearest:
            return GL_LINEAR_MIPMAP_NEAREST;
        case texture_parameter::filter_nearest_mipmap_linear:
            return GL_NEAREST_MIPMAP_LINEAR;
        case texture_parameter::filter_linear_mipmap_linear:
            return GL_LINEAR_MIPMAP_LINEAR;
        default:
            MANGO_LOG_ERROR("Unknown texture filter parameter.");
            return GL_NONE;
        }
    }
    //! \brief Converts an OpenGl enumeration value to a wrapping \a texture_parameter.
    //! \param[in] wrapping The enumeration value to convert.
    //! \return The \a texture_parameter.
    inline texture_parameter wrap_parameter_from_gl(const g_enum& wrapping)
    {
        switch (wrapping)
        {
        case GL_REPEAT:
            return texture_parameter::wrap_repeat;
        case GL_CLAMP_TO_EDGE:
            return texture_parameter::wrap_clamp_to_edge;
        case GL_CLAMP_TO_BORDER:
            return texture_parameter::wrap_clamp_to_border;
        default:
            MANGO_LOG_WARN("Unknown texture wrap parameter.");
            return texture_parameter::wrap_repeat;
        }
    }
    //! \brief Converts an OpenGl enumeration value to a filter \a texture_parameter.
    //! \param[in] filtering The enumeration value to convert.
    //! \return The \a texture_parameter.
    inline texture_parameter filter_parameter_from_gl(const g_enum& filtering)
    {
        switch (filtering)
        {
        case GL_NEAREST:
            return texture_parameter::filter_nearest;
        case GL_LINEAR:
            return texture_parameter::filter_linear;
        case GL_NEAREST_MIPMAP_NEAREST:
            return texture_parameter::filter_nearest_mipmap_nearest;
        case GL_LINEAR_MIPMAP_NEAREST:
            return texture_parameter::filter_linear_mipmap_nearest;
        case GL_NEAREST_MIPMAP_LINEAR:
            return texture_parameter::filter_nearest_mipmap_linear;
        case GL_LINEAR_MIPMAP_LINEAR:
            return texture_parameter::filter_linear_mipmap_linear;
        default:
            MANGO_LOG_WARN("Unknown texture filter parameter.");
            return texture_parameter::filter_nearest;
        }
    }

    //! \brief Specification of attachments in a \a framebuffer.
    enum class framebuffer_attachment : uint8
    {
        color_attachment0,
        color_attachment1,
        color_attachment2,
        color_attachment3,
        depth_attachment,
        stencil_attachment,
        depth_stencil_attachment
    };

    //! \brief Specification of barrier bits to block Opengl.
    enum class memory_barrier_bit : uint8
    {
        vertex_attrib_array_barrier_bit,
        element_array_barrier_bit,
        uniform_barrier_bit,
        texture_fetch_barrier_bit,
        shader_image_access_barrier_bit,
        command_barrier_bit,
        pixel_buffer_barrier_bit,
        texture_update_barrier_bit,
        buffer_update_barrier_bit,
        framebuffer_barrier_bit,
        transform_feedback_barrier_bit,
        atomic_counter_barrier_bit,
        shader_storage_barrier_bit,
        query_buffer_barrier_bit
    };
    //! \brief Converts a \a memory_barrier_bit to an OpenGl enumeration value.
    //! \param[in] barrier_bit The \a memory_barrier_bit to convert.
    //! \return The corresponding enumeration value.
    inline g_enum memory_barrier_bit_to_gl(const memory_barrier_bit& barrier_bit)
    {
        switch (barrier_bit)
        {
        case memory_barrier_bit::vertex_attrib_array_barrier_bit:
            return GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT;
        case memory_barrier_bit::element_array_barrier_bit:
            return GL_ELEMENT_ARRAY_BARRIER_BIT;
        case memory_barrier_bit::uniform_barrier_bit:
            return GL_UNIFORM_BARRIER_BIT;
        case memory_barrier_bit::texture_fetch_barrier_bit:
            return GL_TEXTURE_FETCH_BARRIER_BIT;
        case memory_barrier_bit::shader_image_access_barrier_bit:
            return GL_SHADER_IMAGE_ACCESS_BARRIER_BIT;
        case memory_barrier_bit::command_barrier_bit:
            return GL_COMMAND_BARRIER_BIT;
        case memory_barrier_bit::pixel_buffer_barrier_bit:
            return GL_PIXEL_BUFFER_BARRIER_BIT;
        case memory_barrier_bit::texture_update_barrier_bit:
            return GL_TEXTURE_UPDATE_BARRIER_BIT;
        case memory_barrier_bit::buffer_update_barrier_bit:
            return GL_BUFFER_UPDATE_BARRIER_BIT;
        case memory_barrier_bit::framebuffer_barrier_bit:
            return GL_FRAMEBUFFER_BARRIER_BIT;
        case memory_barrier_bit::transform_feedback_barrier_bit:
            return GL_TRANSFORM_FEEDBACK_BARRIER_BIT;
        case memory_barrier_bit::atomic_counter_barrier_bit:
            return GL_ATOMIC_COUNTER_BARRIER_BIT;
        case memory_barrier_bit::shader_storage_barrier_bit:
            return GL_SHADER_STORAGE_BARRIER_BIT;
        case memory_barrier_bit::query_buffer_barrier_bit:
            return GL_QUERY_BUFFER_BARRIER_BIT;
        default:
            MANGO_LOG_WARN("Unknown memory barrier bit.");
            return GL_NONE;
        }
    }

} // namespace mango

#endif // MANGO_GRAPHICS_COMMON_HPP