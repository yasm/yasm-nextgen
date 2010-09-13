[bits 32]
and esi, 4294967287	; OUT: 83 e6 f7
and esi, 0x100000000	; OUT: 83 e6 00
and esi, 0x180000000	; OUT: 81 e6 00 00 00 80
and esi, 0x1fffffff7	; OUT: 83 e6 f7
[bits 64]
and esi, 4294967287	; OUT: 83 e6 f7
