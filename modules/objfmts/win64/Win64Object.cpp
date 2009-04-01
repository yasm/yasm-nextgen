//
// Win64 object format
//
//  Copyright (C) 2002-2009  Peter Johnson
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
#include <util.h>

#include <yasmx/Support/Compose.h>
#include <yasmx/Support/errwarn.h>
#include <yasmx/Support/nocase.h>
#include <yasmx/Support/registry.h>
#include <yasmx/Arch.h>
#include <yasmx/BytecodeContainer_util.h>
#include <yasmx/Directive.h>
#include <yasmx/Errwarns.h>
#include <yasmx/Object.h>
#include <yasmx/NameValue.h>
#include <yasmx/Symbol.h>

#include "modules/objfmts/win32/Win32Object.h"
#include "modules/objfmts/coff/CoffSection.h"
#include "UnwindCode.h"
#include "UnwindInfo.h"

namespace yasm
{
namespace objfmt
{
namespace win64
{

using yasm::objfmt::coff::CoffSection;
using yasm::objfmt::coff::get_coff;

class Win64Object : public yasm::objfmt::win32::Win32Object
{
public:
    Win64Object();
    virtual ~Win64Object();

    virtual std::string get_name() const;
    virtual std::string get_keyword() const;
    virtual void add_directives(Directives& dirs, const std::string& parser);

    //virtual bool ok_object(Object* object) const;

    virtual unsigned int get_default_x86_mode_bits() const;

    //virtual void init_symbols(const std::string& parser);
    //virtual bool taste(std::istream& is,
    //                   /*@out@*/ std::string* arch_keyword,
    //                   /*@out@*/ std::string* machine);
    //virtual void read(std::istream& is);
    virtual void output(std::ostream& os, bool all_syms, Errwarns& errwarns);

private:
    virtual bool init_section(const std::string& name,
                              Section& section,
                              CoffSection* coffsect);

    void procframe_checkstate(const char* dirname);

    void dir_proc_frame(Object& object,
                        NameValues& namevals,
                        NameValues& objext_namevals,
                        unsigned long line);
    void dir_pushreg(Object& object,
                     NameValues& namevals,
                     NameValues& objext_namevals,
                     unsigned long line);
    void dir_setframe(Object& object,
                      NameValues& namevals,
                      NameValues& objext_namevals,
                      unsigned long line);
    void dir_allocstack(Object& object,
                        NameValues& namevals,
                        NameValues& objext_namevals,
                        unsigned long line);

    void save_common(Object& object,
                     NameValues& namevals,
                     unsigned long line,
                     const char* dirname,
                     UnwindCode::Opcode op);
    void dir_savereg(Object& object,
                     NameValues& namevals,
                     NameValues& objext_namevals,
                     unsigned long line);
    void dir_savexmm128(Object& object,
                        NameValues& namevals,
                        NameValues& objext_namevals,
                        unsigned long line);
    void dir_pushframe(Object& object,
                       NameValues& namevals,
                       NameValues& objext_namevals,
                       unsigned long line);
    void dir_endprolog(Object& object,
                       NameValues& namevals,
                       NameValues& objext_namevals,
                       unsigned long line);
    void dir_endproc_frame(Object& object,
                           NameValues& namevals,
                           NameValues& objext_namevals,
                           unsigned long line);

