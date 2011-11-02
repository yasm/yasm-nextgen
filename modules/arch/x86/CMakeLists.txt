ADD_CUSTOM_COMMAND(
    OUTPUT
        ${CMAKE_CURRENT_BINARY_DIR}/X86Insns.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/X86Insn_gas.gperf
        ${CMAKE_CURRENT_BINARY_DIR}/X86Insn_nasm.gperf
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/arch/x86/gen_x86_insn.py
           ${CMAKE_CURRENT_BINARY_DIR}/X86Insns.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/X86Insn_gas.gperf
           ${CMAKE_CURRENT_BINARY_DIR}/X86Insn_nasm.gperf
    MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/arch/x86/gen_x86_insn.py
    )

YASM_GENPERF(
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/x86/X86Arch_cpu.gperf
    ${CMAKE_CURRENT_BINARY_DIR}/X86Arch_cpu.cpp
    )

YASM_GENPERF(
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/x86/X86Register.gperf
    ${CMAKE_CURRENT_BINARY_DIR}/X86Register.cpp
    )

YASM_GENPERF(
    ${CMAKE_CURRENT_BINARY_DIR}/X86Insn_nasm.gperf
    ${CMAKE_CURRENT_BINARY_DIR}/X86Insn_nasm.cpp
    )

YASM_GENPERF(
    ${CMAKE_CURRENT_BINARY_DIR}/X86Insn_gas.gperf
    ${CMAKE_CURRENT_BINARY_DIR}/X86Insn_gas.cpp
    )

SOURCE_GROUP("Source Files\\arch\\x86" FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/x86/gen_x86_insn.py
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/x86/X86Arch_cpu.gperf
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/x86/X86Register.gperf
    )

YASM_ADD_MODULE(arch_x86
    arch/x86/X86Arch.cpp
    arch/x86/X86Common.cpp
    arch/x86/X86EffAddr.cpp
    arch/x86/X86General.cpp
    arch/x86/X86Jmp.cpp
    arch/x86/X86JmpFar.cpp
    arch/x86/X86Opcode.cpp
    arch/x86/X86Insn.cpp
    X86Arch_cpu.cpp
    X86Register.cpp
    )

SET(insn_DEPS
    ${CMAKE_CURRENT_BINARY_DIR}/X86Insn_nasm.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/X86Insn_gas.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/X86Insns.cpp
    )

SET_SOURCE_FILES_PROPERTIES(arch/x86/X86Insn.cpp PROPERTIES
    OBJECT_DEPENDS "${insn_DEPS}"
    )
