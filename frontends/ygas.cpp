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

#include <memory>

#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/ManagedStatic.h"
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

#include "frontends/license.cpp"
#include "frontends/DiagnosticOptions.h"
#include "frontends/TextDiagnosticPrinter.h"


// Preprocess-only buffer size
#define PREPROC_BUF_SIZE    16384

namespace cl = llvm::cl;

static std::auto_ptr<llvm::raw_ostream> errfile;

// version message
static const char* full_version =
    PACKAGE_NAME " " PACKAGE_INTVER "." PACKAGE_BUILD;
void
PrintVersion()
{
    llvm::outs()
        << full_version << '\n'
        << "Compiled on " __DATE__ ".\n"
        << "Copyright (c) 2001-2010 Peter Johnson and other Yasm developers.\n"
        << "Run ygas --license for licensing overview and summary.\n";
}

// extra help messages
static cl::extrahelp help_tail(
    "\n"
    "Files are asm sources to be assembled.\n"
    "\n"
    "Sample invocation:\n"
    "   ygas -32 -o object.o source.s\n"
    "\n"
    "Report bugs to bug-yasm@tortall.net\n");

static cl::opt<std::string> in_filename(cl::Positional,
    cl::desc("file"));

// -32
static cl::list<bool> bits_32("32",
    cl::desc("set 32-bit output"));

// -64
static cl::list<bool> bits_64("64",
    cl::desc("set 64-bit output"));

// -defsym
static cl::list<std::string> defsym("defsym",
    cl::desc("define symbol"));

// -D (ignored)
static cl::list<std::string> ignored_D("D",
    cl::desc("Ignored"),
    cl::Prefix,
    cl::Hidden);

// -dump-object
static llvm::cl::opt<yasm::Assembler::ObjectDumpTime> dump_object("dump-object",
    llvm::cl::desc("Dump object in YAML after this phase:"),
    llvm::cl::values(
        clEnumValN(yasm::Assembler::DUMP_NEVER, "never", "never dump"),
        clEnumValN(yasm::Assembler::DUMP_AFTER_PARSE, "parsed",
                   "after parse phase"),
        clEnumValN(yasm::Assembler::DUMP_AFTER_FINALIZE, "finalized",
                   "after finalization"),
        clEnumValN(yasm::Assembler::DUMP_AFTER_OPTIMIZE, "optimized",
                   "after optimization"),
        clEnumValN(yasm::Assembler::DUMP_AFTER_OUTPUT, "output",
                   "after output"),
        clEnumValEnd));

// --execstack, --noexecstack
static cl::list<bool> execstack("execstack",
    cl::desc("require executable stack for this object"));
static cl::list<bool> noexecstack("noexecstack",
    cl::desc("don't require executable stack for this object"));

// -J
static cl::list<bool> no_signed_overflow("J",
    cl::desc("don't warn about signed overflow"));

// -I
static cl::list<std::string> include_paths("I",
    cl::desc("Add include path"),
    cl::value_desc("path"),
    cl::Prefix);

// --license
static cl::opt<bool> show_license("license",
    cl::desc("Show license text"));

// --plugin
#ifndef BUILD_STATIC
static cl::list<std::string> plugin_names("plugin",
    cl::desc("Load plugin module"),
    cl::value_desc("plugin"));
#endif

// -o
static cl::opt<std::string> obj_filename("o",
    cl::desc("Name of object-file output"),
    cl::value_desc("filename"),
    cl::Prefix);

// -w
static cl::opt<bool> ignored_w("w",
    cl::desc("Ignored"),
    cl::ZeroOrMore,
    cl::Hidden);

// -x
static cl::opt<bool> ignored_x("x",
    cl::desc("Ignored"),
    cl::ZeroOrMore,
    cl::Hidden);

// -Qy
static cl::opt<bool> ignored_qy("Qy",
    cl::desc("Ignored"),
    cl::ZeroOrMore,
    cl::Hidden);

// -Qn
static cl::opt<bool> ignored_qn("Qn",
    cl::desc("Ignored"),
    cl::ZeroOrMore,
    cl::Hidden);

// -W, --no-warn
static cl::list<bool> inhibit_warnings("W",
    cl::desc("Suppress warning messages"));
static cl::alias inhibit_warnings_long("no-warn",
    cl::desc("Alias for -W"),
    cl::aliasopt(inhibit_warnings));

// --fatal-warnings
static cl::list<bool> fatal_warnings("fatal-warnings",
    cl::desc("Suppress warning messages"));

// --warn
static cl::list<bool> enable_warnings("warn",
    cl::desc("Don't suppress warning messages or treat them as errors"));

