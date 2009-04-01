IF(NOT BUILD_STATIC)
    INSTALL(FILES
        bitcount.h
        BitVector.h
        Compose.h
	errwarn.h
        IntervalTree.h
        marg_ostream_fwd.h
        marg_ostream.h
        MD5.h
        nocase.h
        phash.h
        ptr_vector.h
        registry.h
        scoped_array.h
        scoped_ptr.h
        DESTINATION ${INCLUDE_INSTALL_DIR}/libyasmx/Support
        )
ENDIF(NOT BUILD_STATIC)

