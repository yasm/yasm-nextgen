INCLUDE_DIRECTORIES(${yasm_SOURCE_DIR}/lib/yasmx)

YASM_ADD_UNIT_TEST(libyasmx_tests
    align_test.cpp
    bytes_util_test.cpp
    compose_test.cpp
    expr_test.cpp
    expr_util_test.cpp
    file_combpath_test.cpp
    floatnum_test.cpp
    hamt_test.cpp
    intnum_test.cpp
    location_test.cpp
    unescape_test.cpp
    value_test.cpp
    )
target_link_libraries(libyasmx_tests libyasmx ${GTEST_BOTH_LIBRARIES})
