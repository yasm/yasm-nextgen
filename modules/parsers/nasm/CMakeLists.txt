ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/nasm-token.cpp
    COMMAND ${RE2C_EXECUTABLE} -b
            -o ${CMAKE_CURRENT_BINARY_DIR}/nasm-token.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/parsers/nasm/nasm-token.re
    MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/parsers/nasm/nasm-token.re
    )

YASM_ADD_MODULE(parser_nasm
    parsers/nasm/nasm-parse.cpp
    parsers/nasm/nasm-parser.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/nasm-token.cpp
    )
