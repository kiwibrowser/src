// This file was created by Filewrap 1.1
// Little endian mode
// DO NOT EDIT

#include "../PVRTMemoryFileSystem.h"

// using 32 bit to guarantee alignment.
#ifndef A32BIT
 #define A32BIT static const unsigned int
#endif

// ******** Start: FragShader.fsh ********

// File data
static const char _FragShader_fsh[] = 
	"#version 300 es\n"
	"\n"
	"uniform  sampler2D     sTexture;\n"
	"uniform  mediump sampler3D\t\tsColourLUT;\n"
	"\n"
	"in mediump vec2 texCoords;\n"
	"layout(location = 0) out lowp vec4 oFragColour;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    highp vec3 vCol = texture(sTexture, texCoords).rgb;\n"
	"\tlowp vec3 vAlteredCol = texture(sColourLUT, vCol.rgb).rgb;\n"
	"    oFragColour = vec4(vAlteredCol, 1.0);\n"
	"}\n";

// Register FragShader.fsh in memory file system at application startup time
static CPVRTMemoryFileSystem RegisterFile_FragShader_fsh("FragShader.fsh", _FragShader_fsh, 341);

// ******** End: FragShader.fsh ********

