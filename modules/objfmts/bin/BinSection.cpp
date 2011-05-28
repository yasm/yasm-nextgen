//
// Flat-format binary object format section data
//
//  Copyright (C) 2002-2008  Peter Johnson
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
#include "BinSection.h"


using namespace yasm;
using namespace yasm::objfmt;

const char* BinSection::key = "objfmt::bin::BinSection";

BinSection::BinSection()
    : has_align(false),
      has_valign(false),
      has_istart(false),
      has_ivstart(false),
      has_length(false)
{
}

BinSection::~BinSection()
{
}

pugi::xml_node
BinSection::Write(pugi::xml_node out) const
{
    pugi::xml_node root = out.append_child("BinSection");
    root.append_attribute("key") = key;
    if (has_align)
        append_child(root, "Align", align);
    if (has_valign)
        append_child(root, "VAlign", valign);

    if (start)
        append_child(root, "Start", *start);

    if (vstart)
        append_child(root, "VStart", *vstart);

    if (!follows.empty())
        append_child(root, "Follows", follows);
    if (!vfollows.empty())
        append_child(root, "VFollows", vfollows);

    if (has_length)
        append_child(root, "Length", length);
    return root;
}
