<?xml version='1.0'?>

<xsl:stylesheet 
   version='1.0'
   xmlns:xsl='http://www.w3.org/1999/XSL/Transform'
   xmlns:dtb="http://www.daisy.org/z3986/2005/dtbook/"	
   xmlns:louis="http://liblouis.org/liblouis"
   exclude-result-prefixes="dtb">
  
  <xsl:output omit-xml-declaration="no" encoding="UTF-8" method="xml"
	      indent="yes" media-type="text/xml"/>

  <xsl:param name="translation_table">de-ch-g2.ctb</xsl:param>

  <xsl:template match="dtb:em">
    <xsl:apply-templates mode="italic"/>
  </xsl:template>

  <xsl:template match="dtb:strong">
    <xsl:apply-templates mode="bold"/>
  </xsl:template>

  <xsl:template match="text()">
    <xsl:value-of select='louis:translate(string(),string($translation_table))'/>
  </xsl:template>
  
  <xsl:template match="text()" mode="italic">
    <xsl:value-of select='louis:translate(string(),string($translation_table),"italic")'/>
  </xsl:template>
  
  <xsl:template match="text()" mode="bold">
    <xsl:value-of select='louis:translate(string(),string($translation_table),"bold")'/>
  </xsl:template>
  
  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>
  
</xsl:stylesheet>

