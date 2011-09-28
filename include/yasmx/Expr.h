#ifndef YASM_EXPR_H
#define YASM_EXPR_H
///
/// @file
/// @brief Expression interface.
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
#include <assert.h>
#include <memory>

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/SmallVector.h"
#include "yasmx/Basic/SourceLocation.h"
#include "yasmx/Config/export.h"
#include "yasmx/DebugDumper.h"
#include "yasmx/IntNum.h"
#include "yasmx/Location.h"
#include "yasmx/Op.h"
#include "yasmx/SymbolRef.h"


namespace llvm { class APFloat; class raw_ostream; struct fltSemantics; }

namespace yasm
{

class Bytecode;
class Diagnostic;
class Expr;
class ExprTest;
class Register;
class Symbol;

/// An term inside an expression.
class YASM_LIB_EXPORT ExprTerm : public DebugDumper<ExprTerm>
{
public:
    /// Note loc must be used carefully (in a-b pairs), as only symrecs
    /// can become the relative term in a #Value.
    /// Testing uses bit comparison (&) so these have to be in bitmask form.
    enum Type
    {
        NONE = 0,       ///< Nothing (temporary placeholder only).
        REG = 1<<0,     ///< Register.
        INT = 1<<1,     ///< Integer (IntNum).
        SUBST = 1<<2,   ///< Substitution value.
        FLOAT = 1<<3,   ///< Float (APFloat).
        SYM = 1<<4,     ///< Symbol.
        LOC = 1<<5,     ///< Direct Location ref (rather than via symbol).
        OP = 1<<6       ///< Operator.
    };

    /// Substitution value.
    struct Subst
    {
        explicit Subst(unsigned int v) : subst(v) {}
        unsigned int subst;
    };

    ExprTerm() : m_type(NONE), m_depth(0) {}
    explicit ExprTerm(const Register& reg,
                      SourceLocation source = SourceLocation(),
                      int depth=0)
        : m_source(source), m_type(REG), m_depth(depth)
    {
        m_data.reg = &reg;
    }
    explicit ExprTerm(IntNum intn,
                      SourceLocation source = SourceLocation(),
                      int depth=0)
        : m_source(source), m_type(INT), m_depth(depth)
    {
        m_data.intn.m_type = IntNumData::INTNUM_SV;
        intn.swap(static_cast<IntNum&>(m_data.intn));
    }
    explicit ExprTerm(const Subst& subst,
                      SourceLocation source = SourceLocation(),
                      int depth=0)
        : m_source(source), m_type(SUBST), m_depth(depth)
    {
        m_data.subst = subst.subst;
    }
    explicit ExprTerm(SymbolRef sym,
                      SourceLocation source = SourceLocation(),
                      int depth=0)
        : m_source(source), m_type(SYM), m_depth(depth)
    {
        m_data.sym = sym;
    }
    explicit ExprTerm(Location loc,
                      SourceLocation source = SourceLocation(),
                      int depth=0)
        : m_source(source), m_type(LOC), m_depth(depth)
    {
        m_data.loc = loc;
    }
    // Depth must be explicit to avoid conflict with IntNum constructor.
    ExprTerm(Op::Op op, int nchild, SourceLocation source, int depth)
        : m_source(source), m_type(OP), m_depth(depth)
    {
        m_data.op.op = op;
        m_data.op.nchild = nchild;
    }

    // auto_ptr constructors

    ExprTerm(std::auto_ptr<IntNum> intn,
             SourceLocation source = SourceLocation(),
             int depth=0);
    ExprTerm(std::auto_ptr<llvm::APFloat> flt,
             SourceLocation source = SourceLocation(),
             int depth=0);

    /// Assignment operator.
    ExprTerm& operator= (const ExprTerm& rhs);

    /// Copy constructor.
    ExprTerm(const ExprTerm& term);

    /// Destructor.
    ~ExprTerm() { Clear(); }

    /// Exchanges this term with another term.
    /// @param oth      other term
    void swap(ExprTerm& oth);

    /// Clear the term.
    inline void Clear();

    /// Make the term zero.
    inline void Zero();

