[bits 64]

pclmulqdq xmm1, xmm2, 5 			; 66 0f 3a 44 ca 05
pclmulqdq xmm1, [rax], byte 5 			; 66 0f 3a 44 08 05
pclmulqdq xmm1, dqword [rax], 5 		; 66 0f 3a 44 08 05
vpclmulqdq xmm1, xmm2, 0x10 			; c4 e3 71 44 ca 10
vpclmulqdq xmm1, dqword [rbx], 0x10 		; c4 e3 71 44 0b 10
vpclmulqdq xmm0, xmm1, xmm2, 0x10 		; c4 e3 71 44 c2 10
vpclmulqdq xmm0, xmm1, dqword [rbx], 0x10	; c4 e3 71 44 03 10

pclmullqlqdq xmm1, xmm2 			; 66 0f 3a 44 ca 00
pclmullqlqdq xmm1, [rax] 			; 66 0f 3a 44 08 00
pclmullqlqdq xmm1, dqword [rax] 		; 66 0f 3a 44 08 00
vpclmullqlqdq xmm1, xmm2 			; c4 e3 71 44 ca 00
vpclmullqlqdq xmm1, dqword [rbx] 		; c4 e3 71 44 0b 00
vpclmullqlqdq xmm0, xmm1, xmm2 			; c4 e3 71 44 c2 00
vpclmullqlqdq xmm0, xmm1, dqword [rbx] 		; c4 e3 71 44 03 00

pclmulhqlqdq xmm1, xmm2 			; 66 0f 3a 44 ca 01
pclmulhqlqdq xmm1, [rax] 			; 66 0f 3a 44 08 01
pclmulhqlqdq xmm1, dqword [rax] 		; 66 0f 3a 44 08 01
vpclmulhqlqdq xmm1, xmm2 			; c4 e3 71 44 ca 01
vpclmulhqlqdq xmm1, dqword [rbx] 		; c4 e3 71 44 0b 01
vpclmulhqlqdq xmm0, xmm1, xmm2 			; c4 e3 71 44 c2 01
vpclmulhqlqdq xmm0, xmm1, dqword [rbx] 		; c4 e3 71 44 03 01

pclmullqhqdq xmm1, xmm2 			; 66 0f 3a 44 ca 10
pclmullqhqdq xmm1, [rax] 			; 66 0f 3a 44 08 10
pclmullqhqdq xmm1, dqword [rax] 		; 66 0f 3a 44 08 10
vpclmullqhqdq xmm1, xmm2 			; c4 e3 71 44 ca 10
vpclmullqhqdq xmm1, dqword [rbx] 		; c4 e3 71 44 0b 10
vpclmullqhqdq xmm0, xmm1, xmm2 			; c4 e3 71 44 c2 10
vpclmullqhqdq xmm0, xmm1, dqword [rbx] 		; c4 e3 71 44 03 10

pclmulhqhqdq xmm1, xmm2 			; 66 0f 3a 44 ca 11
pclmulhqhqdq xmm1, [rax] 			; 66 0f 3a 44 08 11
pclmulhqhqdq xmm1, dqword [rax] 		; 66 0f 3a 44 08 11
vpclmulhqhqdq xmm1, xmm2 			; c4 e3 71 44 ca 11
vpclmulhqhqdq xmm1, dqword [rbx] 		; c4 e3 71 44 0b 11
vpclmulhqhqdq xmm0, xmm1, xmm2 			; c4 e3 71 44 c2 11
vpclmulhqhqdq xmm0, xmm1, dqword [rbx] 		; c4 e3 71 44 03 11

