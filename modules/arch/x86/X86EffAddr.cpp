//
// x86 effective address handling
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
#include "X86EffAddr.h"

#include "llvm/ADT/Twine.h"
#include "yasmx/Basic/Diagnostic.h"
#include "yasmx/Config/functional.h"
#include "yasmx/Expr.h"
#include "yasmx/Expr_util.h"
#include "yasmx/IntNum.h"

#include "X86Register.h"


using namespace yasm;
using namespace yasm::arch;

bool
arch::setRexFromReg(unsigned char* rex,
                    unsigned char* low3,
                    X86Register::Type reg_type,
                    unsigned int reg_num,
                    unsigned int bits,
                    X86RexBitPos rexbit)
{
    *low3 = reg_num&7;

    if (bits == 64)
    {
        if (reg_type == X86Register::REG8X || reg_num >= 8)
        {
            // Check to make sure we can set it
            if (*rex == 0xff)
                return false;
            *rex |= 0x40 | (((reg_num & 8) >> 3) << rexbit);
        }
        else if (reg_type == X86Register::REG8 && (reg_num & 7) >= 4)
        {
            // AH/BH/CH/DH, so no REX allowed
            if (*rex != 0 && *rex != 0xff)
                return false;
            *rex = 0xff;    // Flag so we can NEVER set it (see above)
        }
    }
    return true;
}

void
X86EffAddr::Init(unsigned int spare)
{
    m_modrm &= 0xC7;                  // zero spare/reg bits
    m_modrm |= (spare << 3) & 0x38;   // plug in provided bits
}

void
X86EffAddr::setDispOnly()
{
    m_valid_modrm = 0;
    m_need_modrm = 0;
    m_valid_sib = 0;
    m_need_sib = 0;
}

X86EffAddr::X86EffAddr()
    : EffAddr(std::auto_ptr<Expr>(0)),
      m_modrm(0),
      m_sib(0),
      m_need_sib(0),
      m_vsib_mode(0),
      m_valid_modrm(false),
      m_need_modrm(false),
      m_valid_sib(false)
{
}

X86EffAddr::X86EffAddr(const X86EffAddr& rhs)
    : EffAddr(rhs),
      m_modrm(rhs.m_modrm),
      m_sib(rhs.m_sib),
      m_need_sib(rhs.m_need_sib),
      m_vsib_mode(rhs.m_vsib_mode),
      m_valid_modrm(rhs.m_valid_modrm),
      m_need_modrm(rhs.m_need_modrm),
      m_valid_sib(rhs.m_valid_sib)
{
}

bool
X86EffAddr::setReg(const X86Register* reg, unsigned char* rex,
                   unsigned int bits)
{
    unsigned char rm;

    if (!setRexFromReg(rex, &rm, reg, bits, X86_REX_B))
        return false;

    m_modrm = 0xC0 | rm;    // Mod=11, R/M=Reg, Reg=0
    m_valid_modrm = true;
    m_need_modrm = true;
    return true;
}

static std::auto_ptr<Expr>
Fixup(bool xform_rip_plus, std::auto_ptr<Expr> e)
{
    if (!xform_rip_plus || !e->isOp(Op::ADD))
        return e;

    // Look for foo+rip or rip+foo.
    int pos = -1;
    int lhs, rhs;
    if (!getChildren(*e, &lhs, &rhs, &pos))
        return e;

    ExprTerms& terms = e->getTerms();
    int regterm;
    if (terms[lhs].isType(ExprTerm::REG))
        regterm = lhs;
    else if (terms[rhs].isType(ExprTerm::REG))
        regterm = rhs;
    else
        return e;

    const X86Register* reg =
        static_cast<const X86Register*>(terms[regterm].getRegister());
    if (reg->isNot(X86Register::RIP))
        return e;

    // replace register with 0
    terms[regterm].Zero();

    // build new wrt expression
    e->Append(*reg);
    e->AppendOp(Op::WRT, 2);

    return e;
}

X86EffAddr::X86EffAddr(bool xform_rip_plus, std::auto_ptr<Expr> e)
    : EffAddr(Fixup(xform_rip_plus, e)),
      m_modrm(0),
      m_sib(0),
      // We won't know whether we need an SIB until we know more about expr
      // and the BITS/address override setting.
      m_need_sib(0xff),
      m_vsib_mode(0),
      m_valid_modrm(false),
      m_need_modrm(true),
      m_valid_sib(false)
{
    m_need_disp = true;
}

void
X86EffAddr::setImm(std::auto_ptr<Expr> imm, unsigned int im_len)
{
    m_disp = Value(im_len, imm);
    m_need_disp = true;
}

X86EffAddr*
X86EffAddr::clone() const
{
    return new X86EffAddr(*this);
}

