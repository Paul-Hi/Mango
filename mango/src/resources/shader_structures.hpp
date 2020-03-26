//! \file      shader_structures.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_SHADER_STRUCTURES_HPP
#define MANGO_SHADER_STRUCTURES_HPP

#include <mango/types.hpp>
#include <util/hashing.hpp>

namespace mango
{
    //! \brief Specifies the type of any shader.
    //! \details This is used later to build a shader program.
    enum shader_type
    {
        vertex_shader,   //!< Vertex shader. Mandatory.
        fragment_shader, //!< Fragment shader. Mandatory.
        geometry_shader  //!< Geometry shader. Optional.
        // more will be added if needed.
    };

    //! \brief The configuration data for shaders.
    //! \details This tells the \a shader_system where to load the shader source and is used for caching.
    struct shader_configuration
    {
        string path; //!< The path to the shader source. Relative to the project folder.
        //! \brief The type of the shader this \a shader_configuration specifies.
        shader_type type;
        // Eventually we add defines in here.
        // If there is something missing, we'll add it later too.

        //! \brief Hash function for the \a shader_configuration.
        //! \return The hash value.
        std::size_t hash_code() const
        {
            fnv1a hash;
            hash(path.data(), path.size());
            hash(&type, sizeof(shader_type));
            return static_cast<std::size_t>(hash);
        }

        //! \brief Comparison operator for \a shader_configurations.
        //! \param[in] other The \a shader_configuration to compare this to.
        //! \return True if this and other are equal, else false.
        bool operator==(const shader_configuration& other) const
        {
            return path == other.path && type == other.type;
        }
    };

    //! \brief The data for shaders.
    //! \details Stores the source type and source as well as some binding information.
    //! Main purpose is to cache the data for specific shaders and to provide hot reloading functionality.
    struct shader_data
    {
        //! \brief The type of the shader this \a shader_data is responsible for.
        shader_type type;
        //! \brief The path to the shader source. Relative to the project folder.
        //! \details This is stored in here to provide reloading possibilities.
        string path;
        string source; //!< The shader source string.
    };

    //! \brief The configuration data for shader programs.
    //! \details This tells the \a shader_system what shaders are used in the \a shader_progam.
    //! This is also used for caching.
    struct shader_program_configuration
    {
        uint32 pipeline_steps; //!< The number of pipeline steps.
        //! \brief The \a shader_configurations to load into the program.
        shader_configuration* shader_configs;

        //! \brief Hash function for the \a shader_program_configuration.
        //! \return The hash value.
        std::size_t hash_code() const
        {
            fnv1a hash;
            hash(&pipeline_steps, sizeof(pipeline_steps));
            uint32 hash_value = static_cast<std::size_t>(hash);
            for (uint32 i = 0; i < pipeline_steps; ++i)
                hash_combine(hash_value, shader_configs[i].hash_code());

            return hash_value;
        }

        //! \brief Comparison operator for \a shader_program_configurations.
        //! \param[in] other The \a shader_program_configuration to compare this to.
        //! \return True if this and other are equal, else false.
        bool operator==(const shader_program_configuration& other) const
        {
            if (pipeline_steps != other.pipeline_steps)
                return false;
            for (uint32 i = 0; i < pipeline_steps; ++i)
                if (!(shader_configs[i] == other.shader_configs[i]))
                    return false;
            return true;
        }
    };

    //! \brief A shader program consisting of multiple shaders.
    //! \details Needs to be bound before rendering.
    struct shader_program
    {
        uint32 handle; //!< The handle of the shader program.

        //! \brief A mapping from names to \a gpu_resource_types and binding locations for all shaders in the \a shader_program.
        //! \details This should be set for every sampler and uniform input and output in the \a shader_programs shaders.
        //! It will be used later on to determine valid inputs and outputs and to retrieve the correct location.
        //! This gets populated by the \a shader_system.
        std::unordered_map<string, std::pair<gpu_resource_type, uint32>> binding_data;
    };
} // namespace mango

#endif // #define MANGO_SHADER_STRUCTURES_HPP
