.code16

.text

data32 ljmp *%gs:0	# out: 65 66 ff 2e 00 00
mov %gs:0, %ax		# out: 65 a1 00 00

