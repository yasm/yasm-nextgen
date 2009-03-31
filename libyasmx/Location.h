#ifndef YASM_LOCATION_H
#define YASM_LOCATION_H
///
/// @file
/// @brief Location interface.
///
/// @license
///  Copyright (C) 2007  Peter Johnson
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
#include "Config/export.h"


namespace yasm
{

class Bytecode;
class IntNum;

/// An assembly location.  The finest granularity a label can directly access,
/// and used throughout yasm to address a specific assembly location.
/// This structure is passed around by value, etc, so it must remain small!
struct YASM_LIB_EXPORT Location
{
    Bytecode* bc;
    unsigned long off;

    /// Get real offset (bc offset + off)
    /// @return Offset.
    unsigned long get_offset() const;
};

inline bool
operator==(const Location& lhs, const Location& rhs)
{
    return (lhs.bc == rhs.bc && lhs.off == rhs.off);
}

/// Determine the distance between the starting offsets of two locations.
/// @param loc1         location 1
/// @param loc2         location 2
/// @param dist         distance in bytes between the two locations
///                     (loc2-loc1); output.
/// @return True if distance calculated; false if the distance was
///         indeterminate.
/// @warning Only valid /after/ optimization.
YASM_LIB_EXPORT
bool calc_dist(Location loc1, Location loc2, /*@out@*/ IntNum* dist);

/// Determine the distance between the starting offsets of two locations.
/// Unlike calc_dist(), this only calculates the distance if both locations
/// are within the same bytecode, so it's safe to call prior to optimization
/// being performed.
/// @param loc1         location 1
/// @param loc2         location 2
/// @param dist         distance in bytes between the two locations
///                     (loc2-loc1); output.
/// @return True if distance calculated; false if the distance was
///         indeterminate (e.g. the locations have different bytecodes).
YASM_LIB_EXPORT
bool calc_dist_no_bc(Location loc1, Location loc2, IntNum* dist);

} // namespace yasm

#endif
