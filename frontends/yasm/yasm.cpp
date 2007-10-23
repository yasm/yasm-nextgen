//
// Program entry point, command line parsing
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
#include <util.h>

#include <iomanip>
#include <iostream>
#include <fstream>

#include <libyasm/arch.h>
#include <libyasm/debug_format.h>
#include <libyasm/compose.h>
#include <libyasm/errwarn.h>
#include <libyasm/factory.h>
#include <libyasm/file.h>
#include <libyasm/linemap.h>
#include <libyasm/list_format.h>
#include <libyasm/object.h>
#include <libyasm/object_format.h>
#include <libyasm/parser.h>
#include <libyasm/preproc.h>
#include <libyasm/nocase.h>

#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif

#include "yasm-options.h"

//#include "license.c"

#include "static_modules.h"


// Preprocess-only buffer size
#define PREPROC_BUF_SIZE    16384

static std::string obj_filename, in_filename;
static std::string list_filename;
static std::string machine_name;
static enum SpecialOption {
    SPECIAL_NONE = 0,
    SPECIAL_SHOW_HELP,
    SPECIAL_SHOW_VERSION,
    SPECIAL_SHOW_LICENSE,
    SPECIAL_LISTED
} special_option = SPECIAL_NONE;
static std::string arch_keyword;
static std::string parser_keyword;
static std::string preproc_keyword;
static std::string objfmt_keyword;
static std::string dbgfmt_keyword;
static std::string listfmt_keyword;
static bool preproc_only = false;
static bool force_strict = false;
static bool generate_make_dependencies = false;
static bool warning_error = false;  // warnings being treated as errors
static std::ofstream errfile;
static std::string error_filename;
static enum {
    EWSTYLE_GNU = 0,
    EWSTYLE_VC
} ewmsg_style = EWSTYLE_GNU;

static bool check_errors(yasm::Errwarns& errwarns,
                         const yasm::Linemap& linemap);

// Forward declarations: cmd line parser handlers
static int opt_special_handler(const std::string& cmd,
                               const std::string& param, int extra);
static int opt_arch_handler(const std::string& cmd,
                            const std::string& param, int extra);
static int opt_parser_handler(const std::string& cmd,
                              const std::string& param, int extra);
static int opt_preproc_handler(const std::string& cmd,
                               const std::string& param, int extra);
static int opt_objfmt_handler(const std::string& cmd,
                              const std::string& param, int extra);
static int opt_dbgfmt_handler(const std::string& cmd,
                              const std::string& param, int extra);
static int opt_listfmt_handler(const std::string& cmd,
                               const std::string& param, int extra);
static int opt_listfile_handler(const std::string& cmd,
                                const std::string& param, int extra);
static int opt_objfile_handler(const std::string& cmd,
                               const std::string& param, int extra);
static int opt_machine_handler(const std::string& cmd,
                               const std::string& param, int extra);
static int opt_strict_handler(const std::string& cmd,
                              const std::string& param, int extra);
static int opt_warning_handler(const std::string& cmd,
                               const std::string& param, int extra);
static int opt_error_file(const std::string& cmd,
                          const std::string& param, int extra);
static int opt_error_stdout(const std::string& cmd,
                            const std::string& param, int extra);
static int preproc_only_handler(const std::string& cmd,
                                const std::string& param, int extra);
static int opt_include_option(const std::string& cmd,
                              const std::string& param, int extra);
static int opt_preproc_option(const std::string& cmd,
                              const std::string& param, int extra);
static int opt_ewmsg_handler(const std::string& cmd,
                             const std::string& param, int extra);
static int opt_makedep_handler(const std::string& cmd,
                               const std::string& param, int extra);

static std::string replace_extension(const std::string& orig,
                                     const std::string& ext,
                                     const std::string& def);

static void print_error(const std::string& msg);

static const char *handle_yasm_gettext(const char *msgid);
static void print_yasm_error(const std::string& filename,
                             unsigned long line,
                             const std::string& msg,
                             const std::string& xref_fn,
                             unsigned long xref_line,
                             const std::string& xref_msg);
