[bits 16]
psrlw mm0, 1 	; 0f 71 d0 01
psrld mm0, 1 	; 0f 72 d0 01
psrlq mm0, 1 	; 0f 73 d0 01
psraw mm1, 1 	; 0f 71 e1 01
psrad mm1, 1 	; 0f 72 e1 01

psrlw xmm0, 1 	; 66 0f 71 d0 01
psrld xmm0, 1 	; 66 0f 72 d0 01
psrlq xmm0, 1 	; 66 0f 73 d0 01
psraw xmm1, 1 	; 66 0f 71 e1 01
psrad xmm1, 1 	; 66 0f 72 e1 01

