//! \file      shader_system.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_SHADER_SYSTEM_HPP
#define MANGO_SHADER_SYSTEM_HPP

#include <core/context_impl.hpp>
#include <mango/system.hpp>
#include <resources/shader_structures.hpp>

namespace mango
{
    //! \brief The \a shader_system of mango.
    //! \details This system manages all aspects of shaders in mango.
    //! It loads the shader source, parses the source to get inputs and outputs, stores and caches shader data and is
    //! also responsible for building shader programs and caching them as well.
    class shader_system : public system
    {
      public:
        //! \brief Constructs the \a shader_system.
        //! \param[in] context The internally shared context of mango.
        shader_system(const shared_ptr<context_impl>& context);
        ~shader_system();
        virtual bool create() override;

        virtual void update(float dt) override;
        virtual void destroy() override;

        //! \brief Returns the pointer to a \a shader_program specified by \a configuration.
        //! \details This checks, if the \a shader_program is already created and cached.
        //! \param[in] configuration The \a shader_program_configuration specifying the \a shader_program to retrieve.
        //! \return The pointer to the \a shader_program.
        const shared_ptr<shader_program> get_shader_program(const shader_program_configuration& configuration);

      private:
        //! \brief Returns pointer to the \a shader_data of a shader specified by \a configuration.
        //! \details This checks, if the \a shader_data is already created and cached.
        //! \param[in] configuration The \a shader_configuration specifying the \a shader_data to retrieve.
        //! \return The pointer to the \a shader_data.
        const shared_ptr<shader_data> get_shader_data(const shader_configuration& configuration);

        //! \brief Populates the binding data by parsing a certain shader \a source and querying the uniform location in the \a program.
        //! \param[out] binding_data The binding data to populate.
        //! \param[in] source The shader source to parse inputs and outputs.
        //! \param[in] program The program the \a source is bound to. Used for getting the uniform location.
        void populate_binding_data(std::unordered_map<string, std::pair<gpu_resource_type, uint32>>& binding_data, const string& source, uint32 program);

        //! \brief Mangos internal context for shared usage in the \a shader_system.
        shared_ptr<context_impl> m_shared_context;

        //! \brief The cache for \a shader_data.
        //! \details The key is a shader_configuration which is hashed with fnv1a.
        std::unordered_map<shader_configuration, shared_ptr<shader_data>, hash<shader_configuration>> m_shader_cache;

        //! \brief The cache for \a shader_programs.
        //! \details The key is a shader_program_configuratoion which is hashed with fnv1a.
        std::unordered_map<shader_program_configuration, shared_ptr<shader_program>, hash<shader_program_configuration>> m_shader_program_cache;
    };

} // namespace mango

#endif // MANGO_SHADER_SYSTEM_HPP
