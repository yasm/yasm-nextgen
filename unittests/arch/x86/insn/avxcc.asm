; Exhaustive test of AVX condition code aliases
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

cmpeqpd xmm1, xmm2			; 66 0f c2 ca 00
cmpltpd xmm1, xmm2			; 66 0f c2 ca 01
cmplepd xmm1, xmm2			; 66 0f c2 ca 02
cmpunordpd xmm1, xmm2			; 66 0f c2 ca 03
cmpneqpd xmm1, xmm2			; 66 0f c2 ca 04
cmpnltpd xmm1, xmm2			; 66 0f c2 ca 05
cmpnlepd xmm1, xmm2			; 66 0f c2 ca 06
cmpordpd xmm1, xmm2			; 66 0f c2 ca 07

vcmpeqpd xmm1, xmm2			; c5 f1 c2 ca 00
vcmpltpd xmm1, xmm2			; c5 f1 c2 ca 01
vcmplepd xmm1, xmm2			; c5 f1 c2 ca 02
vcmpunordpd xmm1, xmm2			; c5 f1 c2 ca 03
vcmpneqpd xmm1, xmm2			; c5 f1 c2 ca 04
vcmpnltpd xmm1, xmm2			; c5 f1 c2 ca 05
vcmpnlepd xmm1, xmm2			; c5 f1 c2 ca 06
vcmpordpd xmm1, xmm2			; c5 f1 c2 ca 07

vcmpeqpd xmm1, xmm2, xmm3		; c5 e9 c2 cb 00
vcmpltpd xmm1, xmm2, xmm3		; c5 e9 c2 cb 01
vcmplepd xmm1, xmm2, xmm3		; c5 e9 c2 cb 02
vcmpunordpd xmm1, xmm2, xmm3		; c5 e9 c2 cb 03
vcmpneqpd xmm1, xmm2, xmm3		; c5 e9 c2 cb 04
vcmpnltpd xmm1, xmm2, xmm3		; c5 e9 c2 cb 05
vcmpnlepd xmm1, xmm2, xmm3		; c5 e9 c2 cb 06
vcmpordpd xmm1, xmm2, xmm3		; c5 e9 c2 cb 07

vcmpeq_uqpd xmm1, xmm2, xmm3		; c5 e9 c2 cb 08
vcmpngepd xmm1, xmm2, xmm3		; c5 e9 c2 cb 09
vcmpngtpd xmm1, xmm2, xmm3		; c5 e9 c2 cb 0a
vcmpfalsepd xmm1, xmm2, xmm3		; c5 e9 c2 cb 0b
vcmpneq_oqpd xmm1, xmm2, xmm3		; c5 e9 c2 cb 0c
vcmpgepd xmm1, xmm2, xmm3		; c5 e9 c2 cb 0d
vcmpgtpd xmm1, xmm2, xmm3		; c5 e9 c2 cb 0e
vcmptruepd xmm1, xmm2, xmm3		; c5 e9 c2 cb 0f

vcmpeq_ospd xmm1, xmm2, xmm3		; c5 e9 c2 cb 10
vcmplt_oqpd xmm1, xmm2, xmm3		; c5 e9 c2 cb 11
vcmple_oqpd xmm1, xmm2, xmm3		; c5 e9 c2 cb 12
vcmpunord_spd xmm1, xmm2, xmm3		; c5 e9 c2 cb 13
vcmpneq_uspd xmm1, xmm2, xmm3		; c5 e9 c2 cb 14
vcmpnlt_uqpd xmm1, xmm2, xmm3		; c5 e9 c2 cb 15
vcmpnle_uqpd xmm1, xmm2, xmm3		; c5 e9 c2 cb 16
vcmpord_spd xmm1, xmm2, xmm3		; c5 e9 c2 cb 17

vcmpeq_uspd xmm1, xmm2, xmm3		; c5 e9 c2 cb 18
vcmpnge_uqpd xmm1, xmm2, xmm3		; c5 e9 c2 cb 19
vcmpngt_uqpd xmm1, xmm2, xmm3		; c5 e9 c2 cb 1a
vcmpfalse_ospd xmm1, xmm2, xmm3		; c5 e9 c2 cb 1b
vcmpneq_ospd xmm1, xmm2, xmm3		; c5 e9 c2 cb 1c
vcmpge_oqpd xmm1, xmm2, xmm3		; c5 e9 c2 cb 1d
vcmpgt_oqpd xmm1, xmm2, xmm3		; c5 e9 c2 cb 1e
vcmptrue_uspd xmm1, xmm2, xmm3		; c5 e9 c2 cb 1f

cmpeqpd xmm1, [rax]			; 66 0f c2 08 00
cmpltpd xmm1, [rax]			; 66 0f c2 08 01
cmplepd xmm1, [rax]			; 66 0f c2 08 02
cmpunordpd xmm1, [rax]			; 66 0f c2 08 03
cmpneqpd xmm1, [rax]			; 66 0f c2 08 04
cmpnltpd xmm1, [rax]			; 66 0f c2 08 05
cmpnlepd xmm1, [rax]			; 66 0f c2 08 06
cmpordpd xmm1, [rax]			; 66 0f c2 08 07

vcmpeqpd xmm1, [rax]			; c5 f1 c2 08 00
vcmpltpd xmm1, [rax]			; c5 f1 c2 08 01
vcmplepd xmm1, [rax]			; c5 f1 c2 08 02
vcmpunordpd xmm1, [rax]			; c5 f1 c2 08 03
vcmpneqpd xmm1, [rax]			; c5 f1 c2 08 04
vcmpnltpd xmm1, [rax]			; c5 f1 c2 08 05
vcmpnlepd xmm1, [rax]			; c5 f1 c2 08 06
vcmpordpd xmm1, [rax]			; c5 f1 c2 08 07

vcmpeqpd xmm1, xmm2, [rax]		; c5 e9 c2 08 00
vcmpltpd xmm1, xmm2, [rax]		; c5 e9 c2 08 01
vcmplepd xmm1, xmm2, [rax]		; c5 e9 c2 08 02
vcmpunordpd xmm1, xmm2, [rax]		; c5 e9 c2 08 03
vcmpneqpd xmm1, xmm2, [rax]		; c5 e9 c2 08 04
vcmpnltpd xmm1, xmm2, [rax]		; c5 e9 c2 08 05
vcmpnlepd xmm1, xmm2, [rax]		; c5 e9 c2 08 06
vcmpordpd xmm1, xmm2, [rax]		; c5 e9 c2 08 07

vcmpeq_uqpd xmm1, xmm2, [rax]		; c5 e9 c2 08 08
vcmpngepd xmm1, xmm2, [rax]		; c5 e9 c2 08 09
vcmpngtpd xmm1, xmm2, [rax]		; c5 e9 c2 08 0a
vcmpfalsepd xmm1, xmm2, [rax]		; c5 e9 c2 08 0b
vcmpneq_oqpd xmm1, xmm2, [rax]		; c5 e9 c2 08 0c
vcmpgepd xmm1, xmm2, [rax]		; c5 e9 c2 08 0d
vcmpgtpd xmm1, xmm2, [rax]		; c5 e9 c2 08 0e
vcmptruepd xmm1, xmm2, [rax]		; c5 e9 c2 08 0f

vcmpeq_ospd xmm1, xmm2, [rax]		; c5 e9 c2 08 10
vcmplt_oqpd xmm1, xmm2, [rax]		; c5 e9 c2 08 11
vcmple_oqpd xmm1, xmm2, [rax]		; c5 e9 c2 08 12
vcmpunord_spd xmm1, xmm2, [rax]		; c5 e9 c2 08 13
vcmpneq_uspd xmm1, xmm2, [rax]		; c5 e9 c2 08 14
vcmpnlt_uqpd xmm1, xmm2, [rax]		; c5 e9 c2 08 15
vcmpnle_uqpd xmm1, xmm2, [rax]		; c5 e9 c2 08 16
vcmpord_spd xmm1, xmm2, [rax]		; c5 e9 c2 08 17

vcmpeq_uspd xmm1, xmm2, [rax]		; c5 e9 c2 08 18
vcmpnge_uqpd xmm1, xmm2, [rax]		; c5 e9 c2 08 19
vcmpngt_uqpd xmm1, xmm2, [rax]		; c5 e9 c2 08 1a
vcmpfalse_ospd xmm1, xmm2, [rax]	; c5 e9 c2 08 1b
vcmpneq_ospd xmm1, xmm2, [rax]		; c5 e9 c2 08 1c
vcmpge_oqpd xmm1, xmm2, [rax]		; c5 e9 c2 08 1d
vcmpgt_oqpd xmm1, xmm2, [rax]		; c5 e9 c2 08 1e
vcmptrue_uspd xmm1, xmm2, [rax]		; c5 e9 c2 08 1f

