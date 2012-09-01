#ifndef YASM_BINOBJECT_H
#define YASM_BINOBJECT_H
//
// Flat-format binary object format
//
//  Copyright (C) 2002-2007  Peter Johnson
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
#include "yasmx/Basic/SourceLocation.h"
#include "yasmx/Config/export.h"
#include "yasmx/Support/scoped_ptr.h"
#include "yasmx/ObjectFormat.h"

#include "BinLink.h"


namespace yasm
{
class DiagnosticsEngine;
class DirectiveInfo;
class Expr;
class IntNum;
class NameValue;

namespace objfmt
{

class YASM_STD_EXPORT BinObject : public ObjectFormat
{
public:
    /// Constructor.
    BinObject(const ObjectFormatModule& module, Object& object);

    /// Destructor.
    ~BinObject();

    void AddDirectives(Directives& dirs, StringRef parser);

    void Output(raw_fd_ostream& os,
                bool all_syms,
                DebugFormat& dbgfmt,
                DiagnosticsEngine& diags);

    Section* AddDefaultSection();
    Section* AppendSection(StringRef name,
                           SourceLocation source,
                           DiagnosticsEngine& diags);

    static StringRef getName() { return "Flat format binary"; }
    static StringRef getKeyword() { return "bin"; }
    static StringRef getExtension() { return ""; }
    static unsigned int getDefaultX86ModeBits() { return 16; }
    static StringRef getDefaultDebugFormatKeyword() { return "null"; }
    static std::vector<StringRef> getDebugFormatKeywords();
    static bool isOkObject(Object& object) { return true; }
    static bool Taste(const MemoryBuffer& in,
                      /*@out@*/ std::string* arch_keyword,
                      /*@out@*/ std::string* machine)
    { return false; }

private:
    void DirSection(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirOrg(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirMap(DirectiveInfo& info, DiagnosticsEngine& diags);
    bool setMapFilename(const NameValue& nv,
                        SourceLocation dir_source,
                        DiagnosticsEngine& diags);

    void OutputMap(const IntNum& origin,
                   const BinGroups& groups,
                   DiagnosticsEngine& diags) const;

    enum
    {
        NO_MAP = 0,
        MAP_NONE = 0x01,
        MAP_BRIEF = 0x02,
        MAP_SECTIONS = 0x04,
        MAP_SYMBOLS = 0x08
    };
    unsigned long m_map_flags;
    std::string m_map_filename;

    util::scoped_ptr<Expr> m_org;
    SourceLocation m_org_source;
};

}} // namespace yasm::objfmt

#endif