static void print_yasm_warning(const std::string& filename,
                               unsigned long line,
                               const std::string& msg);

static void apply_preproc_builtins(yasm::Preprocessor& preproc);
static void apply_preproc_saved_options(yasm::Preprocessor& preproc);

static void print_list_keyword_desc(const std::string& name,
                                    const std::string& keyword);
template <typename T> static void list_module();

// command line options
static OptOption options[] =
{
    { 0, "version", 0, opt_special_handler, SPECIAL_SHOW_VERSION,
      N_("show version text"), NULL },
    { 0, "license", 0, opt_special_handler, SPECIAL_SHOW_LICENSE,
      N_("show license text"), NULL },
    { 'h', "help", 0, opt_special_handler, SPECIAL_SHOW_HELP,
      N_("show help text"), NULL },
    { 'a', "arch", 1, opt_arch_handler, 0,
      N_("select architecture (list with -a help)"), N_("arch") },
    { 'p', "parser", 1, opt_parser_handler, 0,
      N_("select parser (list with -p help)"), N_("parser") },
    { 'r', "preproc", 1, opt_preproc_handler, 0,
      N_("select preprocessor (list with -r help)"), N_("preproc") },
    { 'f', "oformat", 1, opt_objfmt_handler, 0,
      N_("select object format (list with -f help)"), N_("format") },
    { 'g', "dformat", 1, opt_dbgfmt_handler, 0,
      N_("select debugging format (list with -g help)"), N_("debug") },
    { 'L', "lformat", 1, opt_listfmt_handler, 0,
      N_("select list format (list with -L help)"), N_("list") },
    { 'l', "list", 1, opt_listfile_handler, 0,
      N_("name of list-file output"), N_("listfile") },
    { 'o', "objfile", 1, opt_objfile_handler, 0,
      N_("name of object-file output"), N_("filename") },
    { 'm', "machine", 1, opt_machine_handler, 0,
      N_("select machine (list with -m help)"), N_("machine") },
    { 0, "force-strict", 0, opt_strict_handler, 0,
      N_("treat all sized operands as if `strict' was used"), NULL },
    { 'w', NULL, 0, opt_warning_handler, 1,
      N_("inhibits warning messages"), NULL },
    { 'W', NULL, 0, opt_warning_handler, 0,
      N_("enables/disables warning"), NULL },
    { 'M', NULL, 0, opt_makedep_handler, 0,
      N_("generate Makefile dependencies on stdout"), NULL },
    { 'E', NULL, 1, opt_error_file, 0,
      N_("redirect error messages to file"), N_("file") },
    { 's', NULL, 0, opt_error_stdout, 0,
      N_("redirect error messages to stdout"), NULL },
    { 'e', "preproc-only", 0, preproc_only_handler, 0,
      N_("preprocess only (writes output to stdout by default)"), NULL },
    { 'i', NULL, 1, opt_include_option, 0,
      N_("add include path"), N_("path") },
    { 'I', NULL, 1, opt_include_option, 0,
      N_("add include path"), N_("path") },
    { 'P', NULL, 1, opt_preproc_option, 0,
      N_("pre-include file"), N_("filename") },
    { 'd', NULL, 1, opt_preproc_option, 1,
      N_("pre-define a macro, optionally to value"), N_("macro[=value]") },
    { 'D', NULL, 1, opt_preproc_option, 1,
      N_("pre-define a macro, optionally to value"), N_("macro[=value]") },
    { 'u', NULL, 1, opt_preproc_option, 2,
      N_("undefine a macro"), N_("macro") },
    { 'U', NULL, 1, opt_preproc_option, 2,
      N_("undefine a macro"), N_("macro") },
    { 'X', NULL, 1, opt_ewmsg_handler, 0,
      N_("select error/warning message style (`gnu' or `vc')"), N_("style") },
};

