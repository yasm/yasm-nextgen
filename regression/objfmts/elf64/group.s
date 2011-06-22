# [oformat elf64]
.data
.bss
.section .text.foo,"axG",@progbits,foo,comdat

.section .text.foo2,"axG",@progbits,foo,comdat

.section .text.foo3,"axG",@progbits,foo3,comdat
foo3:
.long foo2
.long .text.foo
.long foo3

.section .text.foo4,"axG",@progbits,foo2

