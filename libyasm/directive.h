#ifndef YASM_VALPARAM_H
#define YASM_VALPARAM_H
///
/// @file libyasm/valparam.h
/// @brief YASM value/parameter interface.
///
/// @license
///  Copyright (C) 2001-2007  Peter Johnson
///
/// Redistribution and use in source and binary forms, with or without
/// modification, are permitted provided that the following conditions
/// are met:
///  - Redistributions of source code must retain the above copyright
///    notice, this list of conditions and the following disclaimer.
///  - Redistributions in binary form must reproduce the above copyright
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
/// @endlicense
///
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>


namespace yasm {

class Expr;
class Object;

/// Name/value pair.
class NameValue : private boost::noncopyable {
public:
    /// Identifier value constructor.
    /// @param name         name; may be empty string if no name
    /// @param id           identifier value
    /// @param id_prefix    identifier prefix for raw identifiers
    NameValue(const std::string& name, const std::string& id, char id_prefix);

    /// String value constructor.
    /// @param name         name; may be empty string if no name
    /// @param str          string value
    NameValue(const std::string& name, const std::string& str);

    /// Expression value constructor.
    /// @param name         name; may be empty string if no name
    /// @param e            expression
    NameValue(const std::string& name, std::auto_ptr<Expr> e);

    /// Identifier value constructor with no name.
    /// @param id           identifier value
    /// @param id_prefix    identifier prefix for raw identifiers
    NameValue(const std::string& id, char id_prefix);

    /// String value constructor with no name.
    /// @param name         name; may be empty string if no name
    /// @param str          string value
    NameValue(const std::string& str);

    /// Expression value constructor with no name.
    /// @param e            expression
    NameValue(std::auto_ptr<Expr> e);

    /// Destructor.
    ~NameValue();

    /// Get name.
    /// @return Name; empty string if no name.
    std::string get_name() const { return m_name; }

    /// Determine if value is convertible to an expression using get_expr().
    /// @return True if convertible.
    bool is_expr() const { return (m_type == ID) || (m_type == EXPR); }

    /// Determine if value is convertible to a string using get_string().
    /// @return True if convertible.
    bool is_string() const { return (m_type == ID) || (m_type == STRING); }

    /// Determine if value is convertible to an identifier using get_id().
    /// @return True if convertible.
    bool is_id() const { return m_type == ID; }

    /// Get value as an expr.  If the parameter is an identifier,
    /// it's treated as a symbol (Symbol::use() is called to convert it).
    /// @param object       object
    /// @param line         virtual line
    /// @return Expression, or NULL if the parameter cannot be
    ///         converted to an expression.
    /*@null@*/ std::auto_ptr<Expr> get_expr
        (Object& object, unsigned long line) const;

    /// Get value as a string.  If the parameter is an identifier,
    /// it's treated as a string.
    /// @return String; raises an exception if the parameter cannot be
    ///         realized as a string.
    std::string get_string() const;

    /// Get value as an identifier.
    /// @return Identifier (string); raises an exception if the parameter
    ///         is not an identifier.
    std::string get_id() const;

private:
    std::string m_name; ///< Name (empty string if no name)

    /// Value type.
    enum Type {
        ID,             ///< Identifier
        STRING,         ///< String
        EXPR            ///< Expression
    };

    Type m_type;

    /// Possible values
    std::string m_idstr;            ///< Identifier or string
    boost::scoped_ptr<Expr> m_expr; ///< Expression

    /// Prefix character that indicates a raw identifier.  When
    /// get_string() is called on a #ID, all characters are
    /// returned.  When get_id() is called on a #ID, if the
    /// identifier begins with this character, this character is stripped
    /// from the returned value.
    char m_id_prefix;
};

/// Vector of name/values.
typedef std::vector<NameValue> NameValues;

/// Print vector of name/values.  For debugging purposes.
/// @param os       output stream
/// @param namevals name/values
std::ostream& operator<< (std::ostream& os, const NameValues& namevals);

/// Directive functor.
class Directive {
public:
    enum Flags {
        ANY = 0,            ///< Any valparams accepted
        ARG_REQUIRED = 1,   ///< Require at least 1 valparam
        ID_REQUIRED = 2     ///< First valparam must be ID
    };

