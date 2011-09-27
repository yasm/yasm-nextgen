//
// GAS-compatible preprocessor implementation
//
//  Copyright (C) 2010  Peter Johnson
//
// Based on the LLVM Compiler Infrastructure
// (distributed under the University of Illinois Open Source License.
// See Copying/LLVM.txt for details).
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
#include "GasPreproc.h"

#include "yasmx/Basic/FileManager.h"
#include "yasmx/Parse/HeaderSearch.h"

#include "GasLexer.h"

using namespace yasm;
using namespace yasm::parser;

GasPreproc::GasPreproc(Diagnostic& diags,
                       SourceManager& sm,
                       HeaderSearch& headers)
    : Preprocessor(diags, sm, headers)
{
}

GasPreproc::~GasPreproc()
{
}

void
GasPreproc::PredefineMacro(llvm::StringRef macronameval)
{
}

void
GasPreproc::UndefineMacro(llvm::StringRef macroname)
{
}

void
GasPreproc::DefineBuiltin(llvm::StringRef macronameval)
{
}

bool
GasPreproc::HandleInclude(llvm::StringRef filename, SourceLocation source)
{
    if (filename.empty())
    {
        Diag(source, diag::err_pp_empty_filename);
        return false;
    }

    // Check that we don't have infinite #include recursion.
    if (m_include_macro_stack.size() == MaxAllowedIncludeStackDepth-1)
    {
        Diag(source, diag::err_pp_include_too_deep);
        return false;
    }

    // Search include directories.
    const DirectoryLookup* cur_dir;
    const FileEntry* file = LookupFile(filename, false, NULL, cur_dir);
    if (file == 0)
    {
        Diag(source, diag::err_pp_file_not_found) << filename;
        return false;
    }

    // Ask HeaderInfo if we should enter this #include file.  If not, #including
    // this file will have no effect.
    if (!m_header_info.ShouldEnterIncludeFile(file, false))
        return true;

    // Look up the file, create a File ID for it.
    FileID fid = m_source_mgr.createFileID(file, source, SrcMgr::C_User);
    if (fid.isInvalid())
    {
        Diag(source, diag::err_pp_file_not_found) << filename;
        return false;
    }

    // Finally, if all is good, enter the new file!
    EnterSourceFile(fid, cur_dir, source);
    return true;
}

void
GasPreproc::RegisterBuiltinMacros()
{
}

Lexer*
GasPreproc::CreateLexer(FileID fid, const llvm::MemoryBuffer* input_buffer)
{
    return new GasLexer(fid, input_buffer, *this);
}
