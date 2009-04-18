SET (PLUGIN_CPP)
IF (NOT BUILD_STATIC)
    SET (PLUGIN_CPP yasmx/plugin.cpp)
ENDIF (NOT BUILD_STATIC)

YASM_ADD_LIBRARY(libyasmx SHARED
    Support/APFloat.cpp
    Support/APInt.cpp
    Support/Debug.cpp
    Support/FoldingSet.cpp
    Support/raw_ostream.cpp
    Support/Streams.cpp
    Support/StringExtras.cpp
    yasmx/AlignBytecode.cpp
    yasmx/Arch.cpp
    yasmx/Assembler.cpp
    yasmx/AssocData.cpp
    yasmx/BytecodeContainer.cpp
    yasmx/BytecodeContainer_util.cpp
    yasmx/BytecodeOutput.cpp
    yasmx/BitVector.cpp
    yasmx/Bytecode.cpp
    yasmx/Bytecode_util.cpp
    yasmx/Bytes.cpp
    yasmx/Bytes_util.cpp
    yasmx/Bytes_leb128.cpp
    yasmx/Compose.cpp
    yasmx/DebugFormat.cpp
    yasmx/Directive.cpp
    yasmx/DirHelpers.cpp
    yasmx/EffAddr.cpp
    yasmx/errwarn.cpp
    yasmx/Errwarns.cpp
    yasmx/Expr.cpp
    yasmx/Expr_util.cpp
    yasmx/file.cpp
    yasmx/IncbinBytecode.cpp
    yasmx/Includes.cpp
    yasmx/Insn.cpp
    yasmx/IntNum.cpp
    yasmx/Linemap.cpp
    yasmx/ListFormat.cpp
    yasmx/Location.cpp
    yasmx/Location_util.cpp
    yasmx/MD5.cpp
    yasmx/Module.cpp
    yasmx/MultipleBytecode.cpp
    yasmx/NameValue.cpp
    yasmx/nocase.cpp
    yasmx/Object.cpp
    yasmx/ObjectFormat.cpp
    yasmx/Object_util.cpp
    yasmx/OrgBytecode.cpp
    yasmx/Op.cpp
    yasmx/Parser.cpp
    yasmx/phash.cpp
    ${PLUGIN_CPP}
    yasmx/Preprocessor.cpp
    yasmx/registry.cpp
    yasmx/Reloc.cpp
    yasmx/Section.cpp
    yasmx/StringTable.cpp
    yasmx/Symbol.cpp
    yasmx/Symbol_util.cpp
    yasmx/Value.cpp
    )
SET_TARGET_PROPERTIES(libyasmx PROPERTIES
    OUTPUT_NAME "yasmx"
    )
IF(NOT BUILD_STATIC)
    TARGET_LINK_LIBRARIES(libyasmx ${LIBDL})
    SET_TARGET_PROPERTIES(libyasmx PROPERTIES
	VERSION "0.0.0"
	SOVERSION 0
	)
ENDIF(NOT BUILD_STATIC)

IF(NOT BUILD_STATIC)
    INSTALL(TARGETS libyasmx ${INSTALL_TARGETS_DEFAULT_ARGS})
ENDIF(NOT BUILD_STATIC)

ADD_SUBDIRECTORY(yasmx)
