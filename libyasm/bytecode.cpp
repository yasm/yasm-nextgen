/*
 * YASM bytecode implementation
 *
 *  Copyright (C) 2001-2007  Peter Johnson
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND OTHER CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR OTHER CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "util.h"

#include <iomanip>

#include "errwarn.h"
#include "intnum.h"
#include "expr.h"
//#include "value.h"
//#include "symrec.h"

#include "bytecode.h"
#include "insn.h"

namespace yasm {

void
Bytecode::set_multiple(std::auto_ptr<Expr> e)
{
    if (m_multiple.get() != 0)
        m_multiple.reset(new Expr(m_multiple, Expr::MUL, e, e->get_line()));
    else
        m_multiple = e;
}

bool
Bytecode::Contents::calc_len(Bytecode* bc, Bytecode::AddSpanFunc add_span)
{
    internal_error(N_("bytecode length cannot be calculated"));
    /*@unreached@*/
    return false;
}

int
Bytecode::Contents::expand(Bytecode *bc, int span,
                           long old_val, long new_val,
                           /*@out@*/ long& neg_thres,
                           /*@out@*/ long& pos_thres)
{
    internal_error(N_("bytecode does not have any dependent spans"));
    /*@unreached@*/
    return 0;
}

bool
Bytecode::Contents::to_bytes(Bytecode* bc, unsigned char* &buf,
                             OutputValueFunc output_value,
                             OutputRelocFunc output_reloc)
{
    internal_error(N_("bytecode cannot be converted to bytes"));
    /*@unreached@*/
    return false;
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
      m_offset(~0UL)    // obviously incorrect / uninitialized value
{
}

Bytecode::Bytecode(const Bytecode& oth)
    : m_contents(oth.m_contents->clone()),
      m_multiple(oth.m_multiple->clone())
{}

Bytecode&
Bytecode::operator= (const Bytecode& oth)
{
    *m_contents = *oth.m_contents;
    m_section = oth.m_section;
    *m_multiple = *oth.m_multiple;
    m_len = oth.m_len;
    m_mult_int = oth.m_mult_int;
    m_line = oth.m_line;
    m_offset = oth.m_offset;
    m_bc_index = oth.m_bc_index;
    m_symbols = oth.m_symbols;
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
    os << std::setw(indent_level) << "" << "Length=" << m_len;
    os << std::setw(indent_level) << "" << "Line Index=" << m_line;
    os << std::setw(indent_level) << "" << "Offset=" << m_offset;
}

void
Bytecode::finalize(Bytecode* prev_bc)
{
    m_contents->finalize(this, prev_bc);
    if (m_multiple.get() != 0) {
#if 0
        Value val;

        if (yasm_value_finalize_expr(&val, bc->multiple, prev_bc, 0))
            yasm_error_set(YASM_ERROR_TOO_COMPLEX,
                           N_("multiple expression too complex"));
        else if (val.rel)
            yasm_error_set(YASM_ERROR_NOT_ABSOLUTE,
                           N_("multiple expression not absolute"));
        /* Finalize creates NULL output if value=0, but bc->multiple is NULL
         * if value=1 (this difference is to make the common case small).
         * However, this means we need to set bc->multiple explicitly to 0
         * here if val.abs is NULL.
         */
        if (val.abs)
            bc->multiple = val.abs;
        else
            bc->multiple = yasm_expr_create_ident(
                yasm_expr_int(yasm_intnum_create_uint(0)), bc->line);
#endif
    }
}

/*@null@*/ IntNum*
Bytecode::calc_dist(const Bytecode* precbc1, const Bytecode* precbc2)
{
    unsigned long dist2, dist1;
    IntNum* intn;

    if (precbc1->m_section != precbc2->m_section)
        return 0;

    dist1 = precbc1->next_offset();
    dist2 = precbc2->next_offset();
    if (dist2 < dist1) {
        intn = new IntNum(dist1 - dist2);
        intn->calc(Expr::NEG);
        return intn;
    }
    dist2 -= dist1;
    return new IntNum(dist2);
}

unsigned long
Bytecode::next_offset() const
{
    return m_offset + m_len * m_mult_int;
}

bool
Bytecode::calc_len(AddSpanFunc add_span)
{
    m_len = 0;
    bool retval = m_contents->calc_len(this, add_span);

    /* Check for multiples */
    m_mult_int = 1;
    if (m_multiple.get() != 0) {
        /*@dependent@*/ /*@null@*/ const IntNum* num;

        num = m_multiple->get_intnum(false);
        if (num) {
            if (num->sign() < 0) {
                error_set(ERROR_VALUE, N_("multiple is negative"));
                retval = true;
            } else
                m_mult_int = num->get_int();
        } else {
            if (m_multiple->contains(Expr::FLOAT)) {
                error_set(ERROR_VALUE,
                    N_("expression must not contain floating point value"));
                retval = true;
            } else {
#if 0
                Value value;
                yasm_value_initialize(&value, bc->multiple, 0);
                add_span(add_span_data, bc, 0, &value, 0, 0);
#endif
                m_mult_int = 0;     // assume 0 to start
            }
        }
    }

    // If we got an error somewhere along the line, clear out any calc len
    if (retval)
        m_len = 0;

    return retval;
}

int
Bytecode::expand(int span, long old_val, long new_val,
                 /*@out@*/ long& neg_thres, /*@out@*/ long& pos_thres)
{
    if (span == 0) {
        m_mult_int = new_val;
        return 1;
    }
    return m_contents->expand(this, span, old_val, new_val, neg_thres,
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

    if (get_multiple(m_mult_int, true) || m_mult_int == 0) {
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
        bool error = m_contents->to_bytes(this, destbuf, output_value,
                                          output_reloc);

        if (!error && ((unsigned long)(destbuf - origbuf) != m_len))
            internal_error(
                N_("written length does not match optimized length"));
    }

    return mybuf;
}

bool
Bytecode::get_multiple(long& multiple, bool calc_bc_dist)
{
    multiple = 1;
    if (m_multiple.get() != 0) {
        const IntNum* num = m_multiple->get_intnum(calc_bc_dist);
        if (!num) {
            error_set(ERROR_VALUE, N_("could not determine multiple"));
            return true;
        }
        if (num->sign() < 0) {
            error_set(ERROR_VALUE, N_("multiple is negative"));
            return true;
        }
        multiple = num->get_int();
    }
    return false;
}

Insn*
Bytecode::get_insn()
{
    if (m_contents->get_special() != Contents::SPECIAL_INSN)
        return 0;
    return static_cast<Insn*>(m_contents.get());
}

} // namespace yasm
