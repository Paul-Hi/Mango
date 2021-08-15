//! \file      context.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_PACKED_FREELIST
#define MANGO_PACKED_FREELIST

#include <mango/types.hpp>

namespace mango
{
    //! \brief An id used to index the \a packed_freelist.
    struct packed_freelist_id
    {
        //! \brief Retrieves the internal lookup id.
        //! \return A constant reference to the internal lookup id.
        inline const uint32& get() const
        {
            return lookup_id;
        }

      private:
        template <typename T, ptr_size C>
        friend class packed_freelist;

        //! \brief Id of the lookup.
        //! \details 16 least significant bits = index of this lookup in the lookup array / 16 most significant bits = usage count of this lookup.
        uint32 lookup_id = 0;
    };

    //! \brief A packed list class providing contiguous access.
    template <typename element, ptr_size capacity>
    class packed_freelist
    {
      private:
        //! \brief Maximum number of elements in a \a packed_freelist. 65536 - 1 -> 2^16 - 1 since 0xFFFF is deleted.
        static const uint16 max_elements = 0xFFFF;
        //! \brief Deleted element.
        static const uint16 deleted = 0xFFFF;
        //! \brief Mask for the id to get the 16 bit index for the lookup array.
        static const uint16 id_index_mask = 0xFFFF;
        //! \brief Adds 1 to 16 most significant bits without touching the 16 least significant bits.
        static const uint32 add_one_msb = 0x10000;

        //! \brief Lookup structure used to provide contiguous access.
        struct lookup
        {
            //! \brief The freelist id of this lookup.
            packed_freelist_id id;

            //! \brief Index in the elements array.
            uint16 element_index;
            //! \brief Next free index in the lookup array.
            uint16 next;
        };

      public:
        //! \brief Iterator for the \a packed_freelist.
        struct iterator
        {
            //! \brief Constructs a new iterator starting on given \a packed_freelist_id.
            //! \param[in] id A pointer to the id, the iterator starts on.
            iterator(packed_freelist_id* id)
            {
                m_current = id;
            }

            //! \brief Dereference operator.
            //! \return The \a packed_freelist_id pointed to by the iterator.
            packed_freelist_id operator*()
            {
                return *m_current;
            }

            //! \brief Arrow operator.
            //! \return The pointer to the \a packed_freelist_id pointed to by the iterator.
            packed_freelist_id* operator->()
            {
                return m_current;
            }

            //! \brief Pre-increment operator.
            //! \return The iterator.
            iterator& operator++()
            {
                m_current++;
                return *this;
            }

            //! \brief Post-increment operator.
            //! \return The iterator.
            iterator& operator++(int32)
            {
                iterator tmp = *this;
                ++(*this);
                return tmp;
            }

            //! \brief Comparison operator equal.
            //! \param other The other iterator.
            //! \return True if other iterator is equal to the current one, else false.
            bool operator==(const iterator& other) const
            {
                return m_current == other.m_current;
            }

            //! \brief Comparison operator not equal.
            //! \param other The other iterator.
            //! \return True if other iterator is not equal to the current one, else false.
            bool operator!=(const iterator& other) const
            {
                return m_current != other.m_current;
            }

          private:
            //! \brief The current \a packed_freelist_id the iterator points to.
            packed_freelist_id* m_current;
        };

        packed_freelist()
        {
            static_assert(capacity < max_elements, "Packed Freelist does support a maximum size of 65536!");
            static_assert(capacity > 0, "Packed Freelist doesn't support a size of 0!");
            m_size             = 0;
            m_element_array    = static_cast<element*>(new element[capacity]);
            m_lookup_array     = static_cast<lookup*>(new lookup[capacity]);
            m_reverse_id_array = static_cast<packed_freelist_id*>(new packed_freelist_id[capacity]);

            m_free_id_dequeue = 0;
            m_free_id_enqueue = capacity - 1;

            for (uint16 i = 0; i < capacity; i++)
            {
                m_lookup_array[i].id.lookup_id  = i;
                m_lookup_array[i].element_index = deleted;
                m_lookup_array[i].next          = i + 1;
            }

            m_lookup_array[m_free_id_enqueue].next = 0;
        }