cmpeqpd xmm1, dqword [rax]			; 66 0f c2 08 00
cmpltpd xmm1, dqword [rax]			; 66 0f c2 08 01
cmplepd xmm1, dqword [rax]			; 66 0f c2 08 02
cmpunordpd xmm1, dqword [rax]			; 66 0f c2 08 03
cmpneqpd xmm1, dqword [rax]			; 66 0f c2 08 04
cmpnltpd xmm1, dqword [rax]			; 66 0f c2 08 05
cmpnlepd xmm1, dqword [rax]			; 66 0f c2 08 06
cmpordpd xmm1, dqword [rax]			; 66 0f c2 08 07

vcmpeqpd xmm1, dqword [rax]			; c5 f1 c2 08 00
vcmpltpd xmm1, dqword [rax]			; c5 f1 c2 08 01
vcmplepd xmm1, dqword [rax]			; c5 f1 c2 08 02
vcmpunordpd xmm1, dqword [rax]			; c5 f1 c2 08 03
vcmpneqpd xmm1, dqword [rax]			; c5 f1 c2 08 04
vcmpnltpd xmm1, dqword [rax]			; c5 f1 c2 08 05
vcmpnlepd xmm1, dqword [rax]			; c5 f1 c2 08 06
vcmpordpd xmm1, dqword [rax]			; c5 f1 c2 08 07

vcmpeqpd xmm1, xmm2, dqword [rax]		; c5 e9 c2 08 00
vcmpltpd xmm1, xmm2, dqword [rax]		; c5 e9 c2 08 01
vcmplepd xmm1, xmm2, dqword [rax]		; c5 e9 c2 08 02
vcmpunordpd xmm1, xmm2, dqword [rax]		; c5 e9 c2 08 03
vcmpneqpd xmm1, xmm2, dqword [rax]		; c5 e9 c2 08 04
vcmpnltpd xmm1, xmm2, dqword [rax]		; c5 e9 c2 08 05
vcmpnlepd xmm1, xmm2, dqword [rax]		; c5 e9 c2 08 06
vcmpordpd xmm1, xmm2, dqword [rax]		; c5 e9 c2 08 07

vcmpeq_uqpd xmm1, xmm2, dqword [rax]		; c5 e9 c2 08 08
vcmpngepd xmm1, xmm2, dqword [rax]		; c5 e9 c2 08 09
vcmpngtpd xmm1, xmm2, dqword [rax]		; c5 e9 c2 08 0a
vcmpfalsepd xmm1, xmm2, dqword [rax]		; c5 e9 c2 08 0b
vcmpneq_oqpd xmm1, xmm2, dqword [rax]		; c5 e9 c2 08 0c
vcmpgepd xmm1, xmm2, dqword [rax]		; c5 e9 c2 08 0d
vcmpgtpd xmm1, xmm2, dqword [rax]		; c5 e9 c2 08 0e
vcmptruepd xmm1, xmm2, dqword [rax]		; c5 e9 c2 08 0f

vcmpeq_ospd xmm1, xmm2, dqword [rax]		; c5 e9 c2 08 10
vcmplt_oqpd xmm1, xmm2, dqword [rax]		; c5 e9 c2 08 11
vcmple_oqpd xmm1, xmm2, dqword [rax]		; c5 e9 c2 08 12
vcmpunord_spd xmm1, xmm2, dqword [rax]		; c5 e9 c2 08 13
vcmpneq_uspd xmm1, xmm2, dqword [rax]		; c5 e9 c2 08 14
vcmpnlt_uqpd xmm1, xmm2, dqword [rax]		; c5 e9 c2 08 15
vcmpnle_uqpd xmm1, xmm2, dqword [rax]		; c5 e9 c2 08 16
vcmpord_spd xmm1, xmm2, dqword [rax]		; c5 e9 c2 08 17

vcmpeq_uspd xmm1, xmm2, dqword [rax]		; c5 e9 c2 08 18
vcmpnge_uqpd xmm1, xmm2, dqword [rax]		; c5 e9 c2 08 19
vcmpngt_uqpd xmm1, xmm2, dqword [rax]		; c5 e9 c2 08 1a
vcmpfalse_ospd xmm1, xmm2, dqword [rax]		; c5 e9 c2 08 1b
vcmpneq_ospd xmm1, xmm2, dqword [rax]		; c5 e9 c2 08 1c
vcmpge_oqpd xmm1, xmm2, dqword [rax]		; c5 e9 c2 08 1d
vcmpgt_oqpd xmm1, xmm2, dqword [rax]		; c5 e9 c2 08 1e
vcmptrue_uspd xmm1, xmm2, dqword [rax]		; c5 e9 c2 08 1f

vcmpeqpd ymm1, ymm2, ymm3		; c5 ed c2 cb 00
vcmpltpd ymm1, ymm2, ymm3		; c5 ed c2 cb 01
vcmplepd ymm1, ymm2, ymm3		; c5 ed c2 cb 02
vcmpunordpd ymm1, ymm2, ymm3		; c5 ed c2 cb 03
vcmpneqpd ymm1, ymm2, ymm3		; c5 ed c2 cb 04
vcmpnltpd ymm1, ymm2, ymm3		; c5 ed c2 cb 05
vcmpnlepd ymm1, ymm2, ymm3		; c5 ed c2 cb 06
vcmpordpd ymm1, ymm2, ymm3		; c5 ed c2 cb 07

vcmpeq_uqpd ymm1, ymm2, ymm3		; c5 ed c2 cb 08
vcmpngepd ymm1, ymm2, ymm3		; c5 ed c2 cb 09
vcmpngtpd ymm1, ymm2, ymm3		; c5 ed c2 cb 0a
vcmpfalsepd ymm1, ymm2, ymm3		; c5 ed c2 cb 0b
vcmpneq_oqpd ymm1, ymm2, ymm3		; c5 ed c2 cb 0c
vcmpgepd ymm1, ymm2, ymm3		; c5 ed c2 cb 0d
vcmpgtpd ymm1, ymm2, ymm3		; c5 ed c2 cb 0e
vcmptruepd ymm1, ymm2, ymm3		; c5 ed c2 cb 0f

vcmpeq_ospd ymm1, ymm2, ymm3		; c5 ed c2 cb 10
vcmplt_oqpd ymm1, ymm2, ymm3		; c5 ed c2 cb 11
vcmple_oqpd ymm1, ymm2, ymm3		; c5 ed c2 cb 12
vcmpunord_spd ymm1, ymm2, ymm3		; c5 ed c2 cb 13
vcmpneq_uspd ymm1, ymm2, ymm3		; c5 ed c2 cb 14
vcmpnlt_uqpd ymm1, ymm2, ymm3		; c5 ed c2 cb 15
vcmpnle_uqpd ymm1, ymm2, ymm3		; c5 ed c2 cb 16
vcmpord_spd ymm1, ymm2, ymm3		; c5 ed c2 cb 17

vcmpeq_uspd ymm1, ymm2, ymm3		; c5 ed c2 cb 18
vcmpnge_uqpd ymm1, ymm2, ymm3		; c5 ed c2 cb 19
vcmpngt_uqpd ymm1, ymm2, ymm3		; c5 ed c2 cb 1a
vcmpfalse_ospd ymm1, ymm2, ymm3		; c5 ed c2 cb 1b
vcmpneq_ospd ymm1, ymm2, ymm3		; c5 ed c2 cb 1c
vcmpge_oqpd ymm1, ymm2, ymm3		; c5 ed c2 cb 1d
vcmpgt_oqpd ymm1, ymm2, ymm3		; c5 ed c2 cb 1e
vcmptrue_uspd ymm1, ymm2, ymm3		; c5 ed c2 cb 1f

vcmpeqpd ymm1, ymm2, [rax]		; c5 ed c2 08 00
vcmpltpd ymm1, ymm2, [rax]		; c5 ed c2 08 01
vcmplepd ymm1, ymm2, [rax]		; c5 ed c2 08 02
vcmpunordpd ymm1, ymm2, [rax]		; c5 ed c2 08 03
vcmpneqpd ymm1, ymm2, [rax]		; c5 ed c2 08 04
vcmpnltpd ymm1, ymm2, [rax]		; c5 ed c2 08 05
vcmpnlepd ymm1, ymm2, [rax]		; c5 ed c2 08 06
vcmpordpd ymm1, ymm2, [rax]		; c5 ed c2 08 07

