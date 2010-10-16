//
// GNU AS-like frontend
//
//  Copyright (C) 2001-2010  Peter Johnson
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

#include <getopt.h>
#include <memory>

#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"
#include "yasmx/Basic/Diagnostic.h"
#include "yasmx/Basic/FileManager.h"
#include "yasmx/Basic/SourceManager.h"
#include "yasmx/Parse/HeaderSearch.h"
#include "yasmx/Parse/Parser.h"
#include "yasmx/Support/registry.h"
#include "yasmx/System/plugin.h"
#include "yasmx/Arch.h"
#include "yasmx/Assembler.h"
#include "yasmx/DebugFormat.h"
#include "yasmx/Expr.h"
#include "yasmx/IntNum.h"
#include "yasmx/ListFormat.h"
#include "yasmx/Module.h"
#include "yasmx/Object.h"
#include "yasmx/ObjectFormat.h"
#include "yasmx/Symbol.h"

#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif

#include "license.cpp"
#include "frontends/DiagnosticOptions.h"
#include "frontends/TextDiagnosticPrinter.h"


// version message
static const char* version_msg =
    PACKAGE_NAME " " PACKAGE_INTVER "." PACKAGE_BUILD "\n"
    "Compiled on " __DATE__ ".\n"
    "Copyright (c) 2001-2010 Peter Johnson and other Yasm developers.\n"
    "Run ygas --license for licensing overview and summary.\n";

// help message
static const char* help_msg =
    "USAGE: ygas [options] file\n"
    "\n"
    "OPTIONS:\n"
    "  -32              - set 32-bit output\n"
    "  -64              - set 64-bit output\n"
    "  -I <path>        - Add include path\n"
    "  -J               - don't warn about signed overflow\n"
    "  -W               - Suppress warning messages\n"
    "  -defsym <string> - define symbol\n"
    "  -fatal-warnings  - Suppress warning messages\n"
    "  -help            - Display available options (-help-hidden for more)\n"
    "  -license         - Show license text\n"
    "  -o <filename>    - Name of object-file output\n"
    "  -version         - Display the version of this program\n"
    "  -warn            - Don't suppress warning messages or treat them as errors\n"
    "\n"
    "Files are asm sources to be assembled.\n"
    "\n"
    "Sample invocation:\n"
    "   ygas -32 -o object.o source.s\n"
    "\n"
    "Report bugs to bug-yasm@tortall.net\n";

static std::string objfmt_bits = YGAS_OBJFMT_BITS;
static std::vector<std::string> defsym;
static std::vector<std::string> include_paths;
static std::string obj_filename;
static std::string in_filename = "-";

enum longopt_val {
    OPT_32 = 1,
    OPT_64,
    OPT_defsym,
    OPT_warn,
    OPT_fatal_warnings,
    OPT_help,
    OPT_version,
    OPT_license
};
static const char* shortopts = "DJI:o:wxWQ";
static struct option longopts[] =
{
    { "32",             no_argument,        NULL,   OPT_32 },
    { "64",             no_argument,        NULL,   OPT_64 },
    { "defsym",         required_argument,  NULL,   OPT_defsym },
    { "warn",           no_argument,        NULL,   OPT_warn },
    { "fatal-warnings", no_argument,        NULL,   OPT_fatal_warnings },
    { "help",           no_argument,        NULL,   OPT_help },
    { "version",        no_argument,        NULL,   OPT_version },
    { "license",        no_argument,        NULL,   OPT_license },
    { "no-warn",        no_argument,        NULL,   'W' },
    { NULL,             0,                  NULL,   0 },
};

