#ifndef YASM_OPTIMIZER_H
#define YASM_OPTIMIZER_H
///
/// @file
/// @brief Optimizer interface.
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
#include "yasmx/Config/export.h"
#include "yasmx/Support/scoped_ptr.h"
#include "yasmx/DebugDumper.h"


namespace yasm
{

class Bytecode;
class DiagnosticsEngine;
class Value;

/// Optimizer.  Determines jump sizes, offset setters, all offsets.
class YASM_LIB_EXPORT Optimizer
{
public:
    Optimizer(DiagnosticsEngine& diags);
    ~Optimizer();
    void AddSpan(Bytecode& bc,
                 int id,
                 const Value& value,
                 long neg_thres,
                 long pos_thres);
    void AddOffsetSetter(Bytecode& bc);

    // Step1a: Set bytecode indexes, initial offsets, add spans and
    // offset setters using the above functions.

    void Step1b();

    // Step1c: update offsets

    // @return True if an error occurred.
    bool Step1d();

    void Step1e();
    void Step2();

    // Step 3: update offsets

#ifdef WITH_XML
    pugi::xml_node Write(pugi::xml_node out) const;
#endif // WITH_XML

    class Impl;

private:
    Optimizer(const Optimizer&);                    // not implemented
    const Optimizer& operator=(const Optimizer&);   // not implemented

    /// Pimpl.
    util::scoped_ptr<Impl> m_impl;
};

} // namespace yasm

#endif
