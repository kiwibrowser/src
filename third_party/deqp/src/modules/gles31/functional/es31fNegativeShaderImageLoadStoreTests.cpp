/*-------------------------------------------------------------------------
 * drawElements Quality Program OpenGL ES 3.1 Module
 * -------------------------------------------------
 *
 * Copyright 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *//*!
 * \file
 * \brief Negative Shader Image Load Store Tests
 *//*--------------------------------------------------------------------*/

#include "es31fNegativeShaderImageLoadStoreTests.hpp"

#include "deUniquePtr.hpp"

#include "glwEnums.hpp"

#include "gluShaderProgram.hpp"

#include "glsTextureTestUtil.hpp"

#include "tcuStringTemplate.hpp"
#include "tcuTexture.hpp"
#include "tcuTestLog.hpp"

namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace NegativeTestShared
{
namespace
{

enum MemoryQualifier
{
	MEMORY_NONE = 0,
	MEMORY_READONLY,
	MEMORY_WRITEONLY,
	MEMORY_BOTH,

	MEMORY_LAST
};

enum ImageOperation
{
	IMAGE_OPERATION_STORE = 0,
	IMAGE_OPERATION_LOAD,
	IMAGE_OPERATION_ATOMIC_ADD,
	IMAGE_OPERATION_ATOMIC_MIN,
	IMAGE_OPERATION_ATOMIC_MAX,
	IMAGE_OPERATION_ATOMIC_AND,
	IMAGE_OPERATION_ATOMIC_OR,
	IMAGE_OPERATION_ATOMIC_XOR,
	IMAGE_OPERATION_ATOMIC_EXCHANGE,
	IMAGE_OPERATION_ATOMIC_COMP_SWAP,

	IMAGE_OPERATION_LAST
};

static const glu::ShaderType s_shaders[] =
{
	glu::SHADERTYPE_VERTEX,
	glu::SHADERTYPE_FRAGMENT,
	glu::SHADERTYPE_GEOMETRY,
	glu::SHADERTYPE_TESSELLATION_CONTROL,
	glu::SHADERTYPE_TESSELLATION_EVALUATION,
	glu::SHADERTYPE_COMPUTE
};

std::string getShaderImageLayoutQualifier (const tcu::TextureFormat& format)
{
	std::ostringstream qualifier;

	switch (format.order)
	{
		case tcu::TextureFormat::RGBA:	qualifier << "rgba";	break;
		case tcu::TextureFormat::R:		qualifier << "r";		break;
		default:
			DE_ASSERT(false);
			return std::string("");
	}

	switch (format.type)
	{
		case tcu::TextureFormat::FLOAT:				qualifier << "32f";			break;
		case tcu::TextureFormat::HALF_FLOAT:		qualifier << "16f";			break;
		case tcu::TextureFormat::UNORM_INT8:		qualifier << "8";			break;
		case tcu::TextureFormat::SNORM_INT8:		qualifier << "8_snorm";		break;
		case tcu::TextureFormat::SIGNED_INT32:		qualifier << "32i";			break;
		case tcu::TextureFormat::SIGNED_INT16:		qualifier << "16i";			break;
		case tcu::TextureFormat::SIGNED_INT8:		qualifier << "8i";			break;
		case tcu::TextureFormat::UNSIGNED_INT32:	qualifier << "32ui";		break;
		case tcu::TextureFormat::UNSIGNED_INT16:	qualifier << "16ui";		break;
		case tcu::TextureFormat::UNSIGNED_INT8:		qualifier << "8ui";			break;
		default:
			DE_ASSERT(false);
			return std::string("");
	}

	return qualifier.str();
}

std::string getShaderImageTypeDeclaration (const tcu::TextureFormat& format, glu::TextureTestUtil::TextureType imageType)
{
	std::ostringstream declaration;

	switch (format.type)
	{
		case tcu::TextureFormat::FLOAT:
		case tcu::TextureFormat::HALF_FLOAT:
		case tcu::TextureFormat::UNORM_INT8:
		case tcu::TextureFormat::SNORM_INT8:		declaration << "";		break;

		case tcu::TextureFormat::SIGNED_INT32:
		case tcu::TextureFormat::SIGNED_INT16:
		case tcu::TextureFormat::SIGNED_INT8:		declaration << "i";		break;

		case tcu::TextureFormat::UNSIGNED_INT32:
		case tcu::TextureFormat::UNSIGNED_INT16:
		case tcu::TextureFormat::UNSIGNED_INT8:		declaration << "u";		break;

		default:
			DE_ASSERT(false);
			return std::string("");
	}

	declaration << "image";

	switch(imageType)
	{
		case glu::TextureTestUtil::TEXTURETYPE_2D:			declaration << "2D";			break;
		case glu::TextureTestUtil::TEXTURETYPE_3D:			declaration << "3D";			break;
		case glu::TextureTestUtil::TEXTURETYPE_CUBE:		declaration << "Cube";			break;
		case glu::TextureTestUtil::TEXTURETYPE_2D_ARRAY:	declaration << "2DArray";		break;
		case glu::TextureTestUtil::TEXTURETYPE_BUFFER:		declaration << "Buffer";		break;
		case glu::TextureTestUtil::TEXTURETYPE_CUBE_ARRAY:	declaration << "CubeArray";		break;
		default:
			DE_ASSERT(false);
			return std::string("");
	}

	return declaration.str();
}

std::string getShaderImageTypeExtensionString (glu::TextureTestUtil::TextureType imageType)
{
	std::string extension;

	switch(imageType)
	{
		case glu::TextureTestUtil::TEXTURETYPE_2D:
		case glu::TextureTestUtil::TEXTURETYPE_3D:
		case glu::TextureTestUtil::TEXTURETYPE_CUBE:
		case glu::TextureTestUtil::TEXTURETYPE_2D_ARRAY:
			extension = "";
			break;

		case glu::TextureTestUtil::TEXTURETYPE_BUFFER:
			extension = "#extension GL_EXT_texture_buffer : enable";
			break;

		case glu::TextureTestUtil::TEXTURETYPE_CUBE_ARRAY:
			extension = "#extension GL_EXT_texture_cube_map_array : enable";
			break;

		default:
			DE_ASSERT(false);
			return std::string("");
	}

	return extension;
}

std::string getShaderImageParamP (glu::TextureTestUtil::TextureType imageType)
{
	switch(imageType)
	{
		case glu::TextureTestUtil::TEXTURETYPE_2D:
			return "ivec2(1, 1)";

		case glu::TextureTestUtil::TEXTURETYPE_3D:
		case glu::TextureTestUtil::TEXTURETYPE_CUBE:
		case glu::TextureTestUtil::TEXTURETYPE_2D_ARRAY:
		case glu::TextureTestUtil::TEXTURETYPE_CUBE_ARRAY:
			return "ivec3(1, 1, 1)";

		case glu::TextureTestUtil::TEXTURETYPE_BUFFER:
			return "1";

		default:
			DE_ASSERT(false);
			return std::string("");
	}
}

std::string getOtherFunctionArguments (const tcu::TextureFormat& format, ImageOperation function)
{
	std::ostringstream data;
	data << ", ";

	bool isFloat = false;

	switch(format.type)
	{
		case tcu::TextureFormat::FLOAT:
		case tcu::TextureFormat::HALF_FLOAT:
		case tcu::TextureFormat::UNORM_INT8:
		case tcu::TextureFormat::SNORM_INT8:
			data << "";
			isFloat = true;
			break;

		case tcu::TextureFormat::SIGNED_INT32:
		case tcu::TextureFormat::SIGNED_INT16:
		case tcu::TextureFormat::SIGNED_INT8:
			data << "i";
			break;

		case tcu::TextureFormat::UNSIGNED_INT32:
		case tcu::TextureFormat::UNSIGNED_INT16:
		case tcu::TextureFormat::UNSIGNED_INT8:
			data << "u";
			break;

		default:
			DE_ASSERT(false);
			return std::string("");
	}

	switch (function)
	{
		case IMAGE_OPERATION_LOAD:
			return "";

		case IMAGE_OPERATION_STORE:
			data << "vec4(1, 1, 1, 1)";
			break;

		case IMAGE_OPERATION_ATOMIC_ADD:
		case IMAGE_OPERATION_ATOMIC_MIN:
		case IMAGE_OPERATION_ATOMIC_MAX:
		case IMAGE_OPERATION_ATOMIC_AND:
		case IMAGE_OPERATION_ATOMIC_OR:
		case IMAGE_OPERATION_ATOMIC_XOR:
			return ", 1";

		case IMAGE_OPERATION_ATOMIC_EXCHANGE:
			return isFloat ? ", 1.0" : ", 1";

		case IMAGE_OPERATION_ATOMIC_COMP_SWAP:
			return ", 1, 1";

		default:
			DE_ASSERT(false);
			return std::string("");
	}
	return data.str();
}

std::string getMemoryQualifier (MemoryQualifier memory)
{
	switch (memory)
	{
		case MEMORY_NONE:
			return std::string("");

		case MEMORY_WRITEONLY:
			return std::string("writeonly");

		case MEMORY_READONLY:
			return std::string("readonly");

		case MEMORY_BOTH:
			return std::string("writeonly readonly");

		default:
			DE_ASSERT(DE_FALSE);
	}

	return std::string("");
}

std::string getShaderImageFunctionExtensionString (ImageOperation function)
{
	switch (function)
	{
		case IMAGE_OPERATION_STORE:
		case IMAGE_OPERATION_LOAD:
			return std::string("");

		case IMAGE_OPERATION_ATOMIC_ADD:
		case IMAGE_OPERATION_ATOMIC_MIN:
		case IMAGE_OPERATION_ATOMIC_MAX:
		case IMAGE_OPERATION_ATOMIC_AND:
		case IMAGE_OPERATION_ATOMIC_OR:
		case IMAGE_OPERATION_ATOMIC_XOR:
		case IMAGE_OPERATION_ATOMIC_EXCHANGE:
		case IMAGE_OPERATION_ATOMIC_COMP_SWAP:
			return std::string("#extension GL_OES_shader_image_atomic : enable");

		default:
			DE_ASSERT(DE_FALSE);
	}
	return std::string("");
}

std::string getFunctionName (ImageOperation function)
{
	switch (function)
	{
		case IMAGE_OPERATION_STORE:				return std::string("imageStore");
		case IMAGE_OPERATION_LOAD:				return std::string("imageLoad");
		case IMAGE_OPERATION_ATOMIC_ADD:		return std::string("imageAtomicAdd");
		case IMAGE_OPERATION_ATOMIC_MIN:		return std::string("imageAtomicMin");
		case IMAGE_OPERATION_ATOMIC_MAX:		return std::string("imageAtomicMax");
		case IMAGE_OPERATION_ATOMIC_AND:		return std::string("imageAtomicAnd");
		case IMAGE_OPERATION_ATOMIC_OR:			return std::string("imageAtomicOr");
		case IMAGE_OPERATION_ATOMIC_XOR:		return std::string("imageAtomicXor");
		case IMAGE_OPERATION_ATOMIC_EXCHANGE:	return std::string("imageAtomicExchange");
		case IMAGE_OPERATION_ATOMIC_COMP_SWAP:	return std::string("imageAtomicCompSwap");
		default:
			DE_ASSERT(DE_FALSE);
	}
	return std::string("");
}

std::string generateShaderSource (ImageOperation function, MemoryQualifier memory, glu::TextureTestUtil::TextureType imageType, const tcu::TextureFormat& format, glu::ShaderType shaderType)
{
	const char* shaderTemplate =	"${GLSL_VERSION_DECL}\n"
									"${GLSL_TYPE_EXTENSION}\n"
									"${GLSL_FUNCTION_EXTENSION}\n"
									"${GEOMETRY_SHADER_LAYOUT}\n"
									"layout(${LAYOUT_FORMAT}, binding = 0) highp uniform ${MEMORY_QUALIFIER} ${IMAGE_TYPE} u_img0;\n"
									"void main(void)\n"
									"{\n"
									" ${FUNCTION_NAME}(u_img0, ${IMAGE_PARAM_P}${FUNCTION_ARGUMENTS});\n"
									"}\n";

	std::map<std::string, std::string> params;

	params["GLSL_VERSION_DECL"] = getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES);
	params["GLSL_TYPE_EXTENSION"] = getShaderImageTypeExtensionString(imageType);
	params["GLSL_FUNCTION_EXTENSION"] = getShaderImageFunctionExtensionString(function);
	params["GEOMETRY_SHADER_LAYOUT"] = getGLShaderType(shaderType) == GL_GEOMETRY_SHADER ? "layout(max_vertices = 3) out;" : "";
	params["LAYOUT_FORMAT"] = getShaderImageLayoutQualifier(format);
	params["MEMORY_QUALIFIER"] = getMemoryQualifier(memory);
	params["IMAGE_TYPE"] = getShaderImageTypeDeclaration(format, imageType);
	params["FUNCTION_NAME"] = getFunctionName(function);
	params["IMAGE_PARAM_P"] = getShaderImageParamP(imageType);
	params["FUNCTION_ARGUMENTS"] = getOtherFunctionArguments(format, function);

	return tcu::StringTemplate(shaderTemplate).specialize(params);
}

void testShader (NegativeTestContext& ctx, ImageOperation function, MemoryQualifier memory, glu::TextureTestUtil::TextureType imageType, const tcu::TextureFormat& format)
{
	tcu::TestLog& log = ctx.getLog();
	ctx.beginSection(getFunctionName(function) + " " + getMemoryQualifier(memory) + " " + getShaderImageLayoutQualifier(format));
	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(s_shaders); ndx++)
	{
		if (ctx.isShaderSupported(s_shaders[ndx]))
		{
			ctx.beginSection(std::string("Verify shader: ") + glu::getShaderTypeName(s_shaders[ndx]));
			std::string					shaderSource(generateShaderSource(function, memory, imageType, format, s_shaders[ndx]));
			const glu::ShaderProgram	program(ctx.getRenderContext(), glu::ProgramSources() << glu::ShaderSource(s_shaders[ndx], shaderSource));
			if (program.getShaderInfo(s_shaders[ndx]).compileOk)
			{
				log << program;
				log << tcu::TestLog::Message << "Expected program to fail, but compilation passed." << tcu::TestLog::EndMessage;
				ctx.fail("Shader was not expected to compile.");
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void image_store (NegativeTestContext& ctx, glu::TextureTestUtil::TextureType imageType)
{
	const tcu::TextureFormat formats[] =
	{
		tcu::TextureFormat(tcu::TextureFormat::RGBA,	tcu::TextureFormat::FLOAT),
		tcu::TextureFormat(tcu::TextureFormat::RGBA,	tcu::TextureFormat::HALF_FLOAT),
		tcu::TextureFormat(tcu::TextureFormat::R,		tcu::TextureFormat::FLOAT),
		tcu::TextureFormat(tcu::TextureFormat::RGBA,	tcu::TextureFormat::UNORM_INT8),
		tcu::TextureFormat(tcu::TextureFormat::RGBA,	tcu::TextureFormat::SNORM_INT8),

		tcu::TextureFormat(tcu::TextureFormat::RGBA,	tcu::TextureFormat::SIGNED_INT32),
		tcu::TextureFormat(tcu::TextureFormat::RGBA,	tcu::TextureFormat::SIGNED_INT16),
		tcu::TextureFormat(tcu::TextureFormat::RGBA,	tcu::TextureFormat::SIGNED_INT8),
		tcu::TextureFormat(tcu::TextureFormat::R,		tcu::TextureFormat::SIGNED_INT32),

		tcu::TextureFormat(tcu::TextureFormat::RGBA,	tcu::TextureFormat::UNSIGNED_INT32),
		tcu::TextureFormat(tcu::TextureFormat::RGBA,	tcu::TextureFormat::UNSIGNED_INT16),
		tcu::TextureFormat(tcu::TextureFormat::RGBA,	tcu::TextureFormat::UNSIGNED_INT8),
		tcu::TextureFormat(tcu::TextureFormat::R,		tcu::TextureFormat::UNSIGNED_INT32)
	};

	const MemoryQualifier memoryOptions[] =
	{
		MEMORY_READONLY,
		MEMORY_BOTH
	};

	ctx.beginSection("It is an error to pass a readonly image to imageStore.");
	for (int memoryNdx = 0; memoryNdx < DE_LENGTH_OF_ARRAY(memoryOptions); ++memoryNdx)
	{
		for (int fmtNdx = 0; fmtNdx < DE_LENGTH_OF_ARRAY(formats); ++fmtNdx)
		{
			testShader(ctx, IMAGE_OPERATION_STORE, memoryOptions[memoryNdx], imageType, formats[fmtNdx]);
		}
	}
	ctx.endSection();
}

void image_load (NegativeTestContext& ctx, glu::TextureTestUtil::TextureType imageType)
{
	const tcu::TextureFormat formats[] =
	{
		tcu::TextureFormat(tcu::TextureFormat::RGBA,	tcu::TextureFormat::FLOAT),
		tcu::TextureFormat(tcu::TextureFormat::RGBA,	tcu::TextureFormat::HALF_FLOAT),
		tcu::TextureFormat(tcu::TextureFormat::R,		tcu::TextureFormat::FLOAT),
		tcu::TextureFormat(tcu::TextureFormat::RGBA,	tcu::TextureFormat::UNORM_INT8),
		tcu::TextureFormat(tcu::TextureFormat::RGBA,	tcu::TextureFormat::SNORM_INT8),

		tcu::TextureFormat(tcu::TextureFormat::RGBA,	tcu::TextureFormat::SIGNED_INT32),
		tcu::TextureFormat(tcu::TextureFormat::RGBA,	tcu::TextureFormat::SIGNED_INT16),
		tcu::TextureFormat(tcu::TextureFormat::RGBA,	tcu::TextureFormat::SIGNED_INT8),
		tcu::TextureFormat(tcu::TextureFormat::R,		tcu::TextureFormat::SIGNED_INT32),

		tcu::TextureFormat(tcu::TextureFormat::RGBA,	tcu::TextureFormat::UNSIGNED_INT32),
		tcu::TextureFormat(tcu::TextureFormat::RGBA,	tcu::TextureFormat::UNSIGNED_INT16),
		tcu::TextureFormat(tcu::TextureFormat::RGBA,	tcu::TextureFormat::UNSIGNED_INT8),
		tcu::TextureFormat(tcu::TextureFormat::R,		tcu::TextureFormat::UNSIGNED_INT32)
	};

	const MemoryQualifier memoryOptions[] =
	{
		MEMORY_WRITEONLY,
		MEMORY_BOTH
	};

	ctx.beginSection("It is an error to pass a writeonly image to imageLoad.");
	for (int memoryNdx = 0; memoryNdx < DE_LENGTH_OF_ARRAY(memoryOptions); ++memoryNdx)
	{
		for (int fmtNdx = 0; fmtNdx < DE_LENGTH_OF_ARRAY(formats); ++fmtNdx)
		{
			testShader(ctx, IMAGE_OPERATION_LOAD, memoryOptions[memoryNdx], imageType, formats[fmtNdx]);
		}
	}
	ctx.endSection();
}

void image_atomic (NegativeTestContext& ctx, glu::TextureTestUtil::TextureType imageType)
{
	const tcu::TextureFormat formats[] =
	{
		tcu::TextureFormat(tcu::TextureFormat::RGBA,	tcu::TextureFormat::SIGNED_INT32),
		tcu::TextureFormat(tcu::TextureFormat::RGBA,	tcu::TextureFormat::SIGNED_INT16),
		tcu::TextureFormat(tcu::TextureFormat::RGBA,	tcu::TextureFormat::SIGNED_INT8),
		tcu::TextureFormat(tcu::TextureFormat::R,		tcu::TextureFormat::SIGNED_INT32),

		tcu::TextureFormat(tcu::TextureFormat::RGBA,	tcu::TextureFormat::UNSIGNED_INT32),
		tcu::TextureFormat(tcu::TextureFormat::RGBA,	tcu::TextureFormat::UNSIGNED_INT16),
		tcu::TextureFormat(tcu::TextureFormat::RGBA,	tcu::TextureFormat::UNSIGNED_INT8),
		tcu::TextureFormat(tcu::TextureFormat::R,		tcu::TextureFormat::UNSIGNED_INT32)
	};

	const MemoryQualifier memoryOptions[] =
	{
		MEMORY_READONLY,
		MEMORY_WRITEONLY,
		MEMORY_BOTH
	};

	const ImageOperation imageOperations[] =
	{
		IMAGE_OPERATION_ATOMIC_ADD,
		IMAGE_OPERATION_ATOMIC_MIN,
		IMAGE_OPERATION_ATOMIC_MAX,
		IMAGE_OPERATION_ATOMIC_AND,
		IMAGE_OPERATION_ATOMIC_OR,
		IMAGE_OPERATION_ATOMIC_XOR,
		IMAGE_OPERATION_ATOMIC_COMP_SWAP
	};

	ctx.beginSection("It is an error to pass a writeonly and/or readonly image to imageAtomic*.");
	for (int memoryNdx = 0; memoryNdx < DE_LENGTH_OF_ARRAY(memoryOptions); ++memoryNdx)
	{
		for (int fmtNdx = 0; fmtNdx < DE_LENGTH_OF_ARRAY(formats); ++fmtNdx)
		{
			for (int functionNdx = 0; functionNdx < DE_LENGTH_OF_ARRAY(imageOperations); ++functionNdx)
			{
				testShader(ctx, imageOperations[functionNdx], memoryOptions[memoryNdx], imageType, formats[fmtNdx]);
			}
		}
	}
	ctx.endSection();
}

void image_atomic_exchange (NegativeTestContext& ctx, glu::TextureTestUtil::TextureType imageType)
{
	const tcu::TextureFormat formats[] =
	{
		tcu::TextureFormat(tcu::TextureFormat::RGBA,	tcu::TextureFormat::FLOAT),
		tcu::TextureFormat(tcu::TextureFormat::RGBA,	tcu::TextureFormat::HALF_FLOAT),
		tcu::TextureFormat(tcu::TextureFormat::R,		tcu::TextureFormat::FLOAT),
		tcu::TextureFormat(tcu::TextureFormat::RGBA,	tcu::TextureFormat::UNORM_INT8),
		tcu::TextureFormat(tcu::TextureFormat::RGBA,	tcu::TextureFormat::SNORM_INT8),

		tcu::TextureFormat(tcu::TextureFormat::RGBA,	tcu::TextureFormat::SIGNED_INT32),
		tcu::TextureFormat(tcu::TextureFormat::RGBA,	tcu::TextureFormat::SIGNED_INT16),
		tcu::TextureFormat(tcu::TextureFormat::RGBA,	tcu::TextureFormat::SIGNED_INT8),
		tcu::TextureFormat(tcu::TextureFormat::R,		tcu::TextureFormat::SIGNED_INT32),

		tcu::TextureFormat(tcu::TextureFormat::RGBA,	tcu::TextureFormat::UNSIGNED_INT32),
		tcu::TextureFormat(tcu::TextureFormat::RGBA,	tcu::TextureFormat::UNSIGNED_INT16),
		tcu::TextureFormat(tcu::TextureFormat::RGBA,	tcu::TextureFormat::UNSIGNED_INT8),
		tcu::TextureFormat(tcu::TextureFormat::R,		tcu::TextureFormat::UNSIGNED_INT32)
	};

	const MemoryQualifier memoryOptions[] =
	{
		MEMORY_READONLY,
		MEMORY_WRITEONLY,
		MEMORY_BOTH
	};

	ctx.beginSection("It is an error to pass a writeonly and/or readonly image to imageAtomic*.");
	for (int memoryNdx = 0; memoryNdx < DE_LENGTH_OF_ARRAY(memoryOptions); ++memoryNdx)
	{
		for (int fmtNdx = 0; fmtNdx < DE_LENGTH_OF_ARRAY(formats); ++fmtNdx)
		{
			testShader(ctx, IMAGE_OPERATION_ATOMIC_EXCHANGE, memoryOptions[memoryNdx], imageType, formats[fmtNdx]);
		}
	}
	ctx.endSection();
}

// Re-routing function template for generating the standard negative
// test function signature with texture type added.

template <int Type>
void loadFuncWrapper (NegativeTestContext& ctx)
{
	image_load(ctx, (glu::TextureTestUtil::TextureType)Type);
}

template <int Type>
void storeFuncWrapper (NegativeTestContext& ctx)
{
	image_store(ctx, (glu::TextureTestUtil::TextureType)Type);
}

template <int Type>
void atomicFuncWrapper (NegativeTestContext& ctx)
{
	image_atomic(ctx, (glu::TextureTestUtil::TextureType)Type);
}

template <int Type>
void atomicExchangeFuncWrapper (NegativeTestContext& ctx)
{
	image_atomic_exchange(ctx, (glu::TextureTestUtil::TextureType)Type);
}

} // anonymous

// Set of texture types to create tests for.
#define CREATE_TEST_FUNC_PER_TEXTURE_TYPE(NAME, FUNC) const FunctionContainer NAME[] =									\
	{																													\
		{FUNC<glu::TextureTestUtil::TEXTURETYPE_2D>,			"texture_2d",	"Texture2D negative tests."},			\
		{FUNC<glu::TextureTestUtil::TEXTURETYPE_3D>,			"texture_3d",	"Texture3D negative tests."},			\
		{FUNC<glu::TextureTestUtil::TEXTURETYPE_CUBE>,			"cube",			"Cube texture negative tests."},		\
		{FUNC<glu::TextureTestUtil::TEXTURETYPE_2D_ARRAY>,		"2d_array",		"2D array texture negative tests."},	\
		{FUNC<glu::TextureTestUtil::TEXTURETYPE_BUFFER>,		"buffer",		"Buffer negative tests."},				\
		{FUNC<glu::TextureTestUtil::TEXTURETYPE_CUBE_ARRAY>,	"cube_array",	"Cube array texture negative tests."}	\
	}

std::vector<FunctionContainer> getNegativeShaderImageLoadTestFunctions (void)
{
	CREATE_TEST_FUNC_PER_TEXTURE_TYPE(funcs, loadFuncWrapper);
	return std::vector<FunctionContainer>(DE_ARRAY_BEGIN(funcs), DE_ARRAY_END(funcs));
}

std::vector<FunctionContainer> getNegativeShaderImageStoreTestFunctions (void)
{
	CREATE_TEST_FUNC_PER_TEXTURE_TYPE(funcs, storeFuncWrapper);
	return std::vector<FunctionContainer>(DE_ARRAY_BEGIN(funcs), DE_ARRAY_END(funcs));
}

std::vector<FunctionContainer> getNegativeShaderImageAtomicTestFunctions (void)
{
	CREATE_TEST_FUNC_PER_TEXTURE_TYPE(funcs, atomicFuncWrapper);
	return std::vector<FunctionContainer>(DE_ARRAY_BEGIN(funcs), DE_ARRAY_END(funcs));
}

std::vector<FunctionContainer> getNegativeShaderImageAtomicExchangeTestFunctions (void)
{
	CREATE_TEST_FUNC_PER_TEXTURE_TYPE(funcs, atomicExchangeFuncWrapper);
	return std::vector<FunctionContainer>(DE_ARRAY_BEGIN(funcs), DE_ARRAY_END(funcs));
}

} // NegativeTestShared
} // Functional
} // gles31
} // deqp
