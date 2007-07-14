PROJECT(yasm)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

OPTION(ENABLE_NLS "Enable message translations" ON)

ENABLE_TESTING()

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
#INCLUDE(CPack)

INCLUDE(ConfigureChecks.cmake)

ADD_SUBDIRECTORY(libyasm)
#ADD_SUBDIRECTORY(modules)
#ADD_SUBDIRECTORY(frontends)

ADD_EXECUTABLE(test_expr
    test_expr.cpp
    )
TARGET_LINK_LIBRARIES(test_expr libyasm)

ADD_TEST(expr_basic test_expr)
