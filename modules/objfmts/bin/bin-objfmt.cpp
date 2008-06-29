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
#include <util.h>

#include <fstream>
#include <iostream>

#include <libyasmx/bytecode.h>
#include <libyasmx/bytes.h>
#include <libyasmx/compose.h>
#include <libyasmx/directive.h>
#include <libyasmx/errwarn.h>
#include <libyasmx/errwarns.h>
#include <libyasmx/expr.h>
#include <libyasmx/intnum.h>
#include <libyasmx/nocase.h>
#include <libyasmx/object.h>
#include <libyasmx/object_format.h>
#include <libyasmx/name_value.h>
#include <libyasmx/registry.h>
#include <libyasmx/section.h>
#include <libyasmx/symbol.h>
#include <libyasmx/value.h>

#include "bin-data.h"
#include "bin-link.h"
#include "bin-map.h"


namespace yasm
{
namespace objfmt
{
namespace bin
{

class BinObject : public ObjectFormat
{
public:
    /// Constructor.
    /// To make object format truly usable, set_object()
    /// needs to be called.
    BinObject();

    /// Destructor.
    ~BinObject();

    std::string get_name() const { return "Flat format binary"; }
    std::string get_keyword() const { return "bin"; }
    void add_directives(Directives& dirs, const std::string& parser);

    std::string get_extension() const { return ""; }
    unsigned int get_default_x86_mode_bits() const { return 16; }

    std::vector<std::string> get_dbgfmt_keywords() const;
    std::string get_default_dbgfmt_keyword() const { return "null"; }

    void output(std::ostream& os, bool all_syms, Errwarns& errwarns);

    Section* add_default_section();
#if 0
    Section* section_switch(yasm_valparamhead *valparams,
         /*@null@*/ yasm_valparamhead *objext_valparams,
         unsigned long line) = 0;
#endif
private:
    void define_section_symbol(Object& object,
                               const BinSectionData& bsd,
                               const std::string& sectname,
                               const char* suffix,
                               BinSymbolData::SpecialSym which,
                               unsigned long line);
    void init_new_section(Section* sect, unsigned long line);
    void dir_org(Object& object,
                 const NameValues& namevals,
                 const NameValues& objext_namevals,
                 unsigned long line);
    void dir_map(Object& object,
                 const NameValues& namevals,
                 const NameValues& objext_namevals,
                 unsigned long line);
    bool map_filename(const NameValue& nv);

    void output_map(const IntNum& origin,
                    const BinGroups& groups,
                    Errwarns& errwarns) const;

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

    boost::scoped_ptr<Expr> m_org;
};

BinObject::BinObject()
    : m_map_flags(NO_MAP), m_org(0)
{
}

BinObject::~BinObject()
{
}

void
BinObject::define_section_symbol(Object& object, const BinSectionData& bsd,
                                 const std::string& sectname,
                                 const char* suffix,
                                 BinSymbolData::SpecialSym which,
                                 unsigned long line)
{
    Symbol& sym = object.get_sym("section."+sectname+suffix);
    sym.declare(Symbol::EXTERN, line);
    std::auto_ptr<AssocData> ad(new BinSymbolData(bsd, which));
    sym.add_assoc_data(this, ad);
}


void
BinObject::output_map(const IntNum& origin,
                      const BinGroups& groups,
                      Errwarns& errwarns) const
{
    int map_flags = m_map_flags;

    if (map_flags == NO_MAP)
        return;

    if (map_flags == MAP_NONE)
        map_flags = MAP_BRIEF;          // default to brief

    std::ofstream os;
    if (m_map_filename.empty())
        os.std::basic_ios<char>::rdbuf(std::cout.rdbuf()); // stdout
    else
    {
        os.open(m_map_filename.c_str());
        if (os.fail())
        {
            warn_set(WARN_GENERAL, String::compose(
                N_("unable to open map file `%1'"), m_map_filename));
            errwarns.propagate(0);
            return;
        }
    }

    MapOutput out(os, *m_object, origin, groups, this);
    out.output_header();
    out.output_origin();

    if (map_flags & MAP_BRIEF)
        out.output_sections_summary();

    if (map_flags & MAP_SECTIONS)
        out.output_sections_detail();

    if (map_flags & MAP_SYMBOLS)
        out.output_sections_symbols();
}

class NoOutput : public BytecodeOutput
{
public:
    NoOutput() {}
    ~NoOutput() {}

