//! \file      graphics_object.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_GRAPHICS_OBJECT_HPP
#define MANGO_GRAPHICS_OBJECT_HPP

#include <graphics/graphics_common.hpp>
#include <mango/types.hpp>

namespace mango
{
    //! \brief The base class for configurations in the graphics part.
    class graphics_configuration
    {
      public:
        //! \brief Returns if the \a graphics_configuration is valid.
        //! \return True, if the configuration is valid, else false.
        virtual bool is_valid() const = 0;

      protected:
        graphics_configuration()  = default;
        ~graphics_configuration() = default;
    };

    //! \brief The base class of every object in the graphics part.
    class graphics_object
    {
      public:
        //! \brief Retrieves the name/handle of the \a graphics_object.
        //! \return The name of the \a graphics_object.
        inline g_uint get_name()
        {
            return m_name;
        }

        //! \brief Checks if the \a graphics_object is in a created state or not.
        //! \return True if object is created, else false.
        inline bool is_created()
        {
            return m_name != 0;
        }

      protected:
        graphics_object() = default;
        ~graphics_object() = default;

        //! \brief The name/handle of the \a graphics_object.
        g_uint m_name;
    };
} // namespace mango

#endif // MANGO_GRAPHICS_OBJECT_HPP