        ~packed_freelist()
        {
            delete[] m_element_array;
            delete[] m_lookup_array;
            delete[] m_reverse_id_array;
        }

        //! \brief Inserts a new element in the \a packed_freelist and returns the corresponding \a packed_freelist_id.
        //! \param[in] value The value of type \a element.
        //! \return The \a packed_freelist_id of the inserted element.
        packed_freelist_id insert(const element& value)
        {
            lookup& look      = m_lookup_array[m_free_id_dequeue];
            m_free_id_dequeue = look.next;
            look.id.lookup_id += add_one_msb; // adds to the use count of the lookup without modifying the lookup index

            // add element to the end ... always
            look.element_index = static_cast<uint16>(m_size);
            m_size++;
            MANGO_ASSERT(m_size <= max_elements, "Element array out of bounds!");

            element* obj = m_element_array + look.element_index; // at the end of the array
            *obj         = value;

            m_reverse_id_array[look.element_index] = look.id;

            return look.id;
        }

        //! \brief Inserts a new element in the \a packed_freelist by moving it and returns the corresponding \a packed_freelist_id.
        //! \param[in] value The value of type \a element to move into the packed_freelist.
        //! \return The \a packed_freelist_id of the inserted element.
        packed_freelist_id insert(const element&& value)
        {
            lookup& look      = m_lookup_array[m_free_id_dequeue];
            m_free_id_dequeue = look.next;
            look.id += add_one_msb; // adds to the use count of the lookup without modifying the lookup index

            // add element to the end ... always
            look.element_index = m_size;
            m_size++;
            MANGO_ASSERT(m_size <= max_elements, "Element array out of bounds!");

            element* obj = m_element_array + look.element_index; // at the end of the array
            *obj         = std::move(value);

            m_reverse_id_array[look.element_index] = look.id;

            return look.id;
        }

        //! \brief Inserts a new element in the \a packed_freelist by emplacing it and returns the corresponding \a packed_freelist_id.
        //! \param[in] args The arguments used for creating the element to emplace.
        //! \return The \a packed_freelist_id of the inserted element.
        template <class... Args>
        packed_freelist_id emplace(Args&&... args)
        {
            lookup& look      = m_lookup_array[m_free_id_dequeue];
            m_free_id_dequeue = look.next;
            look.id.lookup_id += add_one_msb; // adds to the use count of the lookup without modifying the lookup index

            // add element to the end ... always
            look.element_index = static_cast<uint16>(m_size);
            m_size++;
            MANGO_ASSERT(m_size <= max_elements, "Element array out of bounds!");

            element* obj = m_element_array + look.element_index; // at the end of the array
            *obj         = element(std::forward<Args&&...>(args)...);

            m_reverse_id_array[look.element_index] = look.id;

            return look.id;
        }

        //! \brief Checks if a element for a given \a packed_freelist_id is contained in the \a packed_freelist.
        //! \param[in] id The \a packed_freelist_id to check
        //! \return True if the element with \a packed_freelist_id id is contained in the \a packed_freelist, else false.
        inline bool contains(packed_freelist_id id) const
        {
            lookup& look = m_lookup_array[id.lookup_id & id_index_mask];
            return look.id.lookup_id == id.lookup_id && look.element_index != deleted;
        }

        //! \brief Subscript operator.
        //! \param id The \a packed_freelist_id to get the element for.
        //! \return The corresponding element for id.
        inline element& operator[](packed_freelist_id id)
        {
            MANGO_ASSERT(contains(id), "Trying to access non contained value!");
            return *(m_element_array + m_lookup_array[id.lookup_id & id_index_mask].element_index);
        }

