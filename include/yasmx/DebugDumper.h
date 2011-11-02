#ifndef YASM_DEBUGDUMP_H
#define YASM_DEBUGDUMP_H
///
/// @file
/// @brief Debug dump interface.
///
/// @license
///  Copyright (C) 2011  Peter Johnson
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
#ifdef WITH_XML
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"
#include "pugixml/pugixml.h"
#endif // WITH_XML
#include "yasmx/Config/export.h"


namespace yasm
{

#ifdef WITH_XML
class YASM_LIB_EXPORT xml_writer_raw_ostream : public pugi::xml_writer
{
public:
    xml_writer_raw_ostream(llvm::raw_ostream& os) : m_os(os) {}
    virtual ~xml_writer_raw_ostream();
    void write(const void* data, size_t size);

private:
    llvm::raw_ostream& m_os;
};
#endif // WITH_XML

template <typename T>
void
DumpXml(const T& cls)
{
#ifdef WITH_XML
    pugi::xml_document doc;
    cls.Write(doc);
    xml_writer_raw_ostream writer(llvm::errs());
    doc.print(writer);
#endif // WITH_XML
}

#ifdef WITH_XML
/// Generic catch-all helper function to append something.
template <typename T>
inline pugi::xml_node
append_data(pugi::xml_node node, const T& val)
{
    return val.Write(node);
}

/// Helper functions to append a simple pcdata integer or string node.
YASM_LIB_EXPORT
pugi::xml_node append_data(pugi::xml_node node, const char* val);
YASM_LIB_EXPORT
pugi::xml_node append_data(pugi::xml_node node, const std::string& val);
YASM_LIB_EXPORT
pugi::xml_node append_data(pugi::xml_node node, llvm::StringRef val);
YASM_LIB_EXPORT
pugi::xml_node append_data(pugi::xml_node node, int val);
YASM_LIB_EXPORT
pugi::xml_node append_data(pugi::xml_node node, unsigned int val);
YASM_LIB_EXPORT
pugi::xml_node append_data(pugi::xml_node node, long val);
YASM_LIB_EXPORT
pugi::xml_node append_data(pugi::xml_node node, unsigned long val);
YASM_LIB_EXPORT
pugi::xml_node append_data(pugi::xml_node node, bool val);
YASM_LIB_EXPORT
pugi::xml_node append_data(pugi::xml_node node, double val);

/// Generic catch-all helper function to append <tag>something</tag>.
template <typename T>
inline pugi::xml_node
append_child(pugi::xml_node node, const char* name, const T& val)
{
    pugi::xml_node child = node.append_child(name);
    append_data(child, val);
    return child;
}

/// Helper function to append a simple <tag>string</tag> node.
YASM_LIB_EXPORT
pugi::xml_node append_child(pugi::xml_node node, const char* name, const char* val);
YASM_LIB_EXPORT
pugi::xml_node append_child(pugi::xml_node node, const char* name, const std::string& val);
YASM_LIB_EXPORT
pugi::xml_node append_child(pugi::xml_node node, const char* name, llvm::StringRef val);

/// Helper functions to append a simple <tag>integer</tag> node.
YASM_LIB_EXPORT
pugi::xml_node append_child(pugi::xml_node node, const char* name, int val);
YASM_LIB_EXPORT
pugi::xml_node append_child(pugi::xml_node node, const char* name, unsigned int val);
YASM_LIB_EXPORT
pugi::xml_node append_child(pugi::xml_node node, const char* name, long val);
YASM_LIB_EXPORT
pugi::xml_node append_child(pugi::xml_node node, const char* name, unsigned long val);
YASM_LIB_EXPORT
pugi::xml_node append_child(pugi::xml_node node, const char* name, bool val);
YASM_LIB_EXPORT
pugi::xml_node append_child(pugi::xml_node node, const char* name, double val);
#endif // WITH_XML

} // yasm namespace

#endif
