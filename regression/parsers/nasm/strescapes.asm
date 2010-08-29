db "foo"			; out: 66 6f 6f
db `foo`			; out: 66 6f 6f
db `\x`				; out: 78
db `\y`				; out: 79
db `\8`				; out: 38
db `\0007`			; out: 00 37
db `\xffff`			; out: ff 66 66
;db `\uabcg`
db `\u263a`			; out: e2 98 ba
db `\'\"\`\\\?\a\b\t\n\v\f\r\e`	; out: 27 22 60 5c 3f 07 08 09 0a 0b 0c 0d 1a
dd 'abcd'+1			; out: 62 62 63 64
