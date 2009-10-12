#include <cstdlib>

#include "config.h"

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

int
main()
{
    if (!LoadStandardPlugins())
        return EXIT_FAILURE;
    std::auto_ptr<PreprocessorModule> preproc_module =
        LoadModule<PreprocessorModule>("raw");
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
    preproc->Initialize(source_mgr, file_mgr);

    std::string outstr;
    clang::SourceLocation sourceloc;
    if (!preproc->getLine(&outstr, &sourceloc))
        return EXIT_FAILURE;

    llvm::outs() << outstr << '\n';

    if (outstr != instr)
        return EXIT_FAILURE;

    if (preproc->getLine(&outstr, &sourceloc))
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
