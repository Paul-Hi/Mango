//! \file      resource_structures.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_RESOURCE_STRUCTURES_HPP
#define MANGO_RESOURCE_STRUCTURES_HPP

#include <mango/types.hpp>
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE
#include <tiny_gltf.h> // TODO Paul: We should think about an own representation of models.

namespace mango
{
    //! \brief Base resource description.
    struct resource_description
    {
        const char* path; //!< Resource path.
    };

    //! \brief The description for \a image_resources.
    struct image_resource_description : public resource_description
    {
        bool is_standard_color_space; //!< True if the picture is in standard color space (srgb etc.) , else false.
        bool is_hdr;                  //!< True if the picture has high dynamic range , else false.
    };

    //! \brief The description for \a model_resources.
    struct model_resource_description : public resource_description
    {
    };

    //! \brief Structure describing a define in a shader.
    struct shader_define
    {
        const char* name; //!< The name of the define.
        const char* value; //!< The value of the define.
    };

    //! \brief The description for \a shader_resources.
    struct shader_resource_resource_description : public resource_description
    {
        //! \brief The defines injected into a shader source.
        std::vector<shader_define> defines;
    };

    //! \brief Reference counted base for all resources.
    struct resource_base
    {
        friend class resources_impl;
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

        //! \brief The \a image_resource_description of this \a image.
        image_resource_description description;
    };

    //! \brief A model resource.
    struct model_resource : public resource_base
    {
        //! \brief The loaded gltf model.
        tinygltf::Model gltf_model;
        //! \brief The \a model_resource_description of this \a model.
        model_resource_description description;
    };

    //! \brief A shader resource.
    struct shader_resource : public resource_base
    {
        //! \brief The loaded shader source string.
        string source;
        //! \brief The \a shader_resource_resource_description of this \a shader.
        shader_resource_resource_description description;
    };
} // namespace mango

#endif // MANGO_RESOURCE_STRUCTURES_HPP
