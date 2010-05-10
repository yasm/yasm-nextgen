YASM_ADD_LIBRARY(yasmunit SHARED
    NasmInsnRunner.cpp
    unittest_util.cpp
    )
TARGET_LINK_LIBRARIES(yasmunit libyasmx ${GMOCK_LIBRARIES} ${GTEST_LIBRARIES})

ADD_SUBDIRECTORY(arch)
ADD_SUBDIRECTORY(parsers)
ADD_SUBDIRECTORY(yasmx)
