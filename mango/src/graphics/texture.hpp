//! \file      texture.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_TEXTURE_HPP
#define MANGO_TEXTURE_HPP

#include <graphics/graphics_object.hpp>

namespace mango
{
    //! \brief A configuration for \a textures.
    class texture_configuration : public graphics_configuration
    {
      public:
        //! \brief Constructs a new \a texture_configuration with default values.
        texture_configuration()
            : graphics_configuration()
        {
        }

        //! \brief Constructs a new \a texture_configuration.
        //! \param[in] min_filter The filter \a texture_parameter of the \a texture for minification.
        //! \param[in] mag_filter The filter \a texture_parameter of the \a texture for magnification.
        //! \param[in] wrap_s The \a texture_parameter for texture wrapping in s direction.
        //! \param[in] wrap_t The \a texture_parameter for texture wrapping in t direction.
        //! \param[in] standard_color_space True if \a texture is SRGB, else false.
        //! \param[in] generate_mipmaps The number of mipmaps of the \a texture. Has to be greater than 1.
        //! \param[in] is_cubemap True if \a texture is cubemap, else false.
        texture_configuration(texture_parameter min_filter, texture_parameter mag_filter, texture_parameter wrap_s, texture_parameter wrap_t, bool standard_color_space, int32 generate_mipmaps,
                              bool is_cubemap)
            : graphics_configuration()
            , m_texture_min_filter(min_filter)
            , m_texture_mag_filter(mag_filter)
            , m_texture_wrap_s(wrap_s)
            , m_texture_wrap_t(wrap_t)
            , m_is_standard_color_space(standard_color_space)
            , m_generate_mipmaps(generate_mipmaps)
            , m_is_cubemap(is_cubemap)
        {
        }

        //! \brief The filter to use for \a texture minification.
        texture_parameter m_texture_min_filter = texture_parameter::FILTER_LINEAR;
        //! \brief The filter to use for \a texture magnification.
        texture_parameter m_texture_mag_filter = texture_parameter::FILTER_LINEAR;
        //! \brief The wrapping procedure in s direction for texture coordinates not in [0, 1].
        texture_parameter m_texture_wrap_s = texture_parameter::WRAP_REPEAT;
        //! \brief The wrapping procedure in t direction for texture coordinates not in [0, 1].
        texture_parameter m_texture_wrap_t = texture_parameter::WRAP_REPEAT;
        //! \brief Specifies if the \a texture should be interpreted as SRGB etc.
        bool m_is_standard_color_space = true;
        //! \brief Specifies if a mipchain should be generated.
        int32 m_generate_mipmaps = 1;
        //! \brief Specifies if the \a texture is a cubemap.
        bool m_is_cubemap = false;

        // We could need more parameters:
        // glTextureParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        // glTextureParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);

        bool is_valid() const
        {
            return m_texture_mag_filter < texture_parameter::FILTER_NEAREST_MIPMAP_NEAREST && m_generate_mipmaps > 0; // Eventually we should check that the min and mag filtering options fit.
        }
    };

    //! \brief Memory object for \a image data on the gpu.
    //! \details Used to share \a image data between cpu and gpu devices.
    //! Can be bound for sampling in the \a shaders.
    class texture : public graphics_object
    {
      public:
        //! \brief Creates a new \a texture and returns a pointer to it.
        //! \param[in] configuration The \a texture_configuration for the new \a texture.
        //! \return A pointer to the new \a texture.
        static texture_ptr create(const texture_configuration& configuration);

        //! \brief Returns the width of the \a texture in pixels.
        //! \return Width of the \a texture in pixels.
        virtual int32 get_width() = 0;

        //! \brief Returns the height of the \a texture in pixels.
        //! \return Height of the \a texture in pixels.
        virtual int32 get_height() = 0;

