//! \file      image_structures.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_IMAGE_STRUCTURES_HPP
#define MANGO_IMAGE_STRUCTURES_HPP

#include <mango/types.hpp>

namespace mango
{
    //! \brief The configuration for all two dimensional \a images.
    struct image_configuration
    {
        string name;                  //!< The name of the image. Used to store it and retrieve it later on.
        bool is_standard_color_space; //!< True if the picture is in standard color space (srgb etc.) , else false.
        bool is_hdr;                  //!< True if the picture has high dynamic range , else false.
    };

    //! \brief A two dimensional image.
    //! \details Is only stored on the gpu and accessed through the handle.
    struct image
    {
        //! \brief The \a images data.
        void* data;
        //! \brief The loaded width of this \a image.
        int32 width;
        //! \brief The loaded height of this \a image.
        int32 height;
        //! \brief The loaded number of components of this \a image.
        int32 number_components;
        //! \brief The number of bits.
        int32 bits;

        //! \brief The \a image_configuration of this \a image.
        image_configuration configuration;
    };

} // namespace mango

#endif // MANGO_IMAGE_STRUCTURES_HPP
