<?xml version="1.0" encoding="ISO-8859-1"?>
<xsl:stylesheet version="1.0"
xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
xmlns:mime="http://www.freedesktop.org/standards/shared-mime-info">
<xsl:output method="text"
            encoding="UTF-8"
            indent="no"/>
<xsl:template match="/">
      <xsl:for-each select='mime:mime-info/mime:mime-type[@type=$type]'>
        <xsl:text>[Desktop Entry]&#10;</xsl:text>
        <xsl:text># Installed by xdg-mime from </xsl:text><xsl:value-of select="$source"/><xsl:text>&#10;</xsl:text>
        <xsl:text>Type=MimeType&#10;</xsl:text>
        <xsl:text>MimeType=</xsl:text><xsl:value-of select="@type"/><xsl:text>&#10;</xsl:text>
        <xsl:text>Icon=</xsl:text><xsl:value-of select="translate(@type,'/','-')"/><xsl:text>&#10;</xsl:text>
        <xsl:if test="mime:sub-class-of">
           <xsl:text>X-KDE-IsAlso=</xsl:text><xsl:value-of select="mime:sub-class-of/@type"/><xsl:text>&#10;</xsl:text>
        </xsl:if>
        <xsl:if test="mime:glob">
          <xsl:text>Patterns=</xsl:text>
          <xsl:for-each select='mime:glob[@pattern]'>
            <xsl:value-of select="@pattern"/><xsl:text>;</xsl:text>
          </xsl:for-each>
          <xsl:text>&#10;</xsl:text>
        </xsl:if>
        <xsl:text>Comment=</xsl:text><xsl:value-of select="mime:comment[not(@xml:lang)]"/><xsl:text>&#10;</xsl:text>
        <xsl:for-each select='mime:comment[@xml:lang]'>
          <xsl:sort select='@xml:lang'/>
          <xsl:text>Comment[</xsl:text><xsl:value-of select="@xml:lang"/><xsl:text>]=</xsl:text><xsl:value-of select="."/><xsl:text>&#10;</xsl:text>
        </xsl:for-each>
     </xsl:for-each>
</xsl:template>
</xsl:stylesheet>
