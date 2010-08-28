//
// Object dumper entry point, command line parsing
//
//  Copyright (C) 2008  Peter Johnson
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
#include "config.h"

#include <cctype>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"
#include "yasmx/Basic/Diagnostic.h"
#include "yasmx/Basic/FileManager.h"
#include "yasmx/Basic/SourceManager.h"
#include "yasmx/Support/Compose.h"
#include "yasmx/Support/registry.h"
#include "yasmx/System/plugin.h"
#include "yasmx/Arch.h"
#include "yasmx/Bytecode.h"
#include "yasmx/Location.h"
#include "yasmx/Object.h"
#include "yasmx/ObjectFormat.h"
#include "yasmx/Reloc.h"
#include "yasmx/Section.h"
#include "yasmx/Symbol.h"

#include "frontends/license.cpp"
#include "frontends/OffsetDiagnosticPrinter.h"

namespace cl = llvm::cl;

// version message
static const char* full_version = "yobjdump " PACKAGE_INTVER "." PACKAGE_BUILD;
void
PrintVersion()
{
    llvm::outs()
        << full_version << '\n'
        << "Compiled on " __DATE__ ".\n"
        << "Copyright (c) 2001-2009 Peter Johnson and other Yasm developers.\n"
        << "Run yobjdump --license for licensing overview and summary.\n";
}

// extra help messages
static cl::extrahelp help_tail(
    "\n"
    "Files are object files to be dumped.\n"
    "\n"
    "Sample invocation:\n"
    "   yobjdump object.o\n"
    "\n"
    "Report bugs to bug-yasm@tortall.net\n");

static cl::list<std::string> in_filenames(cl::Positional,
    cl::desc("objfile..."));

// -b, --target
static cl::opt<std::string> objfmt_keyword("b",
    cl::desc("Select object format"),
    cl::value_desc("target"),
    cl::Prefix);
static cl::alias objfmt_keyword_long("target",
    cl::desc("Alias for -b"),
    cl::value_desc("target"),
    cl::aliasopt(objfmt_keyword));

// -H
static cl::opt<bool> show_help("H",
    cl::desc("Alias for --help"),
    cl::Hidden);

// --license
static cl::opt<bool> show_license("license",
    cl::desc("Show license text"));

// -i, --info
static cl::opt<bool> show_info("i",
    cl::desc("List available object formats"));
static cl::alias show_info_long("info",
    cl::desc("Alias for -i"),
    cl::aliasopt(show_info));

// -f, --file-headers
static cl::opt<bool> show_file_headers("f",
    cl::desc("Display summary information from the overall header"),
    cl::Grouping);
static cl::alias show_file_headers_long("file-headers",
    cl::desc("Alias for -f"),
    cl::aliasopt(show_file_headers));

// -h, --file-headers, --headers
static cl::opt<bool> show_section_headers("h",
    cl::desc("Display summary information from the section headers"),
    cl::Grouping);
static cl::alias show_section_headers_long("section-headers",
    cl::desc("Alias for -h"),
    cl::aliasopt(show_section_headers));
static cl::alias show_section_headers_long2("headers",
    cl::desc("Alias for -h"),
    cl::aliasopt(show_section_headers));

// -p, --private-headers
static cl::opt<bool> show_private_headers("p",
    cl::desc("Display information that is specific to the object format"),
    cl::Grouping);
static cl::alias show_private_headers_long("private-headers",
    cl::desc("Alias for -p"),
    cl::aliasopt(show_private_headers));

// -r, --reloc
static cl::opt<bool> show_relocs("r",
    cl::desc("Display relocation entries"),
    cl::Grouping);
static cl::alias show_relocs_long("reloc",
    cl::desc("Alias for -r"),
    cl::aliasopt(show_relocs));

// -t, --syms
static cl::opt<bool> show_symbols("t",
    cl::desc("Display symbol table entries"),
    cl::Grouping);
static cl::alias show_symbols_long("syms",
    cl::desc("Alias for -t"),
    cl::aliasopt(show_symbols));

// -s, --full-contents
static cl::opt<bool> show_contents("s",
    cl::desc("Display full contents of sections"),
    cl::Grouping);
static cl::alias show_contents_long("full-contents",
    cl::desc("Alias for -s"),
    cl::aliasopt(show_contents));

// -x, --all-headers
static cl::opt<bool> show_all_headers("x",
    cl::desc("Display all available header information (-f -h -p -r -t)"),
    cl::Grouping);
