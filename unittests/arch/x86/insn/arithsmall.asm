[bits 32]
and eax, 3584 			; 25 00 0e 00 00
and eax, 35 			; 83 e0 23
and eax, strict dword 3584 	; 25 00 0e 00 00
and eax, strict dword 35 	; 25 23 00 00 00
and eax, strict byte 3584 	; 83 e0 00 [warning: value does not fit in signed 8 bit field]
and eax, strict byte 35 	; 83 e0 23
and ebx, 3584 			; 81 e3 00 0e 00 00
and ebx, 35 			; 83 e3 23
and ebx, strict dword 3584 	; 81 e3 00 0e 00 00
and ebx, strict dword 35 	; 81 e3 23 00 00 00
and ebx, strict byte 3584 	; 83 e3 00 [warning: value does not fit in signed 8 bit field]
and ebx, strict byte 35		; 83 e3 23
