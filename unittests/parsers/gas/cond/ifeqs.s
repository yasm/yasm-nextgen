.ifeqs "foo","bar"
.byte 1
.endif

.ifeqs "foo","foo"
.byte 2
.endif
