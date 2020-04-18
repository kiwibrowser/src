<?xml version="1.0" encoding="ISO-8859-1"?>
<xsl:stylesheet version="1.0"
xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method="html"
            encoding="UTF-8"
            omit-xml-declaration="yes"
            indent="no"/>
<xsl:template match="/">
      <xsl:for-each select='refentry/refnamediv'>
        <xsl:variable name="href">
          <xsl:value-of select="refname"/>
          <xsl:text>.html</xsl:text>
        </xsl:variable>
        <tr>
         <td><a href="{$href}"><xsl:value-of select="refname"/></a>:</td>
         <td><xsl:value-of select="refpurpose"/></td>
        </tr>
     </xsl:for-each>
</xsl:template>
</xsl:stylesheet>
