.code32
inb (%dx), %al
inw (%dx), %ax
inl (%dx), %eax
inb %dx, %al
inw %dx, %ax
inl %dx, %eax
inb (%dx)
inw (%dx)
inl (%dx)
inb %dx
inw %dx
inl %dx

outb %al, (%dx)
outw %ax, (%dx)
outl %eax, (%dx)
outb %al, %dx
outw %ax, %dx
outl %eax, %dx
outb (%dx)
outw (%dx)
outl (%dx)
outb %dx
outw %dx
outl %dx

