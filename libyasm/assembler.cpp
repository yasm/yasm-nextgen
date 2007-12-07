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
#include "assembler.h"

#include "util.h"

#include <boost/scoped_ptr.hpp>

#include "arch.h"
#include "compose.h"
#include "debug_format.h"
#include "errwarn.h"
#include "factory.h"
#include "file.h"
#include "linemap.h"
#include "list_format.h"
#include "object.h"
#include "object_format.h"
#include "parser.h"
#include "preproc.h"


namespace yasm {

class Assembler::Impl {
public:
    Impl(const std::string& arch_keyword,
         const std::string& parser_keyword,
         const std::string& objfmt_keyword);
    ~Impl();

    void set_machine(const std::string& machine);
    void set_preproc(const std::string& preproc_keyword);
    void set_dbgfmt(const std::string& dbgfmt_keyword);
    void set_listfmt(const std::string& listfmt_keyword);
    bool assemble(std::istream& is,
                  const std::string& src_filename,
                  bool warning_error);

    boost::scoped_ptr<Arch> m_arch;
    boost::scoped_ptr<Parser> m_parser;
    boost::scoped_ptr<Preprocessor> m_preproc;
    boost::scoped_ptr<ObjectFormat> m_objfmt;
    boost::scoped_ptr<DebugFormat> m_dbgfmt;
    boost::scoped_ptr<ListFormat> m_listfmt;

    boost::scoped_ptr<Object> m_object;

    Linemap m_linemap;
    Errwarns m_errwarns;

