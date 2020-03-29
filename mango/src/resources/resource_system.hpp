//! \file      resource_system.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_RESOURCE_SYSTEM_HPP
#define MANGO_RESOURCE_SYSTEM_HPP

#include <core/context_impl.hpp>
#include <mango/system.hpp>
#include <resources/texture_structures.hpp>
#include <util/hashing.hpp>

namespace mango
{
    //! \brief The minimal handle of a resource only used for storing the real resources.
    //! \details This is used so that after creation all resources can recieved only via name.
    //! If we use the real configurations, we would have to specify all the parameters each time we want to get the texture.
    struct resource_handle
    {
        string name; //!< The name used for caching.

        //! \brief Hash function for the \a resource_handle.
        //! \return The hash value.
        std::size_t hash_code() const
        {
            fnv1a hash;
            hash(name.data(), name.size());
            return static_cast<std::size_t>(hash);
        }

        //! \brief Comparison operator for \a shader_configurations.
        //! \param[in] other The \a resource_handle to compare this to.
        //! \return True if this and other are equal, else false.
        bool operator==(const resource_handle& other) const
        {
            return name == other.name;
        }
    };

    //! \brief The \a resource_system of mango.
    //! \details This system is responsible for all resources in mango.
    //! This includes textures and meshes.
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

        //! \brief Loads a texture.
        //! \param[in] path The path to the texture. Relative to the project folder.
        //! \param[in] configuration The \a texture_configuration of the texture.
        //! \return A pointer to the texture loaded before.
        const shared_ptr<texture> load_texture(const string& path, const texture_configuration& configuration);

        //! \brief Retrieves an already loaded texture.
        //! \param[in] name The name of the texture specified in the \a texture_configuration on load.
        //! \return A pointer to the texture specified.
        const shared_ptr<texture> get_texture(const string& name);

      private:
        //! \brief Mangos internal context for shared usage in the \a resource_system.
        shared_ptr<context_impl> m_shared_context;

        //! \brief The storage for \a textures.
        //! \details The key is a resource_handle which is hashed with fnv1a.
        std::unordered_map<resource_handle, shared_ptr<texture>, hash<resource_handle>> m_texture_storage;
    };

} // namespace mango

#endif // MANGO_RESOURCE_SYSTEM_HPP
