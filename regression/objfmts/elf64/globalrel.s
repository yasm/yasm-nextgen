# [oformat elf64]
.globl label1
.globl label2
.data
.byte 5
label1:
.byte 6, 7, 8
label2:
.byte 9
label3:
.word label1
.word label2
.word label3
.word label2-label1
.word label3-label2

.globl label4
.extern label6
.text
label4:
call label4
label5:
call label5
call label6
