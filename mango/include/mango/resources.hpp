//! \file      resources.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_RESOURCES_HPP
#define MANGO_RESOURCES_HPP

#include <mango/resource_structures.hpp>

namespace mango
{
    //! \brief The \a resources of mango.
    //! \details Responsible for loading and releasing resources.
    class resources
    {
      public:
        virtual ~resources() = default;

        //! \brief Retrieves, lazy loads and returns an \a image_resource.
        //! \param[in] description The \a image_resource_description used for retrieving the \a image_resource.
        //! \return A pointer to the \a image_resource. Should be released later on.
        virtual const image_resource* acquire(const image_resource_description& description) = 0;
        //! \brief Releases an aquired \a image_resource.
        //! \param[in] resource The \a image_resource to release.
        virtual void release(const image_resource* resource) = 0;

        //! \brief Retrieves, lazy loads and returns a \a model_resource.
        //! \param[in] description The \a model_resource_description used for retrieving the \a model_resource.
        //! \return A pointer to the \a model_resource. Should be released later on.
        virtual const model_resource* acquire(const model_resource_description& description) = 0;
        //! \brief Releases an aquired \a model_resource.
        //! \param[in] resource The \a model_resource to release.
        virtual void release(const model_resource* resource) = 0;

        //! \brief Retrieves, lazy loads and returns a \a shader_resource.
        //! \param[in] description The \a shader_resource_resource_description used for retrieving the \a shader_resource.
        //! \return A pointer to the \a shader_resource. Should be released later on.
        virtual const shader_resource* acquire(const shader_resource_resource_description& description) = 0;
        //! \brief Releases an aquired \a shader_resource.
        //! \param[in] resource The \a shader_resource to release.
        virtual void release(const shader_resource* resource) = 0;
    };


    //! \brief A unique pointer holding the \a resources.
    using resources_ptr    = std::unique_ptr<resources>;

    //! \brief A pointer pointing to the \a resources.
    using resources_handle = resources*;

} // namespace mango

#endif // MANGO_RESOURCES_HPP
