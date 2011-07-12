.byte 0xff	# out: ff
.word		# no output
.byte 0xff	# out: ff
.word ,		# out: 00 00 00 00
.byte 0xff	# out: ff
.word 5,	# out: 05 00 00 00
.byte 0xff	# out: ff
#.ascii		# error
.byte 0xff	# out: ff
.ascii "1",	# out: 31
.byte 0xff	# out: ff
.uleb128	# should be no output, but gas outputs 00?  we output nothing
.byte 0xff	# out: ff
.uleb128 ,	# out: 00 00
.byte 0xff	# out: ff
.uleb128 5,	# out: 05 00
.byte 0xff	# out: ff
.float		# no output
.byte 0xff	# out: ff
.float ,	# out: 00 00 00 00 00 00 00 00
.byte 0xff	# out: ff
.float 5.0,	# out: 00 00 a0 40 00 00 00 00
.byte 0xff	# out: ff
