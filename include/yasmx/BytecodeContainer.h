#ifndef YASM_BYTECODECONTAINER_H
#define YASM_BYTECODECONTAINER_H
///
/// @file
/// @brief Bytecode container interface.
///
/// @license
///  Copyright (C) 2001-2007  Peter Johnson
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
#include <memory>

#include "llvm/ADT/StringRef.h"
#include "yasmx/Config/export.h"
#include "yasmx/Support/EndianState.h"
#include "yasmx/Support/ptr_vector.h"
#include "yasmx/DebugDumper.h"
#include "yasmx/Location.h"


namespace yasm
{

class Arch;
class Bytecode;
class Diagnostic;
class Expr;
class IntNum;
class Section;
class SourceLocation;

/// A bytecode container.
class YASM_LIB_EXPORT BytecodeContainer : public DebugDumper<BytecodeContainer>
{
public:
    /// Constructor.
    BytecodeContainer(Section* sect);

    /// Destructor.
    ~BytecodeContainer();

    /// Get section parent.
    /// @return Section this container is/is a part of.
    Section* getSection() { return m_sect; }
    const Section* getSection() const { return m_sect; }

    /// Add bytecode to the end of the container.
    /// @param bc       bytecode (may be NULL)
    void AppendBytecode(/*@null@*/ std::auto_ptr<Bytecode> bc);

    /// Add gap space to the end of the container.
    /// @param size     number of bytes of gap
    /// @param source   source location
    /// @return Reference to gap bytecode.
    Bytecode& AppendGap(unsigned long size, SourceLocation source);

    /// Start a new bytecode at the end of the container.  Factory function.
    /// @return Reference to new bytecode.
    Bytecode& StartBytecode();

    /// Ensure the last bytecode in the container has no tail.  If the last
    /// bytecode has no tail, simply returns it; otherwise creates and returns
    /// a fresh bytecode.
    /// @return Reference to last bytecode.
    Bytecode& FreshBytecode();

    typedef stdx::ptr_vector<Bytecode>::iterator bc_iterator;
    typedef stdx::ptr_vector<Bytecode>::const_iterator const_bc_iterator;

    bc_iterator bytecodes_begin() { return m_bcs.begin(); }
    const_bc_iterator bytecodes_begin() const { return m_bcs.begin(); }
    bc_iterator bytecodes_end() { return m_bcs.end(); }
    const_bc_iterator bytecodes_end() const { return m_bcs.end(); }

    Bytecode& bytecodes_front() { return m_bcs.front(); }
    const Bytecode& bytecodes_front() const { return m_bcs.front(); }
    Bytecode& bytecodes_back() { return m_bcs.back(); }
    const Bytecode& bytecodes_back() const { return m_bcs.back(); }

    stdx::ptr_vector<Bytecode>::size_type size() { return m_bcs.size(); }

    /// Get location for start of a bytecode container.
    Location getBeginLoc()
    {
        Location loc = { &m_bcs.front(), 0 };
        return loc;
    }

    /// Get location for (current) end of a bytecode container.
    Location getEndLoc();

    /// Finalize all bytecodes after parsing.
    /// @param diags        diagnostic reporting
    /// @note Errors/warnings are stored into errwarns.
    void Finalize(Diagnostic& diags);

    /// Optimize this container.  Generally, Object::Optimize() should be
    /// called instead to optimize the entire object at once.
    /// @param diags        diagnostic reporting
    /// @note Errors/warnings are stored into diags.
    void Optimize(Diagnostic& diags);

    /// Update all bytecode offsets.
    /// @param diags        diagnostic reporting
    /// @note Errors/warnings are stored into errwarns.
    void UpdateOffsets(Diagnostic& diags);

#ifdef WITH_XML
    /// Write an XML representation.  For debugging purposes.
    /// @param out          XML node
    /// @return Root node.
    pugi::xml_node Write(pugi::xml_node out) const;
#endif // WITH_XML

private:
    // not implemented (noncopyable class)
    BytecodeContainer(const BytecodeContainer&);
    const BytecodeContainer& operator=(const BytecodeContainer&);

    Section* m_sect;        ///< Pointer to parent section

    /// The bytecodes for the section's contents.
    stdx::ptr_vector<Bytecode> m_bcs;
    stdx::ptr_vector_owner<Bytecode> m_bcs_owner;

