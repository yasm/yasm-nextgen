# [yasm -f elf32 -g dwarf2pass -p gas]
	.section .rodata, "a",@progbits
	.section .text
	movl .LC52@GOT(%ebx),%eax

	.section .rodata
.LC52:
