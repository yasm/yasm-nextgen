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
#include <util.h>

#include <cctype>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include <llvm/Support/CommandLine.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Support/MemoryBuffer.h>
#include <yasmx/Support/Compose.h>
#include <yasmx/Support/errwarn.h>
#include <yasmx/Support/nocase.h>
#include <yasmx/Support/registry.h>
#include <yasmx/System/plugin.h>
#include <yasmx/Arch.h>
#include <yasmx/Bytecode.h>
#include <yasmx/IntNum_iomanip.h>
#include <yasmx/Location.h>
#include <yasmx/Object.h>
#include <yasmx/ObjectFormat.h>
#include <yasmx/Reloc.h>
#include <yasmx/Section.h>
#include <yasmx/Symbol.h>

#include "frontends/license.cpp"

namespace cl = llvm::cl;

// version message
static const char* full_version = "yobjdump " PACKAGE_INTVER "." PACKAGE_BUILD;
void
PrintVersion()
{
    std::cout
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
PrintError(const std::string& msg)
{
    std::cerr << "yobjdump: " << msg << std::endl;
}

static void
PrintListKeywordDesc(const std::string& name, const std::string& keyword)
{
    std::cout << std::left << std::setfill(' ') << std::setw(12) << keyword
              << name << std::endl;
}

template <typename T>
static void
list_module()
{
    std::vector<std::string> list = yasm::getModules<T>();
    for (std::vector<std::string>::iterator i=list.begin(), end=list.end();
         i != end; ++i)
    {
        std::auto_ptr<T> obj = yasm::LoadModule<T>(*i);
        PrintListKeywordDesc(obj->getName(), *i);
    }
}

static const char *
handle_yasm_gettext(const char *msgid)
{
    return yasm_gettext(msgid);
}

static void
DumpSectionHeaders(const yasm::Object& object)
{
    std::cout << "Sections:\n";
    std::cout << "Idx Name          Size      ";
    unsigned int bits = 64; // FIXME
    std::cout << std::left << std::setfill(' ');
    std::cout << std::setw(bits/4) << "VMA" << "  ";
    std::cout << std::setw(bits/4) << "LMA" << "  ";
    std::cout << "File off  Algn\n";

    unsigned int idx = 0;
    for (yasm::Object::const_section_iterator sect=object.sections_begin(),
         end=object.sections_end(); sect != end; ++sect, ++idx)
    {
        std::cout << std::setfill(' ') << std::dec;
        std::cout << std::right << std::setw(3) << idx << ' ';
        std::cout << std::left << std::setw(13) << sect->getName() << ' ';
        std::cout << std::hex;
        std::cout << yasm::set_intnum_bits(32);
        std::cout << yasm::IntNum(sect->bytecodes_last().getNextOffset())
                  << "  ";
        std::cout << yasm::set_intnum_bits(bits);
        std::cout << sect->getVMA() << "  ";
        std::cout << sect->getLMA() << "  ";
        std::cout << yasm::set_intnum_bits(32);
        std::cout << yasm::IntNum(sect->getFilePos()) << "  ";
        std::cout << std::dec << sect->getAlign();
        std::cout << '\n';
    }
}

static void
DumpSymbols(const yasm::Object& object)
{
    std::cout << "SYMBOL TABLE:\n";
    unsigned int bits = 64; // FIXME
    std::cout << yasm::set_intnum_bits(bits);
    for (yasm::Object::const_symbol_iterator sym=object.symbols_begin(),
         end=object.symbols_end(); sym != end; ++sym)
    {
        std::cout << std::hex;

        yasm::Location loc;
        bool is_label = sym->getLabel(&loc);
        const yasm::Expr* equ = sym->getEqu();

        if (is_label)
            std::cout << yasm::IntNum(loc.getOffset());
        else if (equ)
            std::cout << *equ;
        else
            std::cout << yasm::IntNum(0);
        std::cout << std::left;
        std::cout << "  ";
        // TODO: symbol flags
        int vis = sym->getVisibility();
        if (is_label)
        {
            std::cout << loc.bc->getContainer()->AsSection()->getName() << '\t';
        }
        else if (sym->getEqu())
            std::cout << "*ABS*\t";
        else if ((vis & yasm::Symbol::EXTERN) != 0)
            std::cout << "*UND*\t";
        else if ((vis & yasm::Symbol::COMMON) != 0)
            std::cout << "*COM*\t";
        std::cout << sym->getName();
        std::cout << '\n';
    }
}

static void
DumpRelocs(const yasm::Object& object)
{
    unsigned int bits = 64; // FIXME
    std::cout << yasm::set_intnum_bits(bits);

    for (yasm::Object::const_section_iterator sect=object.sections_begin(),
         end=object.sections_end(); sect != end; ++sect)
    {
        if (sect->getRelocs().empty())
            continue;

        std::cout << "RELOCATION RECORDS FOR [" << sect->getName() << "]:\n";
        std::cout << std::left << std::setfill(' ');
        std::cout << std::setw(bits/4) << "OFFSET";
        std::cout << " TYPE              VALUE\n";

        for (yasm::Section::const_reloc_iterator reloc=sect->relocs_begin(),
             endr=sect->relocs_end(); reloc != endr; ++reloc)
        {
            std::cout << std::noshowbase;
            std::cout << std::hex << (sect->getVMA()+reloc->getAddress())
                      << ' ';
            std::cout << std::setw(16) << reloc->getTypeName() << "  ";
            std::cout << std::showbase;
            std::cout << reloc->getValue();
            std::cout << '\n';
        }
        std::cout << std::noshowbase;
        std::cout << "\n\n";
    }
}

static void
DumpContentsLine(const yasm::IntNum& addr, const unsigned char* data, int len)
{
    // address
    std::cout << ' ' << addr;

    // hex dump
    for (int i=0; i<16; ++i)
    {
        if ((i & 3) == 0)
            std::cout << ' ';
        if (i<len)
            std::cout << std::setw(2) << static_cast<unsigned int>(data[i]);
        else
            std::cout << "  ";
    }

    // ascii dump
    std::cout << "  ";
    for (int i=0; i<16; ++i)
    {
        if (i>=len)
            std::cout << ' ';
        else if (!std::isprint(data[i]))
            std::cout << '.';
        else
            std::cout << data[i];
    }

    std::cout << '\n';
}

static void
DumpContents(const yasm::Object& object)
{
    std::cout << std::hex << std::setfill('0') << std::right;

    for (yasm::Object::const_section_iterator sect=object.sections_begin(),
         end=object.sections_end(); sect != end; ++sect)
    {
        if (sect->isBSS())
            continue;

        unsigned long size = sect->bytecodes_last().getNextOffset();
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
        std::cout << yasm::set_intnum_bits(addr_bits);

        std::cout << "Contents of section " << sect->getName() << ":\n";

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
                    DumpContentsLine(addr, line, 16);
                    addr += 16;
                    line_pos = 0;
                }
            }
        }

        // output any remaining
        if (line_pos != 0)
            DumpContentsLine(addr, line, line_pos);
    }

    std::cout << std::dec << std::setfill(' ');
}

