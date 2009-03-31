#ifndef YASM_BYTECODECONTAINER_UTIL_H
#define YASM_BYTECODECONTAINER_UTIL_H
///
/// @file
/// @brief Bytecode container utility functions.
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
#include <string>

#include "yasmx/Config/export.h"


namespace yasm
{

class Arch;
class BytecodeContainer;
class Expr;
class IntNum;

/// The factory functions append to the end of a section.
/// The functions will throw an error if unable to append.

/// Append a single byte value to the end of a section.
/// @param sect         section
/// @param val          byte value
YASM_LIB_EXPORT
void append_byte(BytecodeContainer& container, unsigned char val);

/// Append a raw data value to the end of a section.
/// @param sect         section
/// @param val          data value
/// @param size         storage size (in bytes) for the data value
/// @param arch         architecture
YASM_LIB_EXPORT
void append_data(BytecodeContainer& container,
                 const IntNum& val,
                 unsigned int size,
                 const Arch& arch);

/// Append a data value to the end of a section.
/// @param sect         section
/// @param expr         data value
/// @param size         storage size (in bytes) for the data value
/// @param arch         architecture
/// @param line         virtual line number
YASM_LIB_EXPORT
void append_data(BytecodeContainer& container,
                 std::auto_ptr<Expr> expr,
                 unsigned int size,
                 const Arch& arch,
                 unsigned long line);

/// Append a string value to the end of a section.
/// @param sect         section
/// @param str          string/data (may contain 0 values)
/// @param append_zero  append a single zero byte after the string (if true)
YASM_LIB_EXPORT
void append_data(BytecodeContainer& container,
                 const std::string& str,
                 bool append_zero);

/// Append a sized string value to the end of a section.
/// @param sect         section
/// @param str          string/data (may contain 0 values)
/// @param size         storage size (in bytes) to round up to
/// @param append_zero  append a single zero byte after the string (if true)
YASM_LIB_EXPORT
void append_data(BytecodeContainer& container,
                 const std::string& str,
                 unsigned int size,
                 bool append_zero);

/// Append an LEB128-encoded data value to the end of a section.
/// @param sect         section
/// @param expr         data value
/// @param sign         signedness (True=signed, False=unsigned) of the
///                     data value
/// @param line         virtual line number
YASM_LIB_EXPORT
void append_leb128(BytecodeContainer& container,
                   std::auto_ptr<Expr> expr,
                   bool sign,
                   unsigned long line);

/// Append a binary file verbatim to the end of a section.
/// @param sect             section
/// @param filename         path to binary file
/// @param start            starting location in file (in bytes) to read data
///                         from; may be NULL to indicate 0.
/// @param maxlen           maximum number of bytes to read from the file;
///                         may be NULL to indicate no maximum
/// @param line             virtual line number
YASM_LIB_EXPORT
void append_incbin(BytecodeContainer& container,
                   const std::string& filename,
                   /*@null@*/ std::auto_ptr<Expr> start,
                   /*@null@*/ std::auto_ptr<Expr> maxlen,
                   unsigned long line);

/// Append an alignment constraint that aligns the following data to a boundary.
/// @param sect         section
/// @param boundary     byte alignment (must be a power of two)
/// @param fill         fill data (if NULL, code_fill or 0 is used)
/// @param maxskip      maximum number of bytes to skip
/// @param code_fill    code fill data (if NULL, 0 is used)
/// @param line         virtual line number
/// @note The precedence on generated fill is as follows:
///       - from fill parameter (if not NULL)
///       - from code_fill parameter (if not NULL)
///       - 0
YASM_LIB_EXPORT
void append_align(BytecodeContainer& container,
                  std::auto_ptr<Expr> boundary,
                  /*@null@*/ std::auto_ptr<Expr> fill,
                  /*@null@*/ std::auto_ptr<Expr> maxskip,
                  /*@null@*/ const unsigned char** code_fill,
                  unsigned long line);

/// Append a location constraint that puts the following data at a fixed
/// section offset.
/// @param sect         section
/// @param start        section offset of following bytecode
/// @param fill         fill value
/// @param line         virtual line number
YASM_LIB_EXPORT
void append_org(BytecodeContainer& container,
                unsigned long start,
                unsigned long fill,
                unsigned long line);

/// Append a multiple container.
/// @param container    bytecode container
/// @param multiple     multiple expression
/// @param line         virtual line number
/// @return Multiple inner container.
YASM_LIB_EXPORT
BytecodeContainer& append_multiple(BytecodeContainer& container,
                                   std::auto_ptr<Expr> multiple,
                                   unsigned long line);

} // namespace yasm

#endif
