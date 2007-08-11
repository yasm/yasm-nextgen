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

ADD_SUBDIRECTORY(libyasm)

YASM_ADD_MODULE_SUBDIRECTORY(modules)
MESSAGE(STATUS "Building modules: ${YASM_MODULES}")

# Generate static_modules.cpp
# This file ensures all modules are brought in by the linker.
SET(STATIC_MODULES_SRC ${CMAKE_BINARY_DIR}/static_modules.cpp)
FILE(WRITE ${STATIC_MODULES_SRC} "// Static module references\n")
FILE(APPEND ${STATIC_MODULES_SRC} "namespace yasm {\n") 
FOREACH(module ${YASM_MODULES})
    FILE(APPEND ${STATIC_MODULES_SRC} "// ${module}\n")
    STRING(REGEX MATCHALL "[a-zA-Z][a-zA-Z0-9]+" _modulepath ${module})
    FOREACH(_pathpart ${_modulepath})
	FILE(APPEND ${STATIC_MODULES_SRC} "namespace ${_pathpart} { ")
    ENDFOREACH(_pathpart)
    FILE(APPEND ${STATIC_MODULES_SRC} "\nextern bool static_ref;\n")
    FILE(APPEND ${STATIC_MODULES_SRC} "static bool do_static_ref = static_ref;\n")
    FOREACH(_pathpart ${_modulepath})
	FILE(APPEND ${STATIC_MODULES_SRC} "}")
    ENDFOREACH(_pathpart)
ENDFOREACH(module)
FILE(APPEND ${STATIC_MODULES_SRC} "\n}\n") 

#ADD_SUBDIRECTORY(frontends)

ADD_EXECUTABLE(test_expr
    test_expr.cpp
    )
TARGET_LINK_LIBRARIES(test_expr libyasm)
ADD_TEST(expr_basic test_expr)

ADD_EXECUTABLE(test_preproc
    test_preproc.cpp
    ${STATIC_MODULES_SRC}
    )
TARGET_LINK_LIBRARIES(test_preproc ${YASM_MODULES} libyasm)
ADD_TEST(preproc_basic test_preproc)

