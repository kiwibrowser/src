<?xml version='1.0' encoding='utf-8'?>
<xsl:stylesheet version="1.0"
                id="stylesheet"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
    <xsl:template match="items">
        <html>
        <head>
        </head>
        <body>
        <xsl:for-each select="item">
          <span><xsl:value-of select="."/></span>
        </xsl:for-each>
        </body>
        </html>
    </xsl:template>
</xsl:stylesheet>

