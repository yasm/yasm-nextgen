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
#include "yasmx/Support/errwarn.h"

#include "util.h"

#include <list>


namespace yasm
{

// Default handlers for replacable functions
static const char *def_gettext_hook(const char *msgid);

// Storage for errwarn's "extern" functions
const char * (*gettext_hook) (const char *msgid) = def_gettext_hook;

class ErrwarnManager
{
public:
    static ErrwarnManager & Instance()
    {
        static ErrwarnManager inst;
        return inst;
    }

    /// Warning indicator.
    class Warning
    {
    public:
        Warning(unsigned long line, WarnClass wclass, const std::string& wmsg);

        unsigned long m_line;
        WarnClass m_wclass;
        std::string m_wmsg;
    };

    typedef std::list<Warning> Warnings;
    Warnings m_warns;

    /// Enabled warnings.  See errwarn.h for a list.
    unsigned long m_wclass_enabled;

private:
    ErrwarnManager();
    ErrwarnManager(const ErrwarnManager &) {}
    ErrwarnManager & operator= (const ErrwarnManager &) { return *this; }
    ~ErrwarnManager();
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
ConvUnprint(int ch)
{
    std::string unprint;

    if (((ch & ~0x7F) != 0) /*!isascii(ch)*/ && !isprint(ch))
    {
        unprint += 'M';
        unprint += '-';
        ch &= 0x7F; /*toascii(ch)*/
    }

    if (iscntrl(ch))
    {
        unprint += '^';
        unprint += (ch == '\177') ? '?' : ch | 0100;
    }
    else
        unprint += ch;

    return unprint;
}

NotImplementedError::NotImplementedError(const std::string& message)
    : std::runtime_error(message.c_str())
{
}

NotImplementedError::~NotImplementedError() throw()
{
}

Fatal::Fatal(const std::string& message)
    : m_message(message)
{
}

Fatal::~Fatal() throw()
{
}

const char*
Fatal::what() const throw()
{
    return m_message.c_str();
}

Error::Error(const std::string& message)
    : m_line(0),
      m_message(message),
      m_xrefline(0),
      m_parse_error(false)
{
}

Error::Error(unsigned long line, const std::string& message)
    : m_line(line),
      m_message(message),
      m_xrefline(0),
      m_parse_error(false)
{
}

Error::~Error() throw()
{
}

void
Error::setXRef(unsigned long xrefline, const std::string& message)
{
    m_xrefline = xrefline;
    m_xrefmsg = message;
}

const char*
Error::what() const throw()
{
    return m_message.c_str();
}

ArithmeticError::~ArithmeticError() throw()
{
}

OverflowError::~OverflowError() throw()
{
}

FloatingPointError::~FloatingPointError() throw()
{
}

ZeroDivisionError::~ZeroDivisionError() throw()
{
}

AssertionError::~AssertionError() throw()
{
}

ValueError::~ValueError() throw()
{
}

NotAbsoluteError::~NotAbsoluteError() throw()
{
}

TooComplexError::~TooComplexError() throw()
{
}

NotConstantError::~NotConstantError() throw()
{
}

IOError::~IOError() throw()
{
}

TypeError::~TypeError() throw()
{
}

SyntaxError::~SyntaxError() throw()
{
}

ParseError::~ParseError() throw()
{
}

void
ClearWarn()
{
    ErrwarnManager::Instance().m_warns.clear();
}

WarnClass
WarnOccurred()
{
    ErrwarnManager &manager = ErrwarnManager::Instance();
    if (manager.m_warns.empty())
        return WARN_NONE;
    return manager.m_warns.front().m_wclass;
}

ErrwarnManager::Warning::Warning(unsigned long line,
                                 WarnClass wclass,
                                 const std::string& msg)
    : m_line(line),
      m_wclass(wclass),
      m_wmsg(msg)
{
}

void
setWarn(unsigned long line, WarnClass wclass, const std::string& str)
{
    ErrwarnManager &manager = ErrwarnManager::Instance();

    if (!(manager.m_wclass_enabled & (1UL<<wclass)))
        return;     // warning is part of disabled class

    manager.m_warns.push_back(ErrwarnManager::Warning(line, wclass, str));
}

void
WarnUpdateLine(unsigned long line)
{
    ErrwarnManager &manager = ErrwarnManager::Instance();

    for (ErrwarnManager::Warnings::iterator i = manager.m_warns.begin(),
         end = manager.m_warns.end(); i != end; ++i)
    {
        if (i->m_line == 0)
            i->m_line = line;
    }
}

WarnClass
FetchWarn(std::string* wmsg, unsigned long* wline)
{
    ErrwarnManager& manager = ErrwarnManager::Instance();

    if (manager.m_warns.empty())
        return WARN_NONE;

    ErrwarnManager::Warning& w = manager.m_warns.front();

    WarnClass wclass = w.m_wclass;
    if (wmsg)
        *wmsg = w.m_wmsg;
    if (wline)
        *wline = w.m_line;

    manager.m_warns.pop_front();
    return wclass;
}

void
EnableWarn(WarnClass num)
{
    ErrwarnManager::Instance().m_wclass_enabled |= (1UL<<num);
}

void
DisableWarn(WarnClass num)
{
    ErrwarnManager::Instance().m_wclass_enabled &= ~(1UL<<num);
}

void
DisableAllWarn()
{
    ErrwarnManager::Instance().m_wclass_enabled = 0;
}

} // namespace yasm
