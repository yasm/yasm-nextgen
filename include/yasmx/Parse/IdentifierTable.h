#ifndef YASM_PARSE_IDENTIFIERTABLE_H
#define YASM_PARSE_IDENTIFIERTABLE_H
//
// IdentifierInfo and IdentifierTable interface.
//
// Based on the LLVM Compiler Infrastructure
// (distributed under the University of Illinois Open Source License.
// See Copying/LLVM.txt for details).
//
// Modifications copyright (C) 2009  Peter Johnson
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
#include <cassert>
#include <string>

#include "llvm/ADT/StringMap.h"
#include "llvm/Support/PointerLikeTypeTraits.h"
#include "yasmx/Config/export.h"
#include "yasmx/Parse/Token.h"
#include "yasmx/Arch.h"
#include "yasmx/SymbolRef.h"


namespace yasm
{

class IdentifierTable;

class YASM_LIB_EXPORT IdentifierInfo
{
    enum Flags
    {
        DID_INSN_LOOKUP = 0x0001,   // Set if DoInsnLookup() done.
        DID_REG_LOOKUP  = 0x0002,   // Set if DoRegLookup() done.

        // Only one of the below flags can be set at a time; the bit set
        // determines what kind of object m_info points to.

        // These bits are only valid if DID_INSN_LOOKUP is set.
        IS_INSN         = 0x0004,   // Set if identifier is an instruction.
        IS_PREFIX       = 0x0008,   // Set if identifier is a prefix.

        // These are only valid if DID_REG_LOOKUP is set.
        IS_REGISTER     = 0x0010,   // Set if identifier is a register.
        IS_REGGROUP     = 0x0020,   // Set if identifier is a register group.
        IS_SEGREG       = 0x0040,   // Set if identifier is a segment register.
        IS_TARGETMOD    = 0x0080,   // Set if identifier is a target modifier.

        // Others
        IS_CUSTOM       = 0x0100    // Set if identifier is something custom.
    };

    SymbolRef m_sym;    // Symbol reference (may be 0 if not a symbol).
    void* m_info;       // Pointer to appropriate data based on flags.

    llvm::StringMapEntry<IdentifierInfo*>* m_entry;

    unsigned int m_token_id : 16;   // Front-end token ID.
    unsigned int m_flags    : 16;   // Flags

    IdentifierInfo(const IdentifierInfo&);  // NONCOPYABLE.
    void operator=(const IdentifierInfo&);  // NONASSIGNABLE.

    friend class IdentifierTable;

public:
    IdentifierInfo() : m_info(0), m_token_id(Token::unknown), m_flags(0) {}

    /// Return true if this is the identifier for the specified string.
    /// This is intended to be used for string literals only: ii->is_str("foo").
    template <std::size_t StrLen>
    bool isStr(const char (&str)[StrLen]) const
    {
        return getLength() == StrLen-1 && !memcmp(getNameStart(), str, StrLen-1);
    }

    /// Return the actual string for this identifier.  The returned 
    /// string is properly null terminated.
    const char* getNameStart() const
    {
        return m_entry->getKeyData();
    }

    /// Efficiently return the length of this identifier info.
    unsigned int getLength() const
    {
        return m_entry->getKeyLength();
    }

    /// getName - Return the actual identifier string.
    llvm::StringRef getName() const
    {
        return llvm::StringRef(getNameStart(), getLength());
    }

    /// If this is a source-language keyword, this API
    /// can be used to cause the lexer to map identifiers to source-language
    /// tokens.
    unsigned int getTokenKind() const { return m_token_id; }
    void setTokenKind(unsigned int id) { m_token_id = id; }

    // Perform lookup of instruction/register data.
    void DoInsnLookup(const Arch& arch,
                      SourceLocation source,
                      DiagnosticsEngine& diags);
    void DoRegLookup(const Arch& arch,
                     SourceLocation source,
                     DiagnosticsEngine& diags);

    bool isUnknown()
    {
        if ((m_flags & (IS_INSN | IS_PREFIX | IS_REGISTER | IS_REGGROUP |
                        IS_SEGREG | IS_TARGETMOD | IS_CUSTOM)) != 0 ||
            m_sym != 0)
            return false;
        return true;
    }

    const Arch::InsnInfo* getInsn()
    {
        assert((m_flags & DID_INSN_LOOKUP) != 0 &&
               "instruction lookup not done");
        if (!(m_flags & IS_INSN))
            return 0;
        return static_cast<const Arch::InsnInfo*>(m_info);
    }

