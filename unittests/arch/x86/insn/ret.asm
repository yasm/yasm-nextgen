[bits 16]
ret 		; c3
ret 4 		; c2 04 00
ret word 2 	; c2 02 00
retn 6 		; c2 06 00
retn word 2 	; c2 02 00
retf 		; cb
retf 8 		; ca 08 00
retf word 2 	; ca 02 00

[bits 32]
ret 		; c3
ret 4 		; c2 04 00
ret word 2 	; c2 02 00
retn 6 		; c2 06 00
retn word 2 	; c2 02 00
retf 		; cb
retf 8 		; ca 08 00
retf word 2 	; ca 02 00

[bits 64]
ret 		; c3
ret 4 		; c2 04 00
ret word 2 	; c2 02 00
retn 6 		; c2 06 00
retn word 2 	; c2 02 00
retf 		; 48 cb
retf 8 		; 48 ca 08 00
retf word 2 	; 48 ca 02 00

