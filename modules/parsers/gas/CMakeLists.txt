ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/GasParser_token.cpp
    COMMAND ${RE2C_EXECUTABLE} -b
            -o ${CMAKE_CURRENT_BINARY_DIR}/GasParser_token.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/parsers/gas/GasParser_token.re
    MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/parsers/gas/GasParser_token.re
    )

YASM_ADD_MODULE(parser_gas
    parsers/gas/GasParser_parse.cpp
    parsers/gas/GasParser.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/GasParser_token.cpp
    )