vcmpeq_uqpd ymm1, ymm2, [rax]		; c5 ed c2 08 08
vcmpngepd ymm1, ymm2, [rax]		; c5 ed c2 08 09
vcmpngtpd ymm1, ymm2, [rax]		; c5 ed c2 08 0a
vcmpfalsepd ymm1, ymm2, [rax]		; c5 ed c2 08 0b
vcmpneq_oqpd ymm1, ymm2, [rax]		; c5 ed c2 08 0c
vcmpgepd ymm1, ymm2, [rax]		; c5 ed c2 08 0d
vcmpgtpd ymm1, ymm2, [rax]		; c5 ed c2 08 0e
vcmptruepd ymm1, ymm2, [rax]		; c5 ed c2 08 0f

vcmpeq_ospd ymm1, ymm2, [rax]		; c5 ed c2 08 10
vcmplt_oqpd ymm1, ymm2, [rax]		; c5 ed c2 08 11
vcmple_oqpd ymm1, ymm2, [rax]		; c5 ed c2 08 12
vcmpunord_spd ymm1, ymm2, [rax]		; c5 ed c2 08 13
vcmpneq_uspd ymm1, ymm2, [rax]		; c5 ed c2 08 14
vcmpnlt_uqpd ymm1, ymm2, [rax]		; c5 ed c2 08 15
vcmpnle_uqpd ymm1, ymm2, [rax]		; c5 ed c2 08 16
vcmpord_spd ymm1, ymm2, [rax]		; c5 ed c2 08 17

vcmpeq_uspd ymm1, ymm2, [rax]		; c5 ed c2 08 18
vcmpnge_uqpd ymm1, ymm2, [rax]		; c5 ed c2 08 19
vcmpngt_uqpd ymm1, ymm2, [rax]		; c5 ed c2 08 1a
vcmpfalse_ospd ymm1, ymm2, [rax]	; c5 ed c2 08 1b
vcmpneq_ospd ymm1, ymm2, [rax]		; c5 ed c2 08 1c
vcmpge_oqpd ymm1, ymm2, [rax]		; c5 ed c2 08 1d
vcmpgt_oqpd ymm1, ymm2, [rax]		; c5 ed c2 08 1e
vcmptrue_uspd ymm1, ymm2, [rax]		; c5 ed c2 08 1f

vcmpeqpd ymm1, ymm2, yword [rax]		; c5 ed c2 08 00
vcmpltpd ymm1, ymm2, yword [rax]		; c5 ed c2 08 01
vcmplepd ymm1, ymm2, yword [rax]		; c5 ed c2 08 02
vcmpunordpd ymm1, ymm2, yword [rax]		; c5 ed c2 08 03
vcmpneqpd ymm1, ymm2, yword [rax]		; c5 ed c2 08 04
vcmpnltpd ymm1, ymm2, yword [rax]		; c5 ed c2 08 05
vcmpnlepd ymm1, ymm2, yword [rax]		; c5 ed c2 08 06
vcmpordpd ymm1, ymm2, yword [rax]		; c5 ed c2 08 07

vcmpeq_uqpd ymm1, ymm2, yword [rax]		; c5 ed c2 08 08
vcmpngepd ymm1, ymm2, yword [rax]		; c5 ed c2 08 09
vcmpngtpd ymm1, ymm2, yword [rax]		; c5 ed c2 08 0a
vcmpfalsepd ymm1, ymm2, yword [rax]		; c5 ed c2 08 0b
vcmpneq_oqpd ymm1, ymm2, yword [rax]		; c5 ed c2 08 0c
vcmpgepd ymm1, ymm2, yword [rax]		; c5 ed c2 08 0d
vcmpgtpd ymm1, ymm2, yword [rax]		; c5 ed c2 08 0e
vcmptruepd ymm1, ymm2, yword [rax]		; c5 ed c2 08 0f

vcmpeq_ospd ymm1, ymm2, yword [rax]		; c5 ed c2 08 10
vcmplt_oqpd ymm1, ymm2, yword [rax]		; c5 ed c2 08 11
vcmple_oqpd ymm1, ymm2, yword [rax]		; c5 ed c2 08 12
vcmpunord_spd ymm1, ymm2, yword [rax]		; c5 ed c2 08 13
vcmpneq_uspd ymm1, ymm2, yword [rax]		; c5 ed c2 08 14
vcmpnlt_uqpd ymm1, ymm2, yword [rax]		; c5 ed c2 08 15
vcmpnle_uqpd ymm1, ymm2, yword [rax]		; c5 ed c2 08 16
vcmpord_spd ymm1, ymm2, yword [rax]		; c5 ed c2 08 17

vcmpeq_uspd ymm1, ymm2, yword [rax]		; c5 ed c2 08 18
vcmpnge_uqpd ymm1, ymm2, yword [rax]		; c5 ed c2 08 19
vcmpngt_uqpd ymm1, ymm2, yword [rax]		; c5 ed c2 08 1a
vcmpfalse_ospd ymm1, ymm2, yword [rax]		; c5 ed c2 08 1b
vcmpneq_ospd ymm1, ymm2, yword [rax]		; c5 ed c2 08 1c
vcmpge_oqpd ymm1, ymm2, yword [rax]		; c5 ed c2 08 1d
vcmpgt_oqpd ymm1, ymm2, yword [rax]		; c5 ed c2 08 1e
vcmptrue_uspd ymm1, ymm2, yword [rax]		; c5 ed c2 08 1f

;-----------------------------------------------------------------------------

cmpeqps xmm1, xmm2			; 0f c2 ca 00
cmpltps xmm1, xmm2			; 0f c2 ca 01
cmpleps xmm1, xmm2			; 0f c2 ca 02
cmpunordps xmm1, xmm2			; 0f c2 ca 03
cmpneqps xmm1, xmm2			; 0f c2 ca 04
cmpnltps xmm1, xmm2			; 0f c2 ca 05
cmpnleps xmm1, xmm2			; 0f c2 ca 06
cmpordps xmm1, xmm2			; 0f c2 ca 07

vcmpeqps xmm1, xmm2			; c5 f0 c2 ca 00
vcmpltps xmm1, xmm2			; c5 f0 c2 ca 01
vcmpleps xmm1, xmm2			; c5 f0 c2 ca 02
vcmpunordps xmm1, xmm2			; c5 f0 c2 ca 03
vcmpneqps xmm1, xmm2			; c5 f0 c2 ca 04
vcmpnltps xmm1, xmm2			; c5 f0 c2 ca 05
vcmpnleps xmm1, xmm2			; c5 f0 c2 ca 06
vcmpordps xmm1, xmm2			; c5 f0 c2 ca 07

vcmpeqps xmm1, xmm2, xmm3		; c5 e8 c2 cb 00
vcmpltps xmm1, xmm2, xmm3		; c5 e8 c2 cb 01
vcmpleps xmm1, xmm2, xmm3		; c5 e8 c2 cb 02
vcmpunordps xmm1, xmm2, xmm3		; c5 e8 c2 cb 03
vcmpneqps xmm1, xmm2, xmm3		; c5 e8 c2 cb 04
vcmpnltps xmm1, xmm2, xmm3		; c5 e8 c2 cb 05
vcmpnleps xmm1, xmm2, xmm3		; c5 e8 c2 cb 06
vcmpordps xmm1, xmm2, xmm3		; c5 e8 c2 cb 07

vcmpeq_uqps xmm1, xmm2, xmm3		; c5 e8 c2 cb 08
vcmpngeps xmm1, xmm2, xmm3		; c5 e8 c2 cb 09
vcmpngtps xmm1, xmm2, xmm3		; c5 e8 c2 cb 0a
vcmpfalseps xmm1, xmm2, xmm3		; c5 e8 c2 cb 0b
vcmpneq_oqps xmm1, xmm2, xmm3		; c5 e8 c2 cb 0c
vcmpgeps xmm1, xmm2, xmm3		; c5 e8 c2 cb 0d
vcmpgtps xmm1, xmm2, xmm3		; c5 e8 c2 cb 0e
vcmptrueps xmm1, xmm2, xmm3		; c5 e8 c2 cb 0f

vcmpeq_osps xmm1, xmm2, xmm3		; c5 e8 c2 cb 10
vcmplt_oqps xmm1, xmm2, xmm3		; c5 e8 c2 cb 11
vcmple_oqps xmm1, xmm2, xmm3		; c5 e8 c2 cb 12
vcmpunord_sps xmm1, xmm2, xmm3		; c5 e8 c2 cb 13
vcmpneq_usps xmm1, xmm2, xmm3		; c5 e8 c2 cb 14
vcmpnlt_uqps xmm1, xmm2, xmm3		; c5 e8 c2 cb 15
vcmpnle_uqps xmm1, xmm2, xmm3		; c5 e8 c2 cb 16
vcmpord_sps xmm1, xmm2, xmm3		; c5 e8 c2 cb 17