static void
DoDump(const std::string& in_filename)
{
    // open the input file
    std::auto_ptr<llvm::MemoryBuffer>
        in_file(llvm::MemoryBuffer::getFileOrSTDIN(in_filename));
    if (in_file.get() == 0)
        throw yasm::Error(String::Compose(_("could not open file `%1'"),
                          in_filename));

    std::auto_ptr<yasm::ObjectFormatModule> objfmt_module(0);
    std::string arch_keyword, machine;

    if (!objfmt_keyword.empty())
    {
        objfmt_keyword = String::Lowercase(objfmt_keyword);
        if (!yasm::isModule<yasm::ObjectFormatModule>(objfmt_keyword))
        {
            throw yasm::Error(String::Compose(
                _("unrecognized object format `%1'"), objfmt_keyword));
        }

        // Object format forced by user
        objfmt_module =
            yasm::LoadModule<yasm::ObjectFormatModule>(objfmt_keyword);

        if (objfmt_module.get() == 0)
        {
            throw yasm::Error(String::Compose(
                _("could not load object format `%1'"), objfmt_keyword));
        }

        if (!objfmt_module->Taste(*in_file, &arch_keyword, &machine))
        {
            throw yasm::Error(String::Compose(
                _("file is not recognized as a `%1' object file"),
                objfmt_module->getKeyword()));
        }
    }
    else
    {
        // Need to loop through available object formats, and taste each one
        std::vector<std::string> list =
            yasm::getModules<yasm::ObjectFormatModule>();
        std::vector<std::string>::iterator i=list.begin(), end=list.end();
        for (; i != end; ++i)
        {
            objfmt_module = yasm::LoadModule<yasm::ObjectFormatModule>(*i);
            if (objfmt_module->Taste(*in_file, &arch_keyword, &machine))
                break;
        }
        if (i == end)
        {
            throw yasm::Error(_("File format not recognized"));
        }
    }

    std::auto_ptr<yasm::ArchModule> arch_module =
        yasm::LoadModule<yasm::ArchModule>(arch_keyword);
    if (arch_module.get() == 0)
        throw yasm::Error(String::Compose(
            _("could not load architecture `%1'"), arch_keyword));

    std::auto_ptr<yasm::Arch> arch = arch_module->Create();
    if (!arch->setMachine(machine))
    {
        throw yasm::Error(String::Compose(
            _("`%1' is not a valid machine for architecture `%2'"),
            machine, arch_module->getKeyword()));
    }

    yasm::Object object("", in_filename, arch.get());

    if (!objfmt_module->isOkObject(object))
    {
        throw yasm::Error(String::Compose(
            _("object format `%1' does not support architecture `%2' machine `%3'"),
            objfmt_module->getKeyword(),
            arch_module->getKeyword(),
            arch->getMachine()));
    }

    std::auto_ptr<yasm::ObjectFormat> objfmt = objfmt_module->Create(object);
    objfmt->Read(*in_file);

    std::cout << in_filename << ":     file format "
              << objfmt_module->getKeyword() << "\n\n";

    if (show_section_headers)
        DumpSectionHeaders(object);
    if (show_symbols)
        DumpSymbols(object);
    if (show_relocs)
        DumpRelocs(object);
    if (show_contents)
        DumpContents(object);
}