    /// Is term cleared?
    bool isEmpty() const { return (m_type == NONE); }

    /// Comparison used for sorting; assumes TermTypes are in sort order.
    bool operator< (const ExprTerm& other) const
    {
        return (m_type < other.m_type);
    }

    /// Match type.  Can take an OR'ed combination of TermTypes.
    bool isType(int type) const { return (m_type & type) != 0; }

    /// Get the type.
    Type getType() const { return m_type; }

    /// Set the source location.
    void setSource(SourceLocation source) { m_source = source; }

    /// Get the source location.
    SourceLocation getSource() const { return m_source; }

    /// Match operator.  Does not match non-operators.
    /// @param op       operator to match
    /// @return True if operator is op, false if non- or other operator.
    bool isOp(Op::Op op) const
    {
        return (m_type == OP && m_data.op.op == op);
    }

    /// Test if term is an operator.
    /// @return True if term is an operator, false otherwise.
    bool isOp() const { return (m_type == OP); }

    /// Change operator.  Term must already be an operator.
    /// Maintains existing depth.
    void setOp(Op::Op op)
    {
        assert(m_type == OP);
        m_data.op.op = op;
    }

    // Helper functions to make it easier to get specific types.

    const Register* getRegister() const
    {
        return (m_type == REG ? m_data.reg : 0);
    }

    const IntNum* getIntNum() const
    {
        return (m_type == INT ? static_cast<const IntNum*>(&m_data.intn) : 0);
    }

    IntNum* getIntNum()
    {
        return (m_type == INT ? static_cast<IntNum*>(&m_data.intn) : 0);
    }

    void setIntNum(IntNum intn);

    const unsigned int* getSubst() const
    {
        return (m_type == SUBST ? &m_data.subst : 0);
    }

    llvm::APFloat* getFloat() const
    {
        return (m_type == FLOAT ? m_data.flt : 0);
    }

    SymbolRef getSymbol() const
    {
        return SymbolRef(m_type == SYM ? m_data.sym : 0);
    }

    const Location* getLocation() const
    {
        return (m_type == LOC ? &m_data.loc : 0);
    }

    Location* getLocation()
    {
        return (m_type == LOC ? &m_data.loc : 0);
    }

    Op::Op getOp() const
    {
        return (m_type == OP ? m_data.op.op : Op::NONNUM);
    }

    int getNumChild() const
    {
        return (m_type == OP ? m_data.op.nchild : 0);
    }

    void AddNumChild(int delta)
    {
        assert(m_type == OP);
        m_data.op.nchild += delta;
    }

    /// Promote from int to float.  No-op if term is already float.
    /// @note Asserts if term is not int or float.
    /// @param semantics    float semantics to use for converted value
    void PromoteToFloat(const llvm::fltSemantics& semantics);

#ifdef WITH_XML
    /// Write an XML representation.  For debugging purposes.
    /// @param out          XML node
    /// @return Root node.
    pugi::xml_node Write(pugi::xml_node out) const;
#endif // WITH_XML

