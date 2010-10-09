.code16
lret		# out: cb
lretw		# out: cb
lretl		# out: 66 cb
.code32
lret		# out: cb
lretw		# out: 66 cb
lretl		# out: cb
.code64
lret		# out: cb
lretw		# out: 66 cb
lretl		# out: cb
lretq		# out: 48 cb
