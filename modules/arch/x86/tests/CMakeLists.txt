INCLUDE_DIRECTORIES(${yasm_SOURCE_DIR}/modules/arch/x86)

ADD_CXXTEST(arch_x86_tests
    x86effaddr_test.h
    EXTRA_SRCS
    ${yasm_SOURCE_DIR}/modules/arch/x86/x86effaddr.cpp
    )
target_link_libraries(arch_x86_tests libyasmx)

SET_SOURCE_FILES_PROPERTIES(x86effaddr_test.h PROPERTIES
    OBJECT_DEPENDS ${yasm_BINARY_DIR}/modules/x86regtmod.cpp
    )
