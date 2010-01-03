[bits 16]
extractps eax, xmm1, 5		; 66 0f 3a 17 c8 05
vextractps eax, xmm1, 5		; c4 e3 79 17 c8 05
pextrb eax, xmm1, 5		; 66 0f 3a 14 c8 05
vpextrb eax, xmm1, 5		; c4 e3 79 14 c8 05
pextrw eax, xmm1, 5		; 66 0f c5 c1 05
vpextrw eax, xmm1, 5		; c5 f9 c5 c1 05
pextrd eax, xmm1, 5		; 66 0f 3a 16 c8 05
vpextrd eax, xmm1, 5		; c4 e3 79 16 c8 05
vpinsrd xmm1, xmm2, eax, 5	; c4 e3 69 22 c8 05
