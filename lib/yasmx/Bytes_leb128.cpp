//
// LEB128 implementation.
//
//  Copyright (C) 2001-2008  Peter Johnson
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
#include "yasmx/Bytes_leb128.h"

#include "llvm/ADT/SmallVector.h"
#include "yasmx/Bytes.h"
#include "yasmx/InputBuffer.h"
#include "yasmx/IntNum.h"


using namespace yasm;
using llvm::APInt;

static APInt staticbv(IntNum::BITVECT_NATIVE_SIZE, 0);

static inline uint64_t
Extract(const APInt& bv, unsigned int width, unsigned int lsb)
{
    uint64_t v;
    APInt::tcExtract(&v, 1, bv.getRawData(), width, lsb);
    return v;
}

unsigned long
yasm::WriteLEB128(Bytes& bytes, const IntNum& intn, bool sign)
{
    // Shortcut 0
    if (intn.isZero())
    {
        bytes.push_back(0);
        return 1;
    }

    const APInt* bv = intn.getBV(&staticbv);
    int size;
    if (sign)
        size = bv->getMinSignedBits();
    else
        size = bv->getActiveBits();

    Bytes::size_type orig_size = bytes.size();
    int i = 0;
    for (; i<size-7; i += 7)
        bytes.push_back(static_cast<unsigned char>(Extract(*bv, 7, i)) | 0x80);
    // last byte does not have MSB set
    bytes.push_back(static_cast<unsigned char>(Extract(*bv, 7, i)));
    return static_cast<unsigned long>(bytes.size()-orig_size);
}

unsigned long
yasm::SizeLEB128(const IntNum& intn, bool sign)
{
    // Shortcut 0
    if (intn.isZero())
        return 1;

    const APInt* bv = intn.getBV(&staticbv);
    if (sign)
        return (bv->getMinSignedBits()+6)/7;
    else
        return (bv->getActiveBits()+6)/7;
}

IntNum
yasm::ReadLEB128(InputBuffer& input, bool sign, /*@out@*/ unsigned long* size)
{
    unsigned int nwords = 1;
    SmallVector<uint64_t, 4> words(nwords);
    unsigned int nread = 0, i = 0;

    for (;;)
    {
        unsigned char chv = input.Read(1)[0];
        uint64_t v = chv & 0x7F;
        words[nwords-1] |= v << (i-64*(nwords-1));
        if ((i+7) > 64*nwords)
        {
            // handle word overflow
            words.push_back(v >> ((i+7)-64*nwords));
            ++nwords;
        }
        ++nread;
        i += 7;
        if ((chv & 0x80) != 0x80)
            break;
    }

    if (size)
        *size = nread;

    APInt val(i, nwords, &words[0]);
    // Zero extend if needed to make positive number
    if (!sign && val.isNegative())
        val = val.zext(i+1);

    // Convert to intnum
    IntNum intn;
    intn.setBV(val);
    return intn;
}
