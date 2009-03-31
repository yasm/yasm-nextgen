//
//  Copyright (C) 2008  Peter Johnson
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND OTHER CONTRIBUTORS ``AS IS''
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR OTHER CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
#include <cxxtest/TestSuite.h>

#include "Bytes_util.h"
#include "IntNum.h"

#ifndef NELEMS
#define NELEMS(array)   (sizeof(array) / sizeof(array[0]))
#endif

using namespace yasm;

class BytesUtilTestSuite : public CxxTest::TestSuite
{
public:

    struct LTest
    {
        long val;
        unsigned char expect[4];
    };

    struct ULTest
    {
        unsigned long val;
        unsigned char expect[4];
    };

    void testWrite8()
    {
        static const LTest lvals[] =
        {
            {0,         {0x00}},
            {-1,        {0xff}},
            {-127,      {0x81}},
            {-128,      {0x80}},
            {-129,      {0x7f}},
            {-254,      {0x02}},
            {-255,      {0x01}},
            {-256,      {0x00}},
            {1,         {0x01}},
            {127,       {0x7f}},
            {128,       {0x80}},
            {129,       {0x81}},
            {254,       {0xfe}},
            {255,       {0xff}},
            {256,       {0x00}},
        };

        static const ULTest ulvals[] =
        {
            {0,         {0x00}},
            {1,         {0x01}},
            {127,       {0x7f}},
            {128,       {0x80}},
            {129,       {0x81}},
            {254,       {0xfe}},
            {255,       {0xff}},
            {256,       {0x00}},
        };

        for (const LTest* ltest=lvals; ltest!=lvals+NELEMS(lvals); ++ltest)
        {
            //TS_TRACE(ltest->val);

            Bytes bytes;
            write_8(bytes, ltest->val);
            TS_ASSERT_EQUALS(bytes.size(), 1U);
            TS_ASSERT_EQUALS(bytes[0], ltest->expect[0]);

            bytes.clear();
            write_8(bytes, IntNum(ltest->val));
            TS_ASSERT_EQUALS(bytes.size(), 1U);
            TS_ASSERT_EQUALS(bytes[0], ltest->expect[0]);
        }

        for (const ULTest* ultest=ulvals; ultest!=ulvals+NELEMS(ulvals); ++ultest)
        {
            //TS_TRACE(ultest->val);

            Bytes bytes;
            write_8(bytes, ultest->val);
            TS_ASSERT_EQUALS(bytes.size(), 1U);
            TS_ASSERT_EQUALS(bytes[0], ultest->expect[0]);

            bytes.clear();
            write_8(bytes, IntNum(ultest->val));
            TS_ASSERT_EQUALS(bytes.size(), 1U);
            TS_ASSERT_EQUALS(bytes[0], ultest->expect[0]);
        }
    }