#ifdef WITH_XML
pugi::xml_node
X86EffAddr::DoWrite(pugi::xml_node out) const
{
    pugi::xml_node root = out.append_child("X86EffAddr");
    pugi::xml_node modrm = root.append_child("ModRM");
    append_data(modrm, Twine::utohexstr(m_modrm).str());
    modrm.append_attribute("need") = static_cast<bool>(m_need_modrm);
    modrm.append_attribute("valid") = static_cast<bool>(m_valid_modrm);

    pugi::xml_node sib = root.append_child("SIB");
    append_data(sib, Twine::utohexstr(m_sib).str());
    sib.append_attribute("need") = static_cast<bool>(m_need_sib);
    sib.append_attribute("valid") = static_cast<bool>(m_valid_sib);
    sib.append_attribute("vsibmode") =
        (Twine::utohexstr(m_vsib_mode).str()).c_str();
    return root;
}
#endif // WITH_XML

namespace {
class X86EAChecker
{
public:
    X86EAChecker(unsigned int bits,
                 unsigned int addrsize,
                 unsigned int vsib_mode,
                 DiagnosticsEngine& diags)
        : m_bits(bits)
        , m_addrsize(addrsize)
        , m_vsib_mode(vsib_mode)
        , m_diags(diags)
    {
        // Normally don't check SIMD regs
        m_regcount = 17;
        if (vsib_mode != 0)
            m_regcount = 33;
        for (int i=0; i<m_regcount; ++i)
            m_regmult[i] = 0;
    }

    int GetRegUsage(Expr& e, /*@null@*/ int* indexreg, bool* ip_rel);

    enum RegIndex
    {
        kREG_NONE = -1,
        kREG_RAX = 0,   kREG_RCX,   kREG_RDX,   kREG_RBX,
        kREG_RSP,       kREG_RBP,   kREG_RSI,   kREG_RDI,
        kREG_R8,        kREG_R9,    kREG_R10,   kREG_R11,
        kREG_R12,       kREG_R13,   kREG_R14,   kREG_R15,
        kREG_RIP,       kSIMDREGS
    };
    int m_regcount;
    int m_regmult[33];

private:
    bool GetTermRegUsage(Expr& e,
                         int pos,
                         /*@null@*/ int* indexreg,
                         int* indexval,
                         bool* indexmult);
    bool getReg(ExprTerm& term, int* regnum);

private:
    unsigned int m_bits;
    unsigned int m_addrsize;
    unsigned int m_vsib_mode;
    DiagnosticsEngine& m_diags;
};

class X86DistRegFunctor
{
public:
    X86DistRegFunctor(bool simplify_reg_mul, DiagnosticsEngine& diags)
        : m_simplify_reg_mul(simplify_reg_mul), m_diags(diags)
    {}

    void operator() (Expr& e, int& pos);

private:
    bool m_simplify_reg_mul;
    DiagnosticsEngine& m_diags;
};
} // anonymous namespace

// Only works if term.type == Expr::REG (doesn't check).
// Overwrites term with intnum of 0 (to eliminate regs from the final expr).
bool
X86EAChecker::getReg(ExprTerm& term, int* regnum)
{
    const X86Register* reg =
        static_cast<const X86Register*>(term.getRegister());
    assert(reg != 0);
    unsigned int myregnum = reg->getNum();

    switch (reg->getType())
    {
        case X86Register::REG16:
            if (m_addrsize != 16)
                return false;
            *regnum = myregnum;
            // only allow BX, SI, DI, BP
            if (myregnum != kREG_RBX && myregnum != kREG_RSI
                && myregnum != kREG_RDI && myregnum != kREG_RBP)
                return false;
            break;
        case X86Register::REG32:
            if (m_addrsize != 32)
                return false;
            *regnum = myregnum;
            break;
        case X86Register::REG64:
            if (m_addrsize != 64)
                return false;
            *regnum = myregnum;
            break;
        case X86Register::XMMREG:
            if (m_vsib_mode != 1)
                return false;
            if (m_bits != 64 && myregnum > 7)
                return false;
            *regnum = 17+myregnum;
            break;
        case X86Register::YMMREG:
            if (m_vsib_mode != 2)
                return false;
            if (m_bits != 64 && myregnum > 7)
                return false;
            *regnum = 17+myregnum;
            break;
        case X86Register::RIP:
            if (m_bits != 64)
                return false;
            *regnum = 16;
            break;
        default:
            return false;
    }

    // overwrite with 0 to eliminate register from displacement expr
    term.Zero();

    // we're okay
    assert(myregnum < static_cast<unsigned int>(m_regcount)
           && "register number too large");
    return true;
}

