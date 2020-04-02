//! \file      model_structures.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_MODEL_STRUCTURES_HPP
#define MANGO_MODEL_STRUCTURES_HPP

#include <mango/types.hpp>
#include <tiny_gltf.h>

namespace mango
{
    //! \brief The configuration for all \a models.
    struct model_configuration
    {
        string name; //!< The name of the model. Used to store it and retrieve it later on.
    };

    //! \brief A model.
    struct model
    {
        //! \brief The loaded gltf model.
        tinygltf::Model gltf_model;
        //! \brief The \a model_configuration of this \a model.
        model_configuration configuration;
    };

} // namespace mango

#endif // #define MANGO_MODEL_STRUCTURES_HPP