// sink to warn instead of error on unrecognized options
static cl::list<std::string> unknown_options(cl::Sink);

static void
ApplyWarningSettings(yasm::Diagnostic& diags)
{
    // Disable init-nobits and uninit-contents by default.
    diags.setDiagnosticGroupMapping("init-nobits", yasm::diag::MAP_IGNORE);
    diags.setDiagnosticGroupMapping("uninit-contents", yasm::diag::MAP_IGNORE);

    // Walk through inhibit_warnings, fatal_warnings, enable_warnings, and
    // no_signed_overflow in parallel, ordering by command line argument
    // position.
    unsigned int inhibit_pos = 0, inhibit_num = 0;
    unsigned int enable_pos = 0,  enable_num = 0;
    unsigned int fatal_pos = 0,   fatal_num = 0;
    unsigned int signed_pos = 0,  signed_num = 0;
    for (;;)
    {
        if (inhibit_num < inhibit_warnings.size())
            inhibit_pos = inhibit_warnings.getPosition(inhibit_num);
        else
            inhibit_pos = 0;
        if (enable_num < enable_warnings.size())
            enable_pos = enable_warnings.getPosition(enable_num);
        else
            enable_pos = 0;
        if (fatal_num < fatal_warnings.size())
            fatal_pos = fatal_warnings.getPosition(fatal_num);
        else
            fatal_pos = 0;
        if (signed_num < no_signed_overflow.size())
            signed_pos = no_signed_overflow.getPosition(signed_num);
        else
            signed_pos = 0;

        if (inhibit_pos != 0 &&
            (enable_pos == 0 || inhibit_pos < enable_pos) &&
            (fatal_pos == 0 || inhibit_pos < fatal_pos) &&
            (signed_pos == 0 || inhibit_pos < signed_pos))
        {
            // Handle inhibit option
            ++inhibit_num;
            diags.setIgnoreAllWarnings(true);
        }
        else if (enable_pos != 0 &&
                 (inhibit_pos == 0 || enable_pos < inhibit_pos) &&
                 (fatal_pos == 0 || enable_pos < fatal_pos) &&
                 (signed_pos == 0 || enable_pos < signed_pos))
        {
            // Handle enable option
            ++enable_num;
            diags.setIgnoreAllWarnings(false);
            diags.setWarningsAsErrors(false);
            diags.setDiagnosticGroupMapping("signed-overflow",
                                            yasm::diag::MAP_WARNING);
        }
        else if (fatal_pos != 0 &&
                 (enable_pos == 0 || fatal_pos < enable_pos) &&
                 (inhibit_pos == 0 || fatal_pos < inhibit_pos) &&
                 (signed_pos == 0 || fatal_pos < signed_pos))
        {
            // Handle fatal option
            ++fatal_num;
            diags.setWarningsAsErrors(true);
        }
        else if (signed_pos != 0 &&
                 (enable_pos == 0 || signed_pos < enable_pos) &&
                 (fatal_pos == 0 || signed_pos < fatal_pos) &&
                 (inhibit_pos == 0 || signed_pos < inhibit_pos))
        {
            // Handle signed option
            ++signed_num;
            diags.setDiagnosticGroupMapping("signed-overflow",
                                            yasm::diag::MAP_IGNORE);
        }
        else
            break; // we're done with the list
    }
}

static std::string
GetBitsSetting()
{
    std::string bits = YGAS_OBJFMT_BITS;

    // Walk through bits_32 and bits_64 in parallel, ordering by command line
    // argument position.
    unsigned int bits32_pos = 0, bits32_num = 0;
    unsigned int bits64_pos = 0, bits64_num = 0;
    for (;;)
    {
        if (bits32_num < bits_32.size())
            bits32_pos = bits_32.getPosition(bits32_num);
        else
            bits32_pos = 0;
        if (bits64_num < bits_64.size())
            bits64_pos = bits_64.getPosition(bits64_num);
        else
            bits64_pos = 0;

        if (bits32_pos != 0 && (bits64_pos == 0 || bits32_pos < bits64_pos))
        {
            // Handle bits32 option
            ++bits32_num;
            bits = "32";
        }
        else if (bits64_pos != 0 &&
                 (bits32_pos == 0 || bits64_pos < bits32_pos))
        {
            // Handle bits64 option
            ++bits64_num;
            bits = "64";
        }
        else
            break; // we're done with the list
    }
    return bits;
}

