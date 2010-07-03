; [yasm -f xdf -p nasm]
[bits 16]
[extern ax]
[extern $ax]
[extern $mov]
$dword:
mov ax, 5
mov [$ax], byte 5
mov [$mov], byte 5
mov dword [$dword], $ax
