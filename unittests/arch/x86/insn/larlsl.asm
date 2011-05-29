[bits 16]
lar ax, bx		; 0f 02 c3
lar ax, [bx]		; 0f 02 07
lar ax, word [bx]	; 0f 02 07
lar eax, bx		; 66 0f 02 c3
lar eax, ebx		; 66 0f 02 c3
lar eax, [bx]		; 66 0f 02 07
lar eax, word [bx]	; 66 0f 02 07

lsl ax, bx		; 0f 03 c3
lsl ax, [bx]		; 0f 03 07
lsl ax, word [bx]	; 0f 03 07
lsl eax, bx		; 66 0f 03 c3
lsl eax, ebx		; 66 0f 03 c3
lsl eax, [bx]		; 66 0f 03 07
lsl eax, word [bx]	; 66 0f 03 07

[bits 32]
lar ax, bx		; 66 0f 02 c3
lar ax, [ebx]		; 66 0f 02 03
lar ax, word [ebx]	; 66 0f 02 03
lar eax, bx		; 0f 02 c3
lar eax, ebx		; 0f 02 c3
lar eax, [ebx]		; 0f 02 03
lar eax, word [ebx]	; 0f 02 03

lsl ax, bx		; 66 0f 03 c3
lsl ax, [ebx]		; 66 0f 03 03
lsl ax, word [ebx]	; 66 0f 03 03
lsl eax, bx		; 0f 03 c3
lsl eax, ebx		; 0f 03 c3
lsl eax, [ebx]		; 0f 03 03
lsl eax, word [ebx]	; 0f 03 03

[bits 64]
lar ax, bx		; 66 0f 02 c3
lar ax, [rbx]		; 66 0f 02 03
lar ax, word [rbx]	; 66 0f 02 03
lar eax, bx		; 0f 02 c3
lar eax, ebx		; 0f 02 c3
lar eax, [rbx]		; 0f 02 03
lar eax, word [rbx]	; 0f 02 03
lar rax, bx		; 48 0f 02 c3
lar rax, ebx		; 48 0f 02 c3
lar rax, [rbx]		; 48 0f 02 03
lar rax, word [rbx]	; 48 0f 02 03

lsl ax, bx		; 66 0f 03 c3
lsl ax, [rbx]		; 66 0f 03 03
lsl ax, word [rbx]	; 66 0f 03 03
lsl eax, bx		; 0f 03 c3
lsl eax, ebx		; 0f 03 c3
lsl eax, [rbx]		; 0f 03 03
lsl eax, word [rbx]	; 0f 03 03
lsl rax, bx		; 48 0f 03 c3
lsl rax, ebx		; 48 0f 03 c3
lsl rax, [rbx]		; 48 0f 03 03
lsl rax, word [rbx]	; 48 0f 03 03

