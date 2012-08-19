; Exhaustive test of AVX instructions
; Also includes based-upon SSE instructions for comparison
;
;  Copyright (C) 2008  Peter Johnson
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions
; are met:
; 1. Redistributions of source code must retain the above copyright
;    notice, this list of conditions and the following disclaimer.
; 2. Redistributions in binary form must reproduce the above copyright
;    notice, this list of conditions and the following disclaimer in the
;    documentation and/or other materials provided with the distribution.
;
; THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND OTHER CONTRIBUTORS ``AS IS''
; AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
; IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
; ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR OTHER CONTRIBUTORS BE
; LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
; CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
; SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
; INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
; CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
; ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
; POSSIBILITY OF SUCH DAMAGE.
;

[bits 64]
addpd xmm1, xmm2 			; 66 0f 58 ca
addpd xmm1, [rax] 			; 66 0f 58 08
addpd xmm1, dqword [rax] 		; 66 0f 58 08
addpd xmm10, xmm12 			; 66 45 0f 58 d4
addpd xmm10, [r15*4+rax] 		; 66 46 0f 58 14 b8
addpd xmm10, [r15*4+r14] 		; 66 47 0f 58 14 be

vaddpd xmm1, xmm2 			; c5 f1 58 ca
vaddpd xmm1, [rax] 			; c5 f1 58 08
vaddpd xmm1, dqword [rax] 		; c5 f1 58 08
vaddpd xmm10, xmm12 			; c4 41 29 58 d4
vaddpd xmm10, [r15*4+rax] 		; c4 21 29 58 14 b8
vaddpd xmm10, [r15*4+r14] 		; c4 01 29 58 14 be

vaddpd xmm1, xmm2, xmm3 		; c5 e9 58 cb
vaddpd xmm1, xmm2, [rax] 		; c5 e9 58 08
vaddpd xmm1, xmm2, dqword [rax] 	; c5 e9 58 08
vaddpd xmm10, xmm12, xmm13 		; c4 41 19 58 d5
vaddpd xmm10, xmm12, [r15*4+rax] 	; c4 21 19 58 14 b8
vaddpd xmm10, xmm12, [r15*4+r14] 	; c4 01 19 58 14 be

vaddpd ymm1, ymm2, ymm3 		; c5 ed 58 cb
vaddpd ymm1, ymm2, [rax] 		; c5 ed 58 08
vaddpd ymm1, ymm2, yword [rax] 		; c5 ed 58 08
vaddpd ymm10, ymm12, ymm13 		; c4 41 1d 58 d5
vaddpd ymm10, ymm12, [r15*4+rax] 	; c4 21 1d 58 14 b8
vaddpd ymm10, ymm12, [r15*4+r14] 	; c4 01 1d 58 14 be

; Further instructions won't test high 8 registers (validated above)
addps xmm1, xmm2 			; 0f 58 ca
addps xmm1, [rax] 			; 0f 58 08
addps xmm1, dqword [rax] 		; 0f 58 08
vaddps xmm1, xmm2 			; c5 f0 58 ca
vaddps xmm1, [rax] 			; c5 f0 58 08
vaddps xmm1, dqword [rax] 		; c5 f0 58 08
vaddps xmm1, xmm2, xmm3 		; c5 e8 58 cb
vaddps xmm1, xmm2, [rax] 		; c5 e8 58 08
vaddps xmm1, xmm2, dqword [rax] 	; c5 e8 58 08
vaddps ymm1, ymm2, ymm3 		; c5 ec 58 cb
vaddps ymm1, ymm2, [rax] 		; c5 ec 58 08
vaddps ymm1, ymm2, yword [rax] 		; c5 ec 58 08

addsd xmm1, xmm2 			; f2 0f 58 ca
addsd xmm1, [rax] 			; f2 0f 58 08
addsd xmm1, qword [rax] 		; f2 0f 58 08
vaddsd xmm1, xmm2 			; c5 f3 58 ca
vaddsd xmm1, [rax] 			; c5 f3 58 08
vaddsd xmm1, qword [rax] 		; c5 f3 58 08
vaddsd xmm1, xmm2, xmm3 		; c5 eb 58 cb
vaddsd xmm1, xmm2, [rax] 		; c5 eb 58 08
vaddsd xmm1, xmm2, qword [rax] 		; c5 eb 58 08

addss xmm1, xmm2 			; f3 0f 58 ca
addss xmm1, [rax] 			; f3 0f 58 08
addss xmm1, dword [rax] 		; f3 0f 58 08
vaddss xmm1, xmm2 			; c5 f2 58 ca
vaddss xmm1, [rax] 			; c5 f2 58 08
vaddss xmm1, dword [rax] 		; c5 f2 58 08
vaddss xmm1, xmm2, xmm3 		; c5 ea 58 cb
vaddss xmm1, xmm2, [rax] 		; c5 ea 58 08
vaddss xmm1, xmm2, dword [rax] 		; c5 ea 58 08

addsubpd xmm1, xmm2 			; 66 0f d0 ca
addsubpd xmm1, [rax] 			; 66 0f d0 08
addsubpd xmm1, dqword [rax] 		; 66 0f d0 08
vaddsubpd xmm1, xmm2 			; c5 f1 d0 ca
vaddsubpd xmm1, [rax] 			; c5 f1 d0 08
vaddsubpd xmm1, dqword [rax] 		; c5 f1 d0 08
vaddsubpd xmm1, xmm2, xmm3 		; c5 e9 d0 cb
vaddsubpd xmm1, xmm2, [rax] 		; c5 e9 d0 08
vaddsubpd xmm1, xmm2, dqword [rax] 	; c5 e9 d0 08
vaddsubpd ymm1, ymm2, ymm3 		; c5 ed d0 cb
vaddsubpd ymm1, ymm2, [rax] 		; c5 ed d0 08
vaddsubpd ymm1, ymm2, yword [rax] 	; c5 ed d0 08

addsubps xmm1, xmm2 			; f2 0f d0 ca
addsubps xmm1, [rax] 			; f2 0f d0 08
addsubps xmm1, dqword [rax] 		; f2 0f d0 08
vaddsubps xmm1, xmm2 			; c5 f3 d0 ca
vaddsubps xmm1, [rax] 			; c5 f3 d0 08
vaddsubps xmm1, dqword [rax] 		; c5 f3 d0 08
vaddsubps xmm1, xmm2, xmm3 		; c5 eb d0 cb
vaddsubps xmm1, xmm2, [rax] 		; c5 eb d0 08
vaddsubps xmm1, xmm2, dqword [rax] 	; c5 eb d0 08
vaddsubps ymm1, ymm2, ymm3 		; c5 ef d0 cb
vaddsubps ymm1, ymm2, [rax] 		; c5 ef d0 08
vaddsubps ymm1, ymm2, yword [rax] 	; c5 ef d0 08

andpd xmm1, xmm2 			; 66 0f 54 ca
andpd xmm1, [rax] 			; 66 0f 54 08
andpd xmm1, dqword [rax] 		; 66 0f 54 08
vandpd xmm1, xmm2 			; c5 f1 54 ca
vandpd xmm1, [rax] 			; c5 f1 54 08
vandpd xmm1, dqword [rax] 		; c5 f1 54 08
vandpd xmm1, xmm2, xmm3 		; c5 e9 54 cb
vandpd xmm1, xmm2, [rax] 		; c5 e9 54 08
vandpd xmm1, xmm2, dqword [rax] 	; c5 e9 54 08
vandpd ymm1, ymm2, ymm3 		; c5 ed 54 cb
vandpd ymm1, ymm2, [rax] 		; c5 ed 54 08
vandpd ymm1, ymm2, yword [rax] 		; c5 ed 54 08

andps xmm1, xmm2 			; 0f 54 ca
andps xmm1, [rax] 			; 0f 54 08
andps xmm1, dqword [rax] 		; 0f 54 08
vandps xmm1, xmm2 			; c5 f0 54 ca
vandps xmm1, [rax] 			; c5 f0 54 08
vandps xmm1, dqword [rax] 		; c5 f0 54 08
vandps xmm1, xmm2, xmm3 		; c5 e8 54 cb
vandps xmm1, xmm2, [rax] 		; c5 e8 54 08
vandps xmm1, xmm2, dqword [rax] 	; c5 e8 54 08
vandps ymm1, ymm2, ymm3 		; c5 ec 54 cb
vandps ymm1, ymm2, [rax] 		; c5 ec 54 08
vandps ymm1, ymm2, yword [rax] 		; c5 ec 54 08

andnpd xmm1, xmm2 			; 66 0f 55 ca
andnpd xmm1, [rax] 			; 66 0f 55 08
andnpd xmm1, dqword [rax] 		; 66 0f 55 08
vandnpd xmm1, xmm2 			; c5 f1 55 ca
vandnpd xmm1, [rax] 			; c5 f1 55 08
vandnpd xmm1, dqword [rax] 		; c5 f1 55 08
vandnpd xmm1, xmm2, xmm3 		; c5 e9 55 cb
vandnpd xmm1, xmm2, [rax] 		; c5 e9 55 08
vandnpd xmm1, xmm2, dqword [rax]	; c5 e9 55 08
vandnpd ymm1, ymm2, ymm3 		; c5 ed 55 cb
vandnpd ymm1, ymm2, [rax] 		; c5 ed 55 08
vandnpd ymm1, ymm2, yword [rax] 	; c5 ed 55 08

andnps xmm1, xmm2 			; 0f 55 ca
andnps xmm1, [rax] 			; 0f 55 08
andnps xmm1, dqword [rax] 		; 0f 55 08
vandnps xmm1, xmm2 			; c5 f0 55 ca
vandnps xmm1, [rax] 			; c5 f0 55 08
vandnps xmm1, dqword [rax] 		; c5 f0 55 08
vandnps xmm1, xmm2, xmm3 		; c5 e8 55 cb
vandnps xmm1, xmm2, [rax] 		; c5 e8 55 08
vandnps xmm1, xmm2, dqword [rax]	; c5 e8 55 08
vandnps ymm1, ymm2, ymm3 		; c5 ec 55 cb
vandnps ymm1, ymm2, [rax] 		; c5 ec 55 08
vandnps ymm1, ymm2, yword [rax] 	; c5 ec 55 08

blendpd xmm1, xmm2, 5 			; 66 0f 3a 0d ca 05
blendpd xmm1, [rax], byte 5 		; 66 0f 3a 0d 08 05
blendpd xmm1, dqword [rax], 5 		; 66 0f 3a 0d 08 05
vblendpd xmm1, xmm2, 5 			; c4 e3 71 0d ca 05
vblendpd xmm1, [rax], byte 5 		; c4 e3 71 0d 08 05
vblendpd xmm1, dqword [rax], 5 		; c4 e3 71 0d 08 05
vblendpd xmm1, xmm2, xmm3, 5 		; c4 e3 69 0d cb 05
vblendpd xmm1, xmm2, [rax], byte 5	; c4 e3 69 0d 08 05
vblendpd xmm1, xmm2, dqword [rax], 5 	; c4 e3 69 0d 08 05
vblendpd ymm1, ymm2, ymm3, 5 		; c4 e3 6d 0d cb 05
vblendpd ymm1, ymm2, [rax], byte 5 	; c4 e3 6d 0d 08 05
vblendpd ymm1, ymm2, yword [rax], 5 	; c4 e3 6d 0d 08 05

blendps xmm1, xmm2, 5 			; 66 0f 3a 0c ca 05
blendps xmm1, [rax], byte 5 		; 66 0f 3a 0c 08 05
blendps xmm1, dqword [rax], 5 		; 66 0f 3a 0c 08 05
vblendps xmm1, xmm2, 5 			; c4 e3 71 0c ca 05
vblendps xmm1, [rax], byte 5 		; c4 e3 71 0c 08 05
vblendps xmm1, dqword [rax], 5 		; c4 e3 71 0c 08 05
vblendps xmm1, xmm2, xmm3, 5 		; c4 e3 69 0c cb 05
vblendps xmm1, xmm2, [rax], byte 5 	; c4 e3 69 0c 08 05
vblendps xmm1, xmm2, dqword [rax], 5 	; c4 e3 69 0c 08 05
vblendps ymm1, ymm2, ymm3, 5 		; c4 e3 6d 0c cb 05
vblendps ymm1, ymm2, [rax], byte 5 	; c4 e3 6d 0c 08 05
vblendps ymm1, ymm2, yword [rax], 5 	; c4 e3 6d 0c 08 05

; blendvpd doesn't have vex-encoded version of implicit xmm0
blendvpd xmm1, xmm3 				; 66 0f 38 15 cb
blendvpd xmm1, [rax] 				; 66 0f 38 15 08
blendvpd xmm1, dqword [rax] 			; 66 0f 38 15 08
blendvpd xmm1, xmm3, xmm0 			; 66 0f 38 15 cb
blendvpd xmm1, [rax], xmm0 			; 66 0f 38 15 08
blendvpd xmm1, dqword [rax], xmm0 		; 66 0f 38 15 08
vblendvpd xmm1, xmm2, xmm3, xmm4 		; c4 e3 69 4b cb 40
vblendvpd xmm1, xmm2, [rax], xmm4 		; c4 e3 69 4b 08 40
vblendvpd xmm1, xmm2, dqword [rax], xmm4 	; c4 e3 69 4b 08 40
vblendvpd ymm1, ymm2, ymm3, ymm4 		; c4 e3 6d 4b cb 40
vblendvpd ymm1, ymm2, [rax], ymm4 		; c4 e3 6d 4b 08 40
vblendvpd ymm1, ymm2, yword [rax], ymm4 	; c4 e3 6d 4b 08 40

; blendvps doesn't have vex-encoded version of implicit xmm0
blendvps xmm1, xmm3 				; 66 0f 38 14 cb
blendvps xmm1, [rax] 				; 66 0f 38 14 08
blendvps xmm1, dqword [rax] 			; 66 0f 38 14 08
blendvps xmm1, xmm3, xmm0 			; 66 0f 38 14 cb
blendvps xmm1, [rax], xmm0 			; 66 0f 38 14 08
blendvps xmm1, dqword [rax], xmm0 		; 66 0f 38 14 08
vblendvps xmm1, xmm2, xmm3, xmm4 		; c4 e3 69 4a cb 40
vblendvps xmm1, xmm2, [rax], xmm4 		; c4 e3 69 4a 08 40
vblendvps xmm1, xmm2, dqword [rax], xmm4 	; c4 e3 69 4a 08 40
vblendvps ymm1, ymm2, ymm3, ymm4 		; c4 e3 6d 4a cb 40
vblendvps ymm1, ymm2, [rax], ymm4 		; c4 e3 6d 4a 08 40
vblendvps ymm1, ymm2, yword [rax], ymm4 	; c4 e3 6d 4a 08 40

vbroadcastss xmm1, [rax] 		; c4 e2 79 18 08
vbroadcastss xmm1, dword [rax] 		; c4 e2 79 18 08
vbroadcastss ymm1, [rax] 		; c4 e2 7d 18 08
vbroadcastss ymm1, dword [rax] 		; c4 e2 7d 18 08

vbroadcastsd ymm1, [rax] 		; c4 e2 7d 19 08
vbroadcastsd ymm1, qword [rax] 		; c4 e2 7d 19 08

vbroadcastf128 ymm1, [rax] 		; c4 e2 7d 1a 08
vbroadcastf128 ymm1, dqword [rax] 	; c4 e2 7d 1a 08

cmppd xmm1, xmm2, 5 			; 66 0f c2 ca 05
cmppd xmm1, [rax], byte 5 		; 66 0f c2 08 05
cmppd xmm1, dqword [rax], 5 		; 66 0f c2 08 05
vcmppd xmm1, xmm2, 5 			; c5 f1 c2 ca 05
vcmppd xmm1, [rax], byte 5 		; c5 f1 c2 08 05
vcmppd xmm1, dqword [rax], 5 		; c5 f1 c2 08 05
vcmppd xmm1, xmm2, xmm3, 5 		; c5 e9 c2 cb 05
vcmppd xmm1, xmm2, [rax], byte 5 	; c5 e9 c2 08 05
vcmppd xmm1, xmm2, dqword [rax], 5 	; c5 e9 c2 08 05
vcmppd ymm1, ymm2, ymm3, 5 		; c5 ed c2 cb 05
vcmppd ymm1, ymm2, [rax], byte 5 	; c5 ed c2 08 05
vcmppd ymm1, ymm2, yword [rax], 5 	; c5 ed c2 08 05

cmpps xmm1, xmm2, 5 			; 0f c2 ca 05
cmpps xmm1, [rax], byte 5 		; 0f c2 08 05
cmpps xmm1, dqword [rax], 5 		; 0f c2 08 05
vcmpps xmm1, xmm2, 5 			; c5 f0 c2 ca 05
vcmpps xmm1, [rax], byte 5 		; c5 f0 c2 08 05
vcmpps xmm1, dqword [rax], 5 		; c5 f0 c2 08 05
vcmpps xmm1, xmm2, xmm3, 5 		; c5 e8 c2 cb 05
vcmpps xmm1, xmm2, [rax], byte 5 	; c5 e8 c2 08 05
vcmpps xmm1, xmm2, dqword [rax], 5 	; c5 e8 c2 08 05
vcmpps ymm1, ymm2, ymm3, 5 		; c5 ec c2 cb 05
vcmpps ymm1, ymm2, [rax], byte 5 	; c5 ec c2 08 05
vcmpps ymm1, ymm2, yword [rax], 5 	; c5 ec c2 08 05

cmpsd xmm1, xmm2, 5 			; f2 0f c2 ca 05
cmpsd xmm1, [rax], byte 5 		; f2 0f c2 08 05
cmpsd xmm1, qword [rax], 5 		; f2 0f c2 08 05
vcmpsd xmm1, xmm2, 5 			; c5 f3 c2 ca 05
vcmpsd xmm1, [rax], byte 5 		; c5 f3 c2 08 05
vcmpsd xmm1, qword [rax], 5 		; c5 f3 c2 08 05
vcmpsd xmm1, xmm2, xmm3, 5 		; c5 eb c2 cb 05
vcmpsd xmm1, xmm2, [rax], byte 5 	; c5 eb c2 08 05
vcmpsd xmm1, xmm2, qword [rax], 5 	; c5 eb c2 08 05

cmpss xmm1, xmm2, 5 			; f3 0f c2 ca 05
cmpss xmm1, [rax], byte 5 		; f3 0f c2 08 05
cmpss xmm1, dword [rax], 5 		; f3 0f c2 08 05
vcmpss xmm1, xmm2, 5 			; c5 f2 c2 ca 05
vcmpss xmm1, [rax], byte 5 		; c5 f2 c2 08 05
vcmpss xmm1, dword [rax], 5 		; c5 f2 c2 08 05
vcmpss xmm1, xmm2, xmm3, 5 		; c5 ea c2 cb 05
vcmpss xmm1, xmm2, [rax], byte 5 	; c5 ea c2 08 05
vcmpss xmm1, xmm2, dword [rax], 5 	; c5 ea c2 08 05

comisd xmm1, xmm2 			; 66 0f 2f ca
comisd xmm1, [rax] 			; 66 0f 2f 08
comisd xmm1, qword [rax] 		; 66 0f 2f 08
vcomisd xmm1, xmm2 			; c5 f9 2f ca
vcomisd xmm1, [rax] 			; c5 f9 2f 08
vcomisd xmm1, qword [rax] 		; c5 f9 2f 08

comiss xmm1, xmm2 			; 0f 2f ca
comiss xmm1, [rax] 			; 0f 2f 08
comiss xmm1, dword [rax] 		; 0f 2f 08
vcomiss xmm1, xmm2 			; c5 f8 2f ca
vcomiss xmm1, [rax] 			; c5 f8 2f 08
vcomiss xmm1, dword [rax] 		; c5 f8 2f 08

cvtdq2pd xmm1, xmm2 			; f3 0f e6 ca
cvtdq2pd xmm1, [rax] 			; f3 0f e6 08
cvtdq2pd xmm1, qword [rax] 		; f3 0f e6 08
vcvtdq2pd xmm1, xmm2 			; c5 fa e6 ca
vcvtdq2pd xmm1, [rax] 			; c5 fa e6 08
vcvtdq2pd xmm1, qword [rax] 		; c5 fa e6 08
vcvtdq2pd ymm1, xmm2 			; c5 fe e6 ca
vcvtdq2pd ymm1, [rax] 			; c5 fe e6 08
vcvtdq2pd ymm1, dqword [rax] 		; c5 fe e6 08

cvtdq2ps xmm1, xmm2 			; 0f 5b ca
cvtdq2ps xmm1, [rax] 			; 0f 5b 08
cvtdq2ps xmm1, dqword [rax] 		; 0f 5b 08
vcvtdq2ps xmm1, xmm2 			; c5 f8 5b ca
vcvtdq2ps xmm1, [rax] 			; c5 f8 5b 08
vcvtdq2ps xmm1, dqword [rax] 		; c5 f8 5b 08
vcvtdq2ps ymm1, ymm2 			; c5 fc 5b ca
vcvtdq2ps ymm1, [rax] 			; c5 fc 5b 08
vcvtdq2ps ymm1, yword [rax] 		; c5 fc 5b 08

; These require memory operand size to be specified (in AVX version)
cvtpd2dq xmm1, xmm2 			; f2 0f e6 ca
cvtpd2dq xmm1, [rax] 			; f2 0f e6 08
cvtpd2dq xmm1, dqword [rax] 		; f2 0f e6 08
vcvtpd2dq xmm1, xmm2 			; c5 fb e6 ca
vcvtpd2dq xmm1, dqword [rax] 		; c5 fb e6 08
vcvtpd2dq xmm1, ymm2 			; c5 ff e6 ca
vcvtpd2dq xmm1, yword [rax] 		; c5 ff e6 08

cvtpd2ps xmm1, xmm2 			; 66 0f 5a ca
cvtpd2ps xmm1, [rax] 			; 66 0f 5a 08
cvtpd2ps xmm1, dqword [rax] 		; 66 0f 5a 08
vcvtpd2ps xmm1, xmm2 			; c5 f9 5a ca
vcvtpd2ps xmm1, dqword [rax] 		; c5 f9 5a 08
vcvtpd2ps xmm1, ymm2 			; c5 fd 5a ca
vcvtpd2ps xmm1, yword [rax] 		; c5 fd 5a 08

