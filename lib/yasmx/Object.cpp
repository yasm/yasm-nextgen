//
// Object implementation.
//
//  Copyright (C) 2001-2007  Peter Johnson
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
#define DEBUG_TYPE "Object"

#include "yasmx/Object.h"

#include <algorithm>
#include <memory>

#include <boost/pool/pool.hpp>

#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/raw_ostream.h"
#include "yasmx/Basic/Diagnostic.h"
#include "yasmx/Config/functional.h"
#include "yasmx/Arch.h"
#include "yasmx/Section.h"
#include "yasmx/Symbol.h"

#include "hamt.h"


STATISTIC(num_exist_symbol, "Number of existing symbols found by name");
STATISTIC(num_new_symbol, "Number of symbols created by name");

using namespace yasm;

namespace {
/// Get name helper for symbol table HAMT.
class SymGetName
{
public:
    llvm::StringRef operator() (const Symbol* sym) const
    { return sym->getName(); }
};
} // anonymous namespace

namespace yasm {
class Object::Impl
{
public:
    Impl(bool nocase)
        : sym_map(nocase)
        , special_sym_map(true)
        , m_sym_pool(sizeof(Symbol))
    {}
    ~Impl() {}

    Symbol* NewSymbol(llvm::StringRef name)
    {
        Symbol* sym = static_cast<Symbol*>(m_sym_pool.malloc());
        new (sym) Symbol(name);
        return sym;
    }

    void DeleteSymbol(Symbol* sym)
    {
        if (sym)
        {
            sym->~Symbol();
            m_sym_pool.free(sym);
        }
    }

    typedef hamt<llvm::StringRef, Symbol, SymGetName> SymbolTable;

    /// Symbol table symbols, indexed by name.
    SymbolTable sym_map;

    /// Special symbols, indexed by name.
    SymbolTable special_sym_map;

private:
    /// Pool for symbols not in the symbol table.
    boost::pool<> m_sym_pool;
};
} // namespace yasm

Object::Object(llvm::StringRef src_filename,
               llvm::StringRef obj_filename,
               Arch* arch)
    : m_src_filename(src_filename),
      m_arch(arch),
      m_cur_section(0),
      m_sections_owner(m_sections),
      m_symbols_owner(m_symbols),
      m_impl(new Impl(false))
{
    m_options.DisableGlobalSubRelative = false;
}

void
Object::setSourceFilename(llvm::StringRef src_filename)
{
    m_src_filename = src_filename;
}

void
Object::setObjectFilename(llvm::StringRef obj_filename)
{
    m_obj_filename = obj_filename;
}

Object::~Object()
{
}

void
Object::Finalize(Diagnostic& diags)
{
    std::for_each(m_sections.begin(), m_sections.end(),
                  BIND::bind(&Section::Finalize, _1, REF::ref(diags)));
}

void
Object::AppendSection(std::auto_ptr<Section> sect)
{
    sect->m_object = this;
    m_sections.push_back(sect.release());
}

Section*
Object::FindSection(llvm::StringRef name)
{
    section_iterator i =
        std::find_if(m_sections.begin(), m_sections.end(),
                     BIND::bind(&Section::isName, _1, REF::ref(name)));
    if (i == m_sections.end())
        return 0;
    return &(*i);
}

SymbolRef
Object::getAbsoluteSymbol()
{
    SymbolRef sym = getSymbol("");

    // If we already defined it, we're done.
    if (sym->isDefined())
        return sym;

    // Define it
    sym->DefineEqu(Expr(0));
    sym->Use(SourceLocation());
    return sym;
}

SymbolRef
Object::FindSymbol(llvm::StringRef name)
{
    return SymbolRef(m_impl->sym_map.Find(name));
}

SymbolRef
Object::getSymbol(llvm::StringRef name)
{
    // Don't use pool allocator for symbols in the symbol table.
    // We have to maintain an ordered link list of all symbols in the symbol
    // table, so it's easy enough to reuse that for deleting the symbols.
    // The memory impact of keeping a second linked list (internal to the pool)
    // seems to outweigh the moderate time savings of pool deletion.
    std::auto_ptr<Symbol> sym(new Symbol(name));
    Symbol* sym2 = m_impl->sym_map.Insert(sym.get());
    if (sym2)
    {
        ++num_exist_symbol;
        return SymbolRef(sym2);
    }

    ++num_new_symbol;
    sym2 = sym.get();
    m_symbols.push_back(sym.release());
    return SymbolRef(sym2);
}

SymbolRef
Object::AppendSymbol(llvm::StringRef name)
{
    Symbol* sym = new Symbol(name);
    m_symbols.push_back(sym);
    return SymbolRef(sym);
}

SymbolRef
Object::AddNonTableSymbol(llvm::StringRef name)
{
    Symbol* sym = m_impl->NewSymbol(name);
    return SymbolRef(sym);
}

void
Object::RenameSymbol(SymbolRef sym, llvm::StringRef name)
{
    m_impl->sym_map.Remove(sym->getName());
    sym->m_name = name;
    m_impl->sym_map.Insert(sym);
}

void
Object::ExternUndefinedSymbols()
{
    for (symbol_iterator i=m_symbols.begin(), end=m_symbols.end();
         i != end; ++i)
    {
        i->ExternUndefined();
    }
}

void
Object::FinalizeSymbols(Diagnostic& diags)
{
    bool firstundef = true;

    for (symbol_iterator i=m_symbols.begin(), end=m_symbols.end();
         i != end; ++i)
    {
        if (i->isUndefined())
        {
            // error if a symbol is used but not defined or extern/common
            diags.Report(i->getUseSource(), diag::err_symbol_undefined)
                << i->getName();
            if (firstundef)
            {
                diags.Report(i->getUseSource(),
                             diag::note_symbol_undefined_once);
                firstundef = false;
            }
        }
    }
}

SymbolRef
Object::AddSpecialSymbol(llvm::StringRef name)
{
    Symbol* sym = m_impl->NewSymbol(name);
    m_impl->special_sym_map.Insert(sym);
    return SymbolRef(sym);
}

SymbolRef
Object::FindSpecialSymbol(llvm::StringRef name)
{
    return SymbolRef(m_impl->special_sym_map.Find(name));
}
