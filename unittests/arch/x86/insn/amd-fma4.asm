[bits 64]
vfmaddpd xmm0, xmm1, xmm2, xmm3 		; c4 e3 71 69 c2 30
vfmaddpd xmm0, xmm1, [rax], xmm3 		; c4 e3 71 69 00 30
vfmaddpd xmm0, xmm1, dqword [rax], xmm3 	; c4 e3 71 69 00 30
vfmaddpd xmm0, xmm1, xmm2, [rax] 		; c4 e3 f1 69 00 20
vfmaddpd xmm0, xmm1, xmm2, dqword [rax] 	; c4 e3 f1 69 00 20
vfmaddpd ymm0, ymm1, ymm2, ymm3 		; c4 e3 75 69 c2 30
vfmaddpd ymm0, ymm1, [rax], ymm3 		; c4 e3 75 69 00 30
vfmaddpd ymm0, ymm1, yword [rax], ymm3 		; c4 e3 75 69 00 30
vfmaddpd ymm0, ymm1, ymm2, [rax] 		; c4 e3 f5 69 00 20
vfmaddpd ymm0, ymm1, ymm2, yword [rax] 		; c4 e3 f5 69 00 20

vfmaddps xmm0, xmm1, xmm2, xmm3 		; c4 e3 71 68 c2 30
vfmaddps xmm0, xmm1, dqword [rax], xmm3 	; c4 e3 71 68 00 30
vfmaddps xmm0, xmm1, xmm2, dqword [rax] 	; c4 e3 f1 68 00 20
vfmaddps ymm0, ymm1, ymm2, ymm3 		; c4 e3 75 68 c2 30
vfmaddps ymm0, ymm1, yword [rax], ymm3 		; c4 e3 75 68 00 30
vfmaddps ymm0, ymm1, ymm2, yword [rax] 		; c4 e3 f5 68 00 20

vfmaddsd xmm0, xmm1, xmm2, xmm3 		; c4 e3 71 6b c2 30
vfmaddsd xmm0, xmm1, [rax], xmm3 		; c4 e3 71 6b 00 30
vfmaddsd xmm0, xmm1, qword [rax], xmm3 		; c4 e3 71 6b 00 30
vfmaddsd xmm0, xmm1, xmm2, [rax] 		; c4 e3 f1 6b 00 20
vfmaddsd xmm0, xmm1, xmm2, qword [rax] 		; c4 e3 f1 6b 00 20

vfmaddss xmm0, xmm1, xmm2, xmm3 		; c4 e3 71 6a c2 30
vfmaddss xmm0, xmm1, dword [rax], xmm3 		; c4 e3 71 6a 00 30
vfmaddss xmm0, xmm1, xmm2, dword [rax] 		; c4 e3 f1 6a 00 20

vfmaddsubpd xmm0, xmm1, xmm2, xmm3 		; c4 e3 71 5d c2 30
vfmaddsubpd xmm0, xmm1, dqword [rax], xmm3 	; c4 e3 71 5d 00 30
vfmaddsubpd xmm0, xmm1, xmm2, dqword [rax] 	; c4 e3 f1 5d 00 20
vfmaddsubpd ymm0, ymm1, ymm2, ymm3 		; c4 e3 75 5d c2 30
vfmaddsubpd ymm0, ymm1, yword [rax], ymm3 	; c4 e3 75 5d 00 30
vfmaddsubpd ymm0, ymm1, ymm2, yword [rax] 	; c4 e3 f5 5d 00 20

vfmaddsubps xmm0, xmm1, xmm2, xmm3 		; c4 e3 71 5c c2 30
vfmaddsubps xmm0, xmm1, dqword [rax], xmm3 	; c4 e3 71 5c 00 30
vfmaddsubps xmm0, xmm1, xmm2, dqword [rax] 	; c4 e3 f1 5c 00 20
vfmaddsubps ymm0, ymm1, ymm2, ymm3 		; c4 e3 75 5c c2 30
vfmaddsubps ymm0, ymm1, yword [rax], ymm3 	; c4 e3 75 5c 00 30
vfmaddsubps ymm0, ymm1, ymm2, yword [rax] 	; c4 e3 f5 5c 00 20

vfmsubpd xmm0, xmm1, xmm2, xmm3 		; c4 e3 71 6d c2 30
vfmsubpd xmm0, xmm1, dqword [rax], xmm3 	; c4 e3 71 6d 00 30
vfmsubpd xmm0, xmm1, xmm2, dqword [rax] 	; c4 e3 f1 6d 00 20
vfmsubpd ymm0, ymm1, ymm2, ymm3 		; c4 e3 75 6d c2 30
vfmsubpd ymm0, ymm1, yword [rax], ymm3 		; c4 e3 75 6d 00 30
vfmsubpd ymm0, ymm1, ymm2, yword [rax] 		; c4 e3 f5 6d 00 20

