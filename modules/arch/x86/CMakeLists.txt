ADD_CUSTOM_COMMAND(
    OUTPUT
        ${CMAKE_CURRENT_BINARY_DIR}/x86insns.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/x86insn_gas.gperf
        ${CMAKE_CURRENT_BINARY_DIR}/x86insn_nasm.gperf
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/gen_x86_insn.py
           ${CMAKE_CURRENT_BINARY_DIR}/x86insns.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/x86insn_gas.gperf
           ${CMAKE_CURRENT_BINARY_DIR}/x86insn_nasm.gperf
    MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/gen_x86_insn.py
    )

YASM_GENPERF(
    ${CMAKE_CURRENT_SOURCE_DIR}/x86cpu.gperf
    ${CMAKE_CURRENT_BINARY_DIR}/x86cpu.cpp
    )

YASM_GENPERF(
    ${CMAKE_CURRENT_SOURCE_DIR}/x86regtmod.gperf
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
    x86arch.cpp
    x86common.cpp
    x86effaddr.cpp
    x86general.cpp
    x86jmp.cpp
    x86jmpfar.cpp
    x86opcode.cpp
    x86insn.cpp
    x86cpu.cpp
    x86regtmod.cpp
    )

SET(insn_DEPS
    ${CMAKE_CURRENT_BINARY_DIR}/x86insn_nasm.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/x86insn_gas.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/x86insns.cpp
    )

SET_SOURCE_FILES_PROPERTIES(x86insn.cpp PROPERTIES
    OBJECT_DEPENDS "${insn_DEPS}"
    )
