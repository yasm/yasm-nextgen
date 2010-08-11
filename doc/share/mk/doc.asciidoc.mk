# $Id: doc.asciidoc.mk 2306 2010-03-10 00:11:29Z peter $
#
# Requires:
# - asciidoc
# - xsltproc (usually part of libxslt package)
# - xmllint (usually part of libxml2 package)
# - dblatex (and dependencies) for PDF output
# - tidy for HTML output

DOCTYPE?=	book
DOCLANG?=	en

ASCIIDOC?=	asciidoc
XMLLINT?=	xmllint

CLEANFILES+=	${DOC}.xml

include ../share/mk/doc.docbookcore.mk

# XML
${DOC}.xml: ${MASTERDOC} ${SRCS}
	${ASCIIDOC} \
		-o $@ \
		-a docinfo \
		-a lang=${DOCLANG} \
		-b docbook \
		-d ${DOCTYPE} \
		-f ../share/asciidoc/asciidoc.conf \
		${ASCIIDOCFLAGS} \
		${MASTERDOC}
	${XMLLINT} --nonet --noout $@

