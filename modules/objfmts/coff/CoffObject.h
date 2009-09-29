#ifndef YASM_COFFOBJECT_H
#define YASM_COFFOBJECT_H
//
// COFF (DJGPP) object format
//
//  Copyright (C) 2007-2008  Peter Johnson
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
#include "yasmx/ObjectFormat.h"

namespace yasm
{

class DirHelpers;
class IntNum;
class NameValues;

namespace objfmt
{
namespace coff
{

struct CoffSection;
struct CoffSymbol;

class CoffObject : public ObjectFormat
{
public:
    enum Machine
    {
        MACHINE_UNKNOWN = 0,
        MACHINE_I386  = 0x014C,
        MACHINE_AMD64 = 0x8664
    };

    CoffObject(const ObjectFormatModule& module,
               Object& object,
               bool set_vma = true,
               bool win32 = false,
               bool win64 = false);
    virtual ~CoffObject();

    virtual void AddDirectives(Directives& dirs, const llvm::StringRef& parser);

    virtual void InitSymbols(const llvm::StringRef& parser);
#if 0
    virtual void read(std::istream& is);
#endif
    virtual void Output(llvm::raw_fd_ostream& os,
                        bool all_syms,
                        Errwarns& errwarns);

    virtual Section* AddDefaultSection();
    virtual Section* AppendSection(const llvm::StringRef& name,
                                   unsigned long line);

    Machine getMachine() const { return m_machine; }

    bool isWin32() const { return m_win32; }
    bool isWin64() const { return m_win64; }

    static const char* getName() { return "COFF (DJGPP)"; }
    static const char* getKeyword() { return "coff"; }
    static const char* getExtension() { return ".o"; }
    static unsigned int getDefaultX86ModeBits() { return 32; }
    static const char* getDefaultDebugFormatKeyword() { return "null"; }
    static std::vector<const char*> getDebugFormatKeywords();
    static bool isOkObject(Object& object);
    static bool Taste(const llvm::MemoryBuffer& in,
                      /*@out@*/ std::string* arch_keyword,
                      /*@out@*/ std::string* machine)
    { return false; }

protected:
    /// Initialize section (and COFF data) based on section name.
    /// @return True if section name recognized, false otherwise.
    virtual bool InitSection(const llvm::StringRef& name,
                             Section& section,
                             CoffSection* coffsect);
    virtual void DirSectionInitHelpers(DirHelpers& helpers,
                                       CoffSection* csd,
                                       IntNum* align,
                                       bool* has_align,
                                       unsigned long line);

private:
    // When this is false, all section VMA's are set to 0 rather than as the
    // same as the LMA.  According to the DJGPP COFF Spec, this should be true
    // (VMA=LMA), and indeed DJGPP's GCC output shows VMA=LMA.  However, NASM
    // outputs VMA=0 (as if this was false), and GNU objdump output looks a lot
    // nicer with VMA=0.  For Win32 PE/COFF, this should be false.
    bool m_set_vma;

    bool m_win32;               // win32 or win64 output?
    bool m_win64;               // win64 output?

    enum Flags
    {
        F_LNNO   = 0x0004,     ///< line number info NOT present
        F_LSYMS  = 0x0008,     ///< local symbols NOT present
        F_AR32WR = 0x0100      ///< 32-bit little endian file
    };

    Machine m_machine;              // COFF machine to use

    CoffSymbol* m_file_coffsym;     // Data for .file symbol

    void DirGasSection(Object& object,
                       NameValues& namevals,
                       NameValues& objext_namevals,
                       unsigned long line);
    void DirSection(Object& object,
                    NameValues& namevals,
                    NameValues& objext_namevals,
                    unsigned long line);
    void DirIdent(Object& object,
                  NameValues& namevals,
                  NameValues& objext_namevals,
                  unsigned long line);
};

}}} // namespace yasm::objfmt::coff

#endif
