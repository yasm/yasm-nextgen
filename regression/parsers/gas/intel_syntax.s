# [oformat elf64]
# 1 "gvmat64.S"
# 1 "<built-in>"
# 1 "<command-line>"
# 1 "gvmat64.S"
# 58 "gvmat64.S"
.intel_syntax noprefix

.globl _match_init, _longest_match
.text
_longest_match:
# 197 "gvmat64.S"
        mov [(rsp + 40 - 96)],rbx
        mov [(rsp + 48 - 96)],rbp


        mov rcx,rdi

        mov r8d,esi


        mov [(rsp + 56 - 96)],r12
        mov [(rsp + 64 - 96)],r13
        mov [(rsp + 72 - 96)],r14
        mov [(rsp + 80 - 96)],r15
# 219 "gvmat64.S"
        mov edi, [ rcx + 168]
        mov esi, [ rcx + 188]
        mov eax, [ rcx + 76]
        mov ebx, [ rcx + 172]
        cmp edi, esi
        jl LastMatchGood
        shr ebx, 2
LastMatchGood:






        dec ebx
        shl ebx, 16
        or ebx, eax






        mov eax, [ rcx + 192]
        mov [(rsp + 8 - 96)], ebx
        mov r10d, [ rcx + 164]
        cmp r10d, eax
        cmovnl r10d, eax
        mov [(rsp + 16 - 96)],r10d




        mov r10, [ rcx + 80]
        mov ebp, [ rcx + 156]
        lea r13, [r10 + rbp]




         mov r9,r13
         neg r13
         and r13,3





        mov eax, [ rcx + 68]
        sub eax, (258 +3 +1)


        xor edi,edi
        sub ebp, eax

        mov r11d, [ rcx + 168]

        cmovng ebp,edi






       lea rsi,[r10+r11]





        movzx r12d,word ptr [r9]
        movzx ebx, word ptr [r9 + r11 - 1]

        mov rdi, [ rcx + 96]



        mov edx, [(rsp + 8 - 96)]

        cmp bx,word ptr [rsi + r8 - 1]
        jz LookupLoopIsZero



LookupLoop1:
        and r8d, edx

        movzx r8d, word ptr [rdi + r8*2]
        cmp r8d, ebp
        jbe LeaveNow



        sub edx, 0x00010000
  .att_syntax
        js LeaveNow
  .intel_syntax noprefix

LoopEntry1:
        cmp bx,word ptr [rsi + r8 - 1]
  .att_syntax
        jz LookupLoopIsZero
  .intel_syntax noprefix

LookupLoop2:
        and r8d, edx

        movzx r8d, word ptr [rdi + r8*2]
        cmp r8d, ebp
  .att_syntax
        jbe LeaveNow
  .intel_syntax noprefix
        sub edx, 0x00010000
  .att_syntax
        js LeaveNow
  .intel_syntax noprefix

LoopEntry2:
        cmp bx,word ptr [rsi + r8 - 1]
  .att_syntax
        jz LookupLoopIsZero
  .intel_syntax noprefix

LookupLoop4:
        and r8d, edx

        movzx r8d, word ptr [rdi + r8*2]
        cmp r8d, ebp
  .att_syntax
        jbe LeaveNow
  .intel_syntax noprefix
        sub edx, 0x00010000
  .att_syntax
        js LeaveNow
  .intel_syntax noprefix

LoopEntry4:

        cmp bx,word ptr [rsi + r8 - 1]
  .att_syntax
        jnz LookupLoop1
        jmp LookupLoopIsZero
  .intel_syntax noprefix
# 383 "gvmat64.S"
.balign 16
LookupLoop:
        and r8d, edx

        movzx r8d, word ptr [rdi + r8*2]
        cmp r8d, ebp
  .att_syntax
        jbe LeaveNow
  .intel_syntax noprefix
        sub edx, 0x00010000
  .att_syntax
        js LeaveNow
  .intel_syntax noprefix

LoopEntry:

        cmp bx,word ptr [rsi + r8 - 1]
  .att_syntax
        jnz LookupLoop1
  .intel_syntax noprefix
LookupLoopIsZero:
        cmp r12w, word ptr [r10 + r8]
  .att_syntax
        jnz LookupLoop1
  .intel_syntax noprefix



        mov [(rsp + 8 - 96)], edx






        lea rsi,[r8+r10]
        mov rdx, 0xfffffffffffffef8
        lea rsi, [rsi + r13 + 0x0108]
        lea rdi, [r9 + r13 + 0x0108]

        prefetcht1 [rsi+rdx]
        prefetcht1 [rdi+rdx]
# 442 "gvmat64.S"
LoopCmps:
        mov rax, [rsi + rdx]
        xor rax, [rdi + rdx]
        jnz LeaveLoopCmps

        mov rax, [rsi + rdx + 8]
        xor rax, [rdi + rdx + 8]
        jnz LeaveLoopCmps8


        mov rax, [rsi + rdx + 8+8]
        xor rax, [rdi + rdx + 8+8]
        jnz LeaveLoopCmps16

        add rdx,8+8+8

  .att_syntax
        jnz LoopCmps
        jmp LenMaximum
  .intel_syntax noprefix

LeaveLoopCmps16: add rdx,8
LeaveLoopCmps8: add rdx,8
LeaveLoopCmps:

        test eax, 0x0000FFFF
        jnz LenLower

        test eax,0xffffffff

        jnz LenLower32

        add rdx,4
        shr rax,32
        or ax,ax
  .att_syntax
        jnz LenLower
  .intel_syntax noprefix

LenLower32:
        shr eax,16
        add rdx,2

LenLower:
        sub al, 1
        adc rdx, 0



        lea rax, [rdi + rdx]
        sub rax, r9
        cmp eax, 258
  .att_syntax
        jge LenMaximum
  .intel_syntax noprefix





        cmp eax, r11d
        jg LongerMatch

        lea rsi,[r10+r11]

        mov rdi, [ rcx + 96]
        mov edx, [(rsp + 8 - 96)]
  .att_syntax
        jmp LookupLoop
  .intel_syntax noprefix






LongerMatch:
        mov r11d, eax
        mov [ rcx + 160], r8d
        cmp eax, [(rsp + 16 - 96)]
  .att_syntax
        jge LeaveNow
  .intel_syntax noprefix

        lea rsi,[r10+rax]

        movzx ebx, word ptr [r9 + rax - 1]
        mov rdi, [ rcx + 96]
        mov edx, [(rsp + 8 - 96)]
  .att_syntax
        jmp LookupLoop
  .intel_syntax noprefix



LenMaximum:
        mov r11d,258
        mov [ rcx + 160], r8d




LeaveNow:
        mov eax, [ rcx + 164]
        cmp r11d, eax
        cmovng eax, r11d
# 556 "gvmat64.S"
        mov rbx,[(rsp + 40 - 96)]
        mov rbp,[(rsp + 48 - 96)]
        mov r12,[(rsp + 56 - 96)]
        mov r13,[(rsp + 64 - 96)]
        mov r14,[(rsp + 72 - 96)]
        mov r15,[(rsp + 80 - 96)]


        ret 0






_match_init:
  ret 0
