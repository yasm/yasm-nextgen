es mov ax, [fs:0]	; out: 26 a1 00 00
es fs mov ax, [0]	; out: 26 a1 00 00
mov ax, [es:fs:0]	; out: 26 a1 00 00
es fs mov ax, [gs:0]	; out: 26 a1 00 00
mov ax, [es:fs:gs:0]	; out: 26 a1 00 00
es fs gs mov ax, [0]	; out: 26 a1 00 00
es cvtsi2ss xmm0, [eax]	; out: 26 67 f3 0f 2a 00
