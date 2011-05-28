#ifndef YASM_DEBUGDUMP_H
#define YASM_DEBUGDUMP_H
///
/// @file
/// @brief Debug dump interface.
///
/// @license
///  Copyright (C) 2011  Peter Johnson
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
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"
#include "YAML/emitter.h"
#include "yasmx/Config/export.h"


namespace yasm
{

template <typename T>
class YASM_LIB_EXPORT DebugDumper
{
public:
    /// Dump a YAML representation to stderr.
    /// For debugging purposes.
    void Dump() const;
};

template <typename T>
void
DebugDumper<T>::Dump() const
{
    YAML::Emitter out;
    static_cast<const T*>(this)->Write(out);
    llvm::errs() << out.c_str() << '\n';
}

/// Dump a YAML representation.  For debugging purposes.
/// @param out          YAML emitter
/// @param dumper       dumper
/// @return Emitter.
template <typename T>
inline YAML::Emitter&
operator<< (YAML::Emitter& out, const DebugDumper<T>& dumper)
{
    dumper.Write(out);
    return out;
}

} // yasm namespace

#endif
