///
/// Value/Parameter type functions
///
///  Copyright (C) 2001-2007  Peter Johnson
///
/// Redistribution and use in source and binary forms, with or without
/// modification, are permitted provided that the following conditions
/// are met:
/// 1. Redistributions of source code must retain the above copyright
///    notice, this list of conditions and the following disclaimer.
/// 2. Redistributions in binary form must reproduce the above copyright
///    notice, this list of conditions and the following disclaimer in the
///    documentation and/or other materials provided with the distribution.
///
/// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND OTHER CONTRIBUTORS ``AS IS''
/// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
/// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
/// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR OTHER CONTRIBUTORS BE
/// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
/// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
/// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
/// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
/// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
/// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
/// POSSIBILITY OF SUCH DAMAGE.
///
#include "util.h"

#include <boost/format.hpp>
#include <boost/ptr_container/ptr_map.hpp>

#include "directive.h"
#include "errwarn.h"
#include "expr.h"
#include "object.h"
#include "symbol.h"


namespace yasm {

class DirectiveManager::Impl {
public:
    Impl() {};
    ~Impl() {};

    boost::ptr_map<std::string, boost::ptr_map<std::string, Directive> > m_dirs;
};

DirectiveManager::DirectiveManager()
    : m_impl(new Impl)
{
}

DirectiveManager::~DirectiveManager()
{
}

void
DirectiveManager::add(const std::string& name, const std::string& parser,
                      Directive* directive, Directive::Flags flags)
{
    m_impl->m_dirs[parser].insert(name, std::auto_ptr<Directive>(directive));
}

Directive&
DirectiveManager::get(const std::string& name,
                      const std::string& parser) const
{
    return m_impl->m_dirs.at(parser).at(name);
}

void
Directive::operator() (Object& object,
                       const NameValues& namevals,
                       const NameValues& objext_namevals,
                       unsigned long line)
{
    if ((m_flags & (ARG_REQUIRED|ID_REQUIRED)) && namevals.empty())
        throw SyntaxError(str(boost::format(
            N_("directive `%s' requires an argument")) % m_name));

    if (!namevals.empty()) {
        if ((m_flags & ID_REQUIRED) && !namevals.front().is_id())
            throw SyntaxError(str(boost::format(
                N_("directive `%s' requires an identifier parameter")) %
                m_name));
    }

    handler(object, namevals, objext_namevals, line);
}

NameValue::NameValue(const std::string& name, const std::string& id,
                     char id_prefix)
    : m_name(name),
      m_type(ID),
      m_idstr(id),
      m_expr(0),
      m_id_prefix(id_prefix)
{
}

NameValue::NameValue(const std::string& name, const std::string& str)
    : m_name(name),
      m_type(STRING),
      m_idstr(str),
      m_expr(0),
      m_id_prefix('\0')
{
}

NameValue::NameValue(const std::string& name, std::auto_ptr<Expr> e)
    : m_name(name),
      m_type(EXPR),
      m_expr(e.release()),
      m_id_prefix('\0')
{
}

NameValue::NameValue(const std::string& id, char id_prefix)
    : m_name(""),
      m_type(ID),
      m_idstr(id),
      m_expr(0),
      m_id_prefix(id_prefix)
{
}

NameValue::NameValue(const std::string& str)
    : m_name(""),
      m_type(STRING),
      m_idstr(str),
      m_expr(0),
      m_id_prefix('\0')
{
}

NameValue::NameValue(std::auto_ptr<Expr> e)
    : m_name(""),
      m_type(EXPR),
      m_expr(e.release()),
      m_id_prefix('\0')
{
}

NameValue::~NameValue()
{
}

/*@null@*/ std::auto_ptr<Expr>
NameValue::get_expr(Object& object, unsigned long line) const
{
    switch (m_type) {
        case ID:
        {
            Symbol* sym = object.get_sym(get_id());
            sym->use(line);
            return std::auto_ptr<Expr>(new Expr(sym, line));
        }
        case EXPR:
            return std::auto_ptr<Expr>(m_expr->clone());
        default:
            return std::auto_ptr<Expr>(0);
    }
}

std::string
NameValue::get_string() const
{
    switch (m_type) {
        case ID:
        case STRING:
            return m_idstr;
        default:
            throw Error(N_("name/value not convertible to string"));
    }
}

std::string
NameValue::get_id() const
{
    if (m_type != ID)
        throw Error(N_("name/value not convertible to identifier"));

    if (m_idstr[0] == m_id_prefix)
        return m_idstr.substr(1);
    else
        return m_idstr;
}
#if 0
int
yasm_dir_helper(void *obj, yasm_valparam *vp_first, unsigned long line,
                const yasm_dir_help *help, size_t nhelp, void *data,
                int (*helper_valparam) (void *obj, yasm_valparam *vp,
                                        unsigned long line, void *data))
{
    yasm_valparam *vp = vp_first;
    int anymatched = 0;
    int matched;

    if (!vp)
        return 0;

    do {
        const char *s;
        size_t i;

        matched = 0;
        if (!vp->val && (s = yasm_vp_id(vp))) {
            for (i=0; i<nhelp; i++) {
                if (help[i].needsparam == 0 &&
                    yasm__strcasecmp(s, help[i].name) == 0) {
                    if (help[i].helper(obj, vp, line,
                                       ((char *)data)+help[i].off,
                                       help[i].arg) != 0)
                        return -1;
                    matched = 1;
                    anymatched = 1;
                    break;
                }
            }
        } else if (vp->val) {
            for (i=0; i<nhelp; i++) {
                if (help[i].needsparam == 1 &&
                    yasm__strcasecmp(vp->val, help[i].name) == 0) {
                    if (help[i].helper(obj, vp, line,
                                       ((char *)data)+help[i].off,
                                       help[i].arg) != 0)
                        return -1;
                    matched = 1;
                    anymatched = 1;
                    break;
                }
            }
        }

        if (!matched) {
            int final = helper_valparam(obj, vp, line, data);
            if (final < 0)
                return -1;
            if (final > 0)
                anymatched = 1;
        }
    } while((vp = yasm_vps_next(vp)));

    return anymatched;
}

int
yasm_dir_helper_flag_or(void *obj, yasm_valparam *vp, unsigned long line,
                        void *d, uintptr_t flag)
{
    unsigned long *flags = (unsigned long *)d;
    *flags |= flag;
    return 0;
}

int
yasm_dir_helper_flag_and(void *obj, yasm_valparam *vp, unsigned long line,
                         void *d, uintptr_t flag)
{
    unsigned long *flags = (unsigned long *)d;
    *flags &= ~flag;
    return 0;
}

int
yasm_dir_helper_flag_set(void *obj, yasm_valparam *vp, unsigned long line,
                         void *d, uintptr_t flag)
{
    unsigned long *flags = (unsigned long *)d;
    *flags = flag;
    return 0;
}

int
yasm_dir_helper_intn(void *obj, yasm_valparam *vp, unsigned long line,
                     void *data, uintptr_t arg)
{
    yasm_object *object = (yasm_object *)obj;
    /*@only@*/ /*@null@*/ yasm_expr *e;
    /*@dependent@*/ /*@null@*/ yasm_intnum *local;
    yasm_intnum **intn = (yasm_intnum **)data;

    if (*intn)
        yasm_intnum_destroy(*intn);
    if (!(e = yasm_vp_expr(vp, object->symtab, line)) ||
        !(local = yasm_expr_get_intnum(&e, 0))) {
        yasm_error_set(YASM_ERROR_NOT_CONSTANT,
                       N_("argument to `%s' is not an integer"),
                       vp->val);
        if (e)
            yasm_expr_destroy(e);
        return -1;
    }
    *intn = yasm_intnum_copy(local);
    yasm_expr_destroy(e);
    return 0;
}

int
yasm_dir_helper_string(void *obj, yasm_valparam *vp, unsigned long line,
                       void *data, uintptr_t arg)
{
    /*@dependent@*/ /*@null@*/ const char *local;
    char **s = (char **)data;

    if (*s)
        yasm_xfree(*s);
    if (!(local = yasm_vp_string(vp))) {
        yasm_error_set(YASM_ERROR_VALUE,
                       N_("argument to `%s' is not a string or identifier"),
                       vp->val);
        return -1;
    }
    *s = yasm__xstrdup(local);
    return 0;
}

int
yasm_dir_helper_valparam_warn(void *obj, yasm_valparam *vp,
                              unsigned long line, void *data)
{
    const char *s;

    if (vp->val) {
        yasm_warn_set(YASM_WARN_GENERAL, N_("Unrecognized qualifier `%s'"),
                      vp->val);
        return 0;
    }

    if ((s = yasm_vp_id(vp)))
        yasm_warn_set(YASM_WARN_GENERAL, N_("Unrecognized qualifier `%s'"), s);
    else if (vp->type == YASM_PARAM_STRING)
        yasm_warn_set(YASM_WARN_GENERAL, N_("Unrecognized string qualifier"));
    else
        yasm_warn_set(YASM_WARN_GENERAL, N_("Unrecognized numeric qualifier"));

    return 0;
}
#endif
} // namespace yasm
