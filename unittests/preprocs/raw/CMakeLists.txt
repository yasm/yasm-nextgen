YASM_ADD_UNIT_TEST(preproc_raw_tests TESTNAME preproc_raw_tests
    raw_preproc_test.cpp
    )
target_link_libraries(preproc_raw_tests libyasmx ${GTEST_BOTH_LIBRARIES})