        //! \brief Returns an element for a given \a packed_freelist_id.
        //! \param id The \a packed_freelist_id to get the element for.
        //! \return The corresponding element for id.
        inline element& at(packed_freelist_id id)
        {
            MANGO_ASSERT(contains(id), "Trying to access non contained value!");
            return *(m_element_array + m_lookup_array[id.lookup_id & id_index_mask].element_index);
        }

        //! \brief Returns the last element in the \a packed_freelist.
        //! \return The last element.
        inline element& back()
        {
            return *(m_element_array + m_size - 1);
        }

        //! \brief Erases an element from the \a packed_freelist.
        //! \param id The \a packed_freelist_id to erase the corresponding the element for.
        void erase(packed_freelist_id id)
        {
            MANGO_ASSERT(contains(id), "Trying to erase non contained value!");
            lookup& look = m_lookup_array[id.lookup_id & id_index_mask];

            element* obj = m_element_array + look.element_index;

            if (look.element_index != m_size - 1)
            {
                // swap element with last one
                element* last = m_element_array + (m_size - 1);

                // Move the object
                *obj = std::move(*last);

                m_reverse_id_array[look.element_index]                                                         = m_reverse_id_array[m_size - 1];
                m_lookup_array[m_reverse_id_array[look.element_index].lookup_id & id_index_mask].element_index = look.element_index;
            }

            m_size--;

            m_lookup_array[m_free_id_enqueue].next = look.id.lookup_id & id_index_mask;
            m_free_id_enqueue                      = look.id.lookup_id & id_index_mask;

            look.element_index = deleted;
        }

        //! \brief Returns an iterator for the \a packed_freelist pointing to the first element.
        //! \return An iterator pointing to the first element.
        iterator begin() const
        {
            return iterator(m_reverse_id_array);
        }

        //! \brief Returns an iterator pointing to the end of the \a packed_freelist.
        //! \details The end is the last element + 1.
        //! \return An iterator pointing to the end of the \a packed_freelist.
        iterator end() const
        {
            return iterator(m_reverse_id_array + m_size);
        }

        //! \brief Checks if the \a packed_freelist contains no elements.
        //! \return True if the \a packed_freelist is empty, else false.
        inline bool empty() const
        {
            return m_size == 0;
        }

        //! \brief Returns the size of the \a packed_freelist.
        //! \return The number of elements in the \a packed_freelist.
        inline ptr_size size() const
        {
            return m_size;
        }

        //! \brief Returns the templated maximum capacity of the \a packed_freelist.
        //! \return The maximum possible number of elements in the \a packed_freelist.
        inline ptr_size array_capacity() const
        {
            return capacity;
        }

      private:
        //! \brief The size of the \a packed_freelist.
        ptr_size m_size;

        //! \brief Index in the lookup array to enqueue free ids.
        int32 m_free_id_enqueue;
        //! \brief Index in the lookup array to dequeue free ids.
        int32 m_free_id_dequeue;

        //! \brief The contiguous list of elements.
        element* m_element_array;
        //! \brief The contiguous list of lokkups.
        lookup* m_lookup_array;
        //! \brief The contiguous list of \a packed_freelist_ids for reverse lookup.
        packed_freelist_id* m_reverse_id_array;
    };

    //! \brief Returns an iterator for the \a packed_freelist pointing to the first element.
    //! \param[in] list the \a packed_freelist to get the iterator for.
    //! \return An iterator pointing to the first element.
    template <class element, ptr_size capacity>
    typename packed_freelist<element, capacity>::iterator begin(const packed_freelist<element, capacity>& list)
    {
        return list.begin();
    }

    //! \brief Returns an iterator pointing to the end of the \a packed_freelist.
    //! \details The end is the last element + 1.
    //! \param[in] list the \a packed_freelist to get the iterator for.
    //! \return An iterator pointing to the end of the \a packed_freelist.
    template <class element, ptr_size capacity>
    typename packed_freelist<element, capacity>::iterator end(const packed_freelist<element, capacity>& list)
    {
        return list.end();
    }

} // namespace mango

#endif // MANGO_PACKED_FREELIST