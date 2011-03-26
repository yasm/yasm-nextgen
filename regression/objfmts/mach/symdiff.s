# [oformat macho64]
foo:
	.long 0

.section __TEXT,__text2
bar:
	.quad foo-bar

