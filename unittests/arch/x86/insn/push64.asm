[bits 64]
push dword 50 			; 6a 32
push dword -1 			; 6a ff
push strict dword 50 		; 68 32 00 00 00
push strict dword -1 		; 68 ff ff ff ff
push dword 1000000000000 	; 68 00 10 a5 d4 [warning: value does not fit in signed 32 bit field]

