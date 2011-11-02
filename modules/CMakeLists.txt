INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

INCLUDE(arch/CMakeLists.txt)
INCLUDE(dbgfmts/CMakeLists.txt)
INCLUDE(objfmts/CMakeLists.txt)
INCLUDE(parsers/CMakeLists.txt)

MESSAGE(STATUS "Standard modules: ${YASM_MODULES}")

#
# Generate init_plugin.cpp
# This file provides the yasm_init_plugin() function for yasmxstd.
#

SET(INIT_PLUGIN_CPP ${CMAKE_CURRENT_BINARY_DIR}/init_plugin.cpp)
SET(INIT_PLUGIN_CPP_REV 5)

# Don't regen if no changes; default to regen
SET(regen_init_plugin_cpp TRUE)
IF(EXISTS ${INIT_PLUGIN_CPP})
    FILE(READ ${INIT_PLUGIN_CPP} _old_init_plugin_cpp)
    STRING(REGEX MATCHALL "[^\n]*\n" _lines "${_old_init_plugin_cpp}")
    #MESSAGE(STATUS "Lines: ${_lines}")

    LIST(GET _lines 0 _line0)
    STRING(REGEX MATCH "([A-Za-z][A-Za-z0-9_]+[ ]?)+" _old_modules "${_line0}")
    #MESSAGE(STATUS "Old modules: ${_old_modules}")
    STRING(REPLACE ";" " " _modules_str "${YASM_MODULES}")
    STRING(COMPARE EQUAL "${_old_modules}" "${_modules_str} " _modules_match)

    LIST(GET _lines 1 _line1)
    STRING(REGEX MATCH "rev [0-9]+" _old_modules_rev "${_line1}")
    #MESSAGE(STATUS "Old modules rev: ${_old_modules_rev}")
    STRING(COMPARE EQUAL "${_old_modules_rev}" "rev ${INIT_PLUGIN_CPP_REV}"
           _modules_rev_match)

    IF(_modules_match AND _modules_rev_match)
        SET(regen_init_plugin_cpp FALSE)
    ENDIF(_modules_match AND _modules_rev_match)
ENDIF(EXISTS ${INIT_PLUGIN_CPP})

IF(regen_init_plugin_cpp)
    MESSAGE(STATUS "Generating standard plugin initialization file")
    STRING(REPLACE ";" " " _modules_str "${YASM_MODULES}")
    FILE(WRITE ${INIT_PLUGIN_CPP} "// ${_modules_str} \n")
    FILE(APPEND ${INIT_PLUGIN_CPP} "// rev ${INIT_PLUGIN_CPP_REV}\n")
    FILE(APPEND ${INIT_PLUGIN_CPP} "#include <yasmx/Config/export.h>\n")
    FILE(APPEND ${INIT_PLUGIN_CPP} "#include <yasmx/System/plugin.h>\n\n")
    FILE(APPEND ${INIT_PLUGIN_CPP} "using namespace yasm;\n\n")
    FOREACH(module ${YASM_MODULES})
	#FILE(APPEND ${INIT_PLUGIN_CPP} "// ${module}\n")
        FILE(APPEND ${INIT_PLUGIN_CPP} "void yasm_")
        STRING(REGEX MATCHALL "[a-zA-Z][a-zA-Z0-9]+" _modulepath ${module})
        FOREACH(_pathpart ${_modulepath})
            FILE(APPEND ${INIT_PLUGIN_CPP} "${_pathpart}_")
        ENDFOREACH(_pathpart)
        FILE(APPEND ${INIT_PLUGIN_CPP} "DoRegister();\n")
    ENDFOREACH(module)
    FILE(APPEND ${INIT_PLUGIN_CPP} "\nYASM_STD_EXPORT\n")
    FILE(APPEND ${INIT_PLUGIN_CPP} "bool\n")
    FILE(APPEND ${INIT_PLUGIN_CPP} "yasm::LoadStandardPlugins()\n")
    FILE(APPEND ${INIT_PLUGIN_CPP} "{\n")
    FOREACH(module ${YASM_MODULES})
        FILE(APPEND ${INIT_PLUGIN_CPP} "    yasm_")
        STRING(REGEX MATCHALL "[a-zA-Z][a-zA-Z0-9]+" _modulepath ${module})
        FOREACH(_pathpart ${_modulepath})
            FILE(APPEND ${INIT_PLUGIN_CPP} "${_pathpart}_")
        ENDFOREACH(_pathpart)
        FILE(APPEND ${INIT_PLUGIN_CPP} "DoRegister();\n")
    ENDFOREACH(module)
    FILE(APPEND ${INIT_PLUGIN_CPP} "    return true;\n}\n") 
ELSE(regen_init_plugin_cpp)
    MESSAGE(STATUS "Not regenerating standard plugin init file (unchanged)")
ENDIF(regen_init_plugin_cpp)

SET_SOURCE_FILES_PROPERTIES(init_plugin.cpp GENERATED)

INCLUDE(CreateSourceGroups)
CreateSourceGroups("Source Files" "" "${YASM_MODULES_SRC}")

YASM_ADD_LIBRARY(yasmstdx SHARED
    init_plugin.cpp
    ${YASM_MODULES_SRC}
    )
TARGET_LINK_LIBRARIES(yasmstdx libyasmx)
IF(NOT BUILD_STATIC)
    TARGET_LINK_LIBRARIES(yasmstdx ${LIBDL})
    SET_TARGET_PROPERTIES(yasmstdx PROPERTIES
	VERSION "0.0.0"
	SOVERSION 0
	)
ENDIF(NOT BUILD_STATIC)

IF(NOT BUILD_STATIC)
    INSTALL(TARGETS yasmstdx ${INSTALL_TARGETS_DEFAULT_ARGS})
ENDIF(NOT BUILD_STATIC)
