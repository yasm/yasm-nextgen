# [oformat elf32]
movl % eax, % ebx
movl $ eax, % ebx
#call 0f
#0 :
#movl $'\00000000, %ebx    # error in yasm
movl $'\003, %ebx
#movl $'\c55, %ebx         # error in yasm
movl $'\x55, %ebx
movl $'\b, %ebx
movl $'\\, %ebx
movl $' , %ebx
movl $'\ , %ebx
#movl "\\", %ebx
movl $ .label, %eax
movl $ .type, %eax
// also a comment
.type = 0x40
.float 0.1
.float .1
//.float .x
.float 0f.1
.float 00.1
.float 1.0
.float 1e5
.float 0e5
.float 5.0
.float 00e5
.float 0b0.1
.float 0x5e1
.float 5e1
//.word 0f5e1
//.word 0.1
//jmp 011f
#.octa 0x333_0_12345678_1
.word 0x
//.word 0eh
//.word 0bh
//.word 0b
.word 0x
//.word 0f
.string < 1 >
.string "\x\y"
.string "\0000"
movl $0, %fs:0
//movl $0, %xx
es movl $0, 0
//movl %,%ebx
.byte  74, 0112, 0x4A, 0X4a, 'J, '\J # All the same value.
.ascii "Ring the bell\7", <7>                  # A string constant.
.string <1>, <2>
.string "foo"
.string "foo", <0>
.octa  0x123456789abcdef0123456789ABCDEF0 # A bignum.
.float 0f-31415926535897932384626433832795028841971.693993751E-40 # - pi, a flonum.
//.octa 123_456
