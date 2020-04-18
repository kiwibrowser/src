// This file was created by Filewrap 1.1
// Little endian mode
// DO NOT EDIT

#include "../PVRTMemoryFileSystem.h"

// using 32 bit to guarantee alignment.
#ifndef A32BIT
 #define A32BIT static const unsigned int
#endif

// ******** Start: VertShader.vsh ********

// File data
static const char _VertShader_vsh[] = 
	"#version 300 es\n"
	"\n"
	"#define VERTEX_ARRAY  0\n"
	"#define TEXCOORD_ARRAY  1\n"
	"\n"
	"layout (location = VERTEX_ARRAY) in highp   vec4  inVertex;\n"
	"layout (location = TEXCOORD_ARRAY) in mediump vec2  inTexCoord;\n"
	"\n"
	"out mediump    vec2  texCoords;\n"
	"\t\t\n"
	"void main() \n"
	"{ \n"
	"\tgl_Position  = inVertex;\n"
	"\ttexCoords    = inTexCoord;\n"
	"} \n";

// Register VertShader.vsh in memory file system at application startup time
static CPVRTMemoryFileSystem RegisterFile_VertShader_vsh("VertShader.vsh", _VertShader_vsh, 301);

// ******** End: VertShader.vsh ********

