//
// Value handling
//
//  Copyright (C) 2006-2007  Peter Johnson
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
#include "value.h"

#include "util.h"

#include <iomanip>
#include <ostream>

#include "arch.h"
#include "bytes.h"
#include "bytecode.h"
#include "errwarn.h"
#include "expr.h"
#include "floatnum.h"
#include "intnum.h"
#include "object.h"
#include "section.h"
#include "symbol.h"


namespace yasm {

Value::Value(unsigned int size)
    : m_abs(0),
      m_rel(0),
      m_wrt(0),
      m_seg_of(false),
      m_rshift(0),
      m_curpos_rel(false),
      m_ip_rel(false),
      m_jump_target(false),
      m_section_rel(false),
      m_sign(false),
      m_size(size)
{
}

Value::Value(unsigned int size, std::auto_ptr<Expr> e)
    : m_abs(e.release()),
      m_rel(0),
      m_wrt(0),
      m_seg_of(false),
      m_rshift(0),
      m_curpos_rel(false),
      m_ip_rel(false),
      m_jump_target(false),
      m_section_rel(false),
      m_sign(false),
      m_size(size)
{
}

Value::Value(unsigned int size, /*@null@*/ Symbol* sym)
    : m_abs(0),
      m_rel(sym),
      m_wrt(0),
      m_seg_of(false),
      m_rshift(0),
      m_curpos_rel(false),
      m_ip_rel(false),
      m_jump_target(false),
      m_section_rel(false),
      m_sign(false),
      m_size(size)
{
}

Value::Value(const Value& oth)
    : m_abs(0),
      m_rel(oth.m_rel),
      m_wrt(oth.m_wrt),
      m_seg_of(oth.m_seg_of),
      m_rshift(oth.m_rshift),
      m_curpos_rel(oth.m_curpos_rel),
      m_ip_rel(oth.m_ip_rel),
      m_jump_target(oth.m_jump_target),
      m_section_rel(oth.m_section_rel),
      m_sign(oth.m_sign),
      m_size(oth.m_size)
{
    if (oth.m_abs)
        m_abs = oth.m_abs->clone();
}

Value::~Value()
{
    delete m_abs;
}

void
Value::clear()
{
    delete m_abs;
    m_abs = 0;
    m_rel = 0;
    m_wrt = 0;
    m_seg_of = false;
    m_rshift = 0;
    m_curpos_rel = false;
    m_ip_rel = false;
    m_jump_target = false;
    m_section_rel = false;
    m_sign = false;
    m_size = 0;
}

Value&
Value::operator= (const Value& rhs)
{
    if (this != &rhs) {
        m_abs = 0;
        if (rhs.m_abs)
            m_abs = rhs.m_abs->clone();
        m_rel = rhs.m_rel;
        m_wrt = rhs.m_wrt;
        m_seg_of = rhs.m_seg_of;
        m_rshift = rhs.m_rshift;
        m_curpos_rel = rhs.m_curpos_rel;
        m_ip_rel = rhs.m_ip_rel;
        m_jump_target = rhs.m_jump_target;
        m_section_rel = rhs.m_section_rel;
        m_sign = rhs.m_sign;
        m_size = rhs.m_size;
    }
    return *this;
}

void
Value::set_curpos_rel(const Bytecode& bc, bool ip_rel)
{
    m_curpos_rel = true;
    m_ip_rel = ip_rel;
    // In order for us to correctly output curpos-relative values, we must
    // have a relative portion of the value.  If one doesn't exist, point
    // to a custom absolute symbol.
    if (!m_rel)
        m_rel = bc.get_section()->get_object()->get_abs_sym();
}

bool
Value::finalize_scan(Expr* e, /*@null@*/ Bytecode* expr_precbc,
                     bool ssym_not_ok)
{
#if 0
    int i;
    /*@dependent@*/ Section* sect;
    /*@dependent@*/ /*@null@*/ Bytecode* precbc;

    unsigned long shamt;    /* for SHR */

    // Yes, this has a maximum upper bound on 32 terms, based on an
    // "insane number of terms" (and ease of implementation) WAG.
    // The right way to do this would be a stack-based alloca, but that's
    // not ISO C.  We really don't want to malloc here as this function is
    // hit a lot!
    //
    // This is a bitmask to keep things small, as this is a recursive
    // routine and we don't want to eat up stack space.
    unsigned long used;     /* for ADD */

    // Thanks to this running after a simplify, we don't need to iterate
    // down through IDENTs or handle SUB.
    //
    // We scan for a single symrec, gathering info along the way.  After
    // we've found the symrec, we keep scanning but error if we find
    // another one.  We pull out the single symrec and any legal operations
    // performed on it.
    //
    // Also, if we find a float anywhere, we don't allow mixing of a single
    // symrec with it.
    switch (e->op) {
        case YASM_EXPR_ADD:
            // Okay for single symrec anywhere in expr.
            // Check for single symrec anywhere.
            // Handle symrec-symrec by checking for (-1*symrec)
            // and symrec term pairs (where both symrecs are in the same
            // segment).
            if (e->numterms > 32)
                yasm__fatal(N_("expression on line %d has too many add terms;"
                               " internal limit of 32"), e->line);

            used = 0;

            for (i=0; i<e->numterms; i++) {
                int j;
                yasm_expr *sube;
                yasm_intnum *intn;
                yasm_symrec *sym;
                /*@dependent@*/ yasm_section *sect2;
                /*@dependent@*/ /*@null@*/ yasm_bytecode *precbc2;

                // First look for an (-1*symrec) term
                if (e->terms[i].type != YASM_EXPR_EXPR)
                    continue;
                sube = e->terms[i].data.expn;

                if (sube->op != YASM_EXPR_MUL || sube->numterms != 2) {
                    // recurse instead
                    if (value_finalize_scan(value, sube, expr_precbc,
                                            ssym_not_ok))
                        return true;
                    continue;
                }

                if (sube->terms[0].type == YASM_EXPR_INT &&
                    sube->terms[1].type == YASM_EXPR_SYM) {
                    intn = sube->terms[0].data.intn;
                    sym = sube->terms[1].data.sym;
                } else if (sube->terms[0].type == YASM_EXPR_SYM &&
                           sube->terms[1].type == YASM_EXPR_INT) {
                    sym = sube->terms[0].data.sym;
                    intn = sube->terms[1].data.intn;
                } else {
                    if (value_finalize_scan(value, sube, expr_precbc,
                                            ssym_not_ok))
                        return true;
                    continue;
                }

                if (!yasm_intnum_is_neg1(intn)) {
                    if (value_finalize_scan(value, sube, expr_precbc,
                                            ssym_not_ok))
                        return true;
                    continue;
                }

                if (!yasm_symrec_get_label(sym, &precbc)) {
                    if (value_finalize_scan(value, sube, expr_precbc,
                                            ssym_not_ok))
                        return true;
                    continue;
                }
                sect2 = yasm_bc_get_section(precbc);

                // Now look for a unused symrec term in the same segment
                for (j=0; j<e->numterms; j++) {
                    if (e->terms[j].type == YASM_EXPR_SYM
                        && yasm_symrec_get_label(e->terms[j].data.sym,
                                                 &precbc2)
                        && (sect = yasm_bc_get_section(precbc2))
                        && sect == sect2
                        && (used & (1<<j)) == 0) {
                        // Mark as used
                        used |= 1<<j;
                        break;  // stop looking
                    }
                }

                // We didn't match in the same segment.  If the
                // -1*symrec is actually -1*curpos, we can match
                // unused symrec terms in other segments and generate
                // a curpos-relative reloc.
                //
                // Similarly, handle -1*symrec in other segment via the
                // following transformation:
                // other-this = (other-.)+(.-this)
                // We can only do this transformation if "this" is in
                // this expr's segment.
                //
                // Don't do this if we've already become curpos-relative.
                // The unmatched symrec will be caught below.
                if (j == e->numterms && !value->curpos_rel
                    && (yasm_symrec_is_curpos(sym)
                        || (expr_precbc
                            && sect2 == yasm_bc_get_section(expr_precbc)))) {
                    for (j=0; j<e->numterms; j++) {
                        if (e->terms[j].type == YASM_EXPR_SYM
                            && yasm_symrec_get_label(e->terms[j].data.sym,
                                                     &precbc2)
                            && (used & (1<<j)) == 0) {
                            // Mark as used
                            used |= 1<<j;
                            // Mark value as curpos-relative
                            if (value->rel || ssym_not_ok)
                                return true;
                            value->rel = e->terms[j].data.sym;
                            value->curpos_rel = 1;
                            if (yasm_symrec_is_curpos(sym)) {
                                // Replace both symrec portions with 0
                                yasm_expr_destroy(sube);
                                e->terms[i].type = YASM_EXPR_INT;
                                e->terms[i].data.intn =
                                    yasm_intnum_create_uint(0);
                                e->terms[j].type = YASM_EXPR_INT;
                                e->terms[j].data.intn =
                                    yasm_intnum_create_uint(0);
                            } else {
                                // Replace positive portion with curpos
                                yasm_object *object =
                                    yasm_section_get_object(sect2);
                                yasm_symtab *symtab = object->symtab;
                                e->terms[j].data.sym =
                                    yasm_symtab_define_curpos
                                    (symtab, ".", expr_precbc, e->line);
                            }
                            break;      // stop looking
                        }
                    }
                }


                if (j == e->numterms)
                    return true;    // We didn't find a match!
            }

            // Look for unmatched symrecs.  If we've already found one or
            // we don't WANT to find one, error out.
            for (i=0; i<e->numterms; i++) {
                if (e->terms[i].type == YASM_EXPR_SYM
                    && (used & (1<<i)) == 0) {
                    if (value->rel || ssym_not_ok)
                        return true;
                    value->rel = e->terms[i].data.sym;
                    // and replace with 0
                    e->terms[i].type = YASM_EXPR_INT;
                    e->terms[i].data.intn = yasm_intnum_create_uint(0);
                }
            }
            break;
        case YASM_EXPR_SHR:
            // Okay for single symrec in LHS and constant on RHS.
            // Single symrecs are not okay on RHS.
            // If RHS is non-constant, don't allow single symrec on LHS.
            // XXX: should rshift be an expr instead??

            // Check for not allowed cases on RHS
            switch (e->terms[1].type) {
                case YASM_EXPR_REG:
                case YASM_EXPR_FLOAT:
                    return true;        // not legal
                case YASM_EXPR_SYM:
                    return true;
                case YASM_EXPR_EXPR:
                    if (value_finalize_scan(value, e->terms[1].data.expn,
                                            expr_precbc, 1))
                        return true;
                    break;
                default:
                    break;
            }

            // Check for single sym and allowed cases on LHS
            switch (e->terms[0].type) {
                //case YASM_EXPR_REG:   ????? should this be illegal ?????
                case YASM_EXPR_FLOAT:
                    return true;        // not legal
                case YASM_EXPR_SYM:
                    if (value->rel || ssym_not_ok)
                        return true;
                    value->rel = e->terms[0].data.sym;
                    // and replace with 0
                    e->terms[0].type = YASM_EXPR_INT;
                    e->terms[0].data.intn = yasm_intnum_create_uint(0);
                    break;
                case YASM_EXPR_EXPR:
                    // recurse
                    if (value_finalize_scan(value, e->terms[0].data.expn,
                                            expr_precbc, ssym_not_ok))
                        return true;
                    break;
                default:
                    break;      // ignore
            }

            // Handle RHS
            if (!value->rel)
                break;          // no handling needed
            if (e->terms[1].type != YASM_EXPR_INT)
                return true;    // can't shift sym by non-constant integer
            shamt = yasm_intnum_get_uint(e->terms[1].data.intn);
            if ((shamt + value->rshift) > YASM_VALUE_RSHIFT_MAX)
                return true;    // total shift would be too large
            value->rshift += shamt;

            // Just leave SHR in place
            break;
        case YASM_EXPR_SEG:
            // Okay for single symrec (can only be done once).
            // Not okay for anything BUT a single symrec as an immediate
            // child.
            if (e->terms[0].type != YASM_EXPR_SYM)
                return true;

            if (value->seg_of)
                return true;    // multiple SEG not legal
            value->seg_of = 1;

            if (value->rel || ssym_not_ok)
                return true;    // got a relative portion somewhere else?
            value->rel = e->terms[0].data.sym;

            // replace with ident'ed 0
            e->op = YASM_EXPR_IDENT;
            e->terms[0].type = YASM_EXPR_INT;
            e->terms[0].data.intn = yasm_intnum_create_uint(0);
            break;
        case YASM_EXPR_WRT:
            // Okay for single symrec in LHS and either a register or single
            // symrec (as an immediate child) on RHS.
            // If a single symrec on RHS, can only be done once.
            // WRT reg is left in expr for arch to look at.

            // Handle RHS
            switch (e->terms[1].type) {
                case YASM_EXPR_SYM:
                    if (value->wrt)
                        return true;
                    value->wrt = e->terms[1].data.sym;
                    // and drop the WRT portion
                    e->op = YASM_EXPR_IDENT;
                    e->numterms = 1;
                    break;
                case YASM_EXPR_REG:
                    break;  // ignore
                default:
                    return true;
            }

            // Handle LHS
            switch (e->terms[0].type) {
                case YASM_EXPR_SYM:
                    if (value->rel || ssym_not_ok)
                        return true;
                    value->rel = e->terms[0].data.sym;
                    // and replace with 0
                    e->terms[0].type = YASM_EXPR_INT;
                    e->terms[0].data.intn = yasm_intnum_create_uint(0);
                    break;
                case YASM_EXPR_EXPR:
                    // recurse
                    return value_finalize_scan(value, e->terms[0].data.expn,
                                               expr_precbc, ssym_not_ok);
                default:
                    break;  // ignore
            }

            break;
        default:
            // Single symrec not allowed anywhere
            for (i=0; i<e->numterms; i++) {
                switch (e->terms[i].type) {
                    case YASM_EXPR_SYM:
                        return true;
                    case YASM_EXPR_EXPR:
                        // recurse
                        return value_finalize_scan(value,
                                                   e->terms[i].data.expn,
                                                   expr_precbc, 1);
                    default:
                        break;
                }
            }
            break;
    }
#endif

    return false;
}

bool
Value::finalize(Bytecode* precbc)
{
    m_abs->level_tree(true, true, false);

    // Handle trivial (IDENT) cases immediately
    if (m_abs->is_op(Op::IDENT)) {
        if (IntNum* intn = m_abs->get_intnum()) {
            if (intn->is_zero()) {
                delete m_abs;
                m_abs = 0;
            }
        } else if (Symbol* sym = m_abs->get_symbol()) {
            m_rel = sym;
            delete m_abs;
            m_abs = 0;
        }
        return false;
    }

    if (finalize_scan(m_abs, precbc, false))
        return true;

    m_abs->level_tree(true, true, false);

    // Simplify 0 in abs to NULL
    IntNum* intn;
    if (m_abs->is_op(Op::IDENT)
        && (intn = m_abs->get_terms()[0].get_int())
        && intn->is_zero()) {
        delete m_abs;
        m_abs = 0;
    }
    return false;
}

std::auto_ptr<IntNum>
Value::get_intnum(Bytecode* bc, bool calc_bc_dist)
{
    /*@dependent@*/ /*@null@*/ IntNum* intn = 0;
    std::auto_ptr<IntNum> outval(0);

    if (m_abs != 0) {
        // Handle integer expressions, if non-integer or too complex, return
        // NULL.
        if (calc_bc_dist)
            m_abs->level_tree(true, true, true, xform_calc_bc_dist);
        intn = m_abs->get_intnum();
        if (!intn)
            return outval;
    }

    if (m_rel) {
        // If relative portion is not in bc section, return NULL.
        // Otherwise get the relative portion's offset.
        if (!bc)
            return outval;  // Can't calculate relative value

        /*@dependent@*/ Bytecode* rel_prevbc;
        bool sym_local = m_rel->get_label(rel_prevbc);
        if (m_wrt || m_seg_of || m_section_rel || !sym_local)
            return outval;  // we can't handle SEG, WRT, or external symbols
        if (rel_prevbc->get_section() != bc->get_section())
            return outval;  // not in this section
        if (!m_curpos_rel)
            return outval;  // not PC-relative

        // Calculate value relative to current assembly position
        unsigned long dist = rel_prevbc->next_offset();
        if (dist < bc->get_offset()) {
            outval.reset(new IntNum(bc->get_offset() - dist));
            outval->calc(Op::NEG);
        } else {
            dist -= bc->get_offset();
            outval.reset(new IntNum(dist));
        }

        if (m_rshift > 0)
            outval->calc(Op::SHR, m_rshift);
        // Add in absolute portion
        if (intn)
            outval->calc(Op::ADD, intn);
        return outval;
    }

    if (intn)
        return std::auto_ptr<IntNum>(intn->clone());

    // No absolute or relative portions: output 0
    return std::auto_ptr<IntNum>(new IntNum(0));
}

void
Value::add_abs(std::auto_ptr<IntNum> delta)
{
    if (!m_abs)
        m_abs = new Expr(delta);
    else
        m_abs = new Expr(m_abs, Op::ADD, delta, m_abs->get_line());
}

void
Value::add_abs(std::auto_ptr<Expr> delta)
{
    if (!m_abs)
        m_abs = delta.release();
    else
        m_abs = new Expr(m_abs, Op::ADD, delta, m_abs->get_line());
}

bool
Value::output_basic(Bytes& bytes, size_t destsize, const Bytecode& bc,
                    int warn, const Arch& arch)
{
    /*@dependent@*/ /*@null@*/ IntNum* intn = 0;

    if (m_abs != 0) {
        // Handle floating point expressions
        FloatNum* flt;
        if (!m_rel && (flt = m_abs->get_float())) {
            arch.floatnum_tobytes(*flt, bytes, destsize, m_size, 0, warn);
            return true;
        }

        // Check for complex float expressions
        if (m_abs->contains(Expr::FLOAT))
            throw FloatingPointError(
                N_("floating point expression too complex"));

        // Handle normal integer expressions
        m_abs->level_tree(true, true, true, xform_calc_bc_dist);
        intn = m_abs->get_intnum();

        if (!intn) {
            // Second try before erroring: Expr::get_intnum() doesn't handle
            // SEG:OFF, so try simplifying out any to just the OFF portion,
            // then getting the intnum again.
            m_abs->extract_deep_segoff(); // returns auto_ptr, so ok to drop
            m_abs->level_tree(true, true, true, xform_calc_bc_dist);
            intn = m_abs->get_intnum();
        }

        if (!intn) {
            // Still don't have an integer!
            throw TooComplexError(N_("expression too complex"));
        }
    }

    // Adjust warn for signed/unsigned integer warnings
    if (warn != 0)
        warn = m_sign ? -1 : 1;

    if (m_rel) {
        // If relative portion is not in bc section, don't try to handle it
        // here.  Otherwise get the relative portion's offset.
        /*@dependent@*/ Bytecode* rel_prevbc;

        bool sym_local = m_rel->get_label(rel_prevbc);
        if (m_wrt || m_seg_of || m_section_rel || !sym_local)
            return false;   // we can't handle SEG, WRT, or external symbols
        if (rel_prevbc->get_section() != bc.get_section())
            return false;   // not in this section
        if (!m_curpos_rel)
            return false;   // not PC-relative

        // Calculate value relative to current assembly position
        IntNum outval(0);
        unsigned long dist = rel_prevbc->next_offset();
        if (dist < bc.get_offset()) {
            outval = bc.get_offset() - dist;
            outval.calc(Op::NEG);
        } else {
            dist -= bc.get_offset();
            outval = dist;
        }

        if (m_rshift > 0)
            outval.calc(Op::SHR, m_rshift);
        // Add in absolute portion
        if (intn)
            outval.calc(Op::ADD, intn);

        // Output!
        arch.intnum_tobytes(outval, bytes, destsize, m_size, 0, bc, warn);
        return true;
    }

    if (m_seg_of || m_rshift || m_curpos_rel || m_ip_rel || m_section_rel)
        return false;   // We can't handle this with just an absolute

    if (intn) {
        // Output just absolute portion
        arch.intnum_tobytes(*intn, bytes, destsize, m_size, 0, bc, warn);
    } else {
        // No absolute or relative portions: output 0
        arch.intnum_tobytes(0, bytes, destsize, m_size, 0, bc, warn);
    }

    return true;
}

void
Value::put(std::ostream& os, int indent_level) const
{
    os << std::setw(indent_level) << "" << m_size << "-bit, ";
    os << (m_sign ? "" : "un") << "signed\n";
    os << std::setw(indent_level) << ""
       << "Absolute portion=" << *m_abs << '\n';
    if (m_rel) {
        os << std::setw(indent_level) << "" << "Relative to=";
        os << (m_seg_of ? "SEG " : "") << m_rel->get_name() << '\n';
        if (m_wrt) {
            os << std::setw(indent_level) << ""
               << "(With respect to=" << m_wrt->get_name() << ")\n";
        }
        if (m_rshift > 0) {
            os << std::setw(indent_level) << ""
               << "(Right shifted by=" << m_rshift << ")\n";
        }
        if (m_curpos_rel) {
            os << std::setw(indent_level) << ""
               << "(Relative to current position)\n";
        }
        if (m_ip_rel)
            os << std::setw(indent_level) << "" << "(IP-relative)\n";
        if (m_jump_target)
            os << std::setw(indent_level) << "" << "(Jump target)\n";
        if (m_section_rel)
            os << std::setw(indent_level) << "" << "(Section-relative)\n";
    }
}

} // namespace yasm
