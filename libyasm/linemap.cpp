//
// YASM assembler virtual line mapping handling (for parse stage)
//
//  Copyright (C) 2002-2007  Peter Johnson
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
#include "util.h"

#include <algorithm>

#include "linemap.h"


namespace yasm {

Linemap::Linemap()
    : m_current(1)
{
}

Linemap::~Linemap()
{
}

void
Linemap::set(unsigned long file_line, unsigned long line_inc)
{
    if (!m_map.empty())
        set(m_map.back().m_filename, file_line, line_inc);
    else
        set("unknown", file_line, line_inc);
}

void
Linemap::set(const std::string& filename,
             unsigned long file_line,
             unsigned long line_inc)
{
    m_filenames.insert(filename);
    m_map.push_back(Mapping(m_current, filename, file_line, line_inc));
}

unsigned long
Linemap::poke(const std::string& filename, unsigned long file_line)
{
    m_current++;
    set(filename, file_line, 0);

    const Mapping& mapping = *(m_map.rbegin()+1);

    unsigned long line = m_current;

    m_current++;
    set(mapping.m_filename,
        mapping.m_file_line + mapping.m_line_inc*(m_current-2-mapping.m_line),
        mapping.m_line_inc);

    return line;
}

unsigned long
Linemap::poke(unsigned long file_line)
{
    if (!m_map.empty())
        return poke(m_map.back().m_filename, file_line);
    else
        return poke("unknown", file_line);
}

void
Linemap::add_source(Bytecode* bc, const std::string& source)
{
    if (m_source.size() < m_current)
        m_source.resize(m_current);
    m_source[m_current-1] = Source(bc, source);
}

bool
Linemap::lookup(unsigned long line,
                std::string& filename,
                unsigned long& file_line) const
{
    if (m_map.empty()) {
        filename = "unknown";
        file_line = 0;
        return false;
    }

    Mapping searchfor(line, "", 0, 0);
    Mappings::const_iterator m =
        std::upper_bound(m_map.begin(), m_map.end(), searchfor);
    if (m != m_map.begin())
        --m;

    filename = m->m_filename;
    file_line = m->m_file_line + m->m_line_inc * (line - m->m_line);
    return true;
}

bool
Linemap::get_source(unsigned long line,
                    Bytecode* &bc,
                    std::string& source) const
{
    if (m_source.size() < line) {
        bc = 0;
        source = "";
        return false;
    }

    bc = m_source[line-1].m_bc;
    source = m_source[line-1].m_source;

    return true;
}

} // namespace yasm
