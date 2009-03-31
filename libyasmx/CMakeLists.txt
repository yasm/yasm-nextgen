SET (PLUGIN_CPP)
IF (NOT BUILD_STATIC)
    SET (PLUGIN_CPP plugin.cpp)
ENDIF (NOT BUILD_STATIC)

YASM_ADD_LIBRARY(libyasmx SHARED
    AlignBytecode.cpp
    Arch.cpp
    Assembler.cpp
    AssocData.cpp
    BytecodeContainer.cpp
    BytecodeContainer_util.cpp
    BytecodeOutput.cpp
    BitVector.cpp
    Bytecode.cpp
    Bytecode_util.cpp
    Bytes.cpp
    Bytes_util.cpp
    Bytes_leb128.cpp
    Compose.cpp
    DebugFormat.cpp
    Directive.cpp
    DirHelpers.cpp
    EffAddr.cpp
    errwarn.cpp
    Errwarns.cpp
    Expr.cpp
    Expr_util.cpp
    file.cpp
    FloatNum.cpp
    IncbinBytecode.cpp
    Insn.cpp
    IntNum.cpp
    Linemap.cpp
    ListFormat.cpp
    Location.cpp
    Location_util.cpp
    MD5.cpp
    Module.cpp
    MultipleBytecode.cpp
    NameValue.cpp
    nocase.cpp
    Object.cpp
    ObjectFormat.cpp
    Object_util.cpp
    OrgBytecode.cpp
    Op.cpp
    Parser.cpp
    phash.cpp
    ${PLUGIN_CPP}
    Preprocessor.cpp
    registry.cpp
    Reloc.cpp
    Section.cpp
    StringTable.cpp
    Symbol.cpp
    Symbol_util.cpp
    Value.cpp
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

    INSTALL(FILES
        Arch.h
        Assembler.h
        AssocData.h
        BytecodeContainer.h
        BytecodeContainer_util.h
        BytecodeOutput.h
        BitVector.h
        Bytecode.h
        Bytecode_util.h
        Bytes.h
        Bytes_util.h
	Bytes_leb128.h
        Compose.h
        DebugFormat.h
        Directive.h
        DirHelpers.h
        EffAddr.h
        errwarn.h
        Errwarns.h
        Expr.h
        Expr_util.h
        FloatNum.h
        Insn.h
        IntNum.h
        IntNum_iomanip.h
        Linemap.h
        ListFormat.h
        Location.h
        Location_util.h
        MD5.h
        Module.h
        NameValue.h
        ObjectFormat.h
        Object.h
        Object_util.h
        Op.h
        Parser.h
        Preprocessor.h
        Reloc.h
        Section.h
        StringTable.h
        Symbol.h
        SymbolRef.h
        Symbol_util.h
        Value.h
        DESTINATION ${INCLUDE_INSTALL_DIR}/libyasmx
        )
ENDIF(NOT BUILD_STATIC)

ADD_SUBDIRECTORY(Config)
ADD_SUBDIRECTORY(Support)
ADD_SUBDIRECTORY(System)
ADD_SUBDIRECTORY(tests)
