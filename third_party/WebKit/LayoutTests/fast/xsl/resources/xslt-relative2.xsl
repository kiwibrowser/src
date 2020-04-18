<?xml version="1.0" encoding="utf-8"?>

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:template match="/">
    <html>
      <body>
        <script>if (window.testRunner) testRunner.dumpAsText();</script>
        <div id="mydiv">
           crbug.com/436479: Test for multiple xsl:import with relative URLs.
           <p>SUCCESS</p>
        </div>
      </body>
    </html>
  </xsl:template>
</xsl:stylesheet>
