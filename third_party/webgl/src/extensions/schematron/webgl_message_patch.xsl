<?xml version="1.0"?>

<xsl:stylesheet version="1.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:axsl="http://www.w3.org/1999/XSL/TransformAlias">

  <xsl:template match="axsl:message">
    <axsl:message>
      <xsl:copy-of select="@*" />
      <xsl:attribute name="terminate">yes</xsl:attribute>
      <xsl:copy-of select="node()" />
    </axsl:message>
  </xsl:template>

  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()" />
    </xsl:copy>
  </xsl:template>

</xsl:stylesheet>