    /// Constructor.
    /// @param flags        Flags for pre-handler parameter checking.
    Directive(std::string name, Flags flags)
        : m_name(name), m_flags(flags)
    {}
    virtual ~Directive() {}

    /// Call a directive.  Performs any valparam checks asked for by the
    /// directive prior to call.  Note that for a variety of reasons, a
    /// directive can raise an exception.
    /// @param object           object 
    /// @param namevals         name/values
    /// @param objext_namevals  object format-specific name/values
    /// @param line             virtual line (from Linemap)
    void operator() (Object& object,
                     const NameValues& namevals,
                     const NameValues& objext_namevals,
                     unsigned long line);

protected:
    /// Handler function for the directive.
    /// @param object           object 
    /// @param namevals         name/values
    /// @param objext_namevals  object format-specific name/values
    /// @param line             virtual line (from Linemap)
    virtual void handler(Object& object,
                         const NameValues& namevals,
                         const NameValues& objext_namevals,
                         unsigned long line) = 0;

private:
    /// Directive name.
    std::string m_name;

    /// Flags for pre-handler parameter checking.
    Flags m_flags;
};

class DirectiveManager {
public:
    DirectiveManager();
    ~DirectiveManager();

    /// Add a directive.
    /// @param name         Directive name.  GAS directives should include
    ///                     the ".", NASM directives should just be the raw
    ///                     name (not including the []).
    /// @param parser       Parser keyword
    /// @param directive    New'ed instance of Directive functor.
    /// @param flags        Flags for pre-handler parameter checking.
    void add(const std::string& name,
             const std::string& parser,
             Directive* directive,
             Directive::Flags flags = Directive::ANY);

    /// Get a directive functor.  Throws an exception if no match.
    /// @param name         directive name
    /// @param parser       parser keyword
    Directive& get(const std::string& name,
                   const std::string& parser) const;

private:
    /// Pimpl for class internals.
    class Impl;
    boost::scoped_ptr<Impl> m_impl;
};
#if 0
/// Directive valparam parse helper structure.
struct DirHelp {
    /// Name portion of name=value (if needsparam=true), or standalone
    /// identifier (if needsparam=false).
    const char *name;

    /// True if value requires parameter, false if it must not have a
    /// parameter.
    bool needsparam;

    /// Helper callback function if name and parameter existence match.
    /// @param obj      obj passed into yasm_dir_helper()
    /// @param nv       name/value
    /// @param line     line passed into yasm_dir_helper()
    /// @param data     data passed into yasm_dir_helper() plus
    ///                 #yasm_dir_help.off offset
    /// @param arg      #yasm_dir_help.arg argument
    /// @return -1 on error, 0 otherwise.
    int (*helper) (void *obj, yasm_valparam *vp, unsigned long line,
                   void *data, uintptr_t arg);

    /// Offset added to data pointer passed into yasm_dir_helper() before
    /// data pointer is given to #yasm_dir_help.helper().  This is so that
    /// a structure can be passed into yasm_dir_helper() and this can be an
    /// offsetof() to point the helper function to a specific structure
    /// member.
    size_t off;

