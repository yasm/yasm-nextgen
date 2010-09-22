//
// Effective address container
//
//  Copyright (C) 2005-2007  Peter Johnson
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
#include "yasmx/EffAddr.h"

#include "llvm/Support/raw_ostream.h"
#include "yasmx/Arch.h"
#include "yasmx/Expr.h"


using namespace yasm;

EffAddr::EffAddr(std::auto_ptr<Expr> e)
    : m_disp(0, e),
      m_segreg(0),
      m_need_nonzero_len(false),
      m_need_disp(false),
      m_nosplit(false),
      m_strong(false),
      m_pc_rel(false),
      m_not_pc_rel(false)
{
}

EffAddr::EffAddr(const EffAddr& rhs)
    : m_disp(rhs.m_disp),
      m_segreg(rhs.m_segreg),
      m_need_nonzero_len(rhs.m_need_nonzero_len),
      m_need_disp(rhs.m_need_disp),
      m_nosplit(rhs.m_nosplit),
      m_strong(rhs.m_strong),
      m_pc_rel(rhs.m_pc_rel),
      m_not_pc_rel(rhs.m_not_pc_rel)
{
}

EffAddr::~EffAddr()
{
}
