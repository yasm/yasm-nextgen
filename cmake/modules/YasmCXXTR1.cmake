# Check for C++ Technical Report 1 (TR1) features, and fall back to Boost
# if they are not included with the compiler.
# Features looked for: ref, function, bind, mem_fn
# All of these should be present in TR1's <functional>, but we need to fill
# in with Boost functions if any of these are missing.

INCLUDE(CheckIncludeFileCXX)

MACRO(YASM_CHECK_CXX TEST_CODE SYMBOL HEADER VARIABLE)
  IF("${VARIABLE}" MATCHES "^${VARIABLE}")
    SET(CMAKE_CONFIGURABLE_FILE_CONTENT
        "#include <algorithm>\n#include <vector>\n#include <${HEADER}>\n\nstruct X {\n  void f() {}\n};\n\nvoid f2(int y) {}\n\nint main() {\n  ${TEST_CODE};\n  return 0;\n}\n")
    CONFIGURE_FILE("${CMAKE_ROOT}/Modules/CMakeConfigurableFile.in"
        "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/CheckTR1.cpp" @ONLY)
    MESSAGE(STATUS "Looking for ${SYMBOL} in ${HEADER}")
    TRY_COMPILE(${VARIABLE}
        ${CMAKE_BINARY_DIR}
        ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/CheckTR1.cpp
        COMPILE_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS}
        CMAKE_FLAGS
        "-DCOMPILE_DEFINITIONS:STRING=${CMAKE_REQUIRED_FLAGS}"
        "-DCMAKE_CXX_FLAGS:STRING=${CMAKE_CXX_FLAGS}"
        "-DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}"
        "-DLINK_LIBRARIES:STRING=${CMAKE_REQUIRED_LIBRARIES}"
        "-DINCLUDE_DIRECTORIES:STRING=${CMAKE_REQUIRED_INCLUDES}"
        OUTPUT_VARIABLE OUTPUT)
    IF(${VARIABLE})
      MESSAGE(STATUS "Looking for ${SYMBOL} in ${HEADER} - found")
      SET(${VARIABLE} 1 CACHE INTERNAL "Have symbol ${SYMBOL} in ${HEADER}")
      FILE(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log 
        "Determining if the ${SYMBOL} "
        "exist passed with the following output:\n"
        "${OUTPUT}\nFile ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/CheckTR1.cpp:\n"
        "${CMAKE_CONFIGURABLE_FILE_CONTENT}\n")
    ELSE(${VARIABLE})
      MESSAGE(STATUS "Looking for ${SYMBOL} in ${HEADER} - not found.")
      SET(${VARIABLE} "" CACHE INTERNAL "Have symbol ${SYMBOL} in ${HEADER}")
      FILE(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log 
        "Determining if the ${SYMBOL} "
        "exist failed with the following output:\n"
        "${OUTPUT}\nFile ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/CheckTR1.cpp:\n"
        "${CMAKE_CONFIGURABLE_FILE_CONTENT}\n")
    ENDIF(${VARIABLE})
  ENDIF("${VARIABLE}" MATCHES "^${VARIABLE}")
ENDMACRO(YASM_CHECK_CXX)

#
# Look for TR1 header files
#
CHECK_INCLUDE_FILE_CXX(tr1/functional HAVE_TR1_FUNCTIONAL)

#
# Look for various pieces of TR1 functionality
#

# function
IF(HAVE_TR1_FUNCTIONAL)
    YASM_CHECK_CXX("std::tr1::function<int ()> x = main;" "std::tr1::function" "tr1/functional" HAVE_TR1_FUNCTION)
ENDIF(HAVE_TR1_FUNCTIONAL)

IF(NOT HAVE_TR1_FUNCTION)
YASM_CHECK_CXX("std::function<int ()> x = main;" "std::function" "functional" HAVE_STD_FUNCTION)
ENDIF(NOT HAVE_TR1_FUNCTION)

# ref
IF(HAVE_TR1_FUNCTIONAL)
    YASM_CHECK_CXX("X x1;\n  X& x2 = std::tr1::ref(x1);" "std::tr1::ref" "tr1/functional" HAVE_TR1_REF)
ENDIF(HAVE_TR1_FUNCTIONAL)

IF(NOT HAVE_TR1_REF)
    YASM_CHECK_CXX("X x1;\n  X& x2 = std::ref(x1);" "std::ref" "functional" HAVE_STD_REF)
ENDIF(NOT HAVE_TR1_REF)

# bind
IF(HAVE_TR1_FUNCTIONAL)
    YASM_CHECK_CXX("std::tr1::bind(f2, 1)();" "std::tr1::bind" "tr1/functional" HAVE_TR1_BIND)
ENDIF(HAVE_TR1_FUNCTIONAL)

IF(NOT HAVE_TR1_BIND)
    YASM_CHECK_CXX("std::bind(f2, 1)();" "std::bind" "functional" HAVE_STD_BIND)
ENDIF(NOT HAVE_TR1_BIND)

# mem_fn
IF(HAVE_TR1_FUNCTIONAL)
    YASM_CHECK_CXX("std::vector<X> v;\n  std::for_each(v.begin(), v.end(), std::tr1::mem_fn(&X::f));" "std::tr1::mem_fn" "tr1/functional" HAVE_TR1_MEMFN)
ENDIF(HAVE_TR1_FUNCTIONAL)

