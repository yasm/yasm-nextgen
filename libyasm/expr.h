#ifndef YASM_EXPR_H
#define YASM_EXPR_H
///
/// @file libyasm/expr.h
/// @brief YASM expression interface.
///
/// @license
///  Copyright (C) 2001-2007  Michael Urman, Peter Johnson
///
/// Redistribution and use in source and binary forms, with or without
/// modification, are permitted provided that the following conditions
/// are met:
///  - Redistributions of source code must retain the above copyright
///    notice, this list of conditions and the following disclaimer.
///  - Redistributions in binary form must reproduce the above copyright
///    notice, this list of conditions and the following disclaimer in the
///    documentation and/or other materials provided with the distribution.
///
/// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND OTHER CONTRIBUTORS ``AS IS''
/// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
/// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
/// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR OTHER CONTRIBUTORS BE
/// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
/// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
/// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
/// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
/// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
/// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
/// POSSIBILITY OF SUCH DAMAGE.
/// @endlicense
///
#include <iosfwd>
#include <memory>
#include <vector>

#include <boost/function.hpp>

#include "operator.h"


namespace yasm {

class Bytecode;
class FloatNum;
class IntNum;
class Register;
class Symbol;

/// An expression.
class Expr {
    friend std::ostream& operator<< (std::ostream&, const Expr&);

public:
    typedef std::auto_ptr<Expr> Ptr;

    /// Types listed in canonical sorting order.  See expr_order_terms().
    /// Note precbc must be used carefully (in a-b pairs), as only symrecs
    /// can become the relative term in a #yasm_value.
    /// Testing uses bit comparison (&) so these have to be in bitmask form.
    enum TermType {
        NONE = 0,       ///< Nothing (temporary placeholder only).
        REG = 1<<0,     ///< Register.
        INT = 1<<1,     ///< Integer.
        SUBST = 1<<2,   ///< Substitution value.
        FLOAT = 1<<3,   ///< Float.
        SYM = 1<<4,     ///< Symbol.
        PRECBC = 1<<5,  ///< Direct bytecode ref (rather than via symrec).
        EXPR = 1<<6     ///< Subexpression.
    };

    /// An term inside the expression.
    class Term {
        friend std::ostream& operator<< (std::ostream&, const Term&);

    public:
        /// Substitution value.
        struct Subst {
            explicit Subst(unsigned int v) : subst(v) {}
            unsigned int subst;
        };

        Term(const Register* reg) : m_type(REG), m_reg(reg) {}
        Term(IntNum* intn) : m_type(INT), m_intn(intn) {}
        Term(FloatNum* flt) : m_type(FLOAT), m_flt(flt) {}
        explicit Term(const Subst& subst)
            : m_type(SUBST), m_subst(subst.subst) {}
        Term(Symbol* sym) : m_type(SYM), m_sym(sym) {}
        Term(Bytecode* bc) : m_type(PRECBC), m_precbc(bc) {}
        Term(Expr* expr) : m_type(EXPR), m_expr(expr) {}

        // auto_ptr constructors

        Term(std::auto_ptr<IntNum> intn);
        Term(std::auto_ptr<FloatNum> flt);
        Term(std::auto_ptr<Expr> expr);

        /// Explicit copy creator.
        /// There's no copy constructor or assignment operator as we want
        /// to use the default bit-copy ones.  Even though this class
        /// contains more complex structures, we don't want to be copying
        /// the contents all the time.
        Term clone() const;

        /// Explicit release.
        /// Doesn't destroy contents, just ensures what contents are there
        /// won't be destroyed via destroy().  Also marks the type as
        /// Expr::NONE for easy filtering (e.g. with std::remove_if()).
        void release() { m_type = NONE; m_reg = 0; }

        /// Explicit destructor.
        /// Similar to clone(), we do smart copying and destruction in
        /// #Expr implementation to prevent over-copying of possibly deep
        /// expression trees.
        void destroy();

        /// Comparison used for sorting; assumes TermTypes are in sort order.
        bool operator< (const Term& other) const
        { return (m_type < other.m_type); }

