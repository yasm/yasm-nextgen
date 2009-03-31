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
    leb128.cpp
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

CONFIGURE_FILE(export.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/export.h)

INCLUDE(YasmCXXTR1)

IF(NOT BUILD_STATIC)
    INSTALL(TARGETS libyasmx ${INSTALL_TARGETS_DEFAULT_ARGS})

    INSTALL(FILES
        Arch.h
        Assembler.h
        AssocData.h
        BytecodeContainer.h
        BytecodeContainer_util.h
        BytecodeOutput.h
        bitcount.h
        BitVector.h
        Bytecode.h
        Bytecode_util.h
        Bytes.h
        Bytes_util.h
        Compose.h
        DebugFormat.h
        Directive.h
        DirHelpers.h
        EffAddr.h
        errwarn.h
        Errwarns.h
        ${CMAKE_CURRENT_BINARY_DIR}/export.h
        Expr.h
        Expr_util.h
        file.h
        FloatNum.h
        ${CMAKE_CURRENT_BINARY_DIR}/functional.h
        #hamt.h         # Don't install; only included by Object.cpp.
        Insn.h
        IntervalTree.h
        IntNum.h
        IntNum_iomanip.h
	leb128.h
        Linemap.h
        ListFormat.h
        Location.h
        Location_util.h
        marg_ostream_fwd.h
        marg_ostream.h
        MD5.h
        Module.h
        NameValue.h
        nocase.h
        ObjectFormat.h
        Object.h
        Object_util.h
        Op.h
        Parser.h
        phash.h
        plugin.h
        Preprocessor.h
        ptr_vector.h
        registry.h
        Reloc.h
        scoped_array.h
        scoped_ptr.h
        Section.h
        StringTable.h
        Symbol.h
        SymbolRef.h
        Symbol_util.h
        Value.h
        DESTINATION ${INCLUDE_INSTALL_DIR}/libyasmx
        )
ENDIF(NOT BUILD_STATIC)

ADD_SUBDIRECTORY(tests)
