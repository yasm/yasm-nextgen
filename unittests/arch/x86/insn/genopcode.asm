[bits 16]
mov al, 0 		; b0 00
mov byte al, 0 		; b0 00
mov al, byte 0 		; b0 00
mov byte al, byte 0 	; b0 00
;mov al, word 0
mov byte [0], 0 	; c6 06 00 00 00
mov [0], word 0 	; c7 06 00 00 00 00
mov dword [0], dword 0 	; 66 c7 06 00 00 00 00 00 00
;mov [0], 0
mov eax, 0 		; 66 b8 00 00 00 00
mov dword eax, 0 	; 66 b8 00 00 00 00
mov eax, dword 0 	; 66 b8 00 00 00 00
;mov eax, word 0
mov dword eax, dword 0 	; 66 b8 00 00 00 00
mov bx, 1h 		; bb 01 00
mov cr0, eax 		; 0f 22 c0
mov cr2, ebx 		; 0f 22 d3
mov cr4, edx 		; 0f 22 e2
mov ecx, cr4 		; 0f 20 e1
mov dr3, edx 		; 0f 23 da
mov eax, dr7 		; 0f 21 f8

mov [0], al 		; a2 00 00
mov [0], bl 		; 88 1e 00 00
mov [1], al 		; a2 01 00
mov [1], bl 		; 88 1e 01 00
mov ecx, edx 		; 66 89 d1
movsx ax, [ecx] 	; 67 0f be 01
;movzx eax, [edx]
movzx ebx, word [eax] 	; 67 66 0f b7 18
movzx ecx, byte [ebx] 	; 67 66 0f b6 0b
fnstenv [ecx+5] 	; 67 d9 71 05
nop 			; 90

push cs 		; 0e
push word cs 		; 0e
push dword cs		; 66 0e - NASM unsupported
push ds 		; 1e
push es 		; 06
push fs 		; 0f a0
push gs 		; 0f a8
pop ds 			; 1f
pop es 			; 07
pop fs 			; 0f a1
pop gs 			; 0f a9
xchg al, bl 		; 86 d8
xchg al, [0] 		; 86 06 00 00
xchg [0], al 		; 86 06 00 00
xchg ax, bx 		; 93
xchg cx, ax 		; 91
xchg [0], ax 		; 87 06 00 00
xchg [0], cx 		; 87 0e 00 00
xchg cx, [0] 		; 87 0e 00 00
xchg eax, edx 		; 66 92
xchg ebx, eax 		; 66 93
xchg ecx, ebx 		; 66 87 d9
xchg [0], ecx 		; 66 87 0e 00 00
xchg eax, [0] 		; 66 87 06 00 00
in al, 55 		; e4 37
in ax, 99 		; e5 63
in eax, 100 		; 66 e5 64
in al, dx 		; ec
in ax, dx 		; ed
in eax, dx 		; 66 ed
out 55, al 		; e6 37
out 66, ax 		; e7 42
out 77, eax 		; 66 e7 4d
out dx, al 		; ee
out dx, ax 		; ef
out dx, eax 		; 66 ef
lea bx, [5] 		; 8d 1e 05 00
lea ebx, [32] 		; 66 8d 1e 20 00
lds si, [0] 		; c5 36 00 00
lds ax, [1] 		; c5 06 01 00
;lds ax, dword [1]
les di, [5] 		; c4 3e 05 00
lds eax, [7] 		; 66 c5 06 07 00
les ebx, [9] 		; 66 c4 1e 09 00
lss esp, [11] 		; 66 0f b2 26 0b 00
lfs ecx, [13] 		; 66 0f b4 0e 0d 00
lgs edx, [15] 		; 66 0f b5 16 0f 00
;; TODO: add arith stuff
imul eax, 4 		; 66 6b c0 04
aad 			; d5 0a
aam 			; d4 0a
aad 5 			; d5 05
aam 10 			; d4 0a
shl al, 5 		; c0 e0 05
shl bl, 1 		; d0 e3
shl cl, cl 		; d2 e1
shr ax, 5 		; c1 e8 05
shr bx, 1 		; d1 eb
shr cx, cl 		; d3 e9
shld ax, bx, 5 		; 0f a4 d8 05
shrd cx, dx, cl 	; 0f ad d1
shld ecx, edx, 10 	; 66 0f a4 d1 0a
shld eax, ebx, cl 	; 66 0f a5 d8
retn 			; c3
retf 			; cb
retn 8 			; c2 08 00
retf 16 		; ca 10 00
enter 10, 12 		; c8 0a 00 0c
setc al 		; 0f 92 d0
setc [0] 		; 0f 92 16 00 00
;; TODO: add bit manip
int 10 			; cd 0a
;; TODO: add bound
;; TODO: add protection control
fld dword [0] 		; d9 06 00 00
fld qword [4] 		; dd 06 04 00
fld tword [16] 		; db 2e 10 00
fld st2 		; d9 c2
fstp dword [0] 		; d9 1e 00 00
fstp st4 		; dd dc
fild word [0] 		; df 06 00 00
fild dword [4] 		; db 06 04 00
fild qword [8] 		; df 2e 08 00
fbld [100] 		; df 26 64 00
fbld tword [10] 	; df 26 0a 00
fst dword [1] 		; d9 16 01 00
fst qword [8] 		; dd 16 08 00
fst st1 		; dd d1
fxch 			; d9 c9
fxch st1 		; d9 c9
fxch st0, st2 		; d9 ca
fxch st2, st0 		; d9 ca
fcom dword [0] 		; d8 16 00 00
fcom qword [8] 		; dc 16 08 00
fcom st1 		; d8 d1
fcom st0, st0 		; d8 d0
fucom st7 		; dd e7
fucomp st0, st5 	; dd ed
fadd dword [10] 	; d8 06 0a 00
fadd qword [5] 		; dc 06 05 00
fadd st0 		; d8 c0
fadd st0, st5 		; d8 c5
;fadd to st7 		; dc c7
fadd st6, st0 		; dc c6
faddp			; de c1 - NASM unsupported
faddp st2 		; de c2
faddp st5, st0 		; de c5
fiadd word [10] 	; de 06 0a 00
fisub dword [4] 	; da 26 04 00
fldcw [0] 		; d9 2e 00 00
fnstcw [4] 		; d9 3e 04 00
fstcw word [4] 		; 9b d9 3e 04 00
fnstsw [8] 		; dd 3e 08 00
fnstsw ax 		; df e0
fstsw word [0] 		; 9b dd 3e 00 00
fstsw ax 		; 9b df e0
ffree st1 		; dd c1
ffreep st0		; df c0 - NASM unsupported
[bits 16]
push si			; 56
push esi		; 66 56
[bits 32]
push esi		; 56