vcmpeq_usps xmm1, xmm2, xmm3		; c5 e8 c2 cb 18
vcmpnge_uqps xmm1, xmm2, xmm3		; c5 e8 c2 cb 19
vcmpngt_uqps xmm1, xmm2, xmm3		; c5 e8 c2 cb 1a
vcmpfalse_osps xmm1, xmm2, xmm3		; c5 e8 c2 cb 1b
vcmpneq_osps xmm1, xmm2, xmm3		; c5 e8 c2 cb 1c
vcmpge_oqps xmm1, xmm2, xmm3		; c5 e8 c2 cb 1d
vcmpgt_oqps xmm1, xmm2, xmm3		; c5 e8 c2 cb 1e
vcmptrue_usps xmm1, xmm2, xmm3		; c5 e8 c2 cb 1f

cmpeqps xmm1, [rax]			; 0f c2 08 00
cmpltps xmm1, [rax]			; 0f c2 08 01
cmpleps xmm1, [rax]			; 0f c2 08 02
cmpunordps xmm1, [rax]			; 0f c2 08 03
cmpneqps xmm1, [rax]			; 0f c2 08 04
cmpnltps xmm1, [rax]			; 0f c2 08 05
cmpnleps xmm1, [rax]			; 0f c2 08 06
cmpordps xmm1, [rax]			; 0f c2 08 07

vcmpeqps xmm1, [rax]			; c5 f0 c2 08 00
vcmpltps xmm1, [rax]			; c5 f0 c2 08 01
vcmpleps xmm1, [rax]			; c5 f0 c2 08 02
vcmpunordps xmm1, [rax]			; c5 f0 c2 08 03
vcmpneqps xmm1, [rax]			; c5 f0 c2 08 04
vcmpnltps xmm1, [rax]			; c5 f0 c2 08 05
vcmpnleps xmm1, [rax]			; c5 f0 c2 08 06
vcmpordps xmm1, [rax]			; c5 f0 c2 08 07

vcmpeqps xmm1, xmm2, [rax]		; c5 e8 c2 08 00
vcmpltps xmm1, xmm2, [rax]		; c5 e8 c2 08 01
vcmpleps xmm1, xmm2, [rax]		; c5 e8 c2 08 02
vcmpunordps xmm1, xmm2, [rax]		; c5 e8 c2 08 03
vcmpneqps xmm1, xmm2, [rax]		; c5 e8 c2 08 04
vcmpnltps xmm1, xmm2, [rax]		; c5 e8 c2 08 05
vcmpnleps xmm1, xmm2, [rax]		; c5 e8 c2 08 06
vcmpordps xmm1, xmm2, [rax]		; c5 e8 c2 08 07

vcmpeq_uqps xmm1, xmm2, [rax]		; c5 e8 c2 08 08
vcmpngeps xmm1, xmm2, [rax]		; c5 e8 c2 08 09
vcmpngtps xmm1, xmm2, [rax]		; c5 e8 c2 08 0a
vcmpfalseps xmm1, xmm2, [rax]		; c5 e8 c2 08 0b
vcmpneq_oqps xmm1, xmm2, [rax]		; c5 e8 c2 08 0c
vcmpgeps xmm1, xmm2, [rax]		; c5 e8 c2 08 0d
vcmpgtps xmm1, xmm2, [rax]		; c5 e8 c2 08 0e
vcmptrueps xmm1, xmm2, [rax]		; c5 e8 c2 08 0f

vcmpeq_osps xmm1, xmm2, [rax]		; c5 e8 c2 08 10
vcmplt_oqps xmm1, xmm2, [rax]		; c5 e8 c2 08 11
vcmple_oqps xmm1, xmm2, [rax]		; c5 e8 c2 08 12
vcmpunord_sps xmm1, xmm2, [rax]		; c5 e8 c2 08 13
vcmpneq_usps xmm1, xmm2, [rax]		; c5 e8 c2 08 14
vcmpnlt_uqps xmm1, xmm2, [rax]		; c5 e8 c2 08 15
vcmpnle_uqps xmm1, xmm2, [rax]		; c5 e8 c2 08 16
vcmpord_sps xmm1, xmm2, [rax]		; c5 e8 c2 08 17

vcmpeq_usps xmm1, xmm2, [rax]		; c5 e8 c2 08 18
vcmpnge_uqps xmm1, xmm2, [rax]		; c5 e8 c2 08 19
vcmpngt_uqps xmm1, xmm2, [rax]		; c5 e8 c2 08 1a
vcmpfalse_osps xmm1, xmm2, [rax]	; c5 e8 c2 08 1b
vcmpneq_osps xmm1, xmm2, [rax]		; c5 e8 c2 08 1c
vcmpge_oqps xmm1, xmm2, [rax]		; c5 e8 c2 08 1d
vcmpgt_oqps xmm1, xmm2, [rax]		; c5 e8 c2 08 1e
vcmptrue_usps xmm1, xmm2, [rax]		; c5 e8 c2 08 1f

cmpeqps xmm1, dqword [rax]			; 0f c2 08 00
cmpltps xmm1, dqword [rax]			; 0f c2 08 01
cmpleps xmm1, dqword [rax]			; 0f c2 08 02
cmpunordps xmm1, dqword [rax]			; 0f c2 08 03
cmpneqps xmm1, dqword [rax]			; 0f c2 08 04
cmpnltps xmm1, dqword [rax]			; 0f c2 08 05
cmpnleps xmm1, dqword [rax]			; 0f c2 08 06
cmpordps xmm1, dqword [rax]			; 0f c2 08 07

vcmpeqps xmm1, dqword [rax]			; c5 f0 c2 08 00
vcmpltps xmm1, dqword [rax]			; c5 f0 c2 08 01
vcmpleps xmm1, dqword [rax]			; c5 f0 c2 08 02
vcmpunordps xmm1, dqword [rax]			; c5 f0 c2 08 03
vcmpneqps xmm1, dqword [rax]			; c5 f0 c2 08 04
vcmpnltps xmm1, dqword [rax]			; c5 f0 c2 08 05
vcmpnleps xmm1, dqword [rax]			; c5 f0 c2 08 06
vcmpordps xmm1, dqword [rax]			; c5 f0 c2 08 07

vcmpeqps xmm1, xmm2, dqword [rax]		; c5 e8 c2 08 00
vcmpltps xmm1, xmm2, dqword [rax]		; c5 e8 c2 08 01
vcmpleps xmm1, xmm2, dqword [rax]		; c5 e8 c2 08 02
vcmpunordps xmm1, xmm2, dqword [rax]		; c5 e8 c2 08 03
vcmpneqps xmm1, xmm2, dqword [rax]		; c5 e8 c2 08 04
vcmpnltps xmm1, xmm2, dqword [rax]		; c5 e8 c2 08 05
vcmpnleps xmm1, xmm2, dqword [rax]		; c5 e8 c2 08 06
vcmpordps xmm1, xmm2, dqword [rax]		; c5 e8 c2 08 07

vcmpeq_uqps xmm1, xmm2, dqword [rax]		; c5 e8 c2 08 08
vcmpngeps xmm1, xmm2, dqword [rax]		; c5 e8 c2 08 09
vcmpngtps xmm1, xmm2, dqword [rax]		; c5 e8 c2 08 0a
vcmpfalseps xmm1, xmm2, dqword [rax]		; c5 e8 c2 08 0b
vcmpneq_oqps xmm1, xmm2, dqword [rax]		; c5 e8 c2 08 0c
vcmpgeps xmm1, xmm2, dqword [rax]		; c5 e8 c2 08 0d
vcmpgtps xmm1, xmm2, dqword [rax]		; c5 e8 c2 08 0e
vcmptrueps xmm1, xmm2, dqword [rax]		; c5 e8 c2 08 0f

vcmpeq_osps xmm1, xmm2, dqword [rax]		; c5 e8 c2 08 10
vcmplt_oqps xmm1, xmm2, dqword [rax]		; c5 e8 c2 08 11
vcmple_oqps xmm1, xmm2, dqword [rax]		; c5 e8 c2 08 12
vcmpunord_sps xmm1, xmm2, dqword [rax]		; c5 e8 c2 08 13
vcmpneq_usps xmm1, xmm2, dqword [rax]		; c5 e8 c2 08 14
vcmpnlt_uqps xmm1, xmm2, dqword [rax]		; c5 e8 c2 08 15
vcmpnle_uqps xmm1, xmm2, dqword [rax]		; c5 e8 c2 08 16
vcmpord_sps xmm1, xmm2, dqword [rax]		; c5 e8 c2 08 17

