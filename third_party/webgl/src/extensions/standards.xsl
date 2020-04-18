<?xml version="1.0"?>

<xsl:stylesheet version="1.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:template match="api|core|removed">
    <a href="http://www.khronos.org/registry/webgl/specs/{@version}/">WebGL API <xsl:value-of select="@version"/></a>
  </xsl:template>

  <xsl:template match="subsumed">
    No longer available as of the <a href="http://www.khronos.org/registry/webgl/specs/latest/{@version}/">WebGL API <xsl:value-of select="@version"/></a> specification. Subsumed by the <a href="../{@by}/"><xsl:value-of select="@by"/></a> extension.
  </xsl:template>

  <xsl:template match="ext">
    <a href="http://www.khronos.org/registry/webgl/extensions/{@name}/">
      <xsl:value-of select="@name"/>
    </a>
  </xsl:template>

  <xsl:template match="glsl">
    <xsl:choose>
      <xsl:when test="@flavor='ES 3.0'">
        <a href="http://www.khronos.org/registry/gles/specs/3.0/GLSL_ES_Specification_{@version}.{@revision}.pdf">
        GLSL ES 3.0 (<xsl:value-of select="@version"/>.<xsl:value-of select="@revision"/>)</a>
      </xsl:when>
      <xsl:when test="@flavor='ES 2.0'">
        <a href="http://www.khronos.org/registry/gles/specs/2.0/GLSL_ES_Specification_{@version}.{@revision}.pdf">
        GLSL ES 2.0 (<xsl:value-of select="@version"/>.<xsl:value-of select="@revision"/>)</a>
      </xsl:when>
      <xsl:otherwise>
        GLSL <xsl:value-of select="@flavor"/> (<xsl:value-of select="@version" />.<xsl:value-of select="@revision"/>)
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="rfc">
    <a href="http://tools.ietf.org/html/rfc{@number}">
      RFC <xsl:value-of select="@number"/>
    </a>
  </xsl:template>
</xsl:stylesheet>