        /// Match type.  Can take an OR'ed combination of TermTypes.
        bool is_type(int type) const { return (m_type & type) != 0; }

        /// Get the type.
        TermType get_type() const { return m_type; }

        /// Match operator.  Does not match non-expressions.
        bool is_op(Op::Op op) const
        {
            Expr* e = get_expr();
            return (e && e->is_op(op));
        }

        // Helper functions to make it easier to get specific types.

        const Register* get_reg() const
        { return (m_type == REG ? m_reg : 0); }
        IntNum* get_int() const
        { return (m_type == INT ? m_intn : 0); }
        const unsigned int* get_subst() const
        { return (m_type == SUBST ? &m_subst : 0); }
        FloatNum* get_float() const
        { return (m_type == FLOAT ? m_flt : 0); }
        Symbol* get_sym() const
        { return (m_type == SYM ? m_sym : 0); }
        Bytecode* get_precbc() const
        { return (m_type == PRECBC ? m_precbc : 0); }
        Expr* get_expr() const
        { return (m_type == EXPR ? m_expr : 0); }

    private:
        TermType m_type;  ///< Type.
        /// Expression item data.  Correct value depends on type.
        union {
            const Register *m_reg;  ///< Register (#REG)
            IntNum *m_intn;         ///< Integer value (#INT)
            unsigned int m_subst;   ///< Subst placeholder (#SUBST)
            FloatNum *m_flt;        ///< Floating point value (#FLOAT)
            Symbol *m_sym;          ///< Symbol (#SYM)
            Bytecode *m_precbc;     ///< Direct bytecode ref (#PRECBC)
            Expr *m_expr;           ///< Subexpression (#EXPR)
        };
    };

    typedef std::vector<Term> Terms;

    Expr& operator= (const Expr& rhs);
    Expr(const Expr& e);
    ~Expr();

    /// Create a new expression e=a op b.
    /// @param a        expression a
    /// @param op       operation
    /// @param b        expression b
    /// @param line     virtual line (where expression defined)
    Expr(const Term& a, Op::Op op, const Term& b, unsigned long line=0);

    /// Create a new expression e=op a.
    /// @param o        operation
    /// @param a        expression a
    /// @param l        virtual line (where expression defined)
    Expr(Op::Op op, const Term& a, unsigned long line=0);

    /// Create a new expression identity e=a.
    /// @param a        identity within new expression
    /// @param line     line
    explicit Expr(const Term& a, unsigned long line=0);

    /// Determine if an expression is a specified operation (at the top
    /// level).
    /// @param op   operator
    /// @return True if the expression was the specified operation at the top
    ///         level.
    bool is_op(Op::Op op) const { return op == m_op; }

    /// Get operation of expression.
    /// @return Operator.
    Op::Op get_op() const { return m_op; }

    /// Level an entire expression tree.  Also expands EQUs.
    /// @param fold_const       enable constant folding if nonzero
    /// @param simplify_ident   simplify identities
    /// @param simplify_reg_mul simplify REG*1 identities
    /// @param xform_extra      extra transformation function
    void level_tree(bool fold_const,
                    bool simplify_ident,
                    bool simplify_reg_mul,
                    boost::function<void (Expr*)> xform_extra = 0);

    /// Simplify an expression as much as possible.  Eliminates extraneous
    /// branches and simplifies integer-only subexpressions.  Simplified
    /// version of level_tree().
    void simplify()
    { level_tree(true, true, true, 0); }

    /// Extract the segment portion of an expression containing SEG:OFF,
    /// leaving the offset.
    /// @return 0 if unable to extract a segment (expr does not contain a
    ///         OP_SEGOFF operator), otherwise the segment expression.
    ///         The input expression is modified such that on return, it's
    ///         the offset expression.
    /*@null@*/ std::auto_ptr<Expr> extract_deep_segoff();