    bool m_last_gap;        ///< Last bytecode is a gap bytecode
};

/// The factory functions append to the end of a section.
/// The functions will throw an error if unable to append.

/// Append a single byte value to the end of a section.
/// @param sect         section
/// @param val          byte value
YASM_LIB_EXPORT
void AppendByte(BytecodeContainer& container, unsigned char val);

/// Append a single byte value to the end of a section.
/// @param sect         section
/// @param expr         byte value
YASM_LIB_EXPORT
void AppendByte(BytecodeContainer& container,
                std::auto_ptr<Expr> expr,
                SourceLocation source,
                Diagnostic& diags);

/// Append a raw data value to the end of a section.
/// @param sect         section
/// @param val          data value
/// @param size         storage size (in bytes) for the data value
/// @param arch         architecture
YASM_LIB_EXPORT
void AppendData(BytecodeContainer& container,
                const IntNum& val,
                unsigned int size,
                const Arch& arch);

/// Append a raw data value to the end of a section.
/// @param sect         section
/// @param val          data value
/// @param size         storage size (in bytes) for the data value
/// @param endian       endianness
YASM_LIB_EXPORT
void AppendData(BytecodeContainer& container,
                const IntNum& val,
                unsigned int size,
                EndianState endian);

/// Append a data value to the end of a section.
/// @param sect         section
/// @param expr         data value
/// @param size         storage size (in bytes) for the data value
/// @param arch         architecture
/// @param source       source location
/// @param diags        diagnostic reporting
YASM_LIB_EXPORT
void AppendData(BytecodeContainer& container,
                std::auto_ptr<Expr> expr,
                unsigned int size,
                const Arch& arch,
                SourceLocation source,
                Diagnostic& diags);

/// Append a string value to the end of a section.
/// @param sect         section
/// @param str          string/data (may contain 0 values)
/// @param append_zero  append a single zero byte after the string (if true)
YASM_LIB_EXPORT
void AppendData(BytecodeContainer& container,
                llvm::StringRef str,
                bool append_zero);

/// Append a sized string value to the end of a section.
/// @param sect         section
/// @param str          string/data (may contain 0 values)
/// @param size         storage size (in bytes) to round up to
/// @param append_zero  append a single zero byte after the string (if true)
YASM_LIB_EXPORT
void AppendData(BytecodeContainer& container,
                llvm::StringRef str,
                unsigned int size,
                bool append_zero);

/// Append an LEB128-encoded data value to the end of a section.
/// @param sect         section
/// @param intn         data value
/// @param sign         signedness (True=signed, False=unsigned) of the
///                     data value
/// @param source       source location
YASM_LIB_EXPORT
void AppendLEB128(BytecodeContainer& container,
                  const IntNum& intn,
                  bool sign,
                  SourceLocation source,
                  Diagnostic& diags);

/// Append an LEB128-encoded data value to the end of a section.
/// @param sect         section
/// @param expr         data value
/// @param sign         signedness (True=signed, False=unsigned) of the
///                     data value
/// @param source       source location
YASM_LIB_EXPORT
void AppendLEB128(BytecodeContainer& container,
                  std::auto_ptr<Expr> expr,
                  bool sign,
                  SourceLocation source,
                  Diagnostic& diags);

/// Append a binary file verbatim to the end of a section.
/// @param sect             section
/// @param filename         path to binary file
/// @param start            starting location in file (in bytes) to read data
///                         from; may be NULL to indicate 0.
/// @param maxlen           maximum number of bytes to read from the file;
///                         may be NULL to indicate no maximum
/// @param source           source location
YASM_LIB_EXPORT
void AppendIncbin(BytecodeContainer& container,
                  llvm::StringRef filename,
                  /*@null@*/ std::auto_ptr<Expr> start,
                  /*@null@*/ std::auto_ptr<Expr> maxlen,
                  SourceLocation source);

/// Append an alignment constraint that aligns the following data to a boundary.
/// @param sect         section
/// @param boundary     byte alignment (must be a power of two)
/// @param fill         fill data (if NULL, code_fill or 0 is used)
/// @param maxskip      maximum number of bytes to skip
/// @param code_fill    code fill data (if NULL, 0 is used)
/// @param source       source location
/// @note The precedence on generated fill is as follows:
///       - from fill parameter (if not NULL)
///       - from code_fill parameter (if not NULL)
///       - 0
YASM_LIB_EXPORT
void AppendAlign(BytecodeContainer& container,
                 const Expr& boundary,
                 const Expr& fill,
                 const Expr& maxskip,
                 /*@null@*/ const unsigned char** code_fill,
                 SourceLocation source);

/// Append a location constraint that puts the following data at a fixed
/// section offset.
/// @param sect         section
/// @param start        section offset of following bytecode
/// @param fill         fill value
/// @param source       source location
YASM_LIB_EXPORT
void AppendOrg(BytecodeContainer& container,
               const Expr& start,
               const Expr& fill,
               SourceLocation source);

/// Append a multiple container.
/// @param container    bytecode container
/// @param contents     multiple contents
/// @param multiple     multiple expression
/// @param source       source location
/// @return Multiple inner container.
YASM_LIB_EXPORT
void AppendMultiple(BytecodeContainer& container,
                    std::auto_ptr<BytecodeContainer> contents,
                    std::auto_ptr<Expr> multiple,
                    SourceLocation source);

/// Append a skip.
/// @param container    bytecode container
/// @param multiple     multiple expression
/// @param size         size of each skip, in bytes
/// @param source       source location.
YASM_LIB_EXPORT
void AppendSkip(BytecodeContainer& container,
                std::auto_ptr<Expr> multiple,
                unsigned int size,
                SourceLocation source);

/// Append a fill.
/// @param container    bytecode container
/// @param multiple     multiple expression
/// @param size         size of each value, in bytes
/// @param value        value expression
/// @param source       source location.
YASM_LIB_EXPORT
void AppendFill(BytecodeContainer& container,
                std::auto_ptr<Expr> multiple,
                unsigned int size,
                std::auto_ptr<Expr> value,
                Arch& arch,
                SourceLocation source,
                Diagnostic& diags);

} // namespace yasm

#endif
