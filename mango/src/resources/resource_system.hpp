//! \file      resource_system.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_RESOURCE_SYSTEM_HPP
#define MANGO_RESOURCE_SYSTEM_HPP

#include <core/context_impl.hpp>
#include <mango/system.hpp>
#include <resources/image_structures.hpp>
#include <resources/model_structures.hpp>
#include <util/hashing.hpp>

namespace mango
{
    //! \brief The minimal handle of a resource only used for storing the real resources.
    //! \details This is used so that after creation all resources can recieved only via name.
    //! If we use the real configurations, we would have to specify all the parameters each time we want to get the image.
    struct resource_handle
    {
        string name; //!< The name used for caching.

        //! \brief Hash function for the \a resource_handle.
        //! \return The hash value.
        ptr_size hash_code() const
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
    //! This includes images and meshes.
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

        //! \brief Gets an image and loads it when necessary.
        //! \param[in] path The path to the image. Relative to the project folder.
        //! \param[in] configuration The \a image_configuration of the image.
        //! \return A pointer to the image loaded before.
        const shared_ptr<image> get_image(const string& path, const image_configuration& configuration);

        //! \brief Gets a gltf model and loads it when necessary.
        //! \param[in] path The path to the model. Relative to the project folder.
        //! \param[in] configuration The \a model_configuration of the model.
        //! \return A pointer to the model loaded before.
        const shared_ptr<model> get_gltf_model(const string& path, const model_configuration& configuration);

      private:
        //! \brief Mangos internal context for shared usage in the \a resource_system.
        shared_ptr<context_impl> m_shared_context;
    };

} // namespace mango

#endif // MANGO_RESOURCE_SYSTEM_HPP
