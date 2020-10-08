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
#define MANGO_GRAPHICS_OBJECT(name) \
    class name;                     \
    using name##_ptr = shared_ptr<name>;
#define MANGO_GRAPHICS_OBJECT_IMPL(name) class name_impl;

    MANGO_GRAPHICS_OBJECT(command_buffer)
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
    MANGO_GRAPHICS_OBJECT(uniform_buffer)

#undef MANGO_GRAPHICS_OBJECT
#undef MANGO_GRAPHICS_OBJECT_IMPL
    //! \endcond

    //! \brief The alpha mode of a material.
    enum class alpha_mode : uint8
    {
        MODE_OPAQUE,
        MODE_MASK,
        MODE_BLEND,
        MODE_DITHER
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
            v = b ? 1 : 0;
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
            v = o;
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
        float p0;
        float p1;
        float p2;
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
        float pad;
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
        operator glm::mat3&()
        {
            auto mat = glm::mat3();
            mat[0]   = r0;
            mat[1]   = r1;
            mat[2]   = r2;
            return mat;
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
        operator glm::mat4&()
        {
            auto mat = glm::mat4();
            mat[0]   = r0;
            mat[1]   = r1;
            mat[2]   = r2;
            mat[3]   = r3;
            return mat;
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
        INVALID = 0x0,
        // vertex attribute formats and buffer format types
        BYTE                        = 0x1400,
        UNSIGNED_BYTE               = 0x1401,
        SHORT                       = 0x1402,
        UNSIGNED_SHORT              = 0x1403,
        HALF_FLOAT                  = 0x140B,
        DOUBLE                      = 0x140A,
        FIXED                       = 0x140C,
        FLOAT                       = 0x1406,
        FLOAT_VEC2                  = 0x8B50,
        FLOAT_VEC3                  = 0x8B51,
        FLOAT_VEC4                  = 0x8B52,
        INT                         = 0x1404,
        INT_VEC2                    = 0x8B53,
        INT_VEC3                    = 0x8B54,
        INT_VEC4                    = 0x8B55,
        UNSIGNED_INT                = 0x1405,
        UNSIGNED_INT_VEC2           = 0x8DC6,
        UNSIGNED_INT_VEC3           = 0x8DC7,
        UNSIGNED_INT_VEC4           = 0x8DC8,
        UNSIGNED_BYTE_3_3_2         = 0x8032,
        UNSIGNED_BYTE_2_3_3_REV     = 0x8362,
        UNSIGNED_SHORT_5_6_5        = 0x8363,
        UNSIGNED_SHORT_5_6_5_REV    = 0x8364,
        UNSIGNED_SHORT_4_4_4_4      = 0x8033,
        UNSIGNED_SHORT_4_4_4_4_REV  = 0x8365,
        UNSIGNED_SHORT_5_5_5_1      = 0x8034,
        UNSIGNED_SHORT_1_5_5_5_REV  = 0x8366,
        UNSIGNED_INT_8_8_8_8        = 0x8035,
        UNSIGNED_INT_8_8_8_8_REV    = 0x8367,
        UNSIGNED_INT_10_10_10_2     = 0x8036,
        UNSIGNED_INT_2_10_10_10_REV = 0x8368,
        INT_2_10_10_10_REV          = 0x8D9F,
        // internal_formats
        R8                 = 0x8229,
        R16                = 0x822A,
        R16F               = 0x822D,
        R32F               = 0x822E,
        R8I                = 0x8231,
        R16I               = 0x8233,
        R32I               = 0x8235,
        R8UI               = 0x8232,
        R16UI              = 0x8234,
        R32UI              = 0x8236,
        RG8                = 0x822B,
        RG16               = 0x822C,
        RG16F              = 0x822F,
        RG32F              = 0x8230,
        RG8I               = 0x8237,
        RG16I              = 0x8239,
        RG32I              = 0x823B,
        RG8UI              = 0x8238,
        RG16UI             = 0x823A,
        RG32UI             = 0x823C,
        RGB4               = 0x804F,
        RGB5               = 0x8050,
        RGB8               = 0x8051,
        RGB10              = 0x8052,
        RGB12              = 0x8053,
        RGB16              = 0x8054,
        SRGB8              = 0x8C41,
        SRGB8_ALPHA8       = 0x8C43,
        RGB8UI             = 0x8D7D,
        RGB8I              = 0x8D8F,
        RGB16F             = 0x881B,
        RGB16UI            = 0x8D77,
        RGB16I             = 0x8D89,
        RGB32F             = 0x8815,
        RGB32I             = 0x8D83,
        RGB32UI            = 0x8D71,
        RGBA2              = 0x8055,
        RGBA4              = 0x8056,
        RGB5_A1            = 0x8057,
        RGBA8              = 0x8058,
        RGB10_A2           = 0x8059,
        RGBA12             = 0x805A,
        RGBA16             = 0x805B,
        RGBA16F            = 0x881A,
        RGBA32F            = 0x8814,
        RGBA8I             = 0x8D8E,
        RGBA16I            = 0x8D88,
        RGBA32I            = 0x8D82,
        RGBA8UI            = 0x8D7C,
        RGBA16UI           = 0x8D76,
        RGBA32UI           = 0x8D70,
        DEPTH_COMPONENT32F = 0x8CAC,
        DEPTH_COMPONENT16  = 0x81A5,
        DEPTH_COMPONENT24  = 0x81A6,
        DEPTH_COMPONENT32  = 0x81A7,
        // Pixel formats
        DEPTH_COMPONENT = 0x1902,
        STENCIL_INDEX   = 0x1901,
        DEPTH_STENCIL   = 0x84F9,
        RED             = 0x1903,
        GREEN           = 0x1904,
        BLUE            = 0x1905,
        RG              = 0x8227,
        RGB             = 0x1907,
        BGR             = 0x80E0,
        RGBA            = 0x1908,
        BGRA            = 0x80E1,
        RED_INTEGER     = 0x8D94,
        GREEN_INTEGER   = 0x8D95,
        BLUE_INTEGER    = 0x8D96,
        RG_INTEGER      = 0x8228,
        RGB_INTEGER     = 0x8D98,
        BGR_INTEGER     = 0x8D9A,
        RGBA_INTEGER    = 0x8D99,
        BGRA_INTEGER    = 0x8D9B,
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
        case format::R8:
            number_of_components = 1;
            normalized           = true;
            return GL_UNSIGNED_BYTE;
        case format::R16:
            number_of_components = 1;
            normalized           = true;
            return GL_UNSIGNED_SHORT;
        case format::R16F:
            number_of_components = 1;
            normalized           = false;
            return GL_HALF_FLOAT;
        case format::R32F:
            number_of_components = 1;
            normalized           = false;
            return GL_FLOAT;
        case format::R8I:
            number_of_components = 1;
            normalized           = true;
            return GL_BYTE;
        case format::R16I:
            number_of_components = 1;
            normalized           = true;
            return GL_SHORT;
        case format::R32I:
            number_of_components = 1;
            normalized           = true;
            return GL_INT;
        case format::R8UI:
            number_of_components = 1;
            normalized           = true;
            return GL_UNSIGNED_BYTE;
        case format::R16UI:
            number_of_components = 1;
            normalized           = true;
            return GL_UNSIGNED_SHORT;
        case format::R32UI:
            number_of_components = 1;
            normalized           = true;
            return GL_UNSIGNED_INT;
        case format::RG8:
            number_of_components = 2;
            normalized           = true;
            return GL_UNSIGNED_BYTE;
        case format::RG16:
            number_of_components = 2;
            normalized           = true;
            return GL_UNSIGNED_SHORT;
        case format::RG16F:
            number_of_components = 2;
            normalized           = false;
            return GL_HALF_FLOAT;
        case format::RG32F:
            number_of_components = 2;
            normalized           = false;
            return GL_FLOAT;
        case format::RG8I:
            number_of_components = 2;
            normalized           = true;
            return GL_BYTE;
        case format::RG16I:
            number_of_components = 2;
            normalized           = true;
            return GL_SHORT;
        case format::RG32I:
            number_of_components = 2;
            normalized           = true;
            return GL_INT;
        case format::RG8UI:
            number_of_components = 2;
            normalized           = true;
            return GL_UNSIGNED_BYTE;
        case format::RG16UI:
            number_of_components = 2;
            normalized           = true;
            return GL_UNSIGNED_SHORT;
        case format::RG32UI:
            number_of_components = 2;
            normalized           = true;
            return GL_UNSIGNED_INT;
        case format::RGB8I:
            number_of_components = 3;
            normalized           = true;
            return GL_BYTE;
        case format::RGB8UI:
            number_of_components = 3;
            normalized           = true;
            return GL_UNSIGNED_BYTE;
        case format::RGB16F:
            number_of_components = 3;
            normalized           = false;
            return GL_HALF_FLOAT;
        case format::RGB16I:
            number_of_components = 3;
            normalized           = true;
            return GL_SHORT;
        case format::RGB16UI:
            number_of_components = 3;
            normalized           = true;
            return GL_UNSIGNED_SHORT;
        case format::RGB32F:
            number_of_components = 3;
            normalized           = false;
            return GL_FLOAT;
        case format::RGB32I:
            number_of_components = 3;
            normalized           = true;
            return GL_INT;
        case format::RGB32UI:
            number_of_components = 3;
            normalized           = true;
            return GL_UNSIGNED_INT;
        case format::RGBA8:
            number_of_components = 4;
            normalized           = true;
            return GL_UNSIGNED_BYTE;
        case format::RGBA16:
            number_of_components = 4;
            normalized           = true;
            return GL_SHORT;
        case format::RGBA16F:
            number_of_components = 4;
            normalized           = false;
            return GL_HALF_FLOAT;
        case format::RGBA32F:
            number_of_components = 4;
            normalized           = false;
            return GL_FLOAT;
        case format::RGBA8I:
            number_of_components = 4;
            normalized           = true;
            return GL_BYTE;
        case format::RGBA16I:
            number_of_components = 4;
            normalized           = true;
            return GL_UNSIGNED_BYTE;
        case format::RGBA32I:
            number_of_components = 4;
            normalized           = true;
            return GL_UNSIGNED_BYTE;
        case format::RGBA8UI:
            number_of_components = 4;
            normalized           = true;
            return GL_UNSIGNED_BYTE;
        case format::RGBA16UI:
            number_of_components = 4;
            normalized           = true;
            return GL_UNSIGNED_SHORT;
        case format::RGBA32UI:
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
        case format::BYTE:
            if (number_of_components == 1)
                return format::R8I;
            if (number_of_components == 2)
                return format::RG8I;
            if (number_of_components == 3)
                return format::RGB8I;
            if (number_of_components == 4)
                return format::RGBA8I;
            break;
        case format::UNSIGNED_BYTE:
            if (number_of_components == 1)
                return format::R8UI;
            if (number_of_components == 2)
                return format::RG8UI;
            if (number_of_components == 3)
                return format::RGB8UI;
            if (number_of_components == 4)
                return format::RGBA8UI;
            break;
        case format::SHORT:
            if (number_of_components == 1)
                return format::R16I;
            if (number_of_components == 2)
                return format::RG16I;
            if (number_of_components == 3)
                return format::RGB16I;
            if (number_of_components == 4)
                return format::RGBA16I;
            break;
        case format::UNSIGNED_SHORT:
            if (number_of_components == 1)
                return format::R16UI;
            if (number_of_components == 2)
                return format::RG16UI;
            if (number_of_components == 3)
                return format::RGB16UI;
            if (number_of_components == 4)
                return format::RGBA16UI;
            break;
        case format::INT:
            if (number_of_components == 1)
                return format::R32I;
            if (number_of_components == 2)
                return format::RG32I;
            if (number_of_components == 3)
                return format::RGB32I;
            if (number_of_components == 4)
                return format::RGBA32I;
            break;
        case format::UNSIGNED_INT:
            if (number_of_components == 1)
                return format::R32UI;
            if (number_of_components == 2)
                return format::RG32UI;
            if (number_of_components == 3)
                return format::RGB32UI;
            if (number_of_components == 4)
                return format::RGBA32UI;
            break;
        case format::HALF_FLOAT:
            if (number_of_components == 1)
                return format::R16F;
            if (number_of_components == 2)
                return format::RG16F;
            if (number_of_components == 3)
                return format::RGB16F;
            if (number_of_components == 4)
                return format::RGBA16F;
            break;
        case format::FLOAT:
            if (number_of_components == 1)
                return format::R32F;
            if (number_of_components == 2)
                return format::RG32F;
            if (number_of_components == 3)
                return format::RGB32F;
            if (number_of_components == 4)
                return format::RGBA32F;
            break;
        default:
            MANGO_ASSERT(false, "Invalid format! Could also be, that I did not think of adding this here!");
            return format::INVALID;
        }
        MANGO_ASSERT(false, "Invalid count! Could also be, that I did not think of adding this here!");
        return format::INVALID;
    }

    //! \brief Retrieves the number of machine units, or size in bytes for a format.
    //! \param[in] internal_format The format to get the byte size for.
    //! \return The size in bytes.
    inline int64 number_of_basic_machine_units(const format& internal_format)
    {
        switch (internal_format)
        {
        case format::R8:
            return 1 * sizeof(g_ubyte);
        case format::R16:
            return 1 * sizeof(g_ushort);
        case format::R16F:
            return 1 * sizeof(g_half);
        case format::R32F:
            return 1 * sizeof(g_float);
        case format::R8I:
            return 1 * sizeof(g_byte);
        case format::R16I:
            return 1 * sizeof(g_short);
        case format::R32I:
            return 1 * sizeof(g_int);
        case format::R8UI:
            return 1 * sizeof(g_ubyte);
        case format::R16UI:
            return 1 * sizeof(g_ushort);
        case format::R32UI:
            return 1 * sizeof(g_uint);
        case format::RG8:
            return 2 * sizeof(g_ubyte);
        case format::RG16:
            return 2 * sizeof(g_ushort);
        case format::RG16F:
            return 2 * sizeof(g_half);
        case format::RG32F:
            return 2 * sizeof(g_float);
        case format::RG8I:
            return 2 * sizeof(g_byte);
        case format::RG16I:
            return 2 * sizeof(g_ushort);
        case format::RG32I:
            return 2 * sizeof(g_int);
        case format::RG8UI:
            return 2 * sizeof(g_ubyte);
        case format::RG16UI:
            return 2 * sizeof(g_ushort);
        case format::RG32UI:
            return 2 * sizeof(g_uint);
        case format::RGB32F:
            return 3 * sizeof(g_float);
        case format::RGB32I:
            return 3 * sizeof(g_int);
        case format::RGB32UI:
            return 3 * sizeof(g_uint);
        case format::RGBA8:
            return 4 * sizeof(g_ubyte);
        case format::RGBA16:
            return 4 * sizeof(g_short);
        case format::RGBA16F:
            return 4 * sizeof(g_half);
        case format::RGBA32F:
            return 4 * sizeof(g_float);
        case format::RGBA8I:
            return 4 * sizeof(g_byte);
        case format::RGBA16I:
            return 4 * sizeof(g_short);
        case format::RGBA32I:
            return 4 * sizeof(g_int);
        case format::RGBA8UI:
            return 4 * sizeof(g_ubyte);
        case format::RGBA16UI:
            return 4 * sizeof(g_ushort);
        case format::RGBA32UI:
            return 4 * sizeof(g_uint);
        default:
            MANGO_ASSERT(false, "Invalid internal format! Could also be, that I did not think of adding this here!");
            return 0;
        }
    }

    //! \brief Compare operation used for depth test and similar things.
    enum class compare_operation : uint8
    {
        NEVER,
        LESS,
        EQUAL,
        LESS_EQUAL,
        GREATER,
        NOT_EQUAL,
        GREATER_EQUAL,
        ALWAYS
    };

    //! \brief Converts a \a compare_operation to an OpenGl enumeration value.
    //! \param[in] op The \a compare_operation to convert.
    //! \return The gl enumeration specifying a \a compare_operation.
    inline g_enum compare_operation_to_gl(const compare_operation& op)
    {
        switch (op)
        {
        case compare_operation::NEVER:
            return GL_NEVER;
        case compare_operation::LESS:
            return GL_LESS;
        case compare_operation::EQUAL:
            return GL_EQUAL;
        case compare_operation::LESS_EQUAL:
            return GL_LEQUAL;
        case compare_operation::GREATER:
            return GL_GREATER;
        case compare_operation::NOT_EQUAL:
            return GL_NOTEQUAL;
        case compare_operation::GREATER_EQUAL:
            return GL_GEQUAL;
        case compare_operation::ALWAYS:
            return GL_ALWAYS;
        default:
            MANGO_ASSERT(false, "Unknown compare operation!");
            return GL_NONE;
        }
    }

    //! \brief Enumeration specifying the face of a polygon. Used for face culling.
    enum class polygon_face : uint8
    {
        FACE_BACK           = 1 << 0,
        FACE_FRONT          = 1 << 1,
        FACE_FRONT_AND_BACK = FACE_BACK | FACE_FRONT
    };
    MANGO_ENABLE_BITMASK_OPERATIONS(polygon_face)

    //! \brief Converts a \a polygon_face to an OpenGl enumeration value.
    //! \param[face] op The \a polygon_face to convert.
    //! \return The gl enumeration specifying a \a polygon_face.
    inline g_enum polygon_face_to_gl(const polygon_face& face)
    {
        switch (face)
        {
        case polygon_face::FACE_BACK:
            return GL_BACK;
        case polygon_face::FACE_FRONT:
            return GL_FRONT;
        case polygon_face::FACE_FRONT_AND_BACK:
            return GL_FRONT_AND_BACK;
        default:
            MANGO_ASSERT(false, "Unknown polygon face!");
            return GL_NONE;
        }
    }

    //! \brief Enumeration specifying how a polygon should be drawn. For example used to render wireframes.
    enum class polygon_mode : uint8
    {
        POINT,
        LINE,
        FILL
    };

    //! \brief Converts a \a polygon_mode to an OpenGl enumeration value.
    //! \param[in] mode The \a polygon_mode to convert.
    //! \return The gl enumeration specifying a \a polygon_mode.
    inline g_enum polygon_mode_to_gl(const polygon_mode& mode)
    {
        switch (mode)
        {
        case polygon_mode::POINT:
            return GL_POINT;
        case polygon_mode::LINE:
            return GL_LINE;
        case polygon_mode::FILL:
            return GL_FILL;
        default:
            MANGO_ASSERT(false, "Unknown polygon mode!");
            return GL_NONE;
        }
    }

    //! \brief The blend factor used for blending operations.
    enum class blend_factor : uint8
    {
        ZERO,
        ONE,
        SRC_COLOR,
        ONE_MINUS_SRC_COLOR,
        DST_COLOR,
        ONE_MINUS_DST_COLOR,
        SRC_ALPHA,
        ONE_MINUS_SRC_ALPHA,
        DST_ALPHA,
        ONE_MINUS_DST_ALPHA,
        CONSTANT_COLOR,
        ONE_MINUS_CONSTANT_COLOR,
        CONSTANT_ALPHA,
        ONE_MINUS_CONSTANT_ALPHA,
        SRC_ALPHA_SATURATE,
        SRC1_COLOR,
        ONE_MINUS_SRC1_COLOR,
        SRC1_ALPHA,
        ONE_MINUS_SRC1_ALPHA
    };

    //! \brief Converts a \a blend_factor to an OpenGl enumeration value.
    //! \param[in] factor The \a blend_factor to convert.
    //! \return The gl enumeration specifying a \a blend_factor.
    inline g_enum blend_factor_to_gl(const blend_factor& factor)
    {
        switch (factor)
        {
        case blend_factor::ZERO:
            return GL_ZERO;
        case blend_factor::ONE:
            return GL_ONE;
        case blend_factor::SRC_COLOR:
            return GL_SRC_COLOR;
        case blend_factor::ONE_MINUS_SRC_COLOR:
            return GL_ONE_MINUS_SRC_COLOR;
        case blend_factor::DST_COLOR:
            return GL_DST_COLOR;
        case blend_factor::ONE_MINUS_DST_COLOR:
            return GL_ONE_MINUS_DST_COLOR;
        case blend_factor::SRC_ALPHA:
            return GL_SRC_ALPHA;
        case blend_factor::ONE_MINUS_SRC_ALPHA:
            return GL_ONE_MINUS_SRC_ALPHA;
        case blend_factor::DST_ALPHA:
            return GL_DST_ALPHA;
        case blend_factor::ONE_MINUS_DST_ALPHA:
            return GL_ONE_MINUS_DST_ALPHA;
        case blend_factor::CONSTANT_COLOR:
            return GL_CONSTANT_COLOR;
        case blend_factor::ONE_MINUS_CONSTANT_COLOR:
            return GL_ONE_MINUS_CONSTANT_COLOR;
        case blend_factor::CONSTANT_ALPHA:
            return GL_CONSTANT_ALPHA;
        case blend_factor::ONE_MINUS_CONSTANT_ALPHA:
            return GL_ONE_MINUS_CONSTANT_ALPHA;
        case blend_factor::SRC_ALPHA_SATURATE:
            return GL_SRC_ALPHA_SATURATE;
        case blend_factor::SRC1_COLOR:
            return GL_SRC1_COLOR;
        case blend_factor::ONE_MINUS_SRC1_COLOR:
            return GL_ONE_MINUS_SRC1_COLOR;
        case blend_factor::SRC1_ALPHA:
            return GL_SRC1_ALPHA;
        case blend_factor::ONE_MINUS_SRC1_ALPHA:
            return GL_ONE_MINUS_SRC1_ALPHA;
        default:
            MANGO_ASSERT(false, "Unknown blend factor!");
            return GL_NONE;
        }
    }

    //! \brief Mask used to specify buffers that should be cleared.
    enum class clear_buffer_mask : uint8
    {
        COLOR_BUFFER         = 1 << 0,
        DEPTH_BUFFER         = 1 << 1,
        STENCIL_BUFFER       = 1 << 2,
        DEPTH_STENCIL_BUFFER = 1 << 3,

        NONE                        = 0,
        COLOR_AND_DEPTH             = COLOR_BUFFER | DEPTH_BUFFER,
        COLOR_AND_STENCIL           = COLOR_BUFFER | STENCIL_BUFFER,
        COLOR_AND_DEPTH_AND_STENCIL = COLOR_BUFFER | DEPTH_BUFFER | STENCIL_BUFFER,
        COLOR_AND_DEPTH_STENCIL     = COLOR_BUFFER | DEPTH_STENCIL_BUFFER
    };
    MANGO_ENABLE_BITMASK_OPERATIONS(clear_buffer_mask)

    //! \brief Mask used to specify attachements that should be cleared.
    enum class attachment_mask : uint8
    {
        DRAW_BUFFER0   = 1 << 0,
        DRAW_BUFFER1   = 1 << 1,
        DRAW_BUFFER2   = 1 << 2,
        DRAW_BUFFER3   = 1 << 3,
        DRAW_BUFFER4   = 1 << 4,
        DRAW_BUFFER5   = 1 << 5,
        DEPTH_BUFFER   = 1 << 6,
        STENCIL_BUFFER = 1 << 7,

        NONE                         = 0,
        ALL_DRAW_BUFFERS             = DRAW_BUFFER0 | DRAW_BUFFER1 | DRAW_BUFFER2 | DRAW_BUFFER3 | DRAW_BUFFER4 | DRAW_BUFFER5,
        ALL_DRAW_BUFFERS_AND_DEPTH   = ALL_DRAW_BUFFERS | DEPTH_BUFFER,
        ALL_DRAW_BUFFERS_AND_STENCIL = ALL_DRAW_BUFFERS | STENCIL_BUFFER,
        DEPTH_STENCIL_BUFFER         = DEPTH_BUFFER | STENCIL_BUFFER,
        ALL                          = ALL_DRAW_BUFFERS | DEPTH_STENCIL_BUFFER,
    };
    MANGO_ENABLE_BITMASK_OPERATIONS(attachment_mask)

    //! \brief The targets buffer can be bound to.
    enum class buffer_target : uint8
    {
        NONE,
        VERTEX_BUFFER,
        INDEX_BUFFER,
        UNIFORM_BUFFER,
        SHADER_STORAGE_BUFFER,
        TEXTURE_BUFFER
    };

    //! \brief A set of access bits used for accessing buffers.
    enum class buffer_access : uint8
    {
        NONE                     = 0,
        DYNAMIC_STORAGE          = 1 << 0,
        MAPPED_ACCESS_READ       = 1 << 1, // We want to map in a specific way, so we do not give any other options.
        MAPPED_ACCESS_WRITE      = 1 << 2, // We want to map in a specific way, so we do not give any other options.
        MAPPED_ACCESS_READ_WRITE = MAPPED_ACCESS_READ | MAPPED_ACCESS_WRITE
    };
    MANGO_ENABLE_BITMASK_OPERATIONS(buffer_access)

    //! \brief A set of access bits used for general access.
    enum class base_access : uint8
    {
        NONE = 0,
        READ_ONLY,
        WRITE_ONLY,
        READ_WRITE
    };
    //! \brief Converts a \a base_access to an OpenGl enumeration value.
    //! \param[in] access The \a base_access to convert.
    //! \return The gl enumeration specifying a \a base_access.
    inline g_enum base_access_to_gl(const base_access& access)
    {
        switch (access)
        {
        case base_access::READ_ONLY:
            return GL_READ_ONLY;
        case base_access::WRITE_ONLY:
            return GL_WRITE_ONLY;
        case base_access::READ_WRITE:
            return GL_READ_WRITE;
        default:
            return GL_NONE;
        }
    }

    //! \brief The type of a \a shader.
    enum class shader_type : uint8
    {
        NONE = 0,

        VERTEX_SHADER,
        TESSELATION_CONTROL_SHADER,
        TESSELATION_EVALUATION_SHADER,
        GEOMETRY_SHADER,
        FRAGMENT_SHADER,

        COMPUTE_SHADER
    };
    //! \brief Converts a \a shader_type to an OpenGl enumeration value.
    //! \param[in] type The \a shader_type to convert.
    //! \return The gl enumeration specifying a \a shader_type.
    inline g_enum shader_type_to_gl(const shader_type& type)
    {
        switch (type)
        {
        case shader_type::VERTEX_SHADER:
            return GL_VERTEX_SHADER;
        case shader_type::TESSELATION_CONTROL_SHADER:
            return GL_TESS_CONTROL_SHADER;
        case shader_type::TESSELATION_EVALUATION_SHADER:
            return GL_TESS_EVALUATION_SHADER;
        case shader_type::GEOMETRY_SHADER:
            return GL_GEOMETRY_SHADER;
        case shader_type::FRAGMENT_SHADER:
            return GL_FRAGMENT_SHADER;
        case shader_type::COMPUTE_SHADER:
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
        NONE,
        FLOAT,
        FVEC2,
        FVEC3,
        FVEC4,
        // DOUBLE,
        // DVEC2,
        // DVEC3,
        // DVEC4,
        INT,
        IVEC2,
        IVEC3,
        IVEC4,
        // UNSIGNED_INT,
        // UVEC2,
        // UVEC3,
        // UVEC4,
        // BOOL,
        // BVEC2,
        // BVEC3,
        // BVEC4,
        // MAT2,
        MAT3,
        MAT4,
    };
    //! \brief Converts an OpenGl enumeration value to a \a shader_resource_type.
    //! \param[in] type The g_enum to convert.
    //! \return The \a shader_resource_type.
    inline shader_resource_type shader_resource_type_from_gl(g_enum type)
    {
        switch (type)
        {
        case GL_FLOAT:
            return shader_resource_type::FLOAT;
        case GL_FLOAT_VEC2:
            return shader_resource_type::FVEC2;
        case GL_FLOAT_VEC3:
            return shader_resource_type::FVEC3;
        case GL_FLOAT_VEC4:
            return shader_resource_type::FVEC4;
        case GL_INT:
            return shader_resource_type::INT;
        case GL_INT_VEC2:
            return shader_resource_type::IVEC2;
        case GL_INT_VEC3:
            return shader_resource_type::IVEC3;
        case GL_INT_VEC4:
            return shader_resource_type::IVEC4;
        case GL_BOOL:
            return shader_resource_type::INT;
        case GL_FLOAT_MAT3:
            return shader_resource_type::MAT3;
        case GL_FLOAT_MAT4:
            return shader_resource_type::MAT4;
        case GL_SAMPLER_2D:
        case GL_SAMPLER_2D_ARRAY:
        case GL_SAMPLER_CUBE:
            return shader_resource_type::INT; // We only need integers, because the binding of the texture is not done with an uniform.
        case GL_IMAGE_2D:
        case GL_IMAGE_2D_ARRAY:
        case GL_IMAGE_CUBE:
            return shader_resource_type::NONE; // We don't need that, because the binding of the image texture is not done with an uniform.
        default:
            MANGO_LOG_ERROR("GL Uniform type {0} currently not supported!", type);
            return shader_resource_type::NONE;
        }
    }

    //! \brief Some parameters required for creation of a \a texture on the gpu.
    enum class texture_parameter : uint8
    {
        FILTER_NEAREST,
        FILTER_LINEAR,
        FILTER_NEAREST_MIPMAP_NEAREST,
        FILTER_LINEAR_MIPMAP_NEAREST,
        FILTER_NEAREST_MIPMAP_LINEAR,
        FILTER_LINEAR_MIPMAP_LINEAR,
        WRAP_REPEAT,
        WRAP_CLAMP_TO_EDGE,
        WRAP_CLAMP_TO_BORDER
    };
    //! \brief Converts an wrapping \a texture_parameter to an OpenGl enumeration value.
    //! \param[in] wrapping The \a texture_parameter to convert.
    //! \return The g_enum.
    inline g_enum wrap_parameter_to_gl(const texture_parameter& wrapping)
    {
        switch (wrapping)
        {
        case texture_parameter::WRAP_REPEAT:
            return GL_REPEAT;
        case texture_parameter::WRAP_CLAMP_TO_EDGE:
            return GL_CLAMP_TO_EDGE;
        case texture_parameter::WRAP_CLAMP_TO_BORDER:
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
        case texture_parameter::FILTER_NEAREST:
            return GL_NEAREST;
        case texture_parameter::FILTER_LINEAR:
            return GL_LINEAR;
        case texture_parameter::FILTER_NEAREST_MIPMAP_NEAREST:
            return GL_NEAREST_MIPMAP_NEAREST;
        case texture_parameter::FILTER_LINEAR_MIPMAP_NEAREST:
            return GL_LINEAR_MIPMAP_NEAREST;
        case texture_parameter::FILTER_NEAREST_MIPMAP_LINEAR:
            return GL_NEAREST_MIPMAP_LINEAR;
        case texture_parameter::FILTER_LINEAR_MIPMAP_LINEAR:
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
            return texture_parameter::WRAP_REPEAT;
        case GL_CLAMP_TO_EDGE:
            return texture_parameter::WRAP_CLAMP_TO_EDGE;
        case GL_CLAMP_TO_BORDER:
            return texture_parameter::WRAP_CLAMP_TO_BORDER;
        default:
            MANGO_LOG_WARN("Unknown texture wrap parameter.");
            return texture_parameter::WRAP_REPEAT;
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
            return texture_parameter::FILTER_NEAREST;
        case GL_LINEAR:
            return texture_parameter::FILTER_LINEAR;
        case GL_NEAREST_MIPMAP_NEAREST:
            return texture_parameter::FILTER_NEAREST_MIPMAP_NEAREST;
        case GL_LINEAR_MIPMAP_NEAREST:
            return texture_parameter::FILTER_LINEAR_MIPMAP_NEAREST;
        case GL_NEAREST_MIPMAP_LINEAR:
            return texture_parameter::FILTER_NEAREST_MIPMAP_LINEAR;
        case GL_LINEAR_MIPMAP_LINEAR:
            return texture_parameter::FILTER_LINEAR_MIPMAP_LINEAR;
        default:
            MANGO_LOG_WARN("Unknown texture filter parameter.");
            return texture_parameter::FILTER_NEAREST;
        }
    }

    //! \brief Specification of attachments in a \a framebuffer.
    enum class framebuffer_attachment : uint8
    {
        COLOR_ATTACHMENT0,
        COLOR_ATTACHMENT1,
        COLOR_ATTACHMENT2,
        COLOR_ATTACHMENT3,
        DEPTH_ATTACHMENT,
        STENCIL_ATTACHMENT,
        DEPTH_STENCIL_ATTACHMENT
    };

    //! \brief Specification of barrier bits to block Opengl.
    enum class memory_barrier_bit : uint8
    {
        VERTEX_ATTRIB_ARRAY_BARRIER_BIT,
        ELEMENT_ARRAY_BARRIER_BIT,
        UNIFORM_BARRIER_BIT,
        TEXTURE_FETCH_BARRIER_BIT,
        SHADER_IMAGE_ACCESS_BARRIER_BIT,
        COMMAND_BARRIER_BIT,
        PIXEL_BUFFER_BARRIER_BIT,
        TEXTURE_UPDATE_BARRIER_BIT,
        BUFFER_UPDATE_BARRIER_BIT,
        FRAMEBUFFER_BARRIER_BIT,
        TRANSFORM_FEEDBACK_BARRIER_BIT,
        ATOMIC_COUNTER_BARRIER_BIT,
        SHADER_STORAGE_BARRIER_BIT,
        QUERY_BUFFER_BARRIER_BIT
    };
    //! \brief Converts a \a memory_barrier_bit to an OpenGl enumeration value.
    //! \param[in] barrier_bit The \a memory_barrier_bit to convert.
    //! \return The corresponding enumeration value.
    inline g_enum memory_barrier_bit_to_gl(const memory_barrier_bit& barrier_bit)
    {
        switch (barrier_bit)
        {
        case memory_barrier_bit::VERTEX_ATTRIB_ARRAY_BARRIER_BIT:
            return GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT;
        case memory_barrier_bit::ELEMENT_ARRAY_BARRIER_BIT:
            return GL_ELEMENT_ARRAY_BARRIER_BIT;
        case memory_barrier_bit::UNIFORM_BARRIER_BIT:
            return GL_UNIFORM_BARRIER_BIT;
        case memory_barrier_bit::TEXTURE_FETCH_BARRIER_BIT:
            return GL_TEXTURE_FETCH_BARRIER_BIT;
        case memory_barrier_bit::SHADER_IMAGE_ACCESS_BARRIER_BIT:
            return GL_SHADER_IMAGE_ACCESS_BARRIER_BIT;
        case memory_barrier_bit::COMMAND_BARRIER_BIT:
            return GL_COMMAND_BARRIER_BIT;
        case memory_barrier_bit::PIXEL_BUFFER_BARRIER_BIT:
            return GL_PIXEL_BUFFER_BARRIER_BIT;
        case memory_barrier_bit::TEXTURE_UPDATE_BARRIER_BIT:
            return GL_TEXTURE_UPDATE_BARRIER_BIT;
        case memory_barrier_bit::BUFFER_UPDATE_BARRIER_BIT:
            return GL_BUFFER_UPDATE_BARRIER_BIT;
        case memory_barrier_bit::FRAMEBUFFER_BARRIER_BIT:
            return GL_FRAMEBUFFER_BARRIER_BIT;
        case memory_barrier_bit::TRANSFORM_FEEDBACK_BARRIER_BIT:
            return GL_TRANSFORM_FEEDBACK_BARRIER_BIT;
        case memory_barrier_bit::ATOMIC_COUNTER_BARRIER_BIT:
            return GL_ATOMIC_COUNTER_BARRIER_BIT;
        case memory_barrier_bit::SHADER_STORAGE_BARRIER_BIT:
            return GL_SHADER_STORAGE_BARRIER_BIT;
        case memory_barrier_bit::QUERY_BUFFER_BARRIER_BIT:
            return GL_QUERY_BUFFER_BARRIER_BIT;
        default:
            MANGO_LOG_WARN("Unknown memory barrier bit.");
            return GL_NONE;
        }
    }

} // namespace mango

#endif // MANGO_GRAPHICS_COMMON_HPP