SET (PLUGIN_CPP)
IF (NOT BUILD_STATIC)
    SET (PLUGIN_CPP plugin.cpp)
ENDIF (NOT BUILD_STATIC)

YASM_ADD_LIBRARY(libyasmx SHARED
    arch.cpp
    assembler.cpp
    assoc_data.cpp
    bc_container.cpp
    bc-align.cpp
    bc-data.cpp
    bc-incbin.cpp
    bc-multiple.cpp
    bc-org.cpp
    bitvect.cpp
    bytecode.cpp
    bytecode_util.cpp
    bytes.cpp
    compose.cpp
    debug_format.cpp
    directive.cpp
    effaddr.cpp
    errwarn.cpp
    errwarns.cpp
    expr.cpp
    expr_util.cpp
    file.cpp
    floatnum.cpp
    insn.cpp
    intnum.cpp
    linemap.cpp
    list_format.cpp
    location.cpp
    location_util.cpp
    md5.cpp
    module.cpp
    name_value.cpp
    nocase.cpp
    object.cpp
    object_format.cpp
    operator.cpp
    parser.cpp
    phash.cpp
    ${PLUGIN_CPP}
    preproc.cpp
    registry.cpp
    section.cpp
    symbol.cpp
    value.cpp
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
        arch.h
        assembler.h
        assoc_data.h
        bc_container.h
        bc_container_util.h
        bitcount.h
        bitvect.h
        bytecode.h
        bytecode_util.h
        bytes.h
        compose.h
        debug_format.h
        directive.h
        effaddr.h
        errwarn.h
        errwarns.h
        ${CMAKE_CURRENT_BINARY_DIR}/export.h
        expr.h
        expr_util.h
        file.h
        floatnum.h
        ${CMAKE_CURRENT_BINARY_DIR}/functional.h
        hamt.h
        insn.h
        interval_tree.h
        intnum.h
        linemap.h
        list_format.h
        location.h
        location_util.h
        marg_ostream_fwd.h
        marg_ostream.h
        md5.h
        module.h
        name_value.h
        nocase.h
        object_format.h
        object.h
        operator.h
        parser.h
        phash.h
        plugin.h
        preproc.h
        ptr_vector.h
        registry.h
        section.h
        symbol.h
        value.h
        DESTINATION ${INCLUDE_INSTALL_DIR}/libyasmx
        )
ENDIF(NOT BUILD_STATIC)

ADD_SUBDIRECTORY(tests)
