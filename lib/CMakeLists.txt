SET (PLUGIN_CPP)
IF (NOT BUILD_STATIC)
    SET (PLUGIN_CPP yasmx/plugin.cpp)
ENDIF (NOT BUILD_STATIC)

YASM_ADD_LIBRARY(libyasmx SHARED
    llvm/Support/Allocator.cpp
    llvm/Support/APFloat.cpp
    llvm/Support/APInt.cpp
    llvm/Support/circular_raw_ostream.cpp
    llvm/Support/CommandLine.cpp
    llvm/Support/Debug.cpp
    llvm/Support/ErrorHandling.cpp
    llvm/Support/FoldingSet.cpp
    llvm/Support/raw_ostream.cpp
    llvm/Support/ManagedStatic.cpp
    llvm/Support/MemoryBuffer.cpp
    llvm/Support/PrettyStackTrace.cpp
    llvm/Support/SmallPtrSet.cpp
    llvm/Support/SmallVector.cpp
    llvm/Support/Statistic.cpp
    llvm/Support/StringExtras.cpp
    llvm/Support/StringMap.cpp
    llvm/Support/StringRef.cpp
    llvm/Support/Timer.cpp
    llvm/Support/Twine.cpp
    llvm/System/Atomic.cpp
    llvm/System/Errno.cpp
    llvm/System/Memory.cpp
    llvm/System/Mutex.cpp
    llvm/System/Path.cpp
    llvm/System/Process.cpp
    llvm/System/Program.cpp
    llvm/System/Signals.cpp
    llvm/System/Threading.cpp
    llvm/System/ThreadLocal.cpp
    llvm/System/TimeValue.cpp
    llvm/System/Valgrind.cpp
    YAML/emitter.cpp
    YAML/emitterstate.cpp
    YAML/emitterutils.cpp
    YAML/ostream.cpp
    YAML/regex.cpp
    YAML/stream.cpp
    yasmx/Basic/Diagnostic.cpp
    yasmx/Basic/FileManager.cpp
    yasmx/Basic/SourceLocation.cpp
    yasmx/Basic/SourceManager.cpp
    yasmx/Parse/HeaderSearch.cpp
    yasmx/Parse/IdentifierTable.cpp
    yasmx/Parse/Lexer.cpp
    yasmx/Parse/NumericParser.cpp
    yasmx/Parse/ParserImpl.cpp
    yasmx/Parse/Preprocessor.cpp
    yasmx/Parse/PPCaching.cpp
    yasmx/Parse/PPLexerChange.cpp
    yasmx/Parse/TokenLexer.cpp
    yasmx/AlignBytecode.cpp
    yasmx/Arch.cpp
    yasmx/Assembler.cpp
    yasmx/AssocData.cpp
    yasmx/BytecodeContainer.cpp
    yasmx/BytecodeOutput.cpp
    yasmx/Bytecode.cpp
    yasmx/Bytes.cpp
    yasmx/Bytes_util.cpp
    yasmx/Bytes_leb128.cpp
    yasmx/Compose.cpp
    yasmx/DataBytecode.cpp
    yasmx/DebugFormat.cpp
    yasmx/Directive.cpp
    yasmx/DirHelpers.cpp
    yasmx/EffAddr.cpp
    yasmx/Expr.cpp
    yasmx/Expr_util.cpp
    yasmx/IncbinBytecode.cpp
    yasmx/InputBuffer.cpp
    yasmx/Insn.cpp
    yasmx/IntNum.cpp
    yasmx/Leb128Bytecode.cpp
    yasmx/ListFormat.cpp
    yasmx/Location.cpp
    yasmx/Location_util.cpp
    yasmx/MD5.cpp
    yasmx/Module.cpp
    yasmx/MultipleBytecode.cpp
    yasmx/NameValue.cpp
    yasmx/NumericOutput.cpp
    yasmx/Object.cpp
    yasmx/Object_optimize.cpp
    yasmx/ObjectFormat.cpp
    yasmx/Object_util.cpp
    yasmx/OrgBytecode.cpp
    yasmx/Op.cpp
    yasmx/Parser.cpp
    yasmx/phash.cpp
    ${PLUGIN_CPP}
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
ADD_DEPENDENCIES(libyasmx DiagnosticIncludes)

IF(NOT BUILD_STATIC)
    INSTALL(TARGETS libyasmx ${INSTALL_TARGETS_DEFAULT_ARGS})
ENDIF(NOT BUILD_STATIC)
