<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  <xsl:output method="html" encoding="UTF-8"
              doctype-public="-//W3C//DTD HTML 4.01 Transitional//EN" />
  <xsl:include href="xslt-apply-enc16-sheet.xsl"/>
  <xsl:template match="/">
    <html>
      <body>
	UTF-8
	<xsl:call-template name="target"/>
      </body>
    </html>
  </xsl:template>
</xsl:stylesheet>
