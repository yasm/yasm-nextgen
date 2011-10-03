#ifndef YASM_BINLINK_H
#define YASM_BINLINK_H
//
// Flat-format binary object format multi-section linking
//
//  Copyright (C) 2002-2008  Peter Johnson
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
#include "yasmx/Config/export.h"
#include "yasmx/Support/ptr_vector.h"
#include "yasmx/Bytes.h"
#include "yasmx/DebugDumper.h"


namespace yasm
{

class Bytecode;
class DiagnosticsEngine;
class IntNum;
class Object;
class Section;
class Value;

namespace objfmt
{

class BinObject;
struct BinSection;

class BinGroup;
typedef stdx::ptr_vector<BinGroup> BinGroups;

class YASM_STD_EXPORT BinGroup
{
public:
    BinGroup(Section& section, BinSection& bsd);
    ~BinGroup();

#ifdef WITH_XML
    pugi::xml_node Write(pugi::xml_node out) const;
#endif // WITH_XML

    void AssignStartRecurse(IntNum& start,
                            IntNum& last,
                            IntNum& vdelta,
                            DiagnosticsEngine& diags);
    void AssignVStartRecurse(IntNum& start, DiagnosticsEngine& diags);

    Section& m_section;
    BinSection& m_bsd;

    // Groups that (in parallel) logically come immediately after this
    // group's section.
    BinGroups m_follow_groups;
    stdx::ptr_vector_owner<BinGroup> m_follow_groups_owner;
};

class YASM_STD_EXPORT BinLink
{
public:
    BinLink(Object& object, DiagnosticsEngine& diags);
    ~BinLink();

#ifdef WITH_XML
    pugi::xml_node Write(pugi::xml_node out) const;
#endif // WITH_XML

    bool DoLink(const IntNum& origin);
    bool CheckLMAOverlap();

    const BinGroups& getLMAGroups() const { return m_lma_groups; }

private:
    bool CreateLMAGroup(Section& sect);
    bool CheckLMAOverlap(const Section& sect, const Section& other);

    void OutputValue(Value& value, Bytes& bytes, unsigned int destsize,
                     /*@unused@*/ unsigned long offset, Bytecode& bc,
                     int warn);
    void OutputBytecode(Bytecode& bc);

    Object& m_object;
    DiagnosticsEngine& m_diags;

    BinGroups m_lma_groups, m_vma_groups;
    stdx::ptr_vector_owner<BinGroup> m_lma_groups_owner, m_vma_groups_owner;
};

#ifdef WITH_XML
YASM_STD_EXPORT
pugi::xml_node append_data(pugi::xml_node out, const BinGroups& groups);
#endif // WITH_XML

}} // namespace yasm::objfmt

#endif