    void testWrite16()
    {
        static const LTest lvals[] =
        {
            {0,         {0x00, 0x00}},
            {-1,        {0xff, 0xff}},
            {-255,      {0x01, 0xff}},
            {-256,      {0x00, 0xff}},
            {-257,      {0xff, 0xfe}},
            {-32767,    {0x01, 0x80}},
            {-32768,    {0x00, 0x80}},
            {-32769,    {0xff, 0x7f}},
            {1,         {0x01, 0x00}},
            {254,       {0xfe, 0x00}},
            {255,       {0xff, 0x00}},
            {256,       {0x00, 0x01}},
            {32766,     {0xfe, 0x7f}},
            {32767,     {0xff, 0x7f}},
            {32768,     {0x00, 0x80}},
        };

        static const ULTest ulvals[] =
        {
            {0,         {0x00, 0x00}},
            {1,         {0x01, 0x00}},
            {254,       {0xfe, 0x00}},
            {255,       {0xff, 0x00}},
            {256,       {0x00, 0x01}},
            {65534,     {0xfe, 0xff}},
            {65535,     {0xff, 0xff}},
            {65536,     {0x00, 0x00}},
        };

        for (const LTest* ltest=lvals; ltest!=lvals+NELEMS(lvals); ++ltest)
        {
            //TS_TRACE(ltest->val);

            Bytes bytes;
            bytes << little_endian;
            write_16(bytes, ltest->val);
            TS_ASSERT_EQUALS(bytes.size(), 2U);
            TS_ASSERT_EQUALS(bytes[0], ltest->expect[0]);
            TS_ASSERT_EQUALS(bytes[1], ltest->expect[1]);

            bytes.clear();
            write_16(bytes, IntNum(ltest->val));
            TS_ASSERT_EQUALS(bytes.size(), 2U);
            TS_ASSERT_EQUALS(bytes[0], ltest->expect[0]);
            TS_ASSERT_EQUALS(bytes[1], ltest->expect[1]);

            bytes << big_endian;

            bytes.clear();
            write_16(bytes, ltest->val);
            TS_ASSERT_EQUALS(bytes.size(), 2U);
            TS_ASSERT_EQUALS(bytes[0], ltest->expect[1]);
            TS_ASSERT_EQUALS(bytes[1], ltest->expect[0]);

            bytes.clear();
            write_16(bytes, IntNum(ltest->val));
            TS_ASSERT_EQUALS(bytes.size(), 2U);
            TS_ASSERT_EQUALS(bytes[0], ltest->expect[1]);
            TS_ASSERT_EQUALS(bytes[1], ltest->expect[0]);
        }

        for (const ULTest* ultest=ulvals; ultest!=ulvals+NELEMS(ulvals);
             ++ultest)
        {
            //TS_TRACE(ultest->val);

            Bytes bytes;
            bytes << little_endian;
            write_16(bytes, ultest->val);
            TS_ASSERT_EQUALS(bytes.size(), 2U);
            TS_ASSERT_EQUALS(bytes[0], ultest->expect[0]);
            TS_ASSERT_EQUALS(bytes[1], ultest->expect[1]);

            bytes.clear();
            write_16(bytes, IntNum(ultest->val));
            TS_ASSERT_EQUALS(bytes.size(), 2U);
            TS_ASSERT_EQUALS(bytes[0], ultest->expect[0]);
            TS_ASSERT_EQUALS(bytes[1], ultest->expect[1]);

            bytes << big_endian;

            bytes.clear();
            write_16(bytes, ultest->val);
            TS_ASSERT_EQUALS(bytes.size(), 2U);
            TS_ASSERT_EQUALS(bytes[0], ultest->expect[1]);
            TS_ASSERT_EQUALS(bytes[1], ultest->expect[0]);

            bytes.clear();
            write_16(bytes, IntNum(ultest->val));
            TS_ASSERT_EQUALS(bytes.size(), 2U);
            TS_ASSERT_EQUALS(bytes[0], ultest->expect[1]);
            TS_ASSERT_EQUALS(bytes[1], ultest->expect[0]);
        }
    }