    // data for proc_frame and related directives
    unsigned long m_proc_frame;         // Line number of start of proc, or 0
    unsigned long m_done_prolog;        // Line number of end of prologue, or 0
    std::auto_ptr<UnwindInfo> m_unwind; // Unwind info
};

Win64Object::Win64Object()
    : m_proc_frame(0)
    , m_done_prolog(0)
    , m_unwind(0)
{
}

Win64Object::~Win64Object()
{
}

std::string
Win64Object::get_name() const
{
    return "Win64";
}

std::string
Win64Object::get_keyword() const
{
    return "win64";
}

unsigned int
Win64Object::get_default_x86_mode_bits() const
{
    return 64;
}

void
Win64Object::output(std::ostream& os, bool all_syms, Errwarns& errwarns)
{
    if (m_proc_frame != 0)
    {
        Error err(N_("end of file in procedure frame"));
        err.set_xref(m_proc_frame, N_("procedure started here"));
        errwarns.propagate(0, err);
        return;
    }

    // Force all syms for win64 because they're needed for relocations.
    // FIXME: Not *all* syms need to be output, only the ones needed for
    // relocation.  Find a way to do that someday.
    Win32Object::output(os, true, errwarns);
}

void
Win64Object::dir_proc_frame(Object& object,
                            NameValues& namevals,
                            NameValues& objext_namevals,
                            unsigned long line)
{
    if (!namevals.front().is_id())
        throw SyntaxError(N_("argument to [PROC_FRAME] must be symbol name"));
    std::string name = namevals.front().get_id();

    if (m_proc_frame != 0)
    {
        SyntaxError err(
            N_("nested procedures not supported (didn't use [ENDPROC_FRAME]?)"));
        err.set_xref(m_proc_frame, N_("previous procedure started here"));
        throw err;
    }
    m_proc_frame = line;
    m_done_prolog = 0;
    m_unwind.reset(new UnwindInfo);

    SymbolRef proc = object.get_symbol(name);
    proc->use(line);
    m_unwind->set_proc(proc);

    // Optional error handler
    if (namevals.size() > 1)
    {
        if (!namevals[1].is_id())
        {
            throw SyntaxError(
                N_("[PROC_FRAME] error handler must be symbol name"));
        }
        SymbolRef ehandler = object.get_symbol(namevals[1].get_id());
        ehandler->use(line);
        m_unwind->set_ehandler(ehandler);
    }
}

void
Win64Object::procframe_checkstate(const char* dirname)
{
    if (m_proc_frame == 0)
    {
        throw SyntaxError(String::compose(
            N_("[%1] without preceding [PROC_FRAME]"), dirname));
    }

    if (m_done_prolog != 0)
    {
        SyntaxError err(String::compose(
            N_("[%1] after end of prologue"), dirname));
        err.set_xref(m_done_prolog, N_("prologue ended here"));
        throw err;
    }
}

// Get current assembly position.
static SymbolRef
get_curpos(Object& object, const char* dirname, unsigned long line)
{
    Section* sect = object.get_cur_section();
    if (!sect)
    {
        throw SyntaxError(String::compose(
            N_("[%1] can only be used inside of a section"), dirname));
    }
    SymbolRef sym = object.add_non_table_symbol("$");
    Bytecode& bc = sect->fresh_bytecode();
    Location loc = {&bc, bc.get_fixed_len()};
    sym->define_label(loc, line);
    return sym;
}

void
Win64Object::dir_pushreg(Object& object,
                         NameValues& namevals,
                         NameValues& objext_namevals,
                         unsigned long line)
{
    assert(!namevals.empty());
    procframe_checkstate("PUSHREG");

    NameValue& nv = namevals.front();
    const Register* reg;
    if (!nv.is_expr() ||
        !(reg = nv.get_expr(object, line)->get_reg()))
    {
        throw SyntaxError(String::compose(
            N_("[%1] requires a register as the first parameter"), "PUSHREG"));
    }

    // Generate a PUSH_NONVOL unwind code.
    m_unwind->add_code(std::auto_ptr<UnwindCode>(new UnwindCode(
        m_unwind->get_proc(),
        get_curpos(object, "PUSHREG", line),
        UnwindCode::PUSH_NONVOL,
        reg->get_num() & 0xF)));
}

void
Win64Object::dir_setframe(Object& object,
                          NameValues& namevals,
                          NameValues& objext_namevals,
                          unsigned long line)
{
    assert(!namevals.empty());
    procframe_checkstate("SETFRAME");

    NameValue& nv = namevals.front();
    const Register* reg;
    if (!nv.is_expr() ||
        !(reg = nv.get_expr(object, line)->get_reg()))
    {
        throw SyntaxError(String::compose(
            N_("[%1] requires a register as the first parameter"), "SETFRAME"));
    }

    std::auto_ptr<Expr> off(0);
    if (namevals.size() > 1)
        off = namevals[1].release_expr(object, line);
    else
        off.reset(new Expr(0));

    // Set the frame fields in the unwind info
    m_unwind->set_framereg(reg->get_num());
    m_unwind->set_frameoff(Value(8, std::auto_ptr<Expr>(off->clone())));

    // Generate a SET_FPREG unwind code
    m_unwind->add_code(std::auto_ptr<UnwindCode>(new UnwindCode(
        m_unwind->get_proc(),
        get_curpos(object, "SETFRAME", line),
        UnwindCode::SET_FPREG,
        reg->get_num() & 0xF,
        8,
        off)));
}

void
Win64Object::dir_allocstack(Object& object,
                            NameValues& namevals,
                            NameValues& objext_namevals,
                            unsigned long line)
{
    assert(!namevals.empty());
    procframe_checkstate("ALLOCSTACK");

    NameValue& nv = namevals.front();
    if (!nv.is_expr())
    {
        throw SyntaxError(String::compose(
            N_("[%1] requires a size"), "ALLOCSTACK"));
    }

    // Generate an ALLOC_SMALL unwind code; this will get enlarged to an
    // ALLOC_LARGE if necessary.
    m_unwind->add_code(std::auto_ptr<UnwindCode>(new UnwindCode(
        m_unwind->get_proc(),
        get_curpos(object, "ALLOCSTACK", line),
        UnwindCode::ALLOC_SMALL,
        0,
        7,
        nv.release_expr(object, line))));
}

void
Win64Object::save_common(Object& object,
                         NameValues& namevals,
                         unsigned long line,
                         const char* dirname,
                         UnwindCode::Opcode op)
{
    assert(!namevals.empty());
    procframe_checkstate(dirname);

    NameValue& nv = namevals.front();
    const Register* reg;
    if (!nv.is_expr() ||
        !(reg = nv.get_expr(object, line)->get_reg()))
    {
        throw SyntaxError(String::compose(
            N_("[%1] requires a register as the first parameter"), dirname));
    }

    if (namevals.size() < 2 || !namevals[1].is_expr())
    {
        throw SyntaxError(String::compose(
            N_("[%1] requires an offset as the second parameter"), dirname));
    }

    // Generate a SAVE_XXX unwind code; this will get enlarged to a
    // SAVE_XXX_FAR if necessary.
    m_unwind->add_code(std::auto_ptr<UnwindCode>(new UnwindCode(
        m_unwind->get_proc(),
        get_curpos(*m_object, dirname, line),
        op,
        reg->get_num() & 0xF,
        16,
        namevals[1].release_expr(object, line))));
}

void
Win64Object::dir_savereg(Object& object,
                         NameValues& namevals,
                         NameValues& objext_namevals,
                         unsigned long line)
{
    save_common(object, namevals, line, "SAVEREG", UnwindCode::SAVE_NONVOL);
}

void
Win64Object::dir_savexmm128(Object& object,
                            NameValues& namevals,
                            NameValues& objext_namevals,
                            unsigned long line)
{
    save_common(object, namevals, line, "SAVEXMM128", UnwindCode::SAVE_XMM128);
}

void
Win64Object::dir_pushframe(Object& object,
                           NameValues& namevals,
                           NameValues& objext_namevals,
                           unsigned long line)
{
    procframe_checkstate("PUSHFRAME");

    // Generate a PUSH_MACHFRAME unwind code.  If there's any parameter,
    // we set info to 1.  Otherwise we set info to 0.
    m_unwind->add_code(std::auto_ptr<UnwindCode>(new UnwindCode(
        m_unwind->get_proc(),
        get_curpos(object, "PUSHFRAME", line),
        UnwindCode::PUSH_MACHFRAME,
        namevals.empty() ? 0 : 1)));
}

void
Win64Object::dir_endprolog(Object& object,
                           NameValues& namevals,
                           NameValues& objext_namevals,
                           unsigned long line)
{
    procframe_checkstate("ENDPROLOG");
    m_done_prolog = line;

    m_unwind->set_prolog(get_curpos(object, "ENDPROLOG", line));
}

void
Win64Object::dir_endproc_frame(Object& object,
                               NameValues& namevals,
                               NameValues& objext_namevals,
                               unsigned long line)
{
    if (m_proc_frame == 0)
    {
        throw SyntaxError(String::compose(
            N_("[%1] without preceding [PROC_FRAME]"), "ENDPROC_FRAME"));
    }
    if (m_done_prolog == 0)
    {
        SyntaxError err(N_("ended procedure without ending prologue"));
        err.set_xref(m_proc_frame, N_("procedure started here"));

        m_unwind.reset(0);
        m_proc_frame = 0;
        throw err;
    }
    assert(m_unwind.get() != 0 && "unwind info not present");

    SymbolRef proc_sym = m_unwind->get_proc();

    SymbolRef curpos = get_curpos(object, "ENDPROC_FRAME", line);

    //
    // Add unwind info to end of .xdata section.
    //

    Section* xdata = m_object->find_section(".xdata");

    // Create xdata section if needed.
    if (!xdata)
        xdata = append_section(".xdata", line);

    // Get current position in .xdata section.
    SymbolRef unwindpos = m_object->add_non_table_symbol("$");
    Location unwindpos_loc =
        {&xdata->bcs_last(), xdata->bcs_last().get_fixed_len()};
    unwindpos->define_label(unwindpos_loc, line);
    // Get symbol for .xdata as we'll want to reference it with WRT.
    SymbolRef xdata_sym = get_coff(*xdata)->m_sym;

    // Add unwind info.  Use line number of start of procedure.
    Arch& arch = *object.get_arch();
    generate(m_unwind, *xdata, m_proc_frame, arch);

    //
    // Add function lookup to end of .pdata section.
    //

    Section* pdata = m_object->find_section(".pdata");

    // Initialize pdata section if needed.
    if (!pdata)
        pdata = append_section(".pdata", line);

    // Add function structure to end of .pdata
    append_data(*pdata, std::auto_ptr<Expr>(new Expr(proc_sym)), 4, arch, line);
    append_data(*pdata, std::auto_ptr<Expr>(new Expr(WRT(curpos, proc_sym))),
                4, arch, line);
    append_data(*pdata,
                std::auto_ptr<Expr>(new Expr(WRT(unwindpos, xdata_sym))),
                4, arch, line);

    m_proc_frame = 0;
    m_done_prolog = 0;
}

void
Win64Object::add_directives(Directives& dirs, const std::string& parser)
{
    static const Directives::Init<Win64Object> gas_dirs[] =
    {
        {".export",     &Win64Object::dir_export,     Directives::ID_REQUIRED},
        {".proc_frame", &Win64Object::dir_proc_frame, Directives::ID_REQUIRED},
        {".pushreg",    &Win64Object::dir_pushreg,    Directives::ARG_REQUIRED},
        {".setframe",   &Win64Object::dir_setframe,   Directives::ARG_REQUIRED},
        {".allocstack", &Win64Object::dir_allocstack, Directives::ARG_REQUIRED},
        {".savereg",    &Win64Object::dir_savereg,    Directives::ARG_REQUIRED},
        {".savexmm128", &Win64Object::dir_savexmm128, Directives::ARG_REQUIRED},
        {".pushframe",  &Win64Object::dir_pushframe,  Directives::ANY},
        {".endprolog",  &Win64Object::dir_endprolog,  Directives::ANY},
        {".endproc_frame", &Win64Object::dir_endproc_frame, Directives::ANY},
    };
    static const Directives::Init<Win64Object> nasm_dirs[] =
    {
        {"export",     &Win64Object::dir_export,     Directives::ID_REQUIRED},
        {"proc_frame", &Win64Object::dir_proc_frame, Directives::ID_REQUIRED},
        {"pushreg",    &Win64Object::dir_pushreg,    Directives::ARG_REQUIRED},
        {"setframe",   &Win64Object::dir_setframe,   Directives::ARG_REQUIRED},
        {"allocstack", &Win64Object::dir_allocstack, Directives::ARG_REQUIRED},
        {"savereg",    &Win64Object::dir_savereg,    Directives::ARG_REQUIRED},
        {"savexmm128", &Win64Object::dir_savexmm128, Directives::ARG_REQUIRED},
        {"pushframe",  &Win64Object::dir_pushframe,  Directives::ANY},
        {"endprolog",  &Win64Object::dir_endprolog,  Directives::ANY},
        {"endproc_frame", &Win64Object::dir_endproc_frame, Directives::ANY},
    };

    if (String::nocase_equal(parser, "nasm"))
        dirs.add_array(this, nasm_dirs, NELEMS(nasm_dirs));
    else if (String::nocase_equal(parser, "gas"))
        dirs.add_array(this, gas_dirs, NELEMS(gas_dirs));

    // Pull in coff directives (but not win32 directives)
    CoffObject::add_directives(dirs, parser);
}

bool
Win64Object::init_section(const std::string& name,
                          Section& section,
                          CoffSection* coffsect)
{
    if (Win32Object::init_section(name, section, coffsect))
        return true;
    else if (name == ".pdata")
    {
        coffsect->m_flags = CoffSection::DATA | CoffSection::READ;
        section.set_align(4);
        coffsect->m_nobase = true;
    }
    else if (name == ".xdata")
    {
        coffsect->m_flags = CoffSection::DATA | CoffSection::READ;
        section.set_align(8);
    }
    else
    {
        // Default to code (NASM default; note GAS has different default.
        coffsect->m_flags =
            CoffSection::TEXT | CoffSection::EXECUTE | CoffSection::READ;
        section.set_code(true);
        return false;
    }
    return true;
}

#if 0
#include "win64-nasm.c"
#include "win64-gas.c"

static const yasm_stdmac win64_objfmt_stdmacs[] =
{
    { "nasm", "nasm", win64_nasm_stdmac },
    { "gas", "nasm", win64_gas_stdmac },
    { NULL, NULL, NULL }
};
#endif

void
do_register()
{
    register_module<ObjectFormat, Win64Object>("win64");
    register_module<ObjectFormat, Win64Object>("x64");
}

}}} // namespace yasm::objfmt::win64
