# [yasm -p gas -f elf32 -g dwarf2pass]
.text
.local sym2
sym2:

.globl sym3

sym3:

sym1 = sym2

.globl sym4
sym4 = sym2 + 2

.local sym5
sym5 = sym4

# ok in GNU as, error in yasm
#.globl sym6
#sym6 = sym0

#sym7 = sym8
#sym8 = sym7

#.globl sym9
#sym9 = sym7