// version message
/*@observer@*/ static const char* version_msg[] = {
#if 0
    PACKAGE_NAME " " PACKAGE_INTVER "." PACKAGE_BUILD,
#endif
    "Compiled on " __DATE__ ".",
    "Copyright (c) 2001-2007 Peter Johnson and other Yasm developers.",
    "Run yasm --license for licensing overview and summary."
};

// help messages
/*@observer@*/ static const char* help_head = N_(
    "usage: yasm [option]* file\n"
    "Options:\n");
/*@observer@*/ static const char* help_tail = N_(
    "\n"
    "Files are asm sources to be assembled.\n"
    "\n"
    "Sample invocation:\n"
    "   yasm -f elf -o object.o source.asm\n"
    "\n"
    "Report bugs to bug-yasm@tortall.net\n");

// parsed command line storage until appropriate modules have been loaded
typedef std::vector<std::pair<std::string, int> > CommandOptions;
static CommandOptions preproc_options;
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
    if (!obj_filename || generate_make_dependencies) {
        out = stdout;

        /* determine the object filename if not specified, but we need a
            file name for the makefile rule */
        if (generate_make_dependencies && !obj_filename) {
            if (in_filename == NULL)
                /* Default to yasm.out if no obj filename specified */
                obj_filename = yasm__xstrdup("yasm.out");
            else {
                /* replace (or add) extension to base filename */
                yasm__splitpath(in_filename, &base_filename);
                if (base_filename[0] == '\0')
                    obj_filename = yasm__xstrdup("yasm.out");
                else
                    obj_filename = replace_extension(base_filename,
                        cur_objfmt_module->extension, "yasm.out");
            }
        }
    } else {
        /* Open output (object) file */
        out = open_file(obj_filename, "wt");
        if (!out) {
            print_error(String::compose(_("could not open file `%1'"), filename));
            yasm_xfree(preproc_buf);
            return EXIT_FAILURE;
        }
    }

    /* Pre-process until done */
    cur_preproc = yasm_preproc_create(cur_preproc_module, in_filename,
                                      linemap, errwarns);

    apply_preproc_builtins();
    apply_preproc_saved_options();

    if (generate_make_dependencies) {
        size_t totlen;

        fprintf(stdout, "%s: %s", obj_filename, in_filename);
        totlen = strlen(obj_filename)+2+strlen(in_filename);

        while ((got = yasm_preproc_get_included_file(cur_preproc, preproc_buf,
                                                     PREPROC_BUF_SIZE)) != 0) {
            totlen += got;
            if (totlen > 72) {
                fputs(" \\\n  ", stdout);
                totlen = 2;
            }
            fputc(' ', stdout);
            fwrite(preproc_buf, got, 1, stdout);
        }
        fputc('\n', stdout);
    } else {
        while ((got = yasm_preproc_input(cur_preproc, preproc_buf,
                                         PREPROC_BUF_SIZE)) != 0)
            fwrite(preproc_buf, got, 1, out);
    }

    if (out != stdout)
        fclose(out);

    if (yasm_errwarns_num_errors(errwarns, warning_error) > 0) {
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
do_assemble(void)
{
    // Initialize line map
    yasm::Linemap linemap;
    linemap.set(in_filename, 1, 1);

    std::auto_ptr<yasm::Arch> arch_auto =
        yasm::load_module<yasm::Arch>(arch_keyword);
    if (arch_auto.get() == 0) {
        print_error(String::compose(_("%1: could not load %2 `%3'"),
                                    _("FATAL"), _("architecture"),
                                    arch_keyword));
        return EXIT_FAILURE;
    }

    // Set up architecture using machine and parser.
    if (!machine_name.empty()) {
        if (!arch_auto->set_machine(machine_name)) {
            print_error(String::compose(
                _("%1: `%2' is not a valid %3 for %4 `%5'"),
                _("FATAL"), machine_name, _("machine"), _("architecture"),
                arch_keyword));
            return EXIT_FAILURE;
        }
    }

    if (!arch_auto->set_parser(parser_keyword)) {
        print_error(String::compose(
            _("%1: `%2' is not a valid %3 for %4 `%5'"),
            _("FATAL"), parser_keyword, _("parser"),
            _("architecture"), arch_keyword));
        return EXIT_FAILURE;
    }

    std::auto_ptr<yasm::Parser> parser =
        yasm::load_module<yasm::Parser>(parser_keyword);
    if (parser.get() == 0) {
        print_error(String::compose(_("%1: could not load %2 `%3'"),
                                    _("FATAL"), _("parser"), parser_keyword));
        return EXIT_FAILURE;
    }

    // If not already specified, default to the parser's default preproc.
    if (preproc_keyword.empty())
        preproc_keyword = parser->get_default_preproc_keyword();

    // Check to see if the requested preprocessor is in the allowed list
    // for the active parser.
    std::vector<std::string> preproc_keywords = parser->get_preproc_keywords();
    if (std::find(preproc_keywords.begin(), preproc_keywords.end(),
                  preproc_keyword) == preproc_keywords.end()) {
        print_error(String::compose(
            _("%1: `%2' is not a valid %3 for %4 `%5'"), _("FATAL"),
            preproc_keyword, _("preprocessor"), _("parser"), parser_keyword));
        return EXIT_FAILURE;
    }

    yasm::Errwarns errwarns;

    std::ifstream in_file(in_filename.c_str());

    std::auto_ptr<yasm::Preprocessor> preproc =
        yasm::load_module<yasm::Preprocessor>(preproc_keyword);
    if (preproc.get() == 0) {
        print_error(String::compose(_("%1: could not load %2 `%3'"),
                                    _("FATAL"), _("preprocessor"),
                                    preproc_keyword));
        return EXIT_FAILURE;
    }

    preproc->init(in_file, in_filename, linemap, errwarns);

    apply_preproc_builtins(*preproc);
    apply_preproc_saved_options(*preproc);

    // Create object
    yasm::Object object(in_filename, arch_auto, machine_name.empty(),
                        objfmt_keyword, dbgfmt_keyword);

    // determine the object filename if not specified
    if (obj_filename.empty()) {
        if (in_filename.empty())
            // Default to yasm.out if no obj filename specified
            obj_filename = "yasm.out";
        else {
            // replace (or add) extension to base filename
            std::string base_filename;
            yasm::splitpath(in_filename, base_filename);
            if (base_filename.empty())
                obj_filename = "yasm.out";
            else
                obj_filename =
                    replace_extension(base_filename,
                                      object.get_objfmt()->get_extension(),
                                      "yasm.out");
        }
    }
    object.set_object_fn(obj_filename);

    /* Get initial x86 BITS setting from object format */
    yasm::Arch* arch = object.get_arch();
    if (arch->get_keyword() == "x86")
        arch->set_var("mode_bits",
                      object.get_objfmt()->get_default_x86_mode_bits());

    arch->set_var("force_strict", force_strict);

    // Parse!
    parser->parse(object, preproc->get_stream(), !list_filename.empty(),
                  linemap, errwarns);

    if (check_errors(errwarns, linemap))
        return EXIT_FAILURE;

    // Finalize parse
    object.finalize(errwarns);
    if (check_errors(errwarns, linemap))
        return EXIT_FAILURE;

    // Optimize
    object.optimize(errwarns);
#if 0
    check_errors(errwarns, linemap);

    // generate any debugging information
    yasm_dbgfmt_generate(object, linemap, errwarns);
    check_errors(errwarns, object, linemap);

    // open the object file for output (if not already opened by dbg objfmt)
    if (!obj && strcmp(cur_objfmt_module->keyword, "dbg") != 0) {
        obj = open_file(obj_filename, "wb");
        if (!obj) {
            print_error(String::compose(_("could not open file `%1'"), filename));
            return EXIT_FAILURE;
        }
    }

    // Write the object file
    yasm_objfmt_output(object, obj?obj:stderr,
                       strcmp(cur_dbgfmt_module->keyword, "null"), errwarns);

    // Close object file
    if (obj)
        fclose(obj);

    // If we had an error at this point, we also need to delete the output
    // object file (to make sure it's not left newer than the source).
    if (yasm_errwarns_num_errors(errwarns, warning_error) > 0)
        remove(obj_filename);
    check_errors(errwarns, object, linemap);

    // Open and write the list file
    if (list_filename) {
        FILE *list = open_file(list_filename, "wt");
        if (!list) {
            print_error(String::compose(_("could not open file `%1'"), filename));
            return EXIT_FAILURE;
        }
        /* Initialize the list format */
        cur_listfmt = yasm_listfmt_create(cur_listfmt_module, in_filename,
                                          obj_filename);
        yasm_listfmt_output(cur_listfmt, list, linemap, cur_arch);
        fclose(list);
    }
#endif
    errwarns.output_all(linemap, warning_error, print_yasm_error,
                        print_yasm_warning);

    return EXIT_SUCCESS;
}

// main function
int
main(int argc, const char* argv[])
{
    errfile.std::basic_ios<char>::rdbuf(std::cerr.rdbuf());

#if 0
#if defined(HAVE_SETLOCALE) && defined(HAVE_LC_MESSAGES)
    setlocale(LC_MESSAGES, "");
#endif
#if defined(LOCALEDIR)
    bindtextdomain(PACKAGE, LOCALEDIR);
#endif
    textdomain(PACKAGE);
#endif

    // Initialize errwarn handling
    yasm::gettext_hook = handle_yasm_gettext;

    if (parse_cmdline(argc, argv, options, NELEMS(options), print_error))
        return EXIT_FAILURE;

    switch (special_option) {
        case SPECIAL_SHOW_HELP:
            // Does gettext calls internally
            help_msg(help_head, help_tail, options, NELEMS(options));
            return EXIT_SUCCESS;
        case SPECIAL_SHOW_VERSION:
            for (std::size_t i=0; i<NELEMS(version_msg); i++)
                std::cout << version_msg[i] << '\n';
            return EXIT_SUCCESS;
        case SPECIAL_SHOW_LICENSE:
#if 0
            for (std::size_t i=0; i<NELEMS(license_msg); i++)
                std::cout << license_msg[i] << '\n';
#endif
            return EXIT_SUCCESS;
        case SPECIAL_LISTED:
            // Printed out earlier
            return EXIT_SUCCESS;
        default:
            break;
    }

    // Open error file if specified.
    if (!error_filename.empty()) {
        errfile.close();
        errfile.open(error_filename.c_str());
        if (errfile.fail()) {
            print_error(String::compose(_("could not open file `%1'"),
                                        error_filename));
            return EXIT_FAILURE;
        }
    }

    // Default to x86 as the architecture
    if (arch_keyword.empty())
        arch_keyword = "x86";

    // Check for arch help
    if (machine_name == "help") {
        std::auto_ptr<yasm::Arch> arch_auto =
            yasm::load_module<yasm::Arch>(arch_keyword);
        std::cout << String::compose(_("Available %1 for %2 `%3':"),
                                     _("machines"), _("architecture"),
                                     arch_keyword) << '\n';
        std::map<std::string, std::string> machines =
            arch_auto->get_machines();

        for (std::map<std::string, std::string>::const_iterator
             i=machines.begin(), end=machines.end(); i != end; ++i)
            print_list_keyword_desc(i->second, i->first);
        return EXIT_SUCCESS;
    }

    // Determine input filename and open input file.
    if (in_filename.empty()) {
        print_error(_("No input files specified"));
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
    if (!list_filename.empty()) {
        // If not already specified, default to nasm as the list format.
        if (listfmt_keyword.empty())
            listfmt_keyword = "nasm";
    }

    // If not already specified, default to null as the debug format.
    if (dbgfmt_keyword.empty())
        dbgfmt_keyword = "null";

    try {
        return do_assemble();
    } catch (yasm::InternalError& err) {
        print_error(String::compose(_("INTERNAL ERROR: %1"), err.what()));
    } catch (yasm::Error& err) {
        print_error(String::compose(_("FATAL: %1"), err.what()));
    }
    return EXIT_FAILURE;
}

static bool
check_errors(yasm::Errwarns& errwarns, const yasm::Linemap& linemap)
{
    if (errwarns.num_errors(warning_error) > 0) {
        errwarns.output_all(linemap, warning_error, print_yasm_error,
                            print_yasm_warning);
        return true;
    }
    return false;
}

//
//  Command line options handlers
//

int
not_an_option_handler(const std::string& param)
{
    if (!in_filename.empty()) {
        print_error(
            _("warning: can open only one input file, only the last file will be processed"));
    }

    in_filename = param;
    return 0;
}

int
other_option_handler(const std::string& option)
{
    // Accept, but ignore, -O and -Onnn, for compatibility with NASM.
    if (option[0] == '-' && option[1] == 'O') {
        if (option.find_first_not_of("0123456789", 2) != std::string::npos)
            return 1;
        return 0;
    }
    return 1;
}

static int
opt_special_handler(/*@unused@*/ const std::string& cmd,
                    /*@unused@*/ const std::string& param, int extra)
{
    if (special_option == 0)
        special_option = static_cast<SpecialOption>(extra);
    return 0;
}

template <typename T>
static std::string
module_common_handler(const std::string& param, const char* name,
                      const char* name_plural)
{
    std::string keyword = String::lowercase(param);
    if (!yasm::is_module<T>(keyword)) {
        if (param == "help") {
            std::cout << String::compose(_("Available yasm %1:"), name_plural)
                      << '\n';
            list_module<T>();
            special_option = SPECIAL_LISTED;
            return keyword;
        }
        print_error(String::compose(_("%1: unrecognized %2 `%3'"), _("FATAL"),
                                    name, param));
        exit(EXIT_FAILURE);
    }
    return keyword;
}

static int
opt_arch_handler(/*@unused@*/ const std::string& cmd,
                 const std::string& param, /*@unused@*/ int extra)
{
    arch_keyword =
        module_common_handler<yasm::Arch>(param, _("architecture"),
                                          _("architectures"));
    return 0;
}

static int
opt_parser_handler(/*@unused@*/ const std::string& cmd,
                   const std::string& param, /*@unused@*/ int extra)
{
    parser_keyword =
        module_common_handler<yasm::Parser>(param, _("parser"), _("parsers"));
    return 0;
}

static int
opt_preproc_handler(/*@unused@*/ const std::string& cmd,
                    const std::string& param, /*@unused@*/ int extra)
{
    preproc_keyword =
        module_common_handler<yasm::Preprocessor>(param, _("preprocessor"),
                                                  _("preprocessors"));
    return 0;
}

static int
opt_objfmt_handler(/*@unused@*/ const std::string& cmd,
                   const std::string& param, /*@unused@*/ int extra)
{
    objfmt_keyword =
        module_common_handler<yasm::ObjectFormat>(param, _("object format"),
                                                  _("object formats"));
    return 0;
}

static int
opt_dbgfmt_handler(/*@unused@*/ const std::string& cmd,
                   const std::string& param, /*@unused@*/ int extra)
{
    dbgfmt_keyword =
        module_common_handler<yasm::DebugFormat>(param, _("debug format"),
                                                 _("debug formats"));
    return 0;
}

static int
opt_listfmt_handler(/*@unused@*/ const std::string& cmd,
                    const std::string& param, /*@unused@*/ int extra)
{
    listfmt_keyword =
        module_common_handler<yasm::ListFormat>(param, _("list format"),
                                                _("list formats"));
    return 0;
}

static int
opt_listfile_handler(/*@unused@*/ const std::string& cmd,
                     const std::string& param, /*@unused@*/ int extra)
{
    if (!list_filename.empty()) {
        print_error(
            _("warning: can output to only one list file, last specified used"));
    }

    list_filename = param;
    return 0;
}

static int
opt_objfile_handler(/*@unused@*/ const std::string& cmd,
                    const std::string& param, /*@unused@*/ int extra)
{
    if (!obj_filename.empty()) {
        print_error(
            _("warning: can output to only one object file, last specified used"));
    }

    obj_filename = param;
    return 0;
}

static int
opt_machine_handler(/*@unused@*/ const std::string& cmd,
                    const std::string& param, /*@unused@*/ int extra)
{
    machine_name = param;
    return 0;
}

static int
opt_strict_handler(/*@unused@*/ const std::string& cmd,
                   /*@unused@*/ const std::string& param,
                   /*@unused@*/ int extra)
{
    force_strict = true;
    return 0;
}

static int
opt_warning_handler(const std::string& cmd,
                    /*@unused@*/ const std::string& param, int extra)
{
    // is it disabling the warning instead of enabling?
    void (*action)(yasm::WarnClass wclass) = yasm::warn_enable;

    if (extra == 1) {
        // -w, disable warnings
        yasm::warn_disable_all();
        return 0;
    }

    // skip past 'W'
    std::string::size_type pos = 1;

    // detect no- prefix to disable the warning
    if (cmd.compare(pos, 3, "no-") == 0) {
        action = yasm::warn_disable;
        pos += 3;   // skip past it to get to the warning name
    }

    if (pos >= cmd.length())
        // just -W or -Wno-, so definitely not valid
        return 1;
    else if (cmd.compare(pos, std::string::npos, "error") == 0)
        warning_error = (action == yasm::warn_enable);
    else if (cmd.compare(pos, std::string::npos, "unrecognized-char") == 0)
        action(yasm::WARN_UNREC_CHAR);
    else if (cmd.compare(pos, std::string::npos, "orphan-labels") == 0)
        action(yasm::WARN_ORPHAN_LABEL);
    else if (cmd.compare(pos, std::string::npos, "uninit-contents") == 0)
        action(yasm::WARN_UNINIT_CONTENTS);
    else if (cmd.compare(pos, std::string::npos, "size-override") == 0)
        action(yasm::WARN_SIZE_OVERRIDE);
    else
        return 1;

    return 0;
}

static int
opt_error_file(/*@unused@*/ const std::string& cmd,
               const std::string& param, /*@unused@*/ int extra)
{
    if (!error_filename.empty()) {
        print_error(
            _("warning: can output to only one error file, last specified used"));
    }

    error_filename = param;
    return 0;
}

static int
opt_error_stdout(/*@unused@*/ const std::string& cmd,
                 /*@unused@*/ const std::string& param,
                 /*@unused@*/ int extra)
{
    // Clear any specified error filename
    error_filename.clear();
    errfile.std::basic_ios<char>::rdbuf(std::cout.rdbuf());
    return 0;
}

static int
preproc_only_handler(/*@unused@*/ const std::string& cmd,
                     /*@unused@*/ const std::string& param,
                     /*@unused@*/ int extra)
{
    preproc_only = 1;
    return 0;
}

static int
opt_include_option(/*@unused@*/ const std::string& cmd,
                   const std::string& param, /*@unused@*/ int extra)
{
#if 0
    yasm_add_include_path(param);
#endif
    return 0;
}

static int
opt_preproc_option(/*@unused@*/ const std::string& cmd,
                   const std::string& param, int extra)
{
    preproc_options.push_back(std::make_pair(param, extra));
    return 0;
}

static int
opt_ewmsg_handler(/*@unused@*/ const std::string& cmd,
                  const std::string& param, /*@unused@*/ int extra)
{
    if (String::nocase_equal(param, "gnu") ||
        String::nocase_equal(param, "gcc")) {
        ewmsg_style = EWSTYLE_GNU;
    } else if (String::nocase_equal(param, "vc")) {
        ewmsg_style = EWSTYLE_VC;
    } else
        print_error(String::compose(
            _("warning: unrecognized message style `%1'"), param));

    return 0;
}

static int
opt_makedep_handler(/*@unused@*/ const std::string& cmd,
                    /*@unused@*/ const std::string& param,
                    /*@unused@*/ int extra)
{
    // Also set preproc_only to 1, we don't want to generate code
    preproc_only = true;
    generate_make_dependencies = true;

    return 0;
}

static void
apply_preproc_builtins(yasm::Preprocessor& preproc)
{
    // Define standard YASM assembly-time macro constants
    std::string predef("__YASM_OBJFMT__=");
    predef += objfmt_keyword;
    preproc.define_builtin(predef);
}

static void
apply_preproc_saved_options(yasm::Preprocessor& preproc)
{
    for (CommandOptions::const_iterator i = preproc_options.begin(),
         end = preproc_options.end(); i != end; ++i) {
        switch (i->second) {
            case 0:
                preproc.add_include_file(i->first);
            case 1:
                preproc.predefine_macro(i->first);
            case 2:
                preproc.undefine_macro(i->first);
        }
    }
}

/// Replace extension on a filename (or append one if none is present).
/// @param orig     original filename
/// @param ext      extension, should include '.'
/// @return Filename with new extension.
static std::string
replace_extension(const std::string& orig, const std::string& ext,
                  const std::string& def)
{
    std::string::size_type origext = orig.find_last_of('.');
    if (origext != std::string::npos) {
        // Existing extension: make sure it's not the same as the replacement
        // (as we don't want to overwrite the source file).
        if (orig.compare(origext, std::string::npos, ext) == 0) {
            print_error(String::compose(
                _("file name already ends in `%1': output will be in `%2'"),
                ext, def));
            return def;
        }
        --origext;  // back up to before '.'
    } else {
        // No extension: make sure the output extension is not empty
        // (again, we don't want to overwrite the source file).
        if (ext.empty()) {
            print_error(String::compose(
                _("file name already has no extension: output will be in `%1'"),
                def));
            return def;
        }
    }

    // replace extension
    std::string out(orig, 0, origext);
    out += ext;
    return out;
}

static void
print_list_keyword_desc(const std::string& name, const std::string& keyword)
{
    std::cout << "    "
              << std::left << std::setfill(' ') << std::setw(12) << keyword
              << name << std::endl;
}

template <typename T>
static void
list_module()
{
    ddj::genericFactory<T>& factory = ddj::genericFactory<T>::instance();
    std::vector<std::string> list = factory.getRegisteredClasses();
    for (std::vector<std::string>::iterator i=list.begin(), end=list.end();
         i != end; ++i) {
        std::auto_ptr<T> obj = factory.create(*i);
        print_list_keyword_desc(obj->get_name(), *i);
    }
}

static void
print_error(const std::string& msg)
{
    errfile << "yasm: " << msg << std::endl;
}

static const char *
handle_yasm_gettext(const char *msgid)
{
    return gettext(msgid);
}

static const char *fmt[2] = {
        "%1:%2: %3%4",      // GNU
        "%1(%2) : %3%4"     // VC
};

static const char *fmt_noline[2] = {
        "%1: %3%4",         // GNU
        "%1 : %3%4"         // VC
};

static void
print_yasm_error(const std::string& filename, unsigned long line,
                 const std::string& msg,
                 const std::string& xref_fn, unsigned long xref_line,
                 const std::string& xref_msg)
{
    errfile <<
        String::compose(line ? fmt[ewmsg_style] : fmt_noline[ewmsg_style],
                        filename, line, "", msg) << std::endl;

    if (!xref_fn.empty() && !xref_msg.empty()) {
        errfile <<
            String::compose(xref_line ?
                            fmt[ewmsg_style] : fmt_noline[ewmsg_style],
                            xref_fn, xref_line, "", xref_msg) << std::endl;
    }
}

static void
print_yasm_warning(const std::string& filename, unsigned long line,
                   const std::string& msg)
{
    errfile <<
        String::compose(line ? fmt[ewmsg_style] : fmt_noline[ewmsg_style],
                        filename, line, _("warning: "), msg) << std::endl;
}
