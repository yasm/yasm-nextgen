#include "emitterutils.h"
#include "exp.h"
#include "indentation.h"
#include "stringsource.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/raw_ostream.h"

namespace YAML
{
	namespace Utils
	{
		namespace {
			bool IsPrintable(char ch) {
				return (0x20 <= ch && ch <= 0x7E);
			}
			
			bool IsValidPlainScalar(llvm::StringRef str, bool inFlow) {
				// first check the start
				const RegEx& start = (inFlow ? Exp::PlainScalarInFlow : Exp::PlainScalar);
				if(!start.Matches(str))
					return false;
				
				// and check the end for plain whitespace (which can't be faithfully kept in a plain scalar)
				if(!str.empty() && str.back() == ' ')
					return false;

				// then check until something is disallowed
				const RegEx& disallowed = (inFlow ? Exp::EndScalarInFlow : Exp::EndScalar)
				                          || (Exp::BlankOrBreak + Exp::Comment)
				                          || (!Exp::Printable)
				                          || Exp::Break
				                          || Exp::Tab
				                          || Exp::Null;
				StringCharSource buffer(str.data(), str.size());
				while(buffer) {
					if(disallowed.Matches(buffer))
						return false;
					++buffer;
				}
				
				return true;
			}
		}
		
		bool WriteString(ostream& out, llvm::StringRef str, bool inFlow)
		{
			if(IsValidPlainScalar(str, inFlow)) {
				out << str;
				return true;
			} else
				return WriteDoubleQuotedString(out, str);
		}
		
		bool WriteSingleQuotedString(ostream& out, llvm::StringRef str)
		{
			out << "'";
			for(size_t i=0;i<str.size();i++) {
				char ch = str[i];
				if(!IsPrintable(ch))
					return false;
				
				if(ch == '\'')
					out << "''";
				else
					out << ch;
			}
			out << "'";
			return true;
		}
		
		bool WriteDoubleQuotedString(ostream& out, llvm::StringRef str)
		{
			out << "\"";
			for(size_t i=0;i<str.size();i++) {
				char ch = str[i];
				if(IsPrintable(ch)) {
					if(ch == '\"')
						out << "\\\"";
					else if(ch == '\\')
						out << "\\\\";
					else
						out << ch;
				} else {
					// TODO: for the common escaped characters, give their usual symbol
					llvm::SmallString<128> s;
					llvm::raw_svector_ostream ss(s);
					ss << "\\x" << llvm::format("%x", static_cast <int>(ch));
					out << ss.str();
				}
			}
			out << "\"";
			return true;
		}

		bool WriteLiteralString(ostream& out, llvm::StringRef str, int indent)
		{
			out << "|\n";
			out << IndentTo(indent);
			for(size_t i=0;i<str.size();i++) {
				if(str[i] == '\n')
					out << "\n" << IndentTo(indent);
				else
					out << str[i];
			}
			return true;
		}
		
		bool WriteComment(ostream& out, llvm::StringRef str, int postCommentIndent)
		{
			unsigned curIndent = out.col();
			out << "#" << Indentation(postCommentIndent);
			for(size_t i=0;i<str.size();i++) {
				if(str[i] == '\n')
					out << "\n" << IndentTo(curIndent) << "#" << Indentation(postCommentIndent);
				else
					out << str[i];
			}
			return true;
		}

		bool WriteAlias(ostream& out, const llvm::Twine& twine)
		{
			llvm::SmallString<256> str;
			twine.toVector(str);
			out << "*";
			for(unsigned i=0;i<str.size();i++) {
				if(!IsPrintable(str[i]) || str[i] == ' ' || str[i] == '\t' || str[i] == '\n' || str[i] == '\r')
					return false;
				
				out << str[i];
			}
			return true;
		}
		
		bool WriteAnchor(ostream& out, const llvm::Twine& twine)
		{
			llvm::SmallString<256> str;
			twine.toVector(str);
			out << "&";
			for(unsigned i=0;i<str.size();i++) {
				if(!IsPrintable(str[i]) || str[i] == ' ' || str[i] == '\t' || str[i] == '\n' || str[i] == '\r')
					return false;
				
				out << str[i];
			}
			return true;
		}
	}
}

