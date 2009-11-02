[bits 64]
movbe cx, [5] 		; 66 0f 38 f0 0c 25 05 00 00 00
movbe cx, word [5] 	; 66 0f 38 f0 0c 25 05 00 00 00
movbe ecx, [5] 		; 0f 38 f0 0c 25 05 00 00 00
movbe ecx, dword [5] 	; 0f 38 f0 0c 25 05 00 00 00
movbe rcx, [5] 		; 48 0f 38 f0 0c 25 05 00 00 00
movbe rcx, qword [5] 	; 48 0f 38 f0 0c 25 05 00 00 00
movbe r9, [5] 		; 4c 0f 38 f0 0c 25 05 00 00 00
movbe r9, qword [5] 	; 4c 0f 38 f0 0c 25 05 00 00 00
movbe [5], bx 		; 66 0f 38 f1 1c 25 05 00 00 00
movbe word [5], bx 	; 66 0f 38 f1 1c 25 05 00 00 00
movbe [5], ebx 		; 0f 38 f1 1c 25 05 00 00 00
movbe dword [5], ebx 	; 0f 38 f1 1c 25 05 00 00 00
movbe [5], r10d 	; 44 0f 38 f1 14 25 05 00 00 00
movbe dword [5], r10d 	; 44 0f 38 f1 14 25 05 00 00 00
movbe [5], rbx 		; 48 0f 38 f1 1c 25 05 00 00 00
movbe qword [5], rbx 	; 48 0f 38 f1 1c 25 05 00 00 00
movbe [5], r10 		; 4c 0f 38 f1 14 25 05 00 00 00
movbe qword [5], r10 	; 4c 0f 38 f1 14 25 05 00 00 00

