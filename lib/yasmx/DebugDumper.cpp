//
// Debug dump implementation.
//
//  Copyright (C) 2011  Peter Johnson
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
#include "yasmx/DebugDumper.h"

#include "llvm/ADT/SmallString.h"


using namespace yasm;

xml_writer_raw_ostream::~xml_writer_raw_ostream()
{
}

void
xml_writer_raw_ostream::write(const void* data, size_t size)
{
    m_os.write(static_cast<const char*>(data), size);
}

pugi::xml_node
yasm::append_data(pugi::xml_node node, const char* val)
{
    pugi::xml_node data = node.append_child(pugi::node_pcdata);
    data.set_value(val);
    return data;
}

pugi::xml_node
yasm::append_data(pugi::xml_node node, const std::string& val)
{
    pugi::xml_node data = node.append_child(pugi::node_pcdata);
    data.set_value(val.c_str());
    return data;
}

pugi::xml_node
yasm::append_data(pugi::xml_node node, StringRef val)
{
    pugi::xml_node data = node.append_child(pugi::node_pcdata);
    if (val.back() == '\0')
        data.set_value(val.data());
    else
        data.set_value(val.str().c_str());
    return data;
}

pugi::xml_node
yasm::append_data(pugi::xml_node node, int val)
{
    SmallString<128> ss;
    llvm::raw_svector_ostream oss(ss);
    oss << val << '\0';
    return append_data(node, oss.str().data());
}

pugi::xml_node
yasm::append_data(pugi::xml_node node, unsigned int val)
{
    SmallString<128> ss;
    llvm::raw_svector_ostream oss(ss);
    oss << val << '\0';
    return append_data(node, oss.str().data());
}

pugi::xml_node
yasm::append_data(pugi::xml_node node, long val)
{
    SmallString<128> ss;
    llvm::raw_svector_ostream oss(ss);
    oss << val << '\0';
    return append_data(node, oss.str().data());
}

pugi::xml_node
yasm::append_data(pugi::xml_node node, unsigned long val)
{
    SmallString<128> ss;
    llvm::raw_svector_ostream oss(ss);
    oss << val << '\0';
    return append_data(node, oss.str().data());
}

pugi::xml_node
yasm::append_data(pugi::xml_node node, bool val)
{
    if (val)
        return append_data(node, "true");
    else
        return append_data(node, "false");
}

pugi::xml_node
yasm::append_data(pugi::xml_node node, double val)
{
    SmallString<128> ss;
    llvm::raw_svector_ostream oss(ss);
    oss << val << '\0';
    return append_data(node, oss.str().data());
}

pugi::xml_node
yasm::append_child(pugi::xml_node node, const char* name, const char* val)
{
    pugi::xml_node child = node.append_child(name);
    append_data(child, val);
    return child;
}

pugi::xml_node
yasm::append_child(pugi::xml_node node, const char* name, const std::string& val)
{
    pugi::xml_node child = node.append_child(name);
    append_data(child, val);
    return child;
}

pugi::xml_node
yasm::append_child(pugi::xml_node node, const char* name, StringRef val)
{
    pugi::xml_node child = node.append_child(name);
    append_data(child, val);
    return child;
}

pugi::xml_node
yasm::append_child(pugi::xml_node node, const char* name, int val)
{
    pugi::xml_node child = node.append_child(name);
    append_data(child, val);
    return child;
}

pugi::xml_node
yasm::append_child(pugi::xml_node node, const char* name, unsigned int val)
{
    pugi::xml_node child = node.append_child(name);
    append_data(child, val);
    return child;
}

pugi::xml_node
yasm::append_child(pugi::xml_node node, const char* name, long val)
{
    pugi::xml_node child = node.append_child(name);
    append_data(child, val);
    return child;
}

pugi::xml_node
yasm::append_child(pugi::xml_node node, const char* name, unsigned long val)
{
    pugi::xml_node child = node.append_child(name);
    append_data(child, val);
    return child;
}

pugi::xml_node
yasm::append_child(pugi::xml_node node, const char* name, bool val)
{
    pugi::xml_node child = node.append_child(name);
    append_data(child, val);
    return child;
}

pugi::xml_node
yasm::append_child(pugi::xml_node node, const char* name, double val)
{
    pugi::xml_node child = node.append_child(name);
    append_data(child, val);
    return child;
}
