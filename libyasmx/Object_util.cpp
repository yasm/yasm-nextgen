///
/// @file
/// @brief Object utility functions.
///
/// @license
///  Copyright (C) 2008  Peter Johnson
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
#include "Object_util.h"

#include "util.h"

#include "BytecodeContainer_util.h"
#include "Bytecode.h"
#include "errwarn.h"
#include "NameValue.h"
#include "Section.h"
#include "Object.h"
#include "ObjectFormat.h"

namespace yasm
{

void
dir_ident_common(ObjectFormat& objfmt,
                 const std::string& sectname,
                 Object& object,
                 NameValues& namevals,
                 NameValues& objext_namevals,
                 unsigned long line)
{
    // Accept, but do nothing with empty ident
    if (namevals.empty())
        return;

    // Put ident data into .comment section
    Section* comment = object.find_section(sectname);
    if (!comment)
        comment = objfmt.append_section(sectname, line);

    // To match GAS output, if the comment section is empty, put an
    // initial 0 byte in the section.
    Bytecode& bc = comment->fresh_bytecode();
    if (comment->bcs_begin() == comment->bcs_end() && bc.get_fixed_len() == 0)
        append_byte(*comment, 0);

    for (NameValues::const_iterator nv=namevals.begin(), end=namevals.end();
         nv != end; ++nv)
    {
        if (!nv->is_string())
            throw ValueError(N_(".comment requires string parameters"));
        append_data(*comment, nv->get_string(), true);
    }
}

} // namespace yasm
