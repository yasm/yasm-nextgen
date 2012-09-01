///
/// @file
/// @brief Bytes utility functions implementation.
///
/// @license
///  Copyright (C) 2008  Peter Johnson
///
/// Redistribution and use in source and binary forms, with or without
/// modification, are permitted provided that the following conditions
/// are met:
///  - Redistributions of source code must retain the above copyright
///    notice, this list of conditions and the following disclaimer.
///  - Redistributions in binary form must reproduce the above copyright
///    notice, this list of conditions and the following disclaimer in the
///    documentation and/or other materials provided with the distribution.
///
/// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND OTHER CONTRIBUTORS ``AS IS''
/// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
/// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
/// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR OTHER CONTRIBUTORS BE
/// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
/// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
/// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
/// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
/// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
/// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
/// POSSIBILITY OF SUCH DAMAGE.
/// @endlicense
///
#include "yasmx/Bytes_util.h"

#include <cassert>

#include "llvm/ADT/APInt.h"
#include "yasmx/IntNum.h"


using namespace yasm;
using llvm::APInt;

static APInt staticbv(IntNum::BITVECT_NATIVE_SIZE, 0);

void
yasm::Write8(Bytes& bytes, const IntNum& intn)
{
    bytes.push_back(static_cast<unsigned char>(intn.Extract(8, 0)));
}

void
yasm::Write16(Bytes& bytes, const IntNum& intn)
{
    Write16(bytes, static_cast<unsigned int>(intn.Extract(16, 0)));
}

void
yasm::Write32(Bytes& bytes, const IntNum& intn)
{
    Write32(bytes, intn.Extract(32, 0));
}

void
yasm::Write64(Bytes& bytes, const IntNum& intn)
{
    unsigned long low = intn.Extract(32, 0);
    unsigned long high = intn.Extract(32, 32);
    if (bytes.isBigEndian())
    {
        Write32(bytes, high);
        Write32(bytes, low);
    }
    else
    {
        Write32(bytes, low);
        Write32(bytes, high);
    }
}

static inline void
Write64I(Bytes& bytes, uint64_t val)
{
    unsigned long low = static_cast<unsigned long>(val);
    unsigned long high = static_cast<unsigned long>(val >> 32);
    if (bytes.isBigEndian())
    {
        Write32(bytes, high);
        Write32(bytes, low);
    }
    else
    {
        Write32(bytes, low);
        Write32(bytes, high);
    }
}

void
yasm::WriteN(Bytes& bytes, const IntNum& intn, int n)
{
    assert((n & 7) == 0 && "n must be a multiple of 8");

    // optimize some fixed cases
    switch (n)
    {
        case 8: Write8(bytes, intn); return;
        case 16: Write16(bytes, intn); return;
        case 32: Write32(bytes, intn); return;
        case 64: Write64(bytes, intn); return;
        default: break;
    }

    // harder cases
    const APInt* bv = intn.getBV(&staticbv);
    const uint64_t* words = bv->getRawData();
    unsigned int nwords = bv->getNumWords();
    APInt tmp;    // must be here so it stays in scope
    if (bv->getBitWidth() < static_cast<unsigned int>(n*8))
    {
        tmp = *bv;
        tmp = tmp.sext(n*8);
        words = tmp.getRawData();
        nwords = tmp.getNumWords();
    }
    if (bytes.isBigEndian())
    {
        // start with bytes of most significant word
        int i = n;
        if ((n & 63) != 0)
        {
            int wend = n & ~63;
            uint64_t last = words[nwords-1];
            for (; i>=wend; i-=8)
            {
                bytes.push_back(static_cast<unsigned char>
                                ((last >> (i-wend)) & 0xFF));
            }
        }
        // rest (if any) is whole words
        unsigned int w = nwords-2;
        for (; i>=64; i-=64, --w)
            Write64I(bytes, words[w]);
    }
    else
    {
        // whole words first
        unsigned int w = 0;
        int i = 0;
        for (; i<=n-64; i+=64, ++w)
            Write64I(bytes, words[w]);
        // finish with bytes
        if (i < n)
        {
            uint64_t last = words[w];
            for (; i<n; i+=8)
            {
                bytes.push_back(static_cast<unsigned char>(last & 0xFF));
                last >>= 8;
            }
        }
    }
}
 
void
yasm::WriteN(Bytes& bytes, unsigned long val, int n)
{
    assert((n & 7) == 0 && "n must be a multiple of 8");
    if (bytes.isBigEndian())
    {
        for (int i=n-8; i>=0; i-=8)
            bytes.push_back(static_cast<unsigned char>((val >> i) & 0xFF));
    }
    else
    {
        for (int i=0; i<n; i+=8)
            bytes.push_back(static_cast<unsigned char>((val >> i) & 0xFF));
    }
}
