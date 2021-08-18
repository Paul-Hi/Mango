//! \file      helpers.cpp
//! This file provides helpers functionality for user structures.
//! There is also an implementation of the fnv1a hash function.
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#include <util/helpers.hpp>

using namespace mango;

static bool check_anything(const string& anything, void* ptr, const string& what)
{
    if (!ptr)
    {
        MANGO_LOG_ERROR("{0} of {1} failed! File: {2} Line: {3}!", anything, what, __FILE__, __LINE__);
        return false;
    }
    return true;
}

static bool check_anything(const string& anything, const void* ptr, const string& what)
{
    if (!ptr)
    {
        MANGO_LOG_ERROR("{0} of {1} failed! File: {2} Line: {3}!", anything, what, __FILE__, __LINE__);
        return false;
    }
    return true;
}

bool mango::check_creation(void* ptr, const string& what)
{
    return check_anything("Creation", ptr, what);
}

bool mango::check_mapping(void* ptr, const string& what)
{
    return check_anything("Mapping", ptr, what);
}

bool mango::check_acquisition(void* ptr, const string& what)
{
    return check_anything("Acquisition", ptr, what);
}

bool mango::check_creation(const void* ptr, const string& what)
{
    return check_anything("Creation", ptr, what);
}

bool mango::check_mapping(const void* ptr, const string& what)
{
    return check_anything("Mapping", ptr, what);
}

bool mango::check_acquisition(const void* ptr, const string& what)
{
    return check_anything("Acquisition", ptr, what);
}