    void testWrite32()
    {
        static const LTest lvals[] =
        {
            {0,             {0x00, 0x00, 0x00, 0x00}},
            {-1,            {0xff, 0xff, 0xff, 0xff}},
            {-2147483647,   {0x01, 0x00, 0x00, 0x80}},
            {-2147483648,   {0x00, 0x00, 0x00, 0x80}},
            {1,             {0x01, 0x00, 0x00, 0x00}},
            {2147483646,    {0xfe, 0xff, 0xff, 0x7f}},
            {2147483647,    {0xff, 0xff, 0xff, 0x7f}},
        };

        static const ULTest ulvals[] =
        {
            {0,         {0x00, 0x00, 0x00, 0x00}},
            {1,         {0x01, 0x00, 0x00, 0x00}},
            {65534,     {0xfe, 0xff, 0x00, 0x00}},
            {65535,     {0xff, 0xff, 0x00, 0x00}},
            {65536,     {0x00, 0x00, 0x01, 0x00}},
            {4294967294,{0xfe, 0xff, 0xff, 0xff}},
            {4294967295,{0xff, 0xff, 0xff, 0xff}},
        };

        for (const LTest* ltest=lvals; ltest!=lvals+NELEMS(lvals); ++ltest)
        {
            //TS_TRACE(ltest->val);

            Bytes bytes;
            bytes << little_endian;
            write_32(bytes, ltest->val);
            TS_ASSERT_EQUALS(bytes.size(), 4U);
            TS_ASSERT_EQUALS(bytes[0], ltest->expect[0]);
            TS_ASSERT_EQUALS(bytes[1], ltest->expect[1]);
            TS_ASSERT_EQUALS(bytes[2], ltest->expect[2]);
            TS_ASSERT_EQUALS(bytes[3], ltest->expect[3]);

            bytes.clear();
            write_32(bytes, IntNum(ltest->val));
            TS_ASSERT_EQUALS(bytes.size(), 4U);
            TS_ASSERT_EQUALS(bytes[0], ltest->expect[0]);
            TS_ASSERT_EQUALS(bytes[1], ltest->expect[1]);
            TS_ASSERT_EQUALS(bytes[2], ltest->expect[2]);
            TS_ASSERT_EQUALS(bytes[3], ltest->expect[3]);

            bytes << big_endian;

            bytes.clear();
            write_32(bytes, ltest->val);
            TS_ASSERT_EQUALS(bytes.size(), 4U);
            TS_ASSERT_EQUALS(bytes[0], ltest->expect[3]);
            TS_ASSERT_EQUALS(bytes[1], ltest->expect[2]);
            TS_ASSERT_EQUALS(bytes[2], ltest->expect[1]);
            TS_ASSERT_EQUALS(bytes[3], ltest->expect[0]);

            bytes.clear();
            write_32(bytes, IntNum(ltest->val));
            TS_ASSERT_EQUALS(bytes.size(), 4U);
            TS_ASSERT_EQUALS(bytes[0], ltest->expect[3]);
            TS_ASSERT_EQUALS(bytes[1], ltest->expect[2]);
            TS_ASSERT_EQUALS(bytes[2], ltest->expect[1]);
            TS_ASSERT_EQUALS(bytes[3], ltest->expect[0]);
        }

        for (const ULTest* ultest=ulvals; ultest!=ulvals+NELEMS(ulvals);
             ++ultest)
        {
            //TS_TRACE(ultest->val);

            Bytes bytes;
            bytes << little_endian;
            write_32(bytes, ultest->val);
            TS_ASSERT_EQUALS(bytes.size(), 4U);
            TS_ASSERT_EQUALS(bytes[0], ultest->expect[0]);
            TS_ASSERT_EQUALS(bytes[1], ultest->expect[1]);
            TS_ASSERT_EQUALS(bytes[2], ultest->expect[2]);
            TS_ASSERT_EQUALS(bytes[3], ultest->expect[3]);

            bytes.clear();
            write_32(bytes, IntNum(ultest->val));
            TS_ASSERT_EQUALS(bytes.size(), 4U);
            TS_ASSERT_EQUALS(bytes[0], ultest->expect[0]);
            TS_ASSERT_EQUALS(bytes[1], ultest->expect[1]);
            TS_ASSERT_EQUALS(bytes[2], ultest->expect[2]);
            TS_ASSERT_EQUALS(bytes[3], ultest->expect[3]);

            bytes << big_endian;

            bytes.clear();
            write_32(bytes, ultest->val);
            TS_ASSERT_EQUALS(bytes.size(), 4U);
            TS_ASSERT_EQUALS(bytes[0], ultest->expect[3]);
            TS_ASSERT_EQUALS(bytes[1], ultest->expect[2]);
            TS_ASSERT_EQUALS(bytes[2], ultest->expect[1]);
            TS_ASSERT_EQUALS(bytes[3], ultest->expect[0]);

            bytes.clear();
            write_32(bytes, IntNum(ultest->val));
            TS_ASSERT_EQUALS(bytes.size(), 4U);
            TS_ASSERT_EQUALS(bytes[0], ultest->expect[3]);
            TS_ASSERT_EQUALS(bytes[1], ultest->expect[2]);
            TS_ASSERT_EQUALS(bytes[2], ultest->expect[1]);
            TS_ASSERT_EQUALS(bytes[3], ultest->expect[0]);
        }
    }

