//! \file      scene_component_manager.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_SCENE_COMPONENT_MANAGER_HPP
#define MANGO_SCENE_COMPONENT_MANAGER_HPP

#include <array>
#include <functional>
#include <mango/assert.hpp>
#include <mango/scene_types.hpp>
#include <unordered_map>

namespace mango
{
    class context_impl;

    //! \brief Manages entities and components for a specific component.
    //! \brief Does all the mapping, provides a quick way to iterate and does provide functionality to get components for entities, entities for components etc.
    template <typename component>
    class scene_component_manager
    {
      public:
        scene_component_manager()
            : end(0)
        {
            m_entities.fill(invalid_entity);
            assert_state();
        }

        //! \brief Checks if an \a entity has a \a component.
        //! \param[in] e The \a entity to check.
        //! \return True if the \a component exists, else false.
        bool contains(entity e) const
        {
            return m_lookup.find(e) != m_lookup.end();
        }

        //! \brief Creates a \a component for a specific \a entity.
        //! \param[in] e The \a entity to create the \a component for.
        //! \return A reference to the newly created \a component.
        component& create_component_for(entity e)
        {
            MANGO_ASSERT(e != invalid_entity, "Entity is not valid!");
            assert_state();
            m_lookup.insert({ e, end });
            m_components.at(end) = component(); // reset component
            m_entities.at(end)   = e;

            return m_components.at(end++);
        }

        //! \brief Removes a \a component from a specific \a entity.
        //! \param[in] e The \a entity to remove the \a component from.
        void remove_component_from(entity e)
        {
            auto it = m_lookup.find(e);
            if (it == m_lookup.end())
            {
                MANGO_LOG_DEBUG("Entity does not have a component of type!"); // TODO Paul: Evtl. get the type and print it out.
                return;
            }
            const size_t index   = it->second;
            const entity indexed = m_entities.at(index);
            MANGO_LOG_DEBUG("e == indexed is {0}", e == indexed);

            if (index < end - 1)
            {
                m_components.at(index)            = std::move(m_components.at(end - 1));
                m_entities.at(index)              = m_entities.back();
                m_lookup.at(m_entities.at(index)) = index;
            }

            m_entities.at(end--) = invalid_entity;
            m_lookup.erase(indexed);
        }

        //! \brief Removes a \a component from a specific \a entity but keeps the list sorted.
        //! \details This is used for the \a node_component to prevent unnecessary sorting.
        //! \param[in] e The \a entity to remove the \a component from.
        void sort_remove_component_from(entity e)
        {
            auto it = m_lookup.find(e);
            if (it == m_lookup.end())
            {
                MANGO_LOG_DEBUG("Entity does not have a component of type!"); // TODO Paul: Evtl. get the type and print it out.
                return;
            }
            const size_t index   = it->second;
            const entity indexed = m_entities.at(index);
            MANGO_LOG_DEBUG("e == indexed is {0}", e == indexed);

            if (index < end - 1)
            {
                for (uint32 i = index + 1; i < end; ++i)
                {
                    m_components.at(i - 1)            = std::move(m_components.at(i));
                    m_entities.at(i - 1)              = m_entities.at(i);
                    m_lookup.at(m_entities.at(i - 1)) = i - 1;
                }
            }

            m_entities.at(end--) = invalid_entity;
            m_lookup.erase(indexed);
        }

        //! \brief Retrieves the \a component of a specific \a entity.
        //! \param[in] e The \a entity to get the \a component from.
        //! \return A pointer to the \a component.
        component* get_component_for_entity(entity e)
        {
            auto it = m_lookup.find(e);
            if (it == m_lookup.end())
            {
                MANGO_LOG_DEBUG("Entity does not have a component of type!"); // TODO Paul: Evtl. get the type and print it out.
                return nullptr;
            }

            return &(m_components.at(it->second));
        }

        //! \brief Retrieves a \a component from the array via an index.
        //! \param[in] index The index in the array.
        //! \return A reference to the \a component at \a index.
        component& operator[](size_t index)
        {
            MANGO_ASSERT(index < end, "Index not valid!");
            return m_components.at(index);
        }

        //! \brief Retrieves a \a component from the array via an index.
        //! \param[in] index The index in the array.
        //! \return A reference to the \a component at \a index.
        inline component& component_at(size_t index)
        {
            MANGO_ASSERT(index < end, "Index not valid!");
            return m_components.at(index);
        }

        //! \brief Retrieves a \a entity from the array via an index.
        //! \param[in] index The index in the array.
        //! \return A reference to the \a entity at \a index.
        inline entity entity_at(size_t index)
        {
            MANGO_ASSERT(index < end, "Index not valid!");
            return m_entities.at(index);
        }

        //! \brief Retrieves the array size.
        //! \details This means the size of the internal array used by the \a scene_component_manager.
        //! \return The size of the array.
        inline size_t size() const
        {
            return end;
        }

        //! \brief Iterates over each \a component and call \a lambda on it.
        //! \param[in] lambda The lambda function to call on each \a component.
        //! \param[in] backwards Specifies if the iteration should be from the last to the first element, or from the first to the last.
        inline void for_each(std::function<void(component& c, uint32& index)> lambda, bool backwards)
        {
            if (backwards)
            {
                for (uint32 i = size() - 1; i >= 0; --i)
                {
                    lambda(m_components.at(i), i);
                }
            }
            else
            {
                for (uint32 i = 0; i < size(); ++i)
                {
                    lambda(m_components.at(i), i);
                }
            }
        }

        //! \brief Moves a \a component in the array.
        //! \details This does also move other \a components to prevent hierarchy destruction.
        //! \param[in] from The index where the \a component is and should be moved away.
        //! \param[in] to The index where the \a componentshould be moved to.
        inline void move(size_t from, size_t to)
        {
            MANGO_ASSERT(from < size(), "Index from not valid!");
            MANGO_ASSERT(to < size(), "Index to not valid!");

            if (from == to)
                return;

            component c = std::move(m_components.at(from));
            entity e    = m_entities.at(from);

            const int d = from < to ? 1 : -1;
            for (size_t i = from; i != to; i += d)
            {
                const size_t next             = i + d;
                m_components.at(i)            = std::move(m_components.at(next));
                m_entities.at(i)              = m_entities.at(next);
                m_lookup.at(m_entities.at(i)) = i;
            }

            m_components.at(to) = std::move(c);
            m_entities.at(to)   = e;
            m_lookup.at(e)      = to;
        }

      private:
      //! \brief The list of \a components.
        std::array<component, max_entities + 1> m_components;
      //! \brief The list of \a entities.
        std::array<entity, max_entities + 1> m_entities;
      //! \brief The current number of entries. Also the next free index.
        size_t end;
      //! \brief A mapping from \a entities to indices.
        std::unordered_map<entity, size_t> m_lookup;

        //! \brief Asserts the internal state of the \a scene_component_manager.
        inline void assert_state()
        {
            MANGO_ASSERT(end <= max_entities, "Too many entities in the system!");
            MANGO_ASSERT(m_lookup.size() == end, "Number of lookups in table != Number of entities!");
        }
    };
} // namespace mango

#endif // MANGO_SCENE_COMPONENT_MANAGER_HPP
