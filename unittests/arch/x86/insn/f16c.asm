[bits 64]
vcvtph2ps ymm1, xmm2		; c4 e2 7d 13 ca
vcvtph2ps ymm1, oword [0]	; c4 e2 7d 13 0c 25 00 00 00 00
vcvtph2ps xmm1, xmm2		; c4 e2 79 13 ca
vcvtph2ps xmm1, qword [0]	; c4 e2 79 13 0c 25 00 00 00 00

vcvtps2ph xmm1, ymm2, 4		; c4 e3 7d 1d d1 04
vcvtps2ph oword [0], ymm2, 8	; c4 e3 7d 1d 14 25 00 00 00 00 08
vcvtps2ph xmm1, xmm2, 3		; c4 e3 79 1d d1 03
vcvtps2ph qword [0], xmm2, 5	; c4 e3 79 1d 14 25 00 00 00 00 05
