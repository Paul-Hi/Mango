//! \file      helpers.cpp
//! This file provides helpers functionality for user structures.
//! There is also an implementation of the fnv1a hash function.
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <util/helpers.hpp>

using namespace mango;

static bool check_anything(const string& anything, void* ptr, const string& what, const string& system)
{
    if (!ptr)
    {
        MANGO_LOG_ERROR("{0} of {1} failed! {2} not available!", anything, what, system);
        return false;
    }
    return true;
}

bool mango::check_creation(void* ptr, const string& what, const string& system)
{
    return check_anything("Creation", ptr, what, system);
}

bool mango::check_mapping(void* ptr, const string& what, const string& system)
{
    return check_anything("Mapping", ptr, what, system);
}
