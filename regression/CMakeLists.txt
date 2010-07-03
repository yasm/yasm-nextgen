ADD_TEST(
    NAME regression_tests
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/regression/rtest.py
        ${CMAKE_SOURCE_DIR}/regression
        ${CMAKE_BINARY_DIR}/regression
        $<TARGET_FILE:yasm>)
