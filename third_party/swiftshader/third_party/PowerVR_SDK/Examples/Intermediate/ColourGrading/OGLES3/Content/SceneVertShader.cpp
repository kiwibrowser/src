// This file was created by Filewrap 1.1
// Little endian mode
// DO NOT EDIT

#include "../PVRTMemoryFileSystem.h"

// using 32 bit to guarantee alignment.
#ifndef A32BIT
 #define A32BIT static const unsigned int
#endif

// ******** Start: SceneVertShader.vsh ********

// File data
static const char _SceneVertShader_vsh[] = 
	"attribute highp vec4  inVertex;\n"
	"attribute highp vec3  inNormal;\n"
	"attribute highp vec2  inTexCoord;\n"
	"\n"
	"uniform highp mat4   MVPMatrix;\n"
	"uniform highp vec3   LightDirection;\n"
	"uniform highp float  MaterialBias;\n"
	"uniform highp float  MaterialScale;\n"
	"\n"
	"varying lowp vec3  DiffuseLight;\n"
	"varying lowp vec3  SpecularLight;\n"
	"varying mediump vec2  TexCoord;\n"
	"\n"
	"void main()\n"
	"{\n"
	"\tgl_Position = MVPMatrix * inVertex;\n"
	"\t\n"
	"\tDiffuseLight = vec3(max(dot(inNormal, LightDirection), 0.0));\n"
	"\tSpecularLight = vec3(max((DiffuseLight.x - MaterialBias) * MaterialScale, 0.0));\n"
	"\t\n"
	"\tTexCoord = inTexCoord;\n"
	"}\n";

// Register SceneVertShader.vsh in memory file system at application startup time
static CPVRTMemoryFileSystem RegisterFile_SceneVertShader_vsh("SceneVertShader.vsh", _SceneVertShader_vsh, 566);

// ******** End: SceneVertShader.vsh ********

