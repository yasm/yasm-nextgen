ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/NasmParser_token.cpp
    COMMAND ${RE2C_EXECUTABLE} -b
            -o ${CMAKE_CURRENT_BINARY_DIR}/NasmParser_token.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/parsers/nasm/NasmParser_token.re
    MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/parsers/nasm/NasmParser_token.re
    )

YASM_ADD_MODULE(parser_nasm
    parsers/nasm/NasmParser_parse.cpp
    parsers/nasm/NasmParser.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/NasmParser_token.cpp
    )
