//! \file      hashing.hpp
//! This file provides hashing functionality for user structures.
//! There is also an implementation of the fnv1a hash function.
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_HASHING_HPP
#define MANGO_HASHING_HPP

#include <mango/types.hpp>
#include <type_traits>

namespace mango
{
    //! \brief Implementation of the Fowler–Noll–Vo hash function.
    //! \details Uses 32-bit values. Can be used to hash user structures holding only pod types.
    class fnv1a
    {
        //! \brief Internal hash state to chain calls.
        std::size_t m_state = 0x811c9dc5;

      public:
        //! \brief Hashes \a key and attaches it to the state.
        //! \param[in] key A pointer to the value to hash.
        //! \param[in] length The length of \a key in bytes.
        void operator()(void const* key, std::size_t length) noexcept
        {
            unsigned char const* p       = static_cast<unsigned char const*>(key);
            unsigned char const* const e = p + length;
            for (; p < e; ++p)
                m_state = (m_state ^ *p) * 0x1000193;
        }

        //! \brief Returns the state holding the chained hashes.
        //! \return The state which is a hash value for all added hash calls.
        explicit operator std::size_t() noexcept
        {
            return m_state;
        }
    };

    //! \brief Some helper class to check if a template class has a \a hash_code() function. Using SFINAE.
    template <typename T>
    class has_hash_code_function
    {
        //! \cond NO_COND
        typedef char yes[1];
        typedef char no[2];

        template <typename C>
        static yes& test(decltype(&C::hash_code));
        template <typename C>
        static no& test(...);

      public:
        enum
        {
            value = sizeof(test<T>(0)) == sizeof(yes)
        };
        //! \endcond
    };

    //! \brief Structure to provide generic hash if \a T has hash_code() function.
    template <typename T, typename std::enable_if<has_hash_code_function<T>::value, T>::type* = nullptr>
    struct hash
    {
        //! \brief Returns the hash for a given \a key.
        //! \param[in] key The value to hash.
        //! \return The hash value for the \a key.
        std::size_t operator()(const T& key) const
        {
            return key.hash_code();
        }
    };

    //! \brief Combines two hash values and 'adds' them to the first one.
    //! \param[in,out] h0 The first hash value. \a h1 will be combined with \a h0 and \a h0 then holds the result.
    //! \param[in] h1 The second hash value.
    inline void hash_combine(std::size_t& h0, std::size_t h1)
    {
        h0 ^= h1 + 0x9e3779b9 + (h0 << 6) + (h0 >> 2);
    }
} // namespace mango

#endif // MANGO_HASHING_HPP