cvtps2dq xmm1, xmm2 			; 66 0f 5b ca
cvtps2dq xmm1, [rax] 			; 66 0f 5b 08
cvtps2dq xmm1, dqword [rax] 		; 66 0f 5b 08
vcvtps2dq xmm1, xmm2 			; c5 f9 5b ca
vcvtps2dq xmm1, [rax] 			; c5 f9 5b 08
vcvtps2dq xmm1, dqword [rax] 		; c5 f9 5b 08
vcvtps2dq ymm1, ymm2 			; c5 fd 5b ca
vcvtps2dq ymm1, [rax] 			; c5 fd 5b 08
vcvtps2dq ymm1, yword [rax] 		; c5 fd 5b 08

cvtps2pd xmm1, xmm2 			; 0f 5a ca
cvtps2pd xmm1, [rax] 			; 0f 5a 08
cvtps2pd xmm1, qword [rax] 		; 0f 5a 08
vcvtps2pd xmm1, xmm2 			; c5 f8 5a ca
vcvtps2pd xmm1, [rax] 			; c5 f8 5a 08
vcvtps2pd xmm1, qword [rax] 		; c5 f8 5a 08
vcvtps2pd ymm1, xmm2 			; c5 fc 5a ca
vcvtps2pd ymm1, [rax] 			; c5 fc 5a 08
vcvtps2pd ymm1, dqword [rax] 		; c5 fc 5a 08

cvtsd2si eax, xmm2 			; f2 0f 2d c2
cvtsd2si eax, [rax] 			; f2 0f 2d 00
cvtsd2si eax, qword [rax] 		; f2 0f 2d 00
vcvtsd2si eax, xmm2 			; c5 fb 2d c2
vcvtsd2si eax, [rax] 			; c5 fb 2d 00
vcvtsd2si eax, qword [rax] 		; c5 fb 2d 00
cvtsd2si rax, xmm2 			; f2 48 0f 2d c2
cvtsd2si rax, [rax] 			; f2 48 0f 2d 00
cvtsd2si rax, qword [rax] 		; f2 48 0f 2d 00
vcvtsd2si rax, xmm2 			; c4 e1 fb 2d c2
vcvtsd2si rax, [rax] 			; c4 e1 fb 2d 00
vcvtsd2si rax, qword [rax] 		; c4 e1 fb 2d 00

cvtsd2ss xmm1, xmm2 			; f2 0f 5a ca
cvtsd2ss xmm1, [rax] 			; f2 0f 5a 08
cvtsd2ss xmm1, qword [rax] 		; f2 0f 5a 08
vcvtsd2ss xmm1, xmm2 			; c5 f3 5a ca
vcvtsd2ss xmm1, [rax] 			; c5 f3 5a 08
vcvtsd2ss xmm1, qword [rax] 		; c5 f3 5a 08
vcvtsd2ss xmm1, xmm2, xmm3 		; c5 eb 5a cb
vcvtsd2ss xmm1, xmm2, [rax] 		; c5 eb 5a 08
vcvtsd2ss xmm1, xmm2, qword [rax] 	; c5 eb 5a 08

; unsized not valid
cvtsi2sd xmm1, eax 			; f2 0f 2a c8
cvtsi2sd xmm1, dword [rax] 		; f2 0f 2a 08
vcvtsi2sd xmm1, eax 			; c5 f3 2a c8
vcvtsi2sd xmm1, dword [rax] 		; c5 f3 2a 08
vcvtsi2sd xmm1, xmm2, eax 		; c5 eb 2a c8
vcvtsi2sd xmm1, xmm2, dword [rax] 	; c5 eb 2a 08
cvtsi2sd xmm1, rax 			; f2 48 0f 2a c8
cvtsi2sd xmm1, qword [rax] 		; f2 48 0f 2a 08
vcvtsi2sd xmm1, rax 			; c4 e1 f3 2a c8
vcvtsi2sd xmm1, qword [rax] 		; c4 e1 f3 2a 08
vcvtsi2sd xmm1, xmm2, rax 		; c4 e1 eb 2a c8
vcvtsi2sd xmm1, xmm2, qword [rax] 	; c4 e1 eb 2a 08

cvtsi2ss xmm1, eax 			; f3 0f 2a c8
cvtsi2ss xmm1, dword [rax] 		; f3 0f 2a 08
vcvtsi2ss xmm1, eax 			; c5 f2 2a c8
vcvtsi2ss xmm1, dword [rax] 		; c5 f2 2a 08
vcvtsi2ss xmm1, xmm2, eax 		; c5 ea 2a c8
vcvtsi2ss xmm1, xmm2, dword [rax] 	; c5 ea 2a 08
cvtsi2ss xmm1, rax 			; f3 48 0f 2a c8
cvtsi2ss xmm1, qword [rax] 		; f3 48 0f 2a 08
vcvtsi2ss xmm1, rax 			; c4 e1 f2 2a c8
vcvtsi2ss xmm1, qword [rax] 		; c4 e1 f2 2a 08
vcvtsi2ss xmm1, xmm2, rax 		; c4 e1 ea 2a c8
vcvtsi2ss xmm1, xmm2, qword [rax] 	; c4 e1 ea 2a 08

cvtss2sd xmm1, xmm2 			; f3 0f 5a ca
cvtss2sd xmm1, [rax] 			; f3 0f 5a 08
cvtss2sd xmm1, dword [rax] 		; f3 0f 5a 08
vcvtss2sd xmm1, xmm2 			; c5 f2 5a ca
vcvtss2sd xmm1, [rax] 			; c5 f2 5a 08
vcvtss2sd xmm1, dword [rax] 		; c5 f2 5a 08
vcvtss2sd xmm1, xmm2, xmm3 		; c5 ea 5a cb
vcvtss2sd xmm1, xmm2, [rax] 		; c5 ea 5a 08
vcvtss2sd xmm1, xmm2, dword [rax] 	; c5 ea 5a 08

cvtss2si eax, xmm2 			; f3 0f 2d c2
cvtss2si eax, [rax] 			; f3 0f 2d 00
cvtss2si eax, dword [rax] 		; f3 0f 2d 00
vcvtss2si eax, xmm2 			; c5 fa 2d c2
vcvtss2si eax, [rax] 			; c5 fa 2d 00
vcvtss2si eax, dword [rax] 		; c5 fa 2d 00
cvtss2si rax, xmm2 			; f3 48 0f 2d c2
cvtss2si rax, [rax] 			; f3 48 0f 2d 00
cvtss2si rax, dword [rax] 		; f3 48 0f 2d 00
vcvtss2si rax, xmm2 			; c4 e1 fa 2d c2
vcvtss2si rax, [rax] 			; c4 e1 fa 2d 00
vcvtss2si rax, dword [rax] 		; c4 e1 fa 2d 00

; These require memory operand size to be specified (in AVX version)
cvttpd2dq xmm1, xmm2 			; 66 0f e6 ca
cvttpd2dq xmm1, [rax] 			; 66 0f e6 08
cvttpd2dq xmm1, dqword [rax] 		; 66 0f e6 08
vcvttpd2dq xmm1, xmm2 			; c5 f9 e6 ca
vcvttpd2dq xmm1, dqword [rax] 		; c5 f9 e6 08
vcvttpd2dq xmm1, ymm2 			; c5 fd e6 ca
vcvttpd2dq xmm1, yword [rax] 		; c5 fd e6 08

cvttps2dq xmm1, xmm2 			; f3 0f 5b ca
cvttps2dq xmm1, [rax] 			; f3 0f 5b 08
cvttps2dq xmm1, dqword [rax] 		; f3 0f 5b 08
vcvttps2dq xmm1, xmm2 			; c5 fa 5b ca
vcvttps2dq xmm1, [rax] 			; c5 fa 5b 08
vcvttps2dq xmm1, dqword [rax] 		; c5 fa 5b 08
vcvttps2dq ymm1, ymm2 			; c5 fe 5b ca
vcvttps2dq ymm1, [rax] 			; c5 fe 5b 08
vcvttps2dq ymm1, yword [rax] 		; c5 fe 5b 08

cvttsd2si eax, xmm2 			; f2 0f 2c c2
cvttsd2si eax, [rax] 			; f2 0f 2c 00
cvttsd2si eax, qword [rax] 		; f2 0f 2c 00
vcvttsd2si eax, xmm2 			; c5 fb 2c c2
vcvttsd2si eax, [rax] 			; c5 fb 2c 00
vcvttsd2si eax, qword [rax] 		; c5 fb 2c 00
cvttsd2si rax, xmm2 			; f2 48 0f 2c c2
cvttsd2si rax, [rax] 			; f2 48 0f 2c 00
cvttsd2si rax, qword [rax] 		; f2 48 0f 2c 00
vcvttsd2si rax, xmm2 			; c4 e1 fb 2c c2
vcvttsd2si rax, [rax] 			; c4 e1 fb 2c 00
vcvttsd2si rax, qword [rax] 		; c4 e1 fb 2c 00

cvttss2si eax, xmm2 			; f3 0f 2c c2
cvttss2si eax, [rax] 			; f3 0f 2c 00
cvttss2si eax, dword [rax] 		; f3 0f 2c 00
vcvttss2si eax, xmm2 			; c5 fa 2c c2
vcvttss2si eax, [rax] 			; c5 fa 2c 00
vcvttss2si eax, dword [rax] 		; c5 fa 2c 00
cvttss2si rax, xmm2 			; f3 48 0f 2c c2
cvttss2si rax, [rax] 			; f3 48 0f 2c 00
cvttss2si rax, dword [rax] 		; f3 48 0f 2c 00
vcvttss2si rax, xmm2 			; c4 e1 fa 2c c2
vcvttss2si rax, [rax] 			; c4 e1 fa 2c 00
vcvttss2si rax, dword [rax] 		; c4 e1 fa 2c 00

divpd xmm1, xmm2 			; 66 0f 5e ca
divpd xmm1, [rax] 			; 66 0f 5e 08
divpd xmm1, dqword [rax] 		; 66 0f 5e 08
vdivpd xmm1, xmm2 			; c5 f1 5e ca
vdivpd xmm1, [rax] 			; c5 f1 5e 08
vdivpd xmm1, dqword [rax] 		; c5 f1 5e 08
vdivpd xmm1, xmm2, xmm3 		; c5 e9 5e cb
vdivpd xmm1, xmm2, [rax] 		; c5 e9 5e 08
vdivpd xmm1, xmm2, dqword [rax]		; c5 e9 5e 08
vdivpd ymm1, ymm2, ymm3 		; c5 ed 5e cb
vdivpd ymm1, ymm2, [rax] 		; c5 ed 5e 08
vdivpd ymm1, ymm2, yword [rax] 		; c5 ed 5e 08

divps xmm1, xmm2 			; 0f 5e ca
divps xmm1, [rax] 			; 0f 5e 08
divps xmm1, dqword [rax] 		; 0f 5e 08
vdivps xmm1, xmm2 			; c5 f0 5e ca
vdivps xmm1, [rax] 			; c5 f0 5e 08
vdivps xmm1, dqword [rax] 		; c5 f0 5e 08
vdivps xmm1, xmm2, xmm3 		; c5 e8 5e cb
vdivps xmm1, xmm2, [rax] 		; c5 e8 5e 08
vdivps xmm1, xmm2, dqword [rax]		; c5 e8 5e 08
vdivps ymm1, ymm2, ymm3 		; c5 ec 5e cb
vdivps ymm1, ymm2, [rax] 		; c5 ec 5e 08
vdivps ymm1, ymm2, yword [rax] 		; c5 ec 5e 08

divsd xmm1, xmm2 			; f2 0f 5e ca
divsd xmm1, [rax] 			; f2 0f 5e 08
divsd xmm1, qword [rax] 		; f2 0f 5e 08
vdivsd xmm1, xmm2 			; c5 f3 5e ca
vdivsd xmm1, [rax] 			; c5 f3 5e 08
vdivsd xmm1, qword [rax] 		; c5 f3 5e 08
vdivsd xmm1, xmm2, xmm3 		; c5 eb 5e cb
vdivsd xmm1, xmm2, [rax] 		; c5 eb 5e 08
vdivsd xmm1, xmm2, qword [rax] 		; c5 eb 5e 08

divss xmm1, xmm2 			; f3 0f 5e ca
divss xmm1, [rax] 			; f3 0f 5e 08
divss xmm1, dword [rax] 		; f3 0f 5e 08
vdivss xmm1, xmm2 			; c5 f2 5e ca
vdivss xmm1, [rax] 			; c5 f2 5e 08
vdivss xmm1, dword [rax] 		; c5 f2 5e 08
vdivss xmm1, xmm2, xmm3 		; c5 ea 5e cb
vdivss xmm1, xmm2, [rax] 		; c5 ea 5e 08
vdivss xmm1, xmm2, dword [rax] 		; c5 ea 5e 08

dppd xmm1, xmm2, 5 			; 66 0f 3a 41 ca 05
dppd xmm1, [rax], byte 5 		; 66 0f 3a 41 08 05
dppd xmm1, dqword [rax], 5 		; 66 0f 3a 41 08 05
vdppd xmm1, xmm2, 5 			; c4 e3 71 41 ca 05
vdppd xmm1, [rax], byte 5 		; c4 e3 71 41 08 05
vdppd xmm1, dqword [rax], 5 		; c4 e3 71 41 08 05
vdppd xmm1, xmm2, xmm3, 5 		; c4 e3 69 41 cb 05
vdppd xmm1, xmm2, [rax], byte 5 	; c4 e3 69 41 08 05
vdppd xmm1, xmm2, dqword [rax], 5 	; c4 e3 69 41 08 05
; no ymm version

dpps xmm1, xmm2, 5 			; 66 0f 3a 40 ca 05
dpps xmm1, [rax], byte 5 		; 66 0f 3a 40 08 05
dpps xmm1, dqword [rax], 5 		; 66 0f 3a 40 08 05
vdpps xmm1, xmm2, 5 			; c4 e3 71 40 ca 05
vdpps xmm1, [rax], byte 5 		; c4 e3 71 40 08 05
vdpps xmm1, dqword [rax], 5 		; c4 e3 71 40 08 05
vdpps xmm1, xmm2, xmm3, 5 		; c4 e3 69 40 cb 05
vdpps xmm1, xmm2, [rax], byte 5 	; c4 e3 69 40 08 05
vdpps xmm1, xmm2, dqword [rax], 5 	; c4 e3 69 40 08 05
vdpps ymm1, ymm2, ymm3, 5 		; c4 e3 6d 40 cb 05
vdpps ymm1, ymm2, [rax], byte 5 	; c4 e3 6d 40 08 05
vdpps ymm1, ymm2, yword [rax], 5 	; c4 e3 6d 40 08 05

vextractf128 xmm1, ymm2, 5 		; c4 e3 7d 19 d1 05
vextractf128 [rax], ymm2, byte 5 	; c4 e3 7d 19 10 05
vextractf128 dqword [rax], ymm2, 5 	; c4 e3 7d 19 10 05

extractps eax, xmm1, 5 			; 66 0f 3a 17 c8 05
extractps rax, xmm1, 5 			; 66 48 0f 3a 17 c8 05
extractps [rax], xmm1, byte 5 		; 66 0f 3a 17 08 05
extractps dword [rax], xmm1, 5 		; 66 0f 3a 17 08 05
vextractps eax, xmm1, 5 		; c4 e3 79 17 c8 05
vextractps rax, xmm1, 5 		; c4 e3 f9 17 c8 05
vextractps [rax], xmm1, byte 5 		; c4 e3 79 17 08 05
vextractps dword [rax], xmm1, 5 	; c4 e3 79 17 08 05

haddpd xmm1, xmm2 			; 66 0f 7c ca
haddpd xmm1, [rax] 			; 66 0f 7c 08
haddpd xmm1, dqword [rax] 		; 66 0f 7c 08
vhaddpd xmm1, xmm2 			; c5 f1 7c ca
vhaddpd xmm1, [rax] 			; c5 f1 7c 08
vhaddpd xmm1, dqword [rax] 		; c5 f1 7c 08
vhaddpd xmm1, xmm2, xmm3 		; c5 e9 7c cb
vhaddpd xmm1, xmm2, [rax] 		; c5 e9 7c 08
vhaddpd xmm1, xmm2, dqword [rax] 	; c5 e9 7c 08
vhaddpd ymm1, ymm2, ymm3 		; c5 ed 7c cb
vhaddpd ymm1, ymm2, [rax] 		; c5 ed 7c 08
vhaddpd ymm1, ymm2, yword [rax] 	; c5 ed 7c 08

haddps xmm1, xmm2 			; f2 0f 7c ca
haddps xmm1, [rax] 			; f2 0f 7c 08
haddps xmm1, dqword [rax] 		; f2 0f 7c 08
vhaddps xmm1, xmm2 			; c5 f3 7c ca
vhaddps xmm1, [rax] 			; c5 f3 7c 08
vhaddps xmm1, dqword [rax] 		; c5 f3 7c 08
vhaddps xmm1, xmm2, xmm3 		; c5 eb 7c cb
vhaddps xmm1, xmm2, [rax] 		; c5 eb 7c 08
vhaddps xmm1, xmm2, dqword [rax] 	; c5 eb 7c 08
vhaddps ymm1, ymm2, ymm3 		; c5 ef 7c cb
vhaddps ymm1, ymm2, [rax] 		; c5 ef 7c 08
vhaddps ymm1, ymm2, yword [rax] 	; c5 ef 7c 08

hsubpd xmm1, xmm2 			; 66 0f 7d ca
hsubpd xmm1, [rax] 			; 66 0f 7d 08
hsubpd xmm1, dqword [rax] 		; 66 0f 7d 08
vhsubpd xmm1, xmm2 			; c5 f1 7d ca
vhsubpd xmm1, [rax] 			; c5 f1 7d 08
vhsubpd xmm1, dqword [rax] 		; c5 f1 7d 08
vhsubpd xmm1, xmm2, xmm3 		; c5 e9 7d cb
vhsubpd xmm1, xmm2, [rax] 		; c5 e9 7d 08
vhsubpd xmm1, xmm2, dqword [rax] 	; c5 e9 7d 08
vhsubpd ymm1, ymm2, ymm3 		; c5 ed 7d cb
vhsubpd ymm1, ymm2, [rax] 		; c5 ed 7d 08
vhsubpd ymm1, ymm2, yword [rax] 	; c5 ed 7d 08

hsubps xmm1, xmm2 			; f2 0f 7d ca
hsubps xmm1, [rax] 			; f2 0f 7d 08
hsubps xmm1, dqword [rax] 		; f2 0f 7d 08
vhsubps xmm1, xmm2 			; c5 f3 7d ca
vhsubps xmm1, [rax] 			; c5 f3 7d 08
vhsubps xmm1, dqword [rax] 		; c5 f3 7d 08
vhsubps xmm1, xmm2, xmm3 		; c5 eb 7d cb
vhsubps xmm1, xmm2, [rax] 		; c5 eb 7d 08
vhsubps xmm1, xmm2, dqword [rax] 	; c5 eb 7d 08
vhsubps ymm1, ymm2, ymm3 		; c5 ef 7d cb
vhsubps ymm1, ymm2, [rax] 		; c5 ef 7d 08
vhsubps ymm1, ymm2, yword [rax] 	; c5 ef 7d 08

vinsertf128 ymm1, ymm2, xmm3, 5 	; c4 e3 6d 18 cb 05
vinsertf128 ymm1, ymm2, [rax], byte 5 	; c4 e3 6d 18 08 05
vinsertf128 ymm1, ymm2, dqword [rax], 5 ; c4 e3 6d 18 08 05

insertps xmm1, xmm2, 5 			; 66 0f 3a 21 ca 05
insertps xmm1, [rax], byte 5 		; 66 0f 3a 21 08 05
insertps xmm1, dword [rax], 5 		; 66 0f 3a 21 08 05
vinsertps xmm1, xmm2, 5 		; c4 e3 71 21 ca 05
vinsertps xmm1, [rax], byte 5 		; c4 e3 71 21 08 05
vinsertps xmm1, dword [rax], 5 		; c4 e3 71 21 08 05
vinsertps xmm1, xmm2, xmm3, 5 		; c4 e3 69 21 cb 05
vinsertps xmm1, xmm2, [rax], byte 5 	; c4 e3 69 21 08 05
vinsertps xmm1, xmm2, dword [rax], 5 	; c4 e3 69 21 08 05

lddqu xmm1, [rax] 			; f2 0f f0 08
lddqu xmm1, dqword [rax] 		; f2 0f f0 08
vlddqu xmm1, [rax] 			; c5 fb f0 08
vlddqu xmm1, dqword [rax] 		; c5 fb f0 08
vlddqu ymm1, [rax] 			; c5 ff f0 08
vlddqu ymm1, yword [rax] 		; c5 ff f0 08

ldmxcsr [rax] 				; 0f ae 10
ldmxcsr dword [rax] 			; 0f ae 10
vldmxcsr [rax] 				; c5 f8 ae 10
vldmxcsr dword [rax] 			; c5 f8 ae 10

maskmovdqu xmm1, xmm2 			; 66 0f f7 ca
vmaskmovdqu xmm1, xmm2 			; c5 f9 f7 ca

vmaskmovps xmm1, xmm2, [rax] 		; c4 e2 69 2c 08
vmaskmovps xmm1, xmm2, dqword [rax] 	; c4 e2 69 2c 08
vmaskmovps ymm1, ymm2, [rax] 		; c4 e2 6d 2c 08
vmaskmovps ymm1, ymm2, yword [rax] 	; c4 e2 6d 2c 08
vmaskmovps [rax], xmm2, xmm3 		; c4 e2 69 2e 18
vmaskmovps dqword [rax], xmm2, xmm3 	; c4 e2 69 2e 18
vmaskmovps [rax], ymm2, ymm3 		; c4 e2 6d 2e 18
vmaskmovps yword [rax], ymm2, ymm3 	; c4 e2 6d 2e 18

vmaskmovpd xmm1, xmm2, [rax] 		; c4 e2 69 2d 08
vmaskmovpd xmm1, xmm2, dqword [rax] 	; c4 e2 69 2d 08
vmaskmovpd ymm1, ymm2, [rax] 		; c4 e2 6d 2d 08
vmaskmovpd ymm1, ymm2, yword [rax] 	; c4 e2 6d 2d 08
vmaskmovpd [rax], xmm2, xmm3 		; c4 e2 69 2f 18
vmaskmovpd dqword [rax], xmm2, xmm3 	; c4 e2 69 2f 18
vmaskmovpd [rax], ymm2, ymm3 		; c4 e2 6d 2f 18
vmaskmovpd yword [rax], ymm2, ymm3 	; c4 e2 6d 2f 18