static cl::alias show_all_headers_long("all-headers",
    cl::desc("Alias for -x"),
    cl::aliasopt(show_all_headers));

static void
PrintListKeywordDesc(const std::string& name, const std::string& keyword)
{
    llvm::outs() << llvm::format("%-12s", keyword.c_str()) << name << '\n';
}

template <typename T>
static void
list_module()
{
    yasm::ModuleNames list = yasm::getModules<T>();
    for (yasm::ModuleNames::iterator i=list.begin(), end=list.end();
         i != end; ++i)
    {
        std::auto_ptr<T> obj = yasm::LoadModule<T>(*i);
        PrintListKeywordDesc(obj->getName(), *i);
    }
}

static void
DumpSectionHeaders(const yasm::Object& object)
{
    llvm::raw_ostream& os = llvm::outs();
    os << "Sections:\n"
       << "Idx Name          Size      ";
    unsigned int bits = 64; // FIXME
    os << llvm::format("%-*s", bits/4, (const char*)"VMA") << "  "
       << llvm::format("%-*s", bits/4, (const char*)"LMA") << "  "
       << "File off  Algn\n";

    unsigned int idx = 0;
    for (yasm::Object::const_section_iterator sect=object.sections_begin(),
         end=object.sections_end(); sect != end; ++sect, ++idx)
    {
        os << llvm::format("%3d", idx) << ' '
           << llvm::format("%-13s", sect->getName().str().c_str()) << ' ';
        yasm::IntNum(sect->bytecodes_back().getNextOffset())
            .Print(os, 16, true, false, 32);
        os << "  ";
        sect->getVMA().Print(os, 16, true, false, bits);
        os << "  ";
        sect->getLMA().Print(os, 16, true, false, bits);
        os << "  ";
        yasm::IntNum(sect->getFilePos()).Print(os, 16, true, false, 32);
        os << "  "
           << sect->getAlign()
           << '\n';
    }
}

static void
DumpSymbols(const yasm::Object& object)
{
    llvm::raw_ostream& os = llvm::outs();
    os << "SYMBOL TABLE:\n";
    unsigned int bits = 64; // FIXME
    for (yasm::Object::const_symbol_iterator sym=object.symbols_begin(),
         end=object.symbols_end(); sym != end; ++sym)
    {
        yasm::Location loc;
        bool is_label = sym->getLabel(&loc);
        const yasm::Expr* equ = sym->getEqu();

        if (is_label)
            yasm::IntNum(loc.getOffset()).Print(os, 16, true, false, bits);
        else if (equ)
            equ->Print(os, 16);
        else
            yasm::IntNum(0).Print(os, 16, true, false, bits);

        os << "  ";
        // TODO: symbol flags
        int vis = sym->getVisibility();
        if (is_label)
            os << loc.bc->getContainer()->AsSection()->getName() << '\t';
        else if (sym->getEqu())
            os << "*ABS*\t";
        else if ((vis & yasm::Symbol::EXTERN) != 0)
            os << "*UND*\t";
        else if ((vis & yasm::Symbol::COMMON) != 0)
            os << "*COM*\t";
        os << sym->getName()
           << '\n';
    }
}

static void
DumpRelocs(const yasm::Object& object)
{
    llvm::raw_ostream& os = llvm::outs();
    unsigned int bits = 64; // FIXME

    for (yasm::Object::const_section_iterator sect=object.sections_begin(),
         end=object.sections_end(); sect != end; ++sect)
    {
        if (sect->getRelocs().empty())
            continue;

        os << "RELOCATION RECORDS FOR [" << sect->getName() << "]:\n"
           << llvm::format("%-*s", bits/4, (const char*)"OFFSET")
           << " TYPE              VALUE\n";

        for (yasm::Section::const_reloc_iterator reloc=sect->relocs_begin(),
             endr=sect->relocs_end(); reloc != endr; ++reloc)
        {
            (sect->getVMA()+reloc->getAddress())
                .Print(os, 16, true, false, bits);
            os << ' ';
            os << llvm::format("%-16s", reloc->getTypeName().c_str()) << "  ";
            reloc->getValue().Print(os, 16);
            os << '\n';
        }
        os << "\n\n";
    }
}

