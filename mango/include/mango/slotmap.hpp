//! \file      slotmap.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2022
//! \copyright Apache License 2.0

#ifndef MANGO_SLOTMAP
#define MANGO_SLOTMAP

#include <mango/types.hpp>

namespace mango
{
    //! \brief Data structure storing key value in contiguous memory.
    template <typename T>
    struct slotmap
    {
      public:
        //! \cond NO_COND
        using index_type      = uint32;
        using iterator        = typename std::vector<T>::iterator;
        using const_iterator  = typename std::vector<T>::const_iterator;
        using value_type      = T;
        using reference       = T&;
        using const_reference = const T&;
        using pointer         = T*;
        using const_pointer   = const T*;
        using differnce_type  = typename std::vector<T>::difference_type;
        using size_type       = typename std::vector<T>::size_type;

        // key structure   |  num_bits
        // ----------------+----------------------
        // index           |  32 (0..31)
        // generation      |  32 (32..63)
        static const key INDEX_BIT_MASK       = 0x00000000ffffffffull;
        static const key GENERATION_BIT_MASK  = 0xffffffff00000000ull;
        static const key GENERATION_BIT_SHIFT = 32ull;
        //! \endcond

        //! \brief Clear the \a slotmap.
        void clear()
        {
            for (int32 i = 0; i < size(); ++i)
            {
                deconstruct(m_data, i);
            }

            m_data.clear();
            m_indices.clear();
            m_indices.push_back(0);
            m_erase.clear();
            m_freelist_head = 0;
            m_freelist_tail = 0;
        }

        //! \brief Swap two \a slotmaps.
        //! \param[in, out] other The other \a slotmap to swap with.
        void swap(slotmap& other)
        {
            std::swap(m_data, other.m_data);
            std::swap(m_indices, other.m_indices);
            std::swap(m_erase, other.m_erase);
            m_freelist_head = other.m_freelist_head;
            m_freelist_tail = other.m_freelist_tail;
        }

        //! \brief Insert a value.
        //! \param[in] value The value to insert.
        //! \return The \a key for the inserted value.
        key insert(const T& value)
        {
            if (m_freelist_head == m_freelist_tail)
            {
                m_freelist_tail = static_cast<index_type>(m_indices.size());
                m_indices.push_back(0); // generation 0, index set on insert
                m_indices[m_freelist_head] = (m_indices[m_freelist_head] & ~INDEX_BIT_MASK) | (((key)m_freelist_tail) & INDEX_BIT_MASK);
            }

            key current = m_indices[m_freelist_head];
            m_erase.push_back(m_freelist_head);
            m_indices[m_freelist_head] = (m_indices[m_freelist_head] & ~INDEX_BIT_MASK) | (((key)m_data.size()) & INDEX_BIT_MASK);
            index_type next_one        = current & INDEX_BIT_MASK;
            current                    = (current & ~INDEX_BIT_MASK) | (((key)m_freelist_head) & INDEX_BIT_MASK);
            m_freelist_head            = next_one;
            m_data.push_back(value);

            return current;
        }

        //! \brief Check if a \a key is still valid.
        //! \param[in] k The \a key to check for.
        //! \return True if the \a key is still valid, else false.
        bool valid(key k) const
        {
            index_type key_gen    = (index_type)(k >> GENERATION_BIT_SHIFT);
            index_type key_ind    = (index_type)(k & INDEX_BIT_MASK);
            index_type stored_gen = (index_type)(m_indices[key_ind] >> GENERATION_BIT_SHIFT);
            return key_gen == stored_gen;
        }

        //! \brief Erase a \a key and its value.
        //! \param[in] k The \a key to erase.
        void erase(key k)
        {
            if (!valid(k))
            {
                return;
            }

            index_type key_gen = (index_type)(k >> GENERATION_BIT_SHIFT);
            index_type key_ind = (index_type)(k & INDEX_BIT_MASK);

            index_type data_index = m_indices[key_ind] & INDEX_BIT_MASK;

            // increase generation by 1
            m_indices[key_ind] |= ((key)((-(~(key_gen)))) << GENERATION_BIT_SHIFT);

            deconstruct(m_data, data_index);

            // swap
            index_type last_index = static_cast<index_type>(m_data.size() - 1);
            std::swap(m_data[data_index], m_data[last_index]);
            std::swap(m_erase[data_index], m_erase[last_index]);

            m_data.pop_back();

            if (!m_data.empty())
            {
                m_indices[m_erase[data_index]] = (m_indices[m_erase[data_index]] & ~INDEX_BIT_MASK) | ((key)data_index & INDEX_BIT_MASK);
            }

            m_erase.pop_back();

            m_indices[m_freelist_tail] = (m_indices[m_freelist_tail] & ~INDEX_BIT_MASK) | ((key)key_ind & INDEX_BIT_MASK);
            m_freelist_tail            = key_ind;
        }