    /// Extract the segment portion of a SEG:OFF expression, leaving the
    /// offset.
    /// @return 0 if unable to extract a segment (OP_SEGOFF not the
    ///         top-level operator), otherwise the segment expression.  The
    ///         input expression is modified such that on return, it's the
    ///         offset expression.
    /*@null@*/ std::auto_ptr<Expr> extract_segoff();

    /// Extract the right portion (y) of a x WRT y expression, leaving the
    /// left portion (x).
    /// @return 0 if unable to extract (OP_WRT not the top-level
    ///         operator), otherwise the right side of the WRT expression.
    ///         The input expression is modified such that on return, it's
    ///         the left side of the WRT expression.
    /*@null@*/ std::auto_ptr<Expr> extract_wrt();

    /// Get the float value of an expression if it's just a float.
    /// @return 0 if the expression is too complex; otherwise the float
    ///         value of the expression.
    /*@dependent@*/ /*@null@*/ FloatNum* get_float() const;

    /// Get the integer value of an expression if it's just an integer.
    /// @return 0 if the expression is too complex (contains anything other
    ///         than integers, ie floats, non-valued labels, registers);
    ///         otherwise the intnum value of the expression.
    /*@dependent@*/ /*@null@*/ IntNum* get_intnum() const;

    /// Get the symbol value of an expression if it's just a symbol.
    /// @return 0 if the expression is too complex; otherwise the symbol
    ///         value of the expression.
    /*@dependent@*/ /*@null@*/ Symbol* get_symbol() const;

    /// Get the register value of an expression if it's just a register.
    /// @return 0 if the expression is too complex; otherwise the register
    ///         value of the expression.
    /*@dependent@*/ /*@null@*/ const Register* get_reg() const;

    bool traverse_post(boost::function<bool (Expr*)> func);

    /// Traverse over expression tree in order, calling func for each leaf
    /// (non-operation).
    ///
    /// Stops early (and returns true) if func returns true.
    /// Otherwise returns false.
    bool traverse_leaves_in(boost::function<bool (const Term&)> func)
        const;

    /// Reorder terms of e into canonical order.  Only reorders if reordering
    /// doesn't change meaning of expression.  (eg, doesn't reorder SUB).
    /// Canonical order: REG, INT, FLOAT, SYM, EXPR.
    /// Multiple terms of a single type are kept in the same order as in
    /// the original expression.
    /// @note Only performs reordering on *one* level (no recursion).
    void order_terms();

    bool contains(int type) const;

    /// Substitute terms into expr SUBST terms (by index).  Terms
    /// are cloned.
    /// @param terms        terms
    /// @return True on error (index out of range).
    bool substitute(const Terms& terms);

    Expr* clone(int except = -1) const;

    unsigned long get_line() const { return m_line; }
    Terms& get_terms() { return m_terms; }

    /// Make expression an ident if it only has one term.
    /// This should be used if deep magic has been performed via get_terms().
    void make_ident() { if (m_terms.size() == 1) m_op = Op::IDENT; }

private:
    /// Operation.
    Op::Op m_op;

    /// Line number.
    unsigned long m_line;

    /// Some operations may allow more than two operand terms:
    /// ADD, MUL, OR, AND, XOR.
    Terms m_terms;

    void add_term(const Term& term);
    Expr(unsigned long line, Op::Op op);

    // Internal callbacks
    bool substitute_cb(const Terms& subst_terms);

    // Levelling functions
    void xform_neg_term(Terms::iterator term);
    void xform_neg_helper();
    void xform_neg();
    void simplify_identity(IntNum* &intn, bool simplify_reg_mul);
    void level_op(bool fold_const, bool simplify_ident, bool simplify_reg_mul);
    void expand_equ(std::vector<const Expr*> &seen);
    void level(bool fold_const,
               bool simplify_ident,
               bool simplify_reg_mul,
               boost::function<void (Expr*)> xform_extra);
};

std::ostream& operator<< (std::ostream& os, const Expr::Term& term);
std::ostream& operator<< (std::ostream& os, const Expr& e);

}   // namespace yasm

#endif
