#ifndef YAML_EMITTERUTILS_H
#define YAML_EMITTERUTILS_H

#include "YAML/ostream.h"
#include <string>

namespace llvm { class StringRef; class Twine; }

namespace YAML
{
	namespace Utils
	{
		bool WriteString(ostream& out, const llvm::StringRef& str, bool inFlow);
		bool WriteSingleQuotedString(ostream& out, const llvm::StringRef& str);
		bool WriteDoubleQuotedString(ostream& out, const llvm::StringRef& str);
		bool WriteLiteralString(ostream& out, const llvm::StringRef& str, int indent);
		bool WriteComment(ostream& out, const llvm::StringRef& str, int postCommentIndent);
		bool WriteAlias(ostream& out, const llvm::Twine& str);
		bool WriteAnchor(ostream& out, const llvm::Twine& str);
	}
}

#endif
