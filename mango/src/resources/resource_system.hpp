//! \file      resource_system.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_RESOURCE_SYSTEM_HPP
#define MANGO_RESOURCE_SYSTEM_HPP

#include <core/context_impl.hpp>
#include <mango/system.hpp>
#include <memory/free_list_allocator.hpp>
#include <resources/resource_structures.hpp>
#include <util/hashing.hpp>

namespace mango
{
    using resource_id = uint64;
    struct resource_hash
    {
      public:
        static inline resource_id get_id(const resource_configuration& config)
        {
            auto start = string(config.path).find_last_of("\\/") + 1;
            auto name  = string(config.path).substr(start, string(config.path).find_last_of(".") - start);
            return djb2_string_hash::hash(name.c_str());
        }
    };

    //! \brief The \a resource_system of mango.
    //! \details This system is responsible for loading and releasing resources.
    class resource_system : public system
    {
      public:
        //! \brief Constructs the \a resource_system.
        //! \param[in] context The internally shared context of mango.
        resource_system(const shared_ptr<context_impl>& context);
        ~resource_system();
        virtual bool create() override;

        virtual void update(float dt) override;
        virtual void destroy() override;

        const image_resource* acquire(const image_resource_configuration& configuration);
        const void release(const image_resource* resource);

        const model_resource* acquire(const model_resource_configuration& configuration);
        const void release(const model_resource* resource);

      private:
        //! \brief Mangos internal context for shared usage in the \a resource_system.
        shared_ptr<context_impl> m_shared_context;

        free_list_allocator m_allocator;

        image_resource* load_image_from_file(const image_resource_configuration& configuration);
        model_resource* load_model_from_file(const model_resource_configuration& configuration);

        std::unordered_map<resource_id, void*> m_resource_cache;
    };

} // namespace mango

#endif // MANGO_RESOURCE_SYSTEM_HPP
