# $Id: doc.images.mk 2304 2010-03-10 00:09:31Z peter $
#
# Largely copied from FreeBSD's doc/share/mk/doc.images.mk, but converted
# to GNU make syntax.
#
SCR2PNG?=	scr2png
SCR2PNGOPTS?=	${SCR2PNGFLAGS}
SCR2TXT?=	scr2txt
SCR2TXTOPTS?=	-l ${SCR2TXTFLAGS}
SED?=		sed
EPS2PNM?=	gs
EPS2PNMOPTS?=	-q -dBATCH -dGraphicsAlphaBits=4 -dTextAlphaBits=4 \
		-dEPSCrop -r${EPS2PNM_RES}x${EPS2PNM_RES} \
		-dNOPAUSE -dSAFER -sDEVICE=pnm -sOutputFile=-

#
# epsgeom is a python script for 1) extracting geometry information
# from a .eps file and 2) arrange it for ghostscript's pnm driver.
#
EPSGEOM?=	python ../share/misc/epsgeom.py
EPSGEOMOPTS?=	${EPS2PNM_RES} ${EPS2PNM_RES}
PNMTOPNG?=	pnmtopng
PNMTOPNGOPTS?=	${PNMTOPNGFLAGS}
PNGTOPNM?=	pngtopnm
PNGTOPNMOPTS?=	${PNGTOPNMFLAGS}
PPMTOPGM?=	ppmtopgm
PPMTOPGMOPTS?=	${PPMTOPGMFLAGS}
PNMTOPS?=	pnmtops
PNMTOPSOPTS?=	-noturn ${PNMTOPSFLAGS}
EPSTOPDF?=	${PREFIX}/bin/epstopdf
EPSTOPDFOPTS?=	${EPSTOPDFFLAGS}
#
PIC2PS?=	groff -p -S -Wall -mtty-char -man
#
PS2EPS?=	gs
PS2EPSOPTS?=	-q -dNOPAUSE -dSAFER -dDELAYSAFER \
		-sPAPERSIZE=letter -r72 -sDEVICE=bit \
		-sOutputFile=/dev/null ${PS2EPSFLAGS} ps2epsi.ps
PS2BBOX?=	gs
PS2BBOXOPTS?=	-q -dNOPAUSE -dBATCH -dSAFER -dDELAYSAFER \
		-sPAPERSIZE=letter -r72 -sDEVICE=bbox \
		-sOutputFile=/dev/null ${PS2BBOXFLAGS}

# The default resolution eps2png (82) assumes a 640x480 monitor, and is too
# low for the typical monitor in use today. The resolution of 100 looks
# much better on these monitors without making the image too large for
# a 640x480 monitor.
EPS2PNM_RES?=	100

_IMAGES_PNG= $(filter %.png,${IMAGES})
_IMAGES_EPS= $(filter %.eps,${IMAGES})
_IMAGES_SCR= $(filter %.scr,${IMAGES})
_IMAGES_TXT= $(filter %.txt,${IMAGES})
_IMAGES_PIC= $(filter %.pic,${IMAGES})

IMAGES_GEN_PNG= $(_IMAGES_EPS:.eps=.png)
IMAGES_GEN_EPS= $(_IMAGES_PNG:.png=.eps)
IMAGES_GEN_PDF= $(_IMAGES_EPS:.eps=.pdf)
IMAGES_SCR_PNG= $(_IMAGES_SCR:.scr=.png)
IMAGES_SCR_EPS= $(_IMAGES_SCR:.scr=.eps)
IMAGES_SCR_PDF= $(_IMAGES_SCR:.scr=.pdf)
IMAGES_SCR_TXT= $(_IMAGES_SCR:.scr=.txt)
IMAGES_PIC_PNG= $(_IMAGES_PIC:.pic=.png)
IMAGES_PIC_EPS= $(_IMAGES_PIC:.pic=.eps)
IMAGES_PIC_PDF= $(_IMAGES_PIC:.pic=.pdf)

IMAGES_GEN_PNG+= ${IMAGES_PIC_PNG}
IMAGES_GEN_PDF+= ${IMAGES_PIC_PDF} ${IMAGES_SCR_PDF}

CLEANFILES+= ${IMAGES_GEN_PNG} ${IMAGES_GEN_EPS} ${IMAGES_GEN_PDF}
CLEANFILES+= ${IMAGES_SCR_PNG} ${IMAGES_SCR_EPS} ${IMAGES_SCR_TXT}
CLEANFILES+= ${IMAGES_PIC_PNG} ${IMAGES_PIC_EPS} $(_IMAGES_PIC:.pic=.ps)