    using BytecodeOutput::output;
    void output(Value& value, Bytes& bytes, Location loc, int warn);
    void output_gap(unsigned int size);
    void output(const Bytes& bytes);
};

void
NoOutput::output(Value& value, Bytes& bytes, Location loc, int warn)
{
    // unnecessary; we don't actually output it anyway
}

void
NoOutput::output_gap(unsigned int size)
{
    // expected
}

void
NoOutput::output(const Bytes& bytes)
{
    warn_set(WARN_GENERAL,
        N_("initialized space declared in nobits section: ignoring"));
}

class Output : public BytecodeOutput
{
public:
    Output(std::ostream& os, Object& object, const void* assoc_key);
    ~Output();

    void output_section(Section& sect,
                        const IntNum& origin,
                        Errwarns& errwarns);

    // OutputBytecode overrides
    using BytecodeOutput::output;
    void output(Value& value, Bytes& bytes, Location loc, int warn);
    void output_gap(unsigned int size);
    void output(const Bytes& bytes);

private:
    const void* m_assoc_key;
    Object& m_object;
    std::ostream& m_os;
    NoOutput m_no_output;
};

Output::Output(std::ostream& os, Object& object, const void* assoc_key)
    : m_assoc_key(assoc_key),
      m_object(object),
      m_os(os)
{
}

Output::~Output()
{
}

void
Output::output_section(Section& sect, const IntNum& origin, Errwarns& errwarns)
{
    BytecodeOutput* outputter;

    BinSectionData* bsd =
        static_cast<BinSectionData*>(sect.get_assoc_data(m_assoc_key));
    assert(bsd);

    if (bsd->bss)
    {
        outputter = &m_no_output;
    }
    else
    {
        IntNum file_start = bsd->istart;
        file_start -= origin;
        if (file_start.sign() < 0)
        {
            errwarns.propagate(0, ValueError(String::compose(
                N_("section `%1' starts before origin (ORG)"),
                sect.get_name())));
            return;
        }
        if (!file_start.ok_size(sizeof(unsigned long)*8, 0, 0))
        {
            errwarns.propagate(0, ValueError(String::compose(
                N_("section `%1' start value too large"),
                sect.get_name())));
            return;
        }
        m_os.seekp(file_start.get_uint());
        if (!m_os.good())
            throw Fatal(N_("could not seek on output file"));

        outputter = this;
    }

    for (Section::bc_iterator i=sect.bcs_begin(), end=sect.bcs_end();
         i != end; ++i)
    {
        try
        {
            i->output(*outputter);
        }
        catch (Error& err)
        {
            errwarns.propagate(i->get_line(), err);
        }
        errwarns.propagate(i->get_line());  // propagate warnings
    }
}

void
Output::output(Value& value, Bytes& bytes, Location loc, int warn)
{
    // Binary objects we need to resolve against object, not against section.
    if (value.is_relative())
    {
        Location label_loc;
        unsigned int rshift = (unsigned int)value.m_rshift;
        Expr::Ptr syme(0);
        unsigned long line = loc.bc->get_line();

        if (value.m_rel->is_abs())
        {
            syme.reset(new Expr(new IntNum(0), line));
        }
        else if (value.m_rel->get_label(&label_loc)
                 && label_loc.bc->get_container())
        {
            syme.reset(new Expr(value.m_rel, line));
        }
        else if (const IntNum* ssymval =
                 get_ssym_value(value.m_rel, m_assoc_key))
        {
            syme.reset(new Expr(*ssymval, line));
        }
        else
            goto done;

        // Handle PC-relative
        if (value.m_curpos_rel)
        {
            syme.reset(new Expr(syme, Op::SUB, loc, line));
            value.m_curpos_rel = 0;
            value.m_ip_rel = 0;
        }

        if (value.m_rshift > 0)
            syme.reset(new Expr(syme, Op::SHR, new IntNum(rshift), line));

        // Add into absolute portion
        value.add_abs(syme);
        value.m_rel = 0;
        value.m_rshift = 0;
    }
done:
    // Simplify absolute portion of value, transforming symrecs
    if (Expr* abs = value.get_abs())
        abs->level_tree(true, true, true,
                        BIND::bind(&expr_xform, _1, m_assoc_key));

    // Output
    Arch* arch = m_object.get_arch();
    if (value.output_basic(bytes, loc, warn, *arch))
    {
        m_os << bytes;
        return;
    }

    // Couldn't output, assume it contains an external reference.
    throw Error(
        N_("binary object format does not support external references"));
}

void
Output::output_gap(unsigned int size)
{
    // Warn that gaps are converted to 0 and write out the 0's.
    static const unsigned long BLOCK_SIZE = 4096;

    warn_set(WARN_UNINIT_CONTENTS,
        N_("uninitialized space declared in code/data section: zeroing"));
    // Write out in chunks
    Bytes& bytes = get_scratch();
    bytes.resize(BLOCK_SIZE);
    while (size > BLOCK_SIZE)
    {
        m_os << bytes;
        size -= BLOCK_SIZE;
    }
    bytes.resize(size);
    m_os << bytes;
}

void
Output::output(const Bytes& bytes)
{
    // Output bytes to file
    m_os << bytes;
}

static void
check_sym(const Symbol& sym, const void* assoc_key, Errwarns& errwarns)
{
    int vis = sym.get_visibility();

    // Don't check internally-generated symbols.  Only internally generated
    // symbols have symrec data, so simply check for its presence.
    if (sym.get_assoc_data(assoc_key))
        return;

    if (vis & Symbol::EXTERN)
    {
        warn_set(WARN_GENERAL,
            N_("binary object format does not support extern variables"));
        errwarns.propagate(sym.get_decl_line());
    }
    else if (vis & Symbol::GLOBAL)
    {
        warn_set(WARN_GENERAL,
            N_("binary object format does not support global variables"));
        errwarns.propagate(sym.get_decl_line());
    }
    else if (vis & Symbol::COMMON)
    {
        errwarns.propagate(sym.get_decl_line(), TypeError(
            N_("binary object format does not support common variables")));
    }
}

void
BinObject::output(std::ostream& os, bool all_syms, Errwarns& errwarns)
{
    // Set ORG to 0 unless otherwise specified
    IntNum origin(0);
    if (m_org.get() != 0)
    {
        m_org->simplify();
        const IntNum* orgi = m_org->get_intnum();
        if (!orgi)
        {
            errwarns.propagate(m_org->get_line(),
                TooComplexError(N_("ORG expression is too complex")));
            return;
        }
        if (orgi->sign() < 0)
        {
            errwarns.propagate(m_org->get_line(),
                ValueError(N_("ORG expression is negative")));
            return;
        }
        origin = *orgi;
    }

    // Check symbol table
    for (Object::const_symbol_iterator i=m_object->symbols_begin(),
         end=m_object->symbols_end(); i != end; ++i)
        check_sym(*i, this, errwarns);

    Link link(*m_object, this, errwarns);

    if (!link.do_link(origin))
        return;

    // Output map file
    output_map(origin, link.get_lma_groups(), errwarns);

    // Ensure we don't have overlapping progbits LMAs.
    if (!link.check_lma_overlap())
        return;

    // Output sections
    Output out(os, *m_object, this);
    for (Object::section_iterator i=m_object->sections_begin(),
         end=m_object->sections_end(); i != end; ++i)
    {
        out.output_section(*i, origin, errwarns);
    }
}

void
BinObject::init_new_section(Section* sect, unsigned long line)
{
    std::auto_ptr<BinSectionData> bsd(new BinSectionData());

    m_object->get_sym("section."+sect->get_name()+".start")
        .declare(Symbol::EXTERN, line)
        .add_assoc_data(this, std::auto_ptr<AssocData>
                        (new BinSymbolData(*bsd, BinSymbolData::START)));
    m_object->get_sym("section."+sect->get_name()+".vstart")
        .declare(Symbol::EXTERN, line)
        .add_assoc_data(this, std::auto_ptr<AssocData>
                        (new BinSymbolData(*bsd, BinSymbolData::VSTART)));
    m_object->get_sym("section."+sect->get_name()+".length")
        .declare(Symbol::EXTERN, line)
        .add_assoc_data(this, std::auto_ptr<AssocData>
                        (new BinSymbolData(*bsd, BinSymbolData::LENGTH)));

    sect->add_assoc_data(this, std::auto_ptr<AssocData>(bsd.release()));
}

Section*
BinObject::add_default_section()
{
    Section* section = new Section(".text", 16, true, false, 0);
    m_object->append_section(std::auto_ptr<Section>(section));
    init_new_section(section, 0);
    return section;
}
#if 0
static /*@observer@*/ /*@null@*/ yasm_section *
bin_objfmt_section_switch(yasm_object *object, yasm_valparamhead *valparams,
                          /*@unused@*/ /*@null@*/
                          yasm_valparamhead *objext_valparams,
                          unsigned long line)
{
    yasm_valparam *vp;
    yasm_section *retval;
    int isnew;
    int flags_override = 0;
    const char *sectname;
    bin_section_data *bsd = NULL;

    struct bin_section_switch_data {
        /*@only@*/ /*@null@*/ char *follows;
        /*@only@*/ /*@null@*/ char *vfollows;
        /*@only@*/ /*@null@*/ yasm_expr *start;
        /*@only@*/ /*@null@*/ yasm_expr *vstart;
        /*@only@*/ /*@null@*/ yasm_intnum *align;
        /*@only@*/ /*@null@*/ yasm_intnum *valign;
        unsigned long bss;
        unsigned long code;
    } data;

    static const yasm_dir_help help[] = {
        { "follows", 1, yasm_dir_helper_string,
          offsetof(struct bin_section_switch_data, follows), 0 },
        { "vfollows", 1, yasm_dir_helper_string,
          offsetof(struct bin_section_switch_data, vfollows), 0 },
        { "start", 1, yasm_dir_helper_expr,
          offsetof(struct bin_section_switch_data, start), 0 },
        { "vstart", 1, yasm_dir_helper_expr,
          offsetof(struct bin_section_switch_data, vstart), 0 },
        { "align", 1, yasm_dir_helper_intn,
          offsetof(struct bin_section_switch_data, align), 0 },
        { "valign", 1, yasm_dir_helper_intn,
          offsetof(struct bin_section_switch_data, valign), 0 },
        { "nobits", 0, yasm_dir_helper_flag_set,
          offsetof(struct bin_section_switch_data, bss), 1 },
        { "progbits", 0, yasm_dir_helper_flag_set,
          offsetof(struct bin_section_switch_data, bss), 0 },
        { "code", 0, yasm_dir_helper_flag_set,
          offsetof(struct bin_section_switch_data, code), 1 },
        { "data", 0, yasm_dir_helper_flag_set,
          offsetof(struct bin_section_switch_data, code), 0 },
        { "execute", 0, yasm_dir_helper_flag_set,
          offsetof(struct bin_section_switch_data, code), 1 },
        { "noexecute", 0, yasm_dir_helper_flag_set,
          offsetof(struct bin_section_switch_data, code), 0 }
    };

    vp = yasm_vps_first(valparams);
    sectname = yasm_vp_string(vp);
    if (!sectname)
        return NULL;
    vp = yasm_vps_next(vp);

    retval = yasm_object_find_general(object, sectname);
    if (retval) {
        bsd = yasm_section_get_data(retval, &bin_section_data_cb);
        assert(bsd != NULL);
        data.follows = bsd->follows;
        data.vfollows = bsd->vfollows;
        data.start = bsd->start;
        data.vstart = bsd->vstart;
        data.align = NULL;
        data.valign = NULL;
        data.bss = bsd->bss;
        data.code = yasm_section_is_code(retval);
    } else {
        data.follows = NULL;
        data.vfollows = NULL;
        data.start = NULL;
        data.vstart = NULL;
        data.align = NULL;
        data.valign = NULL;
        data.bss = strcmp(sectname, ".bss") == 0;
        data.code = strcmp(sectname, ".text") == 0;
    }

    flags_override = yasm_dir_helper(object, vp, line, help, NELEMS(help),
                                     &data, yasm_dir_helper_valparam_warn);
    if (flags_override < 0)
        return NULL;    /* error occurred */

    if (data.start && data.follows) {
        yasm_error_set(YASM_ERROR_GENERAL,
            N_("cannot combine `start' and `follows' section attributes"));
        return NULL;
    }

    if (data.vstart && data.vfollows) {
        yasm_error_set(YASM_ERROR_GENERAL,
            N_("cannot combine `vstart' and `vfollows' section attributes"));
        return NULL;
    }

    if (data.align) {
        unsigned long align = yasm_intnum_get_uint(data.align);

        /* Alignments must be a power of two. */
        if (!is_exp2(align)) {
            yasm_error_set(YASM_ERROR_VALUE,
                           N_("argument to `%s' is not a power of two"),
                           "align");
            return NULL;
        }
    } else
        data.align = bsd ? bsd->align : NULL;

    if (data.valign) {
        unsigned long valign = yasm_intnum_get_uint(data.valign);

        /* Alignments must be a power of two. */
        if (!is_exp2(valign)) {
            yasm_error_set(YASM_ERROR_VALUE,
                           N_("argument to `%s' is not a power of two"),
                           "valign");
            return NULL;
        }
    } else
        data.valign = bsd ? bsd->valign : NULL;

    retval = yasm_object_get_general(object, sectname, 0, (int)data.code,
                                     (int)data.bss, &isnew, line);

    if (isnew)
        bsd = bin_objfmt_init_new_section(object, retval, sectname, line);
    else
        bsd = yasm_section_get_data(retval, &bin_section_data_cb);

    if (isnew || yasm_section_is_default(retval)) {
        yasm_section_set_default(retval, 0);
    }

    /* Update section flags */
    bsd->bss = data.bss;
    bsd->align = data.align;
    bsd->valign = data.valign;
    bsd->start = data.start;
    bsd->vstart = data.vstart;
    bsd->follows = data.follows;
    bsd->vfollows = data.vfollows;

    return retval;
}
#endif
void
BinObject::dir_org(Object& object, const NameValues& namevals,
                   const NameValues& objext_namevals, unsigned long line)
{
    // We only allow a single ORG in a program.
    if (m_org.get() != 0)
        throw Error(N_("program origin redefined"));

    // ORG takes just a simple expression as param
    const NameValue& nv = namevals.front();
    if (!nv.is_expr())
        throw SyntaxError(N_("argument to ORG must be expression"));
    m_org.reset(nv.get_expr(object, line).release());
}

bool
BinObject::map_filename(const NameValue& nv)
{
    if (!m_map_filename.empty())
        throw Error(N_("map file already specified"));

    if (!nv.is_string())
        throw SyntaxError(N_("unexpected expression in [map]"));
    m_map_filename = nv.get_string();
    return true;
}

void
BinObject::dir_map(Object& object, const NameValues& namevals,
                   const NameValues& objext_namevals, unsigned long line)
{
    DirHelpers helpers;
    helpers.add("all", false,
                BIND::bind(&dir_flag_set, _1, REF::ref(m_map_flags),
                           MAP_BRIEF|MAP_SECTIONS|MAP_SYMBOLS));
    helpers.add("brief", false,
                BIND::bind(&dir_flag_set, _1, REF::ref(m_map_flags),
                           static_cast<unsigned long>(MAP_BRIEF)));
    helpers.add("sections", false,
                BIND::bind(&dir_flag_set, _1, REF::ref(m_map_flags),
                           static_cast<unsigned long>(MAP_SECTIONS)));
    helpers.add("segments", false,
                BIND::bind(&dir_flag_set, _1, REF::ref(m_map_flags),
                           static_cast<unsigned long>(MAP_SECTIONS)));
    helpers.add("symbols", false,
                BIND::bind(&dir_flag_set, _1, REF::ref(m_map_flags),
                           static_cast<unsigned long>(MAP_SYMBOLS)));

    m_map_flags |= MAP_NONE;

    helpers(namevals.begin(), namevals.end(), 
            BIND::bind(&BinObject::map_filename, this, _1));
}

std::vector<std::string>
BinObject::get_dbgfmt_keywords() const
{
    static const char* keywords[] = {"null"};
    return std::vector<std::string>(keywords, keywords+NELEMS(keywords));
}

void
BinObject::add_directives(Directives& dirs, const std::string& parser)
{
    if (!String::nocase_equal(parser, "nasm"))
        return;
    dirs.add("org",
             BIND::bind(&BinObject::dir_org, this, _1, _2, _3, _4),
             Directives::ARG_REQUIRED);
    dirs.add("map",
             BIND::bind(&BinObject::dir_map, this, _1, _2, _3, _4),
             Directives::ANY);
}

void
do_register()
{
    register_module<ObjectFormat, BinObject>("bin");
}

}}} // namespace yasm::objfmt::bin
