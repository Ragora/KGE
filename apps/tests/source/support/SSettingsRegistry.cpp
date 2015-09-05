/**
 *  @file SSettingsRegistry.cpp
 *  @brief Source file containing coding for the SSettingsRegistry tests.
 *
 *  This software is licensed under the Draconic Free License version 1. Please refer
 *  to LICENSE.txt for more information.
 *
 *  @author Draconic Entertainment
 *  @copyright (c) 2014 Draconic Entertainment
 */

#include <gtest/gtest.h>

#include <support/SSettingsRegistry.hpp>

namespace Kiaro
{
    namespace Support
    {
        TEST(SSettingsRegistry, SetGet)
        {
            SSettingsRegistry* settings = SSettingsRegistry::getPointer();

            Common::U32 integerSetting = 1337;
            Common::F32 floatSetting = 3.14f;

            EXPECT_NO_THROW(settings->setValue("integer", integerSetting));
            EXPECT_NO_THROW(settings->setValue("float", floatSetting));

            // Same values?
            EXPECT_EQ(integerSetting, settings->getValue<Common::U32>("integer"));
            EXPECT_EQ(floatSetting, settings->getValue<Common::F32>("float"));

            // Different addresses?
            EXPECT_NE(&integerSetting, &settings->getValue<Common::U32>("integer"));
            EXPECT_NE(&floatSetting, &settings->getValue<Common::F32>("float"));

            SSettingsRegistry::destroy();
        }

        TEST(SSettingsRegistry, Persistance)
        {
            SSettingsRegistry* settings = SSettingsRegistry::getPointer();

            // Write a test block

            SSettingsRegistry::destroy();
        }
    } // End Namespace Support
} // End namespace Kiaro