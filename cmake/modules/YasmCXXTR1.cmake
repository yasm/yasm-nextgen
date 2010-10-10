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
CHECK_INCLUDE_FILE_CXX(functional HAVE_STD_FUNCTIONAL)
CHECK_INCLUDE_FILE_CXX(tr1/functional HAVE_TR1_FUNCTIONAL)

#
# Look for various pieces of TR1 functionality: function, ref, bind, mem_fn
#

IF(HAVE_TR1_FUNCTIONAL)
    YASM_CHECK_CXX("std::function<void (int)> x = f2;" "std::function" "tr1/functional" HAVE_STD_FUNCTION1)
    YASM_CHECK_CXX("std::tr1::function<void (int)> x = f2;" "std::tr1::function" "tr1/functional" HAVE_TR1_FUNCTION1)
    YASM_CHECK_CXX("X x1;\n  X& x2 = std::ref(x1);" "std::ref" "tr1/functional" HAVE_STD_REF1)
    YASM_CHECK_CXX("X x1;\n  X& x2 = std::tr1::ref(x1);" "std::tr1::ref" "tr1/functional" HAVE_TR1_REF1)
    YASM_CHECK_CXX("std::bind(f2, 1)();" "std::bind" "tr1/functional" HAVE_STD_BIND1)
    YASM_CHECK_CXX("std::tr1::bind(f2, 1)();" "std::tr1::bind" "tr1/functional" HAVE_TR1_BIND1)
    YASM_CHECK_CXX("std::vector<X> v;\n  std::for_each(v.begin(), v.end(), std::mem_fn(&X::f));" "std::mem_fn" "tr1/functional" HAVE_STD_MEMFN1)
    YASM_CHECK_CXX("std::vector<X> v;\n  std::for_each(v.begin(), v.end(), std::tr1::mem_fn(&X::f));" "std::tr1::mem_fn" "tr1/functional" HAVE_TR1_MEMFN1)
ENDIF(HAVE_TR1_FUNCTIONAL)

IF(HAVE_STD_FUNCTIONAL)
    YASM_CHECK_CXX("std::function<void (int)> x = f2;" "std::function" "functional" HAVE_STD_FUNCTION2)
    YASM_CHECK_CXX("std::tr1::function<void (int)> x = f2;" "std::tr1::function" "functional" HAVE_TR1_FUNCTION2)
    YASM_CHECK_CXX("X x1;\n  X& x2 = std::ref(x1);" "std::ref" "functional" HAVE_STD_REF2)
    YASM_CHECK_CXX("X x1;\n  X& x2 = std::tr1::ref(x1);" "std::tr1::ref" "functional" HAVE_TR1_REF2)
    YASM_CHECK_CXX("std::bind(f2, 1)();" "std::bind" "functional" HAVE_STD_BIND2)
    YASM_CHECK_CXX("std::tr1::bind(f2, 1)();" "std::tr1::bind" "functional" HAVE_TR1_BIND2)
    YASM_CHECK_CXX("std::vector<X> v;\n  std::for_each(v.begin(), v.end(), std::mem_fn(&X::f));" "std::mem_fn" "functional" HAVE_STD_MEMFN2)
    YASM_CHECK_CXX("std::vector<X> v;\n  std::for_each(v.begin(), v.end(), std::tr1::mem_fn(&X::f));" "std::tr1::mem_fn" "functional" HAVE_TR1_MEMFN2)
ENDIF(HAVE_STD_FUNCTIONAL)

#
# Generate functional.h code
#
IF(HAVE_STD_FUNCTIONAL)
    SET(FUNCTIONAL_INCLUDES "${FUNCTIONAL_INCLUDES}#include <functional>\n")
ENDIF(HAVE_STD_FUNCTIONAL)

# Bring in generic TR1 functional header if we have it.
IF(HAVE_TR1_FUNCTIONAL)
    SET(FUNCTIONAL_INCLUDES "${FUNCTIONAL_INCLUDES}#include <tr1/functional>\n")
ENDIF(HAVE_TR1_FUNCTIONAL)

# Point to correct namespace, bringing in boost if necessary.
IF(HAVE_STD_FUNCTION2 AND HAVE_STD_REF2 AND HAVE_STD_BIND2 AND HAVE_STD_MEMFN2)
    SET(TR1_NAMESPACE "std")
    SET(TR1_HEADER "functional")
ELSEIF(HAVE_TR1_FUNCTION2 AND HAVE_TR1_REF2 AND HAVE_TR1_BIND2 AND HAVE_TR1_MEMFN2)
    SET(TR1_NAMESPACE "std::tr1")
    SET(TR1_HEADER "functional")
ELSEIF(HAVE_STD_FUNCTION1 AND HAVE_STD_REF1 AND HAVE_STD_BIND1 AND HAVE_STD_MEMFN1)
    SET(TR1_NAMESPACE "std")
    SET(TR1_HEADER "tr1/functional")
ELSEIF(HAVE_TR1_FUNCTION1 AND HAVE_TR1_REF1 AND HAVE_TR1_BIND1 AND HAVE_TR1_MEMFN1)
    SET(TR1_NAMESPACE "std::tr1")
    SET(TR1_HEADER "tr1/functional")
ELSE(HAVE_STD_FUNCTION2 AND HAVE_STD_REF2 AND HAVE_STD_BIND2 AND HAVE_STD_MEMFN2)
    SET(TR1_NAMESPACE "boost")
    SET(TR1_HEADER "boost")
ENDIF(HAVE_STD_FUNCTION2 AND HAVE_STD_REF2 AND HAVE_STD_BIND2 AND HAVE_STD_MEMFN2)
MESSAGE(STATUS "Using functional ${TR1_NAMESPACE}:: from ${TR1_HEADER}")

SET(BOOST_REQUIRED FALSE)
SET(FUNCTIONAL_ALIASES "namespace TR1 = ${TR1_NAMESPACE};\n")
IF(TR1_HEADER STREQUAL "boost")
    SET(BOOST_REQUIRED TRUE)
    SET(FUNCTIONAL_INCLUDES "${FUNCTIONAL_INCLUDES}#include <boost/function.hpp>\n")
    SET(FUNCTIONAL_INCLUDES "${FUNCTIONAL_INCLUDES}#include <boost/ref.hpp>\n")
    SET(FUNCTIONAL_INCLUDES "${FUNCTIONAL_INCLUDES}#include <boost/bind.hpp>\n")
    SET(FUNCTIONAL_INCLUDES "${FUNCTIONAL_INCLUDES}#include <boost/mem_fn.hpp>\n")
ELSE(TR1_HEADER STREQUAL "boost")
    SET(FUNCTIONAL_ALIASES "${FUNCTIONAL_ALIASES}using namespace ${TR1_NAMESPACE}::placeholders;\n")
ENDIF(TR1_HEADER STREQUAL "boost")
