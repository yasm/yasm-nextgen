# [oformat elf64]
movsbw %cl, %cx
movsbl %cl, %ecx
movswl %cx, %ecx
movsbq %cl, %rcx
movswq %cx, %rcx
movslq %ecx, %rcx

movsx %cl, %cx
movsx %cl, %ecx
movsx %cx, %ecx
movsx %cl, %rcx
movsx %cx, %rcx
movsx %ecx, %rcx

movzbw %cl, %cx
movzbl %cl, %ecx
movzwl %cx, %ecx
movzbq %cl, %rcx
movzwq %cx, %rcx

movzx %cl, %cx
movzx %cl, %ecx
movzx %cx, %ecx
movzx %cl, %rcx
movzx %cx, %rcx