    /// Print to stream.
    /// @param os           output stream
    /// @param base         numeric base (10=decimal, etc)
    void Print(llvm::raw_ostream& os, int base=10) const;

private:
    /// Expression item data.  Correct value depends on type.
    union Data
    {
        const Register *reg;    ///< Register (#REG)
        IntNumData intn;        ///< Integer value (#INT)
        unsigned int subst;     ///< Subst placeholder (#SUBST)
        llvm::APFloat *flt;     ///< Floating point value (#FLOAT)
        Symbol *sym;            ///< Symbol (#SYM)
        Location loc;           ///< Direct bytecode ref (#LOC)
        struct
        {
            Op::Op op;          ///< Operator
            int nchild;         ///< Number of children
        }
        op;                     ///< Operator (#OP)
    } m_data;                   ///< Data.
    SourceLocation m_source;    ///< Source location.
    Type m_type;                ///< Type.

public:
    int m_depth;                ///< Depth in tree.
};

typedef llvm::SmallVector<ExprTerm, 3> ExprTerms;

inline ExprTerm&
ExprTerm::operator= (const ExprTerm& rhs)
{
    if (this != &rhs)
        ExprTerm(rhs).swap(*this);
    return *this;
}

inline void
ExprTerm::Clear()
{
    if (m_type == INT)
        static_cast<IntNum&>(m_data.intn).~IntNum();
    else if (m_type == FLOAT)
        delete m_data.flt;
    m_type = NONE;
}

inline void
ExprTerm::Zero()
{
    Clear();
    m_type = INT;
    m_data.intn = IntNum(0);
}

inline void
ExprTerm::setIntNum(IntNum intn)
{
    Clear();
    m_type = INT;
    m_data.intn.m_type = IntNumData::INTNUM_SV;
    intn.swap(static_cast<IntNum&>(m_data.intn));
}

/// An expression.
///
/// Expressions are n-ary trees.  Most operators are either unary or binary,
/// but associative operators such as Op::ADD and Op::MUL may have more than
/// two children.
///
/// Expressions are stored as a vector of terms (ExprTerm) in
/// reverse polish notation (highest operator last).  Each term may be an
/// operator or a value (such as an integer or register) and has an
/// associated depth.  Operator terms also keep track of the number of
/// immediate children they have.
///
/// Examples of expressions stored in vector format:
///
/// <pre>
/// Infix: (a+b)*c
/// Index [0] [1] [2] [3] [4]
/// Depth  2   2   1   1   0
/// Data   a   b   +   c   *
///
/// Infix: (a+b+c)+d+(e*f)
/// Index [0] [1] [2] [3] [4] [5] [6] [7] [8]
/// Depth  2   2   2   1   1   2   2   1   0
/// Data   a   b   c   +   d   e   f   *   +
///
/// Infix: a
/// Index [0]
/// Depth  0
/// Data   a
///
/// Infix: a+(((b+c)*d)-e)+f
/// Index [0] [1] [2] [3] [4] [5] [6] [7] [8] [9]
/// Depth  1   4   4   3   3   2   2   1   1   0
/// Data   a   b   c   +   d   *   e   -   f   +
/// </pre>
///
/// General Expr usage does not need to be aware of this internal data format,
/// but it is key to doing advanced expression manipulation.  Due to the
/// RPN storage style, most processing occurs going from back-to-front within
/// the terms vector.
class YASM_LIB_EXPORT Expr : public DebugDumper<Expr>
{
    friend class ExprTest;

public:
    typedef std::auto_ptr<Expr> Ptr;

    /// Empty constructor.
    Expr() {}

    /// Assign to expression.
    /// @param term     expression value
    template <typename T>
    Expr& operator= (const T& term);

    /// Assign to expression.
    /// @param term     expression value
    template <typename T>
    Expr& operator= (T& term);

    Expr& operator= (const Expr& e);

    /// Copy constructor.
    /// @param e        expression to copy from
    Expr(const Expr& e);

    /// Single-term constructor for register.
    explicit Expr(const Register& reg, SourceLocation source = SourceLocation())
    { m_terms.push_back(ExprTerm(reg, source)); }

    /// Single-term constructor for integer.
    explicit Expr(IntNum intn, SourceLocation source = SourceLocation())
    { m_terms.push_back(ExprTerm(intn, source)); }

    /// Single-term constructor for symbol.
    explicit Expr(SymbolRef sym, SourceLocation source = SourceLocation())
    { m_terms.push_back(ExprTerm(sym, source)); }

    /// Single-term constructor for location.
    explicit Expr(Location loc, SourceLocation source = SourceLocation())
    { m_terms.push_back(ExprTerm(loc, source)); }

    /// Single-term constructor for IntNum auto_ptr.
    explicit Expr(std::auto_ptr<IntNum> intn,
                  SourceLocation source = SourceLocation());

    /// Single-term constructor for APFloat auto_ptr.
    explicit Expr(std::auto_ptr<llvm::APFloat> flt,
                  SourceLocation source = SourceLocation());

    /// Destructor.
    ~Expr();

    /// Determine if an expression is a specified operation (at the top
    /// level).
    /// @param op   operator
    /// @return True if the expression was the specified operation at the top
    ///         level.
    bool isOp(Op::Op op) const
    { return !isEmpty() && m_terms.back().isOp(op); }

