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

#include "yasmx/Bytecode.h"
#include "yasmx/IntNum.h"


namespace yasm
{

unsigned long
Location::get_offset() const
{
    return bc->get_offset() + off;
}

bool
calc_dist(Location loc1, Location loc2, IntNum* dist)
{
    assert(loc1.bc && loc2.bc);
    if (loc1.bc->get_container() != loc2.bc->get_container())
        return false;

    *dist = loc2.get_offset();
    *dist -= loc1.get_offset();
    return true;
}

bool
calc_dist_no_bc(Location loc1, Location loc2, IntNum* dist)
{
    assert(loc1.bc && loc2.bc);
    if (loc1.bc != loc2.bc)
        return false;

    *dist = loc2.off;
    *dist -= loc1.off;
    return true;
}

} // namespace yasm
