//
// Error and warning reporting and related functions.
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
#include "util.h"

#include <list>

#include "linemap.h"
#include "errwarn.h"

namespace yasm {

static const int MSG_MAXSIZE = 1024;

// Default handlers for replacable functions
static const char *def_gettext_hook(const char *msgid);

// Storage for errwarn's "extern" functions
const char * (*gettext_hook) (const char *msgid) = def_gettext_hook;

class ErrwarnManager {
public:
    static ErrwarnManager & instance()
    {
        static ErrwarnManager inst;
        return inst;
    }

    /// Warning indicator.
    class Warning {
    public:
        Warning(WarnClass wclass, const std::string& wstr);

        WarnClass m_wclass;
        std::string m_wstr;
    };

    std::list<Warning> m_warns;

    /// Enabled warnings.  See errwarn.h for a list.
    unsigned long m_wclass_enabled;

private:
    ErrwarnManager();
    ErrwarnManager(const ErrwarnManager &) {}
    ErrwarnManager & operator= (const ErrwarnManager &) { return *this; }
    ~ErrwarnManager();
};

class Errwarns::ErrwarnData {
public:
    ~ErrwarnData();

    enum { WE_UNKNOWN, WE_ERROR, WE_WARNING, WE_PARSERERROR } m_type;

    unsigned long m_line;
    unsigned long m_xrefline;
    std::string m_msg;
    std::string m_xrefmsg;
};

static const char *
def_gettext_hook(const char *msgid)
{
    return msgid;
}

ErrwarnManager::ErrwarnManager()
{
    // Default enabled warnings.  See errwarn.h for a list.
    m_wclass_enabled = 
        (1UL<<WARN_GENERAL) | (1UL<<WARN_UNREC_CHAR) |
        (1UL<<WARN_PREPROC) | (0UL<<WARN_ORPHAN_LABEL) |
        (1UL<<WARN_UNINIT_CONTENTS);
}

ErrwarnManager::~ErrwarnManager()
{
}

std::string
conv_unprint(int ch)
{
    std::string unprint;

    if (((ch & ~0x7F) != 0) /*!isascii(ch)*/ && !isprint(ch)) {
        unprint += 'M';
        unprint += '-';
        ch &= toascii(ch);
    }
    if (iscntrl(ch)) {
        unprint += '^';
        unprint += (ch == '\177') ? '?' : ch | 0100;
    } else
        unprint += ch;

    return unprint;
}

InternalError::InternalError(const std::string& message)
    : std::runtime_error(gettext_hook(N_("INTERNAL ERROR: ")) + message)
{
}

Fatal::Fatal(const std::string& message)
    : m_message(gettext_hook(N_("FATAL: ")) + message)
{
}
#if 0
/// Create an errwarn structure in the correct linked list location.
/// If replace_parser_error is nonzero, overwrites the last error if its
/// type is WE_PARSERERROR.
static errwarn_data *
errwarn_data_new(yasm_errwarns *errwarns, unsigned long line,
                 int replace_parser_error)
{
    errwarn_data *first, *next, *ins_we, *we;
    enum { INS_NONE, INS_HEAD, INS_AFTER } action = INS_NONE;

    // Find the entry with either line=line or the last one with line<line.
    // Start with the last entry added to speed the search.
    ins_we = errwarns->previous_we;
    first = SLIST_FIRST(&errwarns->errwarns);
    if (!ins_we || !first)
        action = INS_HEAD;
    while (action == INS_NONE) {
        next = SLIST_NEXT(ins_we, link);
        if (line < ins_we->line) {
            if (ins_we == first)
                action = INS_HEAD;
            else
                ins_we = first;
        } else if (!next)
            action = INS_AFTER;
        else if (line >= ins_we->line && line < next->line)
            action = INS_AFTER;
        else
            ins_we = next;
    }

    if (replace_parser_error && ins_we && ins_we->type == WE_PARSERERROR) {
        // overwrite last error
        we = ins_we;
    } else {
        // add a new error
        we = yasm_xmalloc(sizeof(errwarn_data));

        we->type = WE_UNKNOWN;
        we->line = line;
        we->xrefline = 0;
        we->msg = NULL;
        we->xrefmsg = NULL;

        if (action == INS_HEAD)
            SLIST_INSERT_HEAD(&errwarns->errwarns, we, link);
        else if (action == INS_AFTER) {
            assert(ins_we != NULL);
            SLIST_INSERT_AFTER(ins_we, we, link);
        } else
            yasm_internal_error(N_("Unexpected errwarn insert action"));
    }

    // Remember previous err/warn
    errwarns->previous_we = we;

    return we;
}
#endif

Error::Error(const std::string& message)
    : m_message(message),
      m_xrefline(0)
{
}

void
warn_clear()
{
    ErrwarnManager::instance().m_warns.clear();
}

WarnClass
warn_occurred()
{
    ErrwarnManager &manager = ErrwarnManager::instance();
    if (manager.m_warns.empty())
        return WARN_NONE;
    return manager.m_warns.front().m_wclass;
}

ErrwarnManager::Warning::Warning(WarnClass wclass,
                                 const std::string& str)
    : m_wclass(wclass), m_wstr(str)
{
}

void
warn_set(WarnClass wclass, const std::string& str)
{
    ErrwarnManager &manager = ErrwarnManager::instance();

    if (!(manager.m_wclass_enabled & (1UL<<wclass)))
        return;     // warning is part of disabled class

    manager.m_warns.push_back(ErrwarnManager::Warning(wclass, str));
}

WarnClass
warn_fetch(std::string& str)
{
    ErrwarnManager& manager = ErrwarnManager::instance();

    if (manager.m_warns.empty())
        return WARN_NONE;

    ErrwarnManager::Warning& w = manager.m_warns.front();

    WarnClass wclass = w.m_wclass;
    str = w.m_wstr;

    manager.m_warns.pop_front();
    return wclass;
}

void
warn_enable(WarnClass num)
{
    ErrwarnManager::instance().m_wclass_enabled |= (1UL<<num);
}

void
warn_disable(WarnClass num)
{
    ErrwarnManager::instance().m_wclass_enabled &= ~(1UL<<num);
}

void
warn_disable_all()
{
    ErrwarnManager::instance().m_wclass_enabled = 0;
}

#if 0
Errwarns::ErrwarnData:~ErrwarnData()
{
    delete[] m_msg;
    delete[] m_xrefmsg;
}

void
yasm_errwarn_propagate(yasm_errwarns *errwarns, unsigned long line)
{
    if (yasm_eclass != ERROR_NONE) {
        errwarn_data *we = errwarn_data_new(errwarns, line, 1);
        yasm_error_class eclass;

        yasm_error_fetch(&eclass, &we->msg, &we->xrefline, &we->xrefmsg);
        if (eclass != ERROR_GENERAL
            && (eclass & ERROR_PARSE) == ERROR_PARSE)
            we->type = WE_PARSERERROR;
        else
            we->type = WE_ERROR;
        errwarns->ecount++;
    }

    while (!STAILQ_EMPTY(&yasm_warns)) {
        errwarn_data *we = errwarn_data_new(errwarns, line, 0);
        yasm_warn_class wclass;

        yasm_warn_fetch(&wclass, &we->msg);
        we->type = WE_WARNING;
        errwarns->wcount++;
    }
}

unsigned int
yasm_errwarns_num_errors(yasm_errwarns *errwarns, int warning_as_error)
{
    if (warning_as_error)
        return errwarns->ecount+errwarns->wcount;
    else
        return errwarns->ecount;
}

void
yasm_errwarns_output_all(yasm_errwarns *errwarns, yasm_linemap *lm,
                         int warning_as_error,
                         yasm_print_error_func print_error,
                         yasm_print_warning_func print_warning)
{
    errwarn_data *we;
    const char *filename, *xref_filename;
    unsigned long line, xref_line;

    // If we're treating warnings as errors, tell the user about it.
    if (warning_as_error && warning_as_error != 2) {
        print_error("", 0,
                    yasm_gettext_hook(N_("warnings being treated as errors")),
                    NULL, 0, NULL);
        warning_as_error = 2;
    }

    // Output error/warnings.
    SLIST_FOREACH(we, &errwarns->errwarns, link) {
        // Output error/warning
        yasm_linemap_lookup(lm, we->line, &filename, &line);
        if (we->xrefline)
            yasm_linemap_lookup(lm, we->xrefline, &xref_filename, &xref_line);
        else {
            xref_filename = NULL;
            xref_line = 0;
        }
        if (we->type == WE_ERROR || we->type == WE_PARSERERROR)
            print_error(filename, line, we->msg, xref_filename, xref_line,
                        we->xrefmsg);
        else
            print_warning(filename, line, we->msg);
    }
}
#endif
} // namespace yasm