    /// Exchanges this expression with another expression.
    /// @param oth      other expression
    void swap(Expr& oth);

    /// Get an allocated copy.
    /// @return Newly allocated copy of expression.
    Expr* clone() const { return new Expr(*this); }

    /// Clear the expression.
    void Clear();

    /// Is expression empty?
    bool isEmpty() const { return m_terms.empty(); }

    /// Simplify an expression as much as possible.  Eliminates extraneous
    /// branches and simplifies integer-only subexpressions.  Does *not*
    /// expand EQUs; use ExpandEqu() in expr_util.h to first expand EQUs.
    /// @param simplify_reg_mul simplify REG*1 identities
    void Simplify(Diagnostic& diags, bool simplify_reg_mul = true);

    /// Simplify an expression as much as possible, taking a functor for
    /// additional processing.  Calls LevelOp() both before and after the
    /// functor in post-order.  Functor is only called on operator terms.
    /// @param func             functor to call on each operator, bottom-up
    ///                         called as (Expr&, int pos)
    /// @param simplify_reg_mul simplify REG*1 identities
    template <typename T>
    void Simplify(Diagnostic& diags,
                  const T& func,
                  bool simplify_reg_mul = true);

    /// Extract the segment portion of an expression containing SEG:OFF,
    /// leaving the offset.
    /// @return Empty expression if unable to extract a segment (expr does not
    ///         contain an Op::SEGOFF operator), otherwise the segment
    ///         expression.
    ///         The input expression is modified such that on return, it's
    ///         the offset expression.
    Expr ExtractDeepSegOff();

    /// Extract the segment portion of a SEG:OFF expression, leaving the
    /// offset.
    /// @return Empty expression if unable to extract a segment (OP::SEGOFF
    ///         not the top-level operator), otherwise the segment expression.
    ///         The input expression is modified such that on return, it's the
    ///         offset expression.
    Expr ExtractSegOff();

    /// Extract the right portion (y) of a x WRT y expression, leaving the
    /// left portion (x).
    /// @return Empty expression if unable to extract (OP::WRT not the
    ///         top-level operator), otherwise the right side of the WRT
    ///         expression.
    ///         The input expression is modified such that on return, it's
    ///         the left side of the WRT expression.
    Expr ExtractWRT();

    /// Determine if an expression is just a float.
    /// @return True if getFloat() is safe to call.
    bool isFloat() const
    {
        return (m_terms.size() == 1
                && m_terms.front().isType(ExprTerm::FLOAT));
    }

    /// Get the float value of an expression if it's just a float.
    /// Asserts if the expression is too complex.
    /// @return The float value of the expression.
    /*@dependent@*/ /*@null@*/ llvm::APFloat* getFloat() const;

    /// Determine if an expression is just an integer.
    /// Returns false if the expression is too complex (contains anything other
    /// than integers, eg floats, non-valued labels, or registers).
    /// @return True if getIntNum() is safe to call.
    bool isIntNum() const
    { return (m_terms.size() == 1 && m_terms.front().isType(ExprTerm::INT)); }

    /// Get the integer value of an expression if it's just an integer.
    /// Asserts if expression is not an integer.
    /// @return The intnum value of the expression.
    IntNum getIntNum() const;

    /// Determine if an expression is just a symbol.
    /// @return True if getSymbol() is safe to call.
    bool isSymbol() const
    { return (m_terms.size() == 1 && m_terms.front().isType(ExprTerm::SYM)); }

    /// Get the symbol value of an expression if it's just a symbol.
    /// Asserts if the expression is too complex.
    /// @return The symbol value of the expression.
    SymbolRef getSymbol() const;

    /// Determine if an expression is just a symbol.
    /// @return True if getRegister() is safe to call.
    bool isRegister() const
    { return (m_terms.size() == 1 && m_terms.front().isType(ExprTerm::REG)); }

    /// Get the register value of an expression if it's just a register.
    /// Asserts if the expression is too complex.
    /// @return The register value of the expression.
    /*@dependent@*/ const Register* getRegister() const;

    bool Contains(int type, int pos=-1) const;