vcmpeq_usps xmm1, xmm2, dqword [rax]		; c5 e8 c2 08 18
vcmpnge_uqps xmm1, xmm2, dqword [rax]		; c5 e8 c2 08 19
vcmpngt_uqps xmm1, xmm2, dqword [rax]		; c5 e8 c2 08 1a
vcmpfalse_osps xmm1, xmm2, dqword [rax]		; c5 e8 c2 08 1b
vcmpneq_osps xmm1, xmm2, dqword [rax]		; c5 e8 c2 08 1c
vcmpge_oqps xmm1, xmm2, dqword [rax]		; c5 e8 c2 08 1d
vcmpgt_oqps xmm1, xmm2, dqword [rax]		; c5 e8 c2 08 1e
vcmptrue_usps xmm1, xmm2, dqword [rax]		; c5 e8 c2 08 1f

vcmpeqps ymm1, ymm2, ymm3		; c5 ec c2 cb 00
vcmpltps ymm1, ymm2, ymm3		; c5 ec c2 cb 01
vcmpleps ymm1, ymm2, ymm3		; c5 ec c2 cb 02
vcmpunordps ymm1, ymm2, ymm3		; c5 ec c2 cb 03
vcmpneqps ymm1, ymm2, ymm3		; c5 ec c2 cb 04
vcmpnltps ymm1, ymm2, ymm3		; c5 ec c2 cb 05
vcmpnleps ymm1, ymm2, ymm3		; c5 ec c2 cb 06
vcmpordps ymm1, ymm2, ymm3		; c5 ec c2 cb 07

vcmpeq_uqps ymm1, ymm2, ymm3		; c5 ec c2 cb 08
vcmpngeps ymm1, ymm2, ymm3		; c5 ec c2 cb 09
vcmpngtps ymm1, ymm2, ymm3		; c5 ec c2 cb 0a
vcmpfalseps ymm1, ymm2, ymm3		; c5 ec c2 cb 0b
vcmpneq_oqps ymm1, ymm2, ymm3		; c5 ec c2 cb 0c
vcmpgeps ymm1, ymm2, ymm3		; c5 ec c2 cb 0d
vcmpgtps ymm1, ymm2, ymm3		; c5 ec c2 cb 0e
vcmptrueps ymm1, ymm2, ymm3		; c5 ec c2 cb 0f

vcmpeq_osps ymm1, ymm2, ymm3		; c5 ec c2 cb 10
vcmplt_oqps ymm1, ymm2, ymm3		; c5 ec c2 cb 11
vcmple_oqps ymm1, ymm2, ymm3		; c5 ec c2 cb 12
vcmpunord_sps ymm1, ymm2, ymm3		; c5 ec c2 cb 13
vcmpneq_usps ymm1, ymm2, ymm3		; c5 ec c2 cb 14
vcmpnlt_uqps ymm1, ymm2, ymm3		; c5 ec c2 cb 15
vcmpnle_uqps ymm1, ymm2, ymm3		; c5 ec c2 cb 16
vcmpord_sps ymm1, ymm2, ymm3		; c5 ec c2 cb 17

vcmpeq_usps ymm1, ymm2, ymm3		; c5 ec c2 cb 18
vcmpnge_uqps ymm1, ymm2, ymm3		; c5 ec c2 cb 19
vcmpngt_uqps ymm1, ymm2, ymm3		; c5 ec c2 cb 1a
vcmpfalse_osps ymm1, ymm2, ymm3		; c5 ec c2 cb 1b
vcmpneq_osps ymm1, ymm2, ymm3		; c5 ec c2 cb 1c
vcmpge_oqps ymm1, ymm2, ymm3		; c5 ec c2 cb 1d
vcmpgt_oqps ymm1, ymm2, ymm3		; c5 ec c2 cb 1e
vcmptrue_usps ymm1, ymm2, ymm3		; c5 ec c2 cb 1f

vcmpeqps ymm1, ymm2, [rax]		; c5 ec c2 08 00
vcmpltps ymm1, ymm2, [rax]		; c5 ec c2 08 01
vcmpleps ymm1, ymm2, [rax]		; c5 ec c2 08 02
vcmpunordps ymm1, ymm2, [rax]		; c5 ec c2 08 03
vcmpneqps ymm1, ymm2, [rax]		; c5 ec c2 08 04
vcmpnltps ymm1, ymm2, [rax]		; c5 ec c2 08 05
vcmpnleps ymm1, ymm2, [rax]		; c5 ec c2 08 06
vcmpordps ymm1, ymm2, [rax]		; c5 ec c2 08 07

vcmpeq_uqps ymm1, ymm2, [rax]		; c5 ec c2 08 08
vcmpngeps ymm1, ymm2, [rax]		; c5 ec c2 08 09
vcmpngtps ymm1, ymm2, [rax]		; c5 ec c2 08 0a
vcmpfalseps ymm1, ymm2, [rax]		; c5 ec c2 08 0b
vcmpneq_oqps ymm1, ymm2, [rax]		; c5 ec c2 08 0c
vcmpgeps ymm1, ymm2, [rax]		; c5 ec c2 08 0d
vcmpgtps ymm1, ymm2, [rax]		; c5 ec c2 08 0e
vcmptrueps ymm1, ymm2, [rax]		; c5 ec c2 08 0f

vcmpeq_osps ymm1, ymm2, [rax]		; c5 ec c2 08 10
vcmplt_oqps ymm1, ymm2, [rax]		; c5 ec c2 08 11
vcmple_oqps ymm1, ymm2, [rax]		; c5 ec c2 08 12
vcmpunord_sps ymm1, ymm2, [rax]		; c5 ec c2 08 13
vcmpneq_usps ymm1, ymm2, [rax]		; c5 ec c2 08 14
vcmpnlt_uqps ymm1, ymm2, [rax]		; c5 ec c2 08 15
vcmpnle_uqps ymm1, ymm2, [rax]		; c5 ec c2 08 16
vcmpord_sps ymm1, ymm2, [rax]		; c5 ec c2 08 17

vcmpeq_usps ymm1, ymm2, [rax]		; c5 ec c2 08 18
vcmpnge_uqps ymm1, ymm2, [rax]		; c5 ec c2 08 19
vcmpngt_uqps ymm1, ymm2, [rax]		; c5 ec c2 08 1a
vcmpfalse_osps ymm1, ymm2, [rax]	; c5 ec c2 08 1b
vcmpneq_osps ymm1, ymm2, [rax]		; c5 ec c2 08 1c
vcmpge_oqps ymm1, ymm2, [rax]		; c5 ec c2 08 1d
vcmpgt_oqps ymm1, ymm2, [rax]		; c5 ec c2 08 1e
vcmptrue_usps ymm1, ymm2, [rax]		; c5 ec c2 08 1f

vcmpeqps ymm1, ymm2, yword [rax]		; c5 ec c2 08 00
vcmpltps ymm1, ymm2, yword [rax]		; c5 ec c2 08 01
vcmpleps ymm1, ymm2, yword [rax]		; c5 ec c2 08 02
vcmpunordps ymm1, ymm2, yword [rax]		; c5 ec c2 08 03
vcmpneqps ymm1, ymm2, yword [rax]		; c5 ec c2 08 04
vcmpnltps ymm1, ymm2, yword [rax]		; c5 ec c2 08 05
vcmpnleps ymm1, ymm2, yword [rax]		; c5 ec c2 08 06
vcmpordps ymm1, ymm2, yword [rax]		; c5 ec c2 08 07

vcmpeq_uqps ymm1, ymm2, yword [rax]		; c5 ec c2 08 08
vcmpngeps ymm1, ymm2, yword [rax]		; c5 ec c2 08 09
vcmpngtps ymm1, ymm2, yword [rax]		; c5 ec c2 08 0a
vcmpfalseps ymm1, ymm2, yword [rax]		; c5 ec c2 08 0b
vcmpneq_oqps ymm1, ymm2, yword [rax]		; c5 ec c2 08 0c
vcmpgeps ymm1, ymm2, yword [rax]		; c5 ec c2 08 0d
vcmpgtps ymm1, ymm2, yword [rax]		; c5 ec c2 08 0e
vcmptrueps ymm1, ymm2, yword [rax]		; c5 ec c2 08 0f

vcmpeq_osps ymm1, ymm2, yword [rax]		; c5 ec c2 08 10
vcmplt_oqps ymm1, ymm2, yword [rax]		; c5 ec c2 08 11
vcmple_oqps ymm1, ymm2, yword [rax]		; c5 ec c2 08 12
vcmpunord_sps ymm1, ymm2, yword [rax]		; c5 ec c2 08 13
vcmpneq_usps ymm1, ymm2, yword [rax]		; c5 ec c2 08 14
vcmpnlt_uqps ymm1, ymm2, yword [rax]		; c5 ec c2 08 15
vcmpnle_uqps ymm1, ymm2, yword [rax]		; c5 ec c2 08 16
vcmpord_sps ymm1, ymm2, yword [rax]		; c5 ec c2 08 17

