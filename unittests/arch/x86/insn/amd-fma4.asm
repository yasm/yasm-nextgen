[bits 64]
vfmaddpd xmm0, xmm1, xmm2, xmm3 		; c4 e3 61 69 c2 10
vfmaddpd xmm0, xmm1, [rax], xmm3 		; c4 e3 61 69 00 10
vfmaddpd xmm0, xmm1, dqword [rax], xmm3 	; c4 e3 61 69 00 10
vfmaddpd xmm0, xmm1, xmm2, [rax] 		; c4 e3 e9 69 00 10
vfmaddpd xmm0, xmm1, xmm2, dqword [rax] 	; c4 e3 e9 69 00 10
vfmaddpd ymm0, ymm1, ymm2, ymm3 		; c4 e3 65 69 c2 10
vfmaddpd ymm0, ymm1, [rax], ymm3 		; c4 e3 65 69 00 10
vfmaddpd ymm0, ymm1, yword [rax], ymm3 		; c4 e3 65 69 00 10
vfmaddpd ymm0, ymm1, ymm2, [rax] 		; c4 e3 ed 69 00 10
vfmaddpd ymm0, ymm1, ymm2, yword [rax] 		; c4 e3 ed 69 00 10

vfmaddps xmm0, xmm1, xmm2, xmm3 		; c4 e3 61 68 c2 10
vfmaddps xmm0, xmm1, dqword [rax], xmm3 	; c4 e3 61 68 00 10
vfmaddps xmm0, xmm1, xmm2, dqword [rax] 	; c4 e3 e9 68 00 10
vfmaddps ymm0, ymm1, ymm2, ymm3 		; c4 e3 65 68 c2 10
vfmaddps ymm0, ymm1, yword [rax], ymm3 		; c4 e3 65 68 00 10
vfmaddps ymm0, ymm1, ymm2, yword [rax] 		; c4 e3 ed 68 00 10

vfmaddsd xmm0, xmm1, xmm2, xmm3 		; c4 e3 61 6b c2 10
vfmaddsd xmm0, xmm1, [rax], xmm3 		; c4 e3 61 6b 00 10
vfmaddsd xmm0, xmm1, qword [rax], xmm3 		; c4 e3 61 6b 00 10
vfmaddsd xmm0, xmm1, xmm2, [rax] 		; c4 e3 e9 6b 00 10
vfmaddsd xmm0, xmm1, xmm2, qword [rax] 		; c4 e3 e9 6b 00 10

vfmaddss xmm0, xmm1, xmm2, xmm3 		; c4 e3 61 6a c2 10
vfmaddss xmm0, xmm1, dword [rax], xmm3 		; c4 e3 61 6a 00 10
vfmaddss xmm0, xmm1, xmm2, dword [rax] 		; c4 e3 e9 6a 00 10

vfmaddsubpd xmm0, xmm1, xmm2, xmm3 		; c4 e3 61 5d c2 10
vfmaddsubpd xmm0, xmm1, dqword [rax], xmm3 	; c4 e3 61 5d 00 10
vfmaddsubpd xmm0, xmm1, xmm2, dqword [rax] 	; c4 e3 e9 5d 00 10
vfmaddsubpd ymm0, ymm1, ymm2, ymm3 		; c4 e3 65 5d c2 10
vfmaddsubpd ymm0, ymm1, yword [rax], ymm3 	; c4 e3 65 5d 00 10
vfmaddsubpd ymm0, ymm1, ymm2, yword [rax] 	; c4 e3 ed 5d 00 10

vfmaddsubps xmm0, xmm1, xmm2, xmm3 		; c4 e3 61 5c c2 10
vfmaddsubps xmm0, xmm1, dqword [rax], xmm3 	; c4 e3 61 5c 00 10
vfmaddsubps xmm0, xmm1, xmm2, dqword [rax] 	; c4 e3 e9 5c 00 10
vfmaddsubps ymm0, ymm1, ymm2, ymm3 		; c4 e3 65 5c c2 10
vfmaddsubps ymm0, ymm1, yword [rax], ymm3 	; c4 e3 65 5c 00 10
vfmaddsubps ymm0, ymm1, ymm2, yword [rax] 	; c4 e3 ed 5c 00 10

vfmsubpd xmm0, xmm1, xmm2, xmm3 		; c4 e3 61 6d c2 10
vfmsubpd xmm0, xmm1, dqword [rax], xmm3 	; c4 e3 61 6d 00 10
vfmsubpd xmm0, xmm1, xmm2, dqword [rax] 	; c4 e3 e9 6d 00 10
vfmsubpd ymm0, ymm1, ymm2, ymm3 		; c4 e3 65 6d c2 10
vfmsubpd ymm0, ymm1, yword [rax], ymm3 		; c4 e3 65 6d 00 10
vfmsubpd ymm0, ymm1, ymm2, yword [rax] 		; c4 e3 ed 6d 00 10