    /// Substitute terms into expr SUBST terms (by index).
    /// @param begin        beginning of terms
    /// @param end          end of terms
    /// @return False on error (index out of range).
    bool Substitute(const ExprTerm* begin, const ExprTerm* end);

    void Calc(Op::Op op, SourceLocation source = SourceLocation());

    template <typename T>
    void Calc(Op::Op op,
              const T& rhs,
              SourceLocation source = SourceLocation());

    void Calc(Op::Op op,
              const Expr& rhs,
              SourceLocation source = SourceLocation());

    /// @defgroup lowlevel Low Level Manipulators
    /// Functions to manipulate the innards of the expression terms.
    /// Use with caution.
    ///@{

    /// Get raw expression terms.
    /// @return Terms reference.
    ExprTerms& getTerms() { return m_terms; }

    /// Get raw expression terms (const version).
    /// @return Const Terms reference.
    const ExprTerms& getTerms() const { return m_terms; }

    /// Append an expression term to terms.
    /// @param term     expression term
    template <typename T>
    void Append(const T& term)
    { m_terms.push_back(ExprTerm(term)); }

    /// Append an operator to terms.  Pushes down all current terms and adds
    /// operator term to end.
    /// @param op       operator
    /// @param nchild   number of children
    void AppendOp(Op::Op op,
                  int nchild,
                  SourceLocation source = SourceLocation());

    /// Make expression an ident if it only has one term.
    /// @param pos      index of operator term, may be negative for "from end"
    void MakeIdent(Diagnostic& diags, int pos=-1);

    /// Levels an expression tree.
    /// a+(b+c) -> a+b+c
    /// (a+b)+(c+d) -> a+b+c+d
    /// Only levels operators that allow more than two operand terms.
    /// Folds (combines by evaluation) *integer* constant values.
    /// @note Only does *one* level of leveling.  Should be called
    ///       post-order on a tree to combine deeper levels.
    /// @param simplify_reg_mul simplify REG*1 identities
    /// @param pos              index of top-level operator term
    void LevelOp(Diagnostic& diags, bool simplify_reg_mul, int pos=-1);

    //@}

#ifdef WITH_XML
    /// Write an XML representation.  For debugging purposes.
    /// @param out          XML node
    /// @return Root node.
    pugi::xml_node Write(pugi::xml_node out) const;
#endif // WITH_XML

    /// Print to stream.
    /// @param os           output stream
    /// @param base         numeric base (10=decimal, etc)
    void Print(llvm::raw_ostream& os, int base=10) const;

    /// Clean up terms by removing all empty (ExprTerm::NONE) elements.
    void Cleanup();

private:
    /// Terms of the expression.  The entire tree is stored here.
    ExprTerms m_terms;

    /// Reduce depth of a subexpression.
    /// @param pos      term index of subexpression operator
    /// @param delta    delta to reduce depth by
    void ReduceDepth(int pos, int delta=1);

    /// Clear all terms of a subexpression, possibly keeping a single term.
    /// @param pos      term index of subexpression operator
    /// @param keep     term index of term to keep; -1 to clear all terms
    void ClearExcept(int pos, int keep=-1);

    /// Transform all Op::SUB and Op::NEG subexpressions into appropriate *-1
    /// variants.  This assists with operator leveling as it transforms the
    /// nonlevelable Op::SUB into the levelable Op::ADD.
    void TransformNeg();

