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
#include "BinLink.h"

#include <algorithm>

#include "llvm/ADT/Twine.h"
#include "llvm/Support/raw_ostream.h"
#include "yasmx/Basic/Diagnostic.h"
#include "yasmx/Bytecode.h"
#include "yasmx/Expr.h"
#include "yasmx/Object.h"
#include "yasmx/Section.h"

#include "BinSection.h"


using namespace yasm;
using namespace yasm::objfmt;

BinGroup::BinGroup(Section& section, BinSection& bsd)
    : m_section(section),
      m_bsd(bsd),
      m_follow_groups_owner(m_follow_groups)
{
}

BinGroup::~BinGroup()
{
}

#ifdef WITH_XML
pugi::xml_node
BinGroup::Write(pugi::xml_node out) const
{
    pugi::xml_node root = out.append_child("BinGroup");
    root.append_attribute("section") = m_section.getName().str().c_str();
    append_child(root, "FollowGroups", m_follow_groups);
    return root;
}

pugi::xml_node
yasm::objfmt::append_data(pugi::xml_node out, const BinGroups& groups)
{
    pugi::xml_node root = out.append_child("BinGroups");
    for (BinGroups::const_iterator group = groups.begin(), end = groups.end();
         group != end; ++group)
        yasm::append_data(root, *group);
    return root;
}
#endif // WITH_XML

// Recursive function to find group containing named section.
static BinGroup*
FindGroup(BinGroups& groups, const std::string& name)
{
    for (BinGroups::iterator group = groups.begin(), end = groups.end();
         group != end; ++group)
    {
        if (group->m_section.getName() == name)
            return &(*group);
        // Recurse to loop through follow groups
        BinGroup* found = FindGroup(group->m_follow_groups, name);
        if (found)
            return found;
    }
    return 0;
}

// Recursive function to find group.  Returns NULL if not found.
static BinGroup*
FindGroup(BinGroups& groups, const Section& section)
{
    for (BinGroups::iterator group = groups.begin(), end = groups.end();
         group != end; ++group)
    {
        if (&group->m_section == &section)
            return &(*group);
        // Recurse to loop through follow groups
        BinGroup* found = FindGroup(group->m_follow_groups, section);
        if (found)
            return found;
    }
    return 0;
}

BinLink::BinLink(Object& object, DiagnosticsEngine& diags)
    : m_object(object),
      m_diags(diags),
      m_lma_groups_owner(m_lma_groups),
      m_vma_groups_owner(m_vma_groups)
{
}

BinLink::~BinLink()
{
}

#ifdef WITH_XML
pugi::xml_node
BinLink::Write(pugi::xml_node out) const
{
    pugi::xml_node root = out.append_child("BinLink");
    append_data(root, m_lma_groups).append_attribute("type") = "lma";
    append_data(root, m_vma_groups).append_attribute("type") = "vma";
    return root;
}
#endif // WITH_XML

bool
BinLink::CreateLMAGroup(Section& section)
{
    BinSection* bsd = section.getAssocData<BinSection>();
    assert(bsd);

    // Determine section alignment as necessary.
    unsigned long align = section.getAlign();
    if (!bsd->has_align)
    {
        bsd->has_align = true;
        bsd->align = (align > 4) ? align : 4;
    }
    else
    {
        if (align > bsd->align)
        {
            m_diags.Report(SourceLocation(), diag::warn_section_align_larger)
                << section.getName()
                << static_cast<unsigned int>(align)
                << "align"
                << bsd->align.getStr();
        }
    }

    // Calculate section integer start.
    if (bsd->start.get() != 0)
    {
        if (!bsd->start->isIntNum())
        {
            m_diags.Report(bsd->start_source, diag::err_start_too_complex);
            return false;
        }
        else
        {
            bsd->has_istart = true;
            section.setLMA(bsd->start->getIntNum());
        }
    }

    // Calculate section integer vstart.
    if (bsd->vstart.get() != 0)
    {
        if (!bsd->vstart->isIntNum())
        {
            m_diags.Report(bsd->vstart_source, diag::err_vstart_too_complex);
            return false;
        }
        else
        {
            bsd->has_ivstart = true;
            section.setVMA(bsd->vstart->getIntNum());
        }
    }

    // Calculate section integer length.
    Location start = {&section.bytecodes_front(), 0};
    Location end = {&section.bytecodes_back(),
                    section.bytecodes_back().getTotalLen()};
    if (!CalcDist(start, end, &bsd->length))
    {
        m_diags.Report(bsd->vstart_source,
                       diag::err_indeterminate_section_length)
            << section.getName();
        return false;
    }
    bsd->has_length = true;

    m_lma_groups.push_back(new BinGroup(section, *bsd));
    return true;
}

