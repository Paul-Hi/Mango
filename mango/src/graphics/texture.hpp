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
        //! \param[in] standard_color_space True if \a texture is srgb, else false.
        //! \param[in] generate_mipmaps The number of mipmaps of the \a texture. Has to be greater than 1.
        //! \param[in] is_cubemap True if \a texture is cubemap, else false.
        //! \param[in] layers Number of layers in this \a texture. Has to be greater than 1.
        texture_configuration(texture_parameter min_filter, texture_parameter mag_filter, texture_parameter wrap_s, texture_parameter wrap_t, bool standard_color_space, int32 generate_mipmaps,
                              bool is_cubemap, int32 layers)
            : graphics_configuration()
            , m_texture_min_filter(min_filter)
            , m_texture_mag_filter(mag_filter)
            , m_texture_wrap_s(wrap_s)
            , m_texture_wrap_t(wrap_t)
            , m_is_standard_color_space(standard_color_space)
            , m_generate_mipmaps(generate_mipmaps)
            , m_is_cubemap(is_cubemap)
            , m_layers(layers)
        {
        }

        //! \brief The filter to use for \a texture minification.
        texture_parameter m_texture_min_filter = texture_parameter::filter_linear;
        //! \brief The filter to use for \a texture magnification.
        texture_parameter m_texture_mag_filter = texture_parameter::filter_linear;
        //! \brief The wrapping procedure in s direction for texture coordinates not in [0, 1].
        texture_parameter m_texture_wrap_s = texture_parameter::wrap_repeat;
        //! \brief The wrapping procedure in t direction for texture coordinates not in [0, 1].
        texture_parameter m_texture_wrap_t = texture_parameter::wrap_repeat;
        //! \brief Specifies if the \a texture should be interpreted as srgb etc.
        bool m_is_standard_color_space = true;
        //! \brief Specifies if a mipchain should be generated.
        int32 m_generate_mipmaps = 1;
        //! \brief Specifies if the \a texture is a cubemap.
        bool m_is_cubemap = false;
        //! \brief Specifies the layer count.
        int32 m_layers = 1;

        // We could need more parameters:
        // glTextureParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        // glTextureParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);

        bool is_valid() const
        {
            return m_texture_mag_filter < texture_parameter::filter_nearest_mipmap_nearest && m_generate_mipmaps > 0; // Eventually we should check that the min and mag filtering options fit.
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

        //! \brief Returns the number of mipmap levels of the \a texture.
        //! \return Number of levels.
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
        //! \brief Returns the number of layers of the \a texture.
        //! \return The number of layers.
        virtual int32 layers() = 0;

        //! \brief Sets the data of the \a texture.
        //! \param[in] internal_format The internal \a texture \a format to use. Has to be \a r8, \a r16, \a r16f, \a r32f, \a r8i, \a r16i, \a r32i, \a r8ui, \a r16ui, \a r32ui, \a rg8, \a rg16,
        //! \a rg16f, \a rg32f, \a rg8i, \a rg16i, \a rg32i, \a rg8ui, \a rg16ui, \a rg32ui, \a rgb32f, \a rgb32i, \a rgb32ui, \a rgba8, \a rgba16, \a rgba16f, \a rgba32f, \a rgba8i, \a rgba16i,
        //! \a rgba32i, \a rgba8ui, \a rgba16ui, \a rgba32ui, \a rgb4, \a rgb5, \a rgb8, \a rgb10, \a rgb12, \a rgb16, \a rgba2, \a rgba4, \a rgb5_a1, \a rgb10_a2 or \a rgba12.
        //! \param[in] width The width of the \a texture. Has to be a positive value.
        //! \param[in] height The height of the \a texture. Has to be a positive value.
        //! \param[in] pixel_format The pixel \a format. Has to be \a red, \a green, \a blue, \a rg, \a rgb, \a bgr, \a rgba, \a bgra, \a red_integer, \a green_integer,
        //! \a blue_integer, \a rg_integer, \a rgb_integer, \a bgr_integer, \a rgba_integer or \a bgra_integer.
        //! \param[in] type the type of the data. has to be \a \a unsigned_byte, \a byte, \a unsigned_short, \a short, \a unsigned_int, \a int, \a float, \a unsigned_byte_3_3_2,
        //! \a unsigned_byte_2_3_3_rev, \a unsigned_short_5_6_5, \a unsigned_short_5_6_5_rev, \a unsigned_short_4_4_4_4, \a unsigned_short_4_4_4_4_rev, \a unsigned_short_5_5_5_1,
        //! \a unsigned_short_1_5_5_5_rev, \a unsigned_int_8_8_8_8, \a unsigned_int_8_8_8_8_rev, \a unsigned_int_10_10_10_2 or \a unsigned_int_2_10_10_10_rev.
        //! \param[in] data The data to set the \a texture memory specified before to.
        //! \param[in] layer The layer of the \a texture to set the data. Has to be a positive value.
        virtual void set_data(format internal_format, int32 width, int32 height, format pixel_format, format type, const void* data,  int32 layer = 0) = 0;

        //! \brief Releases the \a texture.
        virtual void release() = 0;

      protected:
        texture()  = default;
        ~texture() = default;
    };
} // namespace mango

#endif // MANGO_TEXTURE_HPP