// Distribute over registers to help bring them to the topmost level of e.
// Also check for illegal operations against registers.
// Returns 0 if something was illegal, 1 if legal and nothing in e changed,
// and 2 if legal and e needs to be simplified.
//
// Only half joking: Someday make this/checkea able to accept crazy things
//  like: (bx+di)*(bx+di)-bx*bx-2*bx*di-di*di+di?  Probably not: NASM never
//  accepted such things, and it's doubtful such an expn is valid anyway
//  (even though the above one is).  But even macros would be hard-pressed
//  to generate something like this.
//
// e must already have been simplified for this function to work properly
// (as it doesn't think things like SUB are valid).
//
// IMPLEMENTATION NOTE: About the only thing this function really needs to
// "distribute" is: (non-float-expn or intnum) * (sum expn of registers).
//
// XXX: pos is taken by reference so we can update it.  This is somewhat
//      underhanded.
void
X86DistRegFunctor::operator() (Expr& e, int& pos)
{
    ExprTerms& terms = e.getTerms();
    ExprTerm& root = terms[pos];

    // The *only* case we need to distribute is INT*(REG+...)
    if (!root.isOp(Op::MUL))
        return;

    int throwaway = pos;
    int lhs, rhs;
    if (!getChildren(e, &lhs, &rhs, &throwaway))
        return;

    int intpos, otherpos;
    if (terms[lhs].isType(ExprTerm::INT))
    {
        intpos = lhs;
        otherpos = rhs;
    }
    else if (terms[rhs].isType(ExprTerm::INT))
    {
        intpos = rhs;
        otherpos = lhs;
    }
    else
        return; // no integer

    if (!terms[otherpos].isOp(Op::ADD) ||
        !e.Contains(ExprTerm::REG, otherpos))
        return; // not an additive REG-containing term

    // We know we have INT*(REG+...); distribute it.

    // Grab integer multiplier and delete that term.
    IntNum intmult;
    intmult.swap(*terms[intpos].getIntNum());
    terms[intpos].Clear();

    // Make MUL operator ADD now; use number of ADD terms from REG+... ADD.
    // While we could theoretically use the existing ADD, it's not safe, as
    // this operator could be the topmost operator and we can't clobber that.
    terms[pos] = ExprTerm(Op::ADD, terms[otherpos].getNumChild(),
                          terms[pos].getSource(), terms[pos].m_depth);

    // Get ADD depth before deletion.  This will be the new MUL depth for the
    // added (distributed) MUL operations.
    int depth = terms[otherpos].m_depth;

    // Delete ADD operator.
    terms[otherpos].Clear();

    // for each term in ADD expression, insert *intnum.
    for (int n=otherpos-1; n >= 0; --n)
    {
        ExprTerm& child = terms[n];
        if (child.isEmpty())
            continue;
        if (child.m_depth <= depth)
            break;
        if (child.m_depth != depth+1)
            continue;

        // Simply multiply directly into integers
        if (IntNum* intn = child.getIntNum())
        {
            *intn *= intmult;
            --child.m_depth;    // bring up
            continue;
        }

        // Otherwise add *INT
        terms.insert(terms.begin()+n+1, 2,
                     ExprTerm(Op::MUL, 2, terms[n].getSource(), depth));
        // Set multiplier
        terms[n+1] = ExprTerm(intmult, terms[n].getSource(), terms[n].m_depth);

        // Level if child is also a MUL
        if (terms[n].isOp(Op::MUL))
        {
            e.LevelOp(m_diags, m_simplify_reg_mul, n+2);
            // Leveling may have brought up terms, so we need to skip
            // all children explicitly.
            int childnum = terms[n+2].getNumChild();
            int m = n+1;
            for (; m >= 0; --m)
            {
                ExprTerm& child2 = terms[m];
                if (child2.isEmpty())
                    continue;
                if (child2.m_depth <= depth)
                    break;
                if (child2.m_depth != depth+1)
                    continue;
                --childnum;
                if (childnum < 0)
                    break;
            }
            n = m+1;
        }

        // Update pos
        pos += 2;
    }
}

bool
X86EAChecker::GetTermRegUsage(Expr& e,
                              int pos,
                              /*@null@*/ int* indexreg,
                              int* indexval,
                              bool* indexmult)
{
    ExprTerms& terms = e.getTerms();
    ExprTerm& child = terms[pos];

    if (child.isType(ExprTerm::REG))
    {
        int regnum;
        if (!getReg(child, &regnum))
            return false;
        int regmult = ++m_regmult[regnum];

        // Let last, largest multipler win indexreg
        if (indexreg && regmult > 0 && *indexval <= regmult && !*indexmult)
        {
            *indexreg = regnum;
            *indexval = regmult;
        }
    }
    else if (child.isOp(Op::MUL))
    {
        int lhs, rhs;
        if (!getChildren(e, &lhs, &rhs, &pos))
            return true;

        ExprTerm* regterm;
        ExprTerm* intterm;
        if (terms[lhs].isType(ExprTerm::REG) &&
            terms[rhs].isType(ExprTerm::INT))
        {
            regterm = &terms[lhs];
            intterm = &terms[rhs];
        }
        else if (terms[rhs].isType(ExprTerm::REG) &&
                 terms[lhs].isType(ExprTerm::INT))
        {
            regterm = &terms[rhs];
            intterm = &terms[lhs];
        }
        else
            return true;

        IntNum* intn = intterm->getIntNum();
        assert(intn);

        int regnum;
        if (!getReg(*regterm, &regnum))
            return false;

        long delta = intn->getInt();
        m_regmult[regnum] += delta;
        int regmult = m_regmult[regnum];

        // Let last, largest positive multiplier win indexreg
        // If we subtracted from the multiplier such that it dropped to 1 or
        // less, remove indexreg status (and the calling code will try and
        // auto-determine the multiplier).
        if (indexreg && delta > 0 && *indexval <= regmult)
        {
            *indexreg = regnum;
            *indexval = regmult;
            *indexmult = true;
        }
        else if (indexreg && *indexreg == regnum && delta < 0 && regmult <= 1)
        {
            *indexreg = -1;
            *indexval = 0;
            *indexmult = false;
        }
    }
    else if (child.isOp() && e.Contains(ExprTerm::REG, pos))
        return false;   // can't contain reg elsewhere
    return true;
}

