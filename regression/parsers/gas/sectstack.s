# [oformat elf32]
.previous

.section A
# current: A -- previous: .text -- stack:
.byte 'A'

.section B
# current: B -- previous: A -- stack:
.byte 'B'

.pushsection C
# current: C -- previous: B -- stack: B,A
.byte 'C'

.previous
# current: B -- previous: C -- stack: B,A
.byte 'B'

.popsection
# current: B -- previous: A -- stack:
.byte 'B'

.previous
# current: A -- previous: B -- stack:
.byte 'A'

.popsection
