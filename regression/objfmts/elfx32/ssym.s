# [yasm -p gas -f elfx32 -g dwarf2pass]
.text
foo:
movl %eax, foo@PLT
movl %eax, foo@GOTPCREL
movl %eax, bar@TLSGD
movl %eax, bar@TLSLD
movl %eax, bar@GOTTPOFF
movl %eax, bar@TPOFF
movl %eax, bar@DTPOFF
movl %eax, foo@GOT