// Simplify and determine if expression is superficially valid:
// Valid expr should be [(int-equiv expn)]+[reg*(int-equiv expn)+...]
// where the [...] parts are optional.
//
// Don't simplify out constant identities if we're looking for an indexreg: we
// may need the multiplier for determining what the indexreg is!
//
// Returns 1 if invalid register usage, 2 if circular reference,
// and 0 if all values successfully determined and saved in data.
int
X86EAChecker::GetRegUsage(Expr& e, /*@null@*/ int* indexreg, bool* ip_rel)
{
    if (!ExpandEqu(e))
        return 2;

    X86DistRegFunctor dist_reg(indexreg == 0, m_diags);
    e.Simplify(m_diags, dist_reg, indexreg == 0);

    // Check for WRT rip first
    Expr wrt = e.ExtractWRT();
    if (!wrt.isEmpty())
    {
        ExprTerm& wrt_term = wrt.getTerms().front();
        if (!wrt_term.isType(ExprTerm::REG))
            return 1;

        if (m_bits != 64)   // only valid in 64-bit mode
            return 1;
        int regnum;
        if (!getReg(wrt_term, &regnum) || regnum != 16)   // only accept rip
            return 1;
        m_regmult[regnum]++;

        // Delete WRT.  Set ip_rel to 1 to indicate to x86
        // bytecode code to do IP-relative displacement transform.
        *ip_rel = true;
    }

    int indexval = 0;
    bool indexmult = false;
    if (e.isOp(Op::ADD))
    {
        // Check each term for register (and possible multiplier).
        ExprTerms& terms = e.getTerms();
        ExprTerm& root = terms.back();
        int pos = terms.size()-2;
        for (; pos >= 0; --pos)
        {
            ExprTerm& child = terms[pos];
            if (child.isEmpty())
                continue;
            if (child.m_depth <= root.m_depth)
                break;
            if (child.m_depth != root.m_depth+1)
                continue;
        }
        ++pos;

        for (;; ++pos)
        {
            ExprTerm& child = terms[pos];
            if (child.isEmpty())
                continue;
            if (child.m_depth <= root.m_depth)
                break;
            if (child.m_depth != root.m_depth+1)
                continue;

            if (!GetTermRegUsage(e, pos, indexreg, &indexval, &indexmult))
                return 1;
        }
    }
    else if (!GetTermRegUsage(e, e.getTerms().size()-1, indexreg, &indexval,
                              &indexmult))
        return 1;

    // Simplify expr, which is now really just the displacement. This
    // should get rid of the 0's we put in for registers in the callback.
    e.Simplify(m_diags);

    return 0;
}

