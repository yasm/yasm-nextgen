YASM_ADD_UNIT_TEST(parser_nasm_tests
    NasmStringParser_test.cpp
    )

target_link_libraries(parser_nasm_tests libyasmx yasmunit
                      ${GTEST_BOTH_LIBRARIES})
