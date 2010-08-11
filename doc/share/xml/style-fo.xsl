<?xml version='1.0'?> 
<xsl:stylesheet
 xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

 <xsl:import
  href="http://docbook.sourceforge.net/release/xsl/current/fo/docbook.xsl" />
 <xsl:include href="style-common.xsl" />

 <xsl:param name="insert.xref.page.number">yes</xsl:param>
 <xsl:param name="fop.extensions">1</xsl:param>
 <xsl:param name="paper.type">USletter</xsl:param>

 <!-- Don't output any message -->
 <xsl:template name="root.messages">
 </xsl:template>

<!-- Customize function to non-monospace italics with parens. -->
<xsl:template match="function">
  <xsl:choose>
    <xsl:when test="parameter or function or replaceable">
      <xsl:variable name="nodes" select="text()|*"/>
      <xsl:call-template name="inline.italicseq">
        <xsl:with-param name="content">
          <xsl:call-template name="simple.xlink">
            <xsl:with-param name="content">
              <xsl:apply-templates select="$nodes[1]"/>
            </xsl:with-param>
          </xsl:call-template>
        </xsl:with-param>
      </xsl:call-template>
      <xsl:text>(</xsl:text>
      <xsl:apply-templates select="$nodes[position()>1]"/>
      <xsl:text>)</xsl:text>
    </xsl:when>
    <xsl:otherwise>
     <xsl:call-template name="inline.italicseq"/>
     <xsl:text>(</xsl:text>
     <xsl:text>)</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="function/parameter" priority="2">
  <xsl:call-template name="inline.italicseq"/>
  <xsl:if test="following-sibling::*">
    <xsl:text>, </xsl:text>
  </xsl:if>
</xsl:template>

<xsl:template match="function/replaceable" priority="2">
  <xsl:call-template name="inline.italicseq"/>
  <xsl:if test="following-sibling::*">
    <xsl:text>, </xsl:text>
  </xsl:if>
</xsl:template>

</xsl:stylesheet>
