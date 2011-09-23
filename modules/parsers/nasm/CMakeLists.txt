YASM_ADD_MODULE(parser_nasm
    parsers/nasm/NasmNumericParser.cpp
    parsers/nasm/NasmStringParser.cpp
    parsers/nasm/NasmParser_parse.cpp
    parsers/nasm/NasmParser.cpp
    parsers/nasm/NasmPreproc.cpp
    parsers/nasm/NasmLexer.cpp
    parsers/nasm/nasm-eval.cpp
    parsers/nasm/nasmlib.cpp
    parsers/nasm/nasm-pp.cpp
    )

GET_TARGET_PROPERTY(GENMACRO_EXE genmacro LOCATION)
ADD_CUSTOM_COMMAND(
    OUTPUT
        ${CMAKE_CURRENT_BINARY_DIR}/nasm-macros.i
    COMMAND ${GENMACRO_EXE} ${CMAKE_CURRENT_BINARY_DIR}/nasm-macros.i
           nasm_standard_mac
           ${CMAKE_CURRENT_SOURCE_DIR}/parsers/nasm/nasm-std.mac
    MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/parsers/nasm/nasm-std.mac
    )

SET(nasm_parser_DEPS
    ${CMAKE_CURRENT_BINARY_DIR}/nasm-macros.i
    )

SET_SOURCE_FILES_PROPERTIES(parsers/nasm/NasmParser.cpp PROPERTIES
    OBJECT_DEPENDS "${nasm_parser_DEPS}"
    )