    /// LHS expression extractor.
    /// @param op   reverse iterator pointing at operator term to be extracted
    ///             from
    Expr ExtractLHS(ExprTerms::reverse_iterator op);
};

/// Assign to expression.
/// @param term     expression value
template <typename T>
inline Expr&
Expr::operator= (const T& term)
{
    m_terms.clear();
    m_terms.push_back(ExprTerm(term));
    return *this;
}

/// Assign to expression.
/// @param term     expression value
template <typename T>
inline Expr&
Expr::operator= (T& term)
{
    m_terms.clear();
    m_terms.push_back(ExprTerm(term));
    return *this;
}

/// Assign an expression.
/// @param e        expression
inline Expr&
Expr::operator= (const Expr& rhs)
{
    if (this != &rhs)
        Expr(rhs).swap(*this);
    return *this;
}

/// Assign an expression.
/// @param e        expression
template <>
inline Expr&
Expr::operator= (Expr& rhs)
{
    if (this != &rhs)
        Expr(rhs).swap(*this);
    return *this;
}

/// Assign an expression term.
/// @param term     expression term
template <>
inline Expr&
Expr::operator= (const ExprTerm& term)
{
    m_terms.clear();
    m_terms.push_back(term);
    return *this;
}

/// Assign an expression term.
/// @param term     expression term
template <>
inline Expr&
Expr::operator= (ExprTerm& term)
{
    m_terms.clear();
    m_terms.push_back(term);
    return *this;
}

/// Append an expression to terms.
/// @param e        expression
template <>
inline void Expr::Append(const Expr& e)
{ m_terms.insert(m_terms.end(), e.m_terms.begin(), e.m_terms.end()); }

/// Append an expression term to terms.
/// @param term     expression term
template <>
inline void Expr::Append(const ExprTerm& term)
{ m_terms.push_back(term); }

template <typename T>
void
Expr::Simplify(Diagnostic& diags, const T& func, bool simplify_reg_mul)
{
    TransformNeg();

    // Must re-call size() in conditional as it may change during execution.
    for (int pos = 0; pos < static_cast<int>(m_terms.size()); ++pos)
    {
        if (!m_terms[pos].isOp())
            continue;
        LevelOp(diags, simplify_reg_mul, pos);

        if (!m_terms[pos].isOp())
            continue;
        func(*this, pos);

        if (!m_terms[pos].isOp())
            continue;
        LevelOp(diags, simplify_reg_mul, pos);
    }

    Cleanup();
}

inline void
Expr::Calc(Op::Op op, SourceLocation source)
{
    if (!isEmpty())
        AppendOp(op, 1, source);
}

template <typename T>
inline void
Expr::Calc(Op::Op op, const T& rhs, SourceLocation source)
{
    bool was_empty = isEmpty();
    Append(rhs);
    if (!was_empty)
        AppendOp(op, 2, source);
}

inline void
Expr::Calc(Op::Op op, const Expr& rhs, SourceLocation source)
{
    if (rhs.isEmpty())
        return;
    bool was_empty = isEmpty();
    Append(rhs);
    if (!was_empty)
        AppendOp(op, 2, source);
}


/// Expression builder based on operator.
/// Allows building expressions with the syntax Expr e = ADD(0, sym, ...);
/// @note Source locations cannot be specified with this form.
struct ExprBuilder
{
    Op::Op op;

    template <typename T1>
    Expr operator() (const T1& t1) const
    {
        Expr e;
        e.Append(t1);
        e.AppendOp(op, 1);
        return e;
    }

    template <typename T1, typename T2>
    Expr operator() (const T1& t1, const T2& t2) const
    {
        Expr e;
        e.Append(t1);
        e.Append(t2);
        e.AppendOp(op, 2);
        return e;
    }

    template <typename T1, typename T2, typename T3>
    Expr operator() (const T1& t1, const T2& t2, const T3& t3) const
    {
        Expr e;
        e.Append(t1);
        e.Append(t2);
        e.Append(t3);
        e.AppendOp(op, 3);
        return e;
    }

    template <typename T1, typename T2, typename T3, typename T4>
    Expr operator() (const T1& t1, const T2& t2, const T3& t3,
                     const T4& t4) const
    {
        Expr e;
        e.Append(t1);
        e.Append(t2);
        e.Append(t3);
        e.Append(t4);
        e.AppendOp(op, 4);
        return e;
    }

    template <typename T1, typename T2, typename T3, typename T4, typename T5>
    Expr operator() (const T1& t1, const T2& t2, const T3& t3, const T4& t4,
                     const T5& t5) const
    {
        Expr e;
        e.Append(t1);
        e.Append(t2);
        e.Append(t3);
        e.Append(t4);
        e.Append(t5);
        e.AppendOp(op, 5);
        return e;
    }