IMAGES_PNG= ${_IMAGES_PNG} ${IMAGES_GEN_PNG} ${IMAGES_SCR_PNG} ${IMAGES_PIC_PNG}
IMAGES_EPS= ${_IMAGES_EPS} ${IMAGES_GEN_EPS} ${IMAGES_SCR_EPS} ${IMAGES_PIC_EPS}
IMAGES_TXT= ${_IMAGES_TXT} ${IMAGES_SCR_TXT}
IMAGES_PDF= ${IMAGES_GEN_PDF} ${_IMAGES_PNG}

# Use suffix rules to convert .scr files to other formats

%.png: %.scr
	${SCR2PNG} ${SCR2PNGOPTS} < ${.IMPSRC} > ${.TARGET}

## If we want grayscale, convert with ppmtopgm before running through pnmtops
ifdef GREYSCALE_IMAGES
%.eps: %.scr
	${SCR2PNG} ${SCR2PNGOPTS} < $< | \
		${PNGTOPNM} ${PNGTOPNMOPTS} | \
		${PPMTOPGM} ${PPMTOPGMOPTS} | \
		${PNMTOPS} ${PNMTOPSOPTS} > $@
else
%.eps: %.scr
	${SCR2PNG} ${SCR2PNGOPTS} < $< | \
		${PNGTOPNM} ${PNGTOPNMOPTS} | \
		${PNMTOPS} ${PNMTOPSOPTS} > $@
endif

# The .txt files need to have any trailing spaces trimmed from
# each line, which is why the output from ${SCR2TXT} is run
# through ${SED}
%.txt: %.scr
	${SCR2TXT} ${SCR2TXTOPTS} < $< | ${SED} -E -e 's/ +$$//' > $@

%.ps: %.pic
	${PIC2PS} $< > $@

# When ghostscript built with A4=yes is used, ps2epsi's paper size also
# becomes the A4 size.  However, the ps2epsi fails to convert grops(1)
# outputs, which is the letter size, and we cannot change ps2epsi's paper size
# from the command line.  So ps->eps suffix rule is defined.  In the rule,
# gs(1) is used to generate the bitmap preview and the size of the
# bounding box.
#
# ps2epsi.ps in GS 8.70 requires $outfile before the conversion and it
# must contain %%BoundingBox line which the "gs -sDEVICE=bbox" outputs
# (the older versions calculated BBox directly in ps2epsi.ps).
%.eps: %.ps
	${PS2BBOX} ${PS2BBOXOPTS} $< > $@ 2>&1
	${SETENV} outfile=$@ ${PS2EPS} ${PS2EPSOPTS} < $< 1>&2
	(echo "save countdictstack mark newpath /showpage {} def /setpagedevice {pop} def";\
		echo "%%EndProlog";\
		echo "%%Page: 1 1";\
		echo "%%BeginDocument: $<";\
	) >> $@
	${SED}	-e '/^%%BeginPreview:/,/^%%EndPreview[^[:graph:]]*$$/d' \
		-e '/^%!PS-Adobe/d' \
		-e '/^%%[A-Za-z][A-Za-z]*[^[:graph:]]*$$/d'\
		-e '/^%%[A-Za-z][A-Za-z]*: /d' < $< >> $@
	(echo "%%EndDocument";\
		echo "%%Trailer";\
		echo "cleartomark countdictstack exch sub { end } repeat restore";\
		echo "%%EOF";\
	) >> $@

# We can't use suffix rules to generate the rules to convert EPS to PNG and
# PNG to EPS.  This is because a .png file can depend on a .eps file, and
# vice versa, leading to a loop in the dependency graph.  Instead, build
# the targets on the fly.

define genpng_template
${1}: $(1:.png=.eps)
	${EPSGEOM} -offset ${EPSGEOMOPTS} $(1:.png=.eps) \
		| ${EPS2PNM} ${EPS2PNMOPTS} \
		-g`${EPSGEOM} -geom ${EPSGEOMOPTS} $(1:.png=.eps)` - \
		| ${PNMTOPNG} > ${1}
endef
$(foreach image,${IMAGES_GEN_PNG},$(eval $(call genpng_template,$(image))))

define geneps_template
${1}: $(1:.eps=.png)
	${PNGTOPNM} ${PNGTOPNMOPTS} $(1:.eps=.png) | \
		${PNMTOPS} ${PNMTOPSOPTS} > ${1}
endef
$(foreach image,${IMAGES_GEN_EPS},$(eval $(call geneps_template,$(image))))

define genpdf_template
${1}: $(1:.pdf=.eps)
	${EPSTOPDF} ${EPSTOPDFOPTS} --outfile=${1} $(1:.pdf=.eps)
endef
$(foreach image,${IMAGES_GEN_PDF},$(eval $(call genpdf_template,$(image))))