/*@-nullstate@*/
bool
X86EffAddr::CalcDispLen(unsigned int wordsize,
                        bool noreg,
                        bool dispreq,
                        DiagnosticsEngine& diags)
{
    m_valid_modrm = false;      // default to not yet valid

    switch (m_disp.getSize())
    {
        case 0:
            break;
        // If not 0, the displacement length was forced; set the Mod bits
        // appropriately and we're done with the ModRM byte.
        case 8:
            // Byte is only a valid override if there are registers in the
            // EA.  With no registers, we must have a 16/32 value.
            if (noreg)
            {
                diags.Report(m_disp.getSource().getBegin(),
                             diag::warn_fixed_invalid_disp_size);
                m_disp.setSize(wordsize);
            }
            else
                m_modrm |= 0100;
            m_valid_modrm = true;
            return true;
        case 16:
        case 32:
            // Don't allow changing displacement different from BITS setting
            // directly; require an address-size override to change it.
            if (wordsize != m_disp.getSize())
            {
                diags.Report(m_disp.getSource().getBegin(),
                             diag::err_invalid_disp_size);
                return false;
            }
            if (!noreg)
                m_modrm |= 0200;
            m_valid_modrm = true;
            return true;
        default:
            // we shouldn't ever get any other size!
            assert(false && "strange EA displacement size");
            return false;
    }

    // The displacement length hasn't been forced (or the forcing wasn't
    // valid), try to determine what it is.
    if (noreg)
    {
        // No register in ModRM expression, so it must be disp16/32,
        // and as the Mod bits are set to 0 by the caller, we're done
        // with the ModRM byte.
        m_disp.setSize(wordsize);
        m_valid_modrm = true;
        return true;
    }

    if (dispreq)
    {
        // for BP/EBP, there *must* be a displacement value, but we
        // may not know the size (8 or 16/32) for sure right now.
        m_need_nonzero_len = true;
    }

    if (m_disp.isRelative())
    {
        // Relative displacement; basically all object formats need non-byte
        // for relocation here, so just do that. (TODO: handle this
        // differently?)
        m_disp.setSize(wordsize);
        m_modrm |= 0200;
        m_valid_modrm = true;
        return true;
    }

    // At this point there's 3 possibilities for the displacement:
    //  - None (if =0)
    //  - signed 8 bit (if in -128 to 127 range)
    //  - 16/32 bit (word size)
    // For now, check intnum value right now; if it's not 0,
    // assume 8 bit and set up for allowing 16 bit later.
    // FIXME: The complex expression equaling zero is probably a rare case,
    // so we ignore it for now.
    IntNum num;
    if (!m_disp.getIntNum(&num, false, diags))
    {
        // Still has unknown values.
        m_need_nonzero_len = true;
        m_modrm |= 0100;
        m_valid_modrm = true;
        return true;
    }

    // Figure out what size displacement we will have.
    if (num.isZero() && !m_need_nonzero_len)
    {
        // If we know that the displacement is 0 right now,
        // go ahead and delete the expr and make it so no
        // displacement value is included in the output.
        // The Mod bits of ModRM are set to 0 above, and
        // we're done with the ModRM byte!
        m_disp.Clear();
        m_need_disp = false;
    }
    else if (num.isInRange(-128, 127))
    {
        // It fits into a signed byte
        m_disp.setSize(8);
        m_modrm |= 0100;
    }
    else
    {
        // It's a 16/32-bit displacement
        m_disp.setSize(wordsize);
        m_modrm |= 0200;
    }
    m_valid_modrm = true;   // We're done with ModRM
    return true;
}
/*@=nullstate@*/

static bool
getRegSize(const Expr& e, unsigned char* addrsize)
{
    const ExprTerms& terms = e.getTerms();

    for (ExprTerms::const_iterator i=terms.begin(), end=terms.end(); i != end;
         ++i)
    {
        if (const X86Register* reg =
            static_cast<const X86Register*>(i->getRegister()))
        {
            switch (reg->getType())
            {
                case X86Register::REG16:
                    *addrsize = 16;
                    break;
                case X86Register::REG32:
                    *addrsize = 32;
                    break;
                case X86Register::REG64:
                case X86Register::RIP:
                    *addrsize = 64;
                    break;
                default:
                    return false;
            }
            return true;
        }
    }
    return false;
}

