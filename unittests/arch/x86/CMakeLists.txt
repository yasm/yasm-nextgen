YASM_ADD_UNIT_TEST(arch_x86_tests
    x86effaddr_test.cpp
    x86insn_test.cpp
    )
target_link_libraries(arch_x86_tests libyasmx yasmunit
                      ${GMOCK_BOTH_LIBRARIES} ${GTEST_LIBARIES})
