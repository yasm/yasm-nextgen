//
// Program entry point, command line parsing
//
//  Copyright (C) 2001-2008  Peter Johnson
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
#include "yasmx/Parse/DirectoryLookup.h"
#include "yasmx/Parse/HeaderSearch.h"
#include "yasmx/Parse/Parser.h"
#include "yasmx/Support/registry.h"
#include "yasmx/System/plugin.h"
#include "yasmx/Arch.h"
#include "yasmx/Assembler.h"
#include "yasmx/DebugFormat.h"
#include "yasmx/ListFormat.h"
#include "yasmx/Module.h"
#include "yasmx/Object.h"
#include "yasmx/ObjectFormat.h"

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
        << "Copyright (c) 2001-2009 Peter Johnson and other Yasm developers.\n"
        << "Run yasm --license for licensing overview and summary.\n";
}

// extra help messages
static cl::extrahelp help_tail(
    "\n"
    "Files are asm sources to be assembled.\n"
    "\n"
    "Sample invocation:\n"
    "   yasm -f elf -o object.o source.asm\n"
    "\n"
    "Report bugs to bug-yasm@tortall.net\n");

static cl::opt<std::string> in_filename(cl::Positional,
    cl::desc("file"));

// -a, --arch
static cl::opt<std::string> arch_keyword("a",
    cl::desc("Select architecture (list with -a help)"),
    cl::value_desc("arch"),
    cl::Prefix,
    cl::ZeroOrMore);
static cl::alias arch_keyword_long("arch",
    cl::desc("Alias for -a"),
    cl::value_desc("arch"),
    cl::aliasopt(arch_keyword));

// -D, -d
static cl::list<std::string> predefine_macros("D",
    cl::desc("Pre-define a macro, optionally to value"),
    cl::value_desc("macro[=value]"),
    cl::Prefix);
static cl::alias predefine_macros_alias("d",
    cl::desc("Alias for -D"),
    cl::value_desc("macro[=value]"),
    cl::aliasopt(predefine_macros));

