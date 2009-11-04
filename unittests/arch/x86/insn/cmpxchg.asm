[bits 64]
cmpxchg8b qword [0] 	; 0f c7 0c 25 00 00 00 00
cmpxchg8b [0] 		; 0f c7 0c 25 00 00 00 00
cmpxchg16b dqword [0] 	; 48 0f c7 0c 25 00 00 00 00
cmpxchg16b [0] 		; 48 0f c7 0c 25 00 00 00 00