bool
X86EffAddr::Check3264(unsigned int addrsize,
                      unsigned int bits,
                      unsigned char* rex,
                      bool* ip_rel,
                      DiagnosticsEngine& diags)
{
    int i;
    unsigned char low3;

    int basereg = X86EAChecker::kREG_NONE;  // "base" register (for SIB)
    int indexreg = X86EAChecker::kREG_NONE; // "index" register (for SIB)

    // We can only do 64-bit addresses in 64-bit mode.
    if (addrsize == 64 && bits != 64)
    {
        diags.Report(m_disp.getSource().getBegin(),
                     diag::err_64bit_ea_not64mode);
        return false;
    }

    if (m_pc_rel && bits != 64)
    {
        diags.Report(m_disp.getSource().getBegin(),
                     diag::warn_rip_rel_not64mode);
        m_pc_rel = false;
    }

    X86EAChecker checker(bits, addrsize, m_vsib_mode, diags);

    if (m_disp.hasAbs())
    {
        switch (checker.GetRegUsage(*m_disp.getAbs(), &indexreg, ip_rel))
        {
            case 1:
                diags.Report(m_disp.getSource().getBegin(),
                             diag::err_invalid_ea);
                return false;
            case 2:
                diags.Report(m_disp.getSource().getBegin(),
                             diag::err_equ_circular_reference_mem);
                return false;
            default:
                break;
        }
    }

    // If indexreg mult is 0, discard it.
    // This is possible because of the way indexreg is found in
    // expr_checkea_getregusage().
    if (indexreg != X86EAChecker::kREG_NONE && checker.m_regmult[indexreg] == 0)
        indexreg = X86EAChecker::kREG_NONE;

    // Find a basereg (*1, but not indexreg), if there is one.
    // Also, if an indexreg hasn't been assigned, try to find one.
    // Meanwhile, check to make sure there's no negative register mults.
    for (i=0; i<checker.m_regcount; i++)
    {
        if (checker.m_regmult[i] < 0)
        {
            diags.Report(m_disp.getSource().getBegin(), diag::err_invalid_ea);
            return false;
        }
        if (i != indexreg && checker.m_regmult[i] == 1 &&
            basereg == X86EAChecker::kREG_NONE)
            basereg = i;
        else if (indexreg == X86EAChecker::kREG_NONE
                 && checker.m_regmult[i] > 0)
            indexreg = i;
    }

    if (m_vsib_mode != 0)
    {
        // For VSIB, the SIMD register needs to go into the indexreg.
        // Also check basereg (must be a GPR if present) and indexreg
        // (must be a SIMD register).
        if (basereg >= X86EAChecker::kSIMDREGS
            && (indexreg == X86EAChecker::kREG_NONE
                || checker.m_regmult[indexreg] == 1))
        {
            std::swap(basereg, indexreg);
        }
        if (basereg >= X86EAChecker::kREG_RIP
            || indexreg < X86EAChecker::kSIMDREGS)
        {
            diags.Report(m_disp.getSource().getBegin(), diag::err_invalid_ea);
            return false;
        }
    }
    else if (indexreg != X86EAChecker::kREG_NONE
             && basereg == X86EAChecker::kREG_NONE)
    {
        // Handle certain special cases of indexreg mults when basereg is
        // empty.
        switch (checker.m_regmult[indexreg])
        {
            case 1:
                // Only optimize this way if nosplit wasn't specified
                if (!m_nosplit)
                {
                    basereg = indexreg;
                    indexreg = -1;
                }
                break;
            case 2:
                // Only split if nosplit wasn't specified
                if (!m_nosplit)
                {
                    basereg = indexreg;
                    checker.m_regmult[indexreg] = 1;
                }
                break;
            case 3:
            case 5:
            case 9:
                basereg = indexreg;
                checker.m_regmult[indexreg]--;
                break;
        }
    }

    // Make sure there's no other registers than the basereg and indexreg
    // we just found.
    for (i=0; i<checker.m_regcount; i++)
    {
        if (i != basereg && i != indexreg && checker.m_regmult[i] != 0)
        {
            diags.Report(m_disp.getSource().getBegin(), diag::err_invalid_ea);
            return false;
        }
    }

    // Check the index multiplier value for validity if present.
    if (indexreg != X86EAChecker::kREG_NONE &&
        checker.m_regmult[indexreg] != 1 &&
        checker.m_regmult[indexreg] != 2 &&
        checker.m_regmult[indexreg] != 4 &&
        checker.m_regmult[indexreg] != 8)
    {
        diags.Report(m_disp.getSource().getBegin(), diag::err_invalid_ea);
        return false;
    }

    // ESP is not a legal indexreg.
    if (indexreg == X86EAChecker::kREG_RSP)
    {
        // If mult>1 or basereg is ESP also, there's no way to make it
        // legal.
        if (checker.m_regmult[X86EAChecker::kREG_RSP] > 1 ||
            basereg == X86EAChecker::kREG_RSP)
        {
            diags.Report(m_disp.getSource().getBegin(), diag::err_invalid_ea);
            return false;
        }

        // If mult==1 and basereg is not ESP, swap indexreg w/basereg.
        indexreg = basereg;
        basereg = X86EAChecker::kREG_RSP;
    }

    // RIP is only legal if it's the ONLY register used.
    if (indexreg == X86EAChecker::kREG_RIP ||
        (basereg == X86EAChecker::kREG_RIP &&
         indexreg != X86EAChecker::kREG_NONE))
    {
        diags.Report(m_disp.getSource().getBegin(), diag::err_invalid_ea);
        return false;
    }

    // At this point, we know the base and index registers and that the
    // memory expression is (essentially) valid.  Now build the ModRM and
    // (optional) SIB bytes.

    // If we're supposed to be RIP-relative and there's no register
    // usage, change to RIP-relative.
    if (basereg == X86EAChecker::kREG_NONE &&
        indexreg == X86EAChecker::kREG_NONE &&
        m_pc_rel)
    {
        basereg = X86EAChecker::kREG_RIP;
        *ip_rel = true;
    }

    // First determine R/M (Mod is later determined from disp size)
    m_need_modrm = true;    // we always need ModRM
    if (basereg == X86EAChecker::kREG_NONE
        && indexreg == X86EAChecker::kREG_NONE)
    {
        // Just a disp32: in 64-bit mode the RM encoding is used for RIP
        // offset addressing, so we need to use the SIB form instead.
        if (bits == 64)
        {
            m_modrm |= 4;
            m_need_sib = 1;
        }
        else
        {
            m_modrm |= 5;
            m_sib = 0;
            m_valid_sib = false;
            m_need_sib = 0;
        }
    }
    else if (basereg == X86EAChecker::kREG_RIP)
    {
        m_modrm |= 5;
        m_sib = 0;
        m_valid_sib = false;
        m_need_sib = 0;
        // RIP always requires a 32-bit signed displacement
        m_valid_modrm = true;
        m_disp.setSize(32);
        m_disp.setSigned();
        return true;
    }
    else if (indexreg == X86EAChecker::kREG_NONE)
    {
        // basereg only
        // Don't need to go to the full effort of determining what type
        // of register basereg is, as set_rex_from_reg doesn't pay
        // much attention.
        if (!setRexFromReg(rex, &low3, X86Register::REG64, basereg, bits,
                           X86_REX_B))
        {
            diags.Report(m_disp.getSource().getBegin(),
                         diag::err_high8_rex_conflict);
            return false;
        }
        m_modrm |= low3;
        // we don't need an SIB *unless* basereg is ESP or R12
        if (basereg == X86EAChecker::kREG_RSP ||
            basereg == X86EAChecker::kREG_R12)
            m_need_sib = 1;
        else
        {
            m_sib = 0;
            m_valid_sib = false;
            m_need_sib = 0;
        }
    }
    else
    {
        // index or both base and index
        m_modrm |= 4;
        m_need_sib = 1;
    }

    // Determine SIB if needed
    if (m_need_sib == 1)
    {
        m_sib = 0;      // start with 0

        // Special case: no basereg
        if (basereg == X86EAChecker::kREG_NONE)
            m_sib |= 5;
        else
        {
            if (!setRexFromReg(rex, &low3, X86Register::REG64, basereg, bits,
                               X86_REX_B))
            {
                diags.Report(m_disp.getSource().getBegin(),
                             diag::err_high8_rex_conflict);
                return false;
            }
            m_sib |= low3;
        }

        // Put in indexreg, checking for none case
        if (indexreg == X86EAChecker::kREG_NONE)
            m_sib |= 040;
            // Any scale field is valid, just leave at 0.
        else
        {
            X86Register::Type type = X86Register::REG64;
            int indexregnum = indexreg;
            if (indexreg >= X86EAChecker::kSIMDREGS)
            {
                type = X86Register::XMMREG;
                indexregnum = indexreg - X86EAChecker::kSIMDREGS;
            }
            if (!setRexFromReg(rex, &low3, type, indexregnum, bits, X86_REX_X))
            {
                diags.Report(m_disp.getSource().getBegin(),
                             diag::err_high8_rex_conflict);
                return false;
            }

            m_sib |= low3 << 3;
            // Set scale field, 1 case -> 0, so don't bother.
            switch (checker.m_regmult[indexreg])
            {
                case 2:
                    m_sib |= 0100;
                    break;
                case 4:
                    m_sib |= 0200;
                    break;
                case 8:
                    m_sib |= 0300;
                    break;
            }
        }

        m_valid_sib = true;     // Done with SIB
    }

    // Calculate displacement length (if possible)
    return CalcDispLen(32, basereg == X86EAChecker::kREG_NONE,
                       basereg == X86EAChecker::kREG_RBP ||
                       basereg == X86EAChecker::kREG_R13,
                       diags);
}

