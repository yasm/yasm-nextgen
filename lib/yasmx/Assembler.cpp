//
// Assembler implementation.
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
#include "yasmx/Assembler.h"

#include "llvm/Support/MemoryBuffer.h"
#include "llvm/System/Path.h"
#include "yasmx/Basic/Diagnostic.h"
#include "yasmx/Basic/SourceManager.h"
#include "yasmx/Parse/Directive.h"
#include "yasmx/Parse/Parser.h"
#include "yasmx/Support/registry.h"
#include "yasmx/Support/scoped_ptr.h"
#include "yasmx/Arch.h"
#include "yasmx/DebugFormat.h"
#include "yasmx/ListFormat.h"
#include "yasmx/Object.h"
#include "yasmx/ObjectFormat.h"


using namespace yasm;

namespace {
class NocaseEquals
{
    llvm::StringRef s;
public:
    NocaseEquals(llvm::StringRef str) : s(str) {}
    bool operator() (llvm::StringRef oth)
    {
        return s.equals_lower(oth);
    }
};
} // anonymous namespace

Assembler::Assembler(llvm::StringRef arch_keyword,
                     llvm::StringRef objfmt_keyword,
                     Diagnostic& diags,
                     Assembler::ObjectDumpTime dump_time)
    : m_arch_module(LoadModule<ArchModule>(arch_keyword).release()),
      m_parser_module(0),
      m_objfmt_module(LoadModule<ObjectFormatModule>(objfmt_keyword).release()),
      m_dbgfmt_module(0),
      m_listfmt_module(0),
      m_arch(0),
      m_parser(0),
      m_objfmt(0),
      m_dbgfmt(0),
      m_listfmt(0),
      m_object(0),
      m_dump_time(dump_time)
{
    if (m_arch_module.get() == 0)
    {
        diags.Report(SourceLocation(), diag::fatal_module_load)
            << "architecture" << arch_keyword;
        return;
    }

    if (m_objfmt_module.get() == 0)
    {
        diags.Report(SourceLocation(), diag::fatal_module_load)
            << "object format" << objfmt_keyword;
        return;
    }

    // Create architecture.
    m_arch.reset(m_arch_module->Create().release());

    // Get initial x86 BITS setting from object format
    if (m_arch_module->getKeyword().equals_lower("x86"))
        m_arch->setVar("mode_bits",
                       m_objfmt_module->getDefaultX86ModeBits());

}

Assembler::~Assembler()
{
}

void
Assembler::setObjectFilename(llvm::StringRef obj_filename)
{
    m_obj_filename = obj_filename;
}

bool
Assembler::setMachine(llvm::StringRef machine, Diagnostic& diags)
{
    if (!m_arch->setMachine(machine))
    {
        diags.Report(SourceLocation(), diag::fatal_module_combo)
            << "machine" << machine
            << "architecture" << m_arch_module->getKeyword();
        return false;
    }

    m_machine = machine;
    return true;
}

bool
Assembler::setParser(llvm::StringRef parser_keyword, Diagnostic& diags)
{
    // Ensure architecture supports parser.
    if (!m_arch->setParser(parser_keyword))
    {
        diags.Report(SourceLocation(), diag::fatal_module_combo)
            << "parser" << parser_keyword
            << "architecture" << m_arch_module->getKeyword();
        return false;
    }

    std::auto_ptr<ParserModule> parser_module =
        LoadModule<ParserModule>(parser_keyword);
    if (parser_module.get() == 0)
    {
        diags.Report(SourceLocation(), diag::fatal_module_load)
            << "parser" << parser_keyword;
        return false;
    }
    m_parser_module.reset(parser_module.release());
    return true;
}

bool
Assembler::isOkDebugFormat(llvm::StringRef dbgfmt_keyword) const
{
    std::vector<llvm::StringRef> dbgfmt_keywords =
        m_objfmt_module->getDebugFormatKeywords();
    return (std::find_if(dbgfmt_keywords.begin(), dbgfmt_keywords.end(),
                     NocaseEquals(dbgfmt_keyword))
            != dbgfmt_keywords.end());
}

bool
Assembler::setDebugFormat(llvm::StringRef dbgfmt_keyword, Diagnostic& diags)
{
    // Check to see if the requested debug format is in the allowed list
    // for the active object format.
    if (!isOkDebugFormat(dbgfmt_keyword))
    {
        diags.Report(SourceLocation(), diag::fatal_module_combo)
            << "debug format" << dbgfmt_keyword
            << "object format" << m_objfmt_module->getKeyword();
        return false;
    }

    std::auto_ptr<DebugFormatModule> dbgfmt_module =
        LoadModule<DebugFormatModule>(dbgfmt_keyword);
    if (dbgfmt_module.get() == 0)
    {
        diags.Report(SourceLocation(), diag::fatal_module_load)
            << "debug format" << dbgfmt_keyword;
        return false;
    }
    m_dbgfmt_module.reset(dbgfmt_module.release());
    return true;
}

bool
Assembler::setListFormat(llvm::StringRef listfmt_keyword, Diagnostic& diags)
{
    std::auto_ptr<ListFormatModule> listfmt_module =
        LoadModule<ListFormatModule>(listfmt_keyword);
    if (listfmt_module.get() == 0)
    {
        diags.Report(SourceLocation(), diag::fatal_module_load)
            << "list format" << listfmt_keyword;
        return false;
    }
    m_listfmt_module.reset(listfmt_module.release());
    return true;
}

