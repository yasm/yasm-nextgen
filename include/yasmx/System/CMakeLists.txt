IF(NOT BUILD_STATIC)
    INSTALL(FILES
        file.h
        plugin.h
        DESTINATION ${INCLUDE_INSTALL_DIR}/libyasmx/System
        )
ENDIF(NOT BUILD_STATIC)

