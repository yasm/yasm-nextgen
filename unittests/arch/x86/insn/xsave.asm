[bits 16]
xsave [0]	; 0f ae 26 00 00
xrstor [0]	; 0f ae 2e 00 00
xgetbv		; 0f 01 d0
xsetbv		; 0f 01 d1

xsaveopt [0]	; 0f ae 36 00 00
[bits 64]
xsaveopt64 [0]	; 48 0f ae 34 25 00 00 00 00
