//
//  Copyright (C) 2009  Peter Johnson
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
#include "NasmInsnRunner.h"

#include <memory>
#include <ostream>
#include <string>

#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"
#include "yasmx/Basic/Diagnostic.h"
#include "yasmx/Basic/SourceManager.h"
#include "yasmx/Arch.h"
#include "yasmx/Bytecode.h"
#include "yasmx/BytecodeContainer.h"
#include "yasmx/BytecodeOutput.h"
#include "yasmx/EffAddr.h"
#include "yasmx/Expr.h"
#include "yasmx/Insn.h"
#include "yasmx/IntNum.h"

#include "diag_mock.h"
#include "unittest_util.h"


using namespace yasm;
using namespace yasmunit;

namespace llvm
{

// print llvm::StringRef values nicely
void PrintTo(StringRef str, ::std::ostream* os)
{
    *os << str.str();
}

} // namespace llvm

namespace {

llvm::StringRef
strip(const llvm::StringRef in)
{
    size_t i, j;
    for (i=0; i<in.size() && (in[i] == ' ' || in[i] == '\t' || in[i] == '\r'); ++i) {}
    for (j=in.size(); j>i && (in[j-1] == ' ' || in[j-1] == '\t' || in[j-1] == '\r'); --j) {}
    return in.slice(i, j);
}

static inline unsigned int
fromhexdigit(char ch)
{
    if (ch >= '0' && ch <= '9')
        return ch-'0';
    else if (ch >= 'a' && ch <= 'f')
        return ch-'a'+10;
    else if (ch >= 'A' && ch <= 'F')
        return ch-'A'+10;
    return 0x100;
}

static inline unsigned int
fromoctdigit(char ch)
{
    if (ch >= '0' && ch <= '7')
        return ch-'0';
    return 0x100;
}

static IntNum
strtoint(const llvm::StringRef& str)
{
    IntNum intn;
    int neg = (str[0] == '-') ? 1:0;
    if (str.substr(neg).startswith("0x"))
        intn.setStr(str.substr(2+neg), 16);
    else if (str.endswith("h"))
        intn.setStr(str.substr(0, str.size()-1), 16);
    else
        intn.setStr(str.substr(neg));
    if (neg != 0)
        intn = -intn;
    return intn;
}

void
AddSpanTest(Bytecode& bc,
            int id,
            const Value& value,
            long neg_thres,
            long pos_thres)
{
}

class RawOutput : public BytecodeStreamOutput
{
public:
    RawOutput(llvm::raw_ostream& os, const Arch& arch, Diagnostic& diags)
        : BytecodeStreamOutput(os, diags), m_arch(arch)
    {}
    ~RawOutput() {}

    // OutputBytecode overrides
    bool ConvertValueToBytes(Value& value,
                             Location loc,
                             NumericOutput& num_out);

private:
    const Arch& m_arch;
};

bool
RawOutput::ConvertValueToBytes(Value& value,
                               Location loc,
                               NumericOutput& num_out)
{
    // Simplify absolute portion of value
    if (Expr* abs = value.getAbs())
        abs->Simplify(getDiagnostics());

    // Output
    IntNum intn;
    value.OutputBasic(num_out, &intn, getDiagnostics());
    return true;
}

} // anonymous namespace