bool
X86EffAddr::Check16(unsigned int bits,
                    bool address16_op,
                    bool* ip_rel,
                    DiagnosticsEngine& diags)
{
    static const unsigned char modrm16[16] =
    {
                // B D S B
                // P I I X
        0006,   // 0 0 0 0: disp16
        0007,   // 0 0 0 1: [BX]
        0004,   // 0 0 1 0: [SI]
        0000,   // 0 0 1 1: [BX+SI]
        0005,   // 0 1 0 0: [DI]
        0001,   // 0 1 0 1: [BX+DI]
        0377,   // 0 1 1 0: invalid
        0377,   // 0 1 1 1: invalid
        0006,   // 1 0 0 0: [BP]+d
        0377,   // 1 0 0 1: invalid
        0002,   // 1 0 1 0: [BP+SI]
        0377,   // 1 0 1 1: invalid
        0003,   // 1 1 0 0: [BP+DI]
        0377,   // 1 1 0 1: invalid
        0377,   // 1 1 1 0: invalid
        0377    // 1 1 1 1: invalid
    };
    enum HaveReg
    {
        HAVE_NONE = 0,
        HAVE_BX = 1<<0,
        HAVE_SI = 1<<1,
        HAVE_DI = 1<<2,
        HAVE_BP = 1<<3
    };

    // 64-bit mode does not allow 16-bit addresses
    if (bits == 64 && !address16_op)
    {
        diags.Report(m_disp.getSource().getBegin(), diag::err_16bit_ea_64mode);
        return false;
    }

    // 16-bit cannot have SIB
    m_sib = 0;
    m_valid_sib = false;
    m_need_sib = 0;

    X86EAChecker checker(bits, 16, m_vsib_mode, diags);

    if (m_disp.hasAbs())
    {
        switch (checker.GetRegUsage(*m_disp.getAbs(), 0, ip_rel))
        {
            case 1:
                diags.Report(m_disp.getSource().getBegin(),
                             diag::err_invalid_ea);
                return false;
            case 2:
                diags.Report(m_disp.getSource().getBegin(),
                             diag::err_equ_circular_reference_mem);
                return false;
            default:
                break;
        }
    }

    int bx = checker.m_regmult[X86EAChecker::kREG_RBX];
    int si = checker.m_regmult[X86EAChecker::kREG_RSI];
    int di = checker.m_regmult[X86EAChecker::kREG_RDI];
    int bp = checker.m_regmult[X86EAChecker::kREG_RBP];

    // reg multipliers not 0 or 1 are illegal.
    if (bx & ~1 || si & ~1 || di & ~1 || bp & ~1)
    {
        diags.Report(m_disp.getSource().getBegin(), diag::err_invalid_ea);
        return false;
    }

    // Set havereg appropriately
    int havereg = HAVE_NONE;
    if (bx > 0)
        havereg |= HAVE_BX;
    if (si > 0)
        havereg |= HAVE_SI;
    if (di > 0)
        havereg |= HAVE_DI;
    if (bp > 0)
        havereg |= HAVE_BP;

    // Check the modrm value for invalid combinations.
    if (modrm16[havereg] & 0070)
    {
        diags.Report(m_disp.getSource().getBegin(), diag::err_invalid_ea);
        return false;
    }

    // Set ModRM byte for registers
    m_modrm |= modrm16[havereg];

    // Calculate displacement length (if possible)
    return CalcDispLen(16, havereg == HAVE_NONE, havereg == HAVE_BP, diags);
}

