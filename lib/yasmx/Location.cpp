//
// YASM location implementation.
//
//  Copyright (C) 2007  Peter Johnson
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
#include "yasmx/Location.h"

#include "llvm/ADT/Twine.h"
#include "llvm/Support/raw_ostream.h"
#include "YAML/emitter.h"
#include "yasmx/Bytecode.h"
#include "yasmx/IntNum.h"


namespace yasm
{

unsigned long
Location::getOffset() const
{
    return bc->getOffset() + off;
}

void
Location::Write(YAML::Emitter& out) const
{
    out << YAML::Flow << YAML::BeginMap;
    out << YAML::Key << "bc" << YAML::Value
        << YAML::Alias("BC@" + llvm::Twine::utohexstr((uint64_t)bc));
    out << YAML::Key << "off" << YAML::Value << off;
    out << YAML::EndMap;
}

void
Location::Dump() const
{
    YAML::Emitter out;
    Write(out);
    llvm::errs() << out.c_str() << '\n';
}

bool
CalcDist(Location loc1, Location loc2, IntNum* dist)
{
    assert(loc1.bc && loc2.bc);
    if (loc1.bc->getContainer() != loc2.bc->getContainer())
        return false;

    *dist = loc2.getOffset();
    *dist -= loc1.getOffset();
    return true;
}

bool
CalcDistNoBC(Location loc1, Location loc2, IntNum* dist)
{
    assert(loc1.bc && loc2.bc);
    if (loc1.bc != loc2.bc)
        return false;

    *dist = loc2.off;
    *dist -= loc1.off;
    return true;
}

} // namespace yasm
