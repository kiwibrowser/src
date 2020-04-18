// This file was created by Filewrap 1.1
// Little endian mode
// DO NOT EDIT

#include "../PVRTMemoryFileSystem.h"

// using 32 bit to guarantee alignment.
#ifndef A32BIT
 #define A32BIT static const unsigned int
#endif

// ******** Start: DefaultVertShader.vsh ********

// File data
static const char _DefaultVertShader_vsh[] = 
	"attribute highp   vec3 inVertex;\n"
	"attribute mediump vec2 inTexCoord;\n"
	"\n"
	"uniform highp   mat4 MVPMatrix;\n"
	"uniform float\tfUOffset;\n"
	"\n"
	"varying mediump vec2  TexCoord;\n"
	"\n"
	"void main()\n"
	"{\n"
	"\tgl_Position = MVPMatrix * vec4(inVertex, 1.0);\n"
	"\n"
	"\t// Pass through texcoords\n"
	"\tTexCoord = inTexCoord;\n"
	"\tTexCoord.x += fUOffset;\n"
	"}\n"
	" ";

// Register DefaultVertShader.vsh in memory file system at application startup time
static CPVRTMemoryFileSystem RegisterFile_DefaultVertShader_vsh("DefaultVertShader.vsh", _DefaultVertShader_vsh, 301);

// ******** End: DefaultVertShader.vsh ********

