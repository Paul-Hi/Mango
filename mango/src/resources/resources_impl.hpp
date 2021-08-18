//! \file      resources_impl.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_RESOURCES_IMPL_HPP
#define MANGO_RESOURCES_IMPL_HPP

#include <core/context_impl.hpp>
#include <mango/resources.hpp>
#include <memory/free_list_allocator.hpp>
#include <util/hashing.hpp>
#include <util/helpers.hpp>

namespace mango
{
    //! \brief Id used for resources.
    using resource_id = uint64;
    //! \brief Hash for \a resource_description.
    struct resource_hash
    {
      public:
        //! \brief Returns a \a resource_id for a given \a resource_description.
        //! \param[in] description The \a resource_description.
        static inline resource_id get_id(const resource_description& description)
        {
            auto start = string(description.path).find_last_of("\\/") + 1;
            auto name  = string(description.path).substr(start, string(description.path).find_last_of(".") - start);
            return djb2_string_hash::hash(name.c_str());
        }
    };

    //! \brief The \a resources of mango.
    //! \details Responsible for loading and releasing resources.
    class resources_impl : public resources
    {
        MANGO_DISABLE_COPY_AND_ASSIGNMENT(resources_impl)
      public:
        //! \brief Constructs the \a resources_impl.
        resources_impl();
        ~resources_impl();

        const image_resource* acquire(const image_resource_description& description) override;
        void release(const image_resource* resource) override;
        const model_resource* acquire(const model_resource_description& description) override;
        void release(const model_resource* resource) override;
        const shader_resource* acquire(const shader_resource_resource_description& description) override;
        void release(const shader_resource* resource) override;

        //! \brief Updates the \a resources_impl.
        //! \param[in] dt Past time since last call.
        void update(float dt);

      private:
        //! \brief The allocator used to store the resources.
        free_list_allocator m_allocator;

        //! \brief Loads \a image_resource from file.
        //! \param[in] description The \a image_resource_description used for loading the \a image_resource.
        //! \return A pointer to the \a image_resource.
        image_resource* load_image_from_file(const image_resource_description& description);
        //! \brief Loads \a model_resource from file.
        //! \param[in] description The \a model_resource_description used for loading the \a model_resource.
        //! \return A pointer to the \a model_resource.
        model_resource* load_model_from_file(const model_resource_description& description);
        //! \brief Loads \a shader_resource from file.
        //! \param[in] description The \a shader_resource_resource_description used for loading the \a shader_resource.
        //! \return A pointer to the \a shader_resource.
        shader_resource* load_shader_from_file(const shader_resource_resource_description& description);

        //! \brief Loads a shader string from a file.
        //! \param[in] path The full path of the shader source.
        //! \param[in] recursive True if function is called recursive for included shader.
        //! \return The shader source string with all includes and defines.
        string load_shader_string_from_file(const string path, bool recursive);

        //! \brief Cache for resources, mapping \a resource_ids to resource pointers.
        std::unordered_map<resource_id, void*> m_resource_cache;
    };
} // namespace mango

#endif // MANGO_RESOURCES_IMPL_HPP
