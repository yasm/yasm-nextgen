CMAKE_MINIMUM_REQUIRED(VERSION 2.6.2)
INCLUDE(gmock/internal_utils.cmake)
config_compiler_and_linker()

INCLUDE_DIRECTORIES(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/gmock)

# add a unit test, which is executed when running make test
# it will be built with RPATH pointing to the build dir
# The targets are always created, but only built for the "all"
# target if the option BUILD_TESTING is enabled. Otherwise the rules for
# the target are created but not built by default. You can build them by
# manually building the target.
# The name of the target can be specified using TESTNAME <testname>, if it is
# not given the macro will default to the <name>
macro (YASM_ADD_UNIT_TEST _test_NAME _libs)
    if(BUILD_TESTS)
        set(_srcList ${ARGN})
        set(_targetName ${_test_NAME})
        yasm_add_executable( ${_test_NAME} TEST ${_srcList} )

        set(_executable ${_test_NAME})
        if(${CMAKE_VERSION} VERSION_LESS "2.8.0")
            add_test( ${_targetName} ${_executable} )
        else(${CMAKE_VERSION} VERSION_LESS "2.8.0")
            add_test(NAME ${_targetName} COMMAND ${_executable} )
        endif(${CMAKE_VERSION} VERSION_LESS "2.8.0")

        set_source_files_properties(${_srcList} PROPERTIES
            COMPILE_FLAGS "${cxx_default}")

        if (BUILD_SHARED_LIBS)
            set_target_properties(${_test_NAME}
                PROPERTIES
                COMPILE_DEFINITIONS "GTEST_LINKED_AS_SHARED_LIBRARY=1")
        endif (BUILD_SHARED_LIBS)

        # To support mixing linking in static and dynamic libraries, link each
        # library in with an extra call to target_link_libraries.
        foreach (_lib "${_libs}")
            target_link_libraries(${_test_NAME} ${_lib})
        endforeach (_lib "${_libs}")
    endif(BUILD_TESTS)
endmacro (YASM_ADD_UNIT_TEST)

cxx_library_with_type(gmock SHARED "${cxx_strict}" gmock/gmock-gtest-all.cc)
cxx_library_with_type(gmock_main STATIC "${cxx_strict}" gmock/gmock_main.cc)
TARGET_LINK_LIBRARIES(gmock_main gmock)

cxx_library_with_type(yasmunit SHARED "${cxx_default}"
    NasmInsnRunner.cpp
    unittest_util.cpp
    )
TARGET_LINK_LIBRARIES(yasmunit libyasmx gmock)

ADD_SUBDIRECTORY(arch)
ADD_SUBDIRECTORY(parsers)
ADD_SUBDIRECTORY(yasmx)
