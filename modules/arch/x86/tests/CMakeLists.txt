INCLUDE_DIRECTORIES(${yasm_SOURCE_DIR}/modules/arch/x86)

yasm_add_unit_test(x86effaddr_test x86effaddr_test.cpp
                   ${yasm_SOURCE_DIR}/modules/arch/x86/x86effaddr.cpp
		   )
IF(MSVC)
    target_link_libraries(x86effaddr_test libyasmx)
ELSE(MSVC)
    target_link_libraries(x86effaddr_test libyasmx boost_unit_test_framework)
ENDIF(MSVC)