maxpd xmm1, xmm2 			; 66 0f 5f ca
maxpd xmm1, [rax] 			; 66 0f 5f 08
maxpd xmm1, dqword [rax] 		; 66 0f 5f 08
vmaxpd xmm1, xmm2 			; c5 f1 5f ca
vmaxpd xmm1, [rax] 			; c5 f1 5f 08
vmaxpd xmm1, dqword [rax] 		; c5 f1 5f 08
vmaxpd xmm1, xmm2, xmm3 		; c5 e9 5f cb
vmaxpd xmm1, xmm2, [rax] 		; c5 e9 5f 08
vmaxpd xmm1, xmm2, dqword [rax] 	; c5 e9 5f 08
vmaxpd ymm1, ymm2, ymm3 		; c5 ed 5f cb
vmaxpd ymm1, ymm2, [rax] 		; c5 ed 5f 08
vmaxpd ymm1, ymm2, yword [rax] 		; c5 ed 5f 08

maxps xmm1, xmm2 			; 0f 5f ca
maxps xmm1, [rax] 			; 0f 5f 08
maxps xmm1, dqword [rax] 		; 0f 5f 08
vmaxps xmm1, xmm2 			; c5 f0 5f ca
vmaxps xmm1, [rax] 			; c5 f0 5f 08
vmaxps xmm1, dqword [rax] 		; c5 f0 5f 08
vmaxps xmm1, xmm2, xmm3 		; c5 e8 5f cb
vmaxps xmm1, xmm2, [rax] 		; c5 e8 5f 08
vmaxps xmm1, xmm2, dqword [rax] 	; c5 e8 5f 08
vmaxps ymm1, ymm2, ymm3 		; c5 ec 5f cb
vmaxps ymm1, ymm2, [rax] 		; c5 ec 5f 08
vmaxps ymm1, ymm2, yword [rax] 		; c5 ec 5f 08

maxsd xmm1, xmm2 			; f2 0f 5f ca
maxsd xmm1, [rax] 			; f2 0f 5f 08
maxsd xmm1, qword [rax] 		; f2 0f 5f 08
vmaxsd xmm1, xmm2 			; c5 f3 5f ca
vmaxsd xmm1, [rax] 			; c5 f3 5f 08
vmaxsd xmm1, qword [rax] 		; c5 f3 5f 08
vmaxsd xmm1, xmm2, xmm3 		; c5 eb 5f cb
vmaxsd xmm1, xmm2, [rax] 		; c5 eb 5f 08
vmaxsd xmm1, xmm2, qword [rax] 		; c5 eb 5f 08

maxss xmm1, xmm2 			; f3 0f 5f ca
maxss xmm1, [rax] 			; f3 0f 5f 08
maxss xmm1, dword [rax] 		; f3 0f 5f 08
vmaxss xmm1, xmm2 			; c5 f2 5f ca
vmaxss xmm1, [rax] 			; c5 f2 5f 08
vmaxss xmm1, dword [rax] 		; c5 f2 5f 08
vmaxss xmm1, xmm2, xmm3 		; c5 ea 5f cb
vmaxss xmm1, xmm2, [rax] 		; c5 ea 5f 08
vmaxss xmm1, xmm2, dword [rax] 		; c5 ea 5f 08

minpd xmm1, xmm2 			; 66 0f 5d ca
minpd xmm1, [rax] 			; 66 0f 5d 08
minpd xmm1, dqword [rax] 		; 66 0f 5d 08
vminpd xmm1, xmm2 			; c5 f1 5d ca
vminpd xmm1, [rax] 			; c5 f1 5d 08
vminpd xmm1, dqword [rax] 		; c5 f1 5d 08
vminpd xmm1, xmm2, xmm3 		; c5 e9 5d cb
vminpd xmm1, xmm2, [rax] 		; c5 e9 5d 08
vminpd xmm1, xmm2, dqword [rax] 	; c5 e9 5d 08
vminpd ymm1, ymm2, ymm3 		; c5 ed 5d cb
vminpd ymm1, ymm2, [rax] 		; c5 ed 5d 08
vminpd ymm1, ymm2, yword [rax] 		; c5 ed 5d 08

minps xmm1, xmm2 			; 0f 5d ca
minps xmm1, [rax] 			; 0f 5d 08
minps xmm1, dqword [rax] 		; 0f 5d 08
vminps xmm1, xmm2 			; c5 f0 5d ca
vminps xmm1, [rax] 			; c5 f0 5d 08
vminps xmm1, dqword [rax] 		; c5 f0 5d 08
vminps xmm1, xmm2, xmm3 		; c5 e8 5d cb
vminps xmm1, xmm2, [rax] 		; c5 e8 5d 08
vminps xmm1, xmm2, dqword [rax] 	; c5 e8 5d 08
vminps ymm1, ymm2, ymm3 		; c5 ec 5d cb
vminps ymm1, ymm2, [rax] 		; c5 ec 5d 08
vminps ymm1, ymm2, yword [rax] 		; c5 ec 5d 08

minsd xmm1, xmm2 			; f2 0f 5d ca
minsd xmm1, [rax] 			; f2 0f 5d 08
minsd xmm1, qword [rax] 		; f2 0f 5d 08
vminsd xmm1, xmm2 			; c5 f3 5d ca
vminsd xmm1, [rax] 			; c5 f3 5d 08
vminsd xmm1, qword [rax] 		; c5 f3 5d 08
vminsd xmm1, xmm2, xmm3 		; c5 eb 5d cb
vminsd xmm1, xmm2, [rax] 		; c5 eb 5d 08
vminsd xmm1, xmm2, qword [rax] 		; c5 eb 5d 08

minss xmm1, xmm2 			; f3 0f 5d ca
minss xmm1, [rax] 			; f3 0f 5d 08
minss xmm1, dword [rax] 		; f3 0f 5d 08
vminss xmm1, xmm2 			; c5 f2 5d ca
vminss xmm1, [rax] 			; c5 f2 5d 08
vminss xmm1, dword [rax] 		; c5 f2 5d 08
vminss xmm1, xmm2, xmm3 		; c5 ea 5d cb
vminss xmm1, xmm2, [rax] 		; c5 ea 5d 08
vminss xmm1, xmm2, dword [rax] 		; c5 ea 5d 08

movapd xmm1, xmm2 			; 66 0f 28 ca
movapd xmm1, [rax] 			; 66 0f 28 08
movapd xmm1, dqword [rax] 		; 66 0f 28 08
vmovapd xmm1, xmm2 			; c5 f9 28 ca
vmovapd xmm1, [rax] 			; c5 f9 28 08
vmovapd xmm1, dqword [rax] 		; c5 f9 28 08
movapd [rax], xmm2 			; 66 0f 29 10
movapd dqword [rax], xmm2 		; 66 0f 29 10
vmovapd [rax], xmm2 			; c5 f9 29 10
vmovapd dqword [rax], xmm2 		; c5 f9 29 10
vmovapd ymm1, ymm2 			; c5 fd 28 ca
vmovapd ymm1, [rax] 			; c5 fd 28 08
vmovapd ymm1, yword [rax] 		; c5 fd 28 08
vmovapd [rax], ymm2 			; c5 fd 29 10
vmovapd yword [rax], ymm2 		; c5 fd 29 10

movaps xmm1, xmm2 			; 0f 28 ca
movaps xmm1, [rax] 			; 0f 28 08
movaps xmm1, dqword [rax] 		; 0f 28 08
vmovaps xmm1, xmm2 			; c5 f8 28 ca
vmovaps xmm1, [rax] 			; c5 f8 28 08
vmovaps xmm1, dqword [rax] 		; c5 f8 28 08
movaps [rax], xmm2 			; 0f 29 10
movaps dqword [rax], xmm2 		; 0f 29 10
vmovaps [rax], xmm2 			; c5 f8 29 10
vmovaps dqword [rax], xmm2 		; c5 f8 29 10
vmovaps ymm1, ymm2 			; c5 fc 28 ca
vmovaps ymm1, [rax] 			; c5 fc 28 08
vmovaps ymm1, yword [rax] 		; c5 fc 28 08
vmovaps [rax], ymm2 			; c5 fc 29 10
vmovaps yword [rax], ymm2 		; c5 fc 29 10

movd xmm1, eax 				; 66 0f 6e c8
movd xmm1, [rax] 			; 66 0f 6e 08
movd xmm1, dword [rax] 			; 66 0f 6e 08
vmovd xmm1, eax 			; c5 f9 6e c8
vmovd xmm1, [rax] 			; c5 f9 6e 08
vmovd xmm1, dword [rax] 		; c5 f9 6e 08
movd eax, xmm2 				; 66 0f 7e d0
movd [rax], xmm2 			; 66 0f 7e 10
movd dword [rax], xmm2 			; 66 0f 7e 10
vmovd eax, xmm2 			; c5 f9 7e d0
vmovd [rax], xmm2 			; c5 f9 7e 10
vmovd dword [rax], xmm2 		; c5 f9 7e 10

movq xmm1, rax 				; 66 48 0f 6e c8
movq xmm1, [rax] 			; f3 0f 7e 08
movq xmm1, qword [rax] 			; f3 0f 7e 08
vmovq xmm1, rax 			; c4 e1 f9 6e c8
vmovq xmm1, [rax] 			; c5 fa 7e 08
vmovq xmm1, qword [rax] 		; c5 fa 7e 08
movq rax, xmm2 				; 66 48 0f 7e d0
movq [rax], xmm2 			; 66 0f d6 10
movq qword [rax], xmm2 			; 66 0f d6 10
vmovq rax, xmm2 			; c4 e1 f9 7e d0
vmovq [rax], xmm2 			; c5 f9 d6 10
vmovq qword [rax], xmm2 		; c5 f9 d6 10

movq xmm1, xmm2 			; f3 0f 7e ca
movq xmm1, [rax] 			; f3 0f 7e 08
movq xmm1, qword [rax] 			; f3 0f 7e 08
vmovq xmm1, xmm2 			; c5 fa 7e ca
vmovq xmm1, [rax] 			; c5 fa 7e 08
vmovq xmm1, qword [rax] 		; c5 fa 7e 08
movq [rax], xmm1 			; 66 0f d6 08
movq qword [rax], xmm1 			; 66 0f d6 08
vmovq [rax], xmm1 			; c5 f9 d6 08
vmovq qword [rax], xmm1 		; c5 f9 d6 08

movddup xmm1, xmm2 			; f2 0f 12 ca
movddup xmm1, [rax] 			; f2 0f 12 08
movddup xmm1, qword [rax] 		; f2 0f 12 08
vmovddup xmm1, xmm2 			; c5 fb 12 ca
vmovddup xmm1, [rax] 			; c5 fb 12 08
vmovddup xmm1, qword [rax] 		; c5 fb 12 08
vmovddup ymm1, ymm2 			; c5 ff 12 ca
vmovddup ymm1, [rax] 			; c5 ff 12 08
vmovddup ymm1, yword [rax] 		; c5 ff 12 08

movdqa xmm1, xmm2 			; 66 0f 6f ca
movdqa xmm1, [rax] 			; 66 0f 6f 08
movdqa xmm1, dqword [rax] 		; 66 0f 6f 08
movdqa [rax], xmm2 			; 66 0f 7f 10
movdqa dqword [rax], xmm2 		; 66 0f 7f 10
vmovdqa xmm1, xmm2 			; c5 f9 6f ca
vmovdqa xmm1, [rax] 			; c5 f9 6f 08
vmovdqa xmm1, dqword [rax] 		; c5 f9 6f 08
vmovdqa [rax], xmm2 			; c5 f9 7f 10
vmovdqa dqword [rax], xmm2 		; c5 f9 7f 10
vmovdqa ymm1, ymm2 			; c5 fd 6f ca
vmovdqa ymm1, [rax] 			; c5 fd 6f 08
vmovdqa ymm1, yword [rax] 		; c5 fd 6f 08
vmovdqa [rax], ymm2 			; c5 fd 7f 10
vmovdqa yword [rax], ymm2 		; c5 fd 7f 10

movdqu xmm1, xmm2 			; f3 0f 6f ca
movdqu xmm1, [rax] 			; f3 0f 6f 08
movdqu xmm1, dqword [rax] 		; f3 0f 6f 08
movdqu [rax], xmm2 			; f3 0f 7f 10
movdqu dqword [rax], xmm2 		; f3 0f 7f 10
vmovdqu xmm1, xmm2 			; c5 fa 6f ca
vmovdqu xmm1, [rax] 			; c5 fa 6f 08
vmovdqu xmm1, dqword [rax] 		; c5 fa 6f 08
vmovdqu [rax], xmm2 			; c5 fa 7f 10
vmovdqu dqword [rax], xmm2 		; c5 fa 7f 10
vmovdqu ymm1, ymm2 			; c5 fe 6f ca
vmovdqu ymm1, [rax] 			; c5 fe 6f 08
vmovdqu ymm1, yword [rax] 		; c5 fe 6f 08
vmovdqu [rax], ymm2 			; c5 fe 7f 10
vmovdqu yword [rax], ymm2 		; c5 fe 7f 10

movhlps xmm1, xmm2 			; 0f 12 ca
vmovhlps xmm1, xmm2 			; c5 f0 12 ca
vmovhlps xmm1, xmm2, xmm3 		; c5 e8 12 cb

movhpd xmm1, [rax] 			; 66 0f 16 08
movhpd xmm1, qword [rax] 		; 66 0f 16 08
vmovhpd xmm1, [rax] 			; c5 f1 16 08
vmovhpd xmm1, qword [rax] 		; c5 f1 16 08
vmovhpd xmm1, xmm2, [rax] 		; c5 e9 16 08
vmovhpd xmm1, xmm2, qword [rax] 	; c5 e9 16 08
movhpd [rax], xmm2 			; 66 0f 17 10
movhpd qword [rax], xmm2 		; 66 0f 17 10
vmovhpd [rax], xmm2 			; c5 f9 17 10
vmovhpd qword [rax], xmm2 		; c5 f9 17 10

movhps xmm1, [rax] 			; 0f 16 08
movhps xmm1, qword [rax] 		; 0f 16 08
vmovhps xmm1, [rax] 			; c5 f0 16 08
vmovhps xmm1, qword [rax] 		; c5 f0 16 08
vmovhps xmm1, xmm2, [rax] 		; c5 e8 16 08
vmovhps xmm1, xmm2, qword [rax] 	; c5 e8 16 08
movhps [rax], xmm2 			; 0f 17 10
movhps qword [rax], xmm2 		; 0f 17 10
vmovhps [rax], xmm2 			; c5 f8 17 10
vmovhps qword [rax], xmm2 		; c5 f8 17 10

movhlps xmm1, xmm2 			; 0f 12 ca
vmovhlps xmm1, xmm2 			; c5 f0 12 ca
vmovhlps xmm1, xmm2, xmm3 		; c5 e8 12 cb

movlpd xmm1, [rax] 			; 66 0f 12 08
movlpd xmm1, qword [rax] 		; 66 0f 12 08
vmovlpd xmm1, [rax] 			; c5 f1 12 08
vmovlpd xmm1, qword [rax] 		; c5 f1 12 08
vmovlpd xmm1, xmm2, [rax] 		; c5 e9 12 08
vmovlpd xmm1, xmm2, qword [rax] 	; c5 e9 12 08
movlpd [rax], xmm2 			; 66 0f 13 10
movlpd qword [rax], xmm2 		; 66 0f 13 10
vmovlpd [rax], xmm2 			; c5 f9 13 10
vmovlpd qword [rax], xmm2 		; c5 f9 13 10

movlps xmm1, [rax] 			; 0f 12 08
movlps xmm1, qword [rax] 		; 0f 12 08
vmovlps xmm1, [rax] 			; c5 f0 12 08
vmovlps xmm1, qword [rax] 		; c5 f0 12 08
vmovlps xmm1, xmm2, [rax] 		; c5 e8 12 08
vmovlps xmm1, xmm2, qword [rax] 	; c5 e8 12 08
movlps [rax], xmm2 			; 0f 13 10
movlps qword [rax], xmm2 		; 0f 13 10
vmovlps [rax], xmm2 			; c5 f8 13 10
vmovlps qword [rax], xmm2 		; c5 f8 13 10

movmskpd eax, xmm2 			; 66 0f 50 c2
movmskpd rax, xmm2 			; 66 48 0f 50 c2
vmovmskpd eax, xmm2 			; c5 f9 50 c2
vmovmskpd rax, xmm2 			; c4 e1 f9 50 c2
vmovmskpd eax, ymm2 			; c5 fd 50 c2
vmovmskpd rax, ymm2 			; c4 e1 fd 50 c2

movmskps eax, xmm2 			; 0f 50 c2
movmskps rax, xmm2 			; 48 0f 50 c2
vmovmskps eax, xmm2 			; c5 f8 50 c2
vmovmskps rax, xmm2 			; c4 e1 f8 50 c2
vmovmskps eax, ymm2 			; c5 fc 50 c2
vmovmskps rax, ymm2 			; c4 e1 fc 50 c2

movntdq [rax], xmm1 			; 66 0f e7 08
movntdq dqword [rax], xmm1 		; 66 0f e7 08
vmovntdq [rax], xmm1 			; c5 f9 e7 08
vmovntdq dqword [rax], xmm1 		; c5 f9 e7 08
vmovntdq [rax], ymm1 			; c5 fd e7 08
vmovntdq yword [rax], ymm1 		; c5 fd e7 08

movntdqa xmm1, [rax] 			; 66 0f 38 2a 08
movntdqa xmm1, dqword [rax] 		; 66 0f 38 2a 08
vmovntdqa xmm1, [rax] 			; c4 e2 79 2a 08
vmovntdqa xmm1, dqword [rax] 		; c4 e2 79 2a 08

movntpd [rax], xmm1 			; 66 0f 2b 08
movntpd dqword [rax], xmm1 		; 66 0f 2b 08
vmovntpd [rax], xmm1 			; c5 f9 2b 08
vmovntpd dqword [rax], xmm1 		; c5 f9 2b 08
vmovntpd [rax], ymm1 			; c5 fd 2b 08
vmovntpd yword [rax], ymm1 		; c5 fd 2b 08

movntps [rax], xmm1 			; 0f 2b 08
movntps dqword [rax], xmm1 		; 0f 2b 08
vmovntps [rax], xmm1 			; c5 f8 2b 08
vmovntps dqword [rax], xmm1 		; c5 f8 2b 08
vmovntps [rax], ymm1 			; c5 fc 2b 08
vmovntps yword [rax], ymm1 		; c5 fc 2b 08

movsd xmm1, xmm2 			; f2 0f 10 ca
vmovsd xmm1, xmm2 			; c5 f3 10 ca
vmovsd xmm1, xmm2, xmm3 		; c5 eb 10 cb
movsd xmm1, [rax] 			; f2 0f 10 08
movsd xmm1, qword [rax] 		; f2 0f 10 08
vmovsd xmm1, [rax] 			; c5 fb 10 08
vmovsd xmm1, qword [rax] 		; c5 fb 10 08
movsd [rax], xmm2 			; f2 0f 11 10
movsd qword [rax], xmm2 		; f2 0f 11 10
vmovsd [rax], xmm2 			; c5 fb 11 10
vmovsd qword [rax], xmm2 		; c5 fb 11 10

movshdup xmm1, xmm2 			; f3 0f 16 ca
movshdup xmm1, [rax] 			; f3 0f 16 08
movshdup xmm1, dqword [rax] 		; f3 0f 16 08
vmovshdup xmm1, xmm2 			; c5 fa 16 ca
vmovshdup xmm1, [rax] 			; c5 fa 16 08
vmovshdup xmm1, dqword [rax] 		; c5 fa 16 08
vmovshdup ymm1, ymm2 			; c5 fe 16 ca
vmovshdup ymm1, [rax] 			; c5 fe 16 08
vmovshdup ymm1, yword [rax] 		; c5 fe 16 08

movsldup xmm1, xmm2 			; f3 0f 12 ca
movsldup xmm1, [rax] 			; f3 0f 12 08
movsldup xmm1, dqword [rax] 		; f3 0f 12 08
vmovsldup xmm1, xmm2 			; c5 fa 12 ca
vmovsldup xmm1, [rax] 			; c5 fa 12 08
vmovsldup xmm1, dqword [rax] 		; c5 fa 12 08
vmovsldup ymm1, ymm2 			; c5 fe 12 ca
vmovsldup ymm1, [rax] 			; c5 fe 12 08
vmovsldup ymm1, yword [rax] 		; c5 fe 12 08

movss xmm1, xmm2 			; f3 0f 10 ca
vmovss xmm1, xmm2 			; c5 f2 10 ca
vmovss xmm1, xmm2, xmm3 		; c5 ea 10 cb
movss xmm1, [rax] 			; f3 0f 10 08
movss xmm1, dword [rax] 		; f3 0f 10 08
vmovss xmm1, [rax] 			; c5 fa 10 08
vmovss xmm1, dword [rax] 		; c5 fa 10 08
movss [rax], xmm2 			; f3 0f 11 10
movss dword [rax], xmm2 		; f3 0f 11 10
vmovss [rax], xmm2 			; c5 fa 11 10
vmovss dword [rax], xmm2 		; c5 fa 11 10

movupd xmm1, xmm2 			; 66 0f 10 ca
movupd xmm1, [rax] 			; 66 0f 10 08
movupd xmm1, dqword [rax] 		; 66 0f 10 08
vmovupd xmm1, xmm2 			; c5 f9 10 ca
vmovupd xmm1, [rax] 			; c5 f9 10 08
vmovupd xmm1, dqword [rax] 		; c5 f9 10 08
movupd [rax], xmm2 			; 66 0f 11 10
movupd dqword [rax], xmm2 		; 66 0f 11 10
vmovupd [rax], xmm2 			; c5 f9 11 10
vmovupd dqword [rax], xmm2 		; c5 f9 11 10
vmovupd ymm1, ymm2 			; c5 fd 10 ca
vmovupd ymm1, [rax] 			; c5 fd 10 08
vmovupd ymm1, yword [rax] 		; c5 fd 10 08
vmovupd [rax], ymm2 			; c5 fd 11 10
vmovupd yword [rax], ymm2 		; c5 fd 11 10

