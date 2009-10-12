//
// Raw preprocessor (preforms NO preprocessing)
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

#include "clang/Basic/SourceLocation.h"
#include "clang/Basic/SourceManager.h"
#include "llvm/Support/MemoryBuffer.h"
#include "yasmx/Support/errwarn.h"
#include "yasmx/Support/registry.h"
#include "yasmx/Errwarns.h"
#include "yasmx/Preprocessor.h"

namespace yasm
{
namespace preproc
{
namespace raw
{

class RawPreproc : public Preprocessor
{
public:
    RawPreproc(const PreprocessorModule& module, Errwarns& errwarns)
        : Preprocessor(module, errwarns)
    {}
    ~RawPreproc() {}

    static const char* getName() { return "Disable preprocessing"; }
    static const char* getKeyword() { return "raw"; }

    void Initialize(clang::SourceManager& source_mgr,
                    clang::FileManager& file_mgr);

    bool getLine(/*@out@*/ std::string* line,
                 /*@out@*/ clang::SourceLocation* source);

    clang::SourceManager& getSourceManager() { return *m_source_mgr; }

    std::string getIncludedFile() { return ""; }
    void AddIncludeFile(const llvm::StringRef& filename) {}
    void PredefineMacro(const llvm::StringRef& macronameval) {}
    void UndefineMacro(const llvm::StringRef& macroname) {}
    void DefineBuiltin(const llvm::StringRef& macronameval) {}

private:
    const llvm::MemoryBuffer* m_in;
    clang::SourceLocation m_source_start;
    const char* m_pos;
    clang::SourceManager* m_source_mgr;
};

void
RawPreproc::Initialize(clang::SourceManager& source_mgr,
                       clang::FileManager& file_mgr)
{
    clang::FileID main_file = source_mgr.getMainFileID();
    m_in = source_mgr.getBuffer(main_file);
    m_source_start = source_mgr.getLocForStartOfFile(main_file);
    m_pos = m_in->getBufferStart();
    m_source_mgr = &source_mgr;
}

bool
RawPreproc::getLine(/*@out@*/ std::string* line,
                    /*@out@*/ clang::SourceLocation* source)
{
    const char* end = m_in->getBufferEnd();

    if (m_pos >= end)
        return false;

    *source = m_source_start.getFileLocWithOffset(m_pos-m_in->getBufferStart());

    const char* nextline = m_pos+1;
    while (nextline < end && *nextline != '\n')
        ++nextline;
    line->assign(m_pos, nextline-m_pos);    // strip '\n'
    m_pos = nextline+1;                     // skip over '\n' for next line

    return true;
}

void
DoRegister()
{
    RegisterModule<PreprocessorModule,
                   PreprocessorModuleImpl<RawPreproc> >("raw");
}

}}} // namespace yasm::preproc::raw
