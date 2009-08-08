SET (PLUGIN_CPP)
IF (NOT BUILD_STATIC)
    SET (PLUGIN_CPP yasmx/plugin.cpp)
ENDIF (NOT BUILD_STATIC)

YASM_ADD_LIBRARY(libyasmx SHARED
    Support/APFloat.cpp
    Support/APInt.cpp
    Support/CommandLine.cpp
    Support/Debug.cpp
    Support/ErrorHandling.cpp
    Support/FoldingSet.cpp
    Support/raw_ostream.cpp
    Support/ManagedStatic.cpp
    Support/MemoryBuffer.cpp
    Support/SmallPtrSet.cpp
    Support/Statistic.cpp
    Support/Streams.cpp
    Support/StringExtras.cpp
    Support/StringMap.cpp
    Support/Timer.cpp
    Support/Twine.cpp
    System/Atomic.cpp
    System/Errno.cpp
    System/Mutex.cpp
    System/Path.cpp
    System/Process.cpp
    System/Program.cpp
    System/Threading.cpp
    System/TimeValue.cpp
    YAML/emitter.cpp
    YAML/emitterstate.cpp
    YAML/emitterutils.cpp
    YAML/ostream.cpp
    YAML/regex.cpp
    YAML/stream.cpp
    yasmx/AlignBytecode.cpp
    yasmx/Arch.cpp
    yasmx/Assembler.cpp
    yasmx/AssocData.cpp
    yasmx/BytecodeContainer.cpp
    yasmx/BytecodeContainer_util.cpp
    yasmx/BytecodeOutput.cpp
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
    yasmx/InputBuffer.cpp
    yasmx/Insn.cpp
    yasmx/IntNum.cpp
    yasmx/Leb128Bytecode.cpp
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
    yasmx/Object_optimize.cpp
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
    yasmx/StringExtras.cpp
    yasmx/StringTable.cpp
    yasmx/Symbol.cpp
    yasmx/Symbol_util.cpp
    yasmx/SymbolRef.cpp
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
