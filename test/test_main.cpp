//! \file      test_main.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0
//! \details  This is the main file for test running.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

//! \cond NO_DOC

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}

//! \endcond
