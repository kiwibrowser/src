<?xml version="1.0" encoding="ISO-8859-1"?>
<xsl:stylesheet version="1.0"
xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
xmlns:mime="http://www.freedesktop.org/standards/shared-mime-info">
<xsl:output method="text"
            encoding="UTF-8"
            indent="no"/>
<xsl:template match="/">
      <xsl:for-each select='mime:mime-info/mime:mime-type'>
        <xsl:value-of select="@type"/><xsl:text>&#10;</xsl:text>
      </xsl:for-each>
</xsl:template>
</xsl:stylesheet>
