[bits 16]
pushf 	; 9c
pushfw 	; 9c
pushfd 	; 66 9c
;pushfq	; [warning: `pushfq' is an instruction in 64-bit mode]
popf 	; 9d
popfw 	; 9d
popfd 	; 66 9d
;popfq	; [warning: `popfq' is an instruction in 64-bit mode]

[bits 32]
pushf 	; 9c
pushfw 	; 66 9c
pushfd 	; 9c
;pushfq	; [warning: `pushfq' is an instruction in 64-bit mode]
popf 	; 9d
popfw 	; 66 9d
popfd 	; 9d
;popfq	; [warning: `popfq' is an instruction in 64-bit mode]

[bits 64]
pushf 	; 9c
pushfw 	; 66 9c
;pushfd	; [error: `pushfd' invalid in 64-bit mode]
pushfq 	; 9c
popf 	; 9d
popfw 	; 66 9d
;popfd	; [error: `popfd' invalid in 64-bit mode]
popfq 	; 9d

