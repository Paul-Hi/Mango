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
    template <typename T>
    struct slotmap
    {
      public:
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

        void clear()
        {
            for (int32 i = 0; i < size(); ++i)
            {
                deconstruct(m_data, i);
            }

            m_data.clear();
            m_indices.clear();
            m_indices.push(0);
            m_erase.clear();
            m_freelist_head = 0;
            m_freelist_tail = 0;
        }

        void swap(slotmap& other)
        {
            std::swap(m_data, other.m_data);
            std::swap(m_indices, other.m_indices);
            std::swap(m_erase, other.m_erase);
            m_freelist_head = other.m_freelist_head;
            m_freelist_tail = other.m_freelist_tail;
        }

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

        bool valid(key k) const
        {
            index_type key_gen    = (index_type)(k >> GENERATION_BIT_SHIFT);
            index_type key_ind    = (index_type)(k & INDEX_BIT_MASK);
            index_type stored_gen = (index_type)(m_indices[key_ind] >> GENERATION_BIT_SHIFT);
            return key_gen == stored_gen;
        }

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

        reference operator[](key k)
        {
            index_type key_ind = (index_type)(k & INDEX_BIT_MASK);
            return m_data[(key)m_indices[key_ind] & INDEX_BIT_MASK];
        };

        const_reference operator[](key k) const
        {
            index_type key_ind = (index_type)(k & INDEX_BIT_MASK);
            return m_data[(key)m_indices[key_ind] & INDEX_BIT_MASK];
        };

        bool operator==(const slotmap<T>& other) const
        {
            return m_data == other.m_data;
        }

        bool operator!=(const slotmap<T>& other) const
        {
            return m_data != other.m_data;
        }

        pointer get(key k)
        {
            return valid(k) ? &(*this)[k] : nullptr;
        };

        const_pointer get(key k) const
        {
            return valid(k) ? &(*this)[k] : nullptr;
        };

        size_type size() const
        {
            return m_data.size();
        }

        size_type max_size() const
        {
            return std::numeric_limits<size_type>::max();
        }

        bool empty() const
        {
            return m_data.empty();
        }

        reference back()
        {
            return m_data.back();
        };

        iterator begin()
        {
            return m_data.begin();
        }

        iterator end()
        {
            return m_data.end();
        }

        const_iterator begin() const
        {
            return m_data.begin();
        }

        const_iterator end() const
        {
            return m_data.end();
        }

        const_iterator cbegin() const
        {
            return m_data.cbegin();
        }

        const_iterator cend() const
        {
            return m_data.cend();
        }

        std::vector<key> keys() const
        {
            // TODO: Multiple copies are really bad, but we have a duplicate in the beginning ... -.-
            std::vector<key> keys(m_indices.begin() + 1, m_indices.end());
            return keys;
        }

      private:
        std::vector<T> m_data;
        std::vector<key> m_indices{ {} };
        std::vector<index_type> m_erase;

        index_type m_freelist_head = 0;
        index_type m_freelist_tail = 0;

        template <typename S>
        inline typename std::enable_if<std::is_trivially_destructible<S>::value>::type deconstruct(std::vector<S>&, index_type)
        {
        }

        template <typename S>
        inline typename std::enable_if<!std::is_trivially_destructible<S>::value>::type deconstruct(std::vector<S>& src, index_type index)
        {
            src[index].~T();
        }
    };
} // namespace mango

#endif // MANGO_SLOTMAP