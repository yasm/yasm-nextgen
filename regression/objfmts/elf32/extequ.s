# [oformat elf32]
.weak a
.globl c
.equ a, b
.equ c, d
call a
call b
call c
call d
.globl f
