ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/nasm-token.cpp
    COMMAND re2c -b
            -o ${CMAKE_CURRENT_BINARY_DIR}/nasm-token.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/nasm-token.re
    MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/nasm-token.re
    )

YASM_ADD_MODULE(parser_nasm
    nasm-parse.cpp
    nasm-parser.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/nasm-token.cpp
    )
