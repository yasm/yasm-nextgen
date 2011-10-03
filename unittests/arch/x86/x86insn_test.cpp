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
#include <gtest/gtest.h>
#include <set>
#include <string>
#include <vector>

#include "llvm/Support/Path.h"
#include "yasmx/Support/registry.h"
#include "yasmx/System/plugin.h"
#include "yasmx/Arch.h"

#include "unittests/NasmInsnRunner.h"
#include "unittests/unittest_util.h"
#include "unittests/unittest_config.h"


class X86NasmInsnRunner : public yasmunit::NasmInsnRunner
{
public:
    static void SetUpTestCase()
    {
        ASSERT_TRUE(yasm::LoadStandardPlugins());
    }

    virtual void SetUp()
    {
        m_arch_module = yasm::LoadModule<yasm::ArchModule>("x86");
        ASSERT_TRUE(m_arch_module.get() != 0) << "could not load x86 arch";
        m_arch = m_arch_module->Create();
        m_arch->setParser("nasm");
    }

    virtual void TearDown()
    {
        m_arch.reset(0);
        m_arch_module.reset(0);
    }
};

TEST_P(X86NasmInsnRunner, Run)
{
    ParseAndTestFile(GetParam().c_str());
}

std::vector<std::string>
GetTestFiles()
{
    std::string srcdir;
    if (const char* srcdirc = getenv("CMAKE_SOURCE_DIR"))
        srcdir = srcdirc;
    else
        srcdir = CMAKE_SOURCE_DIR;
    srcdir += "/unittests/arch/x86/insn/";

    using llvm::sys::Path;
    Path srcpath(srcdir);
    std::set<Path> paths;
    if (srcpath.getDirectoryContents(paths, NULL))
    {
        std::vector<std::string> v;
        v.push_back("could not get file listing");
        return v;
    }

    std::vector<std::string> v;
    for (std::set<Path>::const_iterator i=paths.begin(), end=paths.end();
         i != end; ++i)
    {
        if (llvm::sys::path::extension(i->str()) == ".asm" ||
            llvm::sys::path::extension(i->str()) == ".ASM")
            v.push_back(i->str());
    }
    return v;
}

INSTANTIATE_TEST_CASE_P(X86InsnTests, X86NasmInsnRunner,
                        ::testing::ValuesIn(GetTestFiles()));