static inline bool
isNotBSS(const BinGroup& group)
{
    return (group.m_bsd.has_istart || !group.m_section.isBSS());
}

static inline bool
CompareIStart(const BinGroup& lhs, const BinGroup& rhs)
{
    if (!lhs.m_bsd.has_istart || !rhs.m_bsd.has_istart)
        return false;
    return (lhs.m_section.getLMA() < rhs.m_section.getLMA());
}

bool
BinLink::DoLink(const IntNum& origin)
{
    BinGroups::iterator group;

    // Create LMA section groups
    for (Object::section_iterator i=m_object.sections_begin(),
         end=m_object.sections_end(); i != end; ++i)
    {
        if (!CreateLMAGroup(*i))
            return false;
    }

    // Determine section order according to LMA.
    // Sections can be ordered either by (priority):
    //  - follows
    //  - start
    //  - progbits/nobits setting
    //  - order in the input file

    // Look at each group with follows specified, and find the section
    // that group is supposed to follow.
    group = m_lma_groups.begin();
    while (group != m_lma_groups.end())
    {
        if (!group->m_bsd.follows.empty())
        {
            BinGroup* found;
            // Need to find group containing section this section follows.
            found = FindGroup(m_lma_groups, group->m_bsd.follows);
            if (!found)
            {
                m_diags.Report(SourceLocation(),
                               diag::err_section_follows_unknown)
                    << group->m_section.getName()
                    << group->m_bsd.follows;
                return false;
            }

            // Check for loops
            if (&group->m_section == &found->m_section ||
                FindGroup(group->m_follow_groups, found->m_section))
            {
                m_diags.Report(SourceLocation(), diag::err_section_follows_loop)
                    << group->m_section.getName()
                    << found->m_section.getName();
                return false;
            }

            // Remove this section from main lma groups list and
            // add it after the section it's supposed to follow.
            found->m_follow_groups.push_back(m_lma_groups.detach(group));
        }
        else
            ++group;
    }

    // Move BSS sections without a start to the end of the top-level groups
    BinGroups::iterator bss_begin =
        stdx::stable_partition(m_lma_groups.begin(), m_lma_groups.end(),
                               isNotBSS);

    // Sort the other top-level groups according to their start address.
    // If no start address is specified for a section, don't change the order.
    stdx::stable_sort(m_lma_groups.begin(), bss_begin, CompareIStart);

    // Assign a LMA start address to every section.
    // Also assign VMA=LMA unless otherwise specified.
    //
    // We need to assign VMA=LMA here (while walking the tree) for the case:
    //  sect1 start=0 (size=0x11)
    //  sect2 follows=sect1 valign=16 (size=0x104)
    //  sect3 follows=sect2 valign=16
    // Where the valign of sect2 will result in a sect3 vaddr higher than a
    // naive segment-by-segment interpretation (where sect3 and sect2 would
    // have a VMA overlap).
    //
    // Algorithm for VMA=LMA setting:
    // Start with delta=0.
    // If there's no virtual attributes, we simply set VMA = LMA+delta.
    // If there's only valign specified, we set VMA = aligned LMA, and add
    // any new alignment difference to delta.
    //
    // We could do the LMA start and VMA=LMA steps in two separate steps,
    // but it's easier to just recurse once.
    IntNum start(origin);
    IntNum last(origin);
    IntNum vdelta(0);

    for (BinGroups::iterator i=m_lma_groups.begin(), end=m_lma_groups.end();
         i != end; ++i)
    {
        if (i->m_bsd.has_istart)
            start = i->m_section.getLMA();
        i->AssignStartRecurse(start, last, vdelta, m_diags);
        start = last;
    }

    //
    // Determine section order according to VMA
    //

    // Create VMA section groups
    for (Object::section_iterator i=m_object.sections_begin(),
         end=m_object.sections_end(); i != end; ++i)
    {
        BinSection* bsd = i->getAssocData<BinSection>();
        assert(bsd);
        m_vma_groups.push_back(new BinGroup(*i, *bsd));
    }

    // Look at each group with vfollows specified, and find the section
    // that group is supposed to follow.
    group = m_vma_groups.begin();
    while (group != m_vma_groups.end())
    {
        if (!group->m_bsd.vfollows.empty())
        {
            BinGroup* found;
            // Need to find group containing section this section follows.
            found = FindGroup(m_vma_groups, group->m_bsd.vfollows);
            if (!found)
            {
                m_diags.Report(SourceLocation(),
                               diag::err_section_vfollows_unknown)
                    << group->m_section.getName()
                    << group->m_bsd.vfollows;
                return false;
            }

            // Check for loops
            if (&group->m_section == &found->m_section ||
                FindGroup(group->m_follow_groups, found->m_section))
            {
                m_diags.Report(SourceLocation(),
                               diag::err_section_vfollows_loop)
                    << group->m_section.getName()
                    << found->m_section.getName();
                return false;
            }

            // Remove this section from main lma groups list and
            // add it after the section it's supposed to follow.
            found->m_follow_groups.push_back(m_vma_groups.detach(group));
        }
        else
            ++group;
    }

    // Due to the combination of steps above, we now know that all top-level
    // groups have integer ivstart:
    // Vstart Vfollows Valign   Handled by
    //     No       No     No   group_assign_start_recurse()
    //     No       No    Yes   group_assign_start_recurse()
    //     No      Yes    -     vfollows loop (above)
    //    Yes      -      -     bin_lma_create_group()
    for (BinGroups::iterator i=m_vma_groups.begin(), end=m_vma_groups.end();
         i != end; ++i)
    {
        start = i->m_section.getVMA();
        i->AssignVStartRecurse(start, m_diags);
    }

    return true;
}

