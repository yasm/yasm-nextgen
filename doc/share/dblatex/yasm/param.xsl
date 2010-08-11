<?xml version='1.0' encoding="iso-8859-1"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version='1.0'>

<xsl:import href="yasmbook.xsl"/>

<!-- DB2LaTeX has its own admonition graphics -->
<xsl:param name="figure.note">note</xsl:param>
<xsl:param name="figure.tip">tip</xsl:param>
<xsl:param name="figure.warning">warning</xsl:param>
<xsl:param name="figure.caution">caution</xsl:param>
<xsl:param name="figure.important">important</xsl:param>

<!-- Options used for documentclass -->
<xsl:param name="latex.class.options">letterpaper,10pt,twoside,openright</xsl:param>
<xsl:param name="latex.class.book">book</xsl:param>

<!-- Hyperref options -->
<xsl:param name="latex.hyperparam">linktocpage,colorlinks,citecolor=blue,pdfstartview=FitH,plainpages=false,pdfpagelabels</xsl:param>

</xsl:stylesheet>