movups xmm1, xmm2 			; 0f 10 ca
movups xmm1, [rax] 			; 0f 10 08
movups xmm1, dqword [rax] 		; 0f 10 08
vmovups xmm1, xmm2 			; c5 f8 10 ca
vmovups xmm1, [rax] 			; c5 f8 10 08
vmovups xmm1, dqword [rax] 		; c5 f8 10 08
movups [rax], xmm2 			; 0f 11 10
movups dqword [rax], xmm2 		; 0f 11 10
vmovups [rax], xmm2 			; c5 f8 11 10
vmovups dqword [rax], xmm2 		; c5 f8 11 10
vmovups ymm1, ymm2 			; c5 fc 10 ca
vmovups ymm1, [rax] 			; c5 fc 10 08
vmovups ymm1, yword [rax] 		; c5 fc 10 08
vmovups [rax], ymm2 			; c5 fc 11 10
vmovups yword [rax], ymm2 		; c5 fc 11 10

mpsadbw xmm1, xmm2, 5 			; 66 0f 3a 42 ca 05
mpsadbw xmm1, [rax], byte 5 		; 66 0f 3a 42 08 05
mpsadbw xmm1, dqword [rax], 5 		; 66 0f 3a 42 08 05
vmpsadbw xmm1, xmm2, 5 			; c4 e3 71 42 ca 05
vmpsadbw xmm1, [rax], byte 5 		; c4 e3 71 42 08 05
vmpsadbw xmm1, dqword [rax], 5 		; c4 e3 71 42 08 05
vmpsadbw xmm1, xmm2, xmm3, 5 		; c4 e3 69 42 cb 05
vmpsadbw xmm1, xmm2, [rax], byte 5 	; c4 e3 69 42 08 05
vmpsadbw xmm1, xmm2, dqword [rax], 5 	; c4 e3 69 42 08 05

mulpd xmm1, xmm2 			; 66 0f 59 ca
mulpd xmm1, [rax] 			; 66 0f 59 08
mulpd xmm1, dqword [rax] 		; 66 0f 59 08
vmulpd xmm1, xmm2 			; c5 f1 59 ca
vmulpd xmm1, [rax] 			; c5 f1 59 08
vmulpd xmm1, dqword [rax] 		; c5 f1 59 08
vmulpd xmm1, xmm2, xmm3 		; c5 e9 59 cb
vmulpd xmm1, xmm2, [rax] 		; c5 e9 59 08
vmulpd xmm1, xmm2, dqword [rax] 	; c5 e9 59 08
vmulpd ymm1, ymm2, ymm3 		; c5 ed 59 cb
vmulpd ymm1, ymm2, [rax] 		; c5 ed 59 08
vmulpd ymm1, ymm2, yword [rax] 		; c5 ed 59 08

mulps xmm1, xmm2 			; 0f 59 ca
mulps xmm1, [rax] 			; 0f 59 08
mulps xmm1, dqword [rax] 		; 0f 59 08
vmulps xmm1, xmm2 			; c5 f0 59 ca
vmulps xmm1, [rax] 			; c5 f0 59 08
vmulps xmm1, dqword [rax] 		; c5 f0 59 08
vmulps xmm1, xmm2, xmm3 		; c5 e8 59 cb
vmulps xmm1, xmm2, [rax] 		; c5 e8 59 08
vmulps xmm1, xmm2, dqword [rax] 	; c5 e8 59 08
vmulps ymm1, ymm2, ymm3 		; c5 ec 59 cb
vmulps ymm1, ymm2, [rax] 		; c5 ec 59 08
vmulps ymm1, ymm2, yword [rax] 		; c5 ec 59 08

mulsd xmm1, xmm2 			; f2 0f 59 ca
mulsd xmm1, [rax] 			; f2 0f 59 08
mulsd xmm1, qword [rax] 		; f2 0f 59 08
vmulsd xmm1, xmm2 			; c5 f3 59 ca
vmulsd xmm1, [rax] 			; c5 f3 59 08
vmulsd xmm1, qword [rax] 		; c5 f3 59 08
vmulsd xmm1, xmm2, xmm3 		; c5 eb 59 cb
vmulsd xmm1, xmm2, [rax] 		; c5 eb 59 08
vmulsd xmm1, xmm2, qword [rax] 		; c5 eb 59 08

mulss xmm1, xmm2 			; f3 0f 59 ca
mulss xmm1, [rax] 			; f3 0f 59 08
mulss xmm1, dword [rax] 		; f3 0f 59 08
vmulss xmm1, xmm2 			; c5 f2 59 ca
vmulss xmm1, [rax] 			; c5 f2 59 08
vmulss xmm1, dword [rax] 		; c5 f2 59 08
vmulss xmm1, xmm2, xmm3 		; c5 ea 59 cb
vmulss xmm1, xmm2, [rax] 		; c5 ea 59 08
vmulss xmm1, xmm2, dword [rax] 		; c5 ea 59 08

orpd xmm1, xmm2 			; 66 0f 56 ca
orpd xmm1, [rax] 			; 66 0f 56 08
orpd xmm1, dqword [rax] 		; 66 0f 56 08
vorpd xmm1, xmm2 			; c5 f1 56 ca
vorpd xmm1, [rax] 			; c5 f1 56 08
vorpd xmm1, dqword [rax] 		; c5 f1 56 08
vorpd xmm1, xmm2, xmm3 			; c5 e9 56 cb
vorpd xmm1, xmm2, [rax] 		; c5 e9 56 08
vorpd xmm1, xmm2, dqword [rax] 		; c5 e9 56 08
vorpd ymm1, ymm2, ymm3 			; c5 ed 56 cb
vorpd ymm1, ymm2, [rax] 		; c5 ed 56 08
vorpd ymm1, ymm2, yword [rax] 		; c5 ed 56 08

orps xmm1, xmm2 			; 0f 56 ca
orps xmm1, [rax] 			; 0f 56 08
orps xmm1, dqword [rax] 		; 0f 56 08
vorps xmm1, xmm2 			; c5 f0 56 ca
vorps xmm1, [rax] 			; c5 f0 56 08
vorps xmm1, dqword [rax] 		; c5 f0 56 08
vorps xmm1, xmm2, xmm3 			; c5 e8 56 cb
vorps xmm1, xmm2, [rax] 		; c5 e8 56 08
vorps xmm1, xmm2, dqword [rax] 		; c5 e8 56 08
vorps ymm1, ymm2, ymm3 			; c5 ec 56 cb
vorps ymm1, ymm2, [rax] 		; c5 ec 56 08
vorps ymm1, ymm2, yword [rax] 		; c5 ec 56 08

pabsb xmm1, xmm2 			; 66 0f 38 1c ca
pabsb xmm1, [rax] 			; 66 0f 38 1c 08
pabsb xmm1, dqword [rax] 		; 66 0f 38 1c 08
vpabsb xmm1, xmm2 			; c4 e2 79 1c ca
vpabsb xmm1, [rax] 			; c4 e2 79 1c 08
vpabsb xmm1, dqword [rax] 		; c4 e2 79 1c 08

pabsw xmm1, xmm2 			; 66 0f 38 1d ca
pabsw xmm1, [rax] 			; 66 0f 38 1d 08
pabsw xmm1, dqword [rax] 		; 66 0f 38 1d 08
vpabsw xmm1, xmm2 			; c4 e2 79 1d ca
vpabsw xmm1, [rax] 			; c4 e2 79 1d 08
vpabsw xmm1, dqword [rax] 		; c4 e2 79 1d 08

pabsd xmm1, xmm2 			; 66 0f 38 1e ca
pabsd xmm1, [rax] 			; 66 0f 38 1e 08
pabsd xmm1, dqword [rax] 		; 66 0f 38 1e 08
vpabsd xmm1, xmm2 			; c4 e2 79 1e ca
vpabsd xmm1, [rax] 			; c4 e2 79 1e 08
vpabsd xmm1, dqword [rax] 		; c4 e2 79 1e 08

packsswb xmm1, xmm2 			; 66 0f 63 ca
packsswb xmm1, [rax] 			; 66 0f 63 08
packsswb xmm1, dqword [rax] 		; 66 0f 63 08
vpacksswb xmm1, xmm2 			; c5 f1 63 ca
vpacksswb xmm1, [rax] 			; c5 f1 63 08
vpacksswb xmm1, dqword [rax] 		; c5 f1 63 08
vpacksswb xmm1, xmm2, xmm3 		; c5 e9 63 cb
vpacksswb xmm1, xmm2, [rax] 		; c5 e9 63 08
vpacksswb xmm1, xmm2, dqword [rax] 	; c5 e9 63 08

packssdw xmm1, xmm2 			; 66 0f 6b ca
packssdw xmm1, [rax] 			; 66 0f 6b 08
packssdw xmm1, dqword [rax] 		; 66 0f 6b 08
vpackssdw xmm1, xmm2 			; c5 f1 6b ca
vpackssdw xmm1, [rax] 			; c5 f1 6b 08
vpackssdw xmm1, dqword [rax] 		; c5 f1 6b 08
vpackssdw xmm1, xmm2, xmm3 		; c5 e9 6b cb
vpackssdw xmm1, xmm2, [rax] 		; c5 e9 6b 08
vpackssdw xmm1, xmm2, dqword [rax] 	; c5 e9 6b 08

packuswb xmm1, xmm2 			; 66 0f 67 ca
packuswb xmm1, [rax] 			; 66 0f 67 08
packuswb xmm1, dqword [rax] 		; 66 0f 67 08
vpackuswb xmm1, xmm2 			; c5 f1 67 ca
vpackuswb xmm1, [rax] 			; c5 f1 67 08
vpackuswb xmm1, dqword [rax] 		; c5 f1 67 08
vpackuswb xmm1, xmm2, xmm3 		; c5 e9 67 cb
vpackuswb xmm1, xmm2, [rax] 		; c5 e9 67 08
vpackuswb xmm1, xmm2, dqword [rax] 	; c5 e9 67 08

packusdw xmm1, xmm2 			; 66 0f 38 2b ca
packusdw xmm1, [rax] 			; 66 0f 38 2b 08
packusdw xmm1, dqword [rax] 		; 66 0f 38 2b 08
vpackusdw xmm1, xmm2 			; c4 e2 71 2b ca
vpackusdw xmm1, [rax] 			; c4 e2 71 2b 08
vpackusdw xmm1, dqword [rax] 		; c4 e2 71 2b 08
vpackusdw xmm1, xmm2, xmm3 		; c4 e2 69 2b cb
vpackusdw xmm1, xmm2, [rax] 		; c4 e2 69 2b 08
vpackusdw xmm1, xmm2, dqword [rax] 	; c4 e2 69 2b 08

paddb xmm1, xmm2 			; 66 0f fc ca
paddb xmm1, [rax] 			; 66 0f fc 08
paddb xmm1, dqword [rax] 		; 66 0f fc 08
vpaddb xmm1, xmm2 			; c5 f1 fc ca
vpaddb xmm1, [rax] 			; c5 f1 fc 08
vpaddb xmm1, dqword [rax] 		; c5 f1 fc 08
vpaddb xmm1, xmm2, xmm3 		; c5 e9 fc cb
vpaddb xmm1, xmm2, [rax] 		; c5 e9 fc 08
vpaddb xmm1, xmm2, dqword [rax] 	; c5 e9 fc 08

paddw xmm1, xmm2 			; 66 0f fd ca
paddw xmm1, [rax] 			; 66 0f fd 08
paddw xmm1, dqword [rax] 		; 66 0f fd 08
vpaddw xmm1, xmm2 			; c5 f1 fd ca
vpaddw xmm1, [rax] 			; c5 f1 fd 08
vpaddw xmm1, dqword [rax] 		; c5 f1 fd 08
vpaddw xmm1, xmm2, xmm3 		; c5 e9 fd cb
vpaddw xmm1, xmm2, [rax] 		; c5 e9 fd 08
vpaddw xmm1, xmm2, dqword [rax] 	; c5 e9 fd 08

paddd xmm1, xmm2 			; 66 0f fe ca
paddd xmm1, [rax] 			; 66 0f fe 08
paddd xmm1, dqword [rax] 		; 66 0f fe 08
vpaddd xmm1, xmm2 			; c5 f1 fe ca
vpaddd xmm1, [rax] 			; c5 f1 fe 08
vpaddd xmm1, dqword [rax] 		; c5 f1 fe 08
vpaddd xmm1, xmm2, xmm3 		; c5 e9 fe cb
vpaddd xmm1, xmm2, [rax] 		; c5 e9 fe 08
vpaddd xmm1, xmm2, dqword [rax] 	; c5 e9 fe 08

paddq xmm1, xmm2 			; 66 0f d4 ca
paddq xmm1, [rax] 			; 66 0f d4 08
paddq xmm1, dqword [rax] 		; 66 0f d4 08
vpaddq xmm1, xmm2 			; c5 f1 d4 ca
vpaddq xmm1, [rax] 			; c5 f1 d4 08
vpaddq xmm1, dqword [rax] 		; c5 f1 d4 08
vpaddq xmm1, xmm2, xmm3 		; c5 e9 d4 cb
vpaddq xmm1, xmm2, [rax] 		; c5 e9 d4 08
vpaddq xmm1, xmm2, dqword [rax] 	; c5 e9 d4 08

paddsb xmm1, xmm2 			; 66 0f ec ca
paddsb xmm1, [rax] 			; 66 0f ec 08
paddsb xmm1, dqword [rax] 		; 66 0f ec 08
vpaddsb xmm1, xmm2 			; c5 f1 ec ca
vpaddsb xmm1, [rax] 			; c5 f1 ec 08
vpaddsb xmm1, dqword [rax] 		; c5 f1 ec 08
vpaddsb xmm1, xmm2, xmm3 		; c5 e9 ec cb
vpaddsb xmm1, xmm2, [rax] 		; c5 e9 ec 08
vpaddsb xmm1, xmm2, dqword [rax] 	; c5 e9 ec 08

paddsw xmm1, xmm2 			; 66 0f ed ca
paddsw xmm1, [rax] 			; 66 0f ed 08
paddsw xmm1, dqword [rax] 		; 66 0f ed 08
vpaddsw xmm1, xmm2 			; c5 f1 ed ca
vpaddsw xmm1, [rax] 			; c5 f1 ed 08
vpaddsw xmm1, dqword [rax] 		; c5 f1 ed 08
vpaddsw xmm1, xmm2, xmm3 		; c5 e9 ed cb
vpaddsw xmm1, xmm2, [rax] 		; c5 e9 ed 08
vpaddsw xmm1, xmm2, dqword [rax] 	; c5 e9 ed 08

paddusb xmm1, xmm2 			; 66 0f dc ca
paddusb xmm1, [rax] 			; 66 0f dc 08
paddusb xmm1, dqword [rax] 		; 66 0f dc 08
vpaddusb xmm1, xmm2 			; c5 f1 dc ca
vpaddusb xmm1, [rax] 			; c5 f1 dc 08
vpaddusb xmm1, dqword [rax] 		; c5 f1 dc 08
vpaddusb xmm1, xmm2, xmm3 		; c5 e9 dc cb
vpaddusb xmm1, xmm2, [rax] 		; c5 e9 dc 08
vpaddusb xmm1, xmm2, dqword [rax] 	; c5 e9 dc 08

paddusw xmm1, xmm2 			; 66 0f dd ca
paddusw xmm1, [rax] 			; 66 0f dd 08
paddusw xmm1, dqword [rax] 		; 66 0f dd 08
vpaddusw xmm1, xmm2 			; c5 f1 dd ca
vpaddusw xmm1, [rax] 			; c5 f1 dd 08
vpaddusw xmm1, dqword [rax] 		; c5 f1 dd 08
vpaddusw xmm1, xmm2, xmm3 		; c5 e9 dd cb
vpaddusw xmm1, xmm2, [rax] 		; c5 e9 dd 08
vpaddusw xmm1, xmm2, dqword [rax] 	; c5 e9 dd 08

palignr xmm1, xmm2, 5 			; 66 0f 3a 0f ca 05
palignr xmm1, [rax], byte 5 		; 66 0f 3a 0f 08 05
palignr xmm1, dqword [rax], 5 		; 66 0f 3a 0f 08 05
vpalignr xmm1, xmm2, 5 			; c4 e3 71 0f ca 05
vpalignr xmm1, [rax], byte 5 		; c4 e3 71 0f 08 05
vpalignr xmm1, dqword [rax], 5 		; c4 e3 71 0f 08 05
vpalignr xmm1, xmm2, xmm3, 5 		; c4 e3 69 0f cb 05
vpalignr xmm1, xmm2, [rax], byte 5 	; c4 e3 69 0f 08 05
vpalignr xmm1, xmm2, dqword [rax], 5 	; c4 e3 69 0f 08 05

pand xmm1, xmm2 			; 66 0f db ca
pand xmm1, [rax] 			; 66 0f db 08
pand xmm1, dqword [rax] 		; 66 0f db 08
vpand xmm1, xmm2 			; c5 f1 db ca
vpand xmm1, [rax] 			; c5 f1 db 08
vpand xmm1, dqword [rax] 		; c5 f1 db 08
vpand xmm1, xmm2, xmm3 			; c5 e9 db cb
vpand xmm1, xmm2, [rax] 		; c5 e9 db 08
vpand xmm1, xmm2, dqword [rax] 		; c5 e9 db 08

pandn xmm1, xmm2 			; 66 0f df ca
pandn xmm1, [rax] 			; 66 0f df 08
pandn xmm1, dqword [rax] 		; 66 0f df 08
vpandn xmm1, xmm2 			; c5 f1 df ca
vpandn xmm1, [rax] 			; c5 f1 df 08
vpandn xmm1, dqword [rax] 		; c5 f1 df 08
vpandn xmm1, xmm2, xmm3 		; c5 e9 df cb
vpandn xmm1, xmm2, [rax] 		; c5 e9 df 08
vpandn xmm1, xmm2, dqword [rax] 	; c5 e9 df 08

pavgb xmm1, xmm2 			; 66 0f e0 ca
pavgb xmm1, [rax] 			; 66 0f e0 08
pavgb xmm1, dqword [rax] 		; 66 0f e0 08
vpavgb xmm1, xmm2 			; c5 f1 e0 ca
vpavgb xmm1, [rax] 			; c5 f1 e0 08
vpavgb xmm1, dqword [rax] 		; c5 f1 e0 08
vpavgb xmm1, xmm2, xmm3 		; c5 e9 e0 cb
vpavgb xmm1, xmm2, [rax] 		; c5 e9 e0 08
vpavgb xmm1, xmm2, dqword [rax] 	; c5 e9 e0 08

pavgw xmm1, xmm2 			; 66 0f e3 ca
pavgw xmm1, [rax] 			; 66 0f e3 08
pavgw xmm1, dqword [rax] 		; 66 0f e3 08
vpavgw xmm1, xmm2 			; c5 f1 e3 ca
vpavgw xmm1, [rax] 			; c5 f1 e3 08
vpavgw xmm1, dqword [rax] 		; c5 f1 e3 08
vpavgw xmm1, xmm2, xmm3 		; c5 e9 e3 cb
vpavgw xmm1, xmm2, [rax] 		; c5 e9 e3 08
vpavgw xmm1, xmm2, dqword [rax] 	; c5 e9 e3 08

; implicit XMM0 cannot be VEX encoded
pblendvb xmm1, xmm2 				; 66 0f 38 10 ca
pblendvb xmm1, [rax] 				; 66 0f 38 10 08
pblendvb xmm1, dqword [rax] 			; 66 0f 38 10 08
pblendvb xmm1, xmm2, xmm0 			; 66 0f 38 10 ca
pblendvb xmm1, [rax], xmm0 			; 66 0f 38 10 08
pblendvb xmm1, dqword [rax], xmm0 		; 66 0f 38 10 08
vpblendvb xmm1, xmm2, xmm3, xmm4 		; c4 e3 69 4c cb 40
vpblendvb xmm1, xmm2, [rax], xmm4 		; c4 e3 69 4c 08 40
vpblendvb xmm1, xmm2, dqword [rax], xmm4 	; c4 e3 69 4c 08 40

pblendw xmm1, xmm2, 5 			; 66 0f 3a 0e ca 05
pblendw xmm1, [rax], byte 5 		; 66 0f 3a 0e 08 05
pblendw xmm1, dqword [rax], 5 		; 66 0f 3a 0e 08 05
vpblendw xmm1, xmm2, 5 			; c4 e3 71 0e ca 05
vpblendw xmm1, [rax], byte 5 		; c4 e3 71 0e 08 05
vpblendw xmm1, dqword [rax], 5 		; c4 e3 71 0e 08 05
vpblendw xmm1, xmm2, xmm3, 5 		; c4 e3 69 0e cb 05
vpblendw xmm1, xmm2, [rax], byte 5 	; c4 e3 69 0e 08 05
vpblendw xmm1, xmm2, dqword [rax], 5 	; c4 e3 69 0e 08 05

pcmpestri xmm1, xmm2, 5 		; 66 0f 3a 61 ca 05
pcmpestri xmm1, [rax], byte 5 		; 66 0f 3a 61 08 05
pcmpestri xmm1, dqword [rax], 5 	; 66 0f 3a 61 08 05
vpcmpestri xmm1, xmm2, 5 		; c4 e3 79 61 ca 05
vpcmpestri xmm1, [rax], byte 5 		; c4 e3 79 61 08 05
vpcmpestri xmm1, dqword [rax], 5 	; c4 e3 79 61 08 05

pcmpestrm xmm1, xmm2, 5 		; 66 0f 3a 60 ca 05
pcmpestrm xmm1, [rax], byte 5 		; 66 0f 3a 60 08 05
pcmpestrm xmm1, dqword [rax], 5 	; 66 0f 3a 60 08 05
vpcmpestrm xmm1, xmm2, 5 		; c4 e3 79 60 ca 05
vpcmpestrm xmm1, [rax], byte 5 		; c4 e3 79 60 08 05
vpcmpestrm xmm1, dqword [rax], 5 	; c4 e3 79 60 08 05

pcmpistri xmm1, xmm2, 5 		; 66 0f 3a 63 ca 05
pcmpistri xmm1, [rax], byte 5 		; 66 0f 3a 63 08 05
pcmpistri xmm1, dqword [rax], 5 	; 66 0f 3a 63 08 05
vpcmpistri xmm1, xmm2, 5 		; c4 e3 79 63 ca 05
vpcmpistri xmm1, [rax], byte 5 		; c4 e3 79 63 08 05
vpcmpistri xmm1, dqword [rax], 5 	; c4 e3 79 63 08 05

