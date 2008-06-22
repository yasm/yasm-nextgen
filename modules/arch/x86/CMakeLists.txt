ADD_CUSTOM_COMMAND(
    OUTPUT
        ${CMAKE_CURRENT_BINARY_DIR}/x86insns.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/x86insn_gas.gperf
        ${CMAKE_CURRENT_BINARY_DIR}/x86insn_nasm.gperf
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/arch/x86/gen_x86_insn.py
           ${CMAKE_CURRENT_BINARY_DIR}/x86insns.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/x86insn_gas.gperf
           ${CMAKE_CURRENT_BINARY_DIR}/x86insn_nasm.gperf
    MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/arch/x86/gen_x86_insn.py
    )

YASM_GENPERF(
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/x86/x86cpu.gperf
    ${CMAKE_CURRENT_BINARY_DIR}/x86cpu.cpp
    )

YASM_GENPERF(
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/x86/x86regtmod.gperf
    ${CMAKE_CURRENT_BINARY_DIR}/x86regtmod.cpp
    )

YASM_GENPERF(
    ${CMAKE_CURRENT_BINARY_DIR}/x86insn_nasm.gperf
    ${CMAKE_CURRENT_BINARY_DIR}/x86insn_nasm.cpp
    )

YASM_GENPERF(
    ${CMAKE_CURRENT_BINARY_DIR}/x86insn_gas.gperf
    ${CMAKE_CURRENT_BINARY_DIR}/x86insn_gas.cpp
    )

YASM_ADD_MODULE(arch_x86
    arch/x86/x86arch.cpp
    arch/x86/x86common.cpp
    arch/x86/x86effaddr.cpp
    arch/x86/x86general.cpp
    arch/x86/x86jmp.cpp
    arch/x86/x86jmpfar.cpp
    arch/x86/x86opcode.cpp
    arch/x86/x86insn.cpp
    x86cpu.cpp
    x86regtmod.cpp
    )

SET(insn_DEPS
    ${CMAKE_CURRENT_BINARY_DIR}/x86insn_nasm.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/x86insn_gas.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/x86insns.cpp
    )

SET_SOURCE_FILES_PROPERTIES(arch/x86/x86insn.cpp PROPERTIES
    OBJECT_DEPENDS "${insn_DEPS}"
    )
