#ifndef YASM_DIR_HELP_H
#define YASM_DIR_HELP_H
///
/// @file libyasm/dir_help.h
/// @brief YASM directive helper interface
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
#include <string>

#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>

#include "directive.h"


namespace yasm {

class IntNum;
class Object;

class DirHelperManager {
public:
    DirHelperManager();
    ~DirHelperManager();

    /// Add a directive helper.
    /// @param name         Name portion of name=value (if needsvalue=true),
    ///                     or standalone identifier (if needsvalue=false)
    /// @param needsvalue   True if name requires value, false if it must not
    ///                     have a value.
    /// @param helper       Helper function
    void add(const std::string& name,
             bool needsvalue,
             boost::function<void (const NameValue&)> helper);

    /// Help parse a list of directive name/values.  Matches name=value
    /// (or just value) against each of the added helper functions.
    /// When no match is found, calls helper_nameval.
    /// @param nv_begin iterator to first value/parameter to examine
    /// @param nv_end   iterator to last value/parameter to examine
    /// @param helper_nameval   catch-all callback; should return
    ///                         false if not matched, true if matched.
    /// @return True if any arguments matched (including via
    ///         catch-all callback), false if no match.
    bool operator() (NameValues::const_iterator nv_first,
                     NameValues::const_iterator nv_last,
                     boost::function<bool (const NameValue&)> helper_nameval);

private:
    /// Pimpl for class internals.
    class Impl;
    boost::scoped_ptr<Impl> m_impl;
};

/// Standard helper for DirHelperManager() that simply resets a flag when
/// called.  It does not look at the nv; rather, it uses the value of the
/// val parameter, and stores it to out.
/// @param nv       unused
/// @param out      reference to unsigned long
/// @param val      value to set
inline void dir_flag_reset(const NameValue& nv, unsigned long& out,
                           unsigned long val)
{
    out = val;
}

/// Standard helper for DirHelperManager() that simply sets a flag when
/// called.  It does not look at the nv; rather, it uses the value of the
/// flag parameter, and ORs it with the unsigned long value in out.
/// @param nv       unused
/// @param out      reference to unsigned long
/// @param flag     flag bit(s) to set
inline void dir_flag_set(const NameValue& nv, unsigned long& out,
                         unsigned long flag)
{
    out |= flag;
}

/// Standard helper for DirHelperManager() that simply ANDs a flag when
/// called.  It does not look at the nv; rather, it uses the value of the
/// flag parameter, and ANDs its bitwise inverse with the unsigned long
/// value in out.
/// @param nv       unused
/// @param out      reference to unsigned long
/// @param flag     flag bit(s) to clear
inline void dir_flag_clear(const NameValue& nv, unsigned long& out,
                           unsigned long flag)
{
    out &= ~flag;
}

/// Standard helper for DirHelperManager() that parses an IntNum value.
/// When calling DirHelperManager::add(), needsparam should be set to true.
/// @param nv       name/value
/// @param obj      object
/// @param line     virtual line number
/// @param out      reference to IntNum
/// @param out_set  reference that is set to 1 when called
void dir_intn(const NameValue& nv, Object& obj, unsigned long line,
              IntNum& out, bool& out_set);

/// Standard helper for DirHelperManager() that parses an string (or
/// standalone identifier) value.
/// When calling DirHelperManager::add(), needsparam should be set to true.
/// @param nv       name/value
/// @param out      reference to string
/// @param out_set  reference that is set to 1 when called
void dir_string(const NameValue& nv, std::string& out, bool& out_set);

/// Standard catch-all callback for DirHelperManager().  Generates standard
/// warning for all valparams.
/// @param nv       name/value
/// @return False
bool dir_nameval_warn(const NameValue& nv);

} // namespace yasm

#endif
