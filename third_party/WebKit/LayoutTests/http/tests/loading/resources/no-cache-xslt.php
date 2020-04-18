<?php
$etag = "must-have-some-etag";
if (isset($_SERVER['HTTP_IF_NONE_MATCH']) && stripslashes($_SERVER['HTTP_IF_NONE_MATCH']) == $etag) {
    header("HTTP/1.0 304 Not Modified");
    die;
}
header("ETag: " . $etag);
header("Cache-Control: no-cache");
header("Content-Type: text/xsl");
print("<?xml version=\"1.0\"?>");
?>
<xsl:stylesheet version="1.0"
                xmlns:xhtml="http://www.w3.org/1999/xhtml"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<xsl:template match="*">
  <xsl:copy>
    <xsl:copy-of select="@*" />
    <xsl:apply-templates select="node()" />
  </xsl:copy>
</xsl:template>

<xsl:template match="xhtml:div[@id='markgreen']">
  <xsl:copy>
    <xsl:copy-of select="@*" />
    <xsl:attribute name="style">
      <xsl:value-of select="'color: green;'" />
    </xsl:attribute>
    <xsl:apply-templates select="node()" />
  </xsl:copy>
</xsl:template>
</xsl:stylesheet>
