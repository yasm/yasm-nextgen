INCLUDE_DIRECTORIES(${yasm_SOURCE_DIR}/lib/yasmx)

YASM_ADD_UNIT_TEST(libyasmx_tests
    "libyasmx;yasmunit;gmock;gmock_main"
    align_test.cpp
    bytes_util_test.cpp
    expr_test.cpp
    expr_util_test.cpp
    floatnum_test.cpp
    hamt_test.cpp
    intnum_test.cpp
    location_test.cpp
    value_test.cpp
    )