    std::string m_obj_filename;
    std::string m_machine;
};

Assembler::Impl::Impl(const std::string& arch_keyword,
                      const std::string& parser_keyword,
                      const std::string& objfmt_keyword)
    : m_arch(load_module<Arch>(arch_keyword).release()),
      m_parser(load_module<Parser>(parser_keyword).release()),
      m_preproc(0),
      m_objfmt(load_module<ObjectFormat>(objfmt_keyword).release()),
      m_dbgfmt(0),
      m_listfmt(0),
      m_object(0)
{
    if (m_arch.get() == 0)
        throw Error(String::compose(N_("could not load architecture `%1'"),
                                    arch_keyword));

    if (m_parser.get() == 0)
        throw Error(String::compose(N_("could not load parser `%1'"),
                                    parser_keyword));

    if (m_objfmt.get() == 0)
        throw Error(String::compose(N_("could not load object format `%1'"),
                                    objfmt_keyword));

    // Ensure architecture supports parser.
    if (!m_arch->set_parser(parser_keyword))
        throw Error(String::compose(
            _("`%1' is not a valid parser for architecture `%2'"),
            parser_keyword, arch_keyword));

    // Get initial x86 BITS setting from object format
    if (m_arch->get_keyword() == "x86")
        m_arch->set_var("mode_bits",
                        m_objfmt->get_default_x86_mode_bits());
}

Assembler::Impl::~Impl()
{
}

Assembler::Assembler(const std::string& arch_keyword,
                     const std::string& parser_keyword,
                     const std::string& objfmt_keyword)
    : m_impl(new Impl(arch_keyword, parser_keyword, objfmt_keyword))
{
    m_impl->set_preproc(m_impl->m_parser->get_default_preproc_keyword());
}

Assembler::~Assembler()
{
}

void
Assembler::set_obj_filename(const std::string& obj_filename)
{
    m_impl->m_obj_filename = obj_filename;
}

void
Assembler::Impl::set_machine(const std::string& machine)
{
    if (!m_arch->set_machine(machine))
        throw Error(String::compose(
            N_("`%1' is not a valid machine for architecture `%2'"),
            machine, m_arch->get_keyword()));

    m_machine = machine;
}

void
Assembler::set_machine(const std::string& machine)
{
    m_impl->set_machine(machine);
}

void
Assembler::Impl::set_preproc(const std::string& preproc_keyword)
{
    // Check to see if the requested preprocessor is in the allowed list
    // for the active parser.
    std::vector<std::string> preproc_keywords =
        m_parser->get_preproc_keywords();
    if (std::find(preproc_keywords.begin(), preproc_keywords.end(),
                  preproc_keyword) == preproc_keywords.end()) {
        throw Error(String::compose(
            N_("`%1' is not a valid preprocessor for parser `%2'"),
            preproc_keyword, m_parser->get_keyword()));
    }

    std::auto_ptr<Preprocessor> preproc = 
        load_module<Preprocessor>(preproc_keyword);
    if (preproc.get() == 0) {
        throw Error(String::compose(N_("could not load preprocessor `%1'"),
                                    preproc_keyword));
    }
    m_preproc.reset(preproc.release());
}

void
Assembler::set_preproc(const std::string& preproc_keyword)
{
    m_impl->set_preproc(preproc_keyword);
}

void
Assembler::Impl::set_dbgfmt(const std::string& dbgfmt_keyword)
{
    // Check to see if the requested debug format is in the allowed list
    // for the active object format.
    std::vector<std::string> dbgfmt_keywords =
        m_objfmt->get_dbgfmt_keywords();
    std::vector<std::string>::iterator f =
        std::find(dbgfmt_keywords.begin(), dbgfmt_keywords.end(),
                  dbgfmt_keyword);
    if (f == dbgfmt_keywords.end()) {
        throw Error(String::compose(
            N_("`%1' is not a valid debug format for object format `%2'"),
            dbgfmt_keyword, m_objfmt->get_keyword()));
    }

    std::auto_ptr<DebugFormat> dbgfmt = 
        load_module<DebugFormat>(dbgfmt_keyword);
    if (dbgfmt.get() == 0)
        throw Error(String::compose(N_("could not load debug format `%1'"),
                                    dbgfmt_keyword));
    m_dbgfmt.reset(dbgfmt.release());
}

void
Assembler::set_dbgfmt(const std::string& dbgfmt_keyword)
{
    m_impl->set_dbgfmt(dbgfmt_keyword);
}

void
Assembler::Impl::set_listfmt(const std::string& listfmt_keyword)
{
    std::auto_ptr<ListFormat> listfmt = 
        load_module<ListFormat>(listfmt_keyword);
    if (listfmt.get() == 0)
        throw Error(String::compose(N_("could not load list format `%1'"),
                                    listfmt_keyword));
    m_listfmt.reset(listfmt.release());
}

void
Assembler::set_listfmt(const std::string& listfmt_keyword)
{
    m_impl->set_listfmt(listfmt_keyword);
}

bool
Assembler::Impl::assemble(std::istream& is, const std::string& src_filename,
                          bool warning_error)
{
    // determine the object filename if not specified
    if (m_obj_filename.empty()) {
        if (src_filename.empty())
            // Default to yasm.out if no obj filename specified
            m_obj_filename = "yasm.out";
        else {
            // replace (or add) extension to base filename
            std::string base_filename;
            splitpath(src_filename, base_filename);
            if (base_filename.empty())
                m_obj_filename = "yasm.out";
            else
                m_obj_filename =
                    replace_extension(base_filename,
                                      m_objfmt->get_extension(),
                                      "yasm.out");
        }
    }

    if (m_machine.empty()) {
        // If we're using x86 and the default objfmt bits is 64, default the
        // machine to amd64.  When we get more arches with multiple machines,
        // we should do this in a more modular fashion.
        if (m_arch->get_keyword() == "x86" &&
            m_objfmt->get_default_x86_mode_bits() == 64)
            set_machine("amd64");
    }

    // Create object
    m_object.reset(new Object(src_filename, m_obj_filename, m_arch.get()));

    // Initialize the object format
    if (!m_objfmt->set_object(m_object.get())) {
        throw Error(String::compose(
            N_("object format `%1' does not support architecture `%2' machine `%3'"),
            m_objfmt->get_keyword(),
            m_arch->get_keyword(),
            m_arch->get_machine()));
    }

    // Add an initial "default" section to object
    m_object->set_cur_section(m_objfmt->add_default_section());

    // Default to null as the debug format if not specified
    if (m_dbgfmt.get() == 0)
        set_dbgfmt("null");

    // Initialize the debug format
    if (!m_dbgfmt->set_object(m_object.get())) {
        throw Error(String::compose(
            N_("debug format `%1' does not work with object format `%2'"),
            m_dbgfmt->get_keyword(), m_objfmt->get_keyword()));
    }

    // Initialize line map
    m_linemap.set(src_filename, 1, 1);

    // Initialize preprocessor
    m_preproc->init(is, src_filename, m_linemap, m_errwarns);

    // Parse!
    m_parser->parse(*m_object, *m_preproc, m_listfmt.get() != 0, m_linemap,
                    m_errwarns);

    if (m_errwarns.num_errors(warning_error) > 0)
        return false;

    // Finalize parse
    m_object->finalize(m_errwarns);
    if (m_errwarns.num_errors(warning_error) > 0)
        return false;

    // Optimize
    m_object->optimize(m_errwarns);

    //object.put(std::cout, 0);

    if (m_errwarns.num_errors(warning_error) > 0)
        return false;

    // generate any debugging information
    //m_dbgfmt->generate(m_linemap, m_errwarns);
    if (m_errwarns.num_errors(warning_error) > 0)
        return false;

    return true;
}

bool
Assembler::assemble(std::istream& is, const std::string& src_filename,
                    bool warning_error)
{
    return m_impl->assemble(is, src_filename, warning_error);
}

bool
Assembler::output(std::ostream& os, bool warning_error)
{
    // Write the object file
    m_impl->m_objfmt->output(os,
                             m_impl->m_dbgfmt->get_keyword() != "null",
                             m_impl->m_errwarns);

    if (m_impl->m_errwarns.num_errors(warning_error) > 0)
        return false;

    return true;
}

Object*
Assembler::get_object()
{
    return m_impl->m_object.get();
}

Preprocessor*
Assembler::get_preproc()
{
    return m_impl->m_preproc.get();
}

Arch*
Assembler::get_arch()
{
    return m_impl->m_arch.get();
}

Errwarns*
Assembler::get_errwarns()
{
    return &m_impl->m_errwarns;
}

Linemap*
Assembler::get_linemap()
{
    return &m_impl->m_linemap;
}

std::string
Assembler::get_obj_filename() const
{
    return m_impl->m_obj_filename;
}

} // namespace yasm