vfmsubps xmm0, xmm1, xmm2, xmm3 		; c4 e3 61 6c c2 10
vfmsubps xmm0, xmm1, dqword [rax], xmm3 	; c4 e3 61 6c 00 10
vfmsubps xmm0, xmm1, xmm2, dqword [rax] 	; c4 e3 e9 6c 00 10
vfmsubps ymm0, ymm1, ymm2, ymm3 		; c4 e3 65 6c c2 10
vfmsubps ymm0, ymm1, yword [rax], ymm3 		; c4 e3 65 6c 00 10
vfmsubps ymm0, ymm1, ymm2, yword [rax] 		; c4 e3 ed 6c 00 10

vfmsubsd xmm0, xmm1, xmm2, xmm3 		; c4 e3 61 6f c2 10
vfmsubsd xmm0, xmm1, qword [rax], xmm3 		; c4 e3 61 6f 00 10
vfmsubsd xmm0, xmm1, xmm2, qword [rax] 		; c4 e3 e9 6f 00 10

vfmsubss xmm0, xmm1, xmm2, xmm3 		; c4 e3 61 6e c2 10
vfmsubss xmm0, xmm1, dword [rax], xmm3 		; c4 e3 61 6e 00 10
vfmsubss xmm0, xmm1, xmm2, dword [rax] 		; c4 e3 e9 6e 00 10

vfnmaddpd xmm0, xmm1, xmm2, xmm3 		; c4 e3 61 79 c2 10
vfnmaddpd xmm0, xmm1, dqword [rax], xmm3 	; c4 e3 61 79 00 10
vfnmaddpd xmm0, xmm1, xmm2, dqword [rax] 	; c4 e3 e9 79 00 10
vfnmaddpd ymm0, ymm1, ymm2, ymm3 		; c4 e3 65 79 c2 10
vfnmaddpd ymm0, ymm1, yword [rax], ymm3 	; c4 e3 65 79 00 10
vfnmaddpd ymm0, ymm1, ymm2, yword [rax] 	; c4 e3 ed 79 00 10

vfnmaddps xmm0, xmm1, xmm2, xmm3 		; c4 e3 61 78 c2 10
vfnmaddps xmm0, xmm1, dqword [rax], xmm3 	; c4 e3 61 78 00 10
vfnmaddps xmm0, xmm1, xmm2, dqword [rax] 	; c4 e3 e9 78 00 10
vfnmaddps ymm0, ymm1, ymm2, ymm3 		; c4 e3 65 78 c2 10
vfnmaddps ymm0, ymm1, yword [rax], ymm3 	; c4 e3 65 78 00 10
vfnmaddps ymm0, ymm1, ymm2, yword [rax] 	; c4 e3 ed 78 00 10

vfnmaddsd xmm0, xmm1, xmm2, xmm3 		; c4 e3 61 7b c2 10
vfnmaddsd xmm0, xmm1, qword [rax], xmm3 	; c4 e3 61 7b 00 10
vfnmaddsd xmm0, xmm1, xmm2, qword [rax] 	; c4 e3 e9 7b 00 10

vfnmaddss xmm0, xmm1, xmm2, xmm3 		; c4 e3 61 7a c2 10
vfnmaddss xmm0, xmm1, dword [rax], xmm3 	; c4 e3 61 7a 00 10
vfnmaddss xmm0, xmm1, xmm2, dword [rax] 	; c4 e3 e9 7a 00 10

vfnmsubpd xmm0, xmm1, xmm2, xmm3 		; c4 e3 61 7d c2 10
vfnmsubpd xmm0, xmm1, dqword [rax], xmm3 	; c4 e3 61 7d 00 10
vfnmsubpd xmm0, xmm1, xmm2, dqword [rax] 	; c4 e3 e9 7d 00 10
vfnmsubpd ymm0, ymm1, ymm2, ymm3 		; c4 e3 65 7d c2 10
vfnmsubpd ymm0, ymm1, yword [rax], ymm3 	; c4 e3 65 7d 00 10
vfnmsubpd ymm0, ymm1, ymm2, yword [rax] 	; c4 e3 ed 7d 00 10

vfnmsubps xmm0, xmm1, xmm2, xmm3 		; c4 e3 61 7c c2 10
vfnmsubps xmm0, xmm1, dqword [rax], xmm3 	; c4 e3 61 7c 00 10
vfnmsubps xmm0, xmm1, xmm2, dqword [rax] 	; c4 e3 e9 7c 00 10
vfnmsubps ymm0, ymm1, ymm2, ymm3 		; c4 e3 65 7c c2 10
vfnmsubps ymm0, ymm1, yword [rax], ymm3 	; c4 e3 65 7c 00 10
vfnmsubps ymm0, ymm1, ymm2, yword [rax] 	; c4 e3 ed 7c 00 10

vfnmsubsd xmm0, xmm1, xmm2, xmm3 		; c4 e3 61 7f c2 10
vfnmsubsd xmm0, xmm1, qword [rax], xmm3 	; c4 e3 61 7f 00 10
vfnmsubsd xmm0, xmm1, xmm2, qword [rax] 	; c4 e3 e9 7f 00 10

vfnmsubss xmm0, xmm1, xmm2, xmm3 		; c4 e3 61 7e c2 10
vfnmsubss xmm0, xmm1, dword [rax], xmm3 	; c4 e3 61 7e 00 10
vfnmsubss xmm0, xmm1, xmm2, dword [rax] 	; c4 e3 e9 7e 00 10

