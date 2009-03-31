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
#include <util.h>

#include <iomanip>
#include <iostream>
#include <fstream>

#include <libyasmx/Arch.h>
#include <libyasmx/Assembler.h>
#include <libyasmx/Compose.h>
#include <libyasmx/DebugFormat.h>
#include <libyasmx/errwarn.h>
#include <libyasmx/Errwarns.h>
#include <libyasmx/file.h>
#include <libyasmx/Linemap.h>
#include <libyasmx/ListFormat.h>
#include <libyasmx/Module.h>
#include <libyasmx/nocase.h>
#include <libyasmx/ObjectFormat.h>
#include <libyasmx/Parser.h>
#include <libyasmx/plugin.h>
#include <libyasmx/Preprocessor.h>
#include <libyasmx/registry.h>

#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif

#include "options.h"

#include "frontends/license.cpp"


// Preprocess-only buffer size
#define PREPROC_BUF_SIZE    16384

static std::string obj_filename, in_filename;
static std::string list_filename;
static std::string machine_name;
static enum SpecialOption
{
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
static enum
{
    EWSTYLE_GNU = 0,
    EWSTYLE_VC
} ewmsg_style = EWSTYLE_GNU;

// version message
/*@observer@*/ static const char* version_msg[] =
{
    PACKAGE_NAME " " PACKAGE_INTVER "." PACKAGE_BUILD,
    "Compiled on " __DATE__ ".",
    "Copyright (c) 2001-2008 Peter Johnson and other Yasm developers.",
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

static void
print_error(const std::string& msg)
{
    errfile << "yasm: " << msg << std::endl;
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
    std::vector<std::string> list = yasm::get_modules<T>();
    for (std::vector<std::string>::iterator i=list.begin(), end=list.end();
         i != end; ++i)
    {
        std::auto_ptr<T> obj = yasm::load_module<T>(*i);
        print_list_keyword_desc(obj->get_name(), *i);
    }
}

//
//  Command line options handlers
//

bool
not_an_option_handler(const std::string& param)
{
    if (!in_filename.empty())
    {
        print_error(
            _("warning: can open only one input file, only the last file will be processed"));
    }

    in_filename = param;
    return true;
}

bool
other_option_handler(const std::string& option)
{
    // Accept, but ignore, -O and -Onnn, for compatibility with NASM.
    if (option[0] == '-' && option[1] == 'O')
    {
        if (option.find_first_not_of("0123456789", 2) != std::string::npos)
            return false;
        return true;
    }
    return false;
}

static bool
opt_special_handler(/*@unused@*/ const std::string& cmd,
                    /*@unused@*/ const std::string& param, int extra)
{
    if (special_option == 0)
        special_option = static_cast<SpecialOption>(extra);
    return true;
}

template <typename T>
static std::string
module_common_handler(const std::string& param, const char* name,
                      const char* name_plural)
{
    std::string keyword = String::lowercase(param);
    if (!yasm::is_module<T>(keyword))
    {
        if (param == "help")
        {
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

static bool
opt_arch_handler(/*@unused@*/ const std::string& cmd,
                 const std::string& param, /*@unused@*/ int extra)
{
    arch_keyword =
        module_common_handler<yasm::Arch>(param, _("architecture"),
                                          _("architectures"));
    return true;
}

static bool
opt_parser_handler(/*@unused@*/ const std::string& cmd,
                   const std::string& param, /*@unused@*/ int extra)
{
    parser_keyword =
        module_common_handler<yasm::Parser>(param, _("parser"), _("parsers"));
    return true;
}

static bool
opt_preproc_handler(/*@unused@*/ const std::string& cmd,
                    const std::string& param, /*@unused@*/ int extra)
{
    preproc_keyword =
        module_common_handler<yasm::Preprocessor>(param, _("preprocessor"),
                                                  _("preprocessors"));
    return true;
}

static bool
opt_objfmt_handler(/*@unused@*/ const std::string& cmd,
                   const std::string& param, /*@unused@*/ int extra)
{
    objfmt_keyword =
        module_common_handler<yasm::ObjectFormat>(param, _("object format"),
                                                  _("object formats"));
    return true;
}

static bool
opt_dbgfmt_handler(/*@unused@*/ const std::string& cmd,
                   const std::string& param, /*@unused@*/ int extra)
{
    dbgfmt_keyword =
        module_common_handler<yasm::DebugFormat>(param, _("debug format"),
                                                 _("debug formats"));
    return true;
}

static bool
opt_listfmt_handler(/*@unused@*/ const std::string& cmd,
                    const std::string& param, /*@unused@*/ int extra)
{
    listfmt_keyword =
        module_common_handler<yasm::ListFormat>(param, _("list format"),
                                                _("list formats"));
    return true;
}

static bool
opt_listfile_handler(/*@unused@*/ const std::string& cmd,
                     const std::string& param, /*@unused@*/ int extra)
{
    if (!list_filename.empty())
    {
        print_error(
            _("warning: can output to only one list file, last specified used"));
    }

    list_filename = param;
    return true;
}

static bool
opt_objfile_handler(/*@unused@*/ const std::string& cmd,
                    const std::string& param, /*@unused@*/ int extra)
{
    if (!obj_filename.empty())
    {
        print_error(
            _("warning: can output to only one object file, last specified used"));
    }

    obj_filename = param;
    return true;
}

static bool
opt_machine_handler(/*@unused@*/ const std::string& cmd,
                    const std::string& param, /*@unused@*/ int extra)
{
    machine_name = param;
    return true;
}

static bool
opt_strict_handler(/*@unused@*/ const std::string& cmd,
                   /*@unused@*/ const std::string& param,
                   /*@unused@*/ int extra)
{
    force_strict = true;
    return true;
}

static bool
opt_warning_handler(const std::string& cmd,
                    /*@unused@*/ const std::string& param, int extra)
{
    // is it disabling the warning instead of enabling?
    void (*action)(yasm::WarnClass wclass) = yasm::warn_enable;

    if (extra == 1)
    {
        // -w, disable warnings
        yasm::warn_disable_all();
        return true;
    }

    // skip past 'W'
    std::string::size_type pos = 1;

    // detect no- prefix to disable the warning
    if (cmd.compare(pos, 3, "no-") == 0)
    {
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
        return false;

    return true;
}

static bool
opt_error_file(/*@unused@*/ const std::string& cmd,
               const std::string& param, /*@unused@*/ int extra)
{
    if (!error_filename.empty())
    {
        print_error(
            _("warning: can output to only one error file, last specified used"));
    }

    error_filename = param;
    return true;
}

static bool
opt_error_stdout(/*@unused@*/ const std::string& cmd,
                 /*@unused@*/ const std::string& param,
                 /*@unused@*/ int extra)
{
    // Clear any specified error filename
    error_filename.clear();
    errfile.std::basic_ios<char>::rdbuf(std::cout.rdbuf());
    return true;
}

static bool
preproc_only_handler(/*@unused@*/ const std::string& cmd,
                     /*@unused@*/ const std::string& param,
                     /*@unused@*/ int extra)
{
    preproc_only = 1;
    return true;
}

static bool
opt_include_option(/*@unused@*/ const std::string& cmd,
                   const std::string& param, /*@unused@*/ int extra)
{
#if 0
    yasm_add_include_path(param);
#endif
    return true;
}

static bool
opt_preproc_option(/*@unused@*/ const std::string& cmd,
                   const std::string& param, int extra)
{
    preproc_options.push_back(std::make_pair(param, extra));
    return true;
}

static bool
opt_ewmsg_handler(/*@unused@*/ const std::string& cmd,
                  const std::string& param, /*@unused@*/ int extra)
{
    if (String::nocase_equal(param, "gnu") ||
        String::nocase_equal(param, "gcc"))
    {
        ewmsg_style = EWSTYLE_GNU;
    }
    else if (String::nocase_equal(param, "vc"))
    {
        ewmsg_style = EWSTYLE_VC;
    }
    else
        print_error(String::compose(
            _("warning: unrecognized message style `%1'"), param));

    return true;
}

static bool
opt_makedep_handler(/*@unused@*/ const std::string& cmd,
                    /*@unused@*/ const std::string& param,
                    /*@unused@*/ int extra)
{
    // Also set preproc_only to 1, we don't want to generate code
    preproc_only = true;
    generate_make_dependencies = true;

    return true;
}

#ifndef BUILD_STATIC
static bool
opt_plugin_handler(/*@unused@*/ const std::string& cmd,
                   const std::string& param,
                   /*@unused@*/ int extra)
{
    if (!yasm::load_plugin(param))
        print_error(String::compose(
            _("warning: could not load plugin `%s'"), param));
    return true;
}
#endif

static void
apply_preproc_builtins(yasm::Preprocessor* preproc)
{
    // Define standard YASM assembly-time macro constants
    std::string predef("__YASM_OBJFMT__=");
    predef += objfmt_keyword;
    preproc->define_builtin(predef);
}

static void
apply_preproc_saved_options(yasm::Preprocessor* preproc)
{
    for (CommandOptions::const_iterator i = preproc_options.begin(),
         end = preproc_options.end(); i != end; ++i)
    {
        switch (i->second)
        {
            case 0:
                preproc->add_include_file(i->first);
            case 1:
                preproc->predefine_macro(i->first);
            case 2:
                preproc->undefine_macro(i->first);
        }
    }
}

static const char *
handle_yasm_gettext(const char *msgid)
{
    return yasm_gettext(msgid);
}

static const char *fmt[2] =
{
    "%1:%2: %3%4",      // GNU
    "%1(%2) : %3%4"     // VC
};

static const char *fmt_noline[2] =
{
    "%1: %3%4",         // GNU
    "%1 : %3%4"         // VC
};

static void
print_yasm_error(const std::string& filename,
                 unsigned long line,
                 const std::string& msg,
                 const std::string& xref_fn,
                 unsigned long xref_line,
                 const std::string& xref_msg)
{
    errfile <<
        String::compose(line ? fmt[ewmsg_style] : fmt_noline[ewmsg_style],
                        filename, line, _("error: "), msg) << std::endl;

    if (!xref_fn.empty() && !xref_msg.empty())
    {
        errfile <<
            String::compose(xref_line ?
                            fmt[ewmsg_style] : fmt_noline[ewmsg_style],
                            xref_fn, xref_line, _("error: "), xref_msg)
            << std::endl;
    }
}

static void
print_yasm_warning(const std::string& filename,
                   unsigned long line,
                   const std::string& msg)
{
    errfile <<
        String::compose(line ? fmt[ewmsg_style] : fmt_noline[ewmsg_style],
                        filename, line, _("warning: "), msg) << std::endl;
}

// command line options
static OptOption options[] =
{
    { 0, "version", false, opt_special_handler, SPECIAL_SHOW_VERSION,
      N_("show version text"), NULL },
    { 0, "license", false, opt_special_handler, SPECIAL_SHOW_LICENSE,
      N_("show license text"), NULL },
    { 'h', "help", false, opt_special_handler, SPECIAL_SHOW_HELP,
      N_("show help text"), NULL },
    { 'a', "arch", true, opt_arch_handler, 0,
      N_("select architecture (list with -a help)"), N_("arch") },
    { 'p', "parser", true, opt_parser_handler, 0,
      N_("select parser (list with -p help)"), N_("parser") },
    { 'r', "preproc", true, opt_preproc_handler, 0,
      N_("select preprocessor (list with -r help)"), N_("preproc") },
    { 'f', "oformat", true, opt_objfmt_handler, 0,
      N_("select object format (list with -f help)"), N_("format") },
    { 'g', "dformat", true, opt_dbgfmt_handler, 0,
      N_("select debugging format (list with -g help)"), N_("debug") },
    { 'L', "lformat", true, opt_listfmt_handler, 0,
      N_("select list format (list with -L help)"), N_("list") },
    { 'l', "list", true, opt_listfile_handler, 0,
      N_("name of list-file output"), N_("listfile") },
    { 'o', "objfile", true, opt_objfile_handler, 0,
      N_("name of object-file output"), N_("filename") },
    { 'm', "machine", true, opt_machine_handler, 0,
      N_("select machine (list with -m help)"), N_("machine") },
    { 0, "force-strict", false, opt_strict_handler, 0,
      N_("treat all sized operands as if `strict' was used"), NULL },
    { 'w', NULL, false, opt_warning_handler, 1,
      N_("inhibits warning messages"), NULL },
    { 'W', NULL, false, opt_warning_handler, 0,
      N_("enables/disables warning"), NULL },
    { 'M', NULL, false, opt_makedep_handler, 0,
      N_("generate Makefile dependencies on stdout"), NULL },
    { 'E', NULL, true, opt_error_file, 0,
      N_("redirect error messages to file"), N_("file") },
    { 's', NULL, false, opt_error_stdout, 0,
      N_("redirect error messages to stdout"), NULL },
    { 'e', "preproc-only", false, preproc_only_handler, 0,
      N_("preprocess only (writes output to stdout by default)"), NULL },
    { 'i', NULL, true, opt_include_option, 0,
      N_("add include path"), N_("path") },
    { 'I', NULL, true, opt_include_option, 0,
      N_("add include path"), N_("path") },
    { 'P', NULL, true, opt_preproc_option, 0,
      N_("pre-include file"), N_("filename") },
    { 'd', NULL, true, opt_preproc_option, 1,
      N_("pre-define a macro, optionally to value"), N_("macro[=value]") },
    { 'D', NULL, true, opt_preproc_option, 1,
      N_("pre-define a macro, optionally to value"), N_("macro[=value]") },
    { 'u', NULL, true, opt_preproc_option, 2,
      N_("undefine a macro"), N_("macro") },
    { 'U', NULL, true, opt_preproc_option, 2,
      N_("undefine a macro"), N_("macro") },
    { 'X', NULL, true, opt_ewmsg_handler, 0,
      N_("select error/warning message style (`gnu' or `vc')"), N_("style") },
#ifndef BUILD_STATIC
    { 'N', "plugin", true, opt_plugin_handler, 0,
      N_("load plugin module"), N_("plugin") },
#endif
};

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
do_assemble(void)
{
    yasm::Assembler assembler(arch_keyword, parser_keyword, objfmt_keyword);

    // Set object filename if specified.
    if (!obj_filename.empty())
        assembler.set_obj_filename(obj_filename);

    // Set machine if specified.
    if (!machine_name.empty())
        assembler.set_machine(machine_name);

    // Set preprocessor if specified.
    if (!preproc_keyword.empty())
        assembler.set_preproc(preproc_keyword);

    apply_preproc_builtins(assembler.get_preproc());
    apply_preproc_saved_options(assembler.get_preproc());

    assembler.get_arch()->set_var("force_strict", force_strict);

    // open the input file
    std::istream* in;
    std::ifstream in_file;
    if (in_filename == "-")
        in = &std::cin;
    else
    {
        in_file.open(in_filename.c_str());
        if (!in_file)
            throw yasm::Error(String::compose(_("could not open file `%1'"),
                              in_filename));
        in = &in_file;
    }

    // assemble the input.
    if (!assembler.assemble(*in, in_filename, warning_error))
    {
        // An error occurred during assembly; output all errors and warnings
        // and then exit.
        assembler.get_errwarns()->output_all(*assembler.get_linemap(),
                                             warning_error,
                                             print_yasm_error,
                                             print_yasm_warning);
        return EXIT_FAILURE;
    }
    in_file.close();

    // open the object file for output (if not already opened by dbg objfmt)
    std::ostream* out;
    std::ofstream out_file;
    if (objfmt_keyword == "dbg")
        out = &std::cerr;
    else
    {
        out_file.open(assembler.get_obj_filename().c_str(), std::ios::binary);
        if (!out_file)
            throw yasm::Error(String::compose(_("could not open file `%1'"),
                              obj_filename));
        out = &out_file;
    }

    if (!assembler.output(*out, warning_error))
    {
        // An error occurred during output; output all errors and warnings.
        // If we had an error at this point, we also need to delete the output
        // object file (to make sure it's not left newer than the source).
        assembler.get_errwarns()->output_all(*assembler.get_linemap(),
                                             warning_error,
                                             print_yasm_error,
                                             print_yasm_warning);
        out_file.close();
        remove(assembler.get_obj_filename().c_str());
        return EXIT_FAILURE;
    }

    // close object file
    out_file.close();
#if 0
    // Open and write the list file
    if (list_filename)
    {
        FILE *list = open_file(list_filename, "wt");
        if (!list)
        {
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
    assembler.get_errwarns()->output_all(*assembler.get_linemap(),
                                         warning_error,
                                         print_yasm_error,
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
    yasm_bindtextdomain(PACKAGE, LOCALEDIR);
#endif
    yasm_textdomain(PACKAGE);
#endif

    // Initialize errwarn handling
    yasm::gettext_hook = handle_yasm_gettext;

    // Load standard modules
    if (!yasm::load_standard_plugins())
    {
        print_error(_("FATAL: could not load standard modules"));
        return EXIT_FAILURE;
    }

    if (!parse_cmdline(argc, argv, options, NELEMS(options), print_error))
        return EXIT_FAILURE;

    switch (special_option)
    {
        case SPECIAL_SHOW_HELP:
            // Does gettext calls internally
            help_msg(help_head, help_tail, options, NELEMS(options));
            return EXIT_SUCCESS;
        case SPECIAL_SHOW_VERSION:
            for (std::size_t i=0; i<NELEMS(version_msg); i++)
                std::cout << version_msg[i] << '\n';
            return EXIT_SUCCESS;
        case SPECIAL_SHOW_LICENSE:
            for (std::size_t i=0; i<NELEMS(license_msg); i++)
                std::cout << license_msg[i] << '\n';
            return EXIT_SUCCESS;
        case SPECIAL_LISTED:
            // Printed out earlier
            return EXIT_SUCCESS;
        default:
            break;
    }

    // Open error file if specified.
    if (!error_filename.empty())
    {
        errfile.close();
        errfile.open(error_filename.c_str());
        if (errfile.fail())
        {
            print_error(String::compose(_("could not open file `%1'"),
                                        error_filename));
            return EXIT_FAILURE;
        }
    }

    // Default to x86 as the architecture
    if (arch_keyword.empty())
        arch_keyword = "x86";

    // Check for arch help
    if (machine_name == "help")
    {
        std::auto_ptr<yasm::Arch> arch_auto =
            yasm::load_module<yasm::Arch>(arch_keyword);
        std::cout << String::compose(_("Available %1 for %2 `%3':"),
                                     _("machines"), _("architecture"),
                                     arch_keyword) << '\n';
        yasm::Arch::MachineNames machines = arch_auto->get_machines();

        for (yasm::Arch::MachineNames::const_iterator
             i=machines.begin(), end=machines.end(); i != end; ++i)
            print_list_keyword_desc(i->second, i->first);
        return EXIT_SUCCESS;
    }

    // Determine input filename and open input file.
    if (in_filename.empty())
    {
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
    if (!list_filename.empty())
    {
        // If not already specified, default to nasm as the list format.
        if (listfmt_keyword.empty())
            listfmt_keyword = "nasm";
    }

    try
    {
        return do_assemble();
    }
    catch (yasm::Error& err)
    {
        print_error(String::compose(_("FATAL: %1"), err.what()));
    }
    return EXIT_FAILURE;
}

