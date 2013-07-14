<!--
   Xsl file for translating Cena config to Lowsars config.
   This should be used together with lowsars-cena.
-->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
<xsl:template match="/">
<xsl:for-each select="cena/contest">
<xsl:for-each select="problem">
problem;<xsl:value-of select="@filename"/>;<xsl:if test="@comparetype=1">1</xsl:if><xsl:if test="@comparetype=0">0</xsl:if><xsl:if test="@comparetype=2">2</xsl:if>;<xsl:value-of select="input/@filename"/>;<xsl:value-of select="output/@filename"/>;<xsl:for-each select="testcase">
case;<xsl:value-of select="input/@filename"/>;<xsl:value-of select="output/@filename"/>;<xsl:value-of select="@timelimit"/>;<xsl:value-of select="@memorylimit"/>;<xsl:value-of select="@score"/>;</xsl:for-each>
</xsl:for-each>
</xsl:for-each>
</xsl:template>
</xsl:stylesheet>

