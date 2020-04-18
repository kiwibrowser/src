/*!****************************************************************************

 @file         OGLES2/PVRTPrint3DShaders.h
 @ingroup      API_OGLES2
 @copyright    Copyright (c) Imagination Technologies Limited.
 @brief        The shaders used by Print3D. Created by Filewrap 1.0. DO NOT EDIT.

******************************************************************************/

// This file was created by Filewrap 1.1
// Little endian mode
// DO NOT EDIT

// ******** Start: Print3DFragShader.fsh ********

// File data
static const char _Print3DFragShader_fsh[] = 
	"uniform sampler2D\tsampler2d;\n"
	"\n"
	"varying lowp vec4\t\tvarColour;\n"
	"varying mediump vec2\ttexCoord;\n"
	"\n"
	"void main()\n"
	"{\n"
	"\tlowp vec4 vTex = texture2D(sampler2d, texCoord);\n"
	"\tgl_FragColor = vec4(varColour.rgb * vTex.r, varColour.a * vTex.a);\n"
	"}\n";

// ******** End: Print3DFragShader.fsh ********

// ******** Start: Print3DVertShader.vsh ********

// File data
static const char _Print3DVertShader_vsh[] = 
	"attribute highp vec4\tmyVertex;\n"
	"attribute mediump vec2\tmyUV;\n"
	"attribute lowp vec4\t\tmyColour;\n"
	"\n"
	"uniform highp mat4\t\tmyMVPMatrix;\n"
	"\n"
	"varying lowp vec4\t\tvarColour;\n"
	"varying mediump vec2\ttexCoord;\n"
	"\n"
	"void main()\n"
	"{\n"
	"\tgl_Position = myMVPMatrix * myVertex;\n"
	"\ttexCoord = myUV.st;\n"
	"\tvarColour = myColour;\n"
	"}\n";

// ******** End: Print3DVertShader.vsh ********

// ******** Start: Print3DFragShaderLogo.fsh ********

// File data
static const char _Print3DFragShaderLogo_fsh[] = 
	"uniform sampler2D\tsampler2d;\n"
	"\n"
	"varying mediump vec2\ttexCoord;\n"
	"\n"
	"void main()\n"
	"{\n"
	"\tgl_FragColor = texture2D(sampler2d, texCoord);\n"
	"}\n";

// ******** End: Print3DFragShaderLogo.fsh ********

// ******** Start: Print3DVertShaderLogo.vsh ********

// File data
static const char _Print3DVertShaderLogo_vsh[] = 
	"attribute highp vec4\tmyVertex;\n"
	"attribute mediump vec2\tmyUV;\n"
	"\n"
	"uniform highp mat4\t\tmyMVPMatrix;\n"
	"\n"
	"varying mediump vec2\ttexCoord;\n"
	"\n"
	"void main()\n"
	"{\n"
	"\tgl_Position = myMVPMatrix * myVertex;\n"
	"\ttexCoord = myUV.st;\n"
	"}\n";

// ******** End: Print3DVertShaderLogo.vsh ********

