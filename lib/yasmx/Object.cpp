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

#include "util.h"

#include <algorithm>
#include <memory>

#include <boost/pool/pool.hpp>

#include "llvm/ADT/Statistic.h"
#include "yasmx/Config/functional.h"
#include "yasmx/Support/errwarn.h"
#include "yasmx/Support/marg_ostream.h"
#include "yasmx/Errwarns.h"
#include "yasmx/Section.h"
#include "yasmx/Symbol.h"

#include "hamt.h"


STATISTIC(num_exist_symbol, "Number of existing symbols found by name");
STATISTIC(num_new_symbol, "Number of symbols created by name");

namespace
{

/// Get name helper for symbol table HAMT.
class SymGetName
{
public:
    const std::string& operator() (const yasm::Symbol* sym) const
    { return sym->get_name(); }
};

} // anonymous namespace

namespace yasm
{

class Object::Impl
{
public:
    Impl(bool nocase)
        : sym_map(nocase)
        , special_sym_map(false)
        , m_sym_pool(sizeof(Symbol))
    {}
    ~Impl() {}

    Symbol* new_symbol(const std::string& name)
    {
        Symbol* sym = static_cast<Symbol*>(m_sym_pool.malloc());
        new (sym) Symbol(name);
        return sym;
    }

    void delete_symbol(Symbol* sym)
    {
        if (sym)
        {
            sym->~Symbol();
            m_sym_pool.free(sym);
        }
    }

    typedef hamt<std::string, Symbol, SymGetName> SymbolTable;

    /// Symbol table symbols, indexed by name.
    SymbolTable sym_map;

    /// Special symbols, indexed by name.
    SymbolTable special_sym_map;

private:
    /// Pool for symbols not in the symbol table.
    boost::pool<> m_sym_pool;
};

Object::Object(const std::string& src_filename,
               const std::string& obj_filename,
               Arch* arch)
    : m_src_filename(src_filename),
      m_arch(arch),
      m_cur_section(0),
      m_sections_owner(m_sections),
      m_symbols_owner(m_symbols),
      m_impl(new Impl(false))
{
}

void
Object::set_source_fn(const std::string& src_filename)
{
    m_src_filename = src_filename;
}

void
Object::set_object_fn(const std::string& obj_filename)
{
    m_obj_filename = obj_filename;
}

Object::~Object()
{
}

marg_ostream&
operator<< (marg_ostream& os, const Object& object)
{
    // Print symbol table
    os << "Symbol Table:\n";
    for (Object::const_symbol_iterator i=object.m_symbols.begin(),
         end=object.m_symbols.end(); i != end; ++i)
    {
        os << "Symbol `" << i->get_name() << "'\n";
        ++os;
        os << *i;
        --os;
    }

    // Print sections and bytecodes
    for (Object::const_section_iterator i=object.m_sections.begin(),
         end=object.m_sections.end(); i != end; ++i)
    {
        os << "Section:\n";
        os << *i;
    }
    return os;
}

void
Object::finalize(Errwarns& errwarns)
{
    std::for_each(m_sections.begin(), m_sections.end(),
                  BIND::bind(&Section::finalize, _1, REF::ref(errwarns)));
}

void
Object::append_section(std::auto_ptr<Section> sect)
{
    sect->m_object = this;
    m_sections.push_back(sect.release());
}

Section*
Object::find_section(const std::string& name)
{
    section_iterator i =
        std::find_if(m_sections.begin(), m_sections.end(),
                     BIND::bind(&Section::is_name, _1, REF::ref(name)));
    if (i == m_sections.end())
        return 0;
    return &(*i);
}

SymbolRef
Object::get_absolute_symbol()
{
    SymbolRef sym = get_symbol("");

    // If we already defined it, we're done.
    if (sym->get_status() & Symbol::DEFINED)
        return sym;

    // Define it
    sym->define_equ(Expr(0), 0);
    sym->use(0);
    return sym;
}

SymbolRef
Object::find_symbol(const std::string& name)
{
    return SymbolRef(m_impl->sym_map.find(name));
}

SymbolRef
Object::get_symbol(const std::string& name)
{
    // Don't use pool allocator for symbols in the symbol table.
    // We have to maintain an ordered link list of all symbols in the symbol
    // table, so it's easy enough to reuse that for deleting the symbols.
    // The memory impact of keeping a second linked list (internal to the pool)
    // seems to outweigh the moderate time savings of pool deletion.
    std::auto_ptr<Symbol> sym(new Symbol(name));
    Symbol* sym2 = m_impl->sym_map.insert(sym.get());
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
Object::append_symbol(const std::string& name)
{
    Symbol* sym = new Symbol(name);
    m_symbols.push_back(sym);
    return SymbolRef(sym);
}

SymbolRef
Object::add_non_table_symbol(const std::string& name)
{
    Symbol* sym = m_impl->new_symbol(name);
    return SymbolRef(sym);
}

void
Object::symbols_finalize(Errwarns& errwarns, bool undef_extern)
{
    unsigned long firstundef_line = ULONG_MAX;

    for (symbol_iterator i=m_symbols.begin(), end=m_symbols.end();
         i != end; ++i)
    {
        try
        {
            i->finalize(undef_extern);
        }
        catch (Error& err)
        {
            unsigned long use_line = i->get_use_line();
            errwarns.propagate(use_line, err);
            if (use_line < firstundef_line)
                firstundef_line = use_line;
        }
    }
    if (firstundef_line < ULONG_MAX)
        errwarns.propagate(firstundef_line,
            Error(N_(" (Each undefined symbol is reported only once.)")));
}

SymbolRef
Object::add_special_symbol(const std::string& name)
{
    Symbol* sym = m_impl->new_symbol(name);
    m_impl->special_sym_map.insert(sym);
    return SymbolRef(sym);
}

SymbolRef
Object::find_special_symbol(const std::string& name)
{
    return SymbolRef(m_impl->special_sym_map.find(name));
}

} // namespace yasm
