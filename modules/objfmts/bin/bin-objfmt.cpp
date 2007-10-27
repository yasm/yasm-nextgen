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

#include <boost/bind.hpp>

#include <libyasm/bytecode.h>
#include <libyasm/bytes.h>
#include <libyasm/directive.h>
#include <libyasm/errwarn.h>
#include <libyasm/expr.h>
#include <libyasm/factory.h>
#include <libyasm/intnum.h>
#include <libyasm/nocase.h>
#include <libyasm/object.h>
#include <libyasm/object_format.h>
#include <libyasm/name_value.h>
#include <libyasm/section.h>
#include <libyasm/symbol.h>
#include <libyasm/value.h>


namespace yasm { namespace objfmt { namespace bin {

class BinObject : public ObjectFormat {
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

    bool set_object(Object* object);

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
    void init_new_section(Section* sect, unsigned long line);
    void dir_org(Object& object, const NameValues& namevals,
                 const NameValues& objext_namevals, unsigned long line);

    Object* m_object;
};

BinObject::BinObject()
{
}

BinObject::~BinObject()
{
}

bool
BinObject::set_object(Object* object)
{
    m_object = object;
    return true;
}

// Aligns sect to either its specified alignment.  Uses prevsect and base to
// both determine the new starting address (returned) and the total length of
// prevsect after sect has been aligned.
static unsigned long
align_section(const Section& sect, const Section& prevsect, unsigned long base,
              /*@out@*/ unsigned long *prevsectlen,
              /*@out@*/ unsigned long *padamt)
{
    // Figure out the size of .text by looking at the last bytecode's offset
    // plus its length.  Add the start and size together to get the new start.
    *prevsectlen = prevsect.bcs_last().next_offset();
    unsigned long start = base + *prevsectlen;

    // Round new start up to alignment of .data section, and adjust textlen to
    // indicate padded size.  Because aignment is always a power of two, we
    // can use some bit trickery to do this easily.
    unsigned long align = sect.get_align();

    if (start & (align-1))
        start = (start & ~(align-1)) + align;

    *padamt = start - (base + *prevsectlen);

    return start;
}

class Output {
public:
    Output(std::ostream& os, Object& object, unsigned long abs_start);
    ~Output();

    void output_section(Section* sect, unsigned long start, Errwarns& errwarns);

private:
    static void expr_xform(Expr* e);
    void output_value(Value& value, Bytes& bytes, unsigned int destsize,
                      /*@unused@*/ unsigned long offset, Bytecode& bc,
                      int warn);
    void output_bytecode(Bytecode& bc);

