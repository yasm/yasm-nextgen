INCLUDE_DIRECTORIES(${yasm_SOURCE_DIR}/modules/parsers/nasm)

YASM_ADD_UNIT_TEST(parser_nasm_tests
    NasmStringParser_test.cpp
    ${yasm_SOURCE_DIR}/modules/parsers/nasm/NasmLexer.cpp
    ${yasm_SOURCE_DIR}/modules/parsers/nasm/NasmPreproc.cpp
    ${yasm_SOURCE_DIR}/modules/parsers/nasm/NasmStringParser.cpp
    )

target_link_libraries(parser_nasm_tests libyasmx yasmunit
                      ${GTEST_BOTH_LIBRARIES})
