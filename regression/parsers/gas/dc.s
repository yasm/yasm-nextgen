.data

.dc.s 5.0, 3.0			# out: 00 00 a0 40
				# out: 00 00 40 40
.byte 0xff ; .balign 16, 0xff	# out: ff ff ff ff ff ff ff ff
.dc.d 5.0, 3.0			# out: 00 00 00 00 00 00 14 40
				# out: 00 00 00 00 00 00 08 40
.byte 0xff ; .balign 16, 0xff	# out: ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff
.dc.x 5.0, 3.0			# out: 00 00 00 00 00 00 00 a0 01 40
				# out: 00 00 00 00 00 00 00 c0 00 40
.byte 0xff ; .balign 16, 0xff	# out: ff ff ff ff ff ff ff ff ff ff ff ff

.dc 5, 3			# out: 05 00 03 00
.byte 0xff ; .balign 16, 0xff	# out: ff ff ff ff ff ff ff ff ff ff ff ff
.dc.b 5, 3			# out: 05 03
.byte 0xff ; .balign 16, 0xff	# out: ff ff ff ff ff ff ff ff ff ff ff ff ff ff
.dc.w 5, 3			# out: 05 00 03 00
.byte 0xff ; .balign 16, 0xff	# out: ff ff ff ff ff ff ff ff ff ff ff ff
.dc.l 5, 3			# out: 05 00 00 00 03 00 00 00
.byte 0xff ; .balign 16, 0xff	# out: ff ff ff ff ff ff ff ff

.dcb.s 2, 5.0			# out: 00 00 a0 40
				# out: 00 00 a0 40
.byte 0xff ; .balign 16, 0xff	# out: ff ff ff ff ff ff ff ff
.dcb.d 2, 5.0			# out: 00 00 00 00 00 00 14 40
				# out: 00 00 00 00 00 00 14 40
.byte 0xff ; .balign 16, 0xff	# out: ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff
.dcb.x 2, 5.0			# out: 00 00 00 00 00 00 00 a0 01 40
				# out: 00 00 00 00 00 00 00 a0 01 40
.byte 0xff ; .balign 16, 0xff	# out: ff ff ff ff ff ff ff ff ff ff ff ff

.dcb 2, 5			# out: 05 00 05 00
.byte 0xff ; .balign 16, 0xff	# out: ff ff ff ff ff ff ff ff ff ff ff ff
.dcb.b 2, 5			# out: 05 05
.byte 0xff ; .balign 16, 0xff	# out: ff ff ff ff ff ff ff ff ff ff ff ff ff ff
.dcb.w 2, 5			# out: 05 00 05 00
.byte 0xff ; .balign 16, 0xff	# out: ff ff ff ff ff ff ff ff ff ff ff ff
.dcb.l 2, 5			# out: 05 00 00 00 05 00 00 00
.byte 0xff ; .balign 16, 0xff	# out: ff ff ff ff ff ff ff ff

.ds.s 2, 5			# out: 05 00 00 00 05 00 00 00
.byte 0xff ; .balign 16, 0xff	# out: ff ff ff ff ff ff ff ff
.ds.d 2, 5			# out: 05 00 00 00 00 00 00 00
				# out: 05 00 00 00 00 00 00 00
.byte 0xff ; .balign 16, 0xff	# out: ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff
# XXX: matches gas output, but why are there 12 instead of 10?
.ds.x 2, 5			# out: 05 00 00 00 00 00 00 00 00 00 00 00
				# out: 05 00 00 00 00 00 00 00 00 00 00 00
.byte 0xff ; .balign 16, 0xff	# out: ff ff ff ff ff ff ff ff

.ds 2, 5			# out: 05 00 05 00
.byte 0xff ; .balign 16, 0xff	# out: ff ff ff ff ff ff ff ff ff ff ff ff
.ds.b 2, 5			# out: 05 05
.byte 0xff ; .balign 16, 0xff	# out: ff ff ff ff ff ff ff ff ff ff ff ff ff ff
.ds.w 2, 5			# out: 05 00 05 00
.byte 0xff ; .balign 16, 0xff	# out: ff ff ff ff ff ff ff ff ff ff ff ff
.ds.l 2, 5			# out: 05 00 00 00 05 00 00 00
.byte 0xff ; .balign 16, 0xff	# out: ff ff ff ff ff ff ff ff

.ds.p 2, 5			# out: 05 00 00 00 00 00 00 00 00 00 00 00
				# out: 05 00 00 00 00 00 00 00 00 00 00 00
.byte 0xff ; .balign 16, 0xff	# out: ff ff ff ff ff ff ff ff

