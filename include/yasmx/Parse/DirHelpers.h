#ifndef YASM_DIRHELPERS_H
#define YASM_DIRHELPERS_H
///
/// @file
/// @brief Directive helpers.
///
/// @license
///  Copyright (C) 2001-2008  Peter Johnson
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
#include <memory>
#include <string>

#include "yasmx/Basic/LLVM.h"
#include "yasmx/Config/export.h"
#include "yasmx/Config/functional.h"
#include "yasmx/Parse/NameValue.h"
#include "yasmx/Support/scoped_ptr.h"


namespace yasm
{

class DiagnosticsEngine;
class Expr;
class IntNum;
class Object;
class SourceLocation;

/// Directive helper function.
/// @param nv       name/value
/// @param diags    diagnostic reporting
typedef TR1::function<void (NameValue& nv,
                            DiagnosticsEngine& diags)> DirHelper;

/// Helper class to make writing directive handlers easier.
class YASM_LIB_EXPORT DirHelpers
{
public:
    DirHelpers();
    ~DirHelpers();

    /// Add a directive helper.
    /// @param name         Name portion of name=value (if needsvalue=true),
    ///                     or standalone identifier (if needsvalue=false)
    /// @param needsvalue   True if name requires value, false if it must not
    ///                     have a value.
    /// @param helper       Helper function
    void Add(StringRef name, bool needsvalue, DirHelper helper);

    /// Help parse a list of directive name/values.  Matches name=value
    /// (or just value) against each of the added helper functions.
    /// When no match is found, calls helper_nameval.
    /// @param nv_begin     iterator to first value/parameter to examine
    /// @param nv_end       iterator to last value/parameter to examine
    /// @param dir_source   source location of directive name
    /// @param diags        diagnostic reporting
    /// @param helper_nameval   catch-all callback; should return
    ///                         false if not matched, true if matched.
    /// @return True if any arguments matched (including via
    ///         catch-all callback), false if no match.
    bool operator()
        (NameValues::iterator nv_first,
         NameValues::iterator nv_last,
         SourceLocation dir_source,
         DiagnosticsEngine& diags,
         TR1::function<bool (NameValue& nv,
                             SourceLocation dir_source,
                             DiagnosticsEngine& diags)>
             helper_nameval);

private:
    /// Pimpl for class internals.
    class Impl;
    util::scoped_ptr<Impl> m_impl;
};

/// Standard helper for DirHelpers() that simply resets a flag when
/// called.  It does not look at the nv; rather, it uses the value of the
/// val parameter, and stores it to out.
/// @param nv       unused
/// @param diags    diagnostic reporting
/// @param out      reference to unsigned long
/// @param val      value to set
inline void
DirResetFlag(NameValue& nv,
             DiagnosticsEngine& diags,
             unsigned long* out,
             unsigned long val)
{
    *out = val;
}

/// Standard helper for DirHelpers() that simply sets a flag when
/// called.  It does not look at the nv; rather, it uses the value of the
/// flag parameter, and ORs it with the unsigned long value in out.
/// @param nv       unused
/// @param diags    diagnostic reporting
/// @param out      reference to unsigned long
/// @param flag     flag bit(s) to set
inline void
DirSetFlag(NameValue& nv,
           DiagnosticsEngine& diags,
           unsigned long* out,
           unsigned long flag)
{
    *out |= flag;
}

/// Standard helper for DirHelpers() that simply ANDs a flag when
/// called.  It does not look at the nv; rather, it uses the value of the
/// flag parameter, and ANDs its bitwise inverse with the unsigned long
/// value in out.
/// @param nv       unused
/// @param diags    diagnostic reporting
/// @param out      reference to unsigned long
/// @param flag     flag bit(s) to clear
inline void
DirClearFlag(NameValue& nv,
             DiagnosticsEngine& diags,
             unsigned long* out,
             unsigned long flag)
{
    *out &= ~flag;
}

/// Standard helper for DirHelpers() that parses a power-of-2 IntNum value.
/// Only powers of 2 are accepted; others generate an error.
/// When calling DirHelpers::add(), needsvalue should be set to true.
/// @param nv       name/value
/// @param diags    diagnostic reporting
/// @param obj      object
/// @param out      reference to IntNum
/// @param out_set  reference that is set to 1 when called
YASM_LIB_EXPORT
void DirIntNumPower2(NameValue& nv,
                     DiagnosticsEngine& diags,
                     Object* obj,
                     IntNum* out,
                     bool* out_set);

/// Standard helper for DirHelpers() that parses an IntNum value.
/// When calling DirHelpers::add(), needsvalue should be set to true.
/// @param nv       name/value
/// @param diags    diagnostic reporting
/// @param obj      object
/// @param out      reference to IntNum
/// @param out_set  reference that is set to 1 when called
YASM_LIB_EXPORT
void DirIntNum(NameValue& nv,
               DiagnosticsEngine& diags,
               Object* obj,
               IntNum* out,
               bool* out_set);

/// Standard helper for DirHelpers() that parses an IntNum value.
/// When calling DirHelpers::add(), needsvalue should be set to true.
/// @param nv       name/value
/// @param diags    diagnostic reporting
/// @param obj      object
/// @param out      reference to Expr auto_ptr
/// @param out_set  reference that is set to 1 when called
YASM_LIB_EXPORT
void DirExpr(NameValue& nv,
             DiagnosticsEngine& diags,
             Object* obj,
             std::auto_ptr<Expr>* out,
             bool* out_set);

/// Standard helper for DirHelpers() that parses a register value.
/// When calling DirHelpers::add(), needsvalue should be set to true.
/// @param nv       name/value
/// @param diags    diagnostic reporting
/// @param out      reference to Register*
/// @param out_set  reference that is set to 1 when called
typedef const Register* RegisterPtr;
YASM_LIB_EXPORT
void DirRegister(NameValue& nv,
                 DiagnosticsEngine& diags,
                 RegisterPtr* out,
                 bool* out_set);

/// Standard helper for DirHelpers() that parses an string (or
/// standalone identifier) value.
/// When calling DirHelpers::add(), needsvalue should be set to true.
/// @param nv       name/value
/// @param diags    diagnostic reporting
/// @param out      reference to string
/// @param out_set  reference that is set to 1 when called
YASM_LIB_EXPORT
void DirString(NameValue& nv,
               DiagnosticsEngine& diags,
               std::string* out,
               bool* out_set);

/// Standard catch-all callback for DirHelpers().  Generates standard
/// warning for all namevals.
/// @param nv           name/value
/// @param dir_source   source location of directive name
/// @param diags        diagnostic reporting
/// @return False
YASM_LIB_EXPORT
bool DirNameValueWarn(NameValue& nv,
                      SourceLocation dir_source,
                      DiagnosticsEngine& diags);

} // namespace yasm

#endif