int
main(int argc, char* argv[])
{
    llvm::llvm_shutdown_obj llvm_manager(false);
#if 0
#if defined(HAVE_SETLOCALE) && defined(HAVE_LC_MESSAGES)
    setlocale(LC_MESSAGES, "");
#endif
#if defined(LOCALEDIR)
    yasm_bindtextdomain(PACKAGE, LOCALEDIR);
#endif
    yasm_textdomain(PACKAGE);
#endif

    // Initialize errwarn handling
    yasm::gettext_hook = handle_yasm_gettext;

    // Load standard modules
    if (!yasm::LoadStandardPlugins())
    {
        PrintError(_("FATAL: could not load standard modules"));
        return EXIT_FAILURE;
    }

    cl::SetVersionPrinter(&PrintVersion);
    cl::ParseCommandLineOptions(argc, argv);

    if (show_all_headers)
    {
        show_file_headers = true;
        show_section_headers = true;
        show_private_headers = true;
        show_relocs = true;
        show_symbols = true;
    }

    if (show_help)
        cl::PrintHelpMessage();

    if (show_license)
    {
        for (std::size_t i=0; i<NELEMS(license_msg); i++)
            std::cout << license_msg[i] << '\n';
        return EXIT_SUCCESS;
    }

    if (show_info)
    {
        std::cout << full_version << '\n';
        list_module<yasm::ObjectFormatModule>();
        return EXIT_SUCCESS;
    }

    // Determine input filename and open input file.
    if (in_filenames.empty())
    {
        PrintError(_("No input file specified"));
        return EXIT_FAILURE;
    }

    int retval = EXIT_SUCCESS;

    for (std::vector<std::string>::const_iterator i=in_filenames.begin(),
         end=in_filenames.end(); i != end; ++i)
    {
        try
        {
            DoDump(*i);
        }
        catch (yasm::Error& err)
        {
            PrintError(String::Compose(_("%1: %2"), *i, err.what()));
            retval = EXIT_FAILURE;
        }
        catch (std::out_of_range& err)
        {
            PrintError(String::Compose(
                _("%1: out of range error while reading (corrupt file?)"), *i));
            retval = EXIT_FAILURE;
        }
    }
    return retval;
}

