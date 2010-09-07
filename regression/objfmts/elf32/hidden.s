# [oformat elf32]
.text
.weak foo
.hidden foo
.type foo, @object
.size foo, 8
foo:
.quad 0

