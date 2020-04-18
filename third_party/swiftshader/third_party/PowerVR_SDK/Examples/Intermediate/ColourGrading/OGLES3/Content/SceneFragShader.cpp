// This file was created by Filewrap 1.1
// Little endian mode
// DO NOT EDIT

#include "../PVRTMemoryFileSystem.h"

// using 32 bit to guarantee alignment.
#ifndef A32BIT
 #define A32BIT static const unsigned int
#endif

// ******** Start: SceneFragShader.fsh ********

// File data
static const char _SceneFragShader_fsh[] = 
	"uniform sampler2D sTexture;\n"
	"\n"
	"varying lowp    vec3  DiffuseLight;\n"
	"varying lowp    vec3  SpecularLight;\n"
	"varying mediump vec2  TexCoord;\n"
	"\n"
	"void main()\n"
	"{\n"
	"\tlowp vec3 texColor  = texture2D(sTexture, TexCoord).rgb;\n"
	"\tlowp vec3 color = (texColor * DiffuseLight) + SpecularLight;\n"
	"\tgl_FragColor = vec4(color, 1.0);\n"
	"}\n"
	"\n";

// Register SceneFragShader.fsh in memory file system at application startup time
static CPVRTMemoryFileSystem RegisterFile_SceneFragShader_fsh("SceneFragShader.fsh", _SceneFragShader_fsh, 306);

// ******** End: SceneFragShader.fsh ********

