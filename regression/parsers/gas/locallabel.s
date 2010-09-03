.text
    jmp 0f
0:
    jmp 0f
    jmp 0b
0:
50:
    jmp 50f
    jmp 50b
50:
.byte 0xf	# should not be local label reference
.byte 0x1b	# also should not be local label reference