vcmpeq_usps ymm1, ymm2, yword [rax]		; c5 ec c2 08 18
vcmpnge_uqps ymm1, ymm2, yword [rax]		; c5 ec c2 08 19
vcmpngt_uqps ymm1, ymm2, yword [rax]		; c5 ec c2 08 1a
vcmpfalse_osps ymm1, ymm2, yword [rax]		; c5 ec c2 08 1b
vcmpneq_osps ymm1, ymm2, yword [rax]		; c5 ec c2 08 1c
vcmpge_oqps ymm1, ymm2, yword [rax]		; c5 ec c2 08 1d
vcmpgt_oqps ymm1, ymm2, yword [rax]		; c5 ec c2 08 1e
vcmptrue_usps ymm1, ymm2, yword [rax]		; c5 ec c2 08 1f

;-----------------------------------------------------------------------------

cmpeqsd xmm1, xmm2			; f2 0f c2 ca 00
cmpltsd xmm1, xmm2			; f2 0f c2 ca 01
cmplesd xmm1, xmm2			; f2 0f c2 ca 02
cmpunordsd xmm1, xmm2			; f2 0f c2 ca 03
cmpneqsd xmm1, xmm2			; f2 0f c2 ca 04
cmpnltsd xmm1, xmm2			; f2 0f c2 ca 05
cmpnlesd xmm1, xmm2			; f2 0f c2 ca 06
cmpordsd xmm1, xmm2			; f2 0f c2 ca 07

vcmpeqsd xmm1, xmm2			; c5 f3 c2 ca 00
vcmpltsd xmm1, xmm2			; c5 f3 c2 ca 01
vcmplesd xmm1, xmm2			; c5 f3 c2 ca 02
vcmpunordsd xmm1, xmm2			; c5 f3 c2 ca 03
vcmpneqsd xmm1, xmm2			; c5 f3 c2 ca 04
vcmpnltsd xmm1, xmm2			; c5 f3 c2 ca 05
vcmpnlesd xmm1, xmm2			; c5 f3 c2 ca 06
vcmpordsd xmm1, xmm2			; c5 f3 c2 ca 07

vcmpeqsd xmm1, xmm2, xmm3		; c5 eb c2 cb 00
vcmpltsd xmm1, xmm2, xmm3		; c5 eb c2 cb 01
vcmplesd xmm1, xmm2, xmm3		; c5 eb c2 cb 02
vcmpunordsd xmm1, xmm2, xmm3		; c5 eb c2 cb 03
vcmpneqsd xmm1, xmm2, xmm3		; c5 eb c2 cb 04
vcmpnltsd xmm1, xmm2, xmm3		; c5 eb c2 cb 05
vcmpnlesd xmm1, xmm2, xmm3		; c5 eb c2 cb 06
vcmpordsd xmm1, xmm2, xmm3		; c5 eb c2 cb 07

vcmpeq_uqsd xmm1, xmm2, xmm3		; c5 eb c2 cb 08
vcmpngesd xmm1, xmm2, xmm3		; c5 eb c2 cb 09
vcmpngtsd xmm1, xmm2, xmm3		; c5 eb c2 cb 0a
vcmpfalsesd xmm1, xmm2, xmm3		; c5 eb c2 cb 0b
vcmpneq_oqsd xmm1, xmm2, xmm3		; c5 eb c2 cb 0c
vcmpgesd xmm1, xmm2, xmm3		; c5 eb c2 cb 0d
vcmpgtsd xmm1, xmm2, xmm3		; c5 eb c2 cb 0e
vcmptruesd xmm1, xmm2, xmm3		; c5 eb c2 cb 0f

vcmpeq_ossd xmm1, xmm2, xmm3		; c5 eb c2 cb 10
vcmplt_oqsd xmm1, xmm2, xmm3		; c5 eb c2 cb 11
vcmple_oqsd xmm1, xmm2, xmm3		; c5 eb c2 cb 12
vcmpunord_ssd xmm1, xmm2, xmm3		; c5 eb c2 cb 13
vcmpneq_ussd xmm1, xmm2, xmm3		; c5 eb c2 cb 14
vcmpnlt_uqsd xmm1, xmm2, xmm3		; c5 eb c2 cb 15
vcmpnle_uqsd xmm1, xmm2, xmm3		; c5 eb c2 cb 16
vcmpord_ssd xmm1, xmm2, xmm3		; c5 eb c2 cb 17

vcmpeq_ussd xmm1, xmm2, xmm3		; c5 eb c2 cb 18
vcmpnge_uqsd xmm1, xmm2, xmm3		; c5 eb c2 cb 19
vcmpngt_uqsd xmm1, xmm2, xmm3		; c5 eb c2 cb 1a
vcmpfalse_ossd xmm1, xmm2, xmm3		; c5 eb c2 cb 1b
vcmpneq_ossd xmm1, xmm2, xmm3		; c5 eb c2 cb 1c
vcmpge_oqsd xmm1, xmm2, xmm3		; c5 eb c2 cb 1d
vcmpgt_oqsd xmm1, xmm2, xmm3		; c5 eb c2 cb 1e
vcmptrue_ussd xmm1, xmm2, xmm3		; c5 eb c2 cb 1f

cmpeqsd xmm1, [rax]			; f2 0f c2 08 00
cmpltsd xmm1, [rax]			; f2 0f c2 08 01
cmplesd xmm1, [rax]			; f2 0f c2 08 02
cmpunordsd xmm1, [rax]			; f2 0f c2 08 03
cmpneqsd xmm1, [rax]			; f2 0f c2 08 04
cmpnltsd xmm1, [rax]			; f2 0f c2 08 05
cmpnlesd xmm1, [rax]			; f2 0f c2 08 06
cmpordsd xmm1, [rax]			; f2 0f c2 08 07

vcmpeqsd xmm1, [rax]			; c5 f3 c2 08 00
vcmpltsd xmm1, [rax]			; c5 f3 c2 08 01
vcmplesd xmm1, [rax]			; c5 f3 c2 08 02
vcmpunordsd xmm1, [rax]			; c5 f3 c2 08 03
vcmpneqsd xmm1, [rax]			; c5 f3 c2 08 04
vcmpnltsd xmm1, [rax]			; c5 f3 c2 08 05
vcmpnlesd xmm1, [rax]			; c5 f3 c2 08 06
vcmpordsd xmm1, [rax]			; c5 f3 c2 08 07

vcmpeqsd xmm1, xmm2, [rax]		; c5 eb c2 08 00
vcmpltsd xmm1, xmm2, [rax]		; c5 eb c2 08 01
vcmplesd xmm1, xmm2, [rax]		; c5 eb c2 08 02
vcmpunordsd xmm1, xmm2, [rax]		; c5 eb c2 08 03
vcmpneqsd xmm1, xmm2, [rax]		; c5 eb c2 08 04
vcmpnltsd xmm1, xmm2, [rax]		; c5 eb c2 08 05
vcmpnlesd xmm1, xmm2, [rax]		; c5 eb c2 08 06
vcmpordsd xmm1, xmm2, [rax]		; c5 eb c2 08 07

vcmpeq_uqsd xmm1, xmm2, [rax]		; c5 eb c2 08 08
vcmpngesd xmm1, xmm2, [rax]		; c5 eb c2 08 09
vcmpngtsd xmm1, xmm2, [rax]		; c5 eb c2 08 0a
vcmpfalsesd xmm1, xmm2, [rax]		; c5 eb c2 08 0b
vcmpneq_oqsd xmm1, xmm2, [rax]		; c5 eb c2 08 0c
vcmpgesd xmm1, xmm2, [rax]		; c5 eb c2 08 0d
vcmpgtsd xmm1, xmm2, [rax]		; c5 eb c2 08 0e
vcmptruesd xmm1, xmm2, [rax]		; c5 eb c2 08 0f

vcmpeq_ossd xmm1, xmm2, [rax]		; c5 eb c2 08 10
vcmplt_oqsd xmm1, xmm2, [rax]		; c5 eb c2 08 11
vcmple_oqsd xmm1, xmm2, [rax]		; c5 eb c2 08 12
vcmpunord_ssd xmm1, xmm2, [rax]		; c5 eb c2 08 13
vcmpneq_ussd xmm1, xmm2, [rax]		; c5 eb c2 08 14
vcmpnlt_uqsd xmm1, xmm2, [rax]		; c5 eb c2 08 15
vcmpnle_uqsd xmm1, xmm2, [rax]		; c5 eb c2 08 16
vcmpord_ssd xmm1, xmm2, [rax]		; c5 eb c2 08 17

