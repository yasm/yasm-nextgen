.code64
movw %cs, 0(%rdx)	# out: 8c 0a
movw %es, 0(%rdx)	# out: 8c 02
movw %ds, 0(%rdx)	# out: 8c 1a
movl %ss,%eax		# out: 8c d0

cs movw %ax, 0(%rdx)	# out: 2e 66 89 02
es movw %ax, 0(%rdx)	# out: 26 66 89 02
ds movw %ax, 0(%rdx)	# out: 3e 66 89 02
ss movw %ax, 0(%rdx)	# out: 36 66 89 02

movw %ax, %cs:0(%rdx)	# out: 2e 66 89 02
movw %ax, %es:0(%rdx)	# out: 26 66 89 02
movw %ax, %ds:0(%rdx)	# out: 3e 66 89 02
movw %ax, %ss:0(%rdx)	# out: 36 66 89 02