vfmsubps xmm0, xmm1, xmm2, xmm3 		; c4 e3 71 6c c2 30
vfmsubps xmm0, xmm1, dqword [rax], xmm3 	; c4 e3 71 6c 00 30
vfmsubps xmm0, xmm1, xmm2, dqword [rax] 	; c4 e3 f1 6c 00 20
vfmsubps ymm0, ymm1, ymm2, ymm3 		; c4 e3 75 6c c2 30
vfmsubps ymm0, ymm1, yword [rax], ymm3 		; c4 e3 75 6c 00 30
vfmsubps ymm0, ymm1, ymm2, yword [rax] 		; c4 e3 f5 6c 00 20

vfmsubsd xmm0, xmm1, xmm2, xmm3 		; c4 e3 71 6f c2 30
vfmsubsd xmm0, xmm1, qword [rax], xmm3 		; c4 e3 71 6f 00 30
vfmsubsd xmm0, xmm1, xmm2, qword [rax] 		; c4 e3 f1 6f 00 20

vfmsubss xmm0, xmm1, xmm2, xmm3 		; c4 e3 71 6e c2 30
vfmsubss xmm0, xmm1, dword [rax], xmm3 		; c4 e3 71 6e 00 30
vfmsubss xmm0, xmm1, xmm2, dword [rax] 		; c4 e3 f1 6e 00 20

vfnmaddpd xmm0, xmm1, xmm2, xmm3 		; c4 e3 71 79 c2 30
vfnmaddpd xmm0, xmm1, dqword [rax], xmm3 	; c4 e3 71 79 00 30
vfnmaddpd xmm0, xmm1, xmm2, dqword [rax] 	; c4 e3 f1 79 00 20
vfnmaddpd ymm0, ymm1, ymm2, ymm3 		; c4 e3 75 79 c2 30
vfnmaddpd ymm0, ymm1, yword [rax], ymm3 	; c4 e3 75 79 00 30
vfnmaddpd ymm0, ymm1, ymm2, yword [rax] 	; c4 e3 f5 79 00 20

vfnmaddps xmm0, xmm1, xmm2, xmm3 		; c4 e3 71 78 c2 30
vfnmaddps xmm0, xmm1, dqword [rax], xmm3 	; c4 e3 71 78 00 30
vfnmaddps xmm0, xmm1, xmm2, dqword [rax] 	; c4 e3 f1 78 00 20
vfnmaddps ymm0, ymm1, ymm2, ymm3 		; c4 e3 75 78 c2 30
vfnmaddps ymm0, ymm1, yword [rax], ymm3 	; c4 e3 75 78 00 30
vfnmaddps ymm0, ymm1, ymm2, yword [rax] 	; c4 e3 f5 78 00 20

vfnmaddsd xmm0, xmm1, xmm2, xmm3 		; c4 e3 71 7b c2 30
vfnmaddsd xmm0, xmm1, qword [rax], xmm3 	; c4 e3 71 7b 00 30
vfnmaddsd xmm0, xmm1, xmm2, qword [rax] 	; c4 e3 f1 7b 00 20

vfnmaddss xmm0, xmm1, xmm2, xmm3 		; c4 e3 71 7a c2 30
vfnmaddss xmm0, xmm1, dword [rax], xmm3 	; c4 e3 71 7a 00 30
vfnmaddss xmm0, xmm1, xmm2, dword [rax] 	; c4 e3 f1 7a 00 20

vfnmsubpd xmm0, xmm1, xmm2, xmm3 		; c4 e3 71 7d c2 30
vfnmsubpd xmm0, xmm1, dqword [rax], xmm3 	; c4 e3 71 7d 00 30
vfnmsubpd xmm0, xmm1, xmm2, dqword [rax] 	; c4 e3 f1 7d 00 20
vfnmsubpd ymm0, ymm1, ymm2, ymm3 		; c4 e3 75 7d c2 30
vfnmsubpd ymm0, ymm1, yword [rax], ymm3 	; c4 e3 75 7d 00 30
vfnmsubpd ymm0, ymm1, ymm2, yword [rax] 	; c4 e3 f5 7d 00 20

vfnmsubps xmm0, xmm1, xmm2, xmm3 		; c4 e3 71 7c c2 30
vfnmsubps xmm0, xmm1, dqword [rax], xmm3 	; c4 e3 71 7c 00 30
vfnmsubps xmm0, xmm1, xmm2, dqword [rax] 	; c4 e3 f1 7c 00 20
vfnmsubps ymm0, ymm1, ymm2, ymm3 		; c4 e3 75 7c c2 30
vfnmsubps ymm0, ymm1, yword [rax], ymm3 	; c4 e3 75 7c 00 30
vfnmsubps ymm0, ymm1, ymm2, yword [rax] 	; c4 e3 f5 7c 00 20

vfnmsubsd xmm0, xmm1, xmm2, xmm3 		; c4 e3 71 7f c2 30
vfnmsubsd xmm0, xmm1, qword [rax], xmm3 	; c4 e3 71 7f 00 30
vfnmsubsd xmm0, xmm1, xmm2, qword [rax] 	; c4 e3 f1 7f 00 20

vfnmsubss xmm0, xmm1, xmm2, xmm3 		; c4 e3 71 7e c2 30
vfnmsubss xmm0, xmm1, dword [rax], xmm3 	; c4 e3 71 7e 00 30
vfnmsubss xmm0, xmm1, xmm2, dword [rax] 	; c4 e3 f1 7e 00 20