static void
DumpContentsLine(const yasm::IntNum& addr,
                 const unsigned char* data,
                 int len,
                 int addr_bits)
{
    llvm::raw_ostream& os = llvm::outs();

    // address
    os << ' ';
    addr.Print(os, 16, true, false, addr_bits);

    // hex dump
    for (int i=0; i<16; ++i)
    {
        if ((i & 3) == 0)
            os << ' ';
        if (i<len)
            os << llvm::format("%02x", static_cast<unsigned int>(data[i]));
        else
            os << "  ";
    }

    // ascii dump
    os << "  ";
    for (int i=0; i<16; ++i)
    {
        if (i>=len)
            os << ' ';
        else if (!std::isprint(data[i]))
            os << '.';
        else
            os << data[i];
    }

    os << '\n';
}

static void
DumpContents(const yasm::Object& object)
{
    llvm::raw_ostream& os = llvm::outs();

    for (yasm::Object::const_section_iterator sect=object.sections_begin(),
         end=object.sections_end(); sect != end; ++sect)
    {
        if (sect->isBSS())
            continue;

        unsigned long size = sect->bytecodes_back().getNextOffset();
        if (size == 0)
            continue;   // empty

        // figure out how many hex digits we should have for the address
        yasm::IntNum last_addr = sect->getVMA() + size;
        unsigned int addr_bits = 0;
        while (!last_addr.isZero())
        {
            last_addr >>= 1;
            ++addr_bits;
        }
        if (addr_bits < 16)
            addr_bits = 16;

        os << "Contents of section " << sect->getName() << ":\n";

        unsigned char line[16];
        int line_pos = 0;
        yasm::IntNum addr = sect->getVMA();

        for (yasm::Section::const_bc_iterator bc=sect->bytecodes_begin(),
             endbc=sect->bytecodes_end(); bc != endbc; ++bc)
        {
            // XXX: only outputs fixed portions
            const yasm::Bytes& fixed = bc->getFixed();
            long fixed_pos = 0;
            long fixed_size = fixed.size();
            while (fixed_pos < fixed_size)
            {
                long tocopy = 16-line_pos;
                if (tocopy > (fixed_size-fixed_pos))
                    tocopy = fixed_size-fixed_pos;
                std::memcpy(&line[line_pos], &fixed[fixed_pos], tocopy);
                line_pos += tocopy;
                fixed_pos += tocopy;

                // when we've filled up a line, output it.
                if (line_pos == 16)
                {
                    DumpContentsLine(addr, line, 16, addr_bits);
                    addr += 16;
                    line_pos = 0;
                }
            }
        }

        // output any remaining
        if (line_pos != 0)
            DumpContentsLine(addr, line, line_pos, addr_bits);
    }
}