vcmpeq_ussd xmm1, xmm2, [rax]		; c5 eb c2 08 18
vcmpnge_uqsd xmm1, xmm2, [rax]		; c5 eb c2 08 19
vcmpngt_uqsd xmm1, xmm2, [rax]		; c5 eb c2 08 1a
vcmpfalse_ossd xmm1, xmm2, [rax]	; c5 eb c2 08 1b
vcmpneq_ossd xmm1, xmm2, [rax]		; c5 eb c2 08 1c
vcmpge_oqsd xmm1, xmm2, [rax]		; c5 eb c2 08 1d
vcmpgt_oqsd xmm1, xmm2, [rax]		; c5 eb c2 08 1e
vcmptrue_ussd xmm1, xmm2, [rax]		; c5 eb c2 08 1f

cmpeqsd xmm1, qword [rax]			; f2 0f c2 08 00
cmpltsd xmm1, qword [rax]			; f2 0f c2 08 01
cmplesd xmm1, qword [rax]			; f2 0f c2 08 02
cmpunordsd xmm1, qword [rax]			; f2 0f c2 08 03
cmpneqsd xmm1, qword [rax]			; f2 0f c2 08 04
cmpnltsd xmm1, qword [rax]			; f2 0f c2 08 05
cmpnlesd xmm1, qword [rax]			; f2 0f c2 08 06
cmpordsd xmm1, qword [rax]			; f2 0f c2 08 07

vcmpeqsd xmm1, qword [rax]			; c5 f3 c2 08 00
vcmpltsd xmm1, qword [rax]			; c5 f3 c2 08 01
vcmplesd xmm1, qword [rax]			; c5 f3 c2 08 02
vcmpunordsd xmm1, qword [rax]			; c5 f3 c2 08 03
vcmpneqsd xmm1, qword [rax]			; c5 f3 c2 08 04
vcmpnltsd xmm1, qword [rax]			; c5 f3 c2 08 05
vcmpnlesd xmm1, qword [rax]			; c5 f3 c2 08 06
vcmpordsd xmm1, qword [rax]			; c5 f3 c2 08 07

vcmpeqsd xmm1, xmm2, qword [rax]		; c5 eb c2 08 00
vcmpltsd xmm1, xmm2, qword [rax]		; c5 eb c2 08 01
vcmplesd xmm1, xmm2, qword [rax]		; c5 eb c2 08 02
vcmpunordsd xmm1, xmm2, qword [rax]		; c5 eb c2 08 03
vcmpneqsd xmm1, xmm2, qword [rax]		; c5 eb c2 08 04
vcmpnltsd xmm1, xmm2, qword [rax]		; c5 eb c2 08 05
vcmpnlesd xmm1, xmm2, qword [rax]		; c5 eb c2 08 06
vcmpordsd xmm1, xmm2, qword [rax]		; c5 eb c2 08 07

vcmpeq_uqsd xmm1, xmm2, qword [rax]		; c5 eb c2 08 08
vcmpngesd xmm1, xmm2, qword [rax]		; c5 eb c2 08 09
vcmpngtsd xmm1, xmm2, qword [rax]		; c5 eb c2 08 0a
vcmpfalsesd xmm1, xmm2, qword [rax]		; c5 eb c2 08 0b
vcmpneq_oqsd xmm1, xmm2, qword [rax]		; c5 eb c2 08 0c
vcmpgesd xmm1, xmm2, qword [rax]		; c5 eb c2 08 0d
vcmpgtsd xmm1, xmm2, qword [rax]		; c5 eb c2 08 0e
vcmptruesd xmm1, xmm2, qword [rax]		; c5 eb c2 08 0f

vcmpeq_ossd xmm1, xmm2, qword [rax]		; c5 eb c2 08 10
vcmplt_oqsd xmm1, xmm2, qword [rax]		; c5 eb c2 08 11
vcmple_oqsd xmm1, xmm2, qword [rax]		; c5 eb c2 08 12
vcmpunord_ssd xmm1, xmm2, qword [rax]		; c5 eb c2 08 13
vcmpneq_ussd xmm1, xmm2, qword [rax]		; c5 eb c2 08 14
vcmpnlt_uqsd xmm1, xmm2, qword [rax]		; c5 eb c2 08 15
vcmpnle_uqsd xmm1, xmm2, qword [rax]		; c5 eb c2 08 16
vcmpord_ssd xmm1, xmm2, qword [rax]		; c5 eb c2 08 17

vcmpeq_ussd xmm1, xmm2, qword [rax]		; c5 eb c2 08 18
vcmpnge_uqsd xmm1, xmm2, qword [rax]		; c5 eb c2 08 19
vcmpngt_uqsd xmm1, xmm2, qword [rax]		; c5 eb c2 08 1a
vcmpfalse_ossd xmm1, xmm2, qword [rax]		; c5 eb c2 08 1b
vcmpneq_ossd xmm1, xmm2, qword [rax]		; c5 eb c2 08 1c
vcmpge_oqsd xmm1, xmm2, qword [rax]		; c5 eb c2 08 1d
vcmpgt_oqsd xmm1, xmm2, qword [rax]		; c5 eb c2 08 1e
vcmptrue_ussd xmm1, xmm2, qword [rax]		; c5 eb c2 08 1f

;-----------------------------------------------------------------------------

cmpeqss xmm1, xmm2			; f3 0f c2 ca 00
cmpltss xmm1, xmm2			; f3 0f c2 ca 01
cmpless xmm1, xmm2			; f3 0f c2 ca 02
cmpunordss xmm1, xmm2			; f3 0f c2 ca 03
cmpneqss xmm1, xmm2			; f3 0f c2 ca 04
cmpnltss xmm1, xmm2			; f3 0f c2 ca 05
cmpnless xmm1, xmm2			; f3 0f c2 ca 06
cmpordss xmm1, xmm2			; f3 0f c2 ca 07

vcmpeqss xmm1, xmm2			; c5 f2 c2 ca 00
vcmpltss xmm1, xmm2			; c5 f2 c2 ca 01
vcmpless xmm1, xmm2			; c5 f2 c2 ca 02
vcmpunordss xmm1, xmm2			; c5 f2 c2 ca 03
vcmpneqss xmm1, xmm2			; c5 f2 c2 ca 04
vcmpnltss xmm1, xmm2			; c5 f2 c2 ca 05
vcmpnless xmm1, xmm2			; c5 f2 c2 ca 06
vcmpordss xmm1, xmm2			; c5 f2 c2 ca 07

vcmpeqss xmm1, xmm2, xmm3		; c5 ea c2 cb 00
vcmpltss xmm1, xmm2, xmm3		; c5 ea c2 cb 01
vcmpless xmm1, xmm2, xmm3		; c5 ea c2 cb 02
vcmpunordss xmm1, xmm2, xmm3		; c5 ea c2 cb 03
vcmpneqss xmm1, xmm2, xmm3		; c5 ea c2 cb 04
vcmpnltss xmm1, xmm2, xmm3		; c5 ea c2 cb 05
vcmpnless xmm1, xmm2, xmm3		; c5 ea c2 cb 06
vcmpordss xmm1, xmm2, xmm3		; c5 ea c2 cb 07

vcmpeq_uqss xmm1, xmm2, xmm3		; c5 ea c2 cb 08
vcmpngess xmm1, xmm2, xmm3		; c5 ea c2 cb 09
vcmpngtss xmm1, xmm2, xmm3		; c5 ea c2 cb 0a
vcmpfalsess xmm1, xmm2, xmm3		; c5 ea c2 cb 0b
vcmpneq_oqss xmm1, xmm2, xmm3		; c5 ea c2 cb 0c
vcmpgess xmm1, xmm2, xmm3		; c5 ea c2 cb 0d
vcmpgtss xmm1, xmm2, xmm3		; c5 ea c2 cb 0e
vcmptruess xmm1, xmm2, xmm3		; c5 ea c2 cb 0f

vcmpeq_osss xmm1, xmm2, xmm3		; c5 ea c2 cb 10
vcmplt_oqss xmm1, xmm2, xmm3		; c5 ea c2 cb 11
vcmple_oqss xmm1, xmm2, xmm3		; c5 ea c2 cb 12
vcmpunord_sss xmm1, xmm2, xmm3		; c5 ea c2 cb 13
vcmpneq_usss xmm1, xmm2, xmm3		; c5 ea c2 cb 14
vcmpnlt_uqss xmm1, xmm2, xmm3		; c5 ea c2 cb 15
vcmpnle_uqss xmm1, xmm2, xmm3		; c5 ea c2 cb 16
vcmpord_sss xmm1, xmm2, xmm3		; c5 ea c2 cb 17

vcmpeq_usss xmm1, xmm2, xmm3		; c5 ea c2 cb 18
vcmpnge_uqss xmm1, xmm2, xmm3		; c5 ea c2 cb 19
vcmpngt_uqss xmm1, xmm2, xmm3		; c5 ea c2 cb 1a
vcmpfalse_osss xmm1, xmm2, xmm3		; c5 ea c2 cb 1b
vcmpneq_osss xmm1, xmm2, xmm3		; c5 ea c2 cb 1c
vcmpge_oqss xmm1, xmm2, xmm3		; c5 ea c2 cb 1d
vcmpgt_oqss xmm1, xmm2, xmm3		; c5 ea c2 cb 1e
vcmptrue_usss xmm1, xmm2, xmm3		; c5 ea c2 cb 1f

