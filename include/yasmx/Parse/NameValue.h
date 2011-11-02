#ifndef YASM_NAMEVALUE_H
#define YASM_NAMEVALUE_H
///
/// @file
/// @brief Name/value interface.
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

#include "llvm/ADT/StringRef.h"
#include "yasmx/Basic/SourceLocation.h"
#include "yasmx/Config/export.h"
#include "yasmx/Parse/Token.h"
#include "yasmx/Support/ptr_vector.h"
#include "yasmx/DebugDumper.h"
#include "yasmx/Expr.h"


namespace yasm
{

class Object;

/// Name/value pair.
class YASM_LIB_EXPORT NameValue
{
public:
    /// Identifier value constructor.
    /// @param name         name; may be empty string if no name
    /// @param id           identifier value
    /// @param id_prefix    identifier prefix for raw identifiers
    NameValue(llvm::StringRef name, llvm::StringRef id, char id_prefix);

    /// String value constructor.
    /// @param name         name; may be empty string if no name
    /// @param str          string value
    NameValue(llvm::StringRef name, llvm::StringRef str);

    /// Expression value constructor.
    /// @param name         name; may be empty string if no name
    /// @param e            expression
    NameValue(llvm::StringRef name, std::auto_ptr<Expr> e);

    /// Identifier value constructor with no name.
    /// @param id           identifier value
    /// @param id_prefix    identifier prefix for raw identifiers
    NameValue(llvm::StringRef id, char id_prefix);

    /// String value constructor with no name.
    /// @param name         name; may be empty string if no name
    /// @param str          string value
    explicit NameValue(llvm::StringRef str);

    /// Expression value constructor with no name.
    /// @param e            expression
    explicit NameValue(std::auto_ptr<Expr> e);

    /// Token value constructor (no name assumed).
    /// @param token        token
    explicit NameValue(const Token& token);

    NameValue(const NameValue& oth);
    NameValue& operator= (const NameValue& rhs)
    {
        if (this != &rhs)
            NameValue(rhs).swap(*this);
        return *this;
    }

    /// Destructor.
    ~NameValue();

    // Exchanges this name/value with another one.
    void swap(NameValue& oth);

    /// Determine if name present.
    /// @return True if name present.
    bool hasName() const { return !m_name.empty(); }

    /// Get name.
    /// @return Name; empty string if no name.
    llvm::StringRef getName() const { return m_name; }

    /// Determine if value is convertible to an expression using getExpr().
    /// @return True if convertible.
    bool isExpr() const { return (m_type == ID) || (m_type == EXPR); }

    /// Determine if value is convertible to a register using getRegister().
    /// @return True if convertible.
    bool isRegister() const { return m_type == EXPR && m_expr->isRegister(); }

    /// Determine if value is convertible to a string using getString().
    /// @return True if convertible.
    bool isString() const { return (m_type == ID) || (m_type == STRING); }

    /// Determine if value is convertible to an identifier using getId().
    /// @return True if convertible.
    bool isId() const { return m_type == ID; }

    /// Determine if value is a token.
    /// @return True if token.
    bool isToken() const { return m_type == TOKEN; }

    /// Get value as an expr.  If the parameter is an identifier,
    /// it's treated as a symbol (Symbol::use() is called to convert it).
    /// @param object       object
    /// @param source       source location
    /// @return Expression; asserts if the parameter cannot be converted to
    ///         an expression.
    Expr getExpr(Object& object) const;

    /// Release value's expression.  Operates just like get_expr() but moves
    /// the expression instead of copying it.
    /// @param object       object
    /// @param source       source location
    /// @return Expression, or NULL if the parameter cannot be
    ///         converted to an expression.
    /*@null@*/ std::auto_ptr<Expr> ReleaseExpr(Object& object);

    /// Get value as a register.
    /// @return Register; asserts if the parameter is not a register.
    const Register* getRegister() const;