bool
Assembler::InitObject(SourceManager& source_mgr, Diagnostic& diags)
{
    llvm::StringRef in_filename =
        source_mgr.getBuffer(source_mgr.getMainFileID())->getBufferIdentifier();
    llvm::StringRef parser_keyword = m_parser_module->getKeyword();

    // determine the object filename if not specified
    if (m_obj_filename.empty())
    {
        if (in_filename[0] == '\0')
            // Default to yasm.out if no obj filename specified
            m_obj_filename = "yasm.out";
        else
        {
            // replace (or add) extension to base filename
            llvm::sys::Path fn(in_filename);
            llvm::StringRef base_filename = fn.getBasename();
            if (base_filename.empty())
                m_obj_filename = "yasm.out";
            else
            {
                m_obj_filename = base_filename;
                m_obj_filename += m_objfmt_module->getExtension();
                if (m_obj_filename == in_filename)
                    m_obj_filename = "yasm.out";
            }
        }
    }

    if (m_machine.empty())
    {
        // If we're using x86 and the default objfmt bits is 64, default the
        // machine to amd64.  When we get more arches with multiple machines,
        // we should do this in a more modular fashion.
        if (m_arch_module->getKeyword().equals_lower("x86") &&
            m_objfmt_module->getDefaultX86ModeBits() == 64)
        {
            if (!setMachine("amd64", diags))
                return false;
        }
    }

    // Create object
    m_object.reset(new Object(in_filename, m_obj_filename, m_arch.get()));

    // See if the object format supports such an object
    if (!m_objfmt_module->isOkObject(*m_object))
    {
        diags.Report(SourceLocation(), diag::fatal_objfmt_machine_mismatch)
            << m_objfmt_module->getKeyword()
            << m_arch_module->getKeyword()
            << m_arch->getMachine();
        return false;
    }

    // Create the object format
    m_objfmt.reset(m_objfmt_module->Create(*m_object).release());

    // Add any object-format special symbols
    m_objfmt->InitSymbols(parser_keyword);

    // Add an initial "default" section to object
    m_object->setCurSection(m_objfmt->AddDefaultSection());

    // Default debug format if not specified
    if (m_dbgfmt_module.get() == 0)
    {
        if (!setDebugFormat(m_objfmt_module->getDefaultDebugFormatKeyword(),
                            diags))
            return false;
    }

    // See if the debug format supports such an object
    if (!m_dbgfmt_module->isOkObject(*m_object))
    {
        diags.Report(SourceLocation(), diag::fatal_module_combo)
            << "debug format" << m_dbgfmt_module->getKeyword()
            << "object format" << m_objfmt_module->getKeyword();
        return false;
    }

    // Create the debug format
    m_dbgfmt.reset(m_dbgfmt_module->Create(*m_object).release());

    return true;
}

Parser&
Assembler::InitParser(SourceManager& source_mgr,
                      Diagnostic& diags,
                      HeaderSearch& headers)
{
    m_parser.reset(m_parser_module->Create(diags, source_mgr, headers).release());
    return *m_parser;
}

bool
Assembler::Assemble(SourceManager& source_mgr, Diagnostic& diags)
{

    llvm::StringRef parser_keyword = m_parser_module->getKeyword();

    // Set up directive handlers
    Directives dirs;
    m_arch->AddDirectives(dirs, parser_keyword);
    m_parser->AddDirectives(dirs, parser_keyword);
    m_objfmt->AddDirectives(dirs, parser_keyword);
    m_dbgfmt->AddDirectives(dirs, parser_keyword);
    if (m_listfmt_module.get() != 0)
    {
        m_listfmt.reset(m_listfmt_module->Create().release());
        m_listfmt->AddDirectives(dirs, parser_keyword);
    }

    // Parse!
    m_parser->Parse(*m_object, dirs, diags);

    if (m_dump_time == Assembler::DUMP_AFTER_PARSE)
        m_object->Dump();
    if (diags.hasErrorOccurred())
        return false;

    // Finalize parse
    m_object->Finalize(diags);
    if (m_dump_time == Assembler::DUMP_AFTER_FINALIZE)
        m_object->Dump();
    if (diags.hasErrorOccurred())
        return false;

    // Optimize
    m_object->Optimize(diags);

    if (m_dump_time == Assembler::DUMP_AFTER_OPTIMIZE)
        m_object->Dump();

    if (diags.hasErrorOccurred())
        return false;

    // generate any debugging information
    m_dbgfmt->Generate(*m_objfmt, source_mgr, diags);

    return true;
}

bool
Assembler::Output(llvm::raw_fd_ostream& os, Diagnostic& diags)
{
    // Write the object file
    m_objfmt->Output(os,
                     !m_dbgfmt_module->getKeyword().equals_lower("null"),
                     *m_dbgfmt,
                     diags);

    if (m_dump_time == DUMP_AFTER_OUTPUT)
        m_object->Dump();

    if (diags.hasErrorOccurred())
        return false;

    return true;
}
