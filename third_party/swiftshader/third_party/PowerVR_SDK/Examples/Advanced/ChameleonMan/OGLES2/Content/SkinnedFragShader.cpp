// This file was created by Filewrap 1.1
// Little endian mode
// DO NOT EDIT

#include "../PVRTMemoryFileSystem.h"

// using 32 bit to guarantee alignment.
#ifndef A32BIT
 #define A32BIT static const unsigned int
#endif

// ******** Start: SkinnedFragShader.fsh ********

// File data
static const char _SkinnedFragShader_fsh[] = 
	"uniform sampler2D sTexture;\n"
	"uniform sampler2D sNormalMap;\n"
	"uniform bool bUseDot3;\n"
	"\n"
	"varying mediump vec2 TexCoord;\n"
	"varying mediump vec3 Light;\n"
	"\n"
	"void main()\n"
	"{\n"
	"\tif(bUseDot3)\n"
	"\t{\n"
	"\t\t/*\n"
	"\t\t\tNote:\n"
	"\t\t\tIn the normal map red = y, green = x, blue = z which is why when we get the normal\n"
	"\t\t\tfrom the texture we use the swizzle .grb so the colours are mapped to the correct\n"
	"\t\t\tco-ordinate variable.\n"
	"\t\t*/\n"
	"\n"
	"\t\tmediump vec3 fNormal = texture2D(sNormalMap, TexCoord).grb;\n"
	"\t\tmediump float fNDotL = dot((fNormal - 0.5) * 2.0, Light);\n"
	"\t\t\n"
	"\t\tgl_FragColor = texture2D(sTexture, TexCoord) * fNDotL;\n"
	"    }\n"
	"    else\n"
	"\t\tgl_FragColor = texture2D(sTexture, TexCoord) * Light.x;\n"
	"}\n";

// Register SkinnedFragShader.fsh in memory file system at application startup time
static CPVRTMemoryFileSystem RegisterFile_SkinnedFragShader_fsh("SkinnedFragShader.fsh", _SkinnedFragShader_fsh, 646);

// ******** End: SkinnedFragShader.fsh ********