static int
do_assemble(SourceManager& source_mgr, Diagnostic& diags)
{
    FileManager file_mgr;
    Assembler assembler("x86", YGAS_OBJFMT_BASE + objfmt_bits, diags);
    HeaderSearch headers(file_mgr);

    if (diags.hasFatalErrorOccurred())
        return EXIT_FAILURE;

    // Set object filename if specified.
    if (!obj_filename.empty())
        assembler.setObjectFilename(obj_filename);

    // Set parser.
    assembler.setParser("gas", diags);

    if (diags.hasFatalErrorOccurred())
        return EXIT_FAILURE;

    // Set debug format to dwarf2pass if it's legal for this object format.
    if (assembler.isOkDebugFormat("dwarf2pass"))
    {
        assembler.setDebugFormat("dwarf2pass", diags);
        if (diags.hasFatalErrorOccurred())
            return EXIT_FAILURE;
    }

    // open the input file or STDIN (for filename of "-")
    if (in_filename == "-")
    {
        source_mgr.createMainFileIDForMemBuffer(llvm::MemoryBuffer::getSTDIN());
    }
    else
    {
        const FileEntry* in = file_mgr.getFile(in_filename);
        if (!in)
        {
            diags.Report(SourceLocation(), diag::fatal_file_open)
                << in_filename;
            return EXIT_FAILURE;
        }
        source_mgr.createMainFileID(in, SourceLocation());
    }

    // Initialize the object.
    if (!assembler.InitObject(source_mgr, diags))
        return EXIT_FAILURE;

    // Predefine symbols.
    for (std::vector<std::string>::const_iterator i=defsym.begin(),
         end=defsym.end(); i != end; ++i)
    {
        llvm::StringRef str(*i);
        size_t equalpos = str.find('=');
        if (equalpos == llvm::StringRef::npos)
        {
            diags.Report(diag::fatal_bad_defsym) << str;
            continue;
        }
        llvm::StringRef name = str.slice(0, equalpos);
        llvm::StringRef vstr = str.slice(equalpos+1, llvm::StringRef::npos);

        IntNum value;
        if (!vstr.empty())
        {
            // determine radix
            unsigned int radix;
            if (vstr[0] == '0' && vstr.size() > 1 &&
                (vstr[1] == 'x' || vstr[1] == 'X'))
            {
                vstr = vstr.substr(2);
                radix = 16;
            }
            else if (vstr[0] == '0')
            {
                vstr = vstr.substr(1);
                radix = 8;
            }
            else
                radix = 10;

            // check validity
            const char* ptr = vstr.begin();
            const char* end = vstr.end();
            if (radix == 16)
            {
                while (ptr != end && isxdigit(*ptr))
                    ++ptr;
            }
            else if (radix == 8)
            {
                while (ptr != end && (*ptr >= '0' && *ptr <= '7'))
                    ++ptr;
            }
            else
            {
                while (ptr != end && isdigit(*ptr))
                    ++ptr;
            }
            if (ptr != end)
            {
                diags.Report(diag::fatal_bad_defsym) << name;
                continue;
            }
            value.setStr(vstr, radix);
        }

        // define equ
        assembler.getObject()->getSymbol(name)->DefineEqu(Expr(value));
    }

    if (diags.hasFatalErrorOccurred())
        return EXIT_FAILURE;

    // Assemble the input.
    if (!assembler.Assemble(source_mgr, file_mgr, diags, headers))
    {
        // An error occurred during assembly.
        return EXIT_FAILURE;
    }

    // open the object file for output
    std::string err;
    llvm::raw_fd_ostream out(assembler.getObjectFilename().str().c_str(),
                             err, llvm::raw_fd_ostream::F_Binary);
    if (!err.empty())
    {
        diags.Report(SourceLocation(), diag::err_cannot_open_file)
            << obj_filename << err;
        return EXIT_FAILURE;
    }

    if (!assembler.Output(out, diags))
    {
        // An error occurred during output.
        // If we had an error at this point, we also need to delete the output
        // object file (to make sure it's not left newer than the source).
        out.close();
        remove(assembler.getObjectFilename().str().c_str());
        return EXIT_FAILURE;
    }

    // close object file
    out.close();
    return EXIT_SUCCESS;
}

// main function
int
main(int argc, char* argv[])
{
    DiagnosticOptions diag_opts;
    diag_opts.ShowOptionNames = 1;
    diag_opts.ShowSourceRanges = 1;
    TextDiagnosticPrinter diag_printer(llvm::errs(), diag_opts);
    Diagnostic diags(&diag_printer);
    SourceManager source_mgr(diags);
    diags.setSourceManager(&source_mgr);
    diag_printer.setPrefix("ygas");

    // Disable init-nobits and uninit-contents by default.
    diags.setDiagnosticGroupMapping("init-nobits", diag::MAP_IGNORE);
    diags.setDiagnosticGroupMapping("uninit-contents", diag::MAP_IGNORE);

    // Parse command line options
    for (;;)
    {
        int longindex;
        int ch = getopt_long_only(argc, argv, shortopts, longopts, &longindex);
        if (ch == -1)
            break;
        switch (ch)
        {
            case 'D': case 'w': case 'x': case 'Q':
                break; // ignored
            case 'I':
                include_paths.push_back(optarg);
                break;
            case 'o':
                obj_filename = optarg;
                break;
            case OPT_32:
                objfmt_bits = "32";
                break;
            case OPT_64:
                objfmt_bits = "64";
                break;
            case OPT_defsym:
                defsym.push_back(optarg);
                break;
            case 'J':
                // Signed overflow warning disable
                diags.setDiagnosticGroupMapping("signed-overflow",
                                                diag::MAP_IGNORE);
                break;
            case OPT_warn:
                // Enable all warnings
                diags.setIgnoreAllWarnings(false);
                diags.setWarningsAsErrors(false);
                diags.setDiagnosticGroupMapping("signed-overflow",
                                                diag::MAP_WARNING);
                break;
            case OPT_fatal_warnings:
                // Make warnings fatal (errors)
                diags.setWarningsAsErrors(true);
                break;
            case 'W':
                // Inhibit all warnings
                diags.setIgnoreAllWarnings(true);
                break;
            case OPT_help:
                llvm::outs() << help_msg;
                return EXIT_SUCCESS;
            case OPT_version:
                llvm::outs() << version_msg;
                return EXIT_SUCCESS;
            case OPT_license:
                llvm::outs() << license_msg;
                return EXIT_SUCCESS;
            default:
                break;
        }
    }

    argc -= optind;
    argv += optind;

    if (argc > 1)
    {
        diags.Report(diag::fatal_too_many_input_files);
        return EXIT_FAILURE;
    }
    if (argc == 1)
        in_filename = argv[0];

    // Load standard modules
    if (!LoadStandardPlugins())
    {
        diags.Report(diag::fatal_standard_modules);
        return EXIT_FAILURE;
    }

    return do_assemble(source_mgr, diags);
}

