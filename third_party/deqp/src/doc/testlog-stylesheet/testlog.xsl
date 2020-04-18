<?xml version="1.0" encoding="utf-8"?>
<!--
    drawElements Quality Program utilities

    Copyright 2016 The Android Open Source Project

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

         http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
-->
<xsl:stylesheet
	version="1.0"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns="http://www.w3.org/1999/xhtml">

	<xsl:output method="xml" indent="yes" encoding="UTF-8"/>

	<xsl:template match="/">
		<xsl:apply-templates/>
	</xsl:template>

	<xsl:template match="BatchResult">
		<html>
			<head>
				<link href="testlog.css" rel="stylesheet" type="text/css"/>
				<title><xsl:value-of select="@FileName"/></title>
			</head>
			<body>
				<table class="Totals">
					<tr><td><b><xsl:text>Total cases</xsl:text></b></td><td><b><xsl:value-of select="ResultTotals/@All"/></b></td></tr>
					<tr><td><xsl:text>Pass</xsl:text></td><td class="Pass"><xsl:value-of select="ResultTotals/@Pass"/></td></tr>
					<tr><td><xsl:text>Fail</xsl:text></td><td class="Fail"><xsl:value-of select="ResultTotals/@Fail"/></td></tr>
					<tr><td><xsl:text>Quality warning</xsl:text></td><td class="QualityWarning"><xsl:value-of select="ResultTotals/@QualityWarning"/></td></tr>
					<tr><td><xsl:text>Compatibility warning</xsl:text></td><td class="CompatibilityWarning"><xsl:value-of select="ResultTotals/@CompatibilityWarning"/></td></tr>
					<!-- <tr><td><xsl:text>Pending</xsl:text></td><td class="Pending"><xsl:value-of select="ResultTotals/@pending"/></td></tr> -->
					<!-- <tr><td><xsl:text>Running</xsl:text></td><td class="Running"><xsl:value-of select="ResultTotals/@Running"/></td></tr> -->
					<tr><td><xsl:text>Not supported</xsl:text></td><td class="NotSupported"><xsl:value-of select="ResultTotals/@NotSupported"/></td></tr>
					<tr><td><xsl:text>Resource error</xsl:text></td><td class="ResourceError"><xsl:value-of select="ResultTotals/@ResourceError"/></td></tr>
					<tr><td><xsl:text>Internal error</xsl:text></td><td class="InternalError"><xsl:value-of select="ResultTotals/@InternalError"/></td></tr>
					<!-- <tr><td><xsl:text>Canceled</xsl:text></td><td class="Canceled"><xsl:value-of select="ResultTotals/@Canceled"/></td></tr> -->
					<tr><td><xsl:text>Timeout</xsl:text></td><td class="Timeout"><xsl:value-of select="ResultTotals/@Timeout"/></td></tr>
					<tr><td><xsl:text>Crash</xsl:text></td><td class="Crash"><xsl:value-of select="ResultTotals/@Crash"/></td></tr>
					<tr><td><xsl:text>Disabled</xsl:text></td><td class="Disabled"><xsl:value-of select="ResultTotals/@Disabled"/></td></tr>
					<!-- <tr><td><xsl:text>Terminated</xsl:text></td><td class="Terminated"><xsl:value-of select="ResultTotals/@Terminated"/></td></tr> -->
				</table>
				<xsl:apply-templates/>
			</body>
		</html>
	</xsl:template>

	<xsl:template match="/TestCaseResult">
		<html>
			<head>
				<link href="testlog.css" rel="stylesheet" type="text/css"/>
				<title><xsl:value-of select="@CasePath"/></title>
			</head>
			<body>
				<h1 class="{Result/@StatusCode}"><xsl:value-of select="@CasePath"/><xsl:text>: </xsl:text><xsl:value-of select="Result"/><xsl:text> (</xsl:text><xsl:value-of select="Result/@StatusCode"/><xsl:text>)</xsl:text></h1>
				<xsl:apply-templates/>
			</body>
		</html>
	</xsl:template>

	<xsl:template match="BatchResult/TestCaseResult">
		<div class="TestCaseResult">
			<h1 class="{Result/@StatusCode}"><xsl:value-of select="@CasePath"/><xsl:text>: </xsl:text><xsl:value-of select="Result"/><xsl:text> (</xsl:text><xsl:value-of select="Result/@StatusCode"/><xsl:text>)</xsl:text></h1>
			<xsl:apply-templates/>
		</div>
	</xsl:template>

	<xsl:template match="Section">
		<div class="Section">
			<h2><xsl:value-of select="@Description"/></h2>
			<xsl:apply-templates/>
		</div>
	</xsl:template>

	<xsl:template match="ImageSet">
		<div class="ImageSet">
			<h3><xsl:value-of select="@Description"/></h3>
			<xsl:apply-templates/>
		</div>
	</xsl:template>

	<xsl:template match="Image">
		<div class="Image">
			<xsl:value-of select="@Description"/><br/>
			<img src="data:image/png;base64,{.}"/>
		</div>
	</xsl:template>

	<xsl:template match="CompileInfo">
		<div class="CompileInfo">
			<h3 class="{@CompileStatus}"><xsl:value-of select="@Description"/></h3>
			<xsl:apply-templates/>
		</div>
	</xsl:template>

	<xsl:template match="ShaderProgram">
		<div class="CompileInfo">
			<h3 class="{@LinkStatus}"><xsl:text>Shader Program</xsl:text></h3>
			<xsl:apply-templates/>
		</div>
	</xsl:template>

	<xsl:template match="VertexShader">
		<div class="Shader">
			<h3 class="{@CompileStatus}"><xsl:text>Vertex Shader</xsl:text></h3>
			<xsl:apply-templates/>
		</div>
	</xsl:template>

	<xsl:template match="FragmentShader">
		<div class="Shader">
			<h3 class="{@CompileStatus}"><xsl:text>Fragment Shader</xsl:text></h3>
			<xsl:apply-templates/>
		</div>
	</xsl:template>

	<xsl:template match="Number">
		<xsl:value-of select="@Description"/><xsl:text>: </xsl:text><xsl:value-of select="."/><xsl:text> </xsl:text><xsl:value-of select="@Unit"/><br/>
	</xsl:template>

	<xsl:template match="Result">
	</xsl:template>

	<xsl:template match="Text">
		<xsl:value-of select="."/><br/>
	</xsl:template>

	<xsl:template match="KernelSource">
		<pre class="KernelSource"><xsl:value-of select="."/></pre>
	</xsl:template>

	<xsl:template match="ShaderSource">
		<pre class="ShaderSource"><xsl:value-of select="."/></pre>
	</xsl:template>

	<xsl:template match="InfoLog">
		<pre class="InfoLog"><xsl:value-of select="."/></pre>
	</xsl:template>

	<xsl:template match="EglConfigSet">
		<div class="Section">
			<h2><xsl:value-of select="@Description"/></h2>
			<table class="EglConfigList">
				<tr>
					<td class="ConfigListTitle"><xsl:text>ID</xsl:text></td>
					<td class="ConfigListTitle"><xsl:text>R</xsl:text></td>
					<td class="ConfigListTitle"><xsl:text>G</xsl:text></td>
					<td class="ConfigListTitle"><xsl:text>B</xsl:text></td>
					<td class="ConfigListTitle"><xsl:text>A</xsl:text></td>
					<td class="ConfigListTitle"><xsl:text>D</xsl:text></td>
					<td class="ConfigListTitle"><xsl:text>S</xsl:text></td>
					<td class="ConfigListTitle"><xsl:text>mS</xsl:text></td>
				</tr>
				<xsl:apply-templates/>
			</table>
		</div>
	</xsl:template>

	<xsl:template match="EglConfig">
		<tr>
			<td class="ConfigListValue"><xsl:value-of select="@ConfigID"/></td>
			<td class="ConfigListValue"><xsl:value-of select="@RedSize"/></td>
			<td class="ConfigListValue"><xsl:value-of select="@GreenSize"/></td>
			<td class="ConfigListValue"><xsl:value-of select="@BlueSize"/></td>
			<td class="ConfigListValue"><xsl:value-of select="@AlphaSize"/></td>
			<td class="ConfigListValue"><xsl:value-of select="@DepthSize"/></td>
			<td class="ConfigListValue"><xsl:value-of select="@StencilSize"/></td>
			<td class="ConfigListValue"><xsl:value-of select="@Samples"/></td>
		</tr>
	</xsl:template>

</xsl:stylesheet>