    void testWrite64_32()
    {
        static const LTest lvals[] =
        {
            {0,             {0x00, 0x00, 0x00, 0x00}},
            {-1,            {0xff, 0xff, 0xff, 0xff}},
            {-2147483647,   {0x01, 0x00, 0x00, 0x80}},
            {-2147483648,   {0x00, 0x00, 0x00, 0x80}},
            {1,             {0x01, 0x00, 0x00, 0x00}},
            {2147483646,    {0xfe, 0xff, 0xff, 0x7f}},
            {2147483647,    {0xff, 0xff, 0xff, 0x7f}},
        };

        static const ULTest ulvals[] =
        {
            {0,         {0x00, 0x00, 0x00, 0x00}},
            {1,         {0x01, 0x00, 0x00, 0x00}},
            {65534,     {0xfe, 0xff, 0x00, 0x00}},
            {65535,     {0xff, 0xff, 0x00, 0x00}},
            {65536,     {0x00, 0x00, 0x01, 0x00}},
            {4294967294,{0xfe, 0xff, 0xff, 0xff}},
            {4294967295,{0xff, 0xff, 0xff, 0xff}},
        };

        for (const LTest* ltest=lvals; ltest!=lvals+NELEMS(lvals); ++ltest)
        {
            //TS_TRACE(ltest->val);

            Bytes bytes;
            bytes << little_endian;
            write_64(bytes, ltest->val);
            TS_ASSERT_EQUALS(bytes.size(), 8U);
            TS_ASSERT_EQUALS(bytes[0], ltest->expect[0]);
            TS_ASSERT_EQUALS(bytes[1], ltest->expect[1]);
            TS_ASSERT_EQUALS(bytes[2], ltest->expect[2]);
            TS_ASSERT_EQUALS(bytes[3], ltest->expect[3]);
            TS_ASSERT_EQUALS(bytes[4], (ltest->expect[3]&0x80) ? 0xff : 0x00);
            TS_ASSERT_EQUALS(bytes[5], (ltest->expect[3]&0x80) ? 0xff : 0x00);
            TS_ASSERT_EQUALS(bytes[6], (ltest->expect[3]&0x80) ? 0xff : 0x00);
            TS_ASSERT_EQUALS(bytes[7], (ltest->expect[3]&0x80) ? 0xff : 0x00);

            bytes.clear();
            write_64(bytes, IntNum(ltest->val));
            TS_ASSERT_EQUALS(bytes.size(), 8U);
            TS_ASSERT_EQUALS(bytes[0], ltest->expect[0]);
            TS_ASSERT_EQUALS(bytes[1], ltest->expect[1]);
            TS_ASSERT_EQUALS(bytes[2], ltest->expect[2]);
            TS_ASSERT_EQUALS(bytes[3], ltest->expect[3]);
            TS_ASSERT_EQUALS(bytes[4], (ltest->expect[3]&0x80) ? 0xff : 0x00);
            TS_ASSERT_EQUALS(bytes[5], (ltest->expect[3]&0x80) ? 0xff : 0x00);
            TS_ASSERT_EQUALS(bytes[6], (ltest->expect[3]&0x80) ? 0xff : 0x00);
            TS_ASSERT_EQUALS(bytes[7], (ltest->expect[3]&0x80) ? 0xff : 0x00);

            bytes << big_endian;

            bytes.clear();
            write_64(bytes, ltest->val);
            TS_ASSERT_EQUALS(bytes.size(), 8U);
            TS_ASSERT_EQUALS(bytes[0], (ltest->expect[3]&0x80) ? 0xff : 0x00);
            TS_ASSERT_EQUALS(bytes[1], (ltest->expect[3]&0x80) ? 0xff : 0x00);
            TS_ASSERT_EQUALS(bytes[2], (ltest->expect[3]&0x80) ? 0xff : 0x00);
            TS_ASSERT_EQUALS(bytes[3], (ltest->expect[3]&0x80) ? 0xff : 0x00);
            TS_ASSERT_EQUALS(bytes[4], ltest->expect[3]);
            TS_ASSERT_EQUALS(bytes[5], ltest->expect[2]);
            TS_ASSERT_EQUALS(bytes[6], ltest->expect[1]);
            TS_ASSERT_EQUALS(bytes[7], ltest->expect[0]);

            bytes.clear();
            write_64(bytes, IntNum(ltest->val));
            TS_ASSERT_EQUALS(bytes.size(), 8U);
            TS_ASSERT_EQUALS(bytes[0], (ltest->expect[3]&0x80) ? 0xff : 0x00);
            TS_ASSERT_EQUALS(bytes[1], (ltest->expect[3]&0x80) ? 0xff : 0x00);
            TS_ASSERT_EQUALS(bytes[2], (ltest->expect[3]&0x80) ? 0xff : 0x00);
            TS_ASSERT_EQUALS(bytes[3], (ltest->expect[3]&0x80) ? 0xff : 0x00);
            TS_ASSERT_EQUALS(bytes[4], ltest->expect[3]);
            TS_ASSERT_EQUALS(bytes[5], ltest->expect[2]);
            TS_ASSERT_EQUALS(bytes[6], ltest->expect[1]);
            TS_ASSERT_EQUALS(bytes[7], ltest->expect[0]);
        }

        for (const ULTest* ultest=ulvals; ultest!=ulvals+NELEMS(ulvals);
             ++ultest)
        {
            //TS_TRACE(ultest->val);

            Bytes bytes;
            bytes << little_endian;
            write_64(bytes, ultest->val);
            TS_ASSERT_EQUALS(bytes.size(), 8U);
            TS_ASSERT_EQUALS(bytes[0], ultest->expect[0]);
            TS_ASSERT_EQUALS(bytes[1], ultest->expect[1]);
            TS_ASSERT_EQUALS(bytes[2], ultest->expect[2]);
            TS_ASSERT_EQUALS(bytes[3], ultest->expect[3]);
            TS_ASSERT_EQUALS(bytes[4], 0x00);
            TS_ASSERT_EQUALS(bytes[5], 0x00);
            TS_ASSERT_EQUALS(bytes[6], 0x00);
            TS_ASSERT_EQUALS(bytes[7], 0x00);

            bytes.clear();
            write_64(bytes, IntNum(ultest->val));
            TS_ASSERT_EQUALS(bytes.size(), 8U);
            TS_ASSERT_EQUALS(bytes[0], ultest->expect[0]);
            TS_ASSERT_EQUALS(bytes[1], ultest->expect[1]);
            TS_ASSERT_EQUALS(bytes[2], ultest->expect[2]);
            TS_ASSERT_EQUALS(bytes[3], ultest->expect[3]);
            TS_ASSERT_EQUALS(bytes[4], 0x00);
            TS_ASSERT_EQUALS(bytes[5], 0x00);
            TS_ASSERT_EQUALS(bytes[6], 0x00);
            TS_ASSERT_EQUALS(bytes[7], 0x00);

            bytes << big_endian;

            bytes.clear();
            write_64(bytes, ultest->val);
            TS_ASSERT_EQUALS(bytes.size(), 8U);
            TS_ASSERT_EQUALS(bytes[0], 0x00);
            TS_ASSERT_EQUALS(bytes[1], 0x00);
            TS_ASSERT_EQUALS(bytes[2], 0x00);
            TS_ASSERT_EQUALS(bytes[3], 0x00);
            TS_ASSERT_EQUALS(bytes[4], ultest->expect[3]);
            TS_ASSERT_EQUALS(bytes[5], ultest->expect[2]);
            TS_ASSERT_EQUALS(bytes[6], ultest->expect[1]);
            TS_ASSERT_EQUALS(bytes[7], ultest->expect[0]);

            bytes.clear();
            write_64(bytes, IntNum(ultest->val));
            TS_ASSERT_EQUALS(bytes.size(), 8U);
            TS_ASSERT_EQUALS(bytes[0], 0x00);
            TS_ASSERT_EQUALS(bytes[1], 0x00);
            TS_ASSERT_EQUALS(bytes[2], 0x00);
            TS_ASSERT_EQUALS(bytes[3], 0x00);
            TS_ASSERT_EQUALS(bytes[4], ultest->expect[3]);
            TS_ASSERT_EQUALS(bytes[5], ultest->expect[2]);
            TS_ASSERT_EQUALS(bytes[6], ultest->expect[1]);
            TS_ASSERT_EQUALS(bytes[7], ultest->expect[0]);
        }
    }