pcmpistrm xmm1, xmm2, 5 		; 66 0f 3a 62 ca 05
pcmpistrm xmm1, [rax], byte 5 		; 66 0f 3a 62 08 05
pcmpistrm xmm1, dqword [rax], 5 	; 66 0f 3a 62 08 05
vpcmpistrm xmm1, xmm2, 5 		; c4 e3 79 62 ca 05
vpcmpistrm xmm1, [rax], byte 5 		; c4 e3 79 62 08 05
vpcmpistrm xmm1, dqword [rax], 5 	; c4 e3 79 62 08 05

pcmpeqb xmm1, xmm2 			; 66 0f 74 ca
pcmpeqb xmm1, [rax] 			; 66 0f 74 08
pcmpeqb xmm1, dqword [rax] 		; 66 0f 74 08
vpcmpeqb xmm1, xmm2 			; c5 f1 74 ca
vpcmpeqb xmm1, [rax] 			; c5 f1 74 08
vpcmpeqb xmm1, dqword [rax] 		; c5 f1 74 08
vpcmpeqb xmm1, xmm2, xmm3 		; c5 e9 74 cb
vpcmpeqb xmm1, xmm2, [rax] 		; c5 e9 74 08
vpcmpeqb xmm1, xmm2, dqword [rax] 	; c5 e9 74 08

pcmpeqw xmm1, xmm2 			; 66 0f 75 ca
pcmpeqw xmm1, [rax] 			; 66 0f 75 08
pcmpeqw xmm1, dqword [rax] 		; 66 0f 75 08
vpcmpeqw xmm1, xmm2 			; c5 f1 75 ca
vpcmpeqw xmm1, [rax] 			; c5 f1 75 08
vpcmpeqw xmm1, dqword [rax] 		; c5 f1 75 08
vpcmpeqw xmm1, xmm2, xmm3 		; c5 e9 75 cb
vpcmpeqw xmm1, xmm2, [rax] 		; c5 e9 75 08
vpcmpeqw xmm1, xmm2, dqword [rax] 	; c5 e9 75 08

pcmpeqd xmm1, xmm2 			; 66 0f 76 ca
pcmpeqd xmm1, [rax] 			; 66 0f 76 08
pcmpeqd xmm1, dqword [rax] 		; 66 0f 76 08
vpcmpeqd xmm1, xmm2 			; c5 f1 76 ca
vpcmpeqd xmm1, [rax] 			; c5 f1 76 08
vpcmpeqd xmm1, dqword [rax] 		; c5 f1 76 08
vpcmpeqd xmm1, xmm2, xmm3 		; c5 e9 76 cb
vpcmpeqd xmm1, xmm2, [rax] 		; c5 e9 76 08
vpcmpeqd xmm1, xmm2, dqword [rax] 	; c5 e9 76 08

pcmpeqq xmm1, xmm2 			; 66 0f 38 29 ca
pcmpeqq xmm1, [rax] 			; 66 0f 38 29 08
pcmpeqq xmm1, dqword [rax] 		; 66 0f 38 29 08
vpcmpeqq xmm1, xmm2 			; c4 e2 71 29 ca
vpcmpeqq xmm1, [rax] 			; c4 e2 71 29 08
vpcmpeqq xmm1, dqword [rax] 		; c4 e2 71 29 08
vpcmpeqq xmm1, xmm2, xmm3 		; c4 e2 69 29 cb
vpcmpeqq xmm1, xmm2, [rax] 		; c4 e2 69 29 08
vpcmpeqq xmm1, xmm2, dqword [rax] 	; c4 e2 69 29 08

pcmpgtb xmm1, xmm2 			; 66 0f 64 ca
pcmpgtb xmm1, [rax] 			; 66 0f 64 08
pcmpgtb xmm1, dqword [rax] 		; 66 0f 64 08
vpcmpgtb xmm1, xmm2 			; c5 f1 64 ca
vpcmpgtb xmm1, [rax] 			; c5 f1 64 08
vpcmpgtb xmm1, dqword [rax] 		; c5 f1 64 08
vpcmpgtb xmm1, xmm2, xmm3 		; c5 e9 64 cb
vpcmpgtb xmm1, xmm2, [rax] 		; c5 e9 64 08
vpcmpgtb xmm1, xmm2, dqword [rax] 	; c5 e9 64 08

pcmpgtw xmm1, xmm2 			; 66 0f 65 ca
pcmpgtw xmm1, [rax] 			; 66 0f 65 08
pcmpgtw xmm1, dqword [rax] 		; 66 0f 65 08
vpcmpgtw xmm1, xmm2 			; c5 f1 65 ca
vpcmpgtw xmm1, [rax] 			; c5 f1 65 08
vpcmpgtw xmm1, dqword [rax] 		; c5 f1 65 08
vpcmpgtw xmm1, xmm2, xmm3 		; c5 e9 65 cb
vpcmpgtw xmm1, xmm2, [rax] 		; c5 e9 65 08
vpcmpgtw xmm1, xmm2, dqword [rax] 	; c5 e9 65 08

pcmpgtd xmm1, xmm2 			; 66 0f 66 ca
pcmpgtd xmm1, [rax] 			; 66 0f 66 08
pcmpgtd xmm1, dqword [rax] 		; 66 0f 66 08
vpcmpgtd xmm1, xmm2 			; c5 f1 66 ca
vpcmpgtd xmm1, [rax] 			; c5 f1 66 08
vpcmpgtd xmm1, dqword [rax] 		; c5 f1 66 08
vpcmpgtd xmm1, xmm2, xmm3 		; c5 e9 66 cb
vpcmpgtd xmm1, xmm2, [rax] 		; c5 e9 66 08
vpcmpgtd xmm1, xmm2, dqword [rax] 	; c5 e9 66 08

pcmpgtq xmm1, xmm2 			; 66 0f 38 37 ca
pcmpgtq xmm1, [rax] 			; 66 0f 38 37 08
pcmpgtq xmm1, dqword [rax] 		; 66 0f 38 37 08
vpcmpgtq xmm1, xmm2 			; c4 e2 71 37 ca
vpcmpgtq xmm1, [rax] 			; c4 e2 71 37 08
vpcmpgtq xmm1, dqword [rax] 		; c4 e2 71 37 08
vpcmpgtq xmm1, xmm2, xmm3 		; c4 e2 69 37 cb
vpcmpgtq xmm1, xmm2, [rax] 		; c4 e2 69 37 08
vpcmpgtq xmm1, xmm2, dqword [rax] 	; c4 e2 69 37 08

vpermilpd xmm1, xmm2, xmm3 		; c4 e2 69 0d cb
vpermilpd xmm1, xmm2, [rax] 		; c4 e2 69 0d 08
vpermilpd xmm1, xmm2, dqword [rax] 	; c4 e2 69 0d 08
vpermilpd ymm1, ymm2, ymm3 		; c4 e2 6d 0d cb
vpermilpd ymm1, ymm2, [rax] 		; c4 e2 6d 0d 08
vpermilpd ymm1, ymm2, yword [rax] 	; c4 e2 6d 0d 08
vpermilpd xmm1, [rax], byte 5 		; c4 e3 79 05 08 05
vpermilpd xmm1, dqword [rax], 5 	; c4 e3 79 05 08 05
vpermilpd ymm1, [rax], byte 5 		; c4 e3 7d 05 08 05
vpermilpd ymm1, yword [rax], 5 		; c4 e3 7d 05 08 05

vpermilps xmm1, xmm2, xmm3 		; c4 e2 69 0c cb
vpermilps xmm1, xmm2, [rax] 		; c4 e2 69 0c 08
vpermilps xmm1, xmm2, dqword [rax] 	; c4 e2 69 0c 08
vpermilps ymm1, ymm2, ymm3 		; c4 e2 6d 0c cb
vpermilps ymm1, ymm2, [rax] 		; c4 e2 6d 0c 08
vpermilps ymm1, ymm2, yword [rax] 	; c4 e2 6d 0c 08
vpermilps xmm1, [rax], byte 5 		; c4 e3 79 04 08 05
vpermilps xmm1, dqword [rax], 5 	; c4 e3 79 04 08 05
vpermilps ymm1, [rax], byte 5 		; c4 e3 7d 04 08 05
vpermilps ymm1, yword [rax], 5 		; c4 e3 7d 04 08 05

vperm2f128 ymm1, ymm2, ymm3, 5 		; c4 e3 6d 06 cb 05
vperm2f128 ymm1, ymm2, [rax], byte 5 	; c4 e3 6d 06 08 05
vperm2f128 ymm1, ymm2, yword [rax], 5 	; c4 e3 6d 06 08 05

pextrb eax, xmm2, 5 			; 66 0f 3a 14 d0 05
pextrb eax, xmm2, byte 5 		; 66 0f 3a 14 d0 05
pextrb rax, xmm2, 5 			; 66 48 0f 3a 14 d0 05
pextrb rax, xmm2, byte 5 		; 66 48 0f 3a 14 d0 05
pextrb byte [rax], xmm2, 5 		; 66 0f 3a 14 10 05
pextrb [rax], xmm2, byte 5 		; 66 0f 3a 14 10 05
vpextrb eax, xmm2, 5 			; c4 e3 79 14 d0 05
vpextrb eax, xmm2, byte 5 		; c4 e3 79 14 d0 05
vpextrb rax, xmm2, 5 			; c4 e3 f9 14 d0 05
vpextrb rax, xmm2, byte 5 		; c4 e3 f9 14 d0 05
vpextrb byte [rax], xmm2, 5 		; c4 e3 79 14 10 05
vpextrb [rax], xmm2, byte 5 		; c4 e3 79 14 10 05

pextrw eax, xmm2, 5 			; 66 0f c5 c2 05
pextrw eax, xmm2, byte 5 		; 66 0f c5 c2 05
pextrw rax, xmm2, 5 			; 66 48 0f c5 c2 05
pextrw rax, xmm2, byte 5 		; 66 48 0f c5 c2 05
pextrw word [rax], xmm2, 5 		; 66 0f 3a 15 10 05
pextrw [rax], xmm2, byte 5 		; 66 0f 3a 15 10 05
vpextrw eax, xmm2, 5 			; c5 f9 c5 c2 05
vpextrw eax, xmm2, byte 5 		; c5 f9 c5 c2 05
vpextrw rax, xmm2, 5 			; c4 e1 f9 c5 c2 05
vpextrw rax, xmm2, byte 5 		; c4 e1 f9 c5 c2 05
vpextrw word [rax], xmm2, 5 		; c4 e3 79 15 10 05
vpextrw [rax], xmm2, byte 5 		; c4 e3 79 15 10 05

pextrd eax, xmm2, 5 			; 66 0f 3a 16 d0 05
pextrd eax, xmm2, byte 5 		; 66 0f 3a 16 d0 05
pextrd dword [rax], xmm2, 5 		; 66 0f 3a 16 10 05
pextrd [rax], xmm2, byte 5 		; 66 0f 3a 16 10 05
vpextrd eax, xmm2, 5 			; c4 e3 79 16 d0 05
vpextrd eax, xmm2, byte 5 		; c4 e3 79 16 d0 05
vpextrd dword [rax], xmm2, 5 		; c4 e3 79 16 10 05
vpextrd [rax], xmm2, byte 5 		; c4 e3 79 16 10 05

pextrq rax, xmm2, 5 			; 66 48 0f 3a 16 d0 05
pextrq rax, xmm2, byte 5 		; 66 48 0f 3a 16 d0 05
pextrq qword [rax], xmm2, 5 		; 66 48 0f 3a 16 10 05
pextrq [rax], xmm2, byte 5 		; 66 48 0f 3a 16 10 05
vpextrq rax, xmm2, 5 			; c4 e3 f9 16 d0 05
vpextrq rax, xmm2, byte 5 		; c4 e3 f9 16 d0 05
vpextrq qword [rax], xmm2, 5 		; c4 e3 f9 16 10 05
vpextrq [rax], xmm2, byte 5 		; c4 e3 f9 16 10 05

phaddw xmm1, xmm2 			; 66 0f 38 01 ca
phaddw xmm1, [rax] 			; 66 0f 38 01 08
phaddw xmm1, dqword [rax] 		; 66 0f 38 01 08
vphaddw xmm1, xmm2 			; c4 e2 71 01 ca
vphaddw xmm1, [rax] 			; c4 e2 71 01 08
vphaddw xmm1, dqword [rax] 		; c4 e2 71 01 08
vphaddw xmm1, xmm2, xmm3 		; c4 e2 69 01 cb
vphaddw xmm1, xmm2, [rax] 		; c4 e2 69 01 08
vphaddw xmm1, xmm2, dqword [rax] 	; c4 e2 69 01 08

phaddd xmm1, xmm2 			; 66 0f 38 02 ca
phaddd xmm1, [rax] 			; 66 0f 38 02 08
phaddd xmm1, dqword [rax] 		; 66 0f 38 02 08
vphaddd xmm1, xmm2 			; c4 e2 71 02 ca
vphaddd xmm1, [rax] 			; c4 e2 71 02 08
vphaddd xmm1, dqword [rax] 		; c4 e2 71 02 08
vphaddd xmm1, xmm2, xmm3 		; c4 e2 69 02 cb
vphaddd xmm1, xmm2, [rax] 		; c4 e2 69 02 08
vphaddd xmm1, xmm2, dqword [rax] 	; c4 e2 69 02 08

phaddsw xmm1, xmm2 			; 66 0f 38 03 ca
phaddsw xmm1, [rax] 			; 66 0f 38 03 08
phaddsw xmm1, dqword [rax] 		; 66 0f 38 03 08
vphaddsw xmm1, xmm2 			; c4 e2 71 03 ca
vphaddsw xmm1, [rax] 			; c4 e2 71 03 08
vphaddsw xmm1, dqword [rax] 		; c4 e2 71 03 08
vphaddsw xmm1, xmm2, xmm3 		; c4 e2 69 03 cb
vphaddsw xmm1, xmm2, [rax] 		; c4 e2 69 03 08
vphaddsw xmm1, xmm2, dqword [rax] 	; c4 e2 69 03 08

phminposuw xmm1, xmm2 			; 66 0f 38 41 ca
phminposuw xmm1, [rax] 			; 66 0f 38 41 08
phminposuw xmm1, dqword [rax] 		; 66 0f 38 41 08
vphminposuw xmm1, xmm2 			; c4 e2 79 41 ca
vphminposuw xmm1, [rax] 		; c4 e2 79 41 08
vphminposuw xmm1, dqword [rax] 		; c4 e2 79 41 08

phsubw xmm1, xmm2 			; 66 0f 38 05 ca
phsubw xmm1, [rax] 			; 66 0f 38 05 08
phsubw xmm1, dqword [rax] 		; 66 0f 38 05 08
vphsubw xmm1, xmm2 			; c4 e2 71 05 ca
vphsubw xmm1, [rax] 			; c4 e2 71 05 08
vphsubw xmm1, dqword [rax] 		; c4 e2 71 05 08
vphsubw xmm1, xmm2, xmm3 		; c4 e2 69 05 cb
vphsubw xmm1, xmm2, [rax] 		; c4 e2 69 05 08
vphsubw xmm1, xmm2, dqword [rax] 	; c4 e2 69 05 08

phsubd xmm1, xmm2 			; 66 0f 38 06 ca
phsubd xmm1, [rax] 			; 66 0f 38 06 08
phsubd xmm1, dqword [rax] 		; 66 0f 38 06 08
vphsubd xmm1, xmm2 			; c4 e2 71 06 ca
vphsubd xmm1, [rax] 			; c4 e2 71 06 08
vphsubd xmm1, dqword [rax] 		; c4 e2 71 06 08
vphsubd xmm1, xmm2, xmm3 		; c4 e2 69 06 cb
vphsubd xmm1, xmm2, [rax] 		; c4 e2 69 06 08
vphsubd xmm1, xmm2, dqword [rax] 	; c4 e2 69 06 08

phsubsw xmm1, xmm2 			; 66 0f 38 07 ca
phsubsw xmm1, [rax] 			; 66 0f 38 07 08
phsubsw xmm1, dqword [rax] 		; 66 0f 38 07 08
vphsubsw xmm1, xmm2 			; c4 e2 71 07 ca
vphsubsw xmm1, [rax] 			; c4 e2 71 07 08
vphsubsw xmm1, dqword [rax] 		; c4 e2 71 07 08
vphsubsw xmm1, xmm2, xmm3 		; c4 e2 69 07 cb
vphsubsw xmm1, xmm2, [rax] 		; c4 e2 69 07 08
vphsubsw xmm1, xmm2, dqword [rax] 	; c4 e2 69 07 08

pinsrb xmm1, eax, 5 			; 66 0f 3a 20 c8 05
pinsrb xmm1, byte [rax], 5 		; 66 0f 3a 20 08 05
pinsrb xmm1, [rax], byte 5 		; 66 0f 3a 20 08 05
vpinsrb xmm1, eax, 5 			; c4 e3 71 20 c8 05
vpinsrb xmm1, byte [rax], 5 		; c4 e3 71 20 08 05
vpinsrb xmm1, [rax], byte 5 		; c4 e3 71 20 08 05
vpinsrb xmm1, xmm2, eax, 5 		; c4 e3 69 20 c8 05
vpinsrb xmm1, xmm2, byte [rax], 5 	; c4 e3 69 20 08 05
vpinsrb xmm1, xmm2, [rax], byte 5 	; c4 e3 69 20 08 05

pinsrw xmm1, eax, 5 			; 66 0f c4 c8 05
pinsrw xmm1, word [rax], 5 		; 66 0f c4 08 05
pinsrw xmm1, [rax], byte 5 		; 66 0f c4 08 05
vpinsrw xmm1, eax, 5 			; c5 f1 c4 c8 05
vpinsrw xmm1, word [rax], 5 		; c5 f1 c4 08 05
vpinsrw xmm1, [rax], byte 5 		; c5 f1 c4 08 05
vpinsrw xmm1, xmm2, eax, 5 		; c5 e9 c4 c8 05
vpinsrw xmm1, xmm2, word [rax], 5 	; c5 e9 c4 08 05
vpinsrw xmm1, xmm2, [rax], byte 5 	; c5 e9 c4 08 05

pinsrd xmm1, eax, 5 			; 66 0f 3a 22 c8 05
pinsrd xmm1, dword [rax], 5 		; 66 0f 3a 22 08 05
pinsrd xmm1, [rax], byte 5 		; 66 0f 3a 22 08 05
vpinsrd xmm1, eax, 5 			; c4 e3 71 22 c8 05
vpinsrd xmm1, dword [rax], 5 		; c4 e3 71 22 08 05
vpinsrd xmm1, [rax], byte 5 		; c4 e3 71 22 08 05
vpinsrd xmm1, xmm2, eax, 5 		; c4 e3 69 22 c8 05
vpinsrd xmm1, xmm2, dword [rax], 5 	; c4 e3 69 22 08 05
vpinsrd xmm1, xmm2, [rax], byte 5 	; c4 e3 69 22 08 05

pinsrq xmm1, rax, 5 			; 66 48 0f 3a 22 c8 05
pinsrq xmm1, qword [rax], 5 		; 66 48 0f 3a 22 08 05
pinsrq xmm1, [rax], byte 5 		; 66 48 0f 3a 22 08 05
vpinsrq xmm1, rax, 5 			; c4 e3 f1 22 c8 05
vpinsrq xmm1, qword [rax], 5 		; c4 e3 f1 22 08 05
vpinsrq xmm1, [rax], byte 5 		; c4 e3 f1 22 08 05
vpinsrq xmm1, xmm2, rax, 5 		; c4 e3 e9 22 c8 05
vpinsrq xmm1, xmm2, qword [rax], 5 	; c4 e3 e9 22 08 05
vpinsrq xmm1, xmm2, [rax], byte 5 	; c4 e3 e9 22 08 05

pmaddwd xmm1, xmm2 			; 66 0f f5 ca
pmaddwd xmm1, [rax] 			; 66 0f f5 08
pmaddwd xmm1, dqword [rax] 		; 66 0f f5 08
vpmaddwd xmm1, xmm2 			; c5 f1 f5 ca
vpmaddwd xmm1, [rax] 			; c5 f1 f5 08
vpmaddwd xmm1, dqword [rax] 		; c5 f1 f5 08
vpmaddwd xmm1, xmm2, xmm3 		; c5 e9 f5 cb
vpmaddwd xmm1, xmm2, [rax] 		; c5 e9 f5 08
vpmaddwd xmm1, xmm2, dqword [rax] 	; c5 e9 f5 08

pmaddubsw xmm1, xmm2 			; 66 0f 38 04 ca
pmaddubsw xmm1, [rax] 			; 66 0f 38 04 08
pmaddubsw xmm1, dqword [rax] 		; 66 0f 38 04 08
vpmaddubsw xmm1, xmm2 			; c4 e2 71 04 ca
vpmaddubsw xmm1, [rax] 			; c4 e2 71 04 08
vpmaddubsw xmm1, dqword [rax] 		; c4 e2 71 04 08
vpmaddubsw xmm1, xmm2, xmm3 		; c4 e2 69 04 cb
vpmaddubsw xmm1, xmm2, [rax] 		; c4 e2 69 04 08
vpmaddubsw xmm1, xmm2, dqword [rax] 	; c4 e2 69 04 08

pmaxsb xmm1, xmm2 			; 66 0f 38 3c ca
pmaxsb xmm1, [rax] 			; 66 0f 38 3c 08
pmaxsb xmm1, dqword [rax] 		; 66 0f 38 3c 08
vpmaxsb xmm1, xmm2 			; c4 e2 71 3c ca
vpmaxsb xmm1, [rax] 			; c4 e2 71 3c 08
vpmaxsb xmm1, dqword [rax] 		; c4 e2 71 3c 08
vpmaxsb xmm1, xmm2, xmm3 		; c4 e2 69 3c cb
vpmaxsb xmm1, xmm2, [rax] 		; c4 e2 69 3c 08
vpmaxsb xmm1, xmm2, dqword [rax] 	; c4 e2 69 3c 08

pmaxsw xmm1, xmm2 			; 66 0f ee ca
pmaxsw xmm1, [rax] 			; 66 0f ee 08
pmaxsw xmm1, dqword [rax] 		; 66 0f ee 08
vpmaxsw xmm1, xmm2 			; c5 f1 ee ca
vpmaxsw xmm1, [rax] 			; c5 f1 ee 08
vpmaxsw xmm1, dqword [rax] 		; c5 f1 ee 08
vpmaxsw xmm1, xmm2, xmm3 		; c5 e9 ee cb
vpmaxsw xmm1, xmm2, [rax] 		; c5 e9 ee 08
vpmaxsw xmm1, xmm2, dqword [rax] 	; c5 e9 ee 08