bool
BinLink::CheckLMAOverlap(const Section& sect, const Section& other)
{
    if (&sect == &other)
        return true;

    const BinSection* bsd = sect.getAssocData<BinSection>();
    const BinSection* bsd2 = other.getAssocData<BinSection>();

    assert(bsd);
    assert(bsd2);

    if (bsd->length.isZero() || bsd2->length.isZero())
        return true;

    IntNum overlap;
    if (sect.getLMA() <= other.getLMA())
    {
        overlap = sect.getLMA();
        overlap += bsd->length;
        overlap -= other.getLMA();
    }
    else
    {
        overlap = other.getLMA();
        overlap += bsd2->length;
        overlap -= sect.getLMA();
    }

    if (overlap.getSign() > 0)
    {
        m_diags.Report(SourceLocation(), diag::err_section_overlap)
            << sect.getName()
            << other.getName()
            << overlap.getStr();
        return false;
    }

    return true;
}

// Check for LMA overlap using a simple N^2 algorithm.
bool
BinLink::CheckLMAOverlap()
{
    for (Object::const_section_iterator i=m_object.sections_begin(),
         end=m_object.sections_end(); i != end; ++i)
    {
        for (Object::const_section_iterator j=i+1, end=m_object.sections_end();
             j != end; ++j)
        {
            if (!CheckLMAOverlap(*i, *j))
                return false;
        }
    }
    return true;
}