IF(NOT HAVE_TR1_MEMFN)
    YASM_CHECK_CXX("std::vector<X> v;\n  std::for_each(v.begin(), v.end(), std::mem_fn(&X::f));" "std::mem_fn" "functional" HAVE_STD_MEMFN)
ENDIF(NOT HAVE_TR1_MEMFN)

#
# Generate functional.h code
#
SET(FUNCTIONAL_INCLUDES "")
SET(FUNCTIONAL_ALIASES "")

# Bring in generic TR1 functional header if anything in it.
IF(HAVE_STD_FUNCTION OR HAVE_STD_REF OR HAVE_STD_BIND OR HAVE_STD_MEMFN)
    SET(FUNCTIONAL_INCLUDES "${FUNCTIONAL_INCLUDES}#include <functional>\n")
ENDIF(HAVE_STD_FUNCTION OR HAVE_STD_REF OR HAVE_STD_BIND OR HAVE_STD_MEMFN)

IF(HAVE_TR1_FUNCTION OR HAVE_TR1_REF OR HAVE_TR1_BIND OR HAVE_TR1_MEMFN)
    SET(FUNCTIONAL_INCLUDES "${FUNCTIONAL_INCLUDES}#include <tr1/functional>\n")
ENDIF(HAVE_TR1_FUNCTION OR HAVE_TR1_REF OR HAVE_TR1_BIND OR HAVE_TR1_MEMFN)

# Point to correct namespace, bringing in boost if necessary.

SET(BOOST_REQUIRED FALSE)

# function
IF(HAVE_STD_FUNCTION)
    SET(FUNCTIONAL_ALIASES "${FUNCTIONAL_ALIASES}namespace FUNCTION = std;\n")
ELSEIF(HAVE_TR1_FUNCTION)
    SET(FUNCTIONAL_ALIASES "${FUNCTIONAL_ALIASES}namespace FUNCTION = std::tr1;\n")
ELSE(HAVE_STD_FUNCTION)
    SET(FUNCTIONAL_INCLUDES "${FUNCTIONAL_INCLUDES}#include <boost/function.hpp>\n")
    SET(FUNCTIONAL_ALIASES "${FUNCTIONAL_ALIASES}namespace FUNCTION = boost;\n")
    SET(BOOST_REQUIRED TRUE)
ENDIF(HAVE_STD_FUNCTION)

# ref
IF(HAVE_STD_REF)
    SET(FUNCTIONAL_ALIASES "${FUNCTIONAL_ALIASES}namespace REF = std;\n")
ELSEIF(HAVE_TR1_REF)
    SET(FUNCTIONAL_ALIASES "${FUNCTIONAL_ALIASES}namespace REF = std::tr1;\n")
ELSE(HAVE_STD_FUNCTION)
    SET(FUNCTIONAL_INCLUDES "${FUNCTIONAL_INCLUDES}#include <boost/ref.hpp>\n")
    SET(FUNCTIONAL_ALIASES "${FUNCTIONAL_ALIASES}namespace REF = boost;\n")
    SET(BOOST_REQUIRED TRUE)
ENDIF(HAVE_STD_REF)

# bind
IF(HAVE_STD_BIND)
    SET(FUNCTIONAL_ALIASES "${FUNCTIONAL_ALIASES}namespace BIND = std;\n")
    SET(FUNCTIONAL_ALIASES "${FUNCTIONAL_ALIASES}using namespace std::placeholders;\n")
ELSEIF(HAVE_TR1_BIND)
    SET(FUNCTIONAL_ALIASES "${FUNCTIONAL_ALIASES}namespace BIND = std::tr1;\n")
    SET(FUNCTIONAL_ALIASES "${FUNCTIONAL_ALIASES}using namespace std::tr1::placeholders;\n")
ELSE(HAVE_STD_FUNCTION)
    SET(FUNCTIONAL_INCLUDES "${FUNCTIONAL_INCLUDES}#include <boost/bind.hpp>\n")
    SET(FUNCTIONAL_ALIASES "${FUNCTIONAL_ALIASES}namespace BIND = boost;\n")
    SET(BOOST_REQUIRED TRUE)
ENDIF(HAVE_STD_BIND)

# mem_fn
IF(HAVE_STD_MEMFN)
    SET(FUNCTIONAL_ALIASES "${FUNCTIONAL_ALIASES}namespace MEMFN = std;\n")
ELSEIF(HAVE_TR1_MEMFN)
    SET(FUNCTIONAL_ALIASES "${FUNCTIONAL_ALIASES}namespace MEMFN = std::tr1;\n")
ELSE(HAVE_STD_MEMFN)
    SET(FUNCTIONAL_INCLUDES "${FUNCTIONAL_INCLUDES}#include <boost/mem_fn.hpp>\n")
    SET(FUNCTIONAL_ALIASES "${FUNCTIONAL_ALIASES}namespace MEMFN = boost;\n")
    SET(BOOST_REQUIRED TRUE)
ENDIF(HAVE_STD_MEMFN)

IF(BOOST_REQUIRED AND NOT Boost_FOUND)
  MESSAGE(FATAL_ERROR "Couldn't find the Boost libraries and/or include directory to fulfill TR1 requirements.  Please install Boost or a TR1 development package. You can set BOOST_ROOT, BOOST_INCLUDEDIR and BOOST_LIBRARYDIR to help find Boost.")
ENDIF(BOOST_REQUIRED AND NOT Boost_FOUND)

CONFIGURE_FILE(functional.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/functional.h)
