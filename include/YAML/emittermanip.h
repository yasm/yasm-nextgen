#ifndef YAML_EMITTERMANIP_H
#define YAML_EMITTERMANIP_H

#include "llvm/ADT/StringRef.h"

namespace llvm { class Twine; }

namespace YAML
{
	enum EMITTER_MANIP {
		// general manipulators
		Auto,
		
		// string manipulators
		// Auto, // duplicate
		SingleQuoted,
		DoubleQuoted,
		Literal,
		Null,
		
		// bool manipulators
		YesNoBool,  // yes, no
		TrueFalseBool,  // true, false
		OnOffBool,  // on, off
		UpperCase,  // TRUE, N
		LowerCase,  // f, yes
		CamelCase,  // No, Off
		LongBool,  // yes, On
		ShortBool,  // y, t
		
		// int manipulators
		Dec,
		Hex,
		Oct,
		
		// sequence manipulators
		BeginSeq,
		EndSeq,
		Flow,
		Block,
		
		// map manipulators
		BeginMap,
		EndMap,
		Key,
		Value,
		// Flow, // duplicate
		// Block, // duplicate
		// Auto, // duplicate
		LongKey
	};
	
	struct _Indent {
		_Indent(int value_): value(value_) {}
		int value;
	};
	
	inline _Indent Indent(int value) {
		return _Indent(value);
	}
	
	struct _Alias {
		_Alias(const llvm::Twine& content_): content(content_) {}
		const llvm::Twine& content;
	};
	
	inline _Alias Alias(const llvm::Twine& content) {
		return _Alias(content);
	}
	
	struct _Anchor {
		_Anchor(const llvm::Twine& content_): content(content_) {}
		const llvm::Twine& content;
	};

	inline _Anchor Anchor(const llvm::Twine& content) {
		return _Anchor(content);
	}

	struct _Comment {
		_Comment(llvm::StringRef content_): content(content_) {}
		llvm::StringRef content;
	};
	
	inline _Comment Comment(llvm::StringRef content) {
		return _Comment(content);
	}
}

#endif
