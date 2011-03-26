# [oformat macho64]
.globl globl
globl:

#.indirect_symbol indirect_symbol

.reference reference

.weak_reference weak_reference

.lazy_reference lazy_reference

.reference reference_def
reference_def:

.weak_reference weak_reference_def
weak_reference_def:

.lazy_reference lazy_reference_def
lazy_reference_def:

.private_extern private_extern
private_extern:

.globl private_global
.private_extern private_global
private_global:

.desc desc_local, 5
desc_local:

.desc desc_undef, 5

.globl desc_global 
.desc desc_global, 5
desc_global:

.no_dead_strip no_dead_strip
no_dead_strip:

#.section __TEXT, __coal, coalesced
#.weak_definition weak_definition
#weak_definition:


