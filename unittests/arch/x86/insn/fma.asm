[bits 64]
vfmadd132ss xmm1, xmm2, xmm3 		; c4 e2 69 99 cb
vfmadd132ss xmm1, xmm2, dword [rax] 	; c4 e2 69 99 08
vfmadd132ss xmm1, xmm2, [rax] 		; c4 e2 69 99 08
vfmadd231ss xmm1, xmm2, xmm3 		; c4 e2 69 b9 cb
vfmadd231ss xmm1, xmm2, dword [rax] 	; c4 e2 69 b9 08
vfmadd231ss xmm1, xmm2, [rax] 		; c4 e2 69 b9 08
vfmadd213ss xmm1, xmm2, xmm3 		; c4 e2 69 a9 cb
vfmadd213ss xmm1, xmm2, dword [rax] 	; c4 e2 69 a9 08
vfmadd213ss xmm1, xmm2, [rax] 		; c4 e2 69 a9 08
vfmadd132sd xmm1, xmm2, xmm3 		; c4 e2 e9 99 cb
vfmadd132sd xmm1, xmm2, qword [rax] 	; c4 e2 e9 99 08
vfmadd132sd xmm1, xmm2, [rax] 		; c4 e2 e9 99 08
vfmadd231sd xmm1, xmm2, xmm3 		; c4 e2 e9 b9 cb
vfmadd231sd xmm1, xmm2, qword [rax] 	; c4 e2 e9 b9 08
vfmadd231sd xmm1, xmm2, [rax] 		; c4 e2 e9 b9 08
vfmadd213sd xmm1, xmm2, xmm3 		; c4 e2 e9 a9 cb
vfmadd213sd xmm1, xmm2, qword [rax] 	; c4 e2 e9 a9 08
vfmadd213sd xmm1, xmm2, [rax] 		; c4 e2 e9 a9 08
vfmadd132ps xmm1, xmm2, xmm3 		; c4 e2 69 98 cb
vfmadd132ps xmm1, xmm2, xmm3 		; c4 e2 69 98 cb
vfmadd132ps xmm1, xmm2, [rax] 		; c4 e2 69 98 08
vfmadd231ps xmm1, xmm2, xmm3 		; c4 e2 69 b8 cb
vfmadd231ps xmm1, xmm2, xmm3 		; c4 e2 69 b8 cb
vfmadd231ps xmm1, xmm2, [rax] 		; c4 e2 69 b8 08
vfmadd213ps xmm1, xmm2, xmm3 		; c4 e2 69 a8 cb
vfmadd213ps xmm1, xmm2, xmm3 		; c4 e2 69 a8 cb
vfmadd213ps xmm1, xmm2, [rax] 		; c4 e2 69 a8 08
vfmadd132ps ymm1, ymm2, ymm3 		; c4 e2 6d 98 cb
vfmadd132ps ymm1, ymm2, yword [rax] 	; c4 e2 6d 98 08
vfmadd132ps ymm1, ymm2, [rax] 		; c4 e2 6d 98 08
vfmadd231ps ymm1, ymm2, ymm3 		; c4 e2 6d b8 cb
vfmadd231ps ymm1, ymm2, yword [rax] 	; c4 e2 6d b8 08
vfmadd231ps ymm1, ymm2, [rax] 		; c4 e2 6d b8 08
vfmadd213ps ymm1, ymm2, ymm3 		; c4 e2 6d a8 cb
vfmadd213ps ymm1, ymm2, yword [rax] 	; c4 e2 6d a8 08
vfmadd213ps ymm1, ymm2, [rax] 		; c4 e2 6d a8 08
vfmadd132pd xmm1, xmm2, xmm3 		; c4 e2 e9 98 cb
vfmadd132pd xmm1, xmm2, dqword [rax] 	; c4 e2 e9 98 08
vfmadd132pd xmm1, xmm2, [rax] 		; c4 e2 e9 98 08
vfmadd231pd xmm1, xmm2, xmm3 		; c4 e2 e9 b8 cb
vfmadd231pd xmm1, xmm2, dqword [rax] 	; c4 e2 e9 b8 08
vfmadd231pd xmm1, xmm2, [rax] 		; c4 e2 e9 b8 08
vfmadd213pd xmm1, xmm2, xmm3 		; c4 e2 e9 a8 cb
vfmadd213pd xmm1, xmm2, dqword [rax] 	; c4 e2 e9 a8 08
vfmadd213pd xmm1, xmm2, [rax] 		; c4 e2 e9 a8 08
vfmadd132pd ymm1, ymm2, ymm3 		; c4 e2 ed 98 cb
vfmadd132pd ymm1, ymm2, yword [rax] 	; c4 e2 ed 98 08
vfmadd132pd ymm1, ymm2, [rax] 		; c4 e2 ed 98 08
vfmadd231pd ymm1, ymm2, ymm3 		; c4 e2 ed b8 cb
vfmadd231pd ymm1, ymm2, yword [rax] 	; c4 e2 ed b8 08
vfmadd231pd ymm1, ymm2, [rax] 		; c4 e2 ed b8 08
vfmadd213pd ymm1, ymm2, ymm3 		; c4 e2 ed a8 cb
vfmadd213pd ymm1, ymm2, yword [rax] 	; c4 e2 ed a8 08
vfmadd213pd ymm1, ymm2, [rax] 		; c4 e2 ed a8 08
vfmsub132ss xmm1, xmm2, xmm3 		; c4 e2 69 9b cb
vfmsub132ss xmm1, xmm2, dword [rax] 	; c4 e2 69 9b 08
vfmsub132ss xmm1, xmm2, [rax] 		; c4 e2 69 9b 08
vfmsub231ss xmm1, xmm2, xmm3 		; c4 e2 69 bb cb
vfmsub231ss xmm1, xmm2, dword [rax] 	; c4 e2 69 bb 08
vfmsub231ss xmm1, xmm2, [rax] 		; c4 e2 69 bb 08
vfmsub213ss xmm1, xmm2, xmm3 		; c4 e2 69 ab cb
vfmsub213ss xmm1, xmm2, dword [rax] 	; c4 e2 69 ab 08
vfmsub213ss xmm1, xmm2, [rax] 		; c4 e2 69 ab 08
vfmsub132sd xmm1, xmm2, xmm3 		; c4 e2 e9 9b cb
vfmsub132sd xmm1, xmm2, qword [rax] 	; c4 e2 e9 9b 08
vfmsub132sd xmm1, xmm2, [rax] 		; c4 e2 e9 9b 08
vfmsub231sd xmm1, xmm2, xmm3 		; c4 e2 e9 bb cb
vfmsub231sd xmm1, xmm2, qword [rax] 	; c4 e2 e9 bb 08
vfmsub231sd xmm1, xmm2, [rax] 		; c4 e2 e9 bb 08
vfmsub213sd xmm1, xmm2, xmm3 		; c4 e2 e9 ab cb
vfmsub213sd xmm1, xmm2, qword [rax] 	; c4 e2 e9 ab 08
vfmsub213sd xmm1, xmm2, [rax] 		; c4 e2 e9 ab 08
vfmsub132ps xmm1, xmm2, xmm3 		; c4 e2 69 9a cb
vfmsub132ps xmm1, xmm2, xmm3 		; c4 e2 69 9a cb
vfmsub132ps xmm1, xmm2, [rax] 		; c4 e2 69 9a 08
vfmsub231ps xmm1, xmm2, xmm3 		; c4 e2 69 ba cb
vfmsub231ps xmm1, xmm2, xmm3 		; c4 e2 69 ba cb
vfmsub231ps xmm1, xmm2, [rax] 		; c4 e2 69 ba 08
vfmsub213ps xmm1, xmm2, xmm3 		; c4 e2 69 aa cb
vfmsub213ps xmm1, xmm2, xmm3 		; c4 e2 69 aa cb
vfmsub213ps xmm1, xmm2, [rax] 		; c4 e2 69 aa 08
vfmsub132ps ymm1, ymm2, ymm3 		; c4 e2 6d 9a cb
vfmsub132ps ymm1, ymm2, yword [rax] 	; c4 e2 6d 9a 08
vfmsub132ps ymm1, ymm2, [rax] 		; c4 e2 6d 9a 08
vfmsub231ps ymm1, ymm2, ymm3 		; c4 e2 6d ba cb
vfmsub231ps ymm1, ymm2, yword [rax] 	; c4 e2 6d ba 08
vfmsub231ps ymm1, ymm2, [rax] 		; c4 e2 6d ba 08
vfmsub213ps ymm1, ymm2, ymm3 		; c4 e2 6d aa cb
vfmsub213ps ymm1, ymm2, yword [rax] 	; c4 e2 6d aa 08
vfmsub213ps ymm1, ymm2, [rax] 		; c4 e2 6d aa 08
vfmsub132pd xmm1, xmm2, xmm3 		; c4 e2 e9 9a cb
vfmsub132pd xmm1, xmm2, dqword [rax] 	; c4 e2 e9 9a 08
vfmsub132pd xmm1, xmm2, [rax] 		; c4 e2 e9 9a 08
vfmsub231pd xmm1, xmm2, xmm3 		; c4 e2 e9 ba cb
vfmsub231pd xmm1, xmm2, dqword [rax] 	; c4 e2 e9 ba 08
vfmsub231pd xmm1, xmm2, [rax] 		; c4 e2 e9 ba 08
vfmsub213pd xmm1, xmm2, xmm3 		; c4 e2 e9 aa cb
vfmsub213pd xmm1, xmm2, dqword [rax] 	; c4 e2 e9 aa 08
vfmsub213pd xmm1, xmm2, [rax] 		; c4 e2 e9 aa 08
vfmsub132pd ymm1, ymm2, ymm3 		; c4 e2 ed 9a cb
vfmsub132pd ymm1, ymm2, yword [rax] 	; c4 e2 ed 9a 08
vfmsub132pd ymm1, ymm2, [rax] 		; c4 e2 ed 9a 08
vfmsub231pd ymm1, ymm2, ymm3 		; c4 e2 ed ba cb
vfmsub231pd ymm1, ymm2, yword [rax] 	; c4 e2 ed ba 08
vfmsub231pd ymm1, ymm2, [rax] 		; c4 e2 ed ba 08
vfmsub213pd ymm1, ymm2, ymm3 		; c4 e2 ed aa cb
vfmsub213pd ymm1, ymm2, yword [rax] 	; c4 e2 ed aa 08
vfmsub213pd ymm1, ymm2, [rax] 		; c4 e2 ed aa 08
vfnmadd132ss xmm1, xmm2, xmm3 		; c4 e2 69 9d cb
vfnmadd132ss xmm1, xmm2, dword [rax] 	; c4 e2 69 9d 08
vfnmadd132ss xmm1, xmm2, [rax] 		; c4 e2 69 9d 08
vfnmadd231ss xmm1, xmm2, xmm3 		; c4 e2 69 bd cb
vfnmadd231ss xmm1, xmm2, dword [rax] 	; c4 e2 69 bd 08
vfnmadd231ss xmm1, xmm2, [rax] 		; c4 e2 69 bd 08
vfnmadd213ss xmm1, xmm2, xmm3 		; c4 e2 69 ad cb
vfnmadd213ss xmm1, xmm2, dword [rax] 	; c4 e2 69 ad 08
vfnmadd213ss xmm1, xmm2, [rax] 		; c4 e2 69 ad 08
vfnmadd132sd xmm1, xmm2, xmm3 		; c4 e2 e9 9d cb
vfnmadd132sd xmm1, xmm2, qword [rax] 	; c4 e2 e9 9d 08
vfnmadd132sd xmm1, xmm2, [rax] 		; c4 e2 e9 9d 08
vfnmadd231sd xmm1, xmm2, xmm3 		; c4 e2 e9 bd cb
vfnmadd231sd xmm1, xmm2, qword [rax] 	; c4 e2 e9 bd 08
vfnmadd231sd xmm1, xmm2, [rax] 		; c4 e2 e9 bd 08
vfnmadd213sd xmm1, xmm2, xmm3 		; c4 e2 e9 ad cb
vfnmadd213sd xmm1, xmm2, qword [rax] 	; c4 e2 e9 ad 08
vfnmadd213sd xmm1, xmm2, [rax] 		; c4 e2 e9 ad 08
vfnmadd132ps xmm1, xmm2, xmm3 		; c4 e2 69 9c cb
vfnmadd132ps xmm1, xmm2, xmm3 		; c4 e2 69 9c cb
vfnmadd132ps xmm1, xmm2, [rax] 		; c4 e2 69 9c 08
vfnmadd231ps xmm1, xmm2, xmm3 		; c4 e2 69 bc cb
vfnmadd231ps xmm1, xmm2, xmm3 		; c4 e2 69 bc cb
vfnmadd231ps xmm1, xmm2, [rax] 		; c4 e2 69 bc 08
vfnmadd213ps xmm1, xmm2, xmm3 		; c4 e2 69 ac cb
vfnmadd213ps xmm1, xmm2, xmm3 		; c4 e2 69 ac cb
vfnmadd213ps xmm1, xmm2, [rax] 		; c4 e2 69 ac 08
vfnmadd132ps ymm1, ymm2, ymm3 		; c4 e2 6d 9c cb
vfnmadd132ps ymm1, ymm2, yword [rax] 	; c4 e2 6d 9c 08
vfnmadd132ps ymm1, ymm2, [rax] 		; c4 e2 6d 9c 08
vfnmadd231ps ymm1, ymm2, ymm3 		; c4 e2 6d bc cb
vfnmadd231ps ymm1, ymm2, yword [rax] 	; c4 e2 6d bc 08
vfnmadd231ps ymm1, ymm2, [rax] 		; c4 e2 6d bc 08
vfnmadd213ps ymm1, ymm2, ymm3 		; c4 e2 6d ac cb
vfnmadd213ps ymm1, ymm2, yword [rax] 	; c4 e2 6d ac 08
vfnmadd213ps ymm1, ymm2, [rax] 		; c4 e2 6d ac 08
vfnmadd132pd xmm1, xmm2, xmm3 		; c4 e2 e9 9c cb
vfnmadd132pd xmm1, xmm2, dqword [rax] 	; c4 e2 e9 9c 08
vfnmadd132pd xmm1, xmm2, [rax] 		; c4 e2 e9 9c 08
vfnmadd231pd xmm1, xmm2, xmm3 		; c4 e2 e9 bc cb
vfnmadd231pd xmm1, xmm2, dqword [rax] 	; c4 e2 e9 bc 08
vfnmadd231pd xmm1, xmm2, [rax] 		; c4 e2 e9 bc 08
vfnmadd213pd xmm1, xmm2, xmm3 		; c4 e2 e9 ac cb
vfnmadd213pd xmm1, xmm2, dqword [rax] 	; c4 e2 e9 ac 08
vfnmadd213pd xmm1, xmm2, [rax] 		; c4 e2 e9 ac 08
vfnmadd132pd ymm1, ymm2, ymm3 		; c4 e2 ed 9c cb
vfnmadd132pd ymm1, ymm2, yword [rax] 	; c4 e2 ed 9c 08
vfnmadd132pd ymm1, ymm2, [rax] 		; c4 e2 ed 9c 08
vfnmadd231pd ymm1, ymm2, ymm3 		; c4 e2 ed bc cb
vfnmadd231pd ymm1, ymm2, yword [rax] 	; c4 e2 ed bc 08
vfnmadd231pd ymm1, ymm2, [rax] 		; c4 e2 ed bc 08
vfnmadd213pd ymm1, ymm2, ymm3 		; c4 e2 ed ac cb
vfnmadd213pd ymm1, ymm2, yword [rax] 	; c4 e2 ed ac 08
vfnmadd213pd ymm1, ymm2, [rax] 		; c4 e2 ed ac 08
vfnmsub132ss xmm1, xmm2, xmm3 		; c4 e2 69 9f cb
vfnmsub132ss xmm1, xmm2, dword [rax] 	; c4 e2 69 9f 08
vfnmsub132ss xmm1, xmm2, [rax] 		; c4 e2 69 9f 08
vfnmsub231ss xmm1, xmm2, xmm3 		; c4 e2 69 bf cb
vfnmsub231ss xmm1, xmm2, dword [rax] 	; c4 e2 69 bf 08
vfnmsub231ss xmm1, xmm2, [rax] 		; c4 e2 69 bf 08
vfnmsub213ss xmm1, xmm2, xmm3 		; c4 e2 69 af cb
vfnmsub213ss xmm1, xmm2, dword [rax] 	; c4 e2 69 af 08
vfnmsub213ss xmm1, xmm2, [rax] 		; c4 e2 69 af 08
vfnmsub132sd xmm1, xmm2, xmm3 		; c4 e2 e9 9f cb
vfnmsub132sd xmm1, xmm2, qword [rax] 	; c4 e2 e9 9f 08
vfnmsub132sd xmm1, xmm2, [rax] 		; c4 e2 e9 9f 08
vfnmsub231sd xmm1, xmm2, xmm3 		; c4 e2 e9 bf cb
vfnmsub231sd xmm1, xmm2, qword [rax] 	; c4 e2 e9 bf 08
vfnmsub231sd xmm1, xmm2, [rax] 		; c4 e2 e9 bf 08
vfnmsub213sd xmm1, xmm2, xmm3 		; c4 e2 e9 af cb
vfnmsub213sd xmm1, xmm2, qword [rax] 	; c4 e2 e9 af 08
vfnmsub213sd xmm1, xmm2, [rax] 		; c4 e2 e9 af 08
vfnmsub132ps xmm1, xmm2, xmm3 		; c4 e2 69 9e cb
vfnmsub132ps xmm1, xmm2, xmm3 		; c4 e2 69 9e cb
vfnmsub132ps xmm1, xmm2, [rax] 		; c4 e2 69 9e 08
vfnmsub231ps xmm1, xmm2, xmm3 		; c4 e2 69 be cb
vfnmsub231ps xmm1, xmm2, xmm3 		; c4 e2 69 be cb
vfnmsub231ps xmm1, xmm2, [rax] 		; c4 e2 69 be 08
vfnmsub213ps xmm1, xmm2, xmm3 		; c4 e2 69 ae cb
vfnmsub213ps xmm1, xmm2, xmm3 		; c4 e2 69 ae cb
vfnmsub213ps xmm1, xmm2, [rax] 		; c4 e2 69 ae 08
vfnmsub132ps ymm1, ymm2, ymm3 		; c4 e2 6d 9e cb
vfnmsub132ps ymm1, ymm2, yword [rax] 	; c4 e2 6d 9e 08
vfnmsub132ps ymm1, ymm2, [rax] 		; c4 e2 6d 9e 08
vfnmsub231ps ymm1, ymm2, ymm3 		; c4 e2 6d be cb
vfnmsub231ps ymm1, ymm2, yword [rax] 	; c4 e2 6d be 08
vfnmsub231ps ymm1, ymm2, [rax] 		; c4 e2 6d be 08
vfnmsub213ps ymm1, ymm2, ymm3 		; c4 e2 6d ae cb
vfnmsub213ps ymm1, ymm2, yword [rax] 	; c4 e2 6d ae 08
vfnmsub213ps ymm1, ymm2, [rax] 		; c4 e2 6d ae 08
vfnmsub132pd xmm1, xmm2, xmm3 		; c4 e2 e9 9e cb
vfnmsub132pd xmm1, xmm2, dqword [rax] 	; c4 e2 e9 9e 08
vfnmsub132pd xmm1, xmm2, [rax] 		; c4 e2 e9 9e 08
vfnmsub231pd xmm1, xmm2, xmm3 		; c4 e2 e9 be cb
vfnmsub231pd xmm1, xmm2, dqword [rax] 	; c4 e2 e9 be 08
vfnmsub231pd xmm1, xmm2, [rax] 		; c4 e2 e9 be 08
vfnmsub213pd xmm1, xmm2, xmm3 		; c4 e2 e9 ae cb
vfnmsub213pd xmm1, xmm2, dqword [rax] 	; c4 e2 e9 ae 08
vfnmsub213pd xmm1, xmm2, [rax] 		; c4 e2 e9 ae 08
vfnmsub132pd ymm1, ymm2, ymm3 		; c4 e2 ed 9e cb
vfnmsub132pd ymm1, ymm2, yword [rax] 	; c4 e2 ed 9e 08
vfnmsub132pd ymm1, ymm2, [rax] 		; c4 e2 ed 9e 08
vfnmsub231pd ymm1, ymm2, ymm3 		; c4 e2 ed be cb
vfnmsub231pd ymm1, ymm2, yword [rax] 	; c4 e2 ed be 08
vfnmsub231pd ymm1, ymm2, [rax] 		; c4 e2 ed be 08
vfnmsub213pd ymm1, ymm2, ymm3 		; c4 e2 ed ae cb
vfnmsub213pd ymm1, ymm2, yword [rax] 	; c4 e2 ed ae 08
vfnmsub213pd ymm1, ymm2, [rax] 		; c4 e2 ed ae 08
vfmaddsub132ps xmm1, xmm2, xmm3 	; c4 e2 69 96 cb
vfmaddsub132ps xmm1, xmm2, xmm3 	; c4 e2 69 96 cb
vfmaddsub132ps xmm1, xmm2, [rax] 	; c4 e2 69 96 08
vfmaddsub231ps xmm1, xmm2, xmm3 	; c4 e2 69 b6 cb
vfmaddsub231ps xmm1, xmm2, xmm3 	; c4 e2 69 b6 cb
vfmaddsub231ps xmm1, xmm2, [rax] 	; c4 e2 69 b6 08
vfmaddsub213ps xmm1, xmm2, xmm3 	; c4 e2 69 a6 cb
vfmaddsub213ps xmm1, xmm2, xmm3 	; c4 e2 69 a6 cb
vfmaddsub213ps xmm1, xmm2, [rax] 	; c4 e2 69 a6 08
vfmaddsub132ps ymm1, ymm2, ymm3 	; c4 e2 6d 96 cb
vfmaddsub132ps ymm1, ymm2, yword [rax] 	; c4 e2 6d 96 08
vfmaddsub132ps ymm1, ymm2, [rax] 	; c4 e2 6d 96 08
vfmaddsub231ps ymm1, ymm2, ymm3 	; c4 e2 6d b6 cb
vfmaddsub231ps ymm1, ymm2, yword [rax] 	; c4 e2 6d b6 08
vfmaddsub231ps ymm1, ymm2, [rax] 	; c4 e2 6d b6 08
vfmaddsub213ps ymm1, ymm2, ymm3 	; c4 e2 6d a6 cb
vfmaddsub213ps ymm1, ymm2, yword [rax] 	; c4 e2 6d a6 08
vfmaddsub213ps ymm1, ymm2, [rax] 	; c4 e2 6d a6 08
vfmaddsub132pd xmm1, xmm2, xmm3 	; c4 e2 e9 96 cb
vfmaddsub132pd xmm1, xmm2, dqword [rax]	; c4 e2 e9 96 08
vfmaddsub132pd xmm1, xmm2, [rax] 	; c4 e2 e9 96 08
vfmaddsub231pd xmm1, xmm2, xmm3 	; c4 e2 e9 b6 cb
vfmaddsub231pd xmm1, xmm2, dqword [rax]	; c4 e2 e9 b6 08
vfmaddsub231pd xmm1, xmm2, [rax] 	; c4 e2 e9 b6 08
vfmaddsub213pd xmm1, xmm2, xmm3 	; c4 e2 e9 a6 cb
vfmaddsub213pd xmm1, xmm2, dqword [rax]	; c4 e2 e9 a6 08
vfmaddsub213pd xmm1, xmm2, [rax] 	; c4 e2 e9 a6 08
vfmaddsub132pd ymm1, ymm2, ymm3 	; c4 e2 ed 96 cb
vfmaddsub132pd ymm1, ymm2, yword [rax] 	; c4 e2 ed 96 08
vfmaddsub132pd ymm1, ymm2, [rax] 	; c4 e2 ed 96 08
vfmaddsub231pd ymm1, ymm2, ymm3 	; c4 e2 ed b6 cb
vfmaddsub231pd ymm1, ymm2, yword [rax] 	; c4 e2 ed b6 08
vfmaddsub231pd ymm1, ymm2, [rax] 	; c4 e2 ed b6 08
vfmaddsub213pd ymm1, ymm2, ymm3 	; c4 e2 ed a6 cb
vfmaddsub213pd ymm1, ymm2, yword [rax] 	; c4 e2 ed a6 08
vfmaddsub213pd ymm1, ymm2, [rax] 	; c4 e2 ed a6 08
vfmsubadd132ps xmm1, xmm2, xmm3 	; c4 e2 69 97 cb
vfmsubadd132ps xmm1, xmm2, xmm3 	; c4 e2 69 97 cb
vfmsubadd132ps xmm1, xmm2, [rax] 	; c4 e2 69 97 08
vfmsubadd231ps xmm1, xmm2, xmm3 	; c4 e2 69 b7 cb
vfmsubadd231ps xmm1, xmm2, xmm3 	; c4 e2 69 b7 cb
vfmsubadd231ps xmm1, xmm2, [rax] 	; c4 e2 69 b7 08
vfmsubadd213ps xmm1, xmm2, xmm3 	; c4 e2 69 a7 cb
vfmsubadd213ps xmm1, xmm2, xmm3 	; c4 e2 69 a7 cb
vfmsubadd213ps xmm1, xmm2, [rax] 	; c4 e2 69 a7 08
vfmsubadd132ps ymm1, ymm2, ymm3 	; c4 e2 6d 97 cb
vfmsubadd132ps ymm1, ymm2, yword [rax] 	; c4 e2 6d 97 08
vfmsubadd132ps ymm1, ymm2, [rax] 	; c4 e2 6d 97 08
vfmsubadd231ps ymm1, ymm2, ymm3 	; c4 e2 6d b7 cb
vfmsubadd231ps ymm1, ymm2, yword [rax] 	; c4 e2 6d b7 08
vfmsubadd231ps ymm1, ymm2, [rax] 	; c4 e2 6d b7 08
vfmsubadd213ps ymm1, ymm2, ymm3 	; c4 e2 6d a7 cb
vfmsubadd213ps ymm1, ymm2, yword [rax] 	; c4 e2 6d a7 08
vfmsubadd213ps ymm1, ymm2, [rax] 	; c4 e2 6d a7 08
vfmsubadd132pd xmm1, xmm2, xmm3 	; c4 e2 e9 97 cb
vfmsubadd132pd xmm1, xmm2, dqword [rax]	; c4 e2 e9 97 08
vfmsubadd132pd xmm1, xmm2, [rax] 	; c4 e2 e9 97 08
vfmsubadd231pd xmm1, xmm2, xmm3 	; c4 e2 e9 b7 cb
vfmsubadd231pd xmm1, xmm2, dqword [rax]	; c4 e2 e9 b7 08
vfmsubadd231pd xmm1, xmm2, [rax] 	; c4 e2 e9 b7 08
vfmsubadd213pd xmm1, xmm2, xmm3 	; c4 e2 e9 a7 cb
vfmsubadd213pd xmm1, xmm2, dqword [rax]	; c4 e2 e9 a7 08
vfmsubadd213pd xmm1, xmm2, [rax] 	; c4 e2 e9 a7 08
vfmsubadd132pd ymm1, ymm2, ymm3 	; c4 e2 ed 97 cb
vfmsubadd132pd ymm1, ymm2, yword [rax] 	; c4 e2 ed 97 08
vfmsubadd132pd ymm1, ymm2, [rax] 	; c4 e2 ed 97 08
vfmsubadd231pd ymm1, ymm2, ymm3 	; c4 e2 ed b7 cb
vfmsubadd231pd ymm1, ymm2, yword [rax] 	; c4 e2 ed b7 08
vfmsubadd231pd ymm1, ymm2, [rax] 	; c4 e2 ed b7 08
vfmsubadd213pd ymm1, ymm2, ymm3 	; c4 e2 ed a7 cb
vfmsubadd213pd ymm1, ymm2, yword [rax] 	; c4 e2 ed a7 08
vfmsubadd213pd ymm1, ymm2, [rax] 	; c4 e2 ed a7 08

