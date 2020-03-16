//! \file      application.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0
//! \details The classes in this file are all used to fake classes for testing.


#include <gmock/gmock.h>
#include <mango/mango.hpp>

using ::testing::_;
using ::testing::Return;

//! \brief A fake application.
class fake_application : public mango::application
{
  public:
    //! \cond NO_DOC
    MOCK_METHOD(bool, create, (), (override));
    MOCK_METHOD(void, update, (float dt), (override));
    MOCK_METHOD(void, destroy, (), (override));
    //! \endcond
};