    const Prefix* getPrefix()
    {
        assert((m_flags & DID_INSN_LOOKUP) != 0 &&
               "instruction lookup not done");
        if (!(m_flags & IS_PREFIX))
            return 0;
        return static_cast<const Prefix*>(m_info);
    }

    const Register* getRegister()
    {
        assert((m_flags & DID_REG_LOOKUP) != 0 && "register lookup not done");
        if (!(m_flags & IS_REGISTER))
            return 0;
        return static_cast<const Register*>(m_info);
    }

    const RegisterGroup* getRegGroup()
    {
        assert((m_flags & DID_REG_LOOKUP) != 0 && "register lookup not done");
        if (!(m_flags & IS_REGGROUP))
            return 0;
        return static_cast<const RegisterGroup*>(m_info);
    }

    const SegmentRegister* getSegReg()
    {
        assert((m_flags & DID_REG_LOOKUP) != 0 && "register lookup not done");
        if (!(m_flags & IS_SEGREG))
            return 0;
        return static_cast<const SegmentRegister*>(m_info);
    }

    const TargetModifier* getTargetModifier()
    {
        assert((m_flags & DID_REG_LOOKUP) != 0 && "register lookup not done");
        if (!(m_flags & IS_TARGETMOD))
            return 0;
        return static_cast<const TargetModifier*>(m_info);
    }

    // symbol interface
    bool isSymbol() const { return m_sym != 0; }
    SymbolRef getSymbol() const { return m_sym; }
    void setSymbol(SymbolRef ref) { m_sym = ref; }

    // custom data interface
    template<typename T>
    T* getCustom() const
    {
        if (!(m_flags & IS_CUSTOM))
            return 0;
        return static_cast<T*>(m_info);
    }
    template<typename T>
    void setCustom(T* d)
    {
        m_flags = IS_CUSTOM | DID_INSN_LOOKUP | DID_REG_LOOKUP;
        m_info = const_cast<void*>(reinterpret_cast<const void*>(d));
    }
};

/// IdentifierTable - This table implements an efficient mapping from strings to
/// IdentifierInfo nodes.  It has no other purpose, but this is an
/// extremely performance-critical piece of the code, as each occurrance of
/// every identifier goes through here when lexed.
class IdentifierTable
{
    // Shark shows that using MallocAllocator is *much* slower than using this
    // BumpPtrAllocator!
    typedef llvm::StringMap<IdentifierInfo*, llvm::BumpPtrAllocator> HashTable;
    HashTable m_hash_table;

public:
    llvm::BumpPtrAllocator& getAllocator()
    {
        return m_hash_table.getAllocator();
    }
  
    /// Return the identifier token info for the specified named identifier.
    IdentifierInfo& get(llvm::StringRef name)
    {
        llvm::StringMapEntry<IdentifierInfo*>& entry =
            m_hash_table.GetOrCreateValue(name);
    
        IdentifierInfo* ii = entry.getValue();
        if (ii) return *ii;
    
        // Lookup failed, make a new IdentifierInfo.
        void* mem = getAllocator().Allocate<IdentifierInfo>();
        ii = new (mem) IdentifierInfo();
        entry.setValue(ii);

        // Make sure get_name() knows how to find the IdentifierInfo
        // contents.
        ii->m_entry = &entry;

        return *ii;
    }

    typedef HashTable::const_iterator iterator;
    typedef HashTable::const_iterator const_iterator;
  
    iterator begin() const      { return m_hash_table.begin(); }
    iterator end() const        { return m_hash_table.end(); }
    unsigned int size() const   { return m_hash_table.size(); }
    void clear()                { m_hash_table.clear(); }
};

} // namespace yasm

namespace llvm
{

// Provide PointerLikeTypeTraits for IdentifierInfo pointers, which
// are not guaranteed to be 8-byte aligned.
template<>
class PointerLikeTypeTraits<yasm::IdentifierInfo*> {
public:
  static inline void *getAsVoidPointer(yasm::IdentifierInfo* P) {
    return P; 
  }
  static inline yasm::IdentifierInfo *getFromVoidPointer(void *P) {
    return static_cast<yasm::IdentifierInfo*>(P);
  }
  enum { NumLowBitsAvailable = 1 };
};

template<>
class PointerLikeTypeTraits<const yasm::IdentifierInfo*> {
public:
  static inline const void *getAsVoidPointer(const yasm::IdentifierInfo* P) {
    return P; 
  }
  static inline const yasm::IdentifierInfo *getFromVoidPointer(const void *P) {
    return static_cast<const yasm::IdentifierInfo*>(P);
  }
  enum { NumLowBitsAvailable = 1 };
};

} // namespace llvm

#endif