namespace yasmunit
{

NasmInsnRunner::NasmInsnRunner()
{
}

NasmInsnRunner::~NasmInsnRunner()
{
}

void
NasmInsnRunner::ParseAndTestFile(const char* filename)
{
    std::string errstr;
    std::auto_ptr<llvm::MemoryBuffer> in(
        llvm::MemoryBuffer::getFile(filename, &errstr));

    ASSERT_TRUE(in.get() != 0)
        << "could not open " << filename << ": " << errstr;
    int linenum = 0;
    llvm::StringRef remainder = in->getBuffer();
    do {
        llvm::StringRef line;
        llvm::tie(line, remainder) = remainder.split('\n');
        ++linenum;
        ParseAndTestLine(filename, line, linenum);
    } while (!remainder.empty());
}

void
NasmInsnRunner::ParseAndTestLine(const char* filename,
                                 const llvm::StringRef& line,
                                 int linenum)
{
    SCOPED_TRACE(llvm::format("%s:%d", filename, linenum));

    llvm::StringRef insn_in, golden_in;
    llvm::tie(insn_in, golden_in) = line.split(';');

    insn_in = strip(insn_in);
    golden_in = strip(golden_in);

    // Handle bits directive
    if (golden_in.empty() && insn_in.startswith("[bits "))
    {
        int bits = atoi(insn_in.substr(6, 2).str().c_str());
        if (bits == 64)
            m_arch->setMachine("amd64");
        else
            m_arch->setMachine("x86");
        m_arch->setVar("mode_bits", bits);
        return;
    }

    if (insn_in.empty() || golden_in.empty())
        return;     // skip lines that don't have both text and a comment

    //
    // parse the golden result
    //
    llvm::SmallVector<unsigned char, 64> golden;
    llvm::StringRef golden_errwarn;

    for (;;)
    {
        // strip whitespace
        golden_in = strip(golden_in);
        if (golden_in.empty() || !isxdigit(golden_in[0]))
            break;

        unsigned int byte_val = 0x100;
        llvm::StringRef byte_str;
        llvm::tie(byte_str, golden_in) = golden_in.split(' ');

        if (byte_str.size() == 2)   // assume hex
            byte_val = (fromhexdigit(byte_str[0]) << 4)
                | fromhexdigit(byte_str[1]);
        else if (byte_str.size() == 3)  // assume octal
            byte_val = (fromoctdigit(byte_str[0]) << 6)
                | (fromoctdigit(byte_str[1]) << 3)
                | fromoctdigit(byte_str[2]);

        ASSERT_LE(byte_val, 0xffU) << "invalid golden value";
        golden.push_back(byte_val);
    }

    // interpret string in [] as error/warning
    if (!golden_in.empty() && golden_in[0] == '[')
        llvm::tie(golden_errwarn, golden_in) = golden_in.substr(1).split(']');

    //
    // parse the instruction
    //
    ::testing::StrictMock<MockDiagnosticString> mock_client;
    Diagnostic diags(&mock_client);
    SourceManager smgr(diags);
    diags.setSourceManager(&smgr);

    // instruction name is the first thing on the line
    llvm::StringRef insn_name;
    llvm::tie(insn_name, insn_in) = insn_in.split(' ');
    Arch::InsnPrefix insnprefix =
        m_arch->ParseCheckInsnPrefix(insn_name, SourceLocation(), diags);
    ASSERT_TRUE(insnprefix.isType(Arch::InsnPrefix::INSN));
    std::auto_ptr<Insn> insn = m_arch->CreateInsn(insnprefix.getInsn());
    ASSERT_TRUE(insn.get() != 0) << "unrecognized instruction '"
        << insn_name.str() << "'";

    // parse insn arguments
    unsigned int wsize = m_arch_module->getWordSize();

    // strip whitespace from arguments
    insn_in = strip(insn_in);

    while (!insn_in.empty())
    {
        llvm::StringRef arg_str;
        llvm::tie(arg_str, insn_in) = insn_in.split(',');

        // strip whitespace from arg
        arg_str = strip(arg_str);

        unsigned int size = 0;
        bool strict = false;
        for (;;)
        {
            int next;
            int nsize = 0;
            // operand overrides (size and strict)
            if (arg_str.startswith("byte ")) { nsize = 8; next = 5; }
            else if (arg_str.startswith("hword ")) { nsize = wsize/2; next = 6; }
            else if (arg_str.startswith("word ")) { nsize = wsize; next = 5; }
            else if (arg_str.startswith("dword ")) { nsize = wsize*2; next = 6; }
            else if (arg_str.startswith("qword ")) { nsize = wsize*4; next = 6; }
            else if (arg_str.startswith("tword ")) { nsize = 80; next = 6; }
            else if (arg_str.startswith("dqword ")) { nsize = wsize*8; next = 7; }
            else if (arg_str.startswith("oword ")) { nsize = wsize*8; next = 6; }
            else if (arg_str.startswith("yword ")) { nsize = 256; next = 6; }
            else if (arg_str.startswith("strict ")) { strict = true; next = 7; }
            else break;

            if (size == 0)
                size = nsize;
            arg_str = arg_str.substr(next);
        }

        if (arg_str[0] == '[')
        {
            // Very simple int/reg expression parser.  Does not handle parens or
            // order of operations; simply builds expr from left to right.
            // This means r8*4+r9 will have a different result than r9+r8*4!
            // Also only handles binary operators * and +.
            llvm::StringRef estr = arg_str.slice(1, arg_str.find(']')+1);
            std::auto_ptr<Expr> e(new Expr);

            char pendingop = '\0';
            std::size_t tokstart = 0;
            for (std::size_t pos = 0; pos < estr.size(); ++pos)
            {
                if (estr[pos] == '*' || estr[pos] == '+' || estr[pos] == ']')
                {
                    // figure out this token
                    llvm::StringRef tok = strip(estr.slice(tokstart, pos));
                    if (isdigit(estr[tokstart]))
                        e->Append(strtoint(tok));
                    else
                    {
                        Arch::RegTmod regtmod =
                            m_arch->ParseCheckRegTmod(tok,
                                                      SourceLocation(),
                                                      diags);
                        ASSERT_TRUE(regtmod.isType(Arch::RegTmod::REG))
                            << "cannot handle label '" << tok.str() << "'";
                        e->Append(*regtmod.getReg());
                    }

                    // append pending operator
                    if (pendingop == '*')
                        e->AppendOp(Op::MUL, 2);
                    else if (pendingop == '+')
                        e->AppendOp(Op::ADD, 2);

                    // store new operator
                    pendingop = estr[pos];
                    tokstart = pos+1;
                }
            }

            Operand operand(m_arch->CreateEffAddr(e));
            operand.setSize(size);
            operand.setStrict(strict);
            insn->AddOperand(operand);
            continue;
        }

        // TODO: split by space to allow target modifiers

        // Test for registers
        Arch::RegTmod regtmod =
            m_arch->ParseCheckRegTmod(arg_str, SourceLocation(), diags);
        if (const Register* reg = regtmod.getReg())
        {
            Operand operand(reg);
            operand.setSize(size);
            operand.setStrict(strict);
            insn->AddOperand(operand);
            continue;
        }
        else if (const SegmentRegister* segreg = regtmod.getSegReg())
        {
            Operand operand(segreg);
            operand.setSize(size);
            operand.setStrict(strict);
            insn->AddOperand(operand);
            continue;
        }
        else if (regtmod.getTargetMod())
        {
            FAIL() << "cannot handle target modifier";
        }
        else if (regtmod.getRegGroup())
        {
            FAIL() << "cannot handle register group";
        }

        // Can't handle labels
        ASSERT_TRUE(isdigit(arg_str[0]) || arg_str[0] == '-')
            << "cannot handle label '" << arg_str.str() << "'";

        // Convert to integer expression
        Operand intop(Expr::Ptr(new Expr(strtoint(arg_str))));
        intop.setSize(size);
        intop.setStrict(strict);
        insn->AddOperand(intop);
    }

    TestInsn(insn.get(), golden.size(), golden.data(), golden_errwarn);
}

void
NasmInsnRunner::TestInsn(yasm::Insn* insn,
                         std::size_t golden_len,
                         const unsigned char* golden,
                         const llvm::StringRef& ew_msg)
{
    //
    // Turn the instruction into bytes
    //
    BytecodeContainer container(0);

    ::testing::StrictMock<MockDiagnosticString> mock_client;
    Diagnostic diags(&mock_client);
    SourceManager smgr(diags);
    diags.setSourceManager(&smgr);

    if (!ew_msg.empty())
    {
        EXPECT_CALL(mock_client, DiagString(ew_msg))
            .Times(1);
    }
    else
    {
        // expect no diagnostic calls
        EXPECT_CALL(mock_client, DiagString(::testing::_))
            .Times(0);
    }

    insn->Append(container, SourceLocation(), diags);

    container.Finalize(diags);
    if (diags.hasErrorOccurred())
        return;

    container.bytecodes_front().CalcLen(AddSpanTest, diags);
    ASSERT_EQ(golden_len, container.bytecodes_front().getTotalLen());
    if (diags.hasErrorOccurred())
        return;

    container.UpdateOffsets(diags);
    if (diags.hasErrorOccurred())
        return;

    llvm::SmallString<64> outbytes;
    llvm::raw_svector_ostream outstream(outbytes);
    RawOutput outputter(outstream, *m_arch, diags);
    container.bytecodes_front().Output(outputter);
    outstream.flush();

    //
    // Compare the result against the golden result
    //
    if (golden_len != outbytes.size())
        goto bad;

    for (std::size_t i=0; i<golden_len; ++i)
    {
        if ((int)(golden[i] & 0xff) != (int)(outbytes[i] & 0xff))
            goto bad;
    }

    return;

bad:
    std::string golden_str, outbytes_str;
    llvm::raw_string_ostream golden_ss(golden_str), outbytes_ss(outbytes_str);

    for (std::size_t i=0; i<golden_len; ++i)
        golden_ss << llvm::format("%02x ", (int)(golden[i] & 0xff));
    golden_ss.flush();

    for (std::size_t i=0; i<outbytes.size(); ++i)
        outbytes_ss << llvm::format("%02x ", (int)(outbytes[i] & 0xff));
    outbytes_ss.flush();

    ASSERT_EQ(golden_str, outbytes_str);
}

} // namespace yasmunit
