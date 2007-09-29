INCLUDE_DIRECTORIES(${yasm_SOURCE_DIR}/libyasm)

MACRO(LIBYASM_UNIT_TESTS)
   FOREACH(_testname ${ARGN})
       yasm_add_unit_test(${_testname} ${_testname}.cpp)
       IF(MSVC)
           target_link_libraries(${_testname} libyasm)
       ELSE(MSVC)
           target_link_libraries(${_testname} libyasm boost_unit_test_framework)
       ENDIF(MSVC)
   ENDFOREACH(_testname)
ENDMACRO(LIBYASM_UNIT_TESTS)

LIBYASM_UNIT_TESTS(
    compose_test
    hamt_test
    linemap_test
    file_combpath_test
    floatnum_new_test
    floatnum_get_test
    unescape_test
    )
