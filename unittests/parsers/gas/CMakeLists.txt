YASM_ADD_UNIT_TEST(parser_gas_tests
    GasStringParser_test.cpp
    )

target_link_libraries(parser_gas_tests libyasmx yasmunit
                      ${GTEST_BOTH_LIBRARIES})
