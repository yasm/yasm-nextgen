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

#include "YAML/emitter.h"


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

void
BinSection::Write(YAML::Emitter& out) const
{
    out << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << key;
    if (has_align)
        out << YAML::Key << "align" << YAML::Value << align;
    if (has_valign)
        out << YAML::Key << "valign" << YAML::Value << valign;

    out << YAML::Key << "start" << YAML::Value;
    if (start)
        out << *start;
    else
        out << YAML::Null;

    out << YAML::Key << "vstart" << YAML::Value;
    if (vstart)
        out << *vstart;
    else
        out << YAML::Null;

    out << YAML::Key << "follows" << YAML::Value << follows;
    out << YAML::Key << "vfollows" << YAML::Value << vfollows;

    if (has_length)
        out << YAML::Key << "length" << YAML::Value << length;
    out << YAML::EndMap;
}
