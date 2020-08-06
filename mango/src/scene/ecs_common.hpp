//! \file      ecs_common.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_ECS_COMMON_HPP
#define MANGO_ECS_COMMON_HPP

#include <mango/types.hpp>
#include <unordered_map>
#include <vector>

namespace mango
{
    using entity_id = uint32;
    // 1 - 16 entity_id
    // 17 child_of flag
    // 18 - 32 version

    const entity_id child_of  = 1 << 23;
    const uint16 version_mask = ((1 << 8) - 1);

    using ecs_type = std::vector<entity_id>;

    struct component_list
    {
        void* elements;
        int size;
    };

    struct entity_archetype;
    struct transition
    {
        entity_archetype* add;
        entity_archetype* remove;
    };

    struct entity_archetype
    {
        ecs_type e_type;
        std::vector<entity_id> entities;
        std::vector<component_list> components;
        std::vector<transition> transitions;
    };

    struct entity_record
    {
        entity_archetype* archetype;
        int32 row;
    };

    class id_gen
    {
        inline static uint16 _id{};

      public:
        template <typename Type>
        inline static const uint16 type = _id++;
    };

    class world
    {
        std::unordered_map<entity_id, entity_record> entity_index;
        std::vector<entity_archetype> archetypes;
        bool has_component(entity_id entity, entity_id component)
        {
            auto e_type = entity_index.at(entity).archetype->e_type;
            for (auto el : e_type)
                if (component == el)
                    return true;
            return false;
        }
    };

    // public API

    class entity_t
    {
        template <typename T>
        bool has_component(world w)
        {
            auto e_type = w.entity_index.at(id).archetype->e_type;
            for (auto el : e_type)
                if (T::id() == el)
                    return true;
            return false;
        }
        entity_id id;
    };

    class ecs
    {
        static entity_t entity(const string& name);
    };
} // namespace mango

#endif // MANGO_ECS_COMMON_HPP