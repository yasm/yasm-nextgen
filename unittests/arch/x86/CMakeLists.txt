INCLUDE_DIRECTORIES(${yasm_SOURCE_DIR}/modules/arch/x86)

YASM_ADD_UNIT_TEST(arch_x86_tests
    x86effaddr_test.cpp
    x86insn_test.cpp
    ${yasm_SOURCE_DIR}/modules/arch/x86/X86EffAddr.cpp
    )
target_link_libraries(arch_x86_tests libyasmx yasmunit ${GTEST_BOTH_LIBRARIES})

SET_SOURCE_FILES_PROPERTIES(x86effaddr_test.h PROPERTIES
    OBJECT_DEPENDS ${yasm_BINARY_DIR}/modules/X86Register.cpp
    )
