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
#include "yasmx/Object_util.h"

#include "yasmx/Basic/Diagnostic.h"
#include "yasmx/Parse/Directive.h"
#include "yasmx/Bytecode.h"
#include "yasmx/Section.h"
#include "yasmx/Object.h"
#include "yasmx/ObjectFormat.h"

using namespace yasm;

void
yasm::DirIdentCommon(ObjectFormat& objfmt,
                     StringRef sectname,
                     DirectiveInfo& info,
                     DiagnosticsEngine& diags)
{
    NameValues& namevals = info.getNameValues();
    // Accept, but do nothing with empty ident
    if (namevals.empty())
        return;

    // Put ident data into .comment section
    Section* comment = info.getObject().FindSection(sectname);
    if (!comment)
        comment = objfmt.AppendSection(sectname, info.getSource(), diags);

    // To match GAS output, if the comment section is empty, put an
    // initial 0 byte in the section.
    Bytecode& bc = comment->FreshBytecode();
    if (comment->bytecodes_begin() == comment->bytecodes_end() &&
        bc.getFixedLen() == 0)
        AppendByte(*comment, 0);

    for (NameValues::const_iterator nv=namevals.begin(), end=namevals.end();
         nv != end; ++nv)
    {
        if (nv->isString())
            AppendData(*comment, nv->getString(), true);
        else
            diags.Report(nv->getValueRange().getBegin(),
                         diag::err_value_string);
    }
}