    void testWrite64_64()
    {
        for (unsigned int i=0; i<64; ++i)
        {
            IntNum intn(1);
            intn <<= i;
            //TS_TRACE(intn);

            Bytes bytes;
            bytes << little_endian;
            write_64(bytes, intn);
            TS_ASSERT_EQUALS(bytes.size(), 8U);
            TS_ASSERT_EQUALS(bytes[0], (i < 8) ? (1<<i) : 0x00);
            TS_ASSERT_EQUALS(bytes[1], (i >= 8 && i < 16) ? (1<<(i-8)) : 0x00);
            TS_ASSERT_EQUALS(bytes[2], (i >= 16 && i < 24) ? (1<<(i-16)) : 0x00);
            TS_ASSERT_EQUALS(bytes[3], (i >= 24 && i < 32) ? (1<<(i-24)) : 0x00);
            TS_ASSERT_EQUALS(bytes[4], (i >= 32 && i < 40) ? (1<<(i-32)) : 0x00);
            TS_ASSERT_EQUALS(bytes[5], (i >= 40 && i < 48) ? (1<<(i-40)) : 0x00);
            TS_ASSERT_EQUALS(bytes[6], (i >= 48 && i < 56) ? (1<<(i-48)) : 0x00);
            TS_ASSERT_EQUALS(bytes[7], (i >= 56 && i < 64) ? (1<<(i-56)) : 0x00);

            bytes << big_endian;

            bytes.clear();
            write_64(bytes, intn);
            TS_ASSERT_EQUALS(bytes.size(), 8U);
            TS_ASSERT_EQUALS(bytes[7], (i < 8) ? (1<<i) : 0x00);
            TS_ASSERT_EQUALS(bytes[6], (i >= 8 && i < 16) ? (1<<(i-8)) : 0x00);
            TS_ASSERT_EQUALS(bytes[5], (i >= 16 && i < 24) ? (1<<(i-16)) : 0x00);
            TS_ASSERT_EQUALS(bytes[4], (i >= 24 && i < 32) ? (1<<(i-24)) : 0x00);
            TS_ASSERT_EQUALS(bytes[3], (i >= 32 && i < 40) ? (1<<(i-32)) : 0x00);
            TS_ASSERT_EQUALS(bytes[2], (i >= 40 && i < 48) ? (1<<(i-40)) : 0x00);
            TS_ASSERT_EQUALS(bytes[1], (i >= 48 && i < 56) ? (1<<(i-48)) : 0x00);
            TS_ASSERT_EQUALS(bytes[0], (i >= 56 && i < 64) ? (1<<(i-56)) : 0x00);

            intn.calc(Op::NOT);

            bytes << little_endian;

            bytes.clear();
            write_64(bytes, intn);
            TS_ASSERT_EQUALS(bytes.size(), 8U);
            TS_ASSERT_EQUALS(bytes[0], 0xff & ((i < 8) ? ~(1<<i) : 0xff));
            TS_ASSERT_EQUALS(bytes[1], 0xff & ((i >= 8 && i < 16) ? ~(1<<(i-8)) : 0xff));
            TS_ASSERT_EQUALS(bytes[2], 0xff & ((i >= 16 && i < 24) ? ~(1<<(i-16)) : 0xff));
            TS_ASSERT_EQUALS(bytes[3], 0xff & ((i >= 24 && i < 32) ? ~(1<<(i-24)) : 0xff));
            TS_ASSERT_EQUALS(bytes[4], 0xff & ((i >= 32 && i < 40) ? ~(1<<(i-32)) : 0xff));
            TS_ASSERT_EQUALS(bytes[5], 0xff & ((i >= 40 && i < 48) ? ~(1<<(i-40)) : 0xff));
            TS_ASSERT_EQUALS(bytes[6], 0xff & ((i >= 48 && i < 56) ? ~(1<<(i-48)) : 0xff));
            TS_ASSERT_EQUALS(bytes[7], 0xff & ((i >= 56 && i < 64) ? ~(1<<(i-56)) : 0xff));

            bytes << big_endian;

            bytes.clear();
            write_64(bytes, intn);
            TS_ASSERT_EQUALS(bytes.size(), 8U);
            TS_ASSERT_EQUALS(bytes[7], 0xff & ((i < 8) ? ~(1<<i) : 0xff));
            TS_ASSERT_EQUALS(bytes[6], 0xff & ((i >= 8 && i < 16) ? ~(1<<(i-8)) : 0xff));
            TS_ASSERT_EQUALS(bytes[5], 0xff & ((i >= 16 && i < 24) ? ~(1<<(i-16)) : 0xff));
            TS_ASSERT_EQUALS(bytes[4], 0xff & ((i >= 24 && i < 32) ? ~(1<<(i-24)) : 0xff));
            TS_ASSERT_EQUALS(bytes[3], 0xff & ((i >= 32 && i < 40) ? ~(1<<(i-32)) : 0xff));
            TS_ASSERT_EQUALS(bytes[2], 0xff & ((i >= 40 && i < 48) ? ~(1<<(i-40)) : 0xff));
            TS_ASSERT_EQUALS(bytes[1], 0xff & ((i >= 48 && i < 56) ? ~(1<<(i-48)) : 0xff));
            TS_ASSERT_EQUALS(bytes[0], 0xff & ((i >= 56 && i < 64) ? ~(1<<(i-56)) : 0xff));
        }
    }
};
