PROJECT(yasm)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

# Where to look first for cmake modules
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

INCLUDE(YasmMacros)

OPTION(ENABLE_NLS "Enable message translations" ON)

OPTION(YASM_BUILD_TESTS "Enable building of tests" ON)

IF(YASM_BUILD_TESTS)
    ENABLE_TESTING()
ENDIF(YASM_BUILD_TESTS)

# Default build type to debug if not set
IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Debug CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

set (YASM_VERSION_MAJOR 0)
set (YASM_VERSION_MINOR 6)
set (YASM_VERSION_SUBMINOR 99)
set (PACKAGE_INTVER "${YASM_VERSION_MAJOR}.${YASM_VERSION_MINOR}.${YASM_VERSION_SUBMINOR}")
set (PACKAGE_VERSION "HEAD")
set (PACKAGE_STRING "yasm ${PACKAGE_VERSION}")

INCLUDE_DIRECTORIES(BEFORE ${CMAKE_BINARY_DIR} ${yasm_SOURCE_DIR})

INCLUDE(CTest)
#INCLUDE(Dart)

#set (CPACK_PACKAGE_DESCRIPTION_SUMMARY "Yasm is a modular assembler")
#set (CPACK_PACKAGE_NAME "yasm")
#set (CPACK_PACKAGE_VENDOR "Tortall Networks")
#set (CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/pkg-descr")
#set (CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/Copying/COPYING")
#set (CPACK_PACKAGE_VERSION_MAJOR "${YASM_VERSION_MAJOR}")
#set (CPACK_PACKAGE_VERSION_MINOR "${YASM_VERSION_MINOR}")
#set (CPACK_PACKAGE_VERSION_PATCH "${YASM_VERSION_SUBMINOR}")
#set (CPACK_SOURCE_GENERATOR "TGZ;TBZ2;ZIP")
#set (CPACK_SOURCE_PACKAGE_FILE_NAME "yasm-${PACKAGE_INTVER}")
#INCLUDE(CPack)

INCLUDE(ConfigureChecks.cmake)

add_subdirectory(tools)

YASM_ADD_MODULE_SUBDIRECTORY(modules)
MESSAGE(STATUS "Building modules: ${YASM_MODULES}")
#MESSAGE(STATUS "Module source files: ${YASM_MODULES_SRC}")

#
# Generate static_modules.cpp
# This file ensures all modules are brought in by the linker.
#

SET(STATIC_MODULES_CPP ${CMAKE_BINARY_DIR}/static_modules.cpp)
SET(STATIC_MODULES_CPP_REV 1)

# Don't regen if no changes; default to regen
SET(regen_static_modules_cpp TRUE)
IF(EXISTS ${STATIC_MODULES_CPP})
    FILE(READ ${STATIC_MODULES_CPP} _old_static_modules_cpp)
    STRING(REGEX MATCHALL "[^\n]*\n" _lines "${_old_static_modules_cpp}")
    #MESSAGE(STATUS "Lines: ${_lines}")

    LIST(GET _lines 0 _line0)
    STRING(REGEX MATCH "([A-Za-z][A-Za-z0-9_]+[ ])+" _old_modules "${_line0}")
    #MESSAGE(STATUS "Old modules: ${_old_modules}")
    STRING(COMPARE EQUAL "${_old_modules}" "${YASM_MODULES} " _modules_match)

    LIST(GET _lines 1 _line1)
    STRING(REGEX MATCH "rev [0-9]+" _old_modules_rev "${_line1}")
    #MESSAGE(STATUS "Old modules rev: ${_old_modules_rev}")
    STRING(COMPARE EQUAL "${_old_modules_rev}" "rev ${STATIC_MODULES_CPP_REV}"
           _modules_rev_match)

    IF(_modules_match AND _modules_rev_match)
        SET(regen_static_modules_cpp FALSE)
    ENDIF(_modules_match AND _modules_rev_match)
ENDIF(EXISTS ${STATIC_MODULES_CPP})

IF(regen_static_modules_cpp)
    MESSAGE(STATUS "Generating static modules file")
    FILE(WRITE ${STATIC_MODULES_CPP} "// ${YASM_MODULES} \n")
    FILE(APPEND ${STATIC_MODULES_CPP} "// rev ${STATIC_MODULES_CPP_REV}\n")
    FILE(APPEND ${STATIC_MODULES_CPP} "// static module references\n")
    FILE(APPEND ${STATIC_MODULES_CPP} "namespace yasm {\n")
    FILE(APPEND ${STATIC_MODULES_CPP} "void\nuse_static_modules()\n{\n}\n")
    FOREACH(module ${YASM_MODULES})
        FILE(APPEND ${STATIC_MODULES_CPP} "// ${module}\n")
        STRING(REGEX MATCHALL "[a-zA-Z][a-zA-Z0-9]+" _modulepath ${module})
        FOREACH(_pathpart ${_modulepath})
            FILE(APPEND ${STATIC_MODULES_CPP} "namespace ${_pathpart} { ")
        ENDFOREACH(_pathpart)
        FILE(APPEND ${STATIC_MODULES_CPP} "\nextern bool static_ref;\n")
        FILE(APPEND ${STATIC_MODULES_CPP}
             "static bool do_static_ref = static_ref;\n")
        FOREACH(_pathpart ${_modulepath})
            FILE(APPEND ${STATIC_MODULES_CPP} "}")
        ENDFOREACH(_pathpart)
    ENDFOREACH(module)
    FILE(APPEND ${STATIC_MODULES_CPP} "\n}\n") 
ELSE(regen_static_modules_cpp)
    MESSAGE(STATUS "Not regenerating static modules file (unchanged)")
ENDIF(regen_static_modules_cpp)

LIST(APPEND YASM_MODULES_SRC ${STATIC_MODULES_CPP})

ADD_SUBDIRECTORY(libyasm)

#ADD_SUBDIRECTORY(frontends)

ADD_EXECUTABLE(test_expr
    test_expr.cpp
    )
TARGET_LINK_LIBRARIES(test_expr libyasm ${YASM_MODULES})
ADD_TEST(expr_basic test_expr)

ADD_EXECUTABLE(test_preproc
    test_preproc.cpp
    )
TARGET_LINK_LIBRARIES(test_preproc libyasm ${YASM_MODULES})
ADD_TEST(preproc_basic test_preproc)

