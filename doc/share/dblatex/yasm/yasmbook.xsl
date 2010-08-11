<?xml version='1.0' encoding="iso-8859-1"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version='1.0'>

<xsl:template match="book">
  <xsl:variable name="info" select="bookinfo|articleinfo|artheader|info"/>
  <xsl:variable name="lang">
    <xsl:call-template name="l10n.language">
      <xsl:with-param name="target" select="(/set|/book|/article)[1]"/>
      <xsl:with-param name="xref-context" select="true()"/>
    </xsl:call-template>
  </xsl:variable>

  <!-- Latex preamble -->
  <xsl:apply-templates select="." mode="preamble">
    <xsl:with-param name="lang" select="$lang"/>
  </xsl:apply-templates>

  <xsl:value-of select="$latex.begindocument"/>
  <xsl:call-template name="lang.document.begin">
    <xsl:with-param name="lang" select="$lang"/>
  </xsl:call-template>
  <xsl:text>\frontmatter&#10;</xsl:text>

  <!-- Apply the legalnotices here, when language is active -->
  <xsl:call-template name="print.legalnotice">
    <xsl:with-param name="nodes" select="$info/legalnotice"/>
  </xsl:call-template>

  <xsl:text>\maketitle&#10;</xsl:text>

  <!-- Print the TOC/LOTs -->
  <xsl:apply-templates select="." mode="toc_lots"/>
  <xsl:call-template name="label.id"/>

  <!-- Print the abstract -->
  <!--<xsl:apply-templates select="(abstract|$info/abstract)[1]"/>-->

  <xsl:text>\mainmatter&#10;</xsl:text>

  <!-- Body content -->
  <xsl:apply-templates select="*[not(self::abstract)]"/>
  <xsl:if test="*//indexterm|*//keyword">
    <xsl:text>\printindex&#10;</xsl:text>
  </xsl:if>
  <xsl:call-template name="lang.document.end">
    <xsl:with-param name="lang" select="$lang"/>
  </xsl:call-template>
  <xsl:value-of select="$latex.enddocument"/>
</xsl:template>

</xsl:stylesheet>