bool
X86EffAddr::Check(unsigned char* addrsize,
                  unsigned int bits,
                  bool address16_op,
                  unsigned char* rex,
                  bool* ip_rel,
                  DiagnosticsEngine& diags)
{
    if (*addrsize == 0)
    {
        // we need to figure out the address size from what we know about:
        // - the displacement length
        // - what registers are used in the expression
        // - the bits setting
        switch (m_disp.getSize())
        {
            case 16:
                // must be 16-bit
                *addrsize = 16;
                break;
            case 64:
                // We have to support this for the MemOffs case, but it's
                // otherwise illegal.  It's also illegal in non-64-bit mode.
                if (m_need_modrm || m_need_sib)
                {
                    diags.Report(m_disp.getSource().getBegin(),
                                 diag::err_invalid_disp_size);
                    return false;
                }
                *addrsize = 64;
                break;
            case 32:
                // Must be 32-bit in 16-bit or 32-bit modes.  In 64-bit mode,
                // we don't know unless we look at the registers, except in the
                // MemOffs case (see the end of this function).
                if (bits != 64 || (!m_need_modrm && !m_need_sib))
                {
                    *addrsize = 32;
                    break;
                }
                /*@fallthrough@*/
            default:
                // If SIB is required, but we're in 16-bit mode, set to 32.
                if (bits == 16 && m_need_sib == 1)
                {
                    *addrsize = 32;
                    break;
                }
                // check for use of 16 or 32-bit registers; if none are used
                // default to bits setting.
                if (!m_disp.hasAbs() ||
                    !getRegSize(*m_disp.getAbs(), addrsize))
                    *addrsize = bits;
                // TODO: Add optional warning here if switched address size
                // from bits setting just by register use.. eg [ax] in
                // 32-bit mode would generate a warning.
        }
    }

    if ((*addrsize == 32 || *addrsize == 64) &&
        ((m_need_modrm && !m_valid_modrm) || (m_need_sib && !m_valid_sib)))
    {
        if (!Check3264(*addrsize, bits, rex, ip_rel, diags))
            return false;
        if (m_disp.getSize() < bits)
            m_disp.setSigned();
    }
    else if (*addrsize == 16 && m_need_modrm && !m_valid_modrm)
    {
        if (!Check16(bits, address16_op, ip_rel, diags))
            return false;
        if (m_disp.getSize() < bits)
            m_disp.setSigned();
    }
    else if (!m_need_modrm && !m_need_sib)
    {
        // Special case for MOV MemOffs opcode: displacement but no modrm.
        m_disp.setSigned(false);    // always unsigned
        switch (*addrsize)
        {
            case 64:
                if (bits != 64)
                {
                    diags.Report(m_disp.getSource().getBegin(),
                                 diag::err_64bit_ea_not64mode);
                    return false;
                }
                m_disp.setSize(64);
                break;
            case 32:
                m_disp.setSize(32);
                break;
            case 16:
                // 64-bit mode does not allow 16-bit addresses
                if (bits == 64 && !address16_op)
                {
                    diags.Report(m_disp.getSource().getBegin(),
                                 diag::err_16bit_ea_64mode);
                    return false;
                }
                m_disp.setSize(16);
                break;
        }
    }
    return true;
}

bool
X86EffAddr::Finalize(DiagnosticsEngine& diags)
{
    if (!m_disp.Finalize(diags, diag::err_ea_too_complex))
        return false;
    return true;
}