    template <typename T1, typename T2, typename T3, typename T4, typename T5,
              typename T6>
    Expr operator() (const T1& t1, const T2& t2, const T3& t3, const T4& t4,
                     const T5& t5, const T6& t6) const
    {
        Expr e;
        e.Append(t1);
        e.Append(t2);
        e.Append(t3);
        e.Append(t4);
        e.Append(t5);
        e.Append(t6);
        e.AppendOp(op, 6);
        return e;
    }
};

/// Specialization of ExprBuilder to allow terms to be passed.
/// This allows e.g. ADD(terms).
template <>
inline Expr ExprBuilder::operator() (const ExprTerms& terms) const
{
    Expr e;
    for (ExprTerms::const_iterator i=terms.begin(), end=terms.end(); i != end;
         ++i)
        e.Append(*i);
    e.AppendOp(op, static_cast<int>(terms.size()));
    return e;
}

extern YASM_LIB_EXPORT const ExprBuilder ADD;        ///< Addition (+).
extern YASM_LIB_EXPORT const ExprBuilder SUB;        ///< Subtraction (-).
extern YASM_LIB_EXPORT const ExprBuilder MUL;        ///< Multiplication (*).
extern YASM_LIB_EXPORT const ExprBuilder DIV;        ///< Unsigned division.
extern YASM_LIB_EXPORT const ExprBuilder SIGNDIV;    ///< Signed division.
extern YASM_LIB_EXPORT const ExprBuilder MOD;        ///< Unsigned modulus.
extern YASM_LIB_EXPORT const ExprBuilder SIGNMOD;    ///< Signed modulus.
extern YASM_LIB_EXPORT const ExprBuilder NEG;        ///< Negation (-).
extern YASM_LIB_EXPORT const ExprBuilder NOT;        ///< Bitwise negation.
extern YASM_LIB_EXPORT const ExprBuilder OR;         ///< Bitwise OR.
extern YASM_LIB_EXPORT const ExprBuilder AND;        ///< Bitwise AND.
extern YASM_LIB_EXPORT const ExprBuilder XOR;        ///< Bitwise XOR.
extern YASM_LIB_EXPORT const ExprBuilder XNOR;       ///< Bitwise XNOR.
extern YASM_LIB_EXPORT const ExprBuilder NOR;        ///< Bitwise NOR.
extern YASM_LIB_EXPORT const ExprBuilder SHL;        ///< Shift left (logical).
extern YASM_LIB_EXPORT const ExprBuilder SHR;        ///< Shift right (logical).
extern YASM_LIB_EXPORT const ExprBuilder LOR;        ///< Logical OR.
extern YASM_LIB_EXPORT const ExprBuilder LAND;       ///< Logical AND.
extern YASM_LIB_EXPORT const ExprBuilder LNOT;       ///< Logical negation.
extern YASM_LIB_EXPORT const ExprBuilder LXOR;       ///< Logical XOR.
extern YASM_LIB_EXPORT const ExprBuilder LXNOR;      ///< Logical XNOR.
extern YASM_LIB_EXPORT const ExprBuilder LNOR;       ///< Logical NOR.
extern YASM_LIB_EXPORT const ExprBuilder LT;         ///< Less than comparison.
extern YASM_LIB_EXPORT const ExprBuilder GT;         ///< Greater than.
extern YASM_LIB_EXPORT const ExprBuilder EQ;         ///< Equality comparison.
extern YASM_LIB_EXPORT const ExprBuilder LE;         ///< Less than or equal to.
extern YASM_LIB_EXPORT const ExprBuilder GE;         ///< Greater than or equal.
extern YASM_LIB_EXPORT const ExprBuilder NE;         ///< Not equal comparison.
/// SEG operator (gets segment portion of address).
extern YASM_LIB_EXPORT const ExprBuilder SEG;
/// WRT operator (gets offset of address relative to some other
/// segment).
extern YASM_LIB_EXPORT const ExprBuilder WRT;
/// The ':' in segment:offset.
extern YASM_LIB_EXPORT const ExprBuilder SEGOFF;

/// Overloaded assignment binary operators.
template <typename T> inline Expr& operator+=(Expr& lhs, const T& rhs)
{ lhs.Calc(Op::ADD, rhs); return lhs; }
template <typename T> inline Expr& operator-=(Expr& lhs, const T& rhs)
{ lhs.Calc(Op::SUB, rhs); return lhs; }
template <typename T> inline Expr& operator*=(Expr& lhs, const T& rhs)
{ lhs.Calc(Op::MUL, rhs); return lhs; }
template <typename T> inline Expr& operator/=(Expr& lhs, const T& rhs)
{ lhs.Calc(Op::DIV, rhs); return lhs; }
template <typename T> inline Expr& operator%=(Expr& lhs, const T& rhs)
{ lhs.Calc(Op::MOD, rhs); return lhs; }
template <typename T> inline Expr& operator^=(Expr& lhs, const T& rhs)
{ lhs.Calc(Op::XOR, rhs); return lhs; }
template <typename T> inline Expr& operator&=(Expr& lhs, const T& rhs)
{ lhs.Calc(Op::AND, rhs); return lhs; }
template <typename T> inline Expr& operator|=(Expr& lhs, const T& rhs)
{ lhs.Calc(Op::OR, rhs); return lhs; }
template <typename T> inline Expr& operator>>=(Expr& lhs, const T& rhs)
{ lhs.Calc(Op::SHR, rhs); return lhs; }
template <typename T> inline Expr& operator<<=(Expr& lhs, const T& rhs)
{ lhs.Calc(Op::SHL, rhs); return lhs; }

inline llvm::raw_ostream&
operator<< (llvm::raw_ostream& os, const ExprTerm& term)
{
    term.Print(os);
    return os;
}

inline llvm::raw_ostream&
operator<< (llvm::raw_ostream& os, const Expr& e)
{
    e.Print(os);
    return os;
}

/// Perform a floating point calculation based on an #Op operator.
/// @param lhs      left hand side of expression (also modified as result)
/// @param op       operator
/// @param rhs      right hand side of expression
/// @param source   operator source location
/// @param diags    diagnostic reporting
/// @return False if an error occurred.
YASM_LIB_EXPORT
bool CalcFloat(llvm::APFloat* lhs,
               Op::Op op,
               const llvm::APFloat& rhs,
               SourceLocation source,
               Diagnostic& diags);

/// Get left and right hand immediate children, or single immediate child.
/// @param e        Expression
/// @param lhs      Term index of left hand side (output)
/// @param rhs      Term index of right hand side (output)
/// @param pos      Term index of operator (root), both input and output.
/// Pos is updated before return with the term index following the tree.
/// For single children, pass lhs=NULL and the rhs output will be the single
/// child.  Passed-in pos may be negative to indicate index "from end".
/// @return False if too many or too few children found.
YASM_LIB_EXPORT
bool getChildren(Expr& e, /*@out@*/ int* lhs, /*@out@*/ int* rhs, int* pos);

/// Determine if a expression subtree is of the form Symbol*-1.
/// @param e        Expression
/// @param sym      Term index of symbol term (output)
/// @param neg1     Term index of -1 term (output)
/// @param pos      Term index of operator (root), both input and output.
/// @param loc_ok   If true, Location*-1 is also accepted
/// @return True if subtree matches.
/// If the subtree matches, pos is updated before return with the term index
/// following the tree.
YASM_LIB_EXPORT
bool isNeg1Sym(Expr& e,
               /*@out@*/ int* sym,
               /*@out@*/ int* neg1,
               int* pos,
               bool loc_ok);

/// Specialized swap for Expr.
inline void
swap(Expr& left, Expr& right)
{
    left.swap(right);
}

/// Specialized swap for ExprTerm.
inline void
swap(ExprTerm& left, ExprTerm& right)
{
    left.swap(right);
}

} // namespace yasm

namespace std
{

/// Specialized std::swap for Expr.
template <>
inline void
swap(yasm::Expr& left, yasm::Expr& right)
{
    left.swap(right);
}

/// Specialized std::swap for ExprTerm.
template <>
inline void
swap(yasm::ExprTerm& left, yasm::ExprTerm& right)
{
    left.swap(right);
}

} // namespace std

#endif
