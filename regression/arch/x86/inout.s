.code32
inb (%dx), %al		# out: ec
inw (%dx), %ax		# out: 66 ed
inl (%dx), %eax		# out: ed
inb %dx, %al		# out: ec
inw %dx, %ax		# out: 66 ed
inl %dx, %eax		# out: ed
inb (%dx)		# out: ec
inw (%dx)		# out: 66 ed
inl (%dx)		# out: ed
inb %dx			# out: ec
inw %dx			# out: 66 ed
inl %dx			# out: ed

outb %al, (%dx)		# out: ee
outw %ax, (%dx)		# out: 66 ef
outl %eax, (%dx)	# out: ef
outb %al, %dx		# out: ee
outw %ax, %dx		# out: 66 ef
outl %eax, %dx		# out: ef
outb (%dx)		# out: ee
outw (%dx)		# out: 66 ef
outl (%dx)		# out: ef
outb %dx		# out: ee
outw %dx		# out: 66 ef
outl %dx		# out: ef
