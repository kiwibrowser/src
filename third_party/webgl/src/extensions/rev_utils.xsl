<?xml version="1.0"?>

<xsl:stylesheet version="1.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<xsl:template name="last_rev">
  <xsl:value-of select="count(history/revision)"/>
</xsl:template>

<xsl:template name="last_mod">
  <xsl:for-each select="history/revision">
    <xsl:sort select="@date" />
    <xsl:if test="position()=last()">
      <xsl:call-template name="month_of_date"/><xsl:text> </xsl:text>
      <xsl:value-of select="substring(@date,9)"/><xsl:text>, </xsl:text>
      <xsl:value-of select="substring(@date,1,4)"/>
    </xsl:if>
  </xsl:for-each>
</xsl:template>

<xsl:template name="month_of_date">
  <xsl:param name="date" select="@date" />
  <xsl:variable name="m" select="substring($date,6,2)" />
  <xsl:choose>
    <xsl:when test="$m='01'">January</xsl:when>
    <xsl:when test="$m='02'">February</xsl:when>
    <xsl:when test="$m='03'">March</xsl:when>
    <xsl:when test="$m='04'">April</xsl:when>
    <xsl:when test="$m='05'">May</xsl:when>
    <xsl:when test="$m='06'">June</xsl:when>
    <xsl:when test="$m='07'">July</xsl:when>
    <xsl:when test="$m='08'">August</xsl:when>
    <xsl:when test="$m='09'">September</xsl:when>
    <xsl:when test="$m='10'">October</xsl:when>
    <xsl:when test="$m='11'">November</xsl:when>
    <xsl:when test="$m='12'">December</xsl:when>
    <xsl:otherwise><xsl:value-of select="$m"/></xsl:otherwise>
  </xsl:choose>
</xsl:template>

</xsl:stylesheet>