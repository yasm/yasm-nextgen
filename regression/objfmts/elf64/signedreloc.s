# [oformat elf64]
.text
label:
	addq label2, %r11
	addq $label2, %r11
	addq label2, %rax
	addq $label2, %rax
label2:
	movq $label, %rax
	movq $label, %rsi