    Object& m_object;
    std::ostream& m_os;
    Bytes m_bytes;
    /*@observer@*/ const Section* m_sect;
    unsigned long m_start;          // what normal variables go against
    unsigned long m_abs_start;      // what absolutes go against
};

Output::Output(std::ostream& os, Object& object, unsigned long abs_start)
    : m_object(object),
      m_os(os),
      m_abs_start(abs_start)
{
}

Output::~Output()
{
}

void
Output::output_section(Section* sect, unsigned long start, Errwarns& errwarns)
{
    m_sect = sect;
    m_start = start;
    for (Section::bc_iterator i=sect->bcs_begin(), end=sect->bcs_end();
         i != end; ++i) {
        try {
            output_bytecode(*i);
        } catch (Error& err) {
            errwarns.propagate(i->get_line(), err);
        }
        errwarns.propagate(i->get_line());  // propagate warnings
    }
}

void
Output::expr_xform(Expr* e)
{
    for (Expr::Terms::iterator i=e->get_terms().begin(),
         end=e->get_terms().end(); i != end; ++i) {
        Symbol* sym;
        Bytecode* precbc;
        Section* sect;
        std::auto_ptr<IntNum> dist(new IntNum(0));

        // Transform symrecs or precbcs that reference sections into
        // start expr + intnum(dist).
        if ((((sym = i->get_sym()) && sym->get_label(precbc)) ||
             (precbc = i->get_precbc())) &&
            (sect = precbc->get_section()) &&
            (calc_bc_dist(sect->bcs_first(), *precbc, *dist))) {
            const Expr* start = sect->get_start();
            //i->destroy(); // don't need to, as it's a sym or precbc
            *i = new Expr(start->clone(), Op::ADD, dist, e->get_line());
        }
    }
}

void
Output::output_value(Value& value, Bytes& bytes, unsigned int destsize,
                     /*@unused@*/ unsigned long offset, Bytecode& bc, int warn)
{
    // Binary objects we need to resolve against object, not against section.
    if (value.is_relative()) {
        Bytecode* precbc;
        Section* sect;
        unsigned int rshift = (unsigned int)value.m_rshift;
        Expr::Ptr syme(0);

        if (value.m_rel->is_abs()) {
            syme.reset(new Expr(new IntNum(0), bc.get_line()));
        } else if (value.m_rel->get_label(precbc)
                   && (sect = precbc->get_section())) {
            syme.reset(new Expr(value.m_rel, bc.get_line()));
        } else
            goto done;

        // Handle PC-relative
        if (value.m_curpos_rel) {
            Expr::Ptr sube(new Expr(&bc, Op::SUB,
                                    new IntNum(bc.get_len() *
                                               bc.get_multiple(true)),
                                    bc.get_line()));
            syme.reset(new Expr(syme, Op::SUB, sube, bc.get_line()));
            value.m_curpos_rel = 0;
            value.m_ip_rel = 0;
        }

        if (value.m_rshift > 0)
            syme.reset(new Expr(syme, Op::SHR, new IntNum(rshift),
                                bc.get_line()));

        // Add into absolute portion
        value.add_abs(syme);
        value.m_rel = 0;
        value.m_rshift = 0;
    }
done:
    // Simplify absolute portion of value, transforming symrecs
    if (Expr* abs = value.get_abs())
        abs->level_tree(true, true, true, &Output::expr_xform);

    // Output
    Arch* arch = m_object.get_arch();
    if (value.output_basic(bytes, destsize, bc, warn, *arch))
        return;

    // Couldn't output, assume it contains an external reference.
    throw Error(
        N_("binary object format does not support external references"));
}

void
Output::output_bytecode(Bytecode& bc)
{
    unsigned long gap = 0;
    m_bytes.clear();
    bc.to_bytes(m_bytes, gap, boost::bind(&Output::output_value, this, _1, _2,
                                          _3, _4, _5, _6));

    // Don't bother doing anything else if size ended up being 0.
    if (m_bytes.empty() && gap == 0)
        return;

    // Warn that gaps are converted to 0 and write out the 0's.
    if (gap) {
        static const unsigned long BLOCK_SIZE = 4096;

        warn_set(WARN_UNINIT_CONTENTS,
            N_("uninitialized space declared in code/data section: zeroing"));
        // Write out in chunks
        m_bytes.clear();
        m_bytes.resize(BLOCK_SIZE);
        while (gap > BLOCK_SIZE) {
            m_os << m_bytes;
            gap -= BLOCK_SIZE;
        }
        m_bytes.resize(gap);
        m_os << m_bytes;
    } else {
        // Output bytes to file
        m_os << m_bytes;
    }
}

static void
check_sym(const Symbol& sym, Errwarns& errwarns)
{
    int vis = sym.get_visibility();

    if (vis & Symbol::EXTERN) {
        warn_set(WARN_GENERAL,
            N_("binary object format does not support extern variables"));
        errwarns.propagate(sym.get_decl_line());
    } else if (vis & Symbol::GLOBAL) {
        warn_set(WARN_GENERAL,
            N_("binary object format does not support global variables"));
        errwarns.propagate(sym.get_decl_line());
    } else if (vis & Symbol::COMMON) {
        errwarns.propagate(sym.get_decl_line(), TypeError(
            N_("binary object format does not support common variables")));
    }
}

void
BinObject::output(std::ostream& os, bool all_syms, Errwarns& errwarns)
{
    // Check symbol table
    for (Object::const_symbol_iterator i=m_object->symbols_begin(),
         end=m_object->symbols_end(); i != end; ++i)
        check_sym(*i, errwarns);

    Section* text = m_object->find_section(".text");
    Section* data = m_object->find_section(".data");
    Section* bss = m_object->find_section(".bss");

    if (!text)
        throw InternalError(N_("No `.text' section in bin objfmt output"));

    // First determine the actual starting offsets for .data and .bss.
    // As the order in the file is .text -> .data -> .bss (not present),
    // use the last bytecode in .text (and the .text section start) to
    // determine the starting offset in .data, and likewise for .bss.
    // Also compensate properly for alignment.

    // Find out the start of .text
    Expr::Ptr startexpr(text->get_start()->clone());
    IntNum* startnum = startexpr->get_intnum();
    if (!startnum) {
        errwarns.propagate(startexpr->get_line(),
            TooComplexError(N_("ORG expression too complex")));
        return;
    }
    unsigned long start = startnum->get_uint();
    unsigned long abs_start = start;
    unsigned long textstart = start, datastart = 0;

    // Align .data and .bss (if present) by adjusting their starts.
    Section* prevsect = text;
    unsigned long textlen = 0, textpad = 0, datalen = 0, datapad = 0;
    unsigned long* prevsectlenptr = &textlen;
    unsigned long* prevsectpadptr = &textpad;
    if (data) {
        start = align_section(*data, *prevsect, start, prevsectlenptr,
                              prevsectpadptr);
        data->set_start(Expr::Ptr(new Expr(new IntNum(start), 0)));
        datastart = start;
        prevsect = data;
        prevsectlenptr = &datalen;
        prevsectpadptr = &datapad;
    }
    if (bss) {
        start = align_section(*bss, *prevsect, start, prevsectlenptr,
                              prevsectpadptr);
        bss->set_start(Expr::Ptr(new Expr(new IntNum(start), 0)));
    }

    // Output .text first.
    Output output(os, *m_object, abs_start);
    output.output_section(text, textstart, errwarns);

    // If .data is present, output it
    if (data) {
        // Add padding to align .data.  Just use a for loop, as this will
        // seldom be very many bytes.
        for (unsigned long i=0; i<textpad; i++)
            os << '\0';

        // Output .data bytecodes
        output.output_section(data, datastart, errwarns);
    }

    // If .bss is present, check it for non-reserve bytecodes
}

void
BinObject::init_new_section(Section* sect, unsigned long line)
{
    m_object->get_sym(sect->get_name())->define_label(sect->bcs_first(), line);
}

Section*
BinObject::add_default_section()
{
    Section* section = new Section(".text", Expr::Ptr(0), 16, true, false, 0);
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
    unsigned long start;
    const char *sectname;
    int resonly = 0;
    /*@only@*/ /*@null@*/ yasm_intnum *align_intn = NULL;
    unsigned long align = 4;
    int have_align = 0;

    static const yasm_dir_help help[] = {
        { "align", 1, yasm_dir_helper_intn, 0, 0 }
    };

    vp = yasm_vps_first(valparams);
    sectname = yasm_vp_string(vp);
    if (!sectname)
        return NULL;
    vp = yasm_vps_next(vp);

    /* If it's the first section output (.text) start at 0, otherwise
     * make sure the start is > 128.
     */
    if (strcmp(sectname, ".text") == 0)
        start = 0;
    else if (strcmp(sectname, ".data") == 0)
        start = 200;
    else if (strcmp(sectname, ".bss") == 0) {
        start = 200;
        resonly = 1;
    } else {
        /* other section names not recognized. */
        yasm_error_set(YASM_ERROR_GENERAL,
                       N_("segment name `%s' not recognized"), sectname);
        return NULL;
    }

    have_align = yasm_dir_helper(object, vp, line, help, NELEMS(help),
                                 &align_intn, yasm_dir_helper_valparam_warn);
    if (have_align < 0)
        return NULL;    /* error occurred */

    if (align_intn) {
        align = yasm_intnum_get_uint(align_intn);
        yasm_intnum_destroy(align_intn);

        /* Alignments must be a power of two. */
        if (!is_exp2(align)) {
            yasm_error_set(YASM_ERROR_VALUE,
                           N_("argument to `%s' is not a power of two"),
                           "align");
            return NULL;
        }
    }

    retval = yasm_object_get_general(object, sectname,
        yasm_expr_create_ident(
            yasm_expr_int(yasm_intnum_create_uint(start)), line), align,
        strcmp(sectname, ".text") == 0, resonly, &isnew, line);

    if (isnew)
        bin_objfmt_init_new_section(object, retval, sectname, line);

    if (isnew || yasm_section_is_default(retval)) {
        yasm_section_set_default(retval, 0);
        yasm_section_set_align(retval, align, line);
    } else if (have_align)
        yasm_warn_set(YASM_WARN_GENERAL,
            N_("alignment value ignored on section redeclaration"));

    return retval;
}
#endif
void
BinObject::dir_org(Object& object, const NameValues& namevals,
                   const NameValues& objext_namevals, unsigned long line)
{
    // ORG takes just a simple integer as param
    const NameValue& nv = namevals.front();
    if (!nv.is_expr())
        throw SyntaxError(N_("argument to ORG must be expression"));
    Expr::Ptr start = nv.get_expr(object, line);

    // ORG changes the start of the .text section
    Section* sect = object.find_section(".text");
    if (!sect)
        throw InternalError(
            N_("bin objfmt: .text section does not exist before ORG is called?"));
    sect->set_start(start);
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
             boost::bind(&BinObject::dir_org, this, _1, _2, _3, _4),
             Directives::ARG_REQUIRED);
}

ddj::registerInFactory<ObjectFormat, BinObject> registerBinObject("bin");
bool static_ref = true;

}}} // namespace yasm::objfmt::bin