static void
ConfigureObject(yasm::Object& object)
{
    yasm::Object::Config& config = object.getConfig();

    // Walk through execstack and noexecstack in parallel, ordering by command
    // line argument position.
    unsigned int exec_pos = 0, exec_num = 0;
    unsigned int noexec_pos = 0, noexec_num = 0;
    for (;;)
    {
        if (exec_num < execstack.size())
            exec_pos = execstack.getPosition(exec_num);
        else
            exec_pos = 0;
        if (noexec_num < noexecstack.size())
            noexec_pos = noexecstack.getPosition(noexec_num);
        else
            noexec_pos = 0;

        if (exec_pos != 0 && (noexec_pos == 0 || exec_pos < noexec_pos))
        {
            // Handle exec option
            ++exec_num;
            config.ExecStack = true;
            config.NoExecStack = false;
        }
        else if (noexec_pos != 0 && (exec_pos == 0 || noexec_pos < exec_pos))
        {
            // Handle noexec option
            ++noexec_num;
            config.ExecStack = false;
            config.NoExecStack = true;
        }
        else
            break; // we're done with the list
    }
}

static int
do_assemble(yasm::SourceManager& source_mgr, yasm::Diagnostic& diags)
{
    // Apply warning settings
    ApplyWarningSettings(diags);

    // Determine objfmt_bits based on -32 and -64 options
    std::string objfmt_bits = GetBitsSetting();

    yasm::FileManager file_mgr;
    yasm::Assembler assembler("x86", YGAS_OBJFMT_BASE + objfmt_bits, diags,
                              dump_object);
    yasm::HeaderSearch headers(file_mgr);

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
        const yasm::FileEntry* in = file_mgr.getFile(in_filename);
        if (!in)
        {
            diags.Report(yasm::SourceLocation(), yasm::diag::fatal_file_open)
                << in_filename;
            return EXIT_FAILURE;
        }
        source_mgr.createMainFileID(in, yasm::SourceLocation());
    }

    // Initialize the object.
    if (!assembler.InitObject(source_mgr, diags))
        return EXIT_FAILURE;

    // Configure object per command line parameters.
    ConfigureObject(*assembler.getObject());

    // Predefine symbols.
    for (std::vector<std::string>::const_iterator i=defsym.begin(),
         end=defsym.end(); i != end; ++i)
    {
        llvm::StringRef str(*i);
        size_t equalpos = str.find('=');
        if (equalpos == llvm::StringRef::npos)
        {
            diags.Report(yasm::diag::fatal_bad_defsym) << str;
            continue;
        }
        llvm::StringRef name = str.slice(0, equalpos);
        llvm::StringRef vstr = str.slice(equalpos+1, llvm::StringRef::npos);

        yasm::IntNum value;
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
                diags.Report(yasm::diag::fatal_bad_defsym) << name;
                continue;
            }
            value.setStr(vstr, radix);
        }

        // define equ
        assembler.getObject()->getSymbol(name)->DefineEqu(yasm::Expr(value));
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
        diags.Report(yasm::SourceLocation(), yasm::diag::err_cannot_open_file)
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
    llvm::llvm_shutdown_obj llvm_manager(false);

    cl::SetVersionPrinter(&PrintVersion);
    cl::ParseCommandLineOptions(argc, argv, "", true);

    // Handle special exiting options
    if (show_license)
    {
        for (std::size_t i=0; i<sizeof(license_msg)/sizeof(license_msg[0]); i++)
            llvm::outs() << license_msg[i] << '\n';
        return EXIT_SUCCESS;
    }

    yasm::DiagnosticOptions diag_opts;
    diag_opts.ShowOptionNames = 1;
    diag_opts.ShowSourceRanges = 1;
    yasm::TextDiagnosticPrinter diag_printer(llvm::errs(), diag_opts);
    yasm::Diagnostic diags(&diag_printer);
    yasm::SourceManager source_mgr(diags);
    diags.setSourceManager(&source_mgr);
    diag_printer.setPrefix("ygas");

    for (std::vector<std::string>::const_iterator i=unknown_options.begin(),
         end=unknown_options.end(); i != end; ++i)
    {
        diags.Report(yasm::diag::warn_unknown_command_line_option) << *i;
    }

    // Load standard modules
    if (!yasm::LoadStandardPlugins())
    {
        diags.Report(yasm::diag::fatal_standard_modules);
        return EXIT_FAILURE;
    }

#ifndef BUILD_STATIC
    // Load plugins
    for (std::vector<std::string>::const_iterator i=plugin_names.begin(),
         end=plugin_names.end(); i != end; ++i)
    {
        if (!yasm::LoadPlugin(*i))
            diags.Report(yasm::diag::warn_plugin_load) << *i;
    }
#endif

    // Default to stdin if no filename specified.
    if (in_filename.empty())
        in_filename = "-";

    return do_assemble(source_mgr, diags);
}

