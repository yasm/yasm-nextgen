#include <cstdlib>

#include <gtest/gtest.h>

#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"
#include "yasmx/Support/errwarn.h"
#include "yasmx/Support/registry.h"
#include "yasmx/System/plugin.h"
#include "yasmx/Errwarns.h"
#include "yasmx/Preprocessor.h"

using namespace yasm;

TEST(RawPreprocTest, Basic)
{
    ASSERT_TRUE(LoadStandardPlugins());
    std::auto_ptr<PreprocessorModule> preproc_module =
        LoadModule<PreprocessorModule>("raw");
    ASSERT_TRUE(preproc_module.get());
    std::string instr("test text");
    clang::FileManager file_mgr;
    clang::SourceManager source_mgr;
    const llvm::MemoryBuffer* inbuf =
        llvm::MemoryBuffer::getMemBuffer(instr.c_str(),
                                         instr.c_str()+instr.size(),
                                         "<string>");
    source_mgr.createMainFileIDForMemBuffer(inbuf);
    Errwarns errwarns;
    std::auto_ptr<Preprocessor> preproc = preproc_module->Create(errwarns);
    ASSERT_TRUE(preproc.get());
    preproc->Initialize(source_mgr, file_mgr);

    std::string outstr;
    clang::SourceLocation sourceloc;
    ASSERT_TRUE(preproc->getLine(&outstr, &sourceloc));

    EXPECT_EQ(instr, outstr);

    EXPECT_FALSE(preproc->getLine(&outstr, &sourceloc));
}