    /// Argument to pass in as the arg parameter to #yasm_dir_help.helper().
    uintptr_t arg;
};

/** Help parse a list of directive value/parameters.  Takes an array of
 * #yasm_dir_help structures and tries to match val=param (or just val)
 * against the passed value/parameters.  When no match is found in the
 * array of help structures, calls helper_valparam.
 * \param obj       object to be passed to yasm_dir_help.helper() or
 *                  helper_valparam() callback
 * \param vp_first  first value/parameter to examine
 * \param line      virtual line number; passed down to helper callback
 * \param help      array of #yasm_dir_help structures
 * \param nhelp     number of array elements
 * \param data      base data pointer; if a match is found,
 *                  the respective #yasm_dir_help.off is added to this
 *                  prior to it being passed to the helper callback
 * \param helper_valparam   catch-all callback; should return -1 on error,
 *                          0 if not matched, 1 if matched.
 * \return -1 on error, 1 if any arguments matched (including via
 *         catch-all callback), 0 if no match.
 */
int yasm_dir_helper(void *obj, yasm_valparam *vp_first, unsigned long line,
                    const yasm_dir_help *help, size_t nhelp, void *data,
                    int (*helper_valparam) (void *object,
                                            yasm_valparam *vp,
                                            unsigned long line,
                                            void *data));

/** Standard helper for yasm_dir_helper() that simply sets a flag when called.
 * It does not look at the vp; rather, it uses the value of the arg parameter,
 * and stores an unsigned long value to data.
 * \param obj   unused
 * \param vp    unused
 * \param line  unused
 * \param data  pointer to an unsigned long
 * \param arg   flag to set
 * \return 0
 */
int yasm_dir_helper_flag_set(void *obj, yasm_valparam *vp, unsigned long line,
                             void *data, uintptr_t arg);

/** Standard helper for yasm_dir_helper() that simply ORs a flag when called.
 * It does not look at the vp; rather, it uses the value of the arg parameter,
 * and ORs it with the unsigned long value in data.
 * \param obj   unused
 * \param vp    unused
 * \param line  unused
 * \param data  pointer to an unsigned long
 * \param arg   flag to OR
 * \return 0
 */
int yasm_dir_helper_flag_or(void *obj, yasm_valparam *vp, unsigned long line,
                            void *data, uintptr_t arg);

/** Standard helper for yasm_dir_helper() that simply ANDs a flag when called.
 * It does not look at the vp; rather, it uses the value of the arg parameter,
 * and ANDs its inverse (~) with the unsigned long value in data.
 * \param obj   unused
 * \param vp    unused
 * \param line  unused
 * \param data  pointer to an unsigned long
 * \param arg   flag to AND
 * \return 0
 */
int yasm_dir_helper_flag_and(void *obj, yasm_valparam *vp, unsigned long line,
                             void *data, uintptr_t arg);

/** Standard helper for yasm_dir_helper() that parses an intnum parameter.
 * The #yasm_dir_help structure that uses this function should have
 * needsparam=1.  The obj parameter to yasm_dir_helper() when this helper
 * is used MUST point to a #yasm_object.  In addition, the data parameter
 * that is ultimately passed to this function (e.g. yasm_dir_helper() data
 * parameter plus #yasm_dir_help.off) must point to a #yasm_intnum *
 * initialized to NULL.
 * \param obj   object; must be #yasm_object
 * \param vp    valparam
 * \param line  virtual line number
 * \param data  pointer to #yasm_intnum *
 * \param arg   unused argument
 * \return -1 on error, 0 otherwise.
 */
int yasm_dir_helper_intn(void *obj, yasm_valparam *vp, unsigned long line,
                         void *data, uintptr_t arg);

/** Standard helper for yasm_dir_helper() that parses an string (or
 * standalone identifier) parameter.
 * The #yasm_dir_help structure that uses this function should have
 * needsparam=1.  The data parameter that is ultimately passed to this
 * function (e.g. yasm_dir_helper() data parameter plus #yasm_dir_help.off)
 * must point to a char * initialized to NULL.
 * \param obj   unused
 * \param vp    valparam
 * \param line  unused
 * \param data  pointer to char *
 * \param arg   unused
 * \return -1 on error, 0 otherwise.
 */
int yasm_dir_helper_string(void *obj, yasm_valparam *vp, unsigned long line,
                           void *data, uintptr_t arg);

/** Standard catch-all callback fro yasm_dir_helper().  Generates standard
 * warning for all valparams.
 * \param obj   unused
 * \param vp    valparam
 * \param line  unused
 * \param data  unused
 * \return 0
 */
int yasm_dir_helper_valparam_warn(void *obj, yasm_valparam *vp,
                                  unsigned long line, void *data);
#endif
} // namespace yasm

#endif
