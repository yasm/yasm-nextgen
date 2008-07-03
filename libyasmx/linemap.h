#ifndef YASM_LINEMAP_H
#define YASM_LINEMAP_H
///
/// @file
/// @brief Virtual line mapping interface.
///
/// @license
///  Copyright (C) 2002-2007  Peter Johnson
///
/// Redistribution and use in source and binary forms, with or without
/// modification, are permitted provided that the following conditions
/// are met:
/// 1. Redistributions of source code must retain the above copyright
///    notice, this list of conditions and the following disclaimer.
/// 2. Redistributions in binary form must reproduce the above copyright
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
#include <set>
#include <string>
#include <vector>

#include "export.h"
#include "location.h"


namespace yasm
{

class Bytecode;

class YASM_LIB_EXPORT Linemap
{
public:
    typedef std::set<std::string> Filenames;

    /// Constructor.  Virtual line number is initialized to 1.
    Linemap();

    /// Destructor.
    ~Linemap();

    /// Get the current line position in a repository.
    /// @return Current virtual line.
    unsigned long get_current() const { return m_current; }

    /// Get bytecode and source line information, if any, for a virtual line.
    /// @param line         virtual line
    /// @param loc          location (output)
    /// @param source       source code line pointer (output)
    /// @return True if source line information available for line, false if
    ///         not.
    /// @note If source line information is not available, loc is set to 0 and
    ///       source is set to "".
    bool get_source(unsigned long line,
                    /*@out@*/ Location& loc,
                    /*@out@*/ std::string& source) const;

    /// Add location and source line information to the current virtual line.
    /// @attention Deletes any existing location and source line information for
    ///            the current virtual line.
    /// @param loc          location
    /// @param source       source code line
    void add_source(Location loc, const std::string& source);

    /// Go to the next line (increments the current virtual line).
    /// @return The current (new) virtual line.
    unsigned long goto_next() { return ++m_current; }

    /// Set a new file/line physical association starting point at the current
    /// virtual line.  line_inc indicates how much the "real" line is
    /// incremented by for each virtual line increment (0 is perfectly legal).
    /// @param filename     physical file name (optional)
    /// @param file_line    physical line number
    /// @param line_inc     line increment
    void set(unsigned long file_line, unsigned long line_inc);
    void set(const std::string& filename,
             unsigned long file_line,
             unsigned long line_inc);

    /// Poke a single file/line association, restoring the original physical
    /// association starting point.  Caution: increments the current virtual
    /// line twice.
    /// @param filename     physical file name (optional)
    /// @param file_line    physical line number
    /// @return The virtual line number of the poked association.
    unsigned long poke(const std::string& filename,
                       unsigned long file_line);
    unsigned long poke(unsigned long file_line);

    /// Look up the associated physical file and line for a virtual line.
    /// @param line         virtual line
    /// @param filename     physical file name (output)
    /// @param file_line    physical line number (output)
    /// @return True if information available for line, false if not.
    bool lookup(unsigned long line,
                /*@out@*/ std::string& filename,
                /*@out@*/ unsigned long& file_line) const;

    /// Get all filenames used in a linemap.
    const Filenames& get_filenames() const { return m_filenames; }

private:
    /// Current virtual line number.
    unsigned long m_current;

    /// Physical mapping info.
    class Mapping
    {
    public:
        Mapping(unsigned long line, const std::string& filename,
                unsigned long file_line, unsigned long line_inc)
            : m_line(line), m_filename(filename), m_file_line(file_line),
              m_line_inc(line_inc)
        {}

        /// Comparison operator to sort on virtual line number.
        bool operator< (const Mapping& other) const
        {
            return (m_line < other.m_line);
        }

        /// Monotonically increasing virtual line number.
        unsigned long m_line;

        /// Physical source filename.
        std::string m_filename;
        /// Physical source base line number.
        unsigned long m_file_line;
        /// Physical source line number increment (for following lines).
        unsigned long m_line_inc;
    };

    typedef std::vector<Mapping> Mappings;

    /// Mappings from virtual to physical line numbers.
    /// Kept sorted on virtual line number.
    Mappings m_map;

    /// Source code line info.
    class Source
    {
    public:
        Source() {}
        Source(Location loc, const std::string& source)
            : m_loc(loc), m_source(source)
        {}

        /// First location on line.  Loc.bc=0 if no locations on line.
        Location m_loc;

        /// Source code line.
        std::string m_source;
    };

    /// Bytecode and source line information.
    std::vector<Source> m_source;

    /// All used filenames.
    Filenames m_filenames;
};

} // namespace yasm

#endif
