[bits 64]
aesenc xmm1, xmm2 			; 66 0f 38 dc ca
aesenc xmm1, [rax] 			; 66 0f 38 dc 08
aesenc xmm1, dqword [rax] 		; 66 0f 38 dc 08
aesenc xmm10, xmm12 			; 66 45 0f 38 dc d4
aesenc xmm10, [r15*4+rax] 		; 66 46 0f 38 dc 14 b8
aesenc xmm10, [r15*4+r14] 		; 66 47 0f 38 dc 14 be
vaesenc xmm1, xmm2 			; c4 e2 71 dc ca
vaesenc xmm1, [rax] 			; c4 e2 71 dc 08
vaesenc xmm1, dqword [rax] 		; c4 e2 71 dc 08
vaesenc xmm1, xmm2, xmm3 		; c4 e2 69 dc cb
vaesenc xmm1, xmm2, [rax] 		; c4 e2 69 dc 08
vaesenc xmm1, xmm2, dqword [rax] 	; c4 e2 69 dc 08

aesenclast xmm1, xmm2 			; 66 0f 38 dd ca
aesenclast xmm1, [rax] 			; 66 0f 38 dd 08
aesenclast xmm1, dqword [rax] 		; 66 0f 38 dd 08
vaesenclast xmm1, xmm2 			; c4 e2 71 dd ca
vaesenclast xmm1, [rax] 		; c4 e2 71 dd 08
vaesenclast xmm1, dqword [rax] 		; c4 e2 71 dd 08
vaesenclast xmm1, xmm2, xmm3 		; c4 e2 69 dd cb
vaesenclast xmm1, xmm2, [rax] 		; c4 e2 69 dd 08
vaesenclast xmm1, xmm2, dqword [rax] 	; c4 e2 69 dd 08

aesdec xmm1, xmm2 			; 66 0f 38 de ca
aesdec xmm1, [rax] 			; 66 0f 38 de 08
aesdec xmm1, dqword [rax] 		; 66 0f 38 de 08
vaesdec xmm1, xmm2 			; c4 e2 71 de ca
vaesdec xmm1, [rax] 			; c4 e2 71 de 08
vaesdec xmm1, dqword [rax] 		; c4 e2 71 de 08
vaesdec xmm1, xmm2, xmm3 		; c4 e2 69 de cb
vaesdec xmm1, xmm2, [rax] 		; c4 e2 69 de 08
vaesdec xmm1, xmm2, dqword [rax] 	; c4 e2 69 de 08

aesdeclast xmm1, xmm2 			; 66 0f 38 df ca
aesdeclast xmm1, [rax] 			; 66 0f 38 df 08
aesdeclast xmm1, dqword [rax] 		; 66 0f 38 df 08
vaesdeclast xmm1, xmm2 			; c4 e2 71 df ca
vaesdeclast xmm1, [rax] 		; c4 e2 71 df 08
vaesdeclast xmm1, dqword [rax] 		; c4 e2 71 df 08
vaesdeclast xmm1, xmm2, xmm3 		; c4 e2 69 df cb
vaesdeclast xmm1, xmm2, [rax] 		; c4 e2 69 df 08
vaesdeclast xmm1, xmm2, dqword [rax] 	; c4 e2 69 df 08

aesimc xmm1, xmm2 			; 66 0f 38 db ca
aesimc xmm1, [rax] 			; 66 0f 38 db 08
aesimc xmm1, dqword [rax] 		; 66 0f 38 db 08
vaesimc xmm1, xmm2 			; c4 e2 79 db ca
vaesimc xmm1, [rax] 			; c4 e2 79 db 08
vaesimc xmm1, dqword [rax] 		; c4 e2 79 db 08
; no 3-operand form

aeskeygenassist xmm1, xmm2, 5 		; 66 0f 3a df ca 05
aeskeygenassist xmm1, [rax], byte 5 	; 66 0f 3a df 08 05
aeskeygenassist xmm1, dqword [rax], 5 	; 66 0f 3a df 08 05
vaeskeygenassist xmm1, xmm2, 5 		; c4 e3 79 df ca 05
vaeskeygenassist xmm1, [rax], byte 5 	; c4 e3 79 df 08 05
vaeskeygenassist xmm1, dqword [rax], 5 	; c4 e3 79 df 08 05

pclmulqdq xmm1, xmm2, 5 		; 66 0f 3a 44 ca 05
pclmulqdq xmm1, [rax], byte 5 		; 66 0f 3a 44 08 05
pclmulqdq xmm1, dqword [rax], 5 	; 66 0f 3a 44 08 05

; pclmulqdq variants
pclmullqlqdq xmm1, xmm2 		; 66 0f 3a 44 ca 00
pclmullqlqdq xmm1, [rax] 		; 66 0f 3a 44 08 00
pclmullqlqdq xmm1, dqword [rax] 	; 66 0f 3a 44 08 00

pclmulhqlqdq xmm1, xmm2 		; 66 0f 3a 44 ca 01
pclmulhqlqdq xmm1, [rax] 		; 66 0f 3a 44 08 01
pclmulhqlqdq xmm1, dqword [rax] 	; 66 0f 3a 44 08 01

pclmullqhqdq xmm1, xmm2 		; 66 0f 3a 44 ca 10
pclmullqhqdq xmm1, [rax] 		; 66 0f 3a 44 08 10
pclmullqhqdq xmm1, dqword [rax] 	; 66 0f 3a 44 08 10

pclmulhqhqdq xmm1, xmm2 		; 66 0f 3a 44 ca 11
pclmulhqhqdq xmm1, [rax] 		; 66 0f 3a 44 08 11
pclmulhqhqdq xmm1, dqword [rax] 	; 66 0f 3a 44 08 11

