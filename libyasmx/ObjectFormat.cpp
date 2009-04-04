//
// Object format module base implementation.
//
//  Copyright (C) 2007  Peter Johnson
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
#include "yasmx/ObjectFormat.h"

#include "util.h"

#include "yasmx/Support/errwarn.h"


namespace yasm
{

ObjectFormat::ObjectFormat()
    : m_object(0)
{
}

ObjectFormat::~ObjectFormat()
{
}

std::string
ObjectFormat::get_type() const
{
    return "ObjectFormat";
}

bool
ObjectFormat::set_object(Object* object)
{
    if (!ok_object(object))
        return false;
    m_object = object;
    initialize();
    return true;
}

bool
ObjectFormat::taste(std::istream& is,
                    /*@out@*/ std::string* arch_keyword,
                    /*@out@*/ std::string* machine)
{
    return false;
}

void
ObjectFormat::read(std::istream& is)
{
    throw NotImplementedError(N_("object format does not support reading"));
}

bool
ObjectFormat::ok_object(Object* object) const
{
    return true;
}

void
ObjectFormat::init_symbols(const std::string& parser)
{
}

void
ObjectFormat::initialize()
{
}

} // namespace yasm
