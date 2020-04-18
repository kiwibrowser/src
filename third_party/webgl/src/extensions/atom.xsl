<?xml version="1.0"?>

<xsl:stylesheet version="1.0"
                xmlns="http://www.w3.org/2005/Atom"
                xmlns:a="http://www.w3.org/2005/Atom"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<xsl:variable name="registry" select="document('registry.xml')/registry" />

<xsl:template match="a:feed">
  <xsl:copy>
    <xsl:copy-of select="@*" />
    <xsl:apply-templates select="*" />

    <xsl:for-each select="$registry/*/history/revision">
      <xsl:sort select="@date" order="descending"/>
      <xsl:sort select="@number" order="descending"/>
      <xsl:if test="position() &lt; 11">
        <entry>
          <title>
            <xsl:value-of select="../../name" />
            <xsl:text> : revision </xsl:text>
            <xsl:value-of select="@number"/>
          </title>
          <link rel="alternate" type="text/html" href="{../../@href}"/>

          <id><xsl:value-of select="../../@href"/>#r=<xsl:value-of select="@number"/></id>
          <updated><xsl:value-of select="translate(@date,'/','-')"/>T23:59:59Z</updated>
          <summary>
            <xsl:for-each select="change">
              <xsl:value-of select="."/><xsl:text>&#xa;&#xa;</xsl:text>
            </xsl:for-each>
          </summary>
        </entry>
      </xsl:if>
    </xsl:for-each>
  </xsl:copy>
</xsl:template>

<xsl:template match="*">
  <xsl:copy>
    <xsl:copy-of select="@*" />
    <xsl:apply-templates select="node()" />
  </xsl:copy>
</xsl:template>

<xsl:template match="a:updated">
  <xsl:for-each select="$registry/*/history/revision">
    <xsl:sort select="@date"/>
    <xsl:if test="position()=last()">
      <updated>
        <xsl:value-of select="translate(@date,'/','-')"/><xsl:text>T23:59:59Z</xsl:text>
      </updated>
    </xsl:if>
  </xsl:for-each>
</xsl:template>

</xsl:stylesheet>
