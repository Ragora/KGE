/**
 *  @file BitStream.cpp
 *  @brief BitStream testing implementation.
 *
 *  This software is licensed under the GNU Lesser General Public License version 3.
 *  Please refer to gpl.txt and lgpl.txt for more information.
 *
 *  @author Draconic Entertainment
 *  @version 0.0.0.19
 *  @date 3/19/2014
 *  @copyright (c) 2014 Draconic Entertainment
 */

#include <engine/Config.hpp>

#if ENGINE_TESTS>0
    #ifndef _INCLUDE_KIARO_TESTS_BITSTREAM_H_
    #define _INCLUDE_KIARO_TESTS_BITSTREAM_H_

    #include <gtest/gtest.h>

    #include <support/BitStream.hpp>

    static Kiaro::Common::F32 float_list[] =
    {
        3.14159,
        1337.125,
    };

    static Kiaro::Common::U32 floatCount = sizeof(float_list) / sizeof(Kiaro::Common::F32);

    inline void PackFloats(Kiaro::Support::BitStream &destination)
    {
        for (Kiaro::Common::U32 iteration = 0; iteration < floatCount; iteration++)
            destination.writeF32(float_list[iteration]);
    }

    TEST(BitStreamTest, Floats)
    {
        Kiaro::Common::U32 expectedStreamSize = floatCount * sizeof(Kiaro::Common::F32);

        Kiaro::Support::BitStream floatStream(expectedStreamSize);
        PackFloats(floatStream);

        // Check if our BitStream size is correct
        EXPECT_EQ(expectedStreamSize, floatStream.length());

        // Now make sure we can unpack the data correctly
        for (Kiaro::Common::U32 iteration = 0; iteration < floatCount; iteration++)
            EXPECT_EQ(float_list[(floatCount - 1) - iteration], floatStream.readF32());
    }

    TEST(BitStreamTest, BadStartFloats)
    {
        Kiaro::Common::U32 expectedStreamSize = floatCount * sizeof(Kiaro::Common::F32);

        Kiaro::Support::BitStream floatStream(expectedStreamSize - 3);
        PackFloats(floatStream);

        // Check if our BitStream size is correct
        EXPECT_EQ(expectedStreamSize, floatStream.length());

        // Now make sure we can unpack the data correctly
        for (Kiaro::Common::S32 iteration = floatCount - 1; iteration > -1; iteration--)
            EXPECT_EQ(float_list[iteration], floatStream.readF32());
    }
    #endif // _INCLUDE_KIARO_TESTS_H_
#endif // ENGINE_TESTS