pmaxsd xmm1, xmm2 			; 66 0f 38 3d ca
pmaxsd xmm1, [rax] 			; 66 0f 38 3d 08
pmaxsd xmm1, dqword [rax] 		; 66 0f 38 3d 08
vpmaxsd xmm1, xmm2 			; c4 e2 71 3d ca
vpmaxsd xmm1, [rax] 			; c4 e2 71 3d 08
vpmaxsd xmm1, dqword [rax] 		; c4 e2 71 3d 08
vpmaxsd xmm1, xmm2, xmm3 		; c4 e2 69 3d cb
vpmaxsd xmm1, xmm2, [rax] 		; c4 e2 69 3d 08
vpmaxsd xmm1, xmm2, dqword [rax] 	; c4 e2 69 3d 08

pmaxub xmm1, xmm2 			; 66 0f de ca
pmaxub xmm1, [rax] 			; 66 0f de 08
pmaxub xmm1, dqword [rax] 		; 66 0f de 08
vpmaxub xmm1, xmm2 			; c5 f1 de ca
vpmaxub xmm1, [rax] 			; c5 f1 de 08
vpmaxub xmm1, dqword [rax] 		; c5 f1 de 08
vpmaxub xmm1, xmm2, xmm3 		; c5 e9 de cb
vpmaxub xmm1, xmm2, [rax] 		; c5 e9 de 08
vpmaxub xmm1, xmm2, dqword [rax] 	; c5 e9 de 08

pmaxuw xmm1, xmm2 			; 66 0f 38 3e ca
pmaxuw xmm1, [rax] 			; 66 0f 38 3e 08
pmaxuw xmm1, dqword [rax] 		; 66 0f 38 3e 08
vpmaxuw xmm1, xmm2 			; c4 e2 71 3e ca
vpmaxuw xmm1, [rax] 			; c4 e2 71 3e 08
vpmaxuw xmm1, dqword [rax] 		; c4 e2 71 3e 08
vpmaxuw xmm1, xmm2, xmm3 		; c4 e2 69 3e cb
vpmaxuw xmm1, xmm2, [rax] 		; c4 e2 69 3e 08
vpmaxuw xmm1, xmm2, dqword [rax] 	; c4 e2 69 3e 08

pmaxud xmm1, xmm2 			; 66 0f 38 3f ca
pmaxud xmm1, [rax] 			; 66 0f 38 3f 08
pmaxud xmm1, dqword [rax] 		; 66 0f 38 3f 08
vpmaxud xmm1, xmm2 			; c4 e2 71 3f ca
vpmaxud xmm1, [rax] 			; c4 e2 71 3f 08
vpmaxud xmm1, dqword [rax] 		; c4 e2 71 3f 08
vpmaxud xmm1, xmm2, xmm3 		; c4 e2 69 3f cb
vpmaxud xmm1, xmm2, [rax] 		; c4 e2 69 3f 08
vpmaxud xmm1, xmm2, dqword [rax] 	; c4 e2 69 3f 08

pminsb xmm1, xmm2 			; 66 0f 38 38 ca
pminsb xmm1, [rax] 			; 66 0f 38 38 08
pminsb xmm1, dqword [rax] 		; 66 0f 38 38 08
vpminsb xmm1, xmm2 			; c4 e2 71 38 ca
vpminsb xmm1, [rax] 			; c4 e2 71 38 08
vpminsb xmm1, dqword [rax] 		; c4 e2 71 38 08
vpminsb xmm1, xmm2, xmm3 		; c4 e2 69 38 cb
vpminsb xmm1, xmm2, [rax] 		; c4 e2 69 38 08
vpminsb xmm1, xmm2, dqword [rax] 	; c4 e2 69 38 08

pminsw xmm1, xmm2 			; 66 0f ea ca
pminsw xmm1, [rax] 			; 66 0f ea 08
pminsw xmm1, dqword [rax] 		; 66 0f ea 08
vpminsw xmm1, xmm2 			; c5 f1 ea ca
vpminsw xmm1, [rax] 			; c5 f1 ea 08
vpminsw xmm1, dqword [rax] 		; c5 f1 ea 08
vpminsw xmm1, xmm2, xmm3 		; c5 e9 ea cb
vpminsw xmm1, xmm2, [rax] 		; c5 e9 ea 08
vpminsw xmm1, xmm2, dqword [rax] 	; c5 e9 ea 08

pminsd xmm1, xmm2 			; 66 0f 38 39 ca
pminsd xmm1, [rax] 			; 66 0f 38 39 08
pminsd xmm1, dqword [rax] 		; 66 0f 38 39 08
vpminsd xmm1, xmm2 			; c4 e2 71 39 ca
vpminsd xmm1, [rax] 			; c4 e2 71 39 08
vpminsd xmm1, dqword [rax] 		; c4 e2 71 39 08
vpminsd xmm1, xmm2, xmm3 		; c4 e2 69 39 cb
vpminsd xmm1, xmm2, [rax] 		; c4 e2 69 39 08
vpminsd xmm1, xmm2, dqword [rax] 	; c4 e2 69 39 08

pminub xmm1, xmm2 			; 66 0f da ca
pminub xmm1, [rax] 			; 66 0f da 08
pminub xmm1, dqword [rax] 		; 66 0f da 08
vpminub xmm1, xmm2 			; c5 f1 da ca
vpminub xmm1, [rax] 			; c5 f1 da 08
vpminub xmm1, dqword [rax] 		; c5 f1 da 08
vpminub xmm1, xmm2, xmm3 		; c5 e9 da cb
vpminub xmm1, xmm2, [rax] 		; c5 e9 da 08
vpminub xmm1, xmm2, dqword [rax] 	; c5 e9 da 08

pminuw xmm1, xmm2 			; 66 0f 38 3a ca
pminuw xmm1, [rax] 			; 66 0f 38 3a 08
pminuw xmm1, dqword [rax] 		; 66 0f 38 3a 08
vpminuw xmm1, xmm2 			; c4 e2 71 3a ca
vpminuw xmm1, [rax] 			; c4 e2 71 3a 08
vpminuw xmm1, dqword [rax] 		; c4 e2 71 3a 08
vpminuw xmm1, xmm2, xmm3 		; c4 e2 69 3a cb
vpminuw xmm1, xmm2, [rax] 		; c4 e2 69 3a 08
vpminuw xmm1, xmm2, dqword [rax] 	; c4 e2 69 3a 08

pminud xmm1, xmm2 			; 66 0f 38 3b ca
pminud xmm1, [rax] 			; 66 0f 38 3b 08
pminud xmm1, dqword [rax] 		; 66 0f 38 3b 08
vpminud xmm1, xmm2 			; c4 e2 71 3b ca
vpminud xmm1, [rax] 			; c4 e2 71 3b 08
vpminud xmm1, dqword [rax] 		; c4 e2 71 3b 08
vpminud xmm1, xmm2, xmm3 		; c4 e2 69 3b cb
vpminud xmm1, xmm2, [rax] 		; c4 e2 69 3b 08
vpminud xmm1, xmm2, dqword [rax] 	; c4 e2 69 3b 08

pmovmskb eax, xmm1 			; 66 0f d7 c1
pmovmskb rax, xmm1 			; 66 0f d7 c1
vpmovmskb eax, xmm1 			; c5 f9 d7 c1
vpmovmskb rax, xmm1 			; c5 f9 d7 c1

pmovsxbw xmm1, xmm2 			; 66 0f 38 20 ca
pmovsxbw xmm1, [rax] 			; 66 0f 38 20 08
pmovsxbw xmm1, qword [rax] 		; 66 0f 38 20 08
vpmovsxbw xmm1, xmm2 			; c4 e2 79 20 ca
vpmovsxbw xmm1, [rax] 			; c4 e2 79 20 08
vpmovsxbw xmm1, qword [rax] 		; c4 e2 79 20 08

pmovsxbd xmm1, xmm2 			; 66 0f 38 21 ca
pmovsxbd xmm1, [rax] 			; 66 0f 38 21 08
pmovsxbd xmm1, dword [rax] 		; 66 0f 38 21 08
vpmovsxbd xmm1, xmm2 			; c4 e2 79 21 ca
vpmovsxbd xmm1, [rax] 			; c4 e2 79 21 08
vpmovsxbd xmm1, dword [rax] 		; c4 e2 79 21 08

pmovsxbq xmm1, xmm2 			; 66 0f 38 22 ca
pmovsxbq xmm1, [rax] 			; 66 0f 38 22 08
pmovsxbq xmm1, word [rax] 		; 66 0f 38 22 08
vpmovsxbq xmm1, xmm2 			; c4 e2 79 22 ca
vpmovsxbq xmm1, [rax] 			; c4 e2 79 22 08
vpmovsxbq xmm1, word [rax] 		; c4 e2 79 22 08

pmovsxwd xmm1, xmm2 			; 66 0f 38 23 ca
pmovsxwd xmm1, [rax] 			; 66 0f 38 23 08
pmovsxwd xmm1, qword [rax] 		; 66 0f 38 23 08
vpmovsxwd xmm1, xmm2 			; c4 e2 79 23 ca
vpmovsxwd xmm1, [rax] 			; c4 e2 79 23 08
vpmovsxwd xmm1, qword [rax] 		; c4 e2 79 23 08

pmovsxwq xmm1, xmm2 			; 66 0f 38 24 ca
pmovsxwq xmm1, [rax] 			; 66 0f 38 24 08
pmovsxwq xmm1, dword [rax] 		; 66 0f 38 24 08
vpmovsxwq xmm1, xmm2 			; c4 e2 79 24 ca
vpmovsxwq xmm1, [rax] 			; c4 e2 79 24 08
vpmovsxwq xmm1, dword [rax] 		; c4 e2 79 24 08

pmovsxdq xmm1, xmm2 			; 66 0f 38 25 ca
pmovsxdq xmm1, [rax] 			; 66 0f 38 25 08
pmovsxdq xmm1, qword [rax] 		; 66 0f 38 25 08
vpmovsxdq xmm1, xmm2 			; c4 e2 79 25 ca
vpmovsxdq xmm1, [rax] 			; c4 e2 79 25 08
vpmovsxdq xmm1, qword [rax] 		; c4 e2 79 25 08

pmovzxbw xmm1, xmm2 			; 66 0f 38 30 ca
pmovzxbw xmm1, [rax] 			; 66 0f 38 30 08
pmovzxbw xmm1, qword [rax] 		; 66 0f 38 30 08
vpmovzxbw xmm1, xmm2 			; c4 e2 79 30 ca
vpmovzxbw xmm1, [rax] 			; c4 e2 79 30 08
vpmovzxbw xmm1, qword [rax] 		; c4 e2 79 30 08

pmovzxbd xmm1, xmm2 			; 66 0f 38 31 ca
pmovzxbd xmm1, [rax] 			; 66 0f 38 31 08
pmovzxbd xmm1, dword [rax] 		; 66 0f 38 31 08
vpmovzxbd xmm1, xmm2 			; c4 e2 79 31 ca
vpmovzxbd xmm1, [rax] 			; c4 e2 79 31 08
vpmovzxbd xmm1, dword [rax] 		; c4 e2 79 31 08

pmovzxbq xmm1, xmm2 			; 66 0f 38 32 ca
pmovzxbq xmm1, [rax] 			; 66 0f 38 32 08
pmovzxbq xmm1, word [rax] 		; 66 0f 38 32 08
vpmovzxbq xmm1, xmm2 			; c4 e2 79 32 ca
vpmovzxbq xmm1, [rax] 			; c4 e2 79 32 08
vpmovzxbq xmm1, word [rax] 		; c4 e2 79 32 08

pmovzxwd xmm1, xmm2 			; 66 0f 38 33 ca
pmovzxwd xmm1, [rax] 			; 66 0f 38 33 08
pmovzxwd xmm1, qword [rax] 		; 66 0f 38 33 08
vpmovzxwd xmm1, xmm2 			; c4 e2 79 33 ca
vpmovzxwd xmm1, [rax] 			; c4 e2 79 33 08
vpmovzxwd xmm1, qword [rax] 		; c4 e2 79 33 08

pmovzxwq xmm1, xmm2 			; 66 0f 38 34 ca
pmovzxwq xmm1, [rax] 			; 66 0f 38 34 08
pmovzxwq xmm1, dword [rax] 		; 66 0f 38 34 08
vpmovzxwq xmm1, xmm2 			; c4 e2 79 34 ca
vpmovzxwq xmm1, [rax] 			; c4 e2 79 34 08
vpmovzxwq xmm1, dword [rax] 		; c4 e2 79 34 08

pmovzxdq xmm1, xmm2 			; 66 0f 38 35 ca
pmovzxdq xmm1, [rax] 			; 66 0f 38 35 08
pmovzxdq xmm1, qword [rax] 		; 66 0f 38 35 08
vpmovzxdq xmm1, xmm2 			; c4 e2 79 35 ca
vpmovzxdq xmm1, [rax] 			; c4 e2 79 35 08
vpmovzxdq xmm1, qword [rax] 		; c4 e2 79 35 08

pmulhuw xmm1, xmm2 			; 66 0f e4 ca
pmulhuw xmm1, [rax] 			; 66 0f e4 08
pmulhuw xmm1, dqword [rax] 		; 66 0f e4 08
vpmulhuw xmm1, xmm2 			; c5 f1 e4 ca
vpmulhuw xmm1, [rax] 			; c5 f1 e4 08
vpmulhuw xmm1, dqword [rax] 		; c5 f1 e4 08
vpmulhuw xmm1, xmm2, xmm3 		; c5 e9 e4 cb
vpmulhuw xmm1, xmm2, [rax] 		; c5 e9 e4 08
vpmulhuw xmm1, xmm2, dqword [rax] 	; c5 e9 e4 08

pmulhrsw xmm1, xmm2 			; 66 0f 38 0b ca
pmulhrsw xmm1, [rax] 			; 66 0f 38 0b 08
pmulhrsw xmm1, dqword [rax] 		; 66 0f 38 0b 08
vpmulhrsw xmm1, xmm2 			; c4 e2 71 0b ca
vpmulhrsw xmm1, [rax] 			; c4 e2 71 0b 08
vpmulhrsw xmm1, dqword [rax] 		; c4 e2 71 0b 08
vpmulhrsw xmm1, xmm2, xmm3 		; c4 e2 69 0b cb
vpmulhrsw xmm1, xmm2, [rax] 		; c4 e2 69 0b 08
vpmulhrsw xmm1, xmm2, dqword [rax] 	; c4 e2 69 0b 08

pmulhw xmm1, xmm2 			; 66 0f e5 ca
pmulhw xmm1, [rax] 			; 66 0f e5 08
pmulhw xmm1, dqword [rax] 		; 66 0f e5 08
vpmulhw xmm1, xmm2 			; c5 f1 e5 ca
vpmulhw xmm1, [rax] 			; c5 f1 e5 08
vpmulhw xmm1, dqword [rax] 		; c5 f1 e5 08
vpmulhw xmm1, xmm2, xmm3 		; c5 e9 e5 cb
vpmulhw xmm1, xmm2, [rax] 		; c5 e9 e5 08
vpmulhw xmm1, xmm2, dqword [rax] 	; c5 e9 e5 08

pmullw xmm1, xmm2 			; 66 0f d5 ca
pmullw xmm1, [rax] 			; 66 0f d5 08
pmullw xmm1, dqword [rax] 		; 66 0f d5 08
vpmullw xmm1, xmm2 			; c5 f1 d5 ca
vpmullw xmm1, [rax] 			; c5 f1 d5 08
vpmullw xmm1, dqword [rax] 		; c5 f1 d5 08
vpmullw xmm1, xmm2, xmm3 		; c5 e9 d5 cb
vpmullw xmm1, xmm2, [rax] 		; c5 e9 d5 08
vpmullw xmm1, xmm2, dqword [rax] 	; c5 e9 d5 08

pmulld xmm1, xmm2 			; 66 0f 38 40 ca
pmulld xmm1, [rax] 			; 66 0f 38 40 08
pmulld xmm1, dqword [rax] 		; 66 0f 38 40 08
vpmulld xmm1, xmm2 			; c4 e2 71 40 ca
vpmulld xmm1, [rax] 			; c4 e2 71 40 08
vpmulld xmm1, dqword [rax] 		; c4 e2 71 40 08
vpmulld xmm1, xmm2, xmm3 		; c4 e2 69 40 cb
vpmulld xmm1, xmm2, [rax] 		; c4 e2 69 40 08
vpmulld xmm1, xmm2, dqword [rax] 	; c4 e2 69 40 08

pmuludq xmm1, xmm2 			; 66 0f f4 ca
pmuludq xmm1, [rax] 			; 66 0f f4 08
pmuludq xmm1, dqword [rax] 		; 66 0f f4 08
vpmuludq xmm1, xmm2 			; c5 f1 f4 ca
vpmuludq xmm1, [rax] 			; c5 f1 f4 08
vpmuludq xmm1, dqword [rax] 		; c5 f1 f4 08
vpmuludq xmm1, xmm2, xmm3 		; c5 e9 f4 cb
vpmuludq xmm1, xmm2, [rax] 		; c5 e9 f4 08
vpmuludq xmm1, xmm2, dqword [rax] 	; c5 e9 f4 08

pmuldq xmm1, xmm2 			; 66 0f 38 28 ca
pmuldq xmm1, [rax] 			; 66 0f 38 28 08
pmuldq xmm1, dqword [rax] 		; 66 0f 38 28 08
vpmuldq xmm1, xmm2 			; c4 e2 71 28 ca
vpmuldq xmm1, [rax] 			; c4 e2 71 28 08
vpmuldq xmm1, dqword [rax] 		; c4 e2 71 28 08
vpmuldq xmm1, xmm2, xmm3 		; c4 e2 69 28 cb
vpmuldq xmm1, xmm2, [rax] 		; c4 e2 69 28 08
vpmuldq xmm1, xmm2, dqword [rax] 	; c4 e2 69 28 08

por xmm1, xmm2 				; 66 0f eb ca
por xmm1, [rax] 			; 66 0f eb 08
por xmm1, dqword [rax] 			; 66 0f eb 08
vpor xmm1, xmm2 			; c5 f1 eb ca
vpor xmm1, [rax] 			; c5 f1 eb 08
vpor xmm1, dqword [rax] 		; c5 f1 eb 08
vpor xmm1, xmm2, xmm3 			; c5 e9 eb cb
vpor xmm1, xmm2, [rax] 			; c5 e9 eb 08
vpor xmm1, xmm2, dqword [rax] 		; c5 e9 eb 08

psadbw xmm1, xmm2 			; 66 0f f6 ca
psadbw xmm1, [rax] 			; 66 0f f6 08
psadbw xmm1, dqword [rax] 		; 66 0f f6 08
vpsadbw xmm1, xmm2 			; c5 f1 f6 ca
vpsadbw xmm1, [rax] 			; c5 f1 f6 08
vpsadbw xmm1, dqword [rax] 		; c5 f1 f6 08
vpsadbw xmm1, xmm2, xmm3 		; c5 e9 f6 cb
vpsadbw xmm1, xmm2, [rax] 		; c5 e9 f6 08
vpsadbw xmm1, xmm2, dqword [rax] 	; c5 e9 f6 08

pshufb xmm1, xmm2 			; 66 0f 38 00 ca
pshufb xmm1, [rax] 			; 66 0f 38 00 08
pshufb xmm1, dqword [rax] 		; 66 0f 38 00 08
vpshufb xmm1, xmm2 			; c4 e2 71 00 ca
vpshufb xmm1, [rax] 			; c4 e2 71 00 08
vpshufb xmm1, dqword [rax] 		; c4 e2 71 00 08
vpshufb xmm1, xmm2, xmm3 		; c4 e2 69 00 cb
vpshufb xmm1, xmm2, [rax] 		; c4 e2 69 00 08
vpshufb xmm1, xmm2, dqword [rax] 	; c4 e2 69 00 08

pshufd xmm1, xmm2, 5 			; 66 0f 70 ca 05
pshufd xmm1, [rax], byte 5 		; 66 0f 70 08 05
pshufd xmm1, dqword [rax], 5 		; 66 0f 70 08 05
vpshufd xmm1, xmm2, 5 			; c5 f9 70 ca 05
vpshufd xmm1, [rax], byte 5 		; c5 f9 70 08 05
vpshufd xmm1, dqword [rax], 5 		; c5 f9 70 08 05

pshufhw xmm1, xmm2, 5 			; f3 0f 70 ca 05
pshufhw xmm1, [rax], byte 5 		; f3 0f 70 08 05
pshufhw xmm1, dqword [rax], 5 		; f3 0f 70 08 05
vpshufhw xmm1, xmm2, 5 			; c5 fa 70 ca 05
vpshufhw xmm1, [rax], byte 5 		; c5 fa 70 08 05
vpshufhw xmm1, dqword [rax], 5 		; c5 fa 70 08 05

pshuflw xmm1, xmm2, 5 			; f2 0f 70 ca 05
pshuflw xmm1, [rax], byte 5 		; f2 0f 70 08 05
pshuflw xmm1, dqword [rax], 5 		; f2 0f 70 08 05
vpshuflw xmm1, xmm2, 5 			; c5 fb 70 ca 05
vpshuflw xmm1, [rax], byte 5 		; c5 fb 70 08 05
vpshuflw xmm1, dqword [rax], 5 		; c5 fb 70 08 05

psignb xmm1, xmm2 			; 66 0f 38 08 ca
psignb xmm1, [rax] 			; 66 0f 38 08 08
psignb xmm1, dqword [rax] 		; 66 0f 38 08 08
vpsignb xmm1, xmm2 			; c4 e2 71 08 ca
vpsignb xmm1, [rax] 			; c4 e2 71 08 08
vpsignb xmm1, dqword [rax] 		; c4 e2 71 08 08
vpsignb xmm1, xmm2, xmm3 		; c4 e2 69 08 cb
vpsignb xmm1, xmm2, [rax] 		; c4 e2 69 08 08
vpsignb xmm1, xmm2, dqword [rax] 	; c4 e2 69 08 08

