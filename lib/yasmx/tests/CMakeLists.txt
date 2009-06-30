INCLUDE_DIRECTORIES(${yasm_SOURCE_DIR}/lib/yasmx)

ADD_CXXTEST(libyasmx_tests
    align_test.h
    bytes_util_test.h
    compose_test.h
    expr_test.h
    hamt_test.h
    linemap_test.h
    location_test.h
    file_combpath_test.h
    floatnum_test.h
    intnum_test.h
    marg_ostream_test.h
    nocase_test.h
    unescape_test.h
    value_test.h
    )
target_link_libraries(libyasmx_tests libyasmx)
