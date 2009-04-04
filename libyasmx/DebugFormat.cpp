///
/// @file libyasm/debug_format.cpp
/// @brief YASM debug format base implementation.
///
/// @license
///  Copyright (C) 2002-2007  Peter Johnson
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
#include "yasmx/DebugFormat.h"


namespace yasm
{

DebugFormat::DebugFormat()
    : m_object(0)
{
}

DebugFormat::~DebugFormat()
{
}

std::string
DebugFormat::get_type() const
{
    return "DebugFormat";
}

bool
DebugFormat::set_object(Object* object)
{
    if (!ok_object(object))
        return false;
    m_object = object;
    initialize();
    return true;
}

bool
DebugFormat::ok_object(Object* object) const
{
    return true;
}

void
DebugFormat::initialize()
{
}

} // namespace yasm