psignw xmm1, xmm2 			; 66 0f 38 09 ca
psignw xmm1, [rax] 			; 66 0f 38 09 08
psignw xmm1, dqword [rax] 		; 66 0f 38 09 08
vpsignw xmm1, xmm2 			; c4 e2 71 09 ca
vpsignw xmm1, [rax] 			; c4 e2 71 09 08
vpsignw xmm1, dqword [rax] 		; c4 e2 71 09 08
vpsignw xmm1, xmm2, xmm3 		; c4 e2 69 09 cb
vpsignw xmm1, xmm2, [rax] 		; c4 e2 69 09 08
vpsignw xmm1, xmm2, dqword [rax] 	; c4 e2 69 09 08

psignd xmm1, xmm2 			; 66 0f 38 0a ca
psignd xmm1, [rax] 			; 66 0f 38 0a 08
psignd xmm1, dqword [rax] 		; 66 0f 38 0a 08
vpsignd xmm1, xmm2 			; c4 e2 71 0a ca
vpsignd xmm1, [rax] 			; c4 e2 71 0a 08
vpsignd xmm1, dqword [rax] 		; c4 e2 71 0a 08
vpsignd xmm1, xmm2, xmm3 		; c4 e2 69 0a cb
vpsignd xmm1, xmm2, [rax] 		; c4 e2 69 0a 08
vpsignd xmm1, xmm2, dqword [rax] 	; c4 e2 69 0a 08

; Test these with high regs as it goes into VEX.B (REX.B)
pslldq xmm11, 5 			; 66 41 0f 73 fb 05
pslldq xmm11, byte 5 			; 66 41 0f 73 fb 05
vpslldq xmm11, 5 			; c4 c1 21 73 fb 05
vpslldq xmm11, byte 5 			; c4 c1 21 73 fb 05
vpslldq xmm11, xmm12, 5 		; c4 c1 21 73 fc 05
vpslldq xmm11, xmm12, byte 5 		; c4 c1 21 73 fc 05

pslldq xmm1, 5 				; 66 0f 73 f9 05
pslldq xmm1, byte 5 			; 66 0f 73 f9 05
vpslldq xmm1, 5 			; c5 f1 73 f9 05
vpslldq xmm1, byte 5 			; c5 f1 73 f9 05
vpslldq xmm1, xmm2, 5 			; c5 f1 73 fa 05
vpslldq xmm1, xmm2, byte 5 		; c5 f1 73 fa 05

psrldq xmm1, 5 				; 66 0f 73 d9 05
psrldq xmm1, byte 5 			; 66 0f 73 d9 05
vpsrldq xmm1, 5 			; c5 f1 73 d9 05
vpsrldq xmm1, byte 5 			; c5 f1 73 d9 05
vpsrldq xmm1, xmm2, 5 			; c5 f1 73 da 05
vpsrldq xmm1, xmm2, byte 5 		; c5 f1 73 da 05

psllw xmm1, xmm2 			; 66 0f f1 ca
psllw xmm1, [rax] 			; 66 0f f1 08
psllw xmm1, dqword [rax] 		; 66 0f f1 08
vpsllw xmm1, xmm2 			; c5 f1 f1 ca
vpsllw xmm1, [rax] 			; c5 f1 f1 08
vpsllw xmm1, dqword [rax] 		; c5 f1 f1 08
vpsllw xmm1, xmm2, xmm3 		; c5 e9 f1 cb
vpsllw xmm1, xmm2, [rax] 		; c5 e9 f1 08
vpsllw xmm1, xmm2, dqword [rax] 	; c5 e9 f1 08
psllw xmm1, 5 				; 66 0f 71 f1 05
psllw xmm1, byte 5 			; 66 0f 71 f1 05
vpsllw xmm1, 5 				; c5 f1 71 f1 05
vpsllw xmm1, byte 5 			; c5 f1 71 f1 05
vpsllw xmm1, xmm2, 5 			; c5 f1 71 f2 05
vpsllw xmm1, xmm2, byte 5 		; c5 f1 71 f2 05

pslld xmm1, xmm2 			; 66 0f f2 ca
pslld xmm1, [rax] 			; 66 0f f2 08
pslld xmm1, dqword [rax] 		; 66 0f f2 08
vpslld xmm1, xmm2 			; c5 f1 f2 ca
vpslld xmm1, [rax] 			; c5 f1 f2 08
vpslld xmm1, dqword [rax] 		; c5 f1 f2 08
vpslld xmm1, xmm2, xmm3 		; c5 e9 f2 cb
vpslld xmm1, xmm2, [rax] 		; c5 e9 f2 08
vpslld xmm1, xmm2, dqword [rax] 	; c5 e9 f2 08
pslld xmm1, 5 				; 66 0f 72 f1 05
pslld xmm1, byte 5 			; 66 0f 72 f1 05
vpslld xmm1, 5 				; c5 f1 72 f1 05
vpslld xmm1, byte 5 			; c5 f1 72 f1 05
vpslld xmm1, xmm2, 5 			; c5 f1 72 f2 05
vpslld xmm1, xmm2, byte 5 		; c5 f1 72 f2 05

psllq xmm1, xmm2 			; 66 0f f3 ca
psllq xmm1, [rax] 			; 66 0f f3 08
psllq xmm1, dqword [rax] 		; 66 0f f3 08
vpsllq xmm1, xmm2 			; c5 f1 f3 ca
vpsllq xmm1, [rax] 			; c5 f1 f3 08
vpsllq xmm1, dqword [rax] 		; c5 f1 f3 08
vpsllq xmm1, xmm2, xmm3 		; c5 e9 f3 cb
vpsllq xmm1, xmm2, [rax] 		; c5 e9 f3 08
vpsllq xmm1, xmm2, dqword [rax] 	; c5 e9 f3 08
psllq xmm1, 5 				; 66 0f 73 f1 05
psllq xmm1, byte 5 			; 66 0f 73 f1 05
vpsllq xmm1, 5 				; c5 f1 73 f1 05
vpsllq xmm1, byte 5 			; c5 f1 73 f1 05
vpsllq xmm1, xmm2, 5 			; c5 f1 73 f2 05
vpsllq xmm1, xmm2, byte 5 		; c5 f1 73 f2 05

psraw xmm1, xmm2 			; 66 0f e1 ca
psraw xmm1, [rax] 			; 66 0f e1 08
psraw xmm1, dqword [rax] 		; 66 0f e1 08
vpsraw xmm1, xmm2 			; c5 f1 e1 ca
vpsraw xmm1, [rax] 			; c5 f1 e1 08
vpsraw xmm1, dqword [rax] 		; c5 f1 e1 08
vpsraw xmm1, xmm2, xmm3 		; c5 e9 e1 cb
vpsraw xmm1, xmm2, [rax] 		; c5 e9 e1 08
vpsraw xmm1, xmm2, dqword [rax] 	; c5 e9 e1 08
psraw xmm1, 5 				; 66 0f 71 e1 05
psraw xmm1, byte 5 			; 66 0f 71 e1 05
vpsraw xmm1, 5 				; c5 f1 71 e1 05
vpsraw xmm1, byte 5 			; c5 f1 71 e1 05
vpsraw xmm1, xmm2, 5 			; c5 f1 71 e2 05
vpsraw xmm1, xmm2, byte 5 		; c5 f1 71 e2 05

psrad xmm1, xmm2 			; 66 0f e2 ca
psrad xmm1, [rax] 			; 66 0f e2 08
psrad xmm1, dqword [rax] 		; 66 0f e2 08
vpsrad xmm1, xmm2 			; c5 f1 e2 ca
vpsrad xmm1, [rax] 			; c5 f1 e2 08
vpsrad xmm1, dqword [rax] 		; c5 f1 e2 08
vpsrad xmm1, xmm2, xmm3 		; c5 e9 e2 cb
vpsrad xmm1, xmm2, [rax] 		; c5 e9 e2 08
vpsrad xmm1, xmm2, dqword [rax] 	; c5 e9 e2 08
psrad xmm1, 5 				; 66 0f 72 e1 05
psrad xmm1, byte 5 			; 66 0f 72 e1 05
vpsrad xmm1, 5 				; c5 f1 72 e1 05
vpsrad xmm1, byte 5 			; c5 f1 72 e1 05
vpsrad xmm1, xmm2, 5 			; c5 f1 72 e2 05
vpsrad xmm1, xmm2, byte 5 		; c5 f1 72 e2 05

psrlw xmm1, xmm2 			; 66 0f d1 ca
psrlw xmm1, [rax] 			; 66 0f d1 08
psrlw xmm1, dqword [rax] 		; 66 0f d1 08
vpsrlw xmm1, xmm2 			; c5 f1 d1 ca
vpsrlw xmm1, [rax] 			; c5 f1 d1 08
vpsrlw xmm1, dqword [rax] 		; c5 f1 d1 08
vpsrlw xmm1, xmm2, xmm3 		; c5 e9 d1 cb
vpsrlw xmm1, xmm2, [rax] 		; c5 e9 d1 08
vpsrlw xmm1, xmm2, dqword [rax] 	; c5 e9 d1 08
psrlw xmm1, 5 				; 66 0f 71 d1 05
psrlw xmm1, byte 5 			; 66 0f 71 d1 05
vpsrlw xmm1, 5 				; c5 f1 71 d1 05
vpsrlw xmm1, byte 5 			; c5 f1 71 d1 05
vpsrlw xmm1, xmm2, 5 			; c5 f1 71 d2 05
vpsrlw xmm1, xmm2, byte 5 		; c5 f1 71 d2 05

psrld xmm1, xmm2 			; 66 0f d2 ca
psrld xmm1, [rax] 			; 66 0f d2 08
psrld xmm1, dqword [rax] 		; 66 0f d2 08
vpsrld xmm1, xmm2 			; c5 f1 d2 ca
vpsrld xmm1, [rax] 			; c5 f1 d2 08
vpsrld xmm1, dqword [rax] 		; c5 f1 d2 08
vpsrld xmm1, xmm2, xmm3 		; c5 e9 d2 cb
vpsrld xmm1, xmm2, [rax] 		; c5 e9 d2 08
vpsrld xmm1, xmm2, dqword [rax] 	; c5 e9 d2 08
psrld xmm1, 5 				; 66 0f 72 d1 05
psrld xmm1, byte 5 			; 66 0f 72 d1 05
vpsrld xmm1, 5 				; c5 f1 72 d1 05
vpsrld xmm1, byte 5 			; c5 f1 72 d1 05
vpsrld xmm1, xmm2, 5 			; c5 f1 72 d2 05
vpsrld xmm1, xmm2, byte 5 		; c5 f1 72 d2 05

psrlq xmm1, xmm2 			; 66 0f d3 ca
psrlq xmm1, [rax] 			; 66 0f d3 08
psrlq xmm1, dqword [rax] 		; 66 0f d3 08
vpsrlq xmm1, xmm2 			; c5 f1 d3 ca
vpsrlq xmm1, [rax] 			; c5 f1 d3 08
vpsrlq xmm1, dqword [rax] 		; c5 f1 d3 08
vpsrlq xmm1, xmm2, xmm3 		; c5 e9 d3 cb
vpsrlq xmm1, xmm2, [rax] 		; c5 e9 d3 08
vpsrlq xmm1, xmm2, dqword [rax] 	; c5 e9 d3 08
psrlq xmm1, 5 				; 66 0f 73 d1 05
psrlq xmm1, byte 5 			; 66 0f 73 d1 05
vpsrlq xmm1, 5 				; c5 f1 73 d1 05
vpsrlq xmm1, byte 5 			; c5 f1 73 d1 05
vpsrlq xmm1, xmm2, 5 			; c5 f1 73 d2 05
vpsrlq xmm1, xmm2, byte 5 		; c5 f1 73 d2 05

ptest xmm1, xmm2 			; 66 0f 38 17 ca
ptest xmm1, [rax] 			; 66 0f 38 17 08
ptest xmm1, dqword [rax] 		; 66 0f 38 17 08
vptest xmm1, xmm2 			; c4 e2 79 17 ca
vptest xmm1, [rax] 			; c4 e2 79 17 08
vptest xmm1, dqword [rax] 		; c4 e2 79 17 08
vptest ymm1, ymm2 			; c4 e2 7d 17 ca
vptest ymm1, [rax] 			; c4 e2 7d 17 08
vptest ymm1, yword [rax] 		; c4 e2 7d 17 08

vtestps xmm1, xmm2 			; c4 e2 79 0e ca
vtestps xmm1, [rax] 			; c4 e2 79 0e 08
vtestps xmm1, dqword [rax] 		; c4 e2 79 0e 08
vtestps ymm1, ymm2 			; c4 e2 7d 0e ca
vtestps ymm1, [rax] 			; c4 e2 7d 0e 08
vtestps ymm1, yword [rax] 		; c4 e2 7d 0e 08

vtestpd xmm1, xmm2 			; c4 e2 79 0f ca
vtestpd xmm1, [rax] 			; c4 e2 79 0f 08
vtestpd xmm1, dqword [rax] 		; c4 e2 79 0f 08
vtestpd ymm1, ymm2 			; c4 e2 7d 0f ca
vtestpd ymm1, [rax] 			; c4 e2 7d 0f 08
vtestpd ymm1, yword [rax] 		; c4 e2 7d 0f 08

psubb xmm1, xmm2 			; 66 0f f8 ca
psubb xmm1, [rax] 			; 66 0f f8 08
psubb xmm1, dqword [rax] 		; 66 0f f8 08
vpsubb xmm1, xmm2 			; c5 f1 f8 ca
vpsubb xmm1, [rax] 			; c5 f1 f8 08
vpsubb xmm1, dqword [rax] 		; c5 f1 f8 08
vpsubb xmm1, xmm2, xmm3 		; c5 e9 f8 cb
vpsubb xmm1, xmm2, [rax] 		; c5 e9 f8 08
vpsubb xmm1, xmm2, dqword [rax] 	; c5 e9 f8 08

psubw xmm1, xmm2 			; 66 0f f9 ca
psubw xmm1, [rax] 			; 66 0f f9 08
psubw xmm1, dqword [rax] 		; 66 0f f9 08
vpsubw xmm1, xmm2 			; c5 f1 f9 ca
vpsubw xmm1, [rax] 			; c5 f1 f9 08
vpsubw xmm1, dqword [rax] 		; c5 f1 f9 08
vpsubw xmm1, xmm2, xmm3 		; c5 e9 f9 cb
vpsubw xmm1, xmm2, [rax] 		; c5 e9 f9 08
vpsubw xmm1, xmm2, dqword [rax] 	; c5 e9 f9 08

psubd xmm1, xmm2 			; 66 0f fa ca
psubd xmm1, [rax] 			; 66 0f fa 08
psubd xmm1, dqword [rax] 		; 66 0f fa 08
vpsubd xmm1, xmm2 			; c5 f1 fa ca
vpsubd xmm1, [rax] 			; c5 f1 fa 08
vpsubd xmm1, dqword [rax] 		; c5 f1 fa 08
vpsubd xmm1, xmm2, xmm3 		; c5 e9 fa cb
vpsubd xmm1, xmm2, [rax] 		; c5 e9 fa 08
vpsubd xmm1, xmm2, dqword [rax] 	; c5 e9 fa 08

psubq xmm1, xmm2 			; 66 0f fb ca
psubq xmm1, [rax] 			; 66 0f fb 08
psubq xmm1, dqword [rax] 		; 66 0f fb 08
vpsubq xmm1, xmm2 			; c5 f1 fb ca
vpsubq xmm1, [rax] 			; c5 f1 fb 08
vpsubq xmm1, dqword [rax] 		; c5 f1 fb 08
vpsubq xmm1, xmm2, xmm3 		; c5 e9 fb cb
vpsubq xmm1, xmm2, [rax] 		; c5 e9 fb 08
vpsubq xmm1, xmm2, dqword [rax] 	; c5 e9 fb 08

psubsb xmm1, xmm2 			; 66 0f e8 ca
psubsb xmm1, [rax] 			; 66 0f e8 08
psubsb xmm1, dqword [rax] 		; 66 0f e8 08
vpsubsb xmm1, xmm2 			; c5 f1 e8 ca
vpsubsb xmm1, [rax] 			; c5 f1 e8 08
vpsubsb xmm1, dqword [rax] 		; c5 f1 e8 08
vpsubsb xmm1, xmm2, xmm3 		; c5 e9 e8 cb
vpsubsb xmm1, xmm2, [rax] 		; c5 e9 e8 08
vpsubsb xmm1, xmm2, dqword [rax] 	; c5 e9 e8 08

psubsw xmm1, xmm2 			; 66 0f e9 ca
psubsw xmm1, [rax] 			; 66 0f e9 08
psubsw xmm1, dqword [rax] 		; 66 0f e9 08
vpsubsw xmm1, xmm2 			; c5 f1 e9 ca
vpsubsw xmm1, [rax] 			; c5 f1 e9 08
vpsubsw xmm1, dqword [rax] 		; c5 f1 e9 08
vpsubsw xmm1, xmm2, xmm3 		; c5 e9 e9 cb
vpsubsw xmm1, xmm2, [rax] 		; c5 e9 e9 08
vpsubsw xmm1, xmm2, dqword [rax] 	; c5 e9 e9 08

psubusb xmm1, xmm2 			; 66 0f d8 ca
psubusb xmm1, [rax] 			; 66 0f d8 08
psubusb xmm1, dqword [rax] 		; 66 0f d8 08
vpsubusb xmm1, xmm2 			; c5 f1 d8 ca
vpsubusb xmm1, [rax] 			; c5 f1 d8 08
vpsubusb xmm1, dqword [rax] 		; c5 f1 d8 08
vpsubusb xmm1, xmm2, xmm3 		; c5 e9 d8 cb
vpsubusb xmm1, xmm2, [rax] 		; c5 e9 d8 08
vpsubusb xmm1, xmm2, dqword [rax] 	; c5 e9 d8 08

psubusw xmm1, xmm2 			; 66 0f d9 ca
psubusw xmm1, [rax] 			; 66 0f d9 08
psubusw xmm1, dqword [rax] 		; 66 0f d9 08
vpsubusw xmm1, xmm2 			; c5 f1 d9 ca
vpsubusw xmm1, [rax] 			; c5 f1 d9 08
vpsubusw xmm1, dqword [rax] 		; c5 f1 d9 08
vpsubusw xmm1, xmm2, xmm3 		; c5 e9 d9 cb
vpsubusw xmm1, xmm2, [rax] 		; c5 e9 d9 08
vpsubusw xmm1, xmm2, dqword [rax] 	; c5 e9 d9 08

punpckhbw xmm1, xmm2 			; 66 0f 68 ca
punpckhbw xmm1, [rax] 			; 66 0f 68 08
punpckhbw xmm1, dqword [rax] 		; 66 0f 68 08
vpunpckhbw xmm1, xmm2 			; c5 f1 68 ca
vpunpckhbw xmm1, [rax] 			; c5 f1 68 08
vpunpckhbw xmm1, dqword [rax] 		; c5 f1 68 08
vpunpckhbw xmm1, xmm2, xmm3 		; c5 e9 68 cb
vpunpckhbw xmm1, xmm2, [rax] 		; c5 e9 68 08
vpunpckhbw xmm1, xmm2, dqword [rax] 	; c5 e9 68 08

punpckhwd xmm1, xmm2 			; 66 0f 69 ca
punpckhwd xmm1, [rax] 			; 66 0f 69 08
punpckhwd xmm1, dqword [rax] 		; 66 0f 69 08
vpunpckhwd xmm1, xmm2 			; c5 f1 69 ca
vpunpckhwd xmm1, [rax] 			; c5 f1 69 08
vpunpckhwd xmm1, dqword [rax] 		; c5 f1 69 08
vpunpckhwd xmm1, xmm2, xmm3 		; c5 e9 69 cb
vpunpckhwd xmm1, xmm2, [rax] 		; c5 e9 69 08
vpunpckhwd xmm1, xmm2, dqword [rax] 	; c5 e9 69 08

punpckhdq xmm1, xmm2 			; 66 0f 6a ca
punpckhdq xmm1, [rax] 			; 66 0f 6a 08
punpckhdq xmm1, dqword [rax] 		; 66 0f 6a 08
vpunpckhdq xmm1, xmm2 			; c5 f1 6a ca
vpunpckhdq xmm1, [rax] 			; c5 f1 6a 08
vpunpckhdq xmm1, dqword [rax] 		; c5 f1 6a 08
vpunpckhdq xmm1, xmm2, xmm3 		; c5 e9 6a cb
vpunpckhdq xmm1, xmm2, [rax] 		; c5 e9 6a 08
vpunpckhdq xmm1, xmm2, dqword [rax] 	; c5 e9 6a 08

punpckhqdq xmm1, xmm2 			; 66 0f 6d ca
punpckhqdq xmm1, [rax] 			; 66 0f 6d 08
punpckhqdq xmm1, dqword [rax] 		; 66 0f 6d 08
vpunpckhqdq xmm1, xmm2 			; c5 f1 6d ca
vpunpckhqdq xmm1, [rax] 		; c5 f1 6d 08
vpunpckhqdq xmm1, dqword [rax] 		; c5 f1 6d 08
vpunpckhqdq xmm1, xmm2, xmm3 		; c5 e9 6d cb
vpunpckhqdq xmm1, xmm2, [rax] 		; c5 e9 6d 08
vpunpckhqdq xmm1, xmm2, dqword [rax] 	; c5 e9 6d 08

punpcklbw xmm1, xmm2 			; 66 0f 60 ca
punpcklbw xmm1, [rax] 			; 66 0f 60 08
punpcklbw xmm1, dqword [rax] 		; 66 0f 60 08
vpunpcklbw xmm1, xmm2 			; c5 f1 60 ca
vpunpcklbw xmm1, [rax] 			; c5 f1 60 08
vpunpcklbw xmm1, dqword [rax] 		; c5 f1 60 08
vpunpcklbw xmm1, xmm2, xmm3 		; c5 e9 60 cb
vpunpcklbw xmm1, xmm2, [rax] 		; c5 e9 60 08
vpunpcklbw xmm1, xmm2, dqword [rax] 	; c5 e9 60 08

punpcklwd xmm1, xmm2 			; 66 0f 61 ca
punpcklwd xmm1, [rax] 			; 66 0f 61 08
punpcklwd xmm1, dqword [rax] 		; 66 0f 61 08
vpunpcklwd xmm1, xmm2 			; c5 f1 61 ca
vpunpcklwd xmm1, [rax] 			; c5 f1 61 08
vpunpcklwd xmm1, dqword [rax] 		; c5 f1 61 08
vpunpcklwd xmm1, xmm2, xmm3 		; c5 e9 61 cb
vpunpcklwd xmm1, xmm2, [rax] 		; c5 e9 61 08
vpunpcklwd xmm1, xmm2, dqword [rax] 	; c5 e9 61 08