#ifdef WITH_XML
// -dump-object
static llvm::cl::opt<yasm::Assembler::ObjectDumpTime> dump_object("dump-object",
    llvm::cl::desc("Dump object in XML after this phase:"),
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
#else
yasm::Assembler::ObjectDumpTime dump_object = yasm::Assembler::DUMP_NEVER;
#endif // WITH_XML

// -E
static cl::opt<std::string> error_filename("E",
    cl::desc("redirect error messages to file"),
    cl::value_desc("file"),
    cl::Prefix,
    cl::ZeroOrMore);

// -e
static cl::opt<bool> preproc_only("e",
    cl::desc("preprocess only (writes output to stdout by default)"),
    cl::ZeroOrMore);
static cl::alias preproc_only_long("preproc-only",
    cl::desc("Alias for -e"),
    cl::aliasopt(preproc_only));

// --execstack, --noexecstack
static cl::list<bool> execstack("execstack",
    cl::desc("require executable stack for this object"));
static cl::list<bool> noexecstack("noexecstack",
    cl::desc("don't require executable stack for this object"));

// -f, --oformat
static cl::opt<std::string> objfmt_keyword("f",
    cl::desc("Select object format (list with -f help)"),
    cl::value_desc("format"),
    cl::Prefix,
    cl::ZeroOrMore);
static cl::alias objfmt_keyword_long("oformat",
    cl::desc("Alias for -f"),
    cl::value_desc("format"),
    cl::aliasopt(objfmt_keyword));

// -g, --dformat
static cl::opt<std::string> dbgfmt_keyword("g",
    cl::desc("Select debugging format (list with -g help)"),
    cl::value_desc("debug"),
    cl::Prefix,
    cl::ZeroOrMore);
static cl::alias dbgfmt_keyword_long("dformat",
    cl::desc("Alias for -g"),
    cl::value_desc("debug"),
    cl::aliasopt(dbgfmt_keyword));

// --force-strict
static cl::opt<bool> force_strict("force-strict",
    cl::desc("treat all sized operands as if `strict' was used"));

// -h
static cl::opt<bool> show_help("h",
    cl::desc("Alias for --help"),
    cl::Hidden);

// -i, -I
static cl::list<std::string> include_paths("I",
    cl::desc("Add include path"),
    cl::value_desc("path"),
    cl::Prefix);
static cl::alias include_paths_alias("i",
    cl::desc("Alias for -I"),
    cl::aliasopt(include_paths),
    cl::Prefix);

// -L, --lformat
static cl::opt<std::string> listfmt_keyword("L",
    cl::desc("Select list format (list with -L help)"),
    cl::value_desc("list"),
    cl::Prefix,
    cl::ZeroOrMore);
static cl::alias listfmt_keyword_long("lformat",
    cl::desc("Alias for -L"),
    cl::value_desc("list"),
    cl::aliasopt(listfmt_keyword));

// --license
static cl::opt<bool> show_license("license",
    cl::desc("Show license text"));

// -l, --list
static cl::opt<std::string> list_filename("l",
    cl::desc("Name of list-file output"),
    cl::value_desc("listfile"),
    cl::Prefix);
static cl::alias list_filename_long("list",
    cl::desc("Alias for -l"),
    cl::value_desc("listfile"),
    cl::aliasopt(list_filename));

// -M
static cl::opt<bool> generate_make_dependencies("M",
    cl::desc("generate Makefile dependencies on stdout"));

// -m, --machine
static cl::opt<std::string> machine_name("m",
    cl::desc("Select machine (list with -m help)"),
    cl::value_desc("machine"),
    cl::Prefix,
    cl::ZeroOrMore);
static cl::alias machine_name_long("machine",
    cl::desc("Alias for -m"),
    cl::value_desc("machine"),
    cl::aliasopt(machine_name));

// -O, -Onnn
static cl::opt<int> optimize_level("O",
    cl::desc("Set optimization level (ignored)"),
    cl::value_desc("level"),
    cl::ValueOptional,
    cl::ZeroOrMore,
    cl::Prefix,
    cl::Hidden);

// -N, --plugin
#ifndef BUILD_STATIC
static cl::list<std::string> plugin_names("N",
    cl::desc("Load plugin module"),
    cl::value_desc("plugin"),
    cl::Prefix);
static cl::alias plugin_names_long("plugin",
    cl::desc("Alias for -N"),
    cl::value_desc("plugin"),
    cl::aliasopt(plugin_names));
#endif

// -o, --objfile
static cl::opt<std::string> obj_filename("o",
    cl::desc("Name of object-file output"),
    cl::value_desc("filename"),
    cl::Prefix);
static cl::alias obj_filename_long("objfile",
    cl::desc("Alias for -o"),
    cl::value_desc("filename"),
    cl::aliasopt(obj_filename));

// -P
static cl::list<std::string> preinclude_files("P",
    cl::desc("Pre-include file"),
    cl::value_desc("filename"),
    cl::Prefix);

// -p, --parser
static cl::opt<std::string> parser_keyword("p",
    cl::desc("Select parser (list with -p help)"),
    cl::value_desc("parser"),
    cl::Prefix,
    cl::ZeroOrMore);
static cl::alias parser_keyword_long("parser",
    cl::desc("Alias for -p"),
    cl::value_desc("parser"),
    cl::aliasopt(parser_keyword));

// -s
static cl::opt<bool> error_stdout("s",
    cl::desc("redirect error messages to stdout"),
    cl::ZeroOrMore);

// -U, -u
static cl::list<std::string> undefine_macros("U",
    cl::desc("Undefine a macro"),
    cl::value_desc("macro"),
    cl::Prefix);
static cl::alias undefine_macros_alias("u",
    cl::desc("Alias for -U"),
    cl::value_desc("macro"),
    cl::aliasopt(undefine_macros));

// -W
static cl::list<std::string> warning_settings("W",
    cl::desc("Enables/disables warning"),
    cl::value_desc("warn-group"),
    cl::Prefix);

// -w
static cl::list<bool> inhibit_warnings("w",
    cl::desc("Inhibits warning messages"));

// -X
enum ErrwarnStyle
{
    EWSTYLE_GNU = 0,
    EWSTYLE_VC
};
static cl::opt<ErrwarnStyle> ewmsg_style("X",
    cl::desc("Set error/warning message style:"),
    cl::Prefix,
    cl::ZeroOrMore,
    cl::init(EWSTYLE_GNU),
    cl::values(
     clEnumValN(EWSTYLE_GNU, "gnu", "GNU (GCC) error/warning style (default)"),
     clEnumValN(EWSTYLE_GNU, "gcc", "Alias for gnu"),
     clEnumValN(EWSTYLE_VC,  "vc",  "Visual Studio error/warning style"),
     clEnumValEnd));

// sink to warn instead of error on unrecognized options
static cl::list<std::string> unknown_options(cl::Sink);

static void
PrintListKeywordDesc(const std::string& name, const std::string& keyword)
{
    llvm::outs() << "    " << llvm::format("%-12s", keyword.c_str())
                 << name << '\n';
}

template <typename T>
static void
ListModule()
{
    yasm::ModuleNames list = yasm::getModules<T>();
    for (yasm::ModuleNames::iterator i=list.begin(), end=list.end();
         i != end; ++i)
    {
        std::auto_ptr<T> obj = yasm::LoadModule<T>(*i);
        PrintListKeywordDesc(obj->getName(), *i);
    }
}

template <typename T>
static std::string
ModuleCommonHandler(const std::string& param,
                    const char* name,
                    const char* name_plural,
                    bool* listed,
                    yasm::Diagnostic& diags)
{
    if (param.empty())
        return param;

    std::string keyword = llvm::LowercaseString(param);
    if (!yasm::isModule<T>(keyword))
    {
        if (param == "help")
        {
            llvm::outs() << "Available yasm " << name_plural << ":\n";
            ListModule<T>();
            *listed = true;
            return keyword;
        }
        diags.Report(yasm::diag::fatal_unrecognized_module) << name << param;
        exit(EXIT_FAILURE);
    }
    return keyword;
}

static void
ApplyWarningSettings(yasm::Diagnostic& diags)
{
    // Walk through warning_settings and inhibit_warnings in parallel,
    // ordering by command line argument position.
    unsigned int setting_num = 0, inhibit_num = 0;
    unsigned int setting_pos = 0, inhibit_pos = 0;
    for (;;)
    {
        if (setting_num < warning_settings.size())
            setting_pos = warning_settings.getPosition(setting_num);
        else
            setting_pos = 0;
        if (inhibit_num < inhibit_warnings.size())
            inhibit_pos = inhibit_warnings.getPosition(inhibit_num);
        else
            inhibit_pos = 0;

        if (inhibit_pos != 0 &&
            (setting_pos == 0 || inhibit_pos < setting_pos))
        {
            // Handle inhibit option
            diags.setIgnoreAllWarnings(true);
            ++inhibit_num;
        }
        else if (setting_pos != 0 &&
                 (inhibit_pos == 0 || setting_pos < inhibit_pos))
        {
            llvm::StringRef setting(warning_settings[setting_num]);

            yasm::diag::Mapping mapping = yasm::diag::MAP_WARNING;
            bool positive = true;
            if (setting.startswith("no-"))
            {
                positive = false;
                mapping = yasm::diag::MAP_IGNORE;
                setting = setting.substr(3);
            }

            if (setting.startswith("error"))
            {
                llvm::StringRef unused;
                llvm::tie(unused, setting) = setting.split('=');
                // Just plain -Werror maps all warnings to errors.
                // -Werror=foo/-Wno-error=foo maps warning foo.
                if (setting.empty())
                {
                    diags.setWarningsAsErrors(positive);
                    ++setting_num;
                    continue;
                }
                else
                {
                    mapping = positive ? yasm::diag::MAP_ERROR
                                       : yasm::diag::MAP_WARNING_NO_WERROR;
                }
            }
#if 0
            else if (setting.startswith("fatal-errors"))
            {
                llvm::StringRef unused;
                llvm::tie(unused, setting) = setting.split('=');
                // Just plain -Wfatal-error maps all errors to fatal errors.
                // -Wfatal-errors=foo/-Wno-fatal-errors=foo maps just foo.
                if (setting.empty())
                {
                    diags.setErrorsAsFatal(positive);
                    ++setting_num;
                    continue;
                }
                else
                {
                    mapping = positive ? yasm::diag::MAP_FATAL
                                       : yasm::diag::MAP_ERROR_NO_WFATAL;
                }
            }
#endif

            if (diags.setDiagnosticGroupMapping(setting, mapping))
                diags.Report(yasm::SourceLocation(),
                             yasm::diag::warn_unknown_warning_option)
                    << ("-W" + warning_settings[setting_num]);
            ++setting_num;
        }
        else
            break; // we're done with the list
    }
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

#if 0
static void
ApplyPreprocessorBuiltins(yasm::Preprocessor* preproc)
{
    // Define standard YASM assembly-time macro constants
    std::string predef("__YASM_OBJFMT__=");
    predef += objfmt_keyword;
    preproc->DefineBuiltin(predef);
}

static void
ApplyPreprocessorSavedOptions(yasm::Preprocessor* preproc)
{
    // Walk through predefine_macros, undefine_macros, and preinclude_files
    // in parallel, ordering by command line argument position.
    unsigned int def_num = 0, undef_num = 0, inc_num = 0;
    unsigned int def_pos = 0, undef_pos = 0, inc_pos = 0;
    for (;;)
    {
        if (def_num < predefine_macros.size())
            def_pos = predefine_macros.getPosition(def_num);
        else
            def_pos = 0;
        if (undef_num < undefine_macros.size())
            undef_pos = undefine_macros.getPosition(undef_num);
        else
            undef_pos = 0;
        if (inc_num < preinclude_files.size())
            inc_pos = preinclude_files.getPosition(inc_num);
        else
            inc_pos = 0;

        if (def_pos != 0 &&
            (undef_pos == 0 || def_pos < undef_pos) &&
            (inc_pos == 0 || def_pos < inc_pos))
        {
            // Handle predefine option
            preproc->PredefineMacro(predefine_macros[def_num]);
            ++def_num;
        }
        else if (undef_pos != 0 &&
                 (def_pos == 0 || undef_pos < def_pos) &&
                 (inc_pos == 0 || undef_pos < inc_pos))
        {
            // Handle undefine option
            preproc->UndefineMacro(undefine_macros[undef_num]);
            ++undef_num;
        }
        else if (inc_pos != 0 &&
                 (def_pos == 0 || inc_pos < def_pos) &&
                 (undef_pos == 0 || inc_pos < undef_pos))
        {
            // Handle preinclude option
            preproc->AddIncludeFile(preinclude_files[inc_num]);
            ++inc_num;
        }
        else
            break; // we're done with the list
    }
}
#endif

#if 0
static int
do_preproc_only(void)
{
    yasm_linemap *linemap;
    char *preproc_buf = yasm_xmalloc(PREPROC_BUF_SIZE);
    size_t got;
    const char *base_filename;
    FILE *out = NULL;
    yasm_errwarns *errwarns = yasm_errwarns_create();

    /* Initialize line map */
    linemap = yasm_linemap_create();
    yasm_linemap_set(linemap, in_filename, 1, 1);

    /* Default output to stdout if not specified or generating dependency
       makefiles */
    if (!obj_filename || generate_make_dependencies)
    {
        out = stdout;

        /* determine the object filename if not specified, but we need a
            file name for the makefile rule */
        if (generate_make_dependencies && !obj_filename)
        {
            if (in_filename == NULL)
                /* Default to yasm.out if no obj filename specified */
                obj_filename = yasm__xstrdup("yasm.out");
            else
            {
                /* replace (or add) extension to base filename */
                yasm__splitpath(in_filename, &base_filename);
                if (base_filename[0] == '\0')
                    obj_filename = yasm__xstrdup("yasm.out");
                else
                    obj_filename = replace_extension(base_filename,
                        cur_objfmt_module->extension, "yasm.out");
            }
        }
    }
    else
    {
        /* Open output (object) file */
        out = open_file(obj_filename, "wt");
        if (!out)
        {
            PrintError(String::Compose(_("could not open file `%1'"), filename));
            yasm_xfree(preproc_buf);
            return EXIT_FAILURE;
        }
    }

    /* Pre-process until done */
    cur_preproc = yasm_preproc_create(cur_preproc_module, in_filename,
                                      linemap, errwarns);

    apply_preproc_builtins();
    apply_preproc_saved_options();

    if (generate_make_dependencies)
    {
        size_t totlen;

        fprintf(stdout, "%s: %s", obj_filename, in_filename);
        totlen = strlen(obj_filename)+2+strlen(in_filename);

        while ((got = yasm_preproc_get_included_file(cur_preproc, preproc_buf,
                                                     PREPROC_BUF_SIZE)) != 0)
        {
            totlen += got;
            if (totlen > 72)
            {
                fputs(" \\\n  ", stdout);
                totlen = 2;
            }
            fputc(' ', stdout);
            fwrite(preproc_buf, got, 1, stdout);
        }
        fputc('\n', stdout);
    }
    else
    {
        while ((got = yasm_preproc_input(cur_preproc, preproc_buf,
                                         PREPROC_BUF_SIZE)) != 0)
            fwrite(preproc_buf, got, 1, out);
    }

    if (out != stdout)
        fclose(out);

    if (yasm_errwarns_num_errors(errwarns, warning_error) > 0)
    {
        yasm_errwarns_output_all(errwarns, linemap, warning_error,
                                 print_yasm_error, print_yasm_warning);
        if (out != stdout)
            remove(obj_filename);
        yasm_xfree(preproc_buf);
        yasm_linemap_destroy(linemap);
        yasm_errwarns_destroy(errwarns);
        return EXIT_FAILURE;
    }

    yasm_errwarns_output_all(errwarns, linemap, warning_error,
                             print_yasm_error, print_yasm_warning);
    yasm_xfree(preproc_buf);
    yasm_linemap_destroy(linemap);
    yasm_errwarns_destroy(errwarns);
    return EXIT_SUCCESS;
}
#endif
static int
do_assemble(yasm::SourceManager& source_mgr, yasm::Diagnostic& diags)
{
    // Apply warning settings
    ApplyWarningSettings(diags);

    yasm::FileManager file_mgr;
    yasm::Assembler assembler(arch_keyword, objfmt_keyword, diags, dump_object);
    yasm::HeaderSearch headers(file_mgr);

    if (diags.hasFatalErrorOccurred())
        return EXIT_FAILURE;

    // Set object filename if specified.
    if (!obj_filename.empty())
        assembler.setObjectFilename(obj_filename);

    // Set parser.
    assembler.setParser(parser_keyword, diags);

    // Set machine if specified.
    if (!machine_name.empty())
        assembler.setMachine(machine_name, diags);

    if (diags.hasFatalErrorOccurred())
        return EXIT_FAILURE;

    // Set debug format if specified.
    if (!dbgfmt_keyword.empty())
        assembler.setDebugFormat(dbgfmt_keyword, diags);

    if (diags.hasFatalErrorOccurred())
        return EXIT_FAILURE;

    // Set up header search paths
    std::vector<yasm::DirectoryLookup> dirs;
    for (std::vector<std::string>::iterator i = include_paths.begin(),
         end = include_paths.end(); i != end; ++i)
    {
        dirs.push_back(yasm::DirectoryLookup(file_mgr.getDirectory(*i), true));
    }
    headers.SetSearchPaths(dirs, 0, false);

#if 0
    ApplyPreprocessorBuiltins(assembler.getPreprocessor());
    ApplyPreprocessorSavedOptions(assembler.getPreprocessor());
#endif

    assembler.getArch()->setVar("force_strict", force_strict);

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

    // initialize the object.
    if (!assembler.InitObject(source_mgr, diags))
        return EXIT_FAILURE;

    // Configure object per command line parameters.
    ConfigureObject(*assembler.getObject());

    // initialize the parser.
    assembler.InitParser(source_mgr, diags, headers);

    // assemble the input.
    if (!assembler.Assemble(source_mgr, diags))
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
#if 0
    // Open and write the list file
    if (list_filename)
    {
        FILE *list = open_file(list_filename, "wt");
        if (!list)
        {
            PrintError(String::Compose(_("could not open file `%1'"), filename));
            return EXIT_FAILURE;
        }
        /* Initialize the list format */
        cur_listfmt = yasm_listfmt_create(cur_listfmt_module, in_filename,
                                          obj_filename);
        yasm_listfmt_output(cur_listfmt, list, linemap, cur_arch);
        fclose(list);
    }
#endif
    return EXIT_SUCCESS;
}

// main function
int
main(int argc, char* argv[])
{
    llvm::llvm_shutdown_obj llvm_manager(false);

    cl::SetVersionPrinter(&PrintVersion);
    cl::ParseCommandLineOptions(argc, argv);

    // Handle special exiting options
    if (show_help)
        cl::PrintHelpMessage();

    if (show_license)
    {
        for (std::size_t i=0; i<sizeof(license_msg)/sizeof(license_msg[0]); i++)
            llvm::outs() << license_msg[i] << '\n';
        return EXIT_SUCCESS;
    }

    // Open error file if specified.
    // -s overrides -e if it comes after it on the command line.
    // Default to stderr if not overridden by -s or -e.
    if (error_stdout &&
        error_stdout.getPosition() > error_filename.getPosition())
    {
        error_filename.clear();
        errfile.reset(new llvm::raw_stdout_ostream);
    }
    else if (!error_filename.empty())
    {
        std::string err;
        errfile.reset(new llvm::raw_fd_ostream(error_filename.c_str(), err));
        if (!err.empty())
        {
            llvm::errs() << "yasm: could not open file '" << error_filename
                         << "': " << err << '\n';
            return EXIT_FAILURE;
        }
    }
    else
        errfile.reset(new llvm::raw_stderr_ostream);

    yasm::DiagnosticOptions diag_opts;
    diag_opts.Microsoft = (ewmsg_style == EWSTYLE_VC);
    diag_opts.ShowOptionNames = 1;
    diag_opts.ShowSourceRanges = 1;
    yasm::TextDiagnosticPrinter diag_printer(*errfile, diag_opts);
    yasm::Diagnostic diags(&diag_printer);
    yasm::SourceManager source_mgr(diags);
    diags.setSourceManager(&source_mgr);
    diag_printer.setPrefix("yasm");

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

    // Handle keywords (including "help").
    bool listed = false;
    arch_keyword = ModuleCommonHandler<yasm::ArchModule>
        (arch_keyword, "architecture", "architectures", &listed, diags);
    parser_keyword = ModuleCommonHandler<yasm::ParserModule>
        (parser_keyword, "parser", "parsers", &listed, diags);
    objfmt_keyword = ModuleCommonHandler<yasm::ObjectFormatModule>
        (objfmt_keyword, "object format", "object formats", &listed, diags);
    dbgfmt_keyword = ModuleCommonHandler<yasm::DebugFormatModule>
        (dbgfmt_keyword, "debug format", "debug formats", &listed, diags);
    listfmt_keyword = ModuleCommonHandler<yasm::ListFormatModule>
        (listfmt_keyword, "list format", "list formats", &listed, diags);
    if (listed)
        return EXIT_SUCCESS;

    // If generating make dependencies, also set preproc_only to true,
    // as we don't want to generate code.
    if (generate_make_dependencies)
        preproc_only = true;

    // Default to x86 as the architecture
    if (arch_keyword.empty())
        arch_keyword = "x86";

    // Check for machine help
    if (machine_name == "help")
    {
        std::auto_ptr<yasm::ArchModule> arch_auto =
            yasm::LoadModule<yasm::ArchModule>(arch_keyword);
        llvm::outs() << "Available machines for architecture '"
                     << arch_keyword << "':\n";
        yasm::ArchModule::MachineNames machines = arch_auto->getMachines();

        for (yasm::ArchModule::MachineNames::const_iterator
             i=machines.begin(), end=machines.end(); i != end; ++i)
            PrintListKeywordDesc(i->second, i->first);
        return EXIT_SUCCESS;
    }

    // Require an input filename.  We don't use llvm::cl facilities for this
    // as we want to allow e.g. "yasm --license".
    if (in_filename.empty())
    {
        diags.Report(yasm::diag::fatal_no_input_files);
        return EXIT_FAILURE;
    }

    // If not already specified, default to bin as the object format.
    if (objfmt_keyword.empty())
        objfmt_keyword = "bin";

    // Default to NASM as the parser
    if (parser_keyword.empty())
        parser_keyword = "nasm";

#if 0
    // handle preproc-only case here
    if (preproc_only)
        return do_preproc_only();
#endif
    // If list file enabled, make sure we have a list format loaded.
    if (!list_filename.empty())
    {
        // If not already specified, default to nasm as the list format.
        if (listfmt_keyword.empty())
            listfmt_keyword = "nasm";
    }

    return do_assemble(source_mgr, diags);
}

