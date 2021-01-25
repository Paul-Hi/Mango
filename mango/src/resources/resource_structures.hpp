//! \file      resource_structures.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_RESOURCE_STRUCTURES_HPP
#define MANGO_RESOURCE_STRUCTURES_HPP

#include <mango/types.hpp>
#include <tiny_gltf.h>

namespace mango
{
    //! \brief Base resource configuration.
    struct resource_configuration
    {
        const char* path; //!< Resource path.
    };

    //! \brief The configuration for \a image_resources.
    struct image_resource_configuration : public resource_configuration
    {
        bool is_standard_color_space; //!< True if the picture is in standard color space (srgb etc.) , else false.
        bool is_hdr;                  //!< True if the picture has high dynamic range , else false.
    };

    //! \brief The configuration for \a model_resources.
    struct model_resource_configuration : public resource_configuration
    {
    };

    //! \brief Reference counted base for all resources.
    struct resource_base
    {
        friend class resource_system;
      private:
        //! \brief Reference counter.
        int32 reference_count = 0;
    };

    //! \brief An image resource.
    struct image_resource : public resource_base
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

        //! \brief The \a image_resource_configuration of this \a image.
        image_resource_configuration configuration;
    };

    //! \brief A model resource.
    struct model_resource : public resource_base
    {
        //! \brief The loaded gltf model.
        tinygltf::Model gltf_model;
        //! \brief The \a model_resource_configuration of this \a model.
        model_resource_configuration configuration;
    };
} // namespace mango

#endif // MANGO_RESOURCE_STRUCTURES_HPP