punpckldq xmm1, xmm2 			; 66 0f 62 ca
punpckldq xmm1, [rax] 			; 66 0f 62 08
punpckldq xmm1, dqword [rax] 		; 66 0f 62 08
vpunpckldq xmm1, xmm2 			; c5 f1 62 ca
vpunpckldq xmm1, [rax] 			; c5 f1 62 08
vpunpckldq xmm1, dqword [rax] 		; c5 f1 62 08
vpunpckldq xmm1, xmm2, xmm3 		; c5 e9 62 cb
vpunpckldq xmm1, xmm2, [rax] 		; c5 e9 62 08
vpunpckldq xmm1, xmm2, dqword [rax] 	; c5 e9 62 08

punpcklqdq xmm1, xmm2 			; 66 0f 6c ca
punpcklqdq xmm1, [rax] 			; 66 0f 6c 08
punpcklqdq xmm1, dqword [rax] 		; 66 0f 6c 08
vpunpcklqdq xmm1, xmm2 			; c5 f1 6c ca
vpunpcklqdq xmm1, [rax] 		; c5 f1 6c 08
vpunpcklqdq xmm1, dqword [rax] 		; c5 f1 6c 08
vpunpcklqdq xmm1, xmm2, xmm3 		; c5 e9 6c cb
vpunpcklqdq xmm1, xmm2, [rax] 		; c5 e9 6c 08
vpunpcklqdq xmm1, xmm2, dqword [rax] 	; c5 e9 6c 08

pxor xmm1, xmm2 			; 66 0f ef ca
pxor xmm1, [rax] 			; 66 0f ef 08
pxor xmm1, dqword [rax] 		; 66 0f ef 08
vpxor xmm1, xmm2 			; c5 f1 ef ca
vpxor xmm1, [rax] 			; c5 f1 ef 08
vpxor xmm1, dqword [rax] 		; c5 f1 ef 08
vpxor xmm1, xmm2, xmm3 			; c5 e9 ef cb
vpxor xmm1, xmm2, [rax] 		; c5 e9 ef 08
vpxor xmm1, xmm2, dqword [rax] 		; c5 e9 ef 08

rcpps xmm1, xmm2 			; 0f 53 ca
rcpps xmm1, [rax] 			; 0f 53 08
rcpps xmm1, dqword [rax] 		; 0f 53 08
vrcpps xmm1, xmm2 			; c5 f8 53 ca
vrcpps xmm1, [rax] 			; c5 f8 53 08
vrcpps xmm1, dqword [rax] 		; c5 f8 53 08
vrcpps ymm1, ymm2 			; c5 fc 53 ca
vrcpps ymm1, [rax] 			; c5 fc 53 08
vrcpps ymm1, yword [rax] 		; c5 fc 53 08

rcpss xmm1, xmm2 			; f3 0f 53 ca
rcpss xmm1, [rax] 			; f3 0f 53 08
rcpss xmm1, dword [rax] 		; f3 0f 53 08
vrcpss xmm1, xmm2 			; c5 f2 53 ca
vrcpss xmm1, [rax] 			; c5 f2 53 08
vrcpss xmm1, dword [rax] 		; c5 f2 53 08
vrcpss xmm1, xmm2, xmm3 		; c5 ea 53 cb
vrcpss xmm1, xmm2, [rax] 		; c5 ea 53 08
vrcpss xmm1, xmm2, dword [rax] 		; c5 ea 53 08

rsqrtps xmm1, xmm2 			; 0f 52 ca
rsqrtps xmm1, [rax] 			; 0f 52 08
rsqrtps xmm1, dqword [rax] 		; 0f 52 08
vrsqrtps xmm1, xmm2 			; c5 f8 52 ca
vrsqrtps xmm1, [rax] 			; c5 f8 52 08
vrsqrtps xmm1, dqword [rax] 		; c5 f8 52 08
vrsqrtps ymm1, ymm2 			; c5 fc 52 ca
vrsqrtps ymm1, [rax] 			; c5 fc 52 08
vrsqrtps ymm1, yword [rax] 		; c5 fc 52 08

rsqrtss xmm1, xmm2 			; f3 0f 52 ca
rsqrtss xmm1, [rax] 			; f3 0f 52 08
rsqrtss xmm1, dword [rax] 		; f3 0f 52 08
vrsqrtss xmm1, xmm2 			; c5 f2 52 ca
vrsqrtss xmm1, [rax] 			; c5 f2 52 08
vrsqrtss xmm1, dword [rax] 		; c5 f2 52 08
vrsqrtss xmm1, xmm2, xmm3 		; c5 ea 52 cb
vrsqrtss xmm1, xmm2, [rax] 		; c5 ea 52 08
vrsqrtss xmm1, xmm2, dword [rax] 	; c5 ea 52 08

roundpd xmm1, xmm2, 5 			; 66 0f 3a 09 ca 05
roundpd xmm1, [rax], byte 5 		; 66 0f 3a 09 08 05
roundpd xmm1, dqword [rax], 5 		; 66 0f 3a 09 08 05
vroundpd xmm1, xmm2, 5 			; c4 e3 79 09 ca 05
vroundpd xmm1, [rax], byte 5 		; c4 e3 79 09 08 05
vroundpd xmm1, dqword [rax], 5 		; c4 e3 79 09 08 05
vroundpd ymm1, ymm2, 5 			; c4 e3 7d 09 ca 05
vroundpd ymm1, [rax], byte 5 		; c4 e3 7d 09 08 05
vroundpd ymm1, yword [rax], 5 		; c4 e3 7d 09 08 05

roundps xmm1, xmm2, 5 			; 66 0f 3a 08 ca 05
roundps xmm1, [rax], byte 5 		; 66 0f 3a 08 08 05
roundps xmm1, dqword [rax], 5 		; 66 0f 3a 08 08 05
vroundps xmm1, xmm2, 5 			; c4 e3 79 08 ca 05
vroundps xmm1, [rax], byte 5 		; c4 e3 79 08 08 05
vroundps xmm1, dqword [rax], 5 		; c4 e3 79 08 08 05
vroundps ymm1, ymm2, 5 			; c4 e3 7d 08 ca 05
vroundps ymm1, [rax], byte 5 		; c4 e3 7d 08 08 05
vroundps ymm1, yword [rax], 5 		; c4 e3 7d 08 08 05

roundsd xmm1, xmm2, 5 			; 66 0f 3a 0b ca 05
roundsd xmm1, [rax], byte 5 		; 66 0f 3a 0b 08 05
roundsd xmm1, qword [rax], 5 		; 66 0f 3a 0b 08 05
vroundsd xmm1, xmm2, 5 			; c4 e3 71 0b ca 05
vroundsd xmm1, [rax], byte 5 		; c4 e3 71 0b 08 05
vroundsd xmm1, qword [rax], 5 		; c4 e3 71 0b 08 05
vroundsd xmm1, xmm2, xmm3, 5 		; c4 e3 69 0b cb 05
vroundsd xmm1, xmm2, [rax], byte 5 	; c4 e3 69 0b 08 05
vroundsd xmm1, xmm2, qword [rax], 5 	; c4 e3 69 0b 08 05

roundss xmm1, xmm2, 5 			; 66 0f 3a 0a ca 05
roundss xmm1, [rax], byte 5 		; 66 0f 3a 0a 08 05
roundss xmm1, dword [rax], 5 		; 66 0f 3a 0a 08 05
vroundss xmm1, xmm2, 5 			; c4 e3 71 0a ca 05
vroundss xmm1, [rax], byte 5 		; c4 e3 71 0a 08 05
vroundss xmm1, dword [rax], 5 		; c4 e3 71 0a 08 05
vroundss xmm1, xmm2, xmm3, 5 		; c4 e3 69 0a cb 05
vroundss xmm1, xmm2, [rax], byte 5 	; c4 e3 69 0a 08 05
vroundss xmm1, xmm2, dword [rax], 5 	; c4 e3 69 0a 08 05

shufpd xmm1, xmm2, 5 			; 66 0f c6 ca 05
shufpd xmm1, [rax], byte 5 		; 66 0f c6 08 05
shufpd xmm1, dqword [rax], 5 		; 66 0f c6 08 05
vshufpd xmm1, xmm2, 5 			; c5 f1 c6 ca 05
vshufpd xmm1, [rax], byte 5 		; c5 f1 c6 08 05
vshufpd xmm1, dqword [rax], 5 		; c5 f1 c6 08 05
vshufpd xmm1, xmm2, xmm3, 5 		; c5 e9 c6 cb 05
vshufpd xmm1, xmm2, [rax], byte 5 	; c5 e9 c6 08 05
vshufpd xmm1, xmm2, dqword [rax], 5 	; c5 e9 c6 08 05
vshufpd ymm1, ymm2, ymm3, 5 		; c5 ed c6 cb 05
vshufpd ymm1, ymm2, [rax], byte 5 	; c5 ed c6 08 05
vshufpd ymm1, ymm2, yword [rax], 5 	; c5 ed c6 08 05

shufps xmm1, xmm2, 5 			; 0f c6 ca 05
shufps xmm1, [rax], byte 5 		; 0f c6 08 05
shufps xmm1, dqword [rax], 5 		; 0f c6 08 05
vshufps xmm1, xmm2, 5 			; c5 f0 c6 ca 05
vshufps xmm1, [rax], byte 5 		; c5 f0 c6 08 05
vshufps xmm1, dqword [rax], 5 		; c5 f0 c6 08 05
vshufps xmm1, xmm2, xmm3, 5 		; c5 e8 c6 cb 05
vshufps xmm1, xmm2, [rax], byte 5 	; c5 e8 c6 08 05
vshufps xmm1, xmm2, dqword [rax], 5 	; c5 e8 c6 08 05
vshufps ymm1, ymm2, ymm3, 5 		; c5 ec c6 cb 05
vshufps ymm1, ymm2, [rax], byte 5 	; c5 ec c6 08 05
vshufps ymm1, ymm2, yword [rax], 5 	; c5 ec c6 08 05

sqrtpd xmm1, xmm2 			; 66 0f 51 ca
sqrtpd xmm1, [rax] 			; 66 0f 51 08
sqrtpd xmm1, dqword [rax] 		; 66 0f 51 08
vsqrtpd xmm1, xmm2 			; c5 f9 51 ca
vsqrtpd xmm1, [rax] 			; c5 f9 51 08
vsqrtpd xmm1, dqword [rax] 		; c5 f9 51 08
vsqrtpd ymm1, ymm2 			; c5 fd 51 ca
vsqrtpd ymm1, [rax] 			; c5 fd 51 08
vsqrtpd ymm1, yword [rax] 		; c5 fd 51 08

sqrtps xmm1, xmm2 			; 0f 51 ca
sqrtps xmm1, [rax] 			; 0f 51 08
sqrtps xmm1, dqword [rax] 		; 0f 51 08
vsqrtps xmm1, xmm2 			; c5 f8 51 ca
vsqrtps xmm1, [rax] 			; c5 f8 51 08
vsqrtps xmm1, dqword [rax] 		; c5 f8 51 08
vsqrtps ymm1, ymm2 			; c5 fc 51 ca
vsqrtps ymm1, [rax] 			; c5 fc 51 08
vsqrtps ymm1, yword [rax] 		; c5 fc 51 08

sqrtsd xmm1, xmm2 			; f2 0f 51 ca
sqrtsd xmm1, [rax] 			; f2 0f 51 08
sqrtsd xmm1, qword [rax] 		; f2 0f 51 08
vsqrtsd xmm1, xmm2 			; c5 f3 51 ca
vsqrtsd xmm1, [rax] 			; c5 f3 51 08
vsqrtsd xmm1, qword [rax] 		; c5 f3 51 08
vsqrtsd xmm1, xmm2, xmm3 		; c5 eb 51 cb
vsqrtsd xmm1, xmm2, [rax] 		; c5 eb 51 08
vsqrtsd xmm1, xmm2, qword [rax] 	; c5 eb 51 08

sqrtss xmm1, xmm2 			; f3 0f 51 ca
sqrtss xmm1, [rax] 			; f3 0f 51 08
sqrtss xmm1, dword [rax] 		; f3 0f 51 08
vsqrtss xmm1, xmm2 			; c5 f2 51 ca
vsqrtss xmm1, [rax] 			; c5 f2 51 08
vsqrtss xmm1, dword [rax] 		; c5 f2 51 08
vsqrtss xmm1, xmm2, xmm3 		; c5 ea 51 cb
vsqrtss xmm1, xmm2, [rax] 		; c5 ea 51 08
vsqrtss xmm1, xmm2, dword [rax] 	; c5 ea 51 08

stmxcsr [rax] 				; 0f ae 18
stmxcsr dword [rax] 			; 0f ae 18
vstmxcsr [rax] 				; c5 f8 ae 18
vstmxcsr dword [rax] 			; c5 f8 ae 18

subpd xmm1, xmm2 			; 66 0f 5c ca
subpd xmm1, [rax] 			; 66 0f 5c 08
subpd xmm1, dqword [rax] 		; 66 0f 5c 08
vsubpd xmm1, xmm2 			; c5 f1 5c ca
vsubpd xmm1, [rax] 			; c5 f1 5c 08
vsubpd xmm1, dqword [rax] 		; c5 f1 5c 08
vsubpd xmm1, xmm2, xmm3 		; c5 e9 5c cb
vsubpd xmm1, xmm2, [rax] 		; c5 e9 5c 08
vsubpd xmm1, xmm2, dqword [rax] 	; c5 e9 5c 08
vsubpd ymm1, ymm2, ymm3 		; c5 ed 5c cb
vsubpd ymm1, ymm2, [rax] 		; c5 ed 5c 08
vsubpd ymm1, ymm2, yword [rax] 		; c5 ed 5c 08

subps xmm1, xmm2 			; 0f 5c ca
subps xmm1, [rax] 			; 0f 5c 08
subps xmm1, dqword [rax] 		; 0f 5c 08
vsubps xmm1, xmm2 			; c5 f0 5c ca
vsubps xmm1, [rax] 			; c5 f0 5c 08
vsubps xmm1, dqword [rax] 		; c5 f0 5c 08
vsubps xmm1, xmm2, xmm3 		; c5 e8 5c cb
vsubps xmm1, xmm2, [rax] 		; c5 e8 5c 08
vsubps xmm1, xmm2, dqword [rax] 	; c5 e8 5c 08
vsubps ymm1, ymm2, ymm3 		; c5 ec 5c cb
vsubps ymm1, ymm2, [rax] 		; c5 ec 5c 08
vsubps ymm1, ymm2, yword [rax] 		; c5 ec 5c 08

subsd xmm1, xmm2 			; f2 0f 5c ca
subsd xmm1, [rax] 			; f2 0f 5c 08
subsd xmm1, qword [rax] 		; f2 0f 5c 08
vsubsd xmm1, xmm2 			; c5 f3 5c ca
vsubsd xmm1, [rax] 			; c5 f3 5c 08
vsubsd xmm1, qword [rax] 		; c5 f3 5c 08
vsubsd xmm1, xmm2, xmm3 		; c5 eb 5c cb
vsubsd xmm1, xmm2, [rax] 		; c5 eb 5c 08
vsubsd xmm1, xmm2, qword [rax] 		; c5 eb 5c 08

subss xmm1, xmm2 			; f3 0f 5c ca
subss xmm1, [rax] 			; f3 0f 5c 08
subss xmm1, dword [rax] 		; f3 0f 5c 08
vsubss xmm1, xmm2 			; c5 f2 5c ca
vsubss xmm1, [rax] 			; c5 f2 5c 08
vsubss xmm1, dword [rax] 		; c5 f2 5c 08
vsubss xmm1, xmm2, xmm3 		; c5 ea 5c cb
vsubss xmm1, xmm2, [rax] 		; c5 ea 5c 08
vsubss xmm1, xmm2, dword [rax] 		; c5 ea 5c 08

ucomisd xmm1, xmm2 			; 66 0f 2e ca
ucomisd xmm1, [rax] 			; 66 0f 2e 08
ucomisd xmm1, qword [rax] 		; 66 0f 2e 08
vucomisd xmm1, xmm2 			; c5 f9 2e ca
vucomisd xmm1, [rax] 			; c5 f9 2e 08
vucomisd xmm1, qword [rax] 		; c5 f9 2e 08

ucomiss xmm1, xmm2 			; 0f 2e ca
ucomiss xmm1, [rax] 			; 0f 2e 08
ucomiss xmm1, dword [rax] 		; 0f 2e 08
vucomiss xmm1, xmm2 			; c5 f8 2e ca
vucomiss xmm1, [rax] 			; c5 f8 2e 08
vucomiss xmm1, dword [rax] 		; c5 f8 2e 08

unpckhpd xmm1, xmm2 			; 66 0f 15 ca
unpckhpd xmm1, [rax] 			; 66 0f 15 08
unpckhpd xmm1, dqword [rax] 		; 66 0f 15 08
vunpckhpd xmm1, xmm2 			; c5 f1 15 ca
vunpckhpd xmm1, [rax] 			; c5 f1 15 08
vunpckhpd xmm1, dqword [rax] 		; c5 f1 15 08
vunpckhpd xmm1, xmm2, xmm3 		; c5 e9 15 cb
vunpckhpd xmm1, xmm2, [rax] 		; c5 e9 15 08
vunpckhpd xmm1, xmm2, dqword [rax] 	; c5 e9 15 08
vunpckhpd ymm1, ymm2, ymm3 		; c5 ed 15 cb
vunpckhpd ymm1, ymm2, [rax] 		; c5 ed 15 08
vunpckhpd ymm1, ymm2, yword [rax] 	; c5 ed 15 08

unpckhps xmm1, xmm2 			; 0f 15 ca
unpckhps xmm1, [rax] 			; 0f 15 08
unpckhps xmm1, dqword [rax] 		; 0f 15 08
vunpckhps xmm1, xmm2 			; c5 f0 15 ca
vunpckhps xmm1, [rax] 			; c5 f0 15 08
vunpckhps xmm1, dqword [rax] 		; c5 f0 15 08
vunpckhps xmm1, xmm2, xmm3 		; c5 e8 15 cb
vunpckhps xmm1, xmm2, [rax] 		; c5 e8 15 08
vunpckhps xmm1, xmm2, dqword [rax] 	; c5 e8 15 08
vunpckhps ymm1, ymm2, ymm3 		; c5 ec 15 cb
vunpckhps ymm1, ymm2, [rax] 		; c5 ec 15 08
vunpckhps ymm1, ymm2, yword [rax] 	; c5 ec 15 08

unpcklpd xmm1, xmm2 			; 66 0f 14 ca
unpcklpd xmm1, [rax] 			; 66 0f 14 08
unpcklpd xmm1, dqword [rax] 		; 66 0f 14 08
vunpcklpd xmm1, xmm2 			; c5 f1 14 ca
vunpcklpd xmm1, [rax] 			; c5 f1 14 08
vunpcklpd xmm1, dqword [rax] 		; c5 f1 14 08
vunpcklpd xmm1, xmm2, xmm3 		; c5 e9 14 cb
vunpcklpd xmm1, xmm2, [rax] 		; c5 e9 14 08
vunpcklpd xmm1, xmm2, dqword [rax] 	; c5 e9 14 08
vunpcklpd ymm1, ymm2, ymm3 		; c5 ed 14 cb
vunpcklpd ymm1, ymm2, [rax] 		; c5 ed 14 08
vunpcklpd ymm1, ymm2, yword [rax] 	; c5 ed 14 08

unpcklps xmm1, xmm2 			; 0f 14 ca
unpcklps xmm1, [rax] 			; 0f 14 08
unpcklps xmm1, dqword [rax] 		; 0f 14 08
vunpcklps xmm1, xmm2 			; c5 f0 14 ca
vunpcklps xmm1, [rax] 			; c5 f0 14 08
vunpcklps xmm1, dqword [rax] 		; c5 f0 14 08
vunpcklps xmm1, xmm2, xmm3 		; c5 e8 14 cb
vunpcklps xmm1, xmm2, [rax] 		; c5 e8 14 08
vunpcklps xmm1, xmm2, dqword [rax] 	; c5 e8 14 08
vunpcklps ymm1, ymm2, ymm3 		; c5 ec 14 cb
vunpcklps ymm1, ymm2, [rax] 		; c5 ec 14 08
vunpcklps ymm1, ymm2, yword [rax] 	; c5 ec 14 08

xorpd xmm1, xmm2 			; 66 0f 57 ca
xorpd xmm1, [rax] 			; 66 0f 57 08
xorpd xmm1, dqword [rax] 		; 66 0f 57 08
vxorpd xmm1, xmm2 			; c5 f1 57 ca
vxorpd xmm1, [rax] 			; c5 f1 57 08
vxorpd xmm1, dqword [rax] 		; c5 f1 57 08
vxorpd xmm1, xmm2, xmm3 		; c5 e9 57 cb
vxorpd xmm1, xmm2, [rax] 		; c5 e9 57 08
vxorpd xmm1, xmm2, dqword [rax] 	; c5 e9 57 08
vxorpd ymm1, ymm2, ymm3 		; c5 ed 57 cb
vxorpd ymm1, ymm2, [rax] 		; c5 ed 57 08
vxorpd ymm1, ymm2, yword [rax] 		; c5 ed 57 08

xorps xmm1, xmm2 			; 0f 57 ca
xorps xmm1, [rax] 			; 0f 57 08
xorps xmm1, dqword [rax] 		; 0f 57 08
vxorps xmm1, xmm2 			; c5 f0 57 ca
vxorps xmm1, [rax] 			; c5 f0 57 08
vxorps xmm1, dqword [rax] 		; c5 f0 57 08
vxorps xmm1, xmm2, xmm3 		; c5 e8 57 cb
vxorps xmm1, xmm2, [rax] 		; c5 e8 57 08
vxorps xmm1, xmm2, dqword [rax] 	; c5 e8 57 08
vxorps ymm1, ymm2, ymm3 		; c5 ec 57 cb
vxorps ymm1, ymm2, [rax] 		; c5 ec 57 08
vxorps ymm1, ymm2, yword [rax] 		; c5 ec 57 08

vzeroall 				; c5 fc 77

vzeroupper 				; c5 f8 77

