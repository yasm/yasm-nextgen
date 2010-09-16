# [oformat elf32]
.globl name1_
.symver name1_, name1@nodename	# ref and def: refs changed to name1@nodename
				# equivalent to: sym rename + alias back
.symver name2_, name2@nodename	# not defined: sym renamed to name2@nodename
.symver name3_, name3@nodename	# no ref or def: doesn't appear in symtab

.text
.type name1_, @function

name1_:
	call name1_
	call name2_

.symver name4_, name4@@nodename # both appear, refs not changed
				# roughly equivalent to name@@nodename = name4_
.weak name4_
name4_:
	call name4_

.symver name5_, name5@@@nodename # not defined: sym renamed to name5@nodename
.symver name6_, name6@@@nodename # defined: sym renamed to name6@@nodename

.globl name6_
name6_:
	call name5_
	call name6_

.symver name7_, name7 @ @ @ nodename
	call name7_

.weak foo
foo:
bar = foo

