//
// YASM bytecode implementation
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
#include "util.h"

#include <iomanip>
#include <boost/bind.hpp>
#include <boost/ref.hpp>

#include "errwarn.h"
#include "intnum.h"
#include "expr.h"
#include "value.h"
//#include "symrec.h"

#include "bytecode.h"
#include "insn.h"

namespace yasm {

void
Bytecode::set_multiple(std::auto_ptr<Expr> e)
{
    if (m_multiple.get() != 0)
        m_multiple.reset(new Expr(m_multiple.get(), Expr::MUL, e,
                                  e->get_line()));
    else
        m_multiple.reset(e.release());
}

void
Bytecode::Contents::calc_len(Bytecode& bc, Bytecode::AddSpanFunc add_span)
{
    throw InternalError(N_("bytecode length cannot be calculated"));
}

bool
Bytecode::Contents::expand(Bytecode& bc, int span,
                           long old_val, long new_val,
                           /*@out@*/ long& neg_thres,
                           /*@out@*/ long& pos_thres)
{
    throw InternalError(N_("bytecode does not have any dependent spans"));
}

void
Bytecode::Contents::to_bytes(Bytecode& bc, unsigned char* &buf,
                             OutputValueFunc output_value,
                             OutputRelocFunc output_reloc)
{
    throw InternalError(N_("bytecode cannot be converted to bytes"));
}

void
Bytecode::transform(std::auto_ptr<Contents> contents)
{
    m_contents.reset(contents.release());
}

Bytecode::Bytecode(std::auto_ptr<Contents> contents, unsigned long line)
    : m_contents(contents),
      m_section(0),
      m_multiple(0),
      m_len(0),
      m_mult_int(1),
      m_line(line),
      m_offset(~0UL),   // obviously incorrect / uninitialized value
      m_bc_index(~0UL)
{
}

Bytecode::Bytecode(const Bytecode& oth)
    : m_contents(oth.m_contents->clone()),
      m_section(oth.m_section),
      m_multiple(oth.m_multiple->clone()),
      m_len(oth.m_len),
      m_mult_int(oth.m_mult_int),
      m_line(oth.m_line),
      m_offset(oth.m_offset),
      m_bc_index(oth.m_bc_index),
      m_symbols(oth.m_symbols)
{}

Bytecode&
Bytecode::operator= (const Bytecode& oth)
{
    if (this != &oth) {
        m_contents.reset(oth.m_contents->clone());
        m_section = oth.m_section;
        m_multiple.reset(oth.m_multiple->clone());
        m_len = oth.m_len;
        m_mult_int = oth.m_mult_int;
        m_line = oth.m_line;
        m_offset = oth.m_offset;
        m_bc_index = oth.m_bc_index;
        m_symbols = oth.m_symbols;
    }
    return *this;
}

void
Bytecode::put(std::ostream& os, int indent_level) const
{
    m_contents->put(os, indent_level);
    os << std::setw(indent_level) << "" << "Multiple=";
    if (m_multiple.get() == 0)
        os << "nil (1)";
    else
        os << *m_multiple;
    os << std::endl;
    os << std::setw(indent_level) << "" << "Length=" << m_len << std::endl;
    os << std::setw(indent_level) << "" << "Line Index=" << m_line
       << std::endl;
    os << std::setw(indent_level) << "" << "Offset=" << m_offset << std::endl;
}

void
Bytecode::finalize(Bytecode& prev_bc)
{
    m_contents->finalize(*this, prev_bc);
    if (m_multiple.get() != 0) {
        Value val(0, std::auto_ptr<Expr>(m_multiple->clone()));

        if (val.finalize(&prev_bc))
            throw TooComplexError(N_("multiple expression too complex"));
        else if (val.is_relative())
            throw NotAbsoluteError(N_("multiple expression not absolute"));
        // Finalize creates NULL output if value=0, but bc->multiple is NULL
        // if value=1 (this difference is to make the common case small).
        // However, this means we need to set bc->multiple explicitly to 0
        // here if val.abs is NULL.
        if (const Expr* e = val.get_abs())
            m_multiple.reset(e->clone());
        else
            m_multiple.reset(new Expr(new IntNum(0), m_line));
    }
}

void
Bytecode::finalize(Bytecode& prev_bc, Errwarns& errwarns)
{
    try {
        finalize(prev_bc);
    } catch (Error& err) {
        errwarns.propagate(m_line, err);
    }
    errwarns.propagate(m_line);     // propagate warnings
}

unsigned long
Bytecode::next_offset() const
{
    return m_offset + m_len * m_mult_int;
}

void
Bytecode::calc_len(AddSpanFunc add_span)
{
    m_len = 0;
    m_contents->calc_len(*this, add_span);

    // Check for multiples
    m_mult_int = 1;
    if (m_multiple.get() != 0) {
        if (const IntNum* num = m_multiple->get_intnum()) {
            if (num->sign() < 0) {
                m_len = 0;
                throw ValueError(N_("multiple is negative"));
            } else
                m_mult_int = num->get_int();
        } else {
            if (m_multiple->contains(Expr::FLOAT)) {
                m_len = 0;
                throw ValueError(
                    N_("expression must not contain floating point value"));
            } else {
                Value value(0, Expr::Ptr(m_multiple->clone()));
                add_span(this, 0, value, 0, 0);
                m_mult_int = 0;     // assume 0 to start
            }
        }
    }
}

bool
Bytecode::expand(int span, long old_val, long new_val,
                 /*@out@*/ long& neg_thres, /*@out@*/ long& pos_thres)
{
    if (span == 0) {
        m_mult_int = new_val;
        return true;
    }
    return m_contents->expand(*this, span, old_val, new_val, neg_thres,
                              pos_thres);
}

/*@null@*/ /*@only@*/ unsigned char *
Bytecode::to_bytes(unsigned char* buf, unsigned long& bufsize,
                   /*@out@*/ bool& gap,
                   OutputValueFunc output_value,
                   /*@null@*/ OutputRelocFunc output_reloc)
    /*@sets *buf@*/
{
    /*@only@*/ /*@null@*/ unsigned char* mybuf = 0;
    unsigned char *origbuf, *destbuf;
    long i;

    m_mult_int = get_multiple(true);
    if (m_mult_int == 0) {
        bufsize = 0;
        return 0;
    }

    // special case for reserve bytecodes
    if (m_contents->get_special() == Contents::SPECIAL_RESERVE) {
        bufsize = m_len * m_mult_int;
        gap = true;
        return 0;   // we didn't allocate a buffer
    }
    gap = false;

    if (bufsize < m_len * m_mult_int) {
        mybuf = new unsigned char[m_len * m_mult_int];
        destbuf = mybuf;
    } else
        destbuf = buf;

    bufsize = m_len * m_mult_int;

    for (i=0; i<m_mult_int; i++) {
        origbuf = destbuf;
        m_contents->to_bytes(*this, destbuf, output_value, output_reloc);

        if ((unsigned long)(destbuf - origbuf) != m_len)
            throw InternalError(
                N_("written length does not match optimized length"));
    }

    return mybuf;
}

long
Bytecode::get_multiple(bool calc_bc_dist)
{
    if (m_multiple.get() != 0) {
        if (calc_bc_dist)
            m_multiple->level_tree(true, true, true, xform_calc_bc_dist);
        const IntNum* num = m_multiple->get_intnum();
        if (!num)
            throw ValueError(N_("could not determine multiple"));
        if (num->sign() < 0)
            throw ValueError(N_("multiple is negative"));
        return num->get_int();
    }
    return 1;
}

Insn*
Bytecode::get_insn() const
{
    if (m_contents->get_special() != Contents::SPECIAL_INSN)
        return 0;
    return static_cast<Insn*>(m_contents.get());
}

unsigned long
Bytecode::update_offset(unsigned long offset, Bytecode& prev_bc)
{
    if (m_contents->get_special() == Contents::SPECIAL_OFFSET) {
        // Recalculate/adjust len of offset-based bytecodes here
        long neg_thres = 0;
        long pos_thres = (long)next_offset();
        expand(1, 0, (long)prev_bc.next_offset(), neg_thres, pos_thres);
    }
    m_offset = offset;
    return offset + m_len*m_mult_int;
}

unsigned long
Bytecode::update_offset(unsigned long offset, Bytecode& prev_bc,
                        Errwarns& errwarns)
{
    unsigned long retval;
    try {
        retval = update_offset(offset, prev_bc);
    } catch (Error& err) {
        errwarns.propagate(m_line, err);
    }
    errwarns.propagate(m_line);     // propagate warnings
    return retval;
}

/*@null@*/ std::auto_ptr<IntNum>
calc_bc_dist(const Bytecode* precbc1, const Bytecode* precbc2)
{
    if (precbc1->get_section() != precbc2->get_section())
        return std::auto_ptr<IntNum>(0);

    unsigned long dist1 = precbc1->next_offset();
    unsigned long dist2 = precbc2->next_offset();
    if (dist2 < dist1) {
        std::auto_ptr<IntNum> intn(new IntNum(dist1 - dist2));
        intn->calc(Expr::NEG);
        return intn;
    }
    dist2 -= dist1;
    return std::auto_ptr<IntNum>(new IntNum(dist2));
}

// Transforms instances of symrec-symrec [symrec+(-1*symrec)] into single
// exprterms if possible.  Uses a simple n^2 algorithm because n is usually
// quite small.  Also works for precbc-precbc (or symrec-precbc,
// precbc-symrec).
static void
xform_bc_dist_base(Expr* e, boost::function<bool (Expr::Term& term,
                                                  Bytecode* precbc,
                                                  Bytecode* precbc2)> func)
{
    // Handle symrec-symrec in ADD exprs by looking for (-1*symrec) and
    // symrec term pairs (where both symrecs are in the same segment).
    if (!e->is_op(Expr::ADD))
        return;

    Expr::Terms& terms = e->get_terms();
    for (Expr::Terms::iterator i=terms.begin(), end=terms.end();
         i != end; ++i) {
        // First look for an (-1*symrec) term
        Expr* sube = i->get_expr();
        if (!sube)
            continue;
        Expr::Terms& subterms = sube->get_terms();
        if (!sube->is_op(Expr::MUL) || subterms.size() != 2)
            continue;

        IntNum* intn;
        Symbol* sym = 0;
        /*@dependent@*/ /*@null@*/ Bytecode* precbc;

        if ((intn = subterms[0].get_int())) {
            if ((sym = subterms[1].get_sym()))
                ;
            else if ((precbc = subterms[1].get_precbc()))
                ;
            else
                continue;
        } else if ((intn = subterms[1].get_int())) {
            if ((sym = subterms[0].get_sym()))
                ;
            else if ((precbc = subterms[0].get_precbc()))
                ;
            else
                continue;
        } else
            continue;

        if (!intn->is_neg1())
            continue;
#if 0
        if (sym && !sym->get_label(precbc))
            continue;
        Section* sect = precbc->get_section();

        // Now look for a symrec term in the same segment
        for (Expr::Terms::iterator j=terms.begin(), end=terms.end();
             j != end; ++j) {
            Symbol* sym2;
            /*@dependent@*/ /*@null@*/ Bytecode* precbc2;

            if ((((sym2 = j->get_sym()) && sym2->get_label(&precbc2)) ||
                 (precbc2 = j->get_precbc())) &&
                (sect == precbc2->get_section()) &&
                func(*j, precbc, precbc2)) {
                // Delete the matching (-1*symrec) term
                i->release();
                break;  // stop looking for matching symrec term
            }
        }
#endif
    }

    // Clean up any deleted (NONE) terms
    Expr::Terms::iterator erasefrom =
        std::remove_if(terms.begin(), terms.end(),
                       boost::bind(&Expr::Term::is_type, _1, Expr::NONE));
    terms.erase(erasefrom, terms.end());
    Expr::Terms(terms).swap(terms);   // trim capacity
}

static inline bool
calc_bc_dist_cb(Expr::Term& term, Bytecode* precbc, Bytecode* precbc2)
{
    std::auto_ptr<IntNum> dist = calc_bc_dist(precbc, precbc2);
    if (dist.get() == 0)
        return false;
    // Change the term to an integer
    term = dist;
    return true;
}

void
xform_calc_bc_dist(Expr* e)
{
    xform_bc_dist_base(e, &calc_bc_dist_cb);
}

static inline bool
subst_bc_dist_cb(Expr::Term& term, Bytecode* precbc, Bytecode* precbc2,
                 unsigned int& subst,
                 boost::function<void (unsigned int subst,
                                       Bytecode* precbc,
                                       Bytecode* precbc2)> func)
{
    // Call higher-level callback
    func(subst, precbc, precbc2);
    // Change the term to an subst
    term = Expr::Term(Expr::Term::Subst(subst));
    subst++;
    return true;
}

static inline void
xform_subst_bc_dist(Expr* e, unsigned int& subst,
                    boost::function<void (unsigned int subst,
                                          Bytecode* precbc,
                                          Bytecode* precbc2)> func)
{
    xform_bc_dist_base(e, boost::bind(&subst_bc_dist_cb, _1, _2, _3,
                                      boost::ref(subst), func));
}

int
subst_bc_dist(Expr* e, boost::function<void (unsigned int subst,
                                             Bytecode* precbc,
                                             Bytecode* precbc2)> func)
{
    unsigned int subst = 0;
    e->level_tree(true, true, true,
                  boost::bind(&xform_subst_bc_dist, _1, boost::ref(subst),
                              func));
    return subst;
}


} // namespace yasm