    /// Get value as a string.  If the parameter is an identifier,
    /// it's treated as a string.
    /// @return String; asserts if the parameter cannot be realized as a
    ///         string.
    llvm::StringRef getString() const;

    /// Get value as an identifier.
    /// @return Identifier (string); asserts if the parameter is not an
    ///         identifier.
    llvm::StringRef getId() const;

    const Token& getToken() const;

#ifdef WITH_XML
    /// Write an XML representation.  For debugging purposes.
    /// @param out          XML node
    /// @return Root node.
    pugi::xml_node Write(pugi::xml_node out) const;
#endif // WITH_XML

    void setNameSource(SourceLocation loc) { m_name_source = loc; }
    void setEqualsSource(SourceLocation loc) { m_equals_source = loc; }
    void setValueRange(SourceRange range) { m_value_range = range; }

    SourceLocation getNameSource() const
    {
        return m_name_source.isValid() ?
            m_name_source : m_value_range.getBegin();
    }
    SourceLocation getEqualsSource() const
    {
        return m_equals_source.isValid() ?
            m_equals_source : m_value_range.getBegin();
    }
    SourceRange getValueRange() const { return m_value_range; }
private:
    std::string m_name; ///< Name (empty string if no name)

    /// Value type.
    enum Type
    {
        ID,             ///< Identifier
        STRING,         ///< String
        EXPR,           ///< Expression
        TOKEN           ///< Token
    };

    Type m_type;

    /// Possible values
    std::string m_idstr;            ///< Identifier or string
    std::auto_ptr<Expr> m_expr;     ///< Expression
    Token m_token;                  ///< Token

    /// Prefix character that indicates a raw identifier.  When
    /// get_string() is called on a #ID, all characters are
    /// returned.  When get_id() is called on a #ID, if the
    /// identifier begins with this character, this character is stripped
    /// from the returned value.
    char m_id_prefix;

    SourceLocation m_name_source;   ///< Name source
    SourceLocation m_equals_source; ///< Source for equals sign
    SourceRange m_value_range;      ///< Value source
};

/// Vector of name/values.
class YASM_LIB_EXPORT NameValues : private stdx::ptr_vector<NameValue>
{
    typedef stdx::ptr_vector<NameValue> base_vector;

public:
    NameValues() {}
    NameValues(iterator first, iterator last) : base_vector(first, last) {}
    ~NameValues();

    typedef base_vector::iterator iterator;
    typedef base_vector::const_iterator const_iterator;
    typedef base_vector::reverse_iterator reverse_iterator;
    typedef base_vector::const_reverse_iterator const_reverse_iterator;

    using base_vector::begin;
    using base_vector::end;
    using base_vector::rbegin;
    using base_vector::rend;
    using base_vector::size;
    using base_vector::max_size;
    using base_vector::empty;
    using base_vector::capacity;
    using base_vector::reserve;
    using base_vector::operator[];
    using base_vector::at;
    using base_vector::front;
    using base_vector::back;
    using base_vector::push_back;
    using base_vector::insert;

    // Exchanges this vector with another one.
    void swap(NameValues& oth) { base_vector::swap(oth); }

#ifdef WITH_XML
    /// Write an XML representation.  For debugging purposes.
    /// @param out          XML node
    /// @return Root node.
    pugi::xml_node Write(pugi::xml_node out) const;
#endif // WITH_XML
};

/// Specialized swap for algorithms.
inline void
swap(NameValue& left, NameValue& right)
{
    left.swap(right);
}

inline void
swap(NameValues& left, NameValues& right)
{
    left.swap(right);
}

} // namespace yasm

namespace std
{

// Specialized std::swap.
template <>
inline void
swap(yasm::NameValue& left, yasm::NameValue& right)
{
    left.swap(right);
}

template <>
inline void
swap(yasm::NameValues& left, yasm::NameValues& right)
{
    left.swap(right);
}

} // namespace std

#endif
