# [oformat elf32]
.globl a
.type a, @function
.size a, 50
.text
a:
.weak b
b = a

