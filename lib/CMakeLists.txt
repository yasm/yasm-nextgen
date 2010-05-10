SET (PLUGIN_CPP)
IF (NOT BUILD_STATIC)
    SET (PLUGIN_CPP yasmx/plugin.cpp)
ENDIF (NOT BUILD_STATIC)

YASM_ADD_LIBRARY(libyasmx SHARED
    Basic/FileManager.cpp
    Basic/SourceLocation.cpp
    Basic/SourceManager.cpp
    Support/Allocator.cpp
    Support/APFloat.cpp
    Support/APInt.cpp
    Support/circular_raw_ostream.cpp
    Support/CommandLine.cpp
    Support/Debug.cpp
    Support/ErrorHandling.cpp
    Support/FoldingSet.cpp
    Support/raw_ostream.cpp
    Support/ManagedStatic.cpp
    Support/MemoryBuffer.cpp
    Support/PrettyStackTrace.cpp
    Support/SmallPtrSet.cpp
    Support/SmallVector.cpp
    Support/Statistic.cpp
    Support/StringExtras.cpp
    Support/StringMap.cpp
    Support/StringRef.cpp
    Support/Timer.cpp
    Support/Twine.cpp
    System/Atomic.cpp
    System/Errno.cpp
    System/Memory.cpp
    System/Mutex.cpp
    System/Path.cpp
    System/Process.cpp
    System/Program.cpp
    System/Signals.cpp
    System/Threading.cpp
    System/ThreadLocal.cpp
    System/TimeValue.cpp
    System/Valgrind.cpp
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
    yasmx/Bytes.cpp
    yasmx/Bytes_util.cpp
    yasmx/Bytes_leb128.cpp
    yasmx/Compose.cpp
    yasmx/DebugFormat.cpp
    yasmx/Diagnostic.cpp
    yasmx/Directive.cpp
    yasmx/DirHelpers.cpp
    yasmx/EffAddr.cpp
    yasmx/errwarn.cpp
    yasmx/Errwarns.cpp
    yasmx/Expr.cpp
    yasmx/Expr_util.cpp
    yasmx/file.cpp
    yasmx/HeaderSearch.cpp
    yasmx/IdentifierTable.cpp
    yasmx/IncbinBytecode.cpp
    yasmx/InputBuffer.cpp
    yasmx/Insn.cpp
    yasmx/IntNum.cpp
    yasmx/Leb128Bytecode.cpp
    yasmx/Lexer.cpp
    yasmx/ListFormat.cpp
    yasmx/Location.cpp
    yasmx/Location_util.cpp
    yasmx/MD5.cpp
    yasmx/Module.cpp
    yasmx/MultipleBytecode.cpp
    yasmx/NameValue.cpp
    yasmx/Object.cpp
    yasmx/Object_optimize.cpp
    yasmx/ObjectFormat.cpp
    yasmx/Object_util.cpp
    yasmx/OrgBytecode.cpp
    yasmx/Op.cpp
    yasmx/Parser.cpp
    yasmx/ParserImpl.cpp
    yasmx/phash.cpp
    ${PLUGIN_CPP}
    yasmx/Preprocessor.cpp
    yasmx/PPCaching.cpp
    yasmx/PPLexerChange.cpp
    yasmx/registry.cpp
    yasmx/Reloc.cpp
    yasmx/Section.cpp
    yasmx/StringExtras.cpp
    yasmx/StringTable.cpp
    yasmx/Symbol.cpp
    yasmx/Symbol_util.cpp
    yasmx/SymbolRef.cpp
    yasmx/TokenLexer.cpp
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
ADD_DEPENDENCIES(libyasmx DiagnosticIncludes)

IF(NOT BUILD_STATIC)
    INSTALL(TARGETS libyasmx ${INSTALL_TARGETS_DEFAULT_ARGS})
ENDIF(NOT BUILD_STATIC)
