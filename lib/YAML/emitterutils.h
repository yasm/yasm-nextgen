#ifndef YAML_EMITTERUTILS_H
#define YAML_EMITTERUTILS_H

#include "YAML/ostream.h"
#include <string>

#include "llvm/ADT/StringRef.h"

namespace llvm { class Twine; }

namespace YAML
{
	namespace Utils
	{
		bool WriteString(ostream& out, llvm::StringRef str, bool inFlow);
		bool WriteSingleQuotedString(ostream& out, llvm::StringRef str);
		bool WriteDoubleQuotedString(ostream& out, llvm::StringRef str);
		bool WriteLiteralString(ostream& out, llvm::StringRef str, int indent);
		bool WriteComment(ostream& out, llvm::StringRef str, int postCommentIndent);
		bool WriteAlias(ostream& out, const llvm::Twine& str);
		bool WriteAnchor(ostream& out, const llvm::Twine& str);
	}
}

#endif