// Calculates new start address based on alignment constraint.
// Start is modified (rounded up) to the closest aligned value greater than
// what was passed in.
// Align must be a power of 2.
static IntNum
AlignStart(const IntNum& start, const IntNum& align)
{
    // Because alignment is always a power of two, we can use some bit
    // trickery to do this easily.
    if ((start & (align-1)) != 0)
        return (start & ~(align-1)) + align;
    else
        return start;
}

// Recursive function to assign start addresses.
// Updates start, last, and vdelta parameters as it goes along.
// The tmp parameter is just a working intnum so one doesn't have to be
// locally allocated for this purpose.
void
BinGroup::AssignStartRecurse(IntNum& start,
                             IntNum& last,
                             IntNum& vdelta,
                             DiagnosticsEngine& diags)
{
    // Determine LMA
    if (m_bsd.has_align)
    {
        m_section.setLMA(AlignStart(start, m_bsd.align));
        if (m_bsd.has_istart && start != m_section.getLMA())
            diags.Report(m_bsd.start_source, diag::warn_start_not_aligned);
    }
    else
        m_section.setLMA(start);
    m_bsd.has_istart = true;

    // Determine VMA if either just valign specified or if no v* specified
    if (m_bsd.vstart.get() == 0)
    {
        if (m_bsd.vfollows.empty() && !m_bsd.has_valign)
        {
            // No v* specified, set VMA=LMA+vdelta.
            m_bsd.has_ivstart = true;
            m_section.setVMA(m_section.getLMA() + vdelta);
        }
        else if (m_bsd.vfollows.empty())
        {
            // Just valign specified: set VMA=LMA+vdelta, align VMA, then add
            // delta between unaligned and aligned to vdelta parameter.
            m_bsd.has_ivstart = true;
            IntNum orig_start = m_section.getLMA();
            orig_start += vdelta;
            m_section.setVMA(AlignStart(orig_start, m_bsd.valign));
            vdelta += m_section.getVMA();
            vdelta -= orig_start;
        }
    }

    // Find the maximum end value
    IntNum tmp = m_section.getLMA();
    tmp += m_bsd.length;
    if (tmp > last)
        last = tmp;

    // Recurse for each following group.
    for (BinGroups::iterator follow_group = m_follow_groups.begin(),
         end = m_follow_groups.end(); follow_group != end; ++follow_group)
    {
        // Following sections have to follow this one,
        // so add length to start.
        start = m_section.getLMA();
        start += m_bsd.length;

        follow_group->AssignStartRecurse(start, last, vdelta, diags);
    }
}

// Recursive function to assign start addresses.
// Updates start parameter as it goes along.
// The tmp parameter is just a working intnum so one doesn't have to be
// locally allocated for this purpose.
void
BinGroup::AssignVStartRecurse(IntNum& start, DiagnosticsEngine& diags)
{
    // Determine VMA section alignment as necessary.
    // Default to LMA alignment if not specified.
    if (!m_bsd.has_valign)
    {
        m_bsd.has_valign = true;
        m_bsd.valign = m_bsd.align;
    }
    else
    {
        if (m_section.getAlign() > m_bsd.valign)
        {
            diags.Report(SourceLocation(), diag::warn_section_align_larger)
                << m_section.getName()
                << static_cast<unsigned int>(m_section.getAlign())
                << "valign"
                << m_bsd.valign.getStr();
        }
    }

    // Determine VMA
    if (m_bsd.has_valign)
    {
        m_section.setVMA(AlignStart(start, m_bsd.valign));
        if (m_bsd.has_ivstart && start != m_section.getVMA())
            diags.Report(m_bsd.vstart_source, diag::err_vstart_not_aligned);
    }
    else
        m_section.setVMA(start);
    m_bsd.has_ivstart = true;

    // Recurse for each following group.
    for (BinGroups::iterator follow_group = m_follow_groups.begin(),
         end = m_follow_groups.end(); follow_group != end; ++follow_group)
    {
        // Following sections have to follow this one,
        // so add length to start.
        start = m_section.getVMA();
        start += m_bsd.length;

        follow_group->AssignVStartRecurse(start, diags);
    }
}