        //! \brief Retrieve a value for a given \a key unchecked.
        //! \param[in] k The \a key to retrieve the value for.
        //! \return A reference to the value.
        reference operator[](key k)
        {
            index_type key_ind = (index_type)(k & INDEX_BIT_MASK);
            return m_data[(key)m_indices[key_ind] & INDEX_BIT_MASK];
        };

        //! \brief Retrieve a const value for a given \a key unchecked.
        //! \param[in] k The \a key to retrieve the value for.
        //! \return A const reference to the value.
        const_reference operator[](key k) const
        {
            index_type key_ind = (index_type)(k & INDEX_BIT_MASK);
            return m_data[(key)m_indices[key_ind] & INDEX_BIT_MASK];
        };

        //! \cond NO_COND

        bool operator==(const slotmap<T>& other) const
        {
            return m_data == other.m_data;
        }

        bool operator!=(const slotmap<T>& other) const
        {
            return m_data != other.m_data;
        }

        //! \endcond

        //! \brief Retrieve a pointer to a value for a given \a key if it is valid.
        //! \param[in] k The \a key to retrieve the value for.
        //! \return A pointer to the value or nullptr if it is not valid.
        pointer get(key k)
        {
            return valid(k) ? &(*this)[k] : nullptr;
        };

        //! \brief Retrieve a const pointer to a value for a given \a key if it is valid.
        //! \param[in] k The \a key to retrieve the value for.
        //! \return A const pointer to the value or nullptr if it is not valid.
        const_pointer get(key k) const
        {
            return valid(k) ? &(*this)[k] : nullptr;
        };

        //! \brief Retrieve current size.
        //! \return The current size of the \a slotmap.
        size_type size() const
        {
            return m_data.size();
        }

        //! \brief Retrieve maximum size.
        //! \return The maximum size of the \a slotmap.
        size_type max_size() const
        {
            return std::numeric_limits<size_type>::max();
        }

        //! \brief Check if the \a slotmap is empty.
        //! \return True if the \a slotmap is empty, else false.
        bool empty() const
        {
            return m_data.empty();
        }

        //! \brief Retrieve the last value unchecked (do not call, when empty).
        //! \return A reference to the last value.
        reference back()
        {
            return m_data.back();
        };

        //! \brief Retrieve \a slotmap begin iterator.
        //! \return A \a slotmap iterator pointing to start of the data.
        iterator begin()
        {
            return m_data.begin();
        }

        //! \brief Retrieve \a slotmap end iterator.
        //! \return A \a slotmap iterator pointing to end of the data.
        iterator end()
        {
            return m_data.end();
        }

        //! \brief Retrieve \a slotmap const begin iterator.
        //! \return A constant \a slotmap iterator pointing to start of the data.
        const_iterator cbegin() const
        {
            return m_data.begin();
        }

        //! \brief Retrieve \a slotmap const end iterator.
        //! \return A constant \a slotmap iterator pointing to end of the data.
        const_iterator cend() const
        {
            return m_data.end();
        }

        //! \brief Retrieve a list of all keys in the \a slotmap.
        //! \return A list of all keys in the \a slotmap.
        std::vector<key> keys() const
        {
            // TODO: Multiple copies are really bad, but we have a duplicate in the beginning ... -.-
            std::vector<key> keys(m_indices.begin() + 1, m_indices.end());
            return keys;
        }

      private:
        //! \brief The \a slotmaps data vector.
        std::vector<T> m_data;
        //! \brief The \a slotmaps index vector.
        std::vector<key> m_indices{ {} };
        //! \brief The \a slotmaps erase vector.
        std::vector<index_type> m_erase;

        //! \brief The \a slotmaps index pointer to the first free slot.
        index_type m_freelist_head = 0;
        //! \brief The \a slotmaps index pointer to the last free slot.
        index_type m_freelist_tail = 0;

        //! \cond NO_COND

        template <typename S>
        inline typename std::enable_if<std::is_trivially_destructible<S>::value>::type deconstruct(std::vector<S>&, index_type)
        {
        }

        template <typename S>
        inline typename std::enable_if<!std::is_trivially_destructible<S>::value>::type deconstruct(std::vector<S>& src, index_type index)
        {
            src[index].~T();
        }

        //! \endcond
    };
} // namespace mango

#endif // MANGO_SLOTMAP