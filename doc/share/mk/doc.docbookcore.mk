# $Id: doc.docbookcore.mk 2354 2010-08-07 21:50:48Z peter $
#
# Requires:
# - xsltproc (usually part of libxslt package)
# - xmllint (usually part of libxml2 package)
# - dblatex (and dependencies) for PDF output
# - tidy for HTML output

DESTDIR?=	.

XSLTPROC?=	xsltproc
DBLATEX?=	dblatex
TIDY?=		tidy

XMLCATALOGFILES?=	../share/xml/catalog.xml

XSLHTML?=	../share/xml/style-html.xsl
XSLHTMLCHUNK?=	../share/xml/style-chunk.xsl

TIDYOPTS?=	-wrap 90 -m -raw -f /dev/null -asxml ${TIDYFLAGS}

.PHONY: all install xml html html-split pdf

.DEFAULT_GOAL := all

all: html html-split pdf
xml: ${DOC}.xml

include ../share/mk/doc.images.mk

# HTML-SPLIT
html-split: index.html
index.html HTML.manifest: ${DOC}.xml ${IMAGES_PNG}
	env XML_CATALOG_FILES="${XMLCATALOGFILES}" ${XSLTPROC} \
		--nonet \
		--stringparam use.extensions 0 \
		${XSLHTMLCHUNK} \
		${DOC}.xml 2> .xslterr
	@grep "Writing" .xslterr | cut -f 2 -d ' ' > HTML.manifest
	@-grep -v "Writing" .xslterr
	@rm -f .xslterr
	-${TIDY} ${TIDYOPTS} `cat HTML.manifest`

# HTML
html: ${DOC}.html
${DOC}.html: ${DOC}.xml ${IMAGES_PNG}
	env XML_CATALOG_FILES="${XMLCATALOGFILES}" ${XSLTPROC} \
		--nonet \
		--output ${DOC}.html \
		--stringparam use.extensions 0 \
		${XSLHTML} \
		${DOC}.xml
	-${TIDY} ${TIDYOPTS} $@

# PDF
pdf: ${DOC}.pdf
${DOC}.pdf: ${DOC}.xml ${IMAGES_EPS}
	${DBLATEX} -t pdf -S ../share/dblatex/yasm.specs -b pdftex \
		-o ${DOC}.pdf \
		-P latex.unicode.use=1 \
		-P doc.lot.show="figure,table,equation" \
		${DOC}.xml

install: all
	mkdir -p ${DESTDIR}
	mkdir -p ${DESTDIR}/html
	for html in `cat HTML.manifest`; do \
	    cp $$html ${DESTDIR}/html; \
	done
	cp ${DOC}.html ${DESTDIR}/html
	for pngdir in $(dir ${IMAGES_PNG}); do \
	    mkdir -p ${DESTDIR}/html/$$pngdir; \
	done
	for png in ${IMAGES_PNG}; do \
	    cp $$png ${DESTDIR}/html/$$png; \
	done
	cp ${DOC}.pdf ${DESTDIR}

clean:
	-rm -f ${CLEANFILES}
	if [ -f HTML.manifest ]; then \
	    rm -f `cat HTML.manifest` HTML.manifest; \
	fi
	-rm -f ${DOC}.html
	-rm -f ${DOC}.pdf