static int
DoDump(const std::string& in_filename,
       yasm::SourceManager& source_mgr,
       yasm::Diagnostic& diags)
{
    yasm::FileManager file_mgr;

    // open the input file or STDIN (for filename of "-")
    if (in_filename == "-")
    {
        source_mgr.createMainFileIDForMemBuffer(llvm::MemoryBuffer::getSTDIN());
    }
    else
    {
        const yasm::FileEntry* in = file_mgr.getFile(in_filename);
        if (!in)
        {
            diags.Report(yasm::SourceLocation(), yasm::diag::err_file_open)
                << in_filename;
        }
        source_mgr.createMainFileID(in, yasm::SourceLocation());
    }

    const llvm::MemoryBuffer* in_file =
        source_mgr.getBuffer(source_mgr.getMainFileID());
    yasm::SourceLocation sloc =
        source_mgr.getLocForStartOfFile(source_mgr.getMainFileID());

    std::auto_ptr<yasm::ObjectFormatModule> objfmt_module(0);
    std::string arch_keyword, machine;

    if (!objfmt_keyword.empty())
    {
        objfmt_keyword = llvm::LowercaseString(objfmt_keyword);
        if (!yasm::isModule<yasm::ObjectFormatModule>(objfmt_keyword))
        {
            diags.Report(sloc, yasm::diag::err_unrecognized_object_format)
                << objfmt_keyword;
            return EXIT_FAILURE;
        }

        // Object format forced by user
        objfmt_module =
            yasm::LoadModule<yasm::ObjectFormatModule>(objfmt_keyword);

        if (objfmt_module.get() == 0)
        {
            diags.Report(sloc, yasm::diag::fatal_module_load)
                << "object format" << objfmt_keyword;
            return EXIT_FAILURE;
        }

        if (!objfmt_module->Taste(*in_file, &arch_keyword, &machine))
        {
            diags.Report(sloc, yasm::diag::err_unrecognized_object_file)
                << objfmt_module->getKeyword();
            return EXIT_FAILURE;
        }
    }
    else
    {
        // Need to loop through available object formats, and taste each one
        yasm::ModuleNames list = yasm::getModules<yasm::ObjectFormatModule>();
        yasm::ModuleNames::iterator i=list.begin(), end=list.end();
        for (; i != end; ++i)
        {
            objfmt_module = yasm::LoadModule<yasm::ObjectFormatModule>(*i);
            if (objfmt_module->Taste(*in_file, &arch_keyword, &machine))
                break;
        }
        if (i == end)
        {
            diags.Report(sloc, yasm::diag::err_unrecognized_file_format);
            return EXIT_FAILURE;
        }
    }

    std::auto_ptr<yasm::ArchModule> arch_module =
        yasm::LoadModule<yasm::ArchModule>(arch_keyword);
    if (arch_module.get() == 0)
    {
        diags.Report(sloc, yasm::diag::fatal_module_load)
            << "architecture" << arch_keyword;
        return EXIT_FAILURE;
    }

    std::auto_ptr<yasm::Arch> arch = arch_module->Create();
    if (!arch->setMachine(machine))
    {
        diags.Report(sloc, yasm::diag::fatal_module_combo)
            << "machine" << machine
            << "architecture" << arch_module->getKeyword();
        return EXIT_FAILURE;
    }

    yasm::Object object("", in_filename, arch.get());

    if (!objfmt_module->isOkObject(object))
    {
        diags.Report(sloc, yasm::diag::fatal_objfmt_machine_mismatch)
            << objfmt_module->getKeyword()
            << arch_module->getKeyword()
            << arch->getMachine();
        return EXIT_FAILURE;
    }

    std::auto_ptr<yasm::ObjectFormat> objfmt = objfmt_module->Create(object);
    if (!objfmt->Read(source_mgr, diags))
        return EXIT_FAILURE;

    llvm::outs() << in_filename << ":     file format "
                 << objfmt_module->getKeyword() << "\n\n";

    if (show_section_headers)
        DumpSectionHeaders(object);
    if (show_symbols)
        DumpSymbols(object);
    if (show_relocs)
        DumpRelocs(object);
    if (show_contents)
        DumpContents(object);
    return EXIT_SUCCESS;
}

int
main(int argc, char* argv[])
{
    llvm::llvm_shutdown_obj llvm_manager(false);

    cl::SetVersionPrinter(&PrintVersion);
    cl::ParseCommandLineOptions(argc, argv);

    if (show_help)
        cl::PrintHelpMessage();

    if (show_license)
    {
        for (std::size_t i=0; i<sizeof(license_msg)/sizeof(license_msg[0]); i++)
            llvm::outs() << license_msg[i] << '\n';
        return EXIT_SUCCESS;
    }

    if (show_info)
    {
        llvm::outs() << full_version << '\n';
        list_module<yasm::ObjectFormatModule>();
        return EXIT_SUCCESS;
    }

    yasm::OffsetDiagnosticPrinter diag_printer(llvm::errs());
    yasm::Diagnostic diags(&diag_printer);
    yasm::SourceManager source_mgr(diags);
    diags.setSourceManager(&source_mgr);
    diag_printer.setPrefix("yobjdump");

    // Load standard modules
    if (!yasm::LoadStandardPlugins())
    {
        diags.Report(yasm::diag::fatal_standard_modules);
        return EXIT_FAILURE;
    }

    if (show_all_headers)
    {
        show_file_headers = true;
        show_section_headers = true;
        show_private_headers = true;
        show_relocs = true;
        show_symbols = true;
    }

    // Determine input filename and open input file.
    if (in_filenames.empty())
    {
        diags.Report(yasm::diag::fatal_no_input_files);
        return EXIT_FAILURE;
    }

    int retval = EXIT_SUCCESS;

    for (std::vector<std::string>::const_iterator i=in_filenames.begin(),
         end=in_filenames.end(); i != end; ++i)
    {
        try
        {
            if (DoDump(*i, source_mgr, diags) != EXIT_SUCCESS)
                retval = EXIT_FAILURE;
        }
        catch (std::out_of_range& err)
        {
            llvm::errs() << *i << ": "
                << "out of range error while reading (corrupt file?)\n";
            retval = EXIT_FAILURE;
        }
    }
    return retval;
}

