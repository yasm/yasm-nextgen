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
#include "bin-link.h"

#include <util.h>

#include <algorithm>

#include <yasmx/Support/Compose.h>
#include <yasmx/Support/errwarn.h>
#include <yasmx/Support/marg_ostream.h>
#include <yasmx/Bytecode.h>
#include <yasmx/Errwarns.h>
#include <yasmx/Expr.h>
#include <yasmx/Object.h>
#include <yasmx/Section.h>

#include "bin-data.h"


namespace yasm
{
namespace objfmt
{
namespace bin
{

BinGroup::BinGroup(Section& section, BinSection& bsd)
    : m_section(section),
      m_bsd(bsd),
      m_follow_groups_owner(m_follow_groups)
{
}

BinGroup::~BinGroup()
{
}

marg_ostream&
operator<< (marg_ostream& os, const BinGroup& group)
{
    os << "Section `" << group.m_section.get_name() << "':\n";
    ++os;
    group.m_bsd.put(os);
    --os;
    if (group.m_follow_groups.size() > 0)
    {
        os << "Following groups:\n";
        ++os;
        os << group.m_follow_groups;
        --os;
    }
    return os;
}

marg_ostream&
operator<< (marg_ostream& os, const BinGroups& groups)
{
    for (BinGroups::const_iterator group = groups.begin(), end = groups.end();
         group != end; ++group)
        os << *group;
    return os;
}

// Recursive function to find group containing named section.
static BinGroup*
find_group(BinGroups& groups, const std::string& name)
{
    for (BinGroups::iterator group = groups.begin(), end = groups.end();
         group != end; ++group)
    {
        if (group->m_section.get_name() == name)
            return &(*group);
        // Recurse to loop through follow groups
        BinGroup* found = find_group(group->m_follow_groups, name);
        if (found)
            return found;
    }
    return 0;
}

// Recursive function to find group.  Returns NULL if not found.
static BinGroup*
find_group(BinGroups& groups, const Section& section)
{
    for (BinGroups::iterator group = groups.begin(), end = groups.end();
         group != end; ++group)
    {
        if (&group->m_section == &section)
            return &(*group);
        // Recurse to loop through follow groups
        BinGroup* found = find_group(group->m_follow_groups, section);
        if (found)
            return found;
    }
    return 0;
}

Link::Link(Object& object, Errwarns& errwarns)
    : m_object(object),
      m_errwarns(errwarns),
      m_lma_groups_owner(m_lma_groups),
      m_vma_groups_owner(m_vma_groups)
{
}

Link::~Link()
{
}

bool
Link::lma_create_group(Section& section)
{
    BinSection* bsd = get_bin_sect(section);
    assert(bsd);

    // Determine section alignment as necessary.
    unsigned long align = section.get_align();
    if (!bsd->has_align)
    {
        bsd->has_align = true;
        bsd->align = (align > 4) ? align : 4;
    }
    else
    {
        if (align > bsd->align)
        {
            warn_set(WARN_GENERAL, String::compose(
                N_("section `%1' internal align of %2 is greater than `%3' of %4; using `%5'"),
                section.get_name(),
                align,
                N_("align"),
                bsd->align,
                N_("align")));
            m_errwarns.propagate(0);
        }
    }

    // Calculate section integer start.
    if (bsd->start.get() != 0)
    {
        const IntNum* istart = bsd->start->get_intnum();
        if (!istart)
        {
            m_errwarns.propagate(bsd->start_line,
                TooComplexError(N_("start expression is too complex")));
            return false;
        }
        else
        {
            bsd->has_istart = true;
            section.set_lma(*istart);
        }
    }

    // Calculate section integer vstart.
    if (bsd->vstart.get() != 0)
    {
        const IntNum* ivstart = bsd->vstart->get_intnum();
        if (!ivstart)
        {
            m_errwarns.propagate(bsd->vstart_line,
                TooComplexError(N_("vstart expression is too complex")));
            return false;
        }
        else
        {
            bsd->has_ivstart = true;
            section.set_vma(*ivstart);
        }
    }

    // Calculate section integer length.
    Location start = {&section.bcs_first(), 0};
    Location end = {&section.bcs_last(), section.bcs_last().get_total_len()};
    if (!calc_dist(start, end, &bsd->length))
        throw ValueError(String::compose(
            N_("could not determine length of section `%1'"),
            section.get_name()));
    bsd->has_length = true;

    m_lma_groups.push_back(new BinGroup(section, *bsd));
    return true;
}

static inline bool
not_bss(const BinGroup& group)
{
    return (group.m_bsd.has_istart || !group.m_section.is_bss());
}

static inline bool
compare_istart(const BinGroup& lhs, const BinGroup& rhs)
{
    if (!lhs.m_bsd.has_istart || !rhs.m_bsd.has_istart)
        return false;
    return (lhs.m_section.get_lma() < rhs.m_section.get_lma());
}

bool
Link::do_link(const IntNum& origin)
{
    BinGroups::iterator group;

    // Create LMA section groups
    for (Object::section_iterator i=m_object.sections_begin(),
         end=m_object.sections_end(); i != end; ++i)
    {
        if (!lma_create_group(*i))
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
            found = find_group(m_lma_groups, group->m_bsd.follows);
            if (!found)
            {
                m_errwarns.propagate(0, ValueError(String::compose(
                    N_("section `%1' follows an invalid or unknown section `%2'"),
                    group->m_section.get_name(),
                    group->m_bsd.follows)));
                return false;
            }

            // Check for loops
            if (&group->m_section == &found->m_section ||
                find_group(group->m_follow_groups, found->m_section))
            {
                m_errwarns.propagate(0, ValueError(String::compose(
                    N_("follows loop between section `%1' and section `%2'"),
                    group->m_section.get_name(),
                    found->m_section.get_name())));
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
                               not_bss);

    // Sort the other top-level groups according to their start address.
    // If no start address is specified for a section, don't change the order.
    stdx::stable_sort(m_lma_groups.begin(), bss_begin, compare_istart);

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
            start = i->m_section.get_lma();
        i->assign_start_recurse(start, last, vdelta, m_errwarns);
        start = last;
    }

    //
    // Determine section order according to VMA
    //

    // Create VMA section groups
    for (Object::section_iterator i=m_object.sections_begin(),
         end=m_object.sections_end(); i != end; ++i)
    {
        BinSection* bsd = get_bin_sect(*i);
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
            found = find_group(m_vma_groups, group->m_bsd.vfollows);
            if (!found)
            {
                m_errwarns.propagate(0, ValueError(String::compose(
                    N_("section `%1' vfollows an invalid or unknown section `%2'"),
                    group->m_section.get_name(),
                    group->m_bsd.vfollows)));
                return false;
            }

            // Check for loops
            if (&group->m_section == &found->m_section ||
                find_group(group->m_follow_groups, found->m_section))
            {
                m_errwarns.propagate(0, ValueError(String::compose(
                    N_("vfollows loop between section `%1' and section `%2'"),
                    group->m_section.get_name(),
                    found->m_section.get_name())));
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
        start = i->m_section.get_vma();
        i->assign_vstart_recurse(start, m_errwarns);
    }

    return true;
}

bool
Link::check_lma_overlap(const Section& sect, const Section& other)
{
    if (&sect == &other)
        return true;

    const BinSection* bsd = get_bin_sect(sect);
    const BinSection* bsd2 = get_bin_sect(other);

    assert(bsd);
    assert(bsd2);

    if (bsd->length.is_zero() || bsd2->length.is_zero())
        return true;

    IntNum overlap;
    if (sect.get_lma() <= other.get_lma())
    {
        overlap = sect.get_lma();
        overlap += bsd->length;
        overlap -= other.get_lma();
    }
    else
    {
        overlap = other.get_lma();
        overlap += bsd2->length;
        overlap -= sect.get_lma();
    }

    if (overlap.sign() > 0)
    {
        m_errwarns.propagate(0, Error(String::compose(
            N_("sections `%1' and `%2' overlap by %3 bytes"),
            sect.get_name(),
            other.get_name(),
            overlap)));
        return false;
    }

    return true;
}

// Check for LMA overlap using a simple N^2 algorithm.
bool
Link::check_lma_overlap()
{
    for (Object::const_section_iterator i=m_object.sections_begin(),
         end=m_object.sections_end(); i != end; ++i)
    {
        for (Object::const_section_iterator j=i+1, end=m_object.sections_end();
             j != end; ++j)
        {
            if (!check_lma_overlap(*i, *j))
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
align_start(const IntNum& start, const IntNum& align)
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
BinGroup::assign_start_recurse(IntNum& start,
                               IntNum& last,
                               IntNum& vdelta,
                               Errwarns& errwarns)
{
    // Determine LMA
    if (m_bsd.has_align)
    {
        m_section.set_lma(align_start(start, m_bsd.align));
        if (m_bsd.has_istart && start != m_section.get_lma())
        {
            warn_set(WARN_GENERAL,
                N_("start inconsistent with align; using aligned value"));
            errwarns.propagate(m_bsd.start_line);
        }
    }
    else
        m_section.set_lma(start);
    m_bsd.has_istart = true;

    // Determine VMA if either just valign specified or if no v* specified
    if (m_bsd.vstart.get() == 0)
    {
        if (m_bsd.vfollows.empty() && !m_bsd.has_valign)
        {
            // No v* specified, set VMA=LMA+vdelta.
            m_bsd.has_ivstart = true;
            m_section.set_vma(m_section.get_lma() + vdelta);
        }
        else if (m_bsd.vfollows.empty())
        {
            // Just valign specified: set VMA=LMA+vdelta, align VMA, then add
            // delta between unaligned and aligned to vdelta parameter.
            m_bsd.has_ivstart = true;
            IntNum orig_start = m_section.get_lma();
            orig_start += vdelta;
            m_section.set_vma(align_start(orig_start, m_bsd.valign));
            vdelta += m_section.get_vma();
            vdelta -= orig_start;
        }
    }

    // Find the maximum end value
    IntNum tmp = m_section.get_lma();
    tmp += m_bsd.length;
    if (tmp > last)
        last = tmp;

    // Recurse for each following group.
    for (BinGroups::iterator follow_group = m_follow_groups.begin(),
         end = m_follow_groups.end(); follow_group != end; ++follow_group)
    {
        // Following sections have to follow this one,
        // so add length to start.
        start = m_section.get_lma();
        start += m_bsd.length;

        follow_group->assign_start_recurse(start, last, vdelta, errwarns);
    }
}

// Recursive function to assign start addresses.
// Updates start parameter as it goes along.
// The tmp parameter is just a working intnum so one doesn't have to be
// locally allocated for this purpose.
void
BinGroup::assign_vstart_recurse(IntNum& start, Errwarns& errwarns)
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
        if (m_section.get_align() > m_bsd.valign)
        {
            warn_set(WARN_GENERAL, String::compose(
                N_("section `%1' internal align of %2 is greater than `%3' of %4; using `%5'"),
                m_section.get_name(),
                m_section.get_align(),
                N_("valign"),
                m_bsd.valign,
                N_("valign")));
            errwarns.propagate(0);
        }
    }

    // Determine VMA
    if (m_bsd.has_valign)
    {
        m_section.set_vma(align_start(start, m_bsd.valign));
        if (m_bsd.has_ivstart && start != m_section.get_vma())
        {
            errwarns.propagate(m_bsd.vstart_line,
                ValueError(N_("vstart inconsistent with valign")));
        }
    }
    else
        m_section.set_vma(start);
    m_bsd.has_ivstart = true;

    // Recurse for each following group.
    for (BinGroups::iterator follow_group = m_follow_groups.begin(),
         end = m_follow_groups.end(); follow_group != end; ++follow_group)
    {
        // Following sections have to follow this one,
        // so add length to start.
        start = m_section.get_vma();
        start += m_bsd.length;

        follow_group->assign_vstart_recurse(start, errwarns);
    }
}

}}} // namespace yasm::objfmt::bin
