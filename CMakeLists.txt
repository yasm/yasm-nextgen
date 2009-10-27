PROJECT(yasm)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4)
if (COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
endif (COMMAND cmake_policy)

# Where to look first for cmake modules
set(YASM_MODULE_DIR "${CMAKE_SOURCE_DIR}/cmake/modules")
set(CMAKE_MODULE_PATH "${YASM_MODULE_DIR}")

set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}/lib")
if (WIN32)
   # we don't want to be forced to set two paths into the build tree 
   set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})
endif (WIN32)

OPTION(BUILD_STATIC "Build a monolithic executable with no plugin support" OFF)

INCLUDE(InstallOptions.cmake)
INCLUDE(YasmMacros)

OPTION(ENABLE_NLS "Enable message translations" OFF)

OPTION(BUILD_TEST_COVERAGE "Enable test coverage if possible" ON)

FIND_PACKAGE(GTest)
IF(GTEST_FOUND)
    INCLUDE_DIRECTORIES(${GTEST_INCLUDE_DIRS})
ENDIF(GTEST_FOUND)

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
set (PACKAGE_NAME "yasm")
set (PACKAGE_BUILD "HEAD")
set (PACKAGE_VERSION ${PACKAGE_BUILD})
set (PACKAGE_STRING "yasm ${PACKAGE_VERSION}")

INCLUDE_DIRECTORIES(BEFORE
    ${CMAKE_BINARY_DIR}
    ${yasm_SOURCE_DIR}
    ${yasm_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
    )

INCLUDE(CTest)
#INCLUDE(Dart)

set (CPACK_PACKAGE_DESCRIPTION_SUMMARY "Yasm is a modular assembler")
set (CPACK_PACKAGE_NAME "yasm")
set (CPACK_PACKAGE_VENDOR "Tortall Networks")
set (CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/pkg-descr")
set (CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/Copying/COPYING")
set (CPACK_PACKAGE_VERSION_MAJOR "${YASM_VERSION_MAJOR}")
set (CPACK_PACKAGE_VERSION_MINOR "${YASM_VERSION_MINOR}")
set (CPACK_PACKAGE_VERSION_PATCH "${YASM_VERSION_SUBMINOR}")
set (CPACK_SOURCE_GENERATOR "TGZ;ZIP")
set (CPACK_SOURCE_PACKAGE_FILE_NAME "yasm-${PACKAGE_INTVER}")
set (CPACK_SOURCE_IGNORE_FILES
     "~$"
     "\\\\.gitignore"
     "/\\\\.git/"
     "\\\\.o$"
     "\\\\.swp$"
     "/objdir.*"
     "/doc.*"
     )
#message ("CPACK_SOURCE_IGNORE_FILES = ${CPACK_SOURCE_IGNORE_FILES}")
INCLUDE(CPack)

INCLUDE(ConfigureChecks.cmake)
INCLUDE(MacroAddCxxTest)

ADD_SUBDIRECTORY(tools)
ADD_SUBDIRECTORY(modules)
ADD_SUBDIRECTORY(lib)
ADD_SUBDIRECTORY(frontends)

YASM_ADD_UNIT_TEST(test_preproc TESTNAME preproc_basic
    test_preproc.cpp
    )
target_link_libraries(test_preproc libyasmx ${GTEST_BOTH_LIBRARIES})