        //! \brief Returns the number mipmap levels of the \a texture.
        //! \return 0 if the \a texture has no mipchain, else number of levels.
        virtual int32 mipmaps() = 0;
        //! \brief Returns standard color space specification of the \a texture.
        //! \return True if the \a texture is in standard color space, else false.
        virtual bool is_in_standard_color_space() = 0;
        //! \brief Returns the format of the \a texture.
        //! \return Format of the \a texture.
        virtual format get_format() = 0;
        //! \brief Returns the internal format of the \a texture.
        //! \return Internal format of the \a texture.
        virtual format get_internal_format() = 0;
        //! \brief Returns the component type of each component of the \a texture.
        //! \return Component type of each component of the \a texture.
        virtual format component_type() = 0;
        //! \brief Returns the minification filter of the \a texture.
        //! \return Minification filter of the \a texture.
        virtual texture_parameter min_filter() = 0;
        //! \brief Returns the magnification filter of the \a texture.
        //! \return Magnification filter of the \a texture.
        virtual texture_parameter mag_filter() = 0;
        //! \brief Returns the wrap parameter in s direction of the \a texture.
        //! \return Wrap parameter in s direction of the \a texture.
        virtual texture_parameter wrap_s() = 0;
        //! \brief Returns the wrap parameter in t direction of the \a texture.
        //! \return Wrap parameter in t direction of the \a texture.
        virtual texture_parameter wrap_t() = 0;
        //! \brief Returns if the \a texture is a cubemap.
        //! \return True if the \a texture is a cubemap, else false.
        virtual bool is_cubemap() = 0;

        //! \brief Sets the data of the \a texture.
        //! \param[in] internal_format The internal \a texture \a format to use. Has to be \a R8, \a R16, \a R16F, \a R32F, \a R8I, \a R16I, \a R32I, \a R8UI, \a R16UI, \a R32UI, \a RG8, \a RG16,
        //! \a RG16F, \a RG32F, \a RG8I, \a RG16I, \a RG32I, \a RG8UI, \a RG16UI, \a RG32UI, \a RGB32F, \a RGB32I, \a RGB32UI, \a RGBA8, \a RGBA16, \a RGBA16F, \a RGBA32F, \a RGBA8I, \a RGBA16I,
        //! \a RGBA32I, \a RGBA8UI, \a RGBA16UI, \a RGBA32UI, \a RGB4, \a RGB5, \a RGB8, \a RGB10, \a RGB12, \a RGB16, \a RGBA2, \a RGBA4, \a RGB5_A1, \a RGB10_A2 or \a RGBA12.
        //! \param[in] width The width of the \a texture. Has to be a positive value.
        //! \param[in] height The height of the \a texture. Has to be a positive value.
        //! \param[in] pixel_format The pixel \a format. Has to be \a RED, \a GREEN, \a BLUE, \a RG, \a RGB, \a BGR, \a RGBA, \a BGRA, \a RED_INTEGER, \a GREEN_INTEGER, 
        //! \a BLUE_INTEGER, \a RG_INTEGER, \a RGB_INTEGER, \a BGR_INTEGER, \a RGBA_INTEGER or \a BGRA_INTEGER.
        //! \param[in] type The type of the data. Has to be \a \a UNSIGNED_BYTE, \a BYTE, \a UNSIGNED_SHORT, \a SHORT, \a UNSIGNED_INT, \a INT, \a FLOAT, \a UNSIGNED_BYTE_3_3_2,
        //! \a UNSIGNED_BYTE_2_3_3_REV, \a UNSIGNED_SHORT_5_6_5, \a UNSIGNED_SHORT_5_6_5_REV, \a UNSIGNED_SHORT_4_4_4_4, \a UNSIGNED_SHORT_4_4_4_4_REV, \a UNSIGNED_SHORT_5_5_5_1,
        //! \a UNSIGNED_SHORT_1_5_5_5_REV, \a UNSIGNED_INT_8_8_8_8, \a UNSIGNED_INT_8_8_8_8_REV, \a UNSIGNED_INT_10_10_10_2 or \a UNSIGNED_INT_2_10_10_10_REV.
        //! \param[in] data The data to set the \a texture memory specified before to.
        virtual void set_data(format internal_format, int32 width, int32 height, format pixel_format, format type, const void* data) = 0;

        //! \brief Binds the \a texture to a specific unit.
        //! \param[in] unit The unit to bind the \a texture to. Has to be a positive value.
        virtual void bind_texture_unit(int32 unit) = 0;

        //! \brief Unbinds the \a texture.
        virtual void unbind() = 0;

        //! \brief Releases the \a texture.
        virtual void release() = 0;

      protected:
        texture()  = default;
        ~texture() = default;
    };
} // namespace mango

#endif // MANGO_TEXTURE_HPP
