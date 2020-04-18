// Copyright 2016 The SwiftShader Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef _COMPILER_INCLUDED_
#define _COMPILER_INCLUDED_

#include "ExtensionBehavior.h"
#include "InfoSink.h"
#include "SymbolTable.h"

enum ShCompileOptions
{
  SH_VALIDATE                = 0,
  SH_VALIDATE_LOOP_INDEXING  = 0x0001,
  SH_INTERMEDIATE_TREE       = 0x0002,
  SH_OBJECT_CODE             = 0x0004,
  SH_ATTRIBUTES_UNIFORMS     = 0x0008,
  SH_LINE_DIRECTIVES         = 0x0010,
  SH_SOURCE_PATH             = 0x0020
};

//
// Implementation dependent built-in resources (constants and extensions).
// The names for these resources has been obtained by stripping gl_/GL_.
//
struct ShBuiltInResources
{
	ShBuiltInResources();

	// Constants.
	int MaxVertexAttribs;
	int MaxVertexUniformVectors;
	int MaxVaryingVectors;
	int MaxVertexTextureImageUnits;
	int MaxCombinedTextureImageUnits;
	int MaxTextureImageUnits;
	int MaxFragmentUniformVectors;
	int MaxDrawBuffers;
	int MaxVertexOutputVectors;
	int MaxFragmentInputVectors;
	int MinProgramTexelOffset;
	int MaxProgramTexelOffset;

	// Extensions.
	// Set to 1 to enable the extension, else 0.
	int OES_standard_derivatives;
	int OES_fragment_precision_high;
	int OES_EGL_image_external;
	int EXT_draw_buffers;
	int ARB_texture_rectangle;

	unsigned int MaxCallStackDepth;
};

typedef unsigned int GLenum;
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31

// Note: GL_ARB_texture_rectangle is part of gl2extchromium.h in the Chromium repo
// GL_ARB_texture_rectangle
#ifndef GL_ARB_texture_rectangle
#define GL_ARB_texture_rectangle 1

#ifndef GL_SAMPLER_2D_RECT_ARB
#define GL_SAMPLER_2D_RECT_ARB 0x8B63
#endif

#ifndef GL_TEXTURE_BINDING_RECTANGLE_ARB
#define GL_TEXTURE_BINDING_RECTANGLE_ARB 0x84F6
#endif

#ifndef GL_TEXTURE_RECTANGLE_ARB
#define GL_TEXTURE_RECTANGLE_ARB 0x84F5
#endif

#ifndef GL_MAX_RECTANGLE_TEXTURE_SIZE_ARB
#define GL_MAX_RECTANGLE_TEXTURE_SIZE_ARB 0x84F8
#endif

#endif  // GL_ARB_texture_rectangle

//
// The base class for the machine dependent compiler to derive from
// for managing object code from the compile.
//
class TCompiler
{
public:
	TCompiler(GLenum shaderType);
	virtual ~TCompiler();
	virtual TCompiler* getAsCompiler() { return this; }

	bool Init(const ShBuiltInResources& resources);
	bool compile(const char* const shaderStrings[],
	             const int numStrings,
	             int compileOptions);

	// Get results of the last compilation.
	int getShaderVersion() const { return shaderVersion; }
	TInfoSink& getInfoSink() { return infoSink; }

protected:
	GLenum getShaderType() const { return shaderType; }
	// Initialize symbol-table with built-in symbols.
	bool InitBuiltInSymbolTable(const ShBuiltInResources& resources);
	// Clears the results from the previous compilation.
	void clearResults();
	// Return true if function recursion is detected or call depth exceeded.
	bool validateCallDepth(TIntermNode *root, TInfoSink &infoSink);
	// Returns true if the given shader does not exceed the minimum
	// functionality mandated in GLSL 1.0 spec Appendix A.
	bool validateLimitations(TIntermNode *root);
	// Translate to object code.
	virtual bool translate(TIntermNode *root) = 0;
	// Get built-in extensions with default behavior.
	const TExtensionBehavior& getExtensionBehavior() const;

private:
	GLenum shaderType;

	unsigned int maxCallStackDepth;

	// Built-in symbol table for the given language, spec, and resources.
	// It is preserved from compile-to-compile.
	TSymbolTable symbolTable;
	// Built-in extensions with default behavior.
	TExtensionBehavior extensionBehavior;

	// Results of compilation.
	int shaderVersion;
	TInfoSink infoSink;  // Output sink.

	// Memory allocator. Allocates and tracks memory required by the compiler.
	// Deallocates all memory when compiler is destructed.
	TPoolAllocator allocator;
};

bool InitCompilerGlobals();
void FreeCompilerGlobals();

#endif // _COMPILER_INCLUDED_
