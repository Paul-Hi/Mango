//! \file      texture_structures.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_TEXTURE_STRUCTURES_HPP
#define MANGO_TEXTURE_STRUCTURES_HPP

#include <mango/types.hpp>

namespace mango
{
    //! \brief Some parameters required for creation a texture on the gpu.
    enum texture_parameter
    {
        filter_nearest,                //!< Nearest or point filtering.
        filter_linear,                 //!< Linear filtering.
        filter_nearest_mipmap_nearest, //!< Nearest or point filtering for one texture level and nearest or point filtering between mipmaps. Not valid for mag filter.
        filter_linear_mipmap_nearest,  //!< Linear for one texture level and nearest or point filtering between mipmaps. Not valid for mag filter.
        filter_nearest_mipmap_linear,  //!< Nearest or point filtering for one texture level and linear between mipmaps. Not valid for mag filter.
        filter_linear_mipmap_linear,   //!< Linear filtering for one texture level and linear between mipmaps. Not valid for mag filter.
        wrap_repeat,                   //!< Repeat on texture coordinates > abs(1.0).
        wrap_clamp_to_edge,            //!< Clamp to the edges on texture coordinates > abs(1.0).
        wrap_clamp_to_border           //!< Clamp to a border on texture coordinates > abs(1.0).
    };

    //! \brief The configuration for all two dimensional \a textures.
    struct texture_configuration
    {
        string name; //!< The name of the texture. Used to store it and retrieve it later on.
        texture_parameter texture_min_filter; //!< The min filter used for the texture.
        texture_parameter texture_mag_filter; //!< The mag filter used for the texture.
        texture_parameter texture_wrap_s; //!< The texture wrapping used for the texture.
        texture_parameter texture_wrap_t; //!< The texture wrapping used for the texture.
        bool is_standard_color_space; //!< True if the picture is in standard color space (SRGB etc.) , else false.
        bool generate_mipmaps; //!< Specifies if mipmaps should be generated.
    };

    //! \brief A two dimensional texture.
    //! \details Is only stored on the gpu and accessed through the handle.
    struct texture
    {
        //! \brief The handle of the \a texture.
        uint32 handle;
    };

} // namespace mango

#endif // #define MANGO_TEXTURE_STRUCTURES_HPP
