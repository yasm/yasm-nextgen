INCLUDE_DIRECTORIES(${yasm_SOURCE_DIR}/libyasmx)

MACRO(LIBYASM_UNIT_TESTS)
   FOREACH(_testname ${ARGN})
       yasm_add_unit_test(${_testname} ${_testname}.cpp ${YASM_MODULES_SRC})
       IF(MSVC)
           target_link_libraries(${_testname} ${YASM_MODULES} libyasmx)
       ELSE(MSVC)
           target_link_libraries(${_testname} ${YASM_MODULES} libyasmx
                                 boost_unit_test_framework)
       ENDIF(MSVC)
   ENDFOREACH(_testname)
ENDMACRO(LIBYASM_UNIT_TESTS)

LIBYASM_UNIT_TESTS(
    align_test
    bytes_util_test
    compose_test
    hamt_test
    linemap_test
    file_combpath_test
    floatnum_new_test
    floatnum_get_test
    intnum_test
    marg_ostream_test
    unescape_test
    )