cmpeqss xmm1, [rax]			; f3 0f c2 08 00
cmpltss xmm1, [rax]			; f3 0f c2 08 01
cmpless xmm1, [rax]			; f3 0f c2 08 02
cmpunordss xmm1, [rax]			; f3 0f c2 08 03
cmpneqss xmm1, [rax]			; f3 0f c2 08 04
cmpnltss xmm1, [rax]			; f3 0f c2 08 05
cmpnless xmm1, [rax]			; f3 0f c2 08 06
cmpordss xmm1, [rax]			; f3 0f c2 08 07

vcmpeqss xmm1, [rax]			; c5 f2 c2 08 00
vcmpltss xmm1, [rax]			; c5 f2 c2 08 01
vcmpless xmm1, [rax]			; c5 f2 c2 08 02
vcmpunordss xmm1, [rax]			; c5 f2 c2 08 03
vcmpneqss xmm1, [rax]			; c5 f2 c2 08 04
vcmpnltss xmm1, [rax]			; c5 f2 c2 08 05
vcmpnless xmm1, [rax]			; c5 f2 c2 08 06
vcmpordss xmm1, [rax]			; c5 f2 c2 08 07

vcmpeqss xmm1, xmm2, [rax]		; c5 ea c2 08 00
vcmpltss xmm1, xmm2, [rax]		; c5 ea c2 08 01
vcmpless xmm1, xmm2, [rax]		; c5 ea c2 08 02
vcmpunordss xmm1, xmm2, [rax]		; c5 ea c2 08 03
vcmpneqss xmm1, xmm2, [rax]		; c5 ea c2 08 04
vcmpnltss xmm1, xmm2, [rax]		; c5 ea c2 08 05
vcmpnless xmm1, xmm2, [rax]		; c5 ea c2 08 06
vcmpordss xmm1, xmm2, [rax]		; c5 ea c2 08 07

vcmpeq_uqss xmm1, xmm2, [rax]		; c5 ea c2 08 08
vcmpngess xmm1, xmm2, [rax]		; c5 ea c2 08 09
vcmpngtss xmm1, xmm2, [rax]		; c5 ea c2 08 0a
vcmpfalsess xmm1, xmm2, [rax]		; c5 ea c2 08 0b
vcmpneq_oqss xmm1, xmm2, [rax]		; c5 ea c2 08 0c
vcmpgess xmm1, xmm2, [rax]		; c5 ea c2 08 0d
vcmpgtss xmm1, xmm2, [rax]		; c5 ea c2 08 0e
vcmptruess xmm1, xmm2, [rax]		; c5 ea c2 08 0f

vcmpeq_osss xmm1, xmm2, [rax]		; c5 ea c2 08 10
vcmplt_oqss xmm1, xmm2, [rax]		; c5 ea c2 08 11
vcmple_oqss xmm1, xmm2, [rax]		; c5 ea c2 08 12
vcmpunord_sss xmm1, xmm2, [rax]		; c5 ea c2 08 13
vcmpneq_usss xmm1, xmm2, [rax]		; c5 ea c2 08 14
vcmpnlt_uqss xmm1, xmm2, [rax]		; c5 ea c2 08 15
vcmpnle_uqss xmm1, xmm2, [rax]		; c5 ea c2 08 16
vcmpord_sss xmm1, xmm2, [rax]		; c5 ea c2 08 17

vcmpeq_usss xmm1, xmm2, [rax]		; c5 ea c2 08 18
vcmpnge_uqss xmm1, xmm2, [rax]		; c5 ea c2 08 19
vcmpngt_uqss xmm1, xmm2, [rax]		; c5 ea c2 08 1a
vcmpfalse_osss xmm1, xmm2, [rax]	; c5 ea c2 08 1b
vcmpneq_osss xmm1, xmm2, [rax]		; c5 ea c2 08 1c
vcmpge_oqss xmm1, xmm2, [rax]		; c5 ea c2 08 1d
vcmpgt_oqss xmm1, xmm2, [rax]		; c5 ea c2 08 1e
vcmptrue_usss xmm1, xmm2, [rax]		; c5 ea c2 08 1f

cmpeqss xmm1, dword [rax]			; f3 0f c2 08 00
cmpltss xmm1, dword [rax]			; f3 0f c2 08 01
cmpless xmm1, dword [rax]			; f3 0f c2 08 02
cmpunordss xmm1, dword [rax]			; f3 0f c2 08 03
cmpneqss xmm1, dword [rax]			; f3 0f c2 08 04
cmpnltss xmm1, dword [rax]			; f3 0f c2 08 05
cmpnless xmm1, dword [rax]			; f3 0f c2 08 06
cmpordss xmm1, dword [rax]			; f3 0f c2 08 07

vcmpeqss xmm1, dword [rax]			; c5 f2 c2 08 00
vcmpltss xmm1, dword [rax]			; c5 f2 c2 08 01
vcmpless xmm1, dword [rax]			; c5 f2 c2 08 02
vcmpunordss xmm1, dword [rax]			; c5 f2 c2 08 03
vcmpneqss xmm1, dword [rax]			; c5 f2 c2 08 04
vcmpnltss xmm1, dword [rax]			; c5 f2 c2 08 05
vcmpnless xmm1, dword [rax]			; c5 f2 c2 08 06
vcmpordss xmm1, dword [rax]			; c5 f2 c2 08 07

vcmpeqss xmm1, xmm2, dword [rax]		; c5 ea c2 08 00
vcmpltss xmm1, xmm2, dword [rax]		; c5 ea c2 08 01
vcmpless xmm1, xmm2, dword [rax]		; c5 ea c2 08 02
vcmpunordss xmm1, xmm2, dword [rax]		; c5 ea c2 08 03
vcmpneqss xmm1, xmm2, dword [rax]		; c5 ea c2 08 04
vcmpnltss xmm1, xmm2, dword [rax]		; c5 ea c2 08 05
vcmpnless xmm1, xmm2, dword [rax]		; c5 ea c2 08 06
vcmpordss xmm1, xmm2, dword [rax]		; c5 ea c2 08 07

vcmpeq_uqss xmm1, xmm2, dword [rax]		; c5 ea c2 08 08
vcmpngess xmm1, xmm2, dword [rax]		; c5 ea c2 08 09
vcmpngtss xmm1, xmm2, dword [rax]		; c5 ea c2 08 0a
vcmpfalsess xmm1, xmm2, dword [rax]		; c5 ea c2 08 0b
vcmpneq_oqss xmm1, xmm2, dword [rax]		; c5 ea c2 08 0c
vcmpgess xmm1, xmm2, dword [rax]		; c5 ea c2 08 0d
vcmpgtss xmm1, xmm2, dword [rax]		; c5 ea c2 08 0e
vcmptruess xmm1, xmm2, dword [rax]		; c5 ea c2 08 0f

vcmpeq_osss xmm1, xmm2, dword [rax]		; c5 ea c2 08 10
vcmplt_oqss xmm1, xmm2, dword [rax]		; c5 ea c2 08 11
vcmple_oqss xmm1, xmm2, dword [rax]		; c5 ea c2 08 12
vcmpunord_sss xmm1, xmm2, dword [rax]		; c5 ea c2 08 13
vcmpneq_usss xmm1, xmm2, dword [rax]		; c5 ea c2 08 14
vcmpnlt_uqss xmm1, xmm2, dword [rax]		; c5 ea c2 08 15
vcmpnle_uqss xmm1, xmm2, dword [rax]		; c5 ea c2 08 16
vcmpord_sss xmm1, xmm2, dword [rax]		; c5 ea c2 08 17

vcmpeq_usss xmm1, xmm2, dword [rax]		; c5 ea c2 08 18
vcmpnge_uqss xmm1, xmm2, dword [rax]		; c5 ea c2 08 19
vcmpngt_uqss xmm1, xmm2, dword [rax]		; c5 ea c2 08 1a
vcmpfalse_osss xmm1, xmm2, dword [rax]		; c5 ea c2 08 1b
vcmpneq_osss xmm1, xmm2, dword [rax]		; c5 ea c2 08 1c
vcmpge_oqss xmm1, xmm2, dword [rax]		; c5 ea c2 08 1d
vcmpgt_oqss xmm1, xmm2, dword [rax]		; c5 ea c2 08 1e
vcmptrue_usss xmm1, xmm2, dword [rax]		; c5 ea c2 08 1f

