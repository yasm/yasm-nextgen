INCLUDE_DIRECTORIES(${yasm_SOURCE_DIR}/libyasm)

MACRO(LIBYASM_UNIT_TESTS)
   FOREACH(_testname ${ARGN})
       yasm_add_unit_test(${_testname} ${_testname}.cpp)
       target_link_libraries(${_testname} libyasm)
   ENDFOREACH(_testname)
ENDMACRO(LIBYASM_UNIT_TESTS)

LIBYASM_UNIT_TESTS(
    floatnum_new_test
    floatnum_get_test
    file_combpath_test
    )
