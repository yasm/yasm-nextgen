ADD_EXECUTABLE(yasm
    yasm.cpp
    yasm-options.cpp
    ${YASM_MODULES_SRC}
    )
TARGET_LINK_LIBRARIES(yasm ${YASM_MODULES} libyasm)

INSTALL(TARGETS yasm RUNTIME DESTINATION bin)
