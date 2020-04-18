/*-------------------------------------------------------------------------
 * drawElements Quality Program OpenGL ES 3.1 Module
 * -------------------------------------------------
 *
 * Copyright 2016 The Android Open Source Project
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
 * \brief Negative Shader Function Tests
 *//*--------------------------------------------------------------------*/

#include "es31fNegativeShaderFunctionTests.hpp"

#include "gluShaderProgram.hpp"

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

enum ShaderFunction
{
	SHADER_FUNCTION_BITFIELD_REVERSE = 0,
	SHADER_FUNCTION_BIT_COUNT,
	SHADER_FUNCTION_FIND_MSB,
	SHADER_FUNCTION_FIND_LSB,
	SHADER_FUNCTION_UADD_CARRY,
	SHADER_FUNCTION_USUB_BORROW,
	SHADER_FUNCTION_UMUL_EXTENDED,
	SHADER_FUNCTION_IMUL_EXTENDED,
	SHADER_FUNCTION_FREXP,
	SHADER_FUNCTION_LDEXP,
	SHADER_FUNCTION_PACK_UNORM_4X8,
	SHADER_FUNCTION_PACK_SNORM_4X8,
	SHADER_FUNCTION_UNPACK_SNORM_4X8,
	SHADER_FUNCTION_UNPACK_UNORM_4X8,
	SHADER_FUNCTION_EMIT_VERTEX,
	SHADER_FUNCTION_END_PRIMITIVE,
	SHADER_FUNCTION_ATOMIC_ADD,
	SHADER_FUNCTION_ATOMIC_MIN,
	SHADER_FUNCTION_ATOMIC_MAX,
	SHADER_FUNCTION_ATOMIC_AND,
	SHADER_FUNCTION_ATOMIC_OR,
	SHADER_FUNCTION_ATOMIC_XOR,
	SHADER_FUNCTION_ATOMIC_EXCHANGE,
	SHADER_FUNCTION_ATOMIC_COMP_SWAP,
	SHADER_FUNCTION_INTERPOLATED_AT_CENTROID,
	SHADER_FUNCTION_INTERPOLATED_AT_SAMPLE,
	SHADER_FUNCTION_INTERPOLATED_AT_OFFSET,

	SHADER_FUNCTION_LAST
};

enum FunctionTextureModes
{
	FUNCTION_TEXTURE_MODE_NO_BIAS_NO_COMPARE = 0,
	FUNCTION_TEXTURE_MODE_BIAS_OR_COMPARE,

	FUNCTION_TEXTURE_MODE_LAST
};

enum FunctionTextureGatherOffsetModes
{
	FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP = 0,
	FUNCTION_TEXTURE_GATHER_OFFSET_MODE_REF_Z,

	FUNCTION_TEXTURE_GATHER_OFFSET_MODE_LAST
};

struct TextureGatherOffsetsTestSpec
{
	FunctionTextureGatherOffsetModes	mode;
	glu::DataType						samplerDataType;
	glu::DataType						pDataType;
	glu::DataType						offsetsDataType;
	glu::DataType						fourthArgument;
	bool								offsetIsConst;
	int									offsetArraySize;
};

static const glu::DataType s_floatTypes[] =
{
	glu::TYPE_FLOAT,
	glu::TYPE_FLOAT_VEC2,
	glu::TYPE_FLOAT_VEC3,
	glu::TYPE_FLOAT_VEC4
};

static const glu::DataType s_intTypes[] =
{
	glu::TYPE_INT,
	glu::TYPE_INT_VEC2,
	glu::TYPE_INT_VEC3,
	glu::TYPE_INT_VEC4
};

static const glu::DataType s_uintTypes[] =
{
	glu::TYPE_UINT,
	glu::TYPE_UINT_VEC2,
	glu::TYPE_UINT_VEC3,
	glu::TYPE_UINT_VEC4
};

static const glu::DataType s_nonScalarIntTypes[] =
{
	glu::TYPE_FLOAT,
	glu::TYPE_FLOAT_VEC2,
	glu::TYPE_FLOAT_VEC3,
	glu::TYPE_FLOAT_VEC4,
	glu::TYPE_INT_VEC2,
	glu::TYPE_INT_VEC3,
	glu::TYPE_INT_VEC4,
	glu::TYPE_UINT,
	glu::TYPE_UINT_VEC2,
	glu::TYPE_UINT_VEC3,
	glu::TYPE_UINT_VEC4
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

static const glu::DataType s_samplerTypes[] =
{
	glu::TYPE_SAMPLER_2D,
	glu::TYPE_INT_SAMPLER_2D,
	glu::TYPE_UINT_SAMPLER_2D,
	glu::TYPE_SAMPLER_3D,
	glu::TYPE_INT_SAMPLER_3D,
	glu::TYPE_UINT_SAMPLER_3D,
	glu::TYPE_SAMPLER_CUBE,
	glu::TYPE_INT_SAMPLER_CUBE,
	glu::TYPE_UINT_SAMPLER_CUBE,
	glu::TYPE_SAMPLER_2D_ARRAY,
	glu::TYPE_INT_SAMPLER_2D_ARRAY,
	glu::TYPE_UINT_SAMPLER_2D_ARRAY,
	glu::TYPE_SAMPLER_CUBE_SHADOW,
	glu::TYPE_SAMPLER_2D_SHADOW,
	glu::TYPE_SAMPLER_2D_ARRAY_SHADOW,
	glu::TYPE_SAMPLER_CUBE_ARRAY,
	glu::TYPE_INT_SAMPLER_CUBE_ARRAY,
	glu::TYPE_UINT_SAMPLER_CUBE_ARRAY,
	glu::TYPE_SAMPLER_CUBE_ARRAY_SHADOW,

	glu::TYPE_SAMPLER_2D_MULTISAMPLE,
	glu::TYPE_INT_SAMPLER_2D_MULTISAMPLE,
	glu::TYPE_UINT_SAMPLER_2D_MULTISAMPLE,

	glu::TYPE_SAMPLER_BUFFER,
	glu::TYPE_INT_SAMPLER_BUFFER,
	glu::TYPE_UINT_SAMPLER_BUFFER,

	glu::TYPE_SAMPLER_2D_MULTISAMPLE_ARRAY,
	glu::TYPE_INT_SAMPLER_2D_MULTISAMPLE_ARRAY,
	glu::TYPE_UINT_SAMPLER_2D_MULTISAMPLE_ARRAY,
};

void verifyShader (NegativeTestContext& ctx, glu::ShaderType shaderType, std::string shaderSource)
{
	tcu::TestLog&	log		= ctx.getLog();
	const char*		source	= shaderSource.c_str();
	const int		length	= (int) shaderSource.size();
	glu::Shader		shader	(ctx.getRenderContext(), shaderType);

	shader.setSources(1, &source, &length);
	shader.compile();

	log << shader;
	if (shader.getCompileStatus())
	{
		log << tcu::TestLog::Message << "Expected shader to fail, but compilation passed." << tcu::TestLog::EndMessage;
		ctx.fail("Shader was not expected to compile.\n");
	}
}

std::string declareAndInitializeShaderVariable (glu::DataType dataType, std::string varName)
{
	std::ostringstream variable;
	variable << getDataTypeName(dataType) << " " << varName << " = " << getDataTypeName(dataType);
	switch (dataType)
	{
		case glu::TYPE_FLOAT:		variable << "(1.0);\n";					break;
		case glu::TYPE_FLOAT_VEC2:	variable << "(1.0, 1.0);\n";			break;
		case glu::TYPE_FLOAT_VEC3:	variable << "(1.0, 1.0, 1.0);\n";		break;
		case glu::TYPE_FLOAT_VEC4:	variable << "(1.0, 1.0, 1.0, 1.0);\n";	break;
		case glu::TYPE_INT:			variable << "(1);\n";					break;
		case glu::TYPE_INT_VEC2:	variable << "(1, 1);\n";				break;
		case glu::TYPE_INT_VEC3:	variable << "(1, 1, 1);\n";				break;
		case glu::TYPE_INT_VEC4:	variable << "(1, 1, 1, 1);\n";			break;
		case glu::TYPE_UINT:		variable << "(1u);\n";					break;
		case glu::TYPE_UINT_VEC2:	variable << "(1u, 1u);\n";				break;
		case glu::TYPE_UINT_VEC3:	variable << "(1u, 1u, 1u);\n";			break;
		case glu::TYPE_UINT_VEC4:	variable << "(1u, 1u, 1u, 1u);\n";		break;
		default:
			DE_FATAL("Unsupported data type.");
	}
	return variable.str();
}

std::string declareShaderUniform (glu::DataType dataType, std::string varName)
{
	std::ostringstream variable;
	variable << getPrecisionName(glu::PRECISION_HIGHP) << " uniform " << getDataTypeName(dataType) << " " << varName << ";\n";
	return variable.str();
}

std::string declareShaderInput (glu::DataType dataType, std::string varName)
{
	std::ostringstream variable;
	variable << "in " << getPrecisionName(glu::PRECISION_HIGHP) << " " << getDataTypeName(dataType) << " " << varName << ";\n";
	return variable.str();
}

std::string declareBuffer (glu::DataType dataType, std::string varName)
{
	std::ostringstream variable;
	variable	<< "buffer SSBO {\n"
				<< "    " << getDataTypeName(dataType) << " " << varName << ";\n"
				<< "};\n";
	return variable.str();
}

std::string declareShaderArrayVariable (glu::DataType dataType, std::string varName, const int arraySize)
{
	std::ostringstream source;
	source << getDataTypeName(dataType) << " " << varName << "[" << arraySize << "]" << " = " << getDataTypeName(dataType) << "[](";

	for (int ndx = 0; ndx < arraySize; ++ndx)
		source << getDataTypeName(dataType) << "(" << 0 << ", " << 0 << ")" << ((ndx < arraySize -1) ? ", " : "");

	source << ");";
	return source.str();
}

std::string getShaderExtensionDeclaration (std::string extension)
{
	if (extension.empty())
		return std::string("");
	else
	{
		std::ostringstream source;
		source << "#extension " << extension << " : enable\n";
		return source.str();
	}
}

std::string getDataTypeExtension (glu::DataType dataType)
{
	std::ostringstream source;
	switch (dataType)
	{
		case glu::TYPE_SAMPLER_CUBE_ARRAY:
		case glu::TYPE_SAMPLER_CUBE_ARRAY_SHADOW:
		case glu::TYPE_INT_SAMPLER_CUBE_ARRAY:
		case glu::TYPE_UINT_SAMPLER_CUBE_ARRAY:
			source << "GL_EXT_texture_cube_map_array";
			break;

		case glu::TYPE_SAMPLER_BUFFER:
		case glu::TYPE_INT_SAMPLER_BUFFER:
		case glu::TYPE_UINT_SAMPLER_BUFFER:
			source << "GL_EXT_texture_buffer";
			break;

		case glu::TYPE_SAMPLER_2D_MULTISAMPLE_ARRAY:
		case glu::TYPE_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
		case glu::TYPE_UINT_SAMPLER_2D_MULTISAMPLE_ARRAY:
			source << "GL_OES_texture_storage_multisample_2d_array";
			break;

		default:
			break;
	}

	return source.str();
}

std::string getShaderInitialization (NegativeTestContext& ctx, glu::ShaderType shaderType)
{
	std::ostringstream source;

	if (!contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)))
	{
		switch (shaderType)
		{
			case glu::SHADERTYPE_GEOMETRY:
				source << "#extension GL_EXT_geometry_shader : enable\n";
				break;

			case glu::SHADERTYPE_TESSELLATION_CONTROL:
				source << "#extension GL_EXT_tessellation_shader : enable\n";
				break;

			case glu::SHADERTYPE_TESSELLATION_EVALUATION:
				source << "#extension GL_EXT_tessellation_shader : enable\n";
				break;

			default:
				break;
		}
	}

	switch (shaderType)
	{
		case glu::SHADERTYPE_GEOMETRY:
			source << "layout(max_vertices = 5) out;\n";
			break;

		case glu::SHADERTYPE_TESSELLATION_CONTROL:
			source << "layout(vertices = 3) out;\n";
			break;

		case glu::SHADERTYPE_TESSELLATION_EVALUATION:
			source << "layout(triangles, equal_spacing, cw) in;\n";
			break;

		default:
			break;
	}

	return source.str();
}

std::string genShaderSourceBitfieldExtract (NegativeTestContext& ctx, glu::ShaderType shaderType, glu::DataType valueDataType, glu::DataType offsetDataType, glu::DataType bitsDataType)
{
	std::ostringstream source;
	source	<< (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) ? glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES)) << "\n"
			<< getShaderInitialization(ctx, shaderType)
			<< "void main(void)\n"
			<< "{\n"
			<< "    " << declareAndInitializeShaderVariable(valueDataType, "value")
			<< "    " << declareAndInitializeShaderVariable(offsetDataType, "offset")
			<< "    " << declareAndInitializeShaderVariable(bitsDataType, "bits")
			<< "    bitfieldExtract(value, offset, bits);\n"
			<< "}\n";

	return source.str();
}

void bitfield_extract_invalid_value_type (NegativeTestContext& ctx)
{
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_intTypes) == DE_LENGTH_OF_ARRAY(s_uintTypes));

	ctx.beginSection("bitfieldExtract: Invalid value type.");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			for (int dataTypeNdx = 0; dataTypeNdx < DE_LENGTH_OF_ARRAY(s_floatTypes); ++dataTypeNdx)
			{
				const std::string shaderSource(genShaderSourceBitfieldExtract(ctx, s_shaders[shaderNdx], s_floatTypes[dataTypeNdx], glu::TYPE_INT, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void bitfield_extract_invalid_offset_type (NegativeTestContext& ctx)
{
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_intTypes) == DE_LENGTH_OF_ARRAY(s_uintTypes));

	ctx.beginSection("bitfieldExtract: Invalid offset type.");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			for (int dataTypeNdx = 0; dataTypeNdx < DE_LENGTH_OF_ARRAY(s_intTypes); ++dataTypeNdx)
			{
				for (int nonIntNdx = 0; nonIntNdx < DE_LENGTH_OF_ARRAY(s_nonScalarIntTypes); ++nonIntNdx)
				{
					{
						const std::string shaderSource(genShaderSourceBitfieldExtract(ctx, s_shaders[shaderNdx], s_intTypes[dataTypeNdx], s_nonScalarIntTypes[nonIntNdx], glu::TYPE_INT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceBitfieldExtract(ctx, s_shaders[shaderNdx], s_uintTypes[dataTypeNdx], s_nonScalarIntTypes[nonIntNdx], glu::TYPE_INT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
				}
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void bitfield_extract_invalid_bits_type (NegativeTestContext& ctx)
{
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_intTypes) == DE_LENGTH_OF_ARRAY(s_uintTypes));

	ctx.beginSection("bitfieldExtract: Invalid bits type.");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			for (int dataTypeNdx = 0; dataTypeNdx < DE_LENGTH_OF_ARRAY(s_intTypes); ++dataTypeNdx)
			{
				for (int nonIntNdx = 0; nonIntNdx < DE_LENGTH_OF_ARRAY(s_nonScalarIntTypes); ++nonIntNdx)
				{
					{
						const std::string shaderSource(genShaderSourceBitfieldExtract(ctx, s_shaders[shaderNdx], s_intTypes[dataTypeNdx], glu::TYPE_INT, s_nonScalarIntTypes[nonIntNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceBitfieldExtract(ctx, s_shaders[shaderNdx], s_uintTypes[dataTypeNdx], glu::TYPE_INT, s_nonScalarIntTypes[nonIntNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
				}
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

std::string genShaderSourceBitfieldInsert (NegativeTestContext& ctx, glu::ShaderType shaderType, glu::DataType baseDataType, glu::DataType insertDataType, glu::DataType offsetDataType, glu::DataType bitsDataType)
{
	std::ostringstream source;
	source	<< (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) ? glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES)) << "\n"
			<< getShaderInitialization(ctx, shaderType)
			<< "void main(void)\n"
			<< "{\n"
			<< "    " << declareAndInitializeShaderVariable(baseDataType, "base")
			<< "    " << declareAndInitializeShaderVariable(insertDataType, "insert")
			<< "    " << declareAndInitializeShaderVariable(offsetDataType, "offset")
			<< "    " << declareAndInitializeShaderVariable(bitsDataType, "bits")
			<< "    bitfieldInsert(base, insert, offset, bits);\n"
			<< "}\n";

	return source.str();
}

void bitfield_insert_invalid_base_type (NegativeTestContext& ctx)
{
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_intTypes) == DE_LENGTH_OF_ARRAY(s_uintTypes));
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_intTypes) == DE_LENGTH_OF_ARRAY(s_floatTypes));

	ctx.beginSection("bitfieldInsert: Invalid base type.");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			for (int dataTypeNdx = 0; dataTypeNdx < DE_LENGTH_OF_ARRAY(s_floatTypes); ++dataTypeNdx)
			{
				{
					const std::string shaderSource(genShaderSourceBitfieldInsert(ctx, s_shaders[shaderNdx], s_floatTypes[dataTypeNdx], s_intTypes[dataTypeNdx], glu::TYPE_INT, glu::TYPE_INT));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceBitfieldInsert(ctx, s_shaders[shaderNdx], s_floatTypes[dataTypeNdx], s_uintTypes[dataTypeNdx], glu::TYPE_INT, glu::TYPE_INT));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void bitfield_insert_invalid_insert_type (NegativeTestContext& ctx)
{
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_intTypes) == DE_LENGTH_OF_ARRAY(s_uintTypes));
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_intTypes) == DE_LENGTH_OF_ARRAY(s_floatTypes));

	ctx.beginSection("bitfieldInsert: Invalid insert type.");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			for (int dataTypeNdx = 0; dataTypeNdx < DE_LENGTH_OF_ARRAY(s_intTypes); ++dataTypeNdx)
			{
				{
					const std::string shaderSource(genShaderSourceBitfieldInsert(ctx, s_shaders[shaderNdx], s_intTypes[dataTypeNdx], s_floatTypes[dataTypeNdx], glu::TYPE_INT, glu::TYPE_INT));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceBitfieldInsert(ctx, s_shaders[shaderNdx], s_uintTypes[dataTypeNdx], s_floatTypes[dataTypeNdx], glu::TYPE_INT, glu::TYPE_INT));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}

				for (int dataTypeNdx2 = 0; dataTypeNdx2 < DE_LENGTH_OF_ARRAY(s_intTypes); ++dataTypeNdx2)
				{
					if (s_intTypes[dataTypeNdx] == s_intTypes[dataTypeNdx2])
						continue;

					{
						const std::string shaderSource(genShaderSourceBitfieldInsert(ctx, s_shaders[shaderNdx], s_intTypes[dataTypeNdx], s_intTypes[dataTypeNdx2], glu::TYPE_INT, glu::TYPE_INT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceBitfieldInsert(ctx, s_shaders[shaderNdx], s_uintTypes[dataTypeNdx], s_uintTypes[dataTypeNdx2], glu::TYPE_INT, glu::TYPE_INT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
				}

			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void bitfield_insert_invalid_offset_type (NegativeTestContext& ctx)
{
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_intTypes) == DE_LENGTH_OF_ARRAY(s_uintTypes));

	ctx.beginSection("bitfieldInsert: Invalid offset type.");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			for (int dataTypeNdx = 0; dataTypeNdx < DE_LENGTH_OF_ARRAY(s_intTypes); ++dataTypeNdx)
			{
				for (int dataTypeNdx2 = 0; dataTypeNdx2 < DE_LENGTH_OF_ARRAY(s_nonScalarIntTypes); ++dataTypeNdx2)
				{
					{
						const std::string shaderSource(genShaderSourceBitfieldInsert(ctx, s_shaders[shaderNdx], s_intTypes[dataTypeNdx], s_intTypes[dataTypeNdx], s_nonScalarIntTypes[dataTypeNdx2], glu::TYPE_INT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceBitfieldInsert(ctx, s_shaders[shaderNdx], s_uintTypes[dataTypeNdx], s_uintTypes[dataTypeNdx], s_nonScalarIntTypes[dataTypeNdx2], glu::TYPE_INT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
				}
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void bitfield_insert_invalid_bits_type (NegativeTestContext& ctx)
{
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_intTypes) == DE_LENGTH_OF_ARRAY(s_uintTypes));

	ctx.beginSection("bitfieldInsert: Invalid bits type.");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			for (int dataTypeNdx = 0; dataTypeNdx < DE_LENGTH_OF_ARRAY(s_intTypes); ++dataTypeNdx)
			{
				for (int dataTypeNdx2 = 0; dataTypeNdx2 < DE_LENGTH_OF_ARRAY(s_nonScalarIntTypes); ++dataTypeNdx2)
				{
					{
						const std::string shaderSource(genShaderSourceBitfieldInsert(ctx, s_shaders[shaderNdx], s_intTypes[dataTypeNdx], s_intTypes[dataTypeNdx], glu::TYPE_INT, s_nonScalarIntTypes[dataTypeNdx2]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceBitfieldInsert(ctx, s_shaders[shaderNdx], s_uintTypes[dataTypeNdx], s_uintTypes[dataTypeNdx], glu::TYPE_INT, s_nonScalarIntTypes[dataTypeNdx2]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
				}
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

// bitfieldReverse, bitCount, findMSB, findLSB
std::string genShaderSourceReverseCountFind (NegativeTestContext& ctx, glu::ShaderType shaderType, ShaderFunction function, glu::DataType baseDataType)
{
	DE_ASSERT(function == SHADER_FUNCTION_BITFIELD_REVERSE ||
		function == SHADER_FUNCTION_BIT_COUNT ||
		function == SHADER_FUNCTION_FIND_MSB ||
		function == SHADER_FUNCTION_FIND_LSB);

	std::ostringstream source;
	source	<< (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) ? glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES)) << "\n"
			<< getShaderInitialization(ctx, shaderType)
			<< "void main(void)\n"
			<< "{\n"
			<< "    " << declareAndInitializeShaderVariable(baseDataType, "value");

	switch (function)
	{
		case SHADER_FUNCTION_BITFIELD_REVERSE:	source << "    bitfieldReverse(value);\n";	break;
		case SHADER_FUNCTION_BIT_COUNT:			source << "    bitCount(value);\n";			break;
		case SHADER_FUNCTION_FIND_MSB:			source << "    findMSB(value);\n";			break;
		case SHADER_FUNCTION_FIND_LSB:			source << "    findLSB(value);\n";			break;
		default:
			DE_FATAL("Unsupported shader function.");
	}

	source << "}\n";

	return source.str();
}


void bitfield_reverse (NegativeTestContext& ctx)
{
	ctx.beginSection("bitfieldReverse: Invalid value type.");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			for (int dataTypeNdx = 0; dataTypeNdx < DE_LENGTH_OF_ARRAY(s_floatTypes); ++dataTypeNdx)
			{
				const std::string shaderSource(genShaderSourceReverseCountFind(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_BITFIELD_REVERSE, s_floatTypes[dataTypeNdx]));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void bit_count (NegativeTestContext& ctx)
{
	ctx.beginSection("bitCount: Invalid value type.");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			for (int dataTypeNdx = 0; dataTypeNdx < DE_LENGTH_OF_ARRAY(s_floatTypes); ++dataTypeNdx)
			{
				const std::string shaderSource(genShaderSourceReverseCountFind(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_BIT_COUNT, s_floatTypes[dataTypeNdx]));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void find_msb (NegativeTestContext& ctx)
{
	ctx.beginSection("findMSB: Invalid value type.");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			for (int dataTypeNdx = 0; dataTypeNdx < DE_LENGTH_OF_ARRAY(s_floatTypes); ++dataTypeNdx)
			{
				const std::string shaderSource(genShaderSourceReverseCountFind(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_FIND_MSB, s_floatTypes[dataTypeNdx]));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void find_lsb (NegativeTestContext& ctx)
{
	ctx.beginSection("findLSB: Invalid value type.");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			for (int dataTypeNdx = 0; dataTypeNdx < DE_LENGTH_OF_ARRAY(s_floatTypes); ++dataTypeNdx)
			{
				const std::string shaderSource(genShaderSourceReverseCountFind(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_FIND_LSB, s_floatTypes[dataTypeNdx]));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

// uaddCarry, usubBorrow
std::string genShaderSourceAddCarrySubBorrow (NegativeTestContext& ctx, glu::ShaderType shaderType, ShaderFunction function, glu::DataType xDataType, glu::DataType yDataType, glu::DataType carryBorrowDataType)
{
	DE_ASSERT(function == SHADER_FUNCTION_UADD_CARRY || function == SHADER_FUNCTION_USUB_BORROW);

	std::ostringstream source;
	source	<< (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) ? glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES)) << "\n"
			<< getShaderInitialization(ctx, shaderType)
			<< "void main(void)\n"
			<< "{\n"
			<< "    " << declareAndInitializeShaderVariable(xDataType, "x")
			<< "    " << declareAndInitializeShaderVariable(yDataType, "y");

	switch (function)
	{
		case SHADER_FUNCTION_UADD_CARRY:
			source	<< "    " << declareAndInitializeShaderVariable(carryBorrowDataType, "carry")
					<< "    uaddCarry(x, y, carry);\n";
			break;

		case SHADER_FUNCTION_USUB_BORROW:
			source	<< "    " << declareAndInitializeShaderVariable(carryBorrowDataType, "borrow")
					<< "    usubBorrow(x, y, borrow);\n";
			break;

		default:
			DE_FATAL("Unsupported shader function.");
	}

	source << "}\n";

	return source.str();
}

void uadd_carry_invalid_x (NegativeTestContext& ctx)
{
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_uintTypes));
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_intTypes));

	ctx.beginSection("uaddCarry: Invalid x type.");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			for (int dataTypeNdx = 0; dataTypeNdx < DE_LENGTH_OF_ARRAY(s_floatTypes); ++dataTypeNdx)
			{
				{
					const std::string shaderSource(genShaderSourceAddCarrySubBorrow(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_UADD_CARRY, s_floatTypes[dataTypeNdx], s_uintTypes[dataTypeNdx], s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceAddCarrySubBorrow(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_UADD_CARRY, s_intTypes[dataTypeNdx], s_uintTypes[dataTypeNdx], s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				for (int dataTypeNdx2 = 0; dataTypeNdx2 < DE_LENGTH_OF_ARRAY(s_floatTypes); ++dataTypeNdx2)
				{
					if (s_uintTypes[dataTypeNdx2] == s_uintTypes[dataTypeNdx])
						continue;

					const std::string shaderSource(genShaderSourceAddCarrySubBorrow(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_UADD_CARRY, s_uintTypes[dataTypeNdx2], s_uintTypes[dataTypeNdx], s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void uadd_carry_invalid_y (NegativeTestContext& ctx)
{
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_uintTypes));
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_intTypes));

	ctx.beginSection("uaddCarry: Invalid y type.");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			for (int dataTypeNdx = 0; dataTypeNdx < DE_LENGTH_OF_ARRAY(s_floatTypes); ++dataTypeNdx)
			{
				{
					const std::string shaderSource(genShaderSourceAddCarrySubBorrow(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_UADD_CARRY, s_uintTypes[dataTypeNdx], s_floatTypes[dataTypeNdx], s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceAddCarrySubBorrow(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_UADD_CARRY, s_uintTypes[dataTypeNdx], s_intTypes[dataTypeNdx], s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}

				for (int dataTypeNdx2 = 0; dataTypeNdx2 < DE_LENGTH_OF_ARRAY(s_floatTypes); ++dataTypeNdx2)
				{
					if (s_uintTypes[dataTypeNdx2] == s_uintTypes[dataTypeNdx])
						continue;

					const std::string shaderSource(genShaderSourceAddCarrySubBorrow(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_UADD_CARRY, s_uintTypes[dataTypeNdx], s_uintTypes[dataTypeNdx2], s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void uadd_carry_invalid_carry (NegativeTestContext& ctx)
{
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_uintTypes));
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_intTypes));

	ctx.beginSection("uaddCarry: Invalid carry type.");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			for (int dataTypeNdx = 0; dataTypeNdx < DE_LENGTH_OF_ARRAY(s_floatTypes); ++dataTypeNdx)
			{
				{
					const std::string shaderSource(genShaderSourceAddCarrySubBorrow(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_UADD_CARRY, s_uintTypes[dataTypeNdx], s_uintTypes[dataTypeNdx], s_floatTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceAddCarrySubBorrow(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_UADD_CARRY, s_uintTypes[dataTypeNdx], s_uintTypes[dataTypeNdx], s_intTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}

				for (int dataTypeNdx2 = 0; dataTypeNdx2 < DE_LENGTH_OF_ARRAY(s_floatTypes); ++dataTypeNdx2)
				{
					if (s_uintTypes[dataTypeNdx2] == s_uintTypes[dataTypeNdx])
						continue;

					const std::string shaderSource(genShaderSourceAddCarrySubBorrow(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_UADD_CARRY, s_uintTypes[dataTypeNdx], s_uintTypes[dataTypeNdx], s_uintTypes[dataTypeNdx2]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void usub_borrow_invalid_x (NegativeTestContext& ctx)
{
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_uintTypes));
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_intTypes));

	ctx.beginSection("usubBorrow: Invalid x type.");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			for (int dataTypeNdx = 0; dataTypeNdx < DE_LENGTH_OF_ARRAY(s_floatTypes); ++dataTypeNdx)
			{
				{
					const std::string shaderSource(genShaderSourceAddCarrySubBorrow(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_USUB_BORROW, s_floatTypes[dataTypeNdx], s_uintTypes[dataTypeNdx], s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceAddCarrySubBorrow(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_USUB_BORROW, s_intTypes[dataTypeNdx], s_uintTypes[dataTypeNdx], s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}

				for (int dataTypeNdx2 = 0; dataTypeNdx2 < DE_LENGTH_OF_ARRAY(s_floatTypes); ++dataTypeNdx2)
				{
					if (s_uintTypes[dataTypeNdx2] == s_uintTypes[dataTypeNdx])
						continue;

					const std::string shaderSource(genShaderSourceAddCarrySubBorrow(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_USUB_BORROW, s_uintTypes[dataTypeNdx2], s_uintTypes[dataTypeNdx], s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void usub_borrow_invalid_y (NegativeTestContext& ctx)
{
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_uintTypes));
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_intTypes));

	ctx.beginSection("usubBorrow: Invalid y type.");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			for (int dataTypeNdx = 0; dataTypeNdx < DE_LENGTH_OF_ARRAY(s_floatTypes); ++dataTypeNdx)
			{
				{
					const std::string shaderSource(genShaderSourceAddCarrySubBorrow(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_USUB_BORROW, s_uintTypes[dataTypeNdx], s_floatTypes[dataTypeNdx], s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceAddCarrySubBorrow(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_USUB_BORROW, s_uintTypes[dataTypeNdx], s_intTypes[dataTypeNdx], s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}

				for (int dataTypeNdx2 = 0; dataTypeNdx2 < DE_LENGTH_OF_ARRAY(s_floatTypes); ++dataTypeNdx2)
				{
					if (s_uintTypes[dataTypeNdx2] == s_uintTypes[dataTypeNdx])
						continue;

					const std::string shaderSource = genShaderSourceAddCarrySubBorrow(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_USUB_BORROW, s_uintTypes[dataTypeNdx], s_uintTypes[dataTypeNdx2], s_uintTypes[dataTypeNdx]);
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void usub_borrow_invalid_borrow (NegativeTestContext& ctx)
{
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_uintTypes));
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_intTypes));

	ctx.beginSection("usubBorrow: Invalid borrow type.");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			for (int dataTypeNdx = 0; dataTypeNdx < DE_LENGTH_OF_ARRAY(s_floatTypes); ++dataTypeNdx)
			{
				{
					const std::string shaderSource(genShaderSourceAddCarrySubBorrow(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_USUB_BORROW, s_uintTypes[dataTypeNdx], s_uintTypes[dataTypeNdx], s_floatTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceAddCarrySubBorrow(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_USUB_BORROW, s_uintTypes[dataTypeNdx], s_uintTypes[dataTypeNdx], s_intTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}

				for (int dataTypeNdx2 = 0; dataTypeNdx2 < DE_LENGTH_OF_ARRAY(s_floatTypes); ++dataTypeNdx2)
				{
					if (s_uintTypes[dataTypeNdx2] == s_uintTypes[dataTypeNdx])
						continue;

					const std::string shaderSource(genShaderSourceAddCarrySubBorrow(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_USUB_BORROW, s_uintTypes[dataTypeNdx], s_uintTypes[dataTypeNdx], s_uintTypes[dataTypeNdx2]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

// umulExtended, imulExtended
std::string genShaderSourceMulExtended (NegativeTestContext& ctx, glu::ShaderType shaderType, ShaderFunction function, glu::DataType xDataType, glu::DataType yDataType, glu::DataType msbDataType, glu::DataType lsbDataType)
{
	DE_ASSERT(function == SHADER_FUNCTION_UMUL_EXTENDED || function == SHADER_FUNCTION_IMUL_EXTENDED);

	std::ostringstream source;
	source	<< (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) ? glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES)) << "\n"
			<< getShaderInitialization(ctx, shaderType)
			<< "void main(void)\n"
			<< "{\n"
			<< "    " << declareAndInitializeShaderVariable(xDataType, "x")
			<< "    " << declareAndInitializeShaderVariable(yDataType, "y")
			<< "    " << declareAndInitializeShaderVariable(msbDataType, "msb")
			<< "    " << declareAndInitializeShaderVariable(lsbDataType, "lsb");

	switch (function)
	{
		case SHADER_FUNCTION_UMUL_EXTENDED:	source << "    umulExtended(x, y, msb, lsb);\n";	break;
		case SHADER_FUNCTION_IMUL_EXTENDED:	source << "    imulExtended(x, y, msb, lsb);\n";	break;
		default:
			DE_FATAL("Unsupported shader function.");
	}

	source << "}\n";

	return source.str();
}

void umul_extended_invalid_x (NegativeTestContext& ctx)
{
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_uintTypes));
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_intTypes));

	ctx.beginSection("umulExtended: Invalid x type.");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			for (int dataTypeNdx = 0; dataTypeNdx < DE_LENGTH_OF_ARRAY(s_uintTypes); ++dataTypeNdx)
			{
				{
					const std::string shaderSource(genShaderSourceMulExtended(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_UMUL_EXTENDED, s_floatTypes[dataTypeNdx], s_uintTypes[dataTypeNdx], s_uintTypes[dataTypeNdx], s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceMulExtended(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_UMUL_EXTENDED, s_intTypes[dataTypeNdx], s_uintTypes[dataTypeNdx], s_uintTypes[dataTypeNdx], s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}

				for (int dataTypeNdx2 = 0; dataTypeNdx2 < DE_LENGTH_OF_ARRAY(s_uintTypes); ++dataTypeNdx2)
				{
					if (s_uintTypes[dataTypeNdx2] == s_uintTypes[dataTypeNdx])
						continue;

					const std::string shaderSource(genShaderSourceMulExtended(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_UMUL_EXTENDED, s_uintTypes[dataTypeNdx2], s_uintTypes[dataTypeNdx], s_uintTypes[dataTypeNdx], s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void umul_extended_invalid_y (NegativeTestContext& ctx)
{
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_uintTypes));
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_intTypes));

	ctx.beginSection("umulExtended: Invalid y type.");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			for (int dataTypeNdx = 0; dataTypeNdx < DE_LENGTH_OF_ARRAY(s_uintTypes); ++dataTypeNdx)
			{
				{
					const std::string shaderSource(genShaderSourceMulExtended(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_UMUL_EXTENDED, s_uintTypes[dataTypeNdx], s_floatTypes[dataTypeNdx], s_uintTypes[dataTypeNdx], s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceMulExtended(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_UMUL_EXTENDED, s_uintTypes[dataTypeNdx], s_intTypes[dataTypeNdx], s_uintTypes[dataTypeNdx], s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}

				for (int dataTypeNdx2 = 0; dataTypeNdx2 < DE_LENGTH_OF_ARRAY(s_uintTypes); ++dataTypeNdx2)
				{
					if (s_uintTypes[dataTypeNdx2] == s_uintTypes[dataTypeNdx])
						continue;

					const std::string shaderSource(genShaderSourceMulExtended(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_UMUL_EXTENDED, s_uintTypes[dataTypeNdx], s_uintTypes[dataTypeNdx2], s_uintTypes[dataTypeNdx], s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void umul_extended_invalid_msb (NegativeTestContext& ctx)
{
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_uintTypes));
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_intTypes));

	ctx.beginSection("umulExtended: Invalid msb type.");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			for (int dataTypeNdx = 0; dataTypeNdx < DE_LENGTH_OF_ARRAY(s_uintTypes); ++dataTypeNdx)
			{
				{
					const std::string shaderSource(genShaderSourceMulExtended(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_UMUL_EXTENDED, s_uintTypes[dataTypeNdx], s_uintTypes[dataTypeNdx], s_floatTypes[dataTypeNdx], s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceMulExtended(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_UMUL_EXTENDED, s_uintTypes[dataTypeNdx], s_uintTypes[dataTypeNdx], s_intTypes[dataTypeNdx], s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}

				for (int dataTypeNdx2 = 0; dataTypeNdx2 < DE_LENGTH_OF_ARRAY(s_uintTypes); ++dataTypeNdx2)
				{
					if (s_uintTypes[dataTypeNdx2] == s_uintTypes[dataTypeNdx])
						continue;

					const std::string shaderSource(genShaderSourceMulExtended(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_UMUL_EXTENDED, s_uintTypes[dataTypeNdx], s_uintTypes[dataTypeNdx], s_uintTypes[dataTypeNdx2], s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void umul_extended_invalid_lsb (NegativeTestContext& ctx)
{
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_uintTypes));
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_intTypes));

	ctx.beginSection("umulExtended: Invalid lsb type.");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			for (int dataTypeNdx = 0; dataTypeNdx < DE_LENGTH_OF_ARRAY(s_uintTypes); ++dataTypeNdx)
			{
				{
					const std::string shaderSource(genShaderSourceMulExtended(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_UMUL_EXTENDED, s_uintTypes[dataTypeNdx], s_uintTypes[dataTypeNdx], s_uintTypes[dataTypeNdx], s_floatTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceMulExtended(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_UMUL_EXTENDED, s_uintTypes[dataTypeNdx], s_uintTypes[dataTypeNdx], s_uintTypes[dataTypeNdx], s_intTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}

				for (int dataTypeNdx2 = 0; dataTypeNdx2 < DE_LENGTH_OF_ARRAY(s_uintTypes); ++dataTypeNdx2)
				{
					if (s_uintTypes[dataTypeNdx2] == s_uintTypes[dataTypeNdx])
						continue;

					const std::string shaderSource(genShaderSourceMulExtended(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_UMUL_EXTENDED, s_uintTypes[dataTypeNdx], s_uintTypes[dataTypeNdx], s_uintTypes[dataTypeNdx], s_uintTypes[dataTypeNdx2]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void imul_extended_invalid_x (NegativeTestContext& ctx)
{
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_uintTypes));
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_intTypes));

	ctx.beginSection("imulExtended: Invalid x type.");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			for (int dataTypeNdx = 0; dataTypeNdx < DE_LENGTH_OF_ARRAY(s_intTypes); ++dataTypeNdx)
			{
				{
					const std::string shaderSource(genShaderSourceMulExtended(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_IMUL_EXTENDED, s_floatTypes[dataTypeNdx], s_intTypes[dataTypeNdx], s_intTypes[dataTypeNdx], s_intTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceMulExtended(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_IMUL_EXTENDED, s_uintTypes[dataTypeNdx], s_intTypes[dataTypeNdx], s_intTypes[dataTypeNdx], s_intTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}

				for (int dataTypeNdx2 = 0; dataTypeNdx2 < DE_LENGTH_OF_ARRAY(s_intTypes); ++dataTypeNdx2)
				{
					if (s_intTypes[dataTypeNdx2] == s_intTypes[dataTypeNdx])
						continue;

					const std::string shaderSource(genShaderSourceMulExtended(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_IMUL_EXTENDED, s_intTypes[dataTypeNdx2], s_intTypes[dataTypeNdx], s_intTypes[dataTypeNdx], s_intTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void imul_extended_invalid_y (NegativeTestContext& ctx)
{
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_uintTypes));
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_intTypes));

	ctx.beginSection("imulExtended: Invalid y type.");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			for (int dataTypeNdx = 0; dataTypeNdx < DE_LENGTH_OF_ARRAY(s_intTypes); ++dataTypeNdx)
			{
				{
					const std::string shaderSource(genShaderSourceMulExtended(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_IMUL_EXTENDED, s_intTypes[dataTypeNdx], s_floatTypes[dataTypeNdx], s_intTypes[dataTypeNdx], s_intTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceMulExtended(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_IMUL_EXTENDED, s_intTypes[dataTypeNdx], s_uintTypes[dataTypeNdx], s_intTypes[dataTypeNdx], s_intTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}

				for (int dataTypeNdx2 = 0; dataTypeNdx2 < DE_LENGTH_OF_ARRAY(s_intTypes); ++dataTypeNdx2)
				{
					if (s_intTypes[dataTypeNdx2] == s_intTypes[dataTypeNdx])
						continue;

					const std::string shaderSource(genShaderSourceMulExtended(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_IMUL_EXTENDED, s_intTypes[dataTypeNdx], s_intTypes[dataTypeNdx2], s_intTypes[dataTypeNdx], s_intTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void imul_extended_invalid_msb (NegativeTestContext& ctx)
{
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_uintTypes));
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_intTypes));

	ctx.beginSection("imulExtended: Invalid msb type.");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			for (int dataTypeNdx = 0; dataTypeNdx < DE_LENGTH_OF_ARRAY(s_intTypes); ++dataTypeNdx)
			{
				{
					const std::string shaderSource(genShaderSourceMulExtended(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_IMUL_EXTENDED, s_intTypes[dataTypeNdx], s_intTypes[dataTypeNdx], s_floatTypes[dataTypeNdx], s_intTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceMulExtended(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_IMUL_EXTENDED, s_intTypes[dataTypeNdx], s_intTypes[dataTypeNdx], s_uintTypes[dataTypeNdx], s_intTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}

				for (int dataTypeNdx2 = 0; dataTypeNdx2 < DE_LENGTH_OF_ARRAY(s_intTypes); ++dataTypeNdx2)
				{
					if (s_intTypes[dataTypeNdx2] == s_intTypes[dataTypeNdx])
						continue;

					const std::string shaderSource(genShaderSourceMulExtended(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_IMUL_EXTENDED, s_intTypes[dataTypeNdx], s_intTypes[dataTypeNdx], s_intTypes[dataTypeNdx2], s_intTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void imul_extended_invalid_lsb (NegativeTestContext& ctx)
{
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_uintTypes));
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_intTypes));

	ctx.beginSection("imulExtended: Invalid lsb type.");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			for (int dataTypeNdx = 0; dataTypeNdx < DE_LENGTH_OF_ARRAY(s_intTypes); ++dataTypeNdx)
			{
				{
					const std::string shaderSource(genShaderSourceMulExtended(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_IMUL_EXTENDED, s_intTypes[dataTypeNdx], s_intTypes[dataTypeNdx], s_intTypes[dataTypeNdx], s_floatTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceMulExtended(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_IMUL_EXTENDED, s_intTypes[dataTypeNdx], s_intTypes[dataTypeNdx], s_intTypes[dataTypeNdx], s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}

				for (int dataTypeNdx2 = 0; dataTypeNdx2 < DE_LENGTH_OF_ARRAY(s_intTypes); ++dataTypeNdx2)
				{
					if (s_intTypes[dataTypeNdx2] == s_intTypes[dataTypeNdx])
						continue;

					const std::string shaderSource(genShaderSourceMulExtended(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_IMUL_EXTENDED, s_intTypes[dataTypeNdx], s_intTypes[dataTypeNdx], s_intTypes[dataTypeNdx], s_intTypes[dataTypeNdx2]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

// frexp, ldexp
std::string genShaderSourceFrexpLdexp (NegativeTestContext& ctx, glu::ShaderType shaderType, ShaderFunction function, glu::DataType xDataType, glu::DataType expDataType)
{
	DE_ASSERT(function == SHADER_FUNCTION_FREXP || function == SHADER_FUNCTION_LDEXP);

	std::ostringstream source;
	source	<< (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) ? glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES)) << "\n"
			<< getShaderInitialization(ctx, shaderType)
			<< "void main(void)\n"
			<< "{\n"
			<< "    " << declareAndInitializeShaderVariable(xDataType, "x")
			<< "    " << declareAndInitializeShaderVariable(expDataType, "exp");

	switch (function)
	{
		case SHADER_FUNCTION_FREXP:
			source << "    frexp(x, exp);\n";
			break;

		case SHADER_FUNCTION_LDEXP:
			source << "    ldexp(x, exp);\n";
			break;

		default:
			DE_FATAL("Unsupported shader function.");
	}

	source << "}\n";

	return source.str();
}

void frexp_invalid_x (NegativeTestContext& ctx)
{
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_uintTypes));
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_intTypes));

	ctx.beginSection("frexp: Invalid x type.");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			for (int dataTypeNdx = 0; dataTypeNdx < DE_LENGTH_OF_ARRAY(s_floatTypes); ++dataTypeNdx)
			{
				{
					const std::string shaderSource(genShaderSourceFrexpLdexp(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_FREXP, s_intTypes[dataTypeNdx], s_intTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceFrexpLdexp(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_FREXP, s_uintTypes[dataTypeNdx], s_intTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void frexp_invalid_exp (NegativeTestContext& ctx)
{
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_uintTypes));
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_intTypes));

	ctx.beginSection("frexp: Invalid exp type.");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			for (int dataTypeNdx = 0; dataTypeNdx < DE_LENGTH_OF_ARRAY(s_floatTypes); ++dataTypeNdx)
			{
				{
					const std::string shaderSource(genShaderSourceFrexpLdexp(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_FREXP, s_floatTypes[dataTypeNdx], s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceFrexpLdexp(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_FREXP, s_floatTypes[dataTypeNdx], s_floatTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void ldexp_invalid_x (NegativeTestContext& ctx)
{
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_uintTypes));
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_intTypes));

	ctx.beginSection("ldexp: Invalid x type.");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			for (int dataTypeNdx = 0; dataTypeNdx < DE_LENGTH_OF_ARRAY(s_floatTypes); ++dataTypeNdx)
			{
				{
					const std::string shaderSource(genShaderSourceFrexpLdexp(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_LDEXP, s_intTypes[dataTypeNdx], s_intTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceFrexpLdexp(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_LDEXP, s_uintTypes[dataTypeNdx], s_intTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void ldexp_invalid_exp (NegativeTestContext& ctx)
{
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_uintTypes));
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_intTypes));

	ctx.beginSection("ldexp: Invalid exp type.");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			for (int dataTypeNdx = 0; dataTypeNdx < DE_LENGTH_OF_ARRAY(s_floatTypes); ++dataTypeNdx)
			{
				{
					const std::string shaderSource(genShaderSourceFrexpLdexp(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_LDEXP, s_floatTypes[dataTypeNdx], s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceFrexpLdexp(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_LDEXP, s_floatTypes[dataTypeNdx], s_floatTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

// packUnorm4x8, packSnorm4x8, unpackSnorm4x8, unpackUnorm4x8
std::string genShaderSourcePackUnpackNorm4x8 (NegativeTestContext& ctx, glu::ShaderType shaderType, ShaderFunction function, glu::DataType dataType)
{
	DE_ASSERT(function == SHADER_FUNCTION_PACK_UNORM_4X8 ||
		function == SHADER_FUNCTION_PACK_SNORM_4X8 ||
		function == SHADER_FUNCTION_UNPACK_SNORM_4X8 ||
		function == SHADER_FUNCTION_UNPACK_UNORM_4X8);

	std::ostringstream source;
	source	<< (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) ? glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES)) << "\n"
			<< getShaderInitialization(ctx, shaderType)
			<< "void main(void)\n"
			<< "{\n";

	switch (function)
	{
		case SHADER_FUNCTION_PACK_UNORM_4X8:
			source	<< "    mediump " << declareAndInitializeShaderVariable(dataType, "v")
					<< "    packUnorm4x8(v);\n";
			break;

		case SHADER_FUNCTION_PACK_SNORM_4X8:
			source	<< "    mediump " << declareAndInitializeShaderVariable(dataType, "v")
					<< "    packSnorm4x8(v);\n";
			break;

		case SHADER_FUNCTION_UNPACK_SNORM_4X8:
			source	<< "    highp " << declareAndInitializeShaderVariable(dataType, "p")
					<< "    unpackSnorm4x8(p);\n";
			break;

		case SHADER_FUNCTION_UNPACK_UNORM_4X8:
			source	<< "    highp " << declareAndInitializeShaderVariable(dataType, "p")
					<< "    unpackUnorm4x8(p);\n";
			break;

		default:
			DE_FATAL("Unsupported shader function.");
	}

	source << "}\n";

	return source.str();
}

void pack_unorm_4x8 (NegativeTestContext& ctx)
{
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_uintTypes));
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_intTypes));

	ctx.beginSection("packUnorm4x8: Invalid v type.");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			for (int dataTypeNdx = 0; dataTypeNdx < DE_LENGTH_OF_ARRAY(s_floatTypes); ++dataTypeNdx)
			{
				if (s_floatTypes[dataTypeNdx] == glu::TYPE_FLOAT_VEC4)
					continue;

				const std::string shaderSource(genShaderSourcePackUnpackNorm4x8(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_PACK_UNORM_4X8, s_floatTypes[dataTypeNdx]));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}

			for (int dataTypeNdx = 0; dataTypeNdx < DE_LENGTH_OF_ARRAY(s_intTypes); ++dataTypeNdx)
			{
				{
					const std::string shaderSource(genShaderSourcePackUnpackNorm4x8(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_PACK_UNORM_4X8, s_intTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourcePackUnpackNorm4x8(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_PACK_UNORM_4X8, s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void pack_snorm_4x8 (NegativeTestContext& ctx)
{
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_uintTypes));
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_intTypes));

	ctx.beginSection("packSnorm4x8: Invalid v type.");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			for (int dataTypeNdx = 0; dataTypeNdx < DE_LENGTH_OF_ARRAY(s_floatTypes); ++dataTypeNdx)
			{
				if (s_floatTypes[dataTypeNdx] == glu::TYPE_FLOAT_VEC4)
					continue;

				const std::string shaderSource(genShaderSourcePackUnpackNorm4x8(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_PACK_SNORM_4X8, s_floatTypes[dataTypeNdx]));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			for (int dataTypeNdx = 0; dataTypeNdx < DE_LENGTH_OF_ARRAY(s_intTypes); ++dataTypeNdx)
			{
				{
					const std::string shaderSource(genShaderSourcePackUnpackNorm4x8(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_PACK_SNORM_4X8, s_intTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourcePackUnpackNorm4x8(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_PACK_SNORM_4X8, s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void unpack_snorm_4x8 (NegativeTestContext& ctx)
{
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_uintTypes));
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_intTypes));

	ctx.beginSection("unpackSnorm4x8: Invalid v type.");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			for (int dataTypeNdx = 0; dataTypeNdx < DE_LENGTH_OF_ARRAY(s_uintTypes); ++dataTypeNdx)
			{
				if (s_uintTypes[dataTypeNdx] == glu::TYPE_UINT)
					continue;

				const std::string shaderSource(genShaderSourcePackUnpackNorm4x8(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_UNPACK_SNORM_4X8, s_uintTypes[dataTypeNdx]));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			for (int dataTypeNdx = 0; dataTypeNdx < DE_LENGTH_OF_ARRAY(s_intTypes); ++dataTypeNdx)
			{
				{
					const std::string shaderSource(genShaderSourcePackUnpackNorm4x8(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_UNPACK_SNORM_4X8, s_intTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourcePackUnpackNorm4x8(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_UNPACK_SNORM_4X8, s_floatTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void unpack_unorm_4x8 (NegativeTestContext& ctx)
{
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_uintTypes));
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_intTypes));

	ctx.beginSection("unpackUnorm4x8: Invalid v type.");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			for (int dataTypeNdx = 0; dataTypeNdx < DE_LENGTH_OF_ARRAY(s_uintTypes); ++dataTypeNdx)
			{
				if (s_uintTypes[dataTypeNdx] == glu::TYPE_UINT)
					continue;

				const std::string shaderSource(genShaderSourcePackUnpackNorm4x8(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_UNPACK_UNORM_4X8, s_uintTypes[dataTypeNdx]));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			for (int dataTypeNdx = 0; dataTypeNdx < DE_LENGTH_OF_ARRAY(s_intTypes); ++dataTypeNdx)
			{
				{
					const std::string shaderSource(genShaderSourcePackUnpackNorm4x8(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_UNPACK_UNORM_4X8, s_intTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourcePackUnpackNorm4x8(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_UNPACK_UNORM_4X8, s_floatTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

// textureSize
std::string genShaderSourceTextureSize_sampler (NegativeTestContext& ctx, glu::ShaderType shaderType, glu::DataType samplerDataType, glu::DataType lodDataType)
{
	std::ostringstream source;
	source	<< (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) ? glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES)) << "\n"
			<< getShaderExtensionDeclaration(getDataTypeExtension(samplerDataType))
			<< getShaderInitialization(ctx, shaderType)
			<< declareShaderUniform(samplerDataType, "sampler")
			<< "void main(void)\n"
			<< "{\n";

	switch (samplerDataType)
	{
		case glu::TYPE_SAMPLER_2D:
		case glu::TYPE_INT_SAMPLER_2D:
		case glu::TYPE_UINT_SAMPLER_2D:
		case glu::TYPE_SAMPLER_3D:
		case glu::TYPE_INT_SAMPLER_3D:
		case glu::TYPE_UINT_SAMPLER_3D:
		case glu::TYPE_SAMPLER_CUBE:
		case glu::TYPE_INT_SAMPLER_CUBE:
		case glu::TYPE_UINT_SAMPLER_CUBE:
		case glu::TYPE_SAMPLER_2D_ARRAY:
		case glu::TYPE_INT_SAMPLER_2D_ARRAY:
		case glu::TYPE_UINT_SAMPLER_2D_ARRAY:
		case glu::TYPE_SAMPLER_CUBE_SHADOW:
		case glu::TYPE_SAMPLER_2D_SHADOW:
		case glu::TYPE_SAMPLER_2D_ARRAY_SHADOW:
		case glu::TYPE_SAMPLER_CUBE_ARRAY:
		case glu::TYPE_INT_SAMPLER_CUBE_ARRAY:
		case glu::TYPE_UINT_SAMPLER_CUBE_ARRAY:
		case glu::TYPE_SAMPLER_CUBE_ARRAY_SHADOW:
			source << "    textureSize(sampler);\n";
			break;

		case glu::TYPE_SAMPLER_2D_MULTISAMPLE:
		case glu::TYPE_INT_SAMPLER_2D_MULTISAMPLE:
		case glu::TYPE_UINT_SAMPLER_2D_MULTISAMPLE:
		case glu::TYPE_SAMPLER_BUFFER:
		case glu::TYPE_INT_SAMPLER_BUFFER:
		case glu::TYPE_UINT_SAMPLER_BUFFER:
		case glu::TYPE_SAMPLER_2D_MULTISAMPLE_ARRAY:
		case glu::TYPE_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
		case glu::TYPE_UINT_SAMPLER_2D_MULTISAMPLE_ARRAY:
			source	<< "    " << declareAndInitializeShaderVariable(lodDataType, "lod")
					<< "    textureSize(sampler, lod);\n";
			break;

		default:
			DE_FATAL("Unsupported data type.");
	}

	source << "}\n";

	return source.str();
}

void texture_size_invalid_sampler (NegativeTestContext& ctx)
{
	ctx.beginSection("textureSize: Invalid sampler type - some overloads take two arguments while others take only one.");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			for (int dataTypeNdx = 0; dataTypeNdx < DE_LENGTH_OF_ARRAY(s_samplerTypes); ++dataTypeNdx)
			{
				if (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) || ctx.isExtensionSupported(getDataTypeExtension(s_samplerTypes[dataTypeNdx])))
				{
					ctx.beginSection("Verify sampler data type: " + std::string(getDataTypeName(s_samplerTypes[dataTypeNdx])));
					const std::string shaderSource(genShaderSourceTextureSize_sampler(ctx, s_shaders[shaderNdx], s_samplerTypes[dataTypeNdx], glu::TYPE_INT));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					ctx.endSection();
				}
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

std::string genShaderSourceTextureSize_lod (NegativeTestContext& ctx, glu::ShaderType shaderType, glu::DataType samplerDataType, glu::DataType lodDataType)
{
	std::ostringstream source;
	source	<< (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) ? glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES)) << "\n"
			<< getShaderExtensionDeclaration(getDataTypeExtension(samplerDataType))
			<< getShaderInitialization(ctx, shaderType)
			<< declareShaderUniform(samplerDataType, "sampler")
			<< "void main(void)\n"
			<< "{\n"
			<< "    " << declareAndInitializeShaderVariable(lodDataType, "lod")
			<< "    textureSize(sampler, lod);\n"
			<< "}\n";

	return source.str();
}

void texture_size_invalid_lod (NegativeTestContext& ctx)
{
	ctx.beginSection("textureSize: Invalid lod type.");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			for (int dataTypeNdx = 0; dataTypeNdx < DE_LENGTH_OF_ARRAY(s_samplerTypes); ++dataTypeNdx)
			{
				if (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) || ctx.isExtensionSupported(getDataTypeExtension(s_samplerTypes[dataTypeNdx])))
				{
					ctx.beginSection("Verify sampler/lod data type" + std::string(getDataTypeName(s_samplerTypes[dataTypeNdx])));
					for (int dataTypeNdx2 = 0; dataTypeNdx2 < DE_LENGTH_OF_ARRAY(s_nonScalarIntTypes); ++dataTypeNdx2)
					{
						if (s_nonScalarIntTypes[dataTypeNdx2] == glu::TYPE_INT)
							continue;

						const std::string shaderSource(genShaderSourceTextureSize_lod(ctx, s_shaders[shaderNdx], s_samplerTypes[dataTypeNdx], s_nonScalarIntTypes[dataTypeNdx2]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					ctx.endSection();
				}
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

// texture
std::string genShaderSourceTexture (NegativeTestContext& ctx, glu::ShaderType shaderType, FunctionTextureModes mode, glu::DataType samplerDataType, glu::DataType pDataType, glu::DataType thirdArgumentDataType)
{
	std::ostringstream source;
	source	<< (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) ? glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES)) << "\n"
			<< getShaderExtensionDeclaration(getDataTypeExtension(samplerDataType))
			<< getShaderInitialization(ctx, shaderType)
			<< declareShaderUniform(samplerDataType, "sampler")
			<< "void main(void)\n"
			<< "{\n"
			<< "    highp " << declareAndInitializeShaderVariable(pDataType, "lod");

	switch (mode)
	{
		case FUNCTION_TEXTURE_MODE_NO_BIAS_NO_COMPARE:
			source << "    texture(sampler, lod);\n";
			break;

		case FUNCTION_TEXTURE_MODE_BIAS_OR_COMPARE:
			source	<< "    highp " << declareAndInitializeShaderVariable(thirdArgumentDataType, "thirdArgument")
					<< "    texture(sampler, lod, thirdArgument);\n";
			break;

		default:
			DE_FATAL("Unsupported shader function overload.");
	}

	source << "}\n";

	return source.str();
}

std::string genShaderSourceTexture (NegativeTestContext& ctx, glu::ShaderType shaderType, glu::DataType samplerDataType, glu::DataType pDataType)
{
	return genShaderSourceTexture(ctx, shaderType, FUNCTION_TEXTURE_MODE_NO_BIAS_NO_COMPARE, samplerDataType, pDataType, glu::TYPE_LAST);
}

std::string genShaderSourceTexture (NegativeTestContext& ctx, glu::ShaderType shaderType, glu::DataType samplerDataType, glu::DataType pDataType, glu::DataType thirdArgumentDataType)
{
	return genShaderSourceTexture(ctx, shaderType, FUNCTION_TEXTURE_MODE_BIAS_OR_COMPARE, samplerDataType, pDataType, thirdArgumentDataType);
}

void texture_invalid_p (NegativeTestContext& ctx)
{
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_intTypes));
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_uintTypes));

	ctx.beginSection("texture: Invalid P type.");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			for (int dataTypeNdx = 0; dataTypeNdx < DE_LENGTH_OF_ARRAY(s_floatTypes); ++dataTypeNdx)
			{
				// SAMPLER_2D
				if (s_floatTypes[dataTypeNdx] != glu::TYPE_FLOAT_VEC2)
				{
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D, s_floatTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D, s_floatTypes[dataTypeNdx], glu::TYPE_FLOAT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_2D, s_floatTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_2D, s_floatTypes[dataTypeNdx], glu::TYPE_FLOAT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_2D, s_floatTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_2D, s_floatTypes[dataTypeNdx], glu::TYPE_FLOAT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D, s_intTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D, s_intTypes[dataTypeNdx], glu::TYPE_FLOAT));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_2D, s_intTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_2D, s_intTypes[dataTypeNdx], glu::TYPE_FLOAT));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_2D, s_intTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_2D, s_intTypes[dataTypeNdx], glu::TYPE_FLOAT));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D, s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D, s_uintTypes[dataTypeNdx], glu::TYPE_FLOAT));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_2D, s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_2D, s_uintTypes[dataTypeNdx], glu::TYPE_FLOAT));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_2D, s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_2D, s_uintTypes[dataTypeNdx], glu::TYPE_FLOAT));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}

				// SAMPLER_3D
				if (s_floatTypes[dataTypeNdx] != glu::TYPE_FLOAT_VEC3)
				{
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_3D, s_floatTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_3D, s_floatTypes[dataTypeNdx], glu::TYPE_FLOAT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_3D, s_floatTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_3D, s_floatTypes[dataTypeNdx], glu::TYPE_FLOAT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_3D, s_floatTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_3D, s_floatTypes[dataTypeNdx], glu::TYPE_FLOAT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_3D, s_intTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_3D, s_intTypes[dataTypeNdx], glu::TYPE_FLOAT));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_3D, s_intTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_3D, s_intTypes[dataTypeNdx], glu::TYPE_FLOAT));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_3D, s_intTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_3D, s_intTypes[dataTypeNdx], glu::TYPE_FLOAT));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_3D, s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_3D, s_uintTypes[dataTypeNdx], glu::TYPE_FLOAT));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_3D, s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_3D, s_uintTypes[dataTypeNdx], glu::TYPE_FLOAT));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_3D, s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_3D, s_uintTypes[dataTypeNdx], glu::TYPE_FLOAT));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}

				// SAMPLER_CUBE
				if (s_floatTypes[dataTypeNdx] != glu::TYPE_FLOAT_VEC3)
				{
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE, s_floatTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE, s_floatTypes[dataTypeNdx], glu::TYPE_FLOAT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_CUBE, s_floatTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_CUBE, s_floatTypes[dataTypeNdx], glu::TYPE_FLOAT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_CUBE, s_floatTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_CUBE, s_floatTypes[dataTypeNdx], glu::TYPE_FLOAT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
				}
				{
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE, s_intTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE, s_intTypes[dataTypeNdx], glu::TYPE_FLOAT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_CUBE, s_intTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_CUBE, s_intTypes[dataTypeNdx], glu::TYPE_FLOAT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_CUBE, s_intTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_CUBE, s_intTypes[dataTypeNdx], glu::TYPE_FLOAT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
				}
				{
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE, s_uintTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE, s_uintTypes[dataTypeNdx], glu::TYPE_FLOAT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_CUBE, s_uintTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_CUBE, s_uintTypes[dataTypeNdx], glu::TYPE_FLOAT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_CUBE, s_uintTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_CUBE, s_uintTypes[dataTypeNdx], glu::TYPE_FLOAT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
				}

				// SAMPLER_2D_ARRAY
				if (s_floatTypes[dataTypeNdx] != glu::TYPE_FLOAT_VEC3)
				{
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D_ARRAY, s_floatTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D_ARRAY, s_floatTypes[dataTypeNdx], glu::TYPE_FLOAT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_2D_ARRAY, s_floatTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_2D_ARRAY, s_floatTypes[dataTypeNdx], glu::TYPE_FLOAT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_2D_ARRAY, s_floatTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_2D_ARRAY, s_floatTypes[dataTypeNdx], glu::TYPE_FLOAT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D_ARRAY, s_intTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D_ARRAY, s_intTypes[dataTypeNdx], glu::TYPE_FLOAT));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_2D_ARRAY, s_intTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_2D_ARRAY, s_intTypes[dataTypeNdx], glu::TYPE_FLOAT));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_2D_ARRAY, s_intTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_2D_ARRAY, s_intTypes[dataTypeNdx], glu::TYPE_FLOAT));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D_ARRAY, s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D_ARRAY, s_uintTypes[dataTypeNdx], glu::TYPE_FLOAT));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_2D_ARRAY, s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_2D_ARRAY, s_uintTypes[dataTypeNdx], glu::TYPE_FLOAT));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_2D_ARRAY, s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_2D_ARRAY, s_uintTypes[dataTypeNdx], glu::TYPE_FLOAT));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}

				// SAMPLER_2D_SHADOW
				if (s_floatTypes[dataTypeNdx] != glu::TYPE_FLOAT_VEC3)
				{
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D_SHADOW, s_floatTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D_SHADOW, s_floatTypes[dataTypeNdx], glu::TYPE_FLOAT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D_SHADOW, s_intTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D_SHADOW, s_intTypes[dataTypeNdx], glu::TYPE_FLOAT));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D_SHADOW, s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D_SHADOW, s_uintTypes[dataTypeNdx], glu::TYPE_FLOAT));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}

				// SAMPLER_CUBE_SHADOW
				if (s_floatTypes[dataTypeNdx] != glu::TYPE_FLOAT_VEC4)
				{
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE_SHADOW, s_floatTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE_SHADOW, s_floatTypes[dataTypeNdx], glu::TYPE_FLOAT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE_SHADOW, s_intTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE_SHADOW, s_intTypes[dataTypeNdx], glu::TYPE_FLOAT));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE_SHADOW, s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE_SHADOW, s_uintTypes[dataTypeNdx], glu::TYPE_FLOAT));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}

				// SAMPLER_2D_ARRAY_SHADOW
				if (s_floatTypes[dataTypeNdx] != glu::TYPE_FLOAT_VEC4)
				{
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D_ARRAY_SHADOW, s_floatTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D_ARRAY_SHADOW, s_floatTypes[dataTypeNdx], glu::TYPE_FLOAT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D_ARRAY_SHADOW, s_intTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D_ARRAY_SHADOW, s_intTypes[dataTypeNdx], glu::TYPE_FLOAT));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D_ARRAY_SHADOW, s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D_ARRAY_SHADOW, s_uintTypes[dataTypeNdx], glu::TYPE_FLOAT));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}

				// SAMPLER_CUBE_ARRAY
				if (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) || ctx.isExtensionSupported(getDataTypeExtension(glu::TYPE_SAMPLER_CUBE_ARRAY)))
				{
					if (s_floatTypes[dataTypeNdx] != glu::TYPE_FLOAT_VEC4)
					{
						{
							const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE_ARRAY, s_floatTypes[dataTypeNdx]));
							verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
						}
						{
							const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE_ARRAY, s_floatTypes[dataTypeNdx], glu::TYPE_FLOAT));
							verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
						}
						{
							const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_CUBE_ARRAY, s_floatTypes[dataTypeNdx]));
							verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
						}
						{
							const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_CUBE_ARRAY, s_floatTypes[dataTypeNdx], glu::TYPE_FLOAT));
							verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
						}
						{
							const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_CUBE_ARRAY, s_floatTypes[dataTypeNdx]));
							verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
						}
						{
							const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_CUBE_ARRAY, s_floatTypes[dataTypeNdx], glu::TYPE_FLOAT));
							verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
						}
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE_ARRAY, s_intTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE_ARRAY, s_intTypes[dataTypeNdx], glu::TYPE_FLOAT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_CUBE_ARRAY, s_intTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_CUBE_ARRAY, s_intTypes[dataTypeNdx], glu::TYPE_FLOAT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_CUBE_ARRAY, s_intTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_CUBE_ARRAY, s_intTypes[dataTypeNdx], glu::TYPE_FLOAT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE_ARRAY, s_uintTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE_ARRAY, s_uintTypes[dataTypeNdx], glu::TYPE_FLOAT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_CUBE_ARRAY, s_uintTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_CUBE_ARRAY, s_uintTypes[dataTypeNdx], glu::TYPE_FLOAT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_CUBE_ARRAY, s_uintTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_CUBE_ARRAY, s_uintTypes[dataTypeNdx], glu::TYPE_FLOAT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
				}

				// SAMPLER_CUBE_ARRAY_SHADOW
				if (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) || ctx.isExtensionSupported(getDataTypeExtension(glu::TYPE_SAMPLER_CUBE_ARRAY_SHADOW)))
				{
					if (s_floatTypes[dataTypeNdx] != glu::TYPE_FLOAT_VEC4)
					{
						std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE_ARRAY_SHADOW, s_floatTypes[dataTypeNdx], glu::TYPE_FLOAT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE_ARRAY_SHADOW, s_intTypes[dataTypeNdx], glu::TYPE_FLOAT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE_ARRAY_SHADOW, s_uintTypes[dataTypeNdx], glu::TYPE_FLOAT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
				}
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void texture_invalid_bias_or_compare (NegativeTestContext& ctx)
{
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_intTypes));
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_uintTypes));

	ctx.beginSection("texture: Invalid bias/compare type.");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			for (int dataTypeNdx = 0; dataTypeNdx < DE_LENGTH_OF_ARRAY(s_floatTypes); ++dataTypeNdx)
			{
				// SAMPLER_2D
				if (s_floatTypes[dataTypeNdx] != glu::TYPE_FLOAT)
				{
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D, glu::TYPE_FLOAT_VEC2, s_floatTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_2D, glu::TYPE_FLOAT_VEC2, s_floatTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_2D, glu::TYPE_FLOAT_VEC2, s_floatTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D, glu::TYPE_FLOAT_VEC2, s_intTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_2D, glu::TYPE_FLOAT_VEC2, s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_2D, glu::TYPE_FLOAT_VEC2, s_intTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_2D, glu::TYPE_FLOAT_VEC2, s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_2D, glu::TYPE_FLOAT_VEC2, s_intTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_2D, glu::TYPE_FLOAT_VEC2, s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}

				// SAMPLER_3D
				if (s_floatTypes[dataTypeNdx] != glu::TYPE_FLOAT)
				{
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_3D, glu::TYPE_FLOAT_VEC3, s_floatTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_3D, glu::TYPE_FLOAT_VEC3, s_floatTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_3D, glu::TYPE_FLOAT_VEC3, s_floatTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_3D, glu::TYPE_FLOAT_VEC3, s_intTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_3D, glu::TYPE_FLOAT_VEC3, s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_3D, glu::TYPE_FLOAT_VEC3, s_intTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_3D, glu::TYPE_FLOAT_VEC3, s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_3D, glu::TYPE_FLOAT_VEC3, s_intTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_3D, glu::TYPE_FLOAT_VEC3, s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}

				// SAMPLER_CUBE
				if (s_floatTypes[dataTypeNdx] != glu::TYPE_FLOAT)
				{
					std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE, glu::TYPE_FLOAT_VEC3, s_floatTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					shaderSource = genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_CUBE, glu::TYPE_FLOAT_VEC3, s_floatTypes[dataTypeNdx]);
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					shaderSource = genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_CUBE, glu::TYPE_FLOAT_VEC3, s_floatTypes[dataTypeNdx]);
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE, glu::TYPE_FLOAT_VEC3, s_intTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE, glu::TYPE_FLOAT_VEC3, s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_CUBE, glu::TYPE_FLOAT_VEC3, s_intTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_CUBE, glu::TYPE_FLOAT_VEC3, s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_CUBE, glu::TYPE_FLOAT_VEC3, s_intTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_CUBE, glu::TYPE_FLOAT_VEC3, s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}

				// SAMPLER_2D_ARRAY
				if (s_floatTypes[dataTypeNdx] != glu::TYPE_FLOAT)
				{
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D_ARRAY, glu::TYPE_FLOAT_VEC3, s_floatTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_2D_ARRAY, glu::TYPE_FLOAT_VEC3, s_floatTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_2D_ARRAY, glu::TYPE_FLOAT_VEC3, s_floatTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D_ARRAY, glu::TYPE_FLOAT_VEC3, s_intTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D_ARRAY, glu::TYPE_FLOAT_VEC3, s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_2D_ARRAY, glu::TYPE_FLOAT_VEC3, s_intTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_2D_ARRAY, glu::TYPE_FLOAT_VEC3, s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_2D_ARRAY, glu::TYPE_FLOAT_VEC3, s_intTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_2D_ARRAY, glu::TYPE_FLOAT_VEC3, s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}

				// SAMPLER_2D_SHADOW
				if (s_floatTypes[dataTypeNdx] != glu::TYPE_FLOAT)
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D_SHADOW, glu::TYPE_FLOAT_VEC3, s_floatTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D_SHADOW, glu::TYPE_FLOAT_VEC3, s_intTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D_SHADOW, glu::TYPE_FLOAT_VEC3, s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}

				// SAMPLER_CUBE_SHADOW
				if (s_floatTypes[dataTypeNdx] != glu::TYPE_FLOAT)
				{
					std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE_SHADOW, glu::TYPE_FLOAT_VEC4, s_floatTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE_SHADOW, glu::TYPE_FLOAT_VEC4, s_intTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE_SHADOW, glu::TYPE_FLOAT_VEC4, s_uintTypes[dataTypeNdx]));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}

				// SAMPLER_CUBE_ARRAY
				if (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) || ctx.isExtensionSupported(getDataTypeExtension(glu::TYPE_SAMPLER_CUBE_ARRAY)))
				{
					if (s_floatTypes[dataTypeNdx] != glu::TYPE_FLOAT)
					{
						{
							const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE_ARRAY, glu::TYPE_FLOAT_VEC4, s_floatTypes[dataTypeNdx]));
							verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
						}
						{
							const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_CUBE_ARRAY, glu::TYPE_FLOAT_VEC4, s_floatTypes[dataTypeNdx]));
							verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
						}
						{
							const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_CUBE_ARRAY, glu::TYPE_FLOAT_VEC4, s_floatTypes[dataTypeNdx]));
							verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
						}
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE_ARRAY, glu::TYPE_FLOAT_VEC4, s_intTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE_ARRAY, glu::TYPE_FLOAT_VEC4, s_uintTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_CUBE_ARRAY, glu::TYPE_FLOAT_VEC4, s_intTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_CUBE_ARRAY, glu::TYPE_FLOAT_VEC4, s_uintTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_CUBE_ARRAY, glu::TYPE_FLOAT_VEC4, s_intTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_CUBE_ARRAY, glu::TYPE_FLOAT_VEC4, s_uintTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
				}

				// SAMPLER_CUBE_ARRAY_SHADOW
				if (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) || ctx.isExtensionSupported(getDataTypeExtension(glu::TYPE_SAMPLER_CUBE_ARRAY_SHADOW)))
				{
					if (s_floatTypes[dataTypeNdx] != glu::TYPE_FLOAT)
					{
						{
							const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE_ARRAY_SHADOW, glu::TYPE_FLOAT_VEC4, s_floatTypes[dataTypeNdx]));
							verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
						}
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE_ARRAY_SHADOW, glu::TYPE_FLOAT_VEC4, s_intTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexture(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE_ARRAY_SHADOW, glu::TYPE_FLOAT_VEC4, s_uintTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
				}
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

// textureLod
std::string genShaderSourceTextureLod (NegativeTestContext& ctx, glu::ShaderType shaderType, glu::DataType samplerDataType, glu::DataType pDataType, glu::DataType lodDataType)
{
	std::ostringstream source;
	source	<< (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) ? glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES)) << "\n"
			<< getShaderExtensionDeclaration(getDataTypeExtension(samplerDataType))
			<< getShaderInitialization(ctx, shaderType)
			<< declareShaderUniform(samplerDataType, "sampler")
			<< "void main(void)\n"
			<< "{\n"
			<< "    " << declareAndInitializeShaderVariable(pDataType, "P")
			<< "    " << declareAndInitializeShaderVariable(lodDataType, "lod")
			<< "    textureLod(sampler, P, lod);\n"
			<< "}\n";

	return source.str();
}

void texture_lod_invalid_p (NegativeTestContext& ctx)
{
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_intTypes));
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_uintTypes));

	if (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) || ctx.isExtensionSupported(getDataTypeExtension(glu::TYPE_SAMPLER_CUBE_ARRAY)))
	{
		ctx.beginSection("textureLod: Invalid P type.");
		for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
		{
			if (ctx.isShaderSupported(s_shaders[shaderNdx]))
			{
				ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
				for (int dataTypeNdx = 0; dataTypeNdx < DE_LENGTH_OF_ARRAY(s_floatTypes); ++dataTypeNdx)
				{
					if (s_floatTypes[dataTypeNdx] != glu::TYPE_FLOAT_VEC4)
					{
						{
							const std::string shaderSource(genShaderSourceTextureLod(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE_ARRAY, s_floatTypes[dataTypeNdx], glu::TYPE_FLOAT));
							verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
						}
						{
							const std::string shaderSource(genShaderSourceTextureLod(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_CUBE_ARRAY, s_floatTypes[dataTypeNdx], glu::TYPE_FLOAT));
							verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
						}
						{
							const std::string shaderSource(genShaderSourceTextureLod(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_CUBE_ARRAY, s_floatTypes[dataTypeNdx], glu::TYPE_FLOAT));
							verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
						}
					}
					{
						const std::string shaderSource(genShaderSourceTextureLod(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE_ARRAY, s_intTypes[dataTypeNdx], glu::TYPE_FLOAT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTextureLod(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_CUBE_ARRAY, s_intTypes[dataTypeNdx], glu::TYPE_FLOAT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTextureLod(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_CUBE_ARRAY, s_intTypes[dataTypeNdx], glu::TYPE_FLOAT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTextureLod(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE_ARRAY, s_uintTypes[dataTypeNdx], glu::TYPE_FLOAT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTextureLod(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_CUBE_ARRAY, s_uintTypes[dataTypeNdx], glu::TYPE_FLOAT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTextureLod(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_CUBE_ARRAY, s_uintTypes[dataTypeNdx], glu::TYPE_FLOAT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
				}
				ctx.endSection();
			}
		}
		ctx.endSection();
	}
}

void texture_lod_invalid_lod (NegativeTestContext& ctx)
{
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_intTypes));
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_uintTypes));

	if (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) || ctx.isExtensionSupported(getDataTypeExtension(glu::TYPE_SAMPLER_CUBE_ARRAY)))
	{
		ctx.beginSection("textureLod: Invalid lod type.");
		for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
		{
			if (ctx.isShaderSupported(s_shaders[shaderNdx]))
			{
				ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
				for (int dataTypeNdx = 0; dataTypeNdx < DE_LENGTH_OF_ARRAY(s_floatTypes); ++dataTypeNdx)
				{
					if (s_floatTypes[dataTypeNdx] != glu::TYPE_FLOAT)
					{
						{
							const std::string shaderSource(genShaderSourceTextureLod(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE_ARRAY, glu::TYPE_FLOAT, s_floatTypes[dataTypeNdx]));
							verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
						}
						{
							const std::string shaderSource(genShaderSourceTextureLod(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_CUBE_ARRAY, glu::TYPE_FLOAT, s_floatTypes[dataTypeNdx]));
							verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
						}
						{
							const std::string shaderSource(genShaderSourceTextureLod(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_CUBE_ARRAY, glu::TYPE_FLOAT, s_floatTypes[dataTypeNdx]));
							verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
						}
					}
					{
						const std::string shaderSource(genShaderSourceTextureLod(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE_ARRAY, glu::TYPE_FLOAT_VEC4, s_intTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTextureLod(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_CUBE_ARRAY, glu::TYPE_FLOAT_VEC4, s_intTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTextureLod(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_CUBE_ARRAY, glu::TYPE_FLOAT_VEC4, s_intTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTextureLod(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE_ARRAY, glu::TYPE_FLOAT_VEC4, s_uintTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTextureLod(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_CUBE_ARRAY, glu::TYPE_FLOAT_VEC4, s_uintTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTextureLod(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_CUBE_ARRAY, glu::TYPE_FLOAT_VEC4, s_uintTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
				}
				ctx.endSection();
			}
		}
		ctx.endSection();
	}
}

// texelFetch
std::string genShaderSourceTexelFetch (NegativeTestContext& ctx, glu::ShaderType shaderType, glu::DataType samplerDataType, glu::DataType pDataType, glu::DataType sampleDataType)
{
	std::ostringstream source;
	source	<< (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) ? glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES)) << "\n"
			<< getShaderExtensionDeclaration(getDataTypeExtension(samplerDataType))
			<< getShaderInitialization(ctx, shaderType)
			<< declareShaderUniform(samplerDataType, "sampler")
			<< "void main(void)\n"
			<< "{\n"
			<< "    " << declareAndInitializeShaderVariable(pDataType, "P");

	switch (samplerDataType)
	{
		case glu::TYPE_SAMPLER_2D_MULTISAMPLE:
		case glu::TYPE_INT_SAMPLER_2D_MULTISAMPLE:
		case glu::TYPE_UINT_SAMPLER_2D_MULTISAMPLE:
		case glu::TYPE_SAMPLER_2D_MULTISAMPLE_ARRAY:
		case glu::TYPE_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
		case glu::TYPE_UINT_SAMPLER_2D_MULTISAMPLE_ARRAY:
			source	<< "    " << declareAndInitializeShaderVariable(sampleDataType, "varSample")
					<< "    texelFetch(sampler, P, varSample);\n";
			break;

		case glu::TYPE_SAMPLER_BUFFER:
		case glu::TYPE_INT_SAMPLER_BUFFER:
		case glu::TYPE_UINT_SAMPLER_BUFFER:
			source << "    texelFetch(sampler, P);\n";
			break;

		default:
			DE_FATAL("Unsupported data type.");
	}

	source << "}\n";

	return source.str();
}

std::string genShaderSourceTexelFetch (NegativeTestContext& ctx, glu::ShaderType shaderType, glu::DataType samplerDataType, glu::DataType pDataType)
{
	return genShaderSourceTexelFetch(ctx, shaderType, samplerDataType, pDataType, glu::TYPE_LAST);
}

void texel_fetch_invalid_p (NegativeTestContext& ctx)
{
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_intTypes));
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_uintTypes));

	if (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) || ctx.isExtensionSupported(getDataTypeExtension(glu::TYPE_SAMPLER_CUBE_ARRAY)))
	{
		ctx.beginSection("texelFetch: Invalid P type.");
		for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
		{
			if (ctx.isShaderSupported(s_shaders[shaderNdx]))
			{
				ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
				for (int dataTypeNdx = 0; dataTypeNdx < DE_LENGTH_OF_ARRAY(s_floatTypes); ++dataTypeNdx)
				{
					// SAMPLER_2D_MULTISAMPLE
					{
						const std::string shaderSource(genShaderSourceTexelFetch(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D_MULTISAMPLE, s_floatTypes[dataTypeNdx], glu::TYPE_INT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexelFetch(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_2D_MULTISAMPLE, s_floatTypes[dataTypeNdx], glu::TYPE_INT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexelFetch(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_2D_MULTISAMPLE, s_floatTypes[dataTypeNdx], glu::TYPE_INT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}

					if (s_intTypes[dataTypeNdx] != glu::TYPE_INT_VEC2)
					{
						{
							const std::string shaderSource(genShaderSourceTexelFetch(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D_MULTISAMPLE, s_intTypes[dataTypeNdx], glu::TYPE_INT));
							verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
						}
						{
							const std::string shaderSource(genShaderSourceTexelFetch(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_2D_MULTISAMPLE, s_intTypes[dataTypeNdx], glu::TYPE_INT));
							verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
						}
						{
							const std::string shaderSource(genShaderSourceTexelFetch(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_2D_MULTISAMPLE, s_intTypes[dataTypeNdx], glu::TYPE_INT));
							verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
						}
					}
					{
						const std::string shaderSource(genShaderSourceTexelFetch(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D_MULTISAMPLE, s_uintTypes[dataTypeNdx], glu::TYPE_INT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexelFetch(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_2D_MULTISAMPLE, s_uintTypes[dataTypeNdx], glu::TYPE_INT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexelFetch(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_2D_MULTISAMPLE, s_uintTypes[dataTypeNdx], glu::TYPE_INT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}

					// SAMPLER_BUFFER
					{
						const std::string shaderSource(genShaderSourceTexelFetch(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_BUFFER, s_floatTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexelFetch(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_BUFFER, s_floatTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexelFetch(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_BUFFER, s_floatTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}

					if (s_intTypes[dataTypeNdx] != glu::TYPE_INT)
					{
						{
							const std::string shaderSource(genShaderSourceTexelFetch(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_BUFFER, s_intTypes[dataTypeNdx]));
							verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
						}
						{
							const std::string shaderSource(genShaderSourceTexelFetch(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_BUFFER, s_intTypes[dataTypeNdx]));
							verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
						}
						{
							const std::string shaderSource(genShaderSourceTexelFetch(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_BUFFER, s_intTypes[dataTypeNdx]));
							verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
						}
					}

					{
						const std::string shaderSource(genShaderSourceTexelFetch(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_BUFFER, s_uintTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexelFetch(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_BUFFER, s_uintTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexelFetch(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_BUFFER, s_uintTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}

					// SAMPLER_2D_MULTISAMPLE_ARRAY
					{
						const std::string shaderSource(genShaderSourceTexelFetch(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D_MULTISAMPLE_ARRAY, s_floatTypes[dataTypeNdx], glu::TYPE_INT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexelFetch(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_2D_MULTISAMPLE_ARRAY, s_floatTypes[dataTypeNdx], glu::TYPE_INT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexelFetch(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_2D_MULTISAMPLE_ARRAY, s_floatTypes[dataTypeNdx], glu::TYPE_INT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}

					if (s_intTypes[dataTypeNdx] != glu::TYPE_INT_VEC3)
					{
						{
							const std::string shaderSource(genShaderSourceTexelFetch(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D_MULTISAMPLE_ARRAY, s_intTypes[dataTypeNdx], glu::TYPE_INT));
							verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
						}
						{
							const std::string shaderSource(genShaderSourceTexelFetch(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_2D_MULTISAMPLE_ARRAY, s_intTypes[dataTypeNdx], glu::TYPE_INT));
							verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
						}
						{
							const std::string shaderSource(genShaderSourceTexelFetch(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_2D_MULTISAMPLE_ARRAY, s_intTypes[dataTypeNdx], glu::TYPE_INT));
							verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
						}
					}
					{
						const std::string shaderSource(genShaderSourceTexelFetch(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D_MULTISAMPLE_ARRAY, s_uintTypes[dataTypeNdx], glu::TYPE_INT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexelFetch(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_2D_MULTISAMPLE_ARRAY, s_uintTypes[dataTypeNdx], glu::TYPE_INT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexelFetch(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_2D_MULTISAMPLE_ARRAY, s_uintTypes[dataTypeNdx], glu::TYPE_INT));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
				}
				ctx.endSection();
			}
		}
		ctx.endSection();
	}
}

void texel_fetch_invalid_sample (NegativeTestContext& ctx)
{
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_intTypes));
	DE_ASSERT(DE_LENGTH_OF_ARRAY(s_floatTypes) == DE_LENGTH_OF_ARRAY(s_uintTypes));

	if (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) || ctx.isExtensionSupported(getDataTypeExtension(glu::TYPE_SAMPLER_CUBE_ARRAY)))
	{
		ctx.beginSection("texelFetch: Invalid sample type.");
		for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
		{
			if (ctx.isShaderSupported(s_shaders[shaderNdx]))
			{
				ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
				for (int dataTypeNdx = 0; dataTypeNdx < DE_LENGTH_OF_ARRAY(s_floatTypes); ++dataTypeNdx)
				{
					// SAMPLER_2D_MULTISAMPLE
					{
						const std::string shaderSource(genShaderSourceTexelFetch(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D_MULTISAMPLE, glu::TYPE_INT_VEC2, s_floatTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexelFetch(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_2D_MULTISAMPLE, glu::TYPE_INT_VEC2, s_floatTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexelFetch(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_2D_MULTISAMPLE, glu::TYPE_INT_VEC2, s_floatTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}

					// SAMPLER_2D_MULTISAMPLE_ARRAY
					{
						const std::string shaderSource(genShaderSourceTexelFetch(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D_MULTISAMPLE_ARRAY, glu::TYPE_INT_VEC3, s_floatTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexelFetch(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_2D_MULTISAMPLE_ARRAY, glu::TYPE_INT_VEC3, s_floatTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexelFetch(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_2D_MULTISAMPLE_ARRAY, glu::TYPE_INT_VEC3, s_floatTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}

					if (s_intTypes[dataTypeNdx] != glu::TYPE_INT)
					{
						// SAMPLER_2D_MULTISAMPLE
						{
							const std::string shaderSource(genShaderSourceTexelFetch(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D_MULTISAMPLE, glu::TYPE_INT_VEC2, s_intTypes[dataTypeNdx]));
							verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
						}
						{
							const std::string shaderSource(genShaderSourceTexelFetch(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_2D_MULTISAMPLE, glu::TYPE_INT_VEC2, s_intTypes[dataTypeNdx]));
							verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
						}
						{
							const std::string shaderSource(genShaderSourceTexelFetch(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_2D_MULTISAMPLE, glu::TYPE_INT_VEC2, s_intTypes[dataTypeNdx]));
							verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
						}

						// SAMPLER_2D_MULTISAMPLE_ARRAY
						{
							const std::string shaderSource(genShaderSourceTexelFetch(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D_MULTISAMPLE_ARRAY, glu::TYPE_INT_VEC3, s_intTypes[dataTypeNdx]));
							verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
						}
						{
							const std::string shaderSource(genShaderSourceTexelFetch(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_2D_MULTISAMPLE_ARRAY, glu::TYPE_INT_VEC3, s_intTypes[dataTypeNdx]));
							verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
						}
						{
							const std::string shaderSource(genShaderSourceTexelFetch(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_2D_MULTISAMPLE_ARRAY, glu::TYPE_INT_VEC3, s_intTypes[dataTypeNdx]));
							verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
						}
					}

					// SAMPLER_2D_MULTISAMPLE
					{
						const std::string shaderSource(genShaderSourceTexelFetch(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D_MULTISAMPLE, glu::TYPE_INT_VEC2, s_uintTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexelFetch(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_2D_MULTISAMPLE, glu::TYPE_INT_VEC2, s_uintTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexelFetch(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_2D_MULTISAMPLE, glu::TYPE_INT_VEC2, s_uintTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}

					// SAMPLER_2D_MULTISAMPLE_ARRAY
					{
						const std::string shaderSource(genShaderSourceTexelFetch(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D_MULTISAMPLE_ARRAY, glu::TYPE_INT_VEC3, s_uintTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexelFetch(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_2D_MULTISAMPLE_ARRAY, glu::TYPE_INT_VEC3, s_uintTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
					{
						const std::string shaderSource(genShaderSourceTexelFetch(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_2D_MULTISAMPLE_ARRAY, glu::TYPE_INT_VEC3, s_uintTypes[dataTypeNdx]));
						verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
					}
				}
				ctx.endSection();
			}
		}
		ctx.endSection();
	}
}

// EmitVertex, EndPrimitive
std::string genShaderSourceGeometry (NegativeTestContext& ctx, glu::ShaderType shaderType, ShaderFunction function)
{
	DE_ASSERT(function == SHADER_FUNCTION_EMIT_VERTEX || function == SHADER_FUNCTION_END_PRIMITIVE);

	std::ostringstream source;
	source	<< (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) ? glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES)) << "\n"
			<< getShaderInitialization(ctx, shaderType)
			<< "void main(void)\n"
			<< "{\n";

	switch (function)
	{
		case SHADER_FUNCTION_EMIT_VERTEX:
			source << "    EmitVertex();\n";
			break;

		case SHADER_FUNCTION_END_PRIMITIVE:
			source << "    EndPrimitive();\n";
			break;

		default:
			DE_FATAL("Unsupported shader function.");
	}

	source << "}\n";

	return source.str();
}

void emit_vertex (NegativeTestContext& ctx)
{
	ctx.beginSection("EmitVertex.");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			if (s_shaders[shaderNdx] == glu::SHADERTYPE_GEOMETRY)
				continue;

			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			const std::string shaderSource =	genShaderSourceGeometry(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_EMIT_VERTEX);
			verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void end_primitive (NegativeTestContext& ctx)
{
	ctx.beginSection("EndPrimitieve.");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			if (s_shaders[shaderNdx] == glu::SHADERTYPE_GEOMETRY)
				continue;

			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			const std::string shaderSource =	genShaderSourceGeometry(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_END_PRIMITIVE);
			verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			ctx.endSection();
		}
	}
	ctx.endSection();
}

// textureGrad
std::string genShaderSourceTextureGrad (NegativeTestContext& ctx, glu::ShaderType shaderType, glu::DataType samplerDataType, glu::DataType pDataType, glu::DataType dpdxDataType, glu::DataType dpdyDataType)
{
	std::ostringstream source;

	source	<< (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) ? glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES)) << "\n"
			<< getShaderExtensionDeclaration(getDataTypeExtension(samplerDataType))
			<< getShaderInitialization(ctx, shaderType)
			<< declareShaderUniform(samplerDataType, "sampler")
			<< "void main(void)\n"
			<< "{\n"
			<< "	mediump " << declareAndInitializeShaderVariable(pDataType, "P")
			<< "	mediump " << declareAndInitializeShaderVariable(dpdxDataType, "dPdx")
			<< "	mediump " << declareAndInitializeShaderVariable(dpdyDataType, "dPdy")
			<< "	textureGrad(sampler, P, dPdx, dPdy);\n"
			<< "}\n";

	return source.str();
}

void texture_grad (NegativeTestContext& ctx)
{
	TCU_CHECK_AND_THROW(NotSupportedError, ctx.isExtensionSupported("GL_EXT_texture_cube_map_array") || contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)), "Test requires support for GL_EXT_texture_cube_map_array or version 3.2.");

	ctx.beginSection("textureGrad.");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			{
				const std::string shaderSource(genShaderSourceTextureGrad(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE_ARRAY, glu::TYPE_FLOAT, glu::TYPE_FLOAT_VEC3, glu::TYPE_FLOAT_VEC3));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGrad(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE_ARRAY, glu::TYPE_FLOAT_VEC4, glu::TYPE_FLOAT, glu::TYPE_FLOAT_VEC3));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGrad(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE_ARRAY, glu::TYPE_FLOAT_VEC4, glu::TYPE_FLOAT_VEC3, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGrad(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_CUBE_ARRAY, glu::TYPE_FLOAT, glu::TYPE_FLOAT_VEC3, glu::TYPE_FLOAT_VEC3));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGrad(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_CUBE_ARRAY, glu::TYPE_FLOAT_VEC4, glu::TYPE_FLOAT, glu::TYPE_FLOAT_VEC3));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGrad(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_CUBE_ARRAY, glu::TYPE_FLOAT_VEC4, glu::TYPE_FLOAT_VEC3, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGrad(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_CUBE_ARRAY, glu::TYPE_FLOAT, glu::TYPE_FLOAT_VEC3, glu::TYPE_FLOAT_VEC3));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGrad(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_CUBE_ARRAY, glu::TYPE_FLOAT_VEC4, glu::TYPE_FLOAT, glu::TYPE_FLOAT_VEC3));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGrad(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_CUBE_ARRAY, glu::TYPE_FLOAT_VEC4, glu::TYPE_FLOAT_VEC3, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

// textureGather
std::string genShaderSourceTextureGather (NegativeTestContext& ctx, glu::ShaderType shaderType, glu::DataType samplerDataType, glu::DataType pDataType, glu::DataType thirdArgument)
{
	std::ostringstream source;

	source	<< (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) ? glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES)) << "\n"
			<< getShaderExtensionDeclaration(getDataTypeExtension(samplerDataType))
			<< getShaderInitialization(ctx, shaderType)
			<< declareShaderUniform(samplerDataType, "sampler")
			<< "void main(void)\n"
			<< "{\n"
			<< "	mediump " << declareAndInitializeShaderVariable(pDataType, "P");

	if (thirdArgument != glu::TYPE_LAST)
		source	<< "	mediump " << declareAndInitializeShaderVariable(thirdArgument, "arg3")
				<< "	textureGather(sampler, P, arg3);\n";
	else
		source << "	textureGather(sampler, P);\n";

	source << "}\n";

	return source.str();
}

std::string genShaderSourceTextureGather (NegativeTestContext& ctx, glu::ShaderType shaderType, glu::DataType samplerDataType, glu::DataType pDataType)
{
	return genShaderSourceTextureGather(ctx, shaderType, samplerDataType, pDataType, glu::TYPE_LAST);
}

void texture_gather_sampler_2d (NegativeTestContext& ctx)
{
	ctx.beginSection("textureGrad - sampler2D");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D, glu::TYPE_FLOAT_VEC2, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D, glu::TYPE_FLOAT, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_2D, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_2D, glu::TYPE_FLOAT_VEC2, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_2D, glu::TYPE_FLOAT, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_2D, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_2D, glu::TYPE_FLOAT_VEC2, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_2D, glu::TYPE_FLOAT, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_3D, glu::TYPE_FLOAT_VEC2));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_3D, glu::TYPE_FLOAT_VEC2, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void texture_gather_sampler_2d_array (NegativeTestContext& ctx)
{
	ctx.beginSection("textureGrad - sampler2DArray");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D_ARRAY, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D_ARRAY, glu::TYPE_FLOAT_VEC3, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D_ARRAY, glu::TYPE_FLOAT, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_2D_ARRAY, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_2D_ARRAY, glu::TYPE_FLOAT_VEC3, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_2D_ARRAY, glu::TYPE_FLOAT, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_2D_ARRAY, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_2D_ARRAY, glu::TYPE_FLOAT_VEC3, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_2D_ARRAY, glu::TYPE_FLOAT, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_3D, glu::TYPE_FLOAT_VEC3));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_3D, glu::TYPE_FLOAT_VEC3, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void texture_gather_sampler_cube (NegativeTestContext& ctx)
{
	ctx.beginSection("textureGrad - samplerCube");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE, glu::TYPE_FLOAT_VEC3, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE, glu::TYPE_FLOAT, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_CUBE, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_CUBE, glu::TYPE_FLOAT_VEC3, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_CUBE, glu::TYPE_FLOAT, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_CUBE, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_CUBE, glu::TYPE_FLOAT_VEC3, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_CUBE, glu::TYPE_FLOAT, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_3D, glu::TYPE_FLOAT_VEC3));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_3D, glu::TYPE_FLOAT_VEC3, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void texture_gather_sampler_2d_shadow (NegativeTestContext& ctx)
{
	ctx.beginSection("textureGrad - sampler2DShadow");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D_SHADOW, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D_SHADOW, glu::TYPE_FLOAT_VEC2, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D_SHADOW, glu::TYPE_FLOAT, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_3D, glu::TYPE_FLOAT_VEC2, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void texture_gather_sampler_2d_array_shadow (NegativeTestContext& ctx)
{
	ctx.beginSection("textureGrad - sampler2DArrayShadow");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D_ARRAY_SHADOW, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D_ARRAY_SHADOW, glu::TYPE_FLOAT_VEC3, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_2D_ARRAY_SHADOW, glu::TYPE_FLOAT, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_3D, glu::TYPE_FLOAT_VEC3, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void texture_gather_sampler_cube_shadow (NegativeTestContext& ctx)
{
	ctx.beginSection("textureGrad - samplerCubeShadow");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE_SHADOW, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE_SHADOW, glu::TYPE_FLOAT_VEC3, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE_SHADOW, glu::TYPE_FLOAT, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_3D, glu::TYPE_FLOAT_VEC3, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void texture_gather_sampler_cube_array (NegativeTestContext& ctx)
{
	TCU_CHECK_AND_THROW(
		NotSupportedError,
		contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) || ctx.isExtensionSupported("GL_EXT_texture_cube_map_array"),
		"Test requires extension GL_EXT_texture_cube_map_array or context version 3.2 or higher.");

	ctx.beginSection("textureGrad - samplerCubeArray");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE_ARRAY, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE_ARRAY, glu::TYPE_FLOAT, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE_ARRAY, glu::TYPE_FLOAT_VEC4, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_CUBE_ARRAY, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_CUBE_ARRAY, glu::TYPE_FLOAT, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_INT_SAMPLER_CUBE_ARRAY, glu::TYPE_FLOAT_VEC4, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_CUBE_ARRAY, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_CUBE_ARRAY, glu::TYPE_FLOAT, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_UINT_SAMPLER_CUBE_ARRAY, glu::TYPE_FLOAT_VEC4, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_3D, glu::TYPE_FLOAT_VEC4, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void texture_gather_sampler_cube_array_shadow (NegativeTestContext& ctx)
{
	TCU_CHECK_AND_THROW(
		NotSupportedError,
		contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) || ctx.isExtensionSupported("GL_EXT_texture_cube_map_array"),
		"Test requires extension GL_EXT_texture_cube_map_array or context version 3.2 or higher.");

	ctx.beginSection("textureGrad - samplerCubeArrayShadow");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE_ARRAY_SHADOW, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE_ARRAY_SHADOW, glu::TYPE_FLOAT, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_CUBE_ARRAY_SHADOW, glu::TYPE_FLOAT_VEC4, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGather(ctx, s_shaders[shaderNdx], glu::TYPE_SAMPLER_3D, glu::TYPE_FLOAT_VEC4, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

// textureGatherOffset
std::string genShaderSourceTextureGatherOffset (NegativeTestContext& ctx, glu::ShaderType shaderType, FunctionTextureGatherOffsetModes mode, glu::DataType samplerDataType, glu::DataType pDataType, glu::DataType offsetDataType, glu::DataType fourthArgument)
{
	DE_ASSERT(mode < FUNCTION_TEXTURE_GATHER_OFFSET_MODE_LAST);

	std::ostringstream source;

	source	<< (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) ? glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES)) << "\n"
			<< getShaderInitialization(ctx, shaderType)
			<< declareShaderUniform(samplerDataType, "sampler")
			<< "void main(void)\n"
			<< "{\n"
			<< "	mediump " << declareAndInitializeShaderVariable(pDataType, "P")
			<< "	mediump " << declareAndInitializeShaderVariable(offsetDataType, "offset");

	switch (mode)
	{
		case FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP:
		{
			if (fourthArgument != glu::TYPE_LAST)
				source	<< "	mediump " << declareAndInitializeShaderVariable(fourthArgument, "comp")
						<< "	textureGatherOffset(sampler, P, offset, comp);\n";
			else
				source << "	textureGatherOffset(sampler, P, offset);\n";
			break;
		}

		case FUNCTION_TEXTURE_GATHER_OFFSET_MODE_REF_Z:
		{
			source	<< "	mediump " << declareAndInitializeShaderVariable(fourthArgument, "refZ")
					<< "	textureGatherOffset(sampler, P, refZ, offset);\n";
			break;
		}

		default:
			DE_FATAL("Unsupported shader function overload.");
	}

	source << "}\n";

	return source.str();
}

std::string genShaderSourceTextureGatherOffset (NegativeTestContext& ctx, glu::ShaderType shaderType, FunctionTextureGatherOffsetModes mode, glu::DataType samplerDataType, glu::DataType pDataType, glu::DataType offsetDataType)
{
	DE_ASSERT(mode == FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP);

	return genShaderSourceTextureGatherOffset(ctx, shaderType, mode, samplerDataType, pDataType, offsetDataType, glu::TYPE_LAST);
}

void texture_gather_offset_sampler_2d (NegativeTestContext& ctx)
{
	ctx.beginSection("textureGatherOffset - sampler2D");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			{
				const std::string shaderSource(genShaderSourceTextureGatherOffset(ctx, s_shaders[shaderNdx], FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP, glu::TYPE_SAMPLER_2D, glu::TYPE_FLOAT, glu::TYPE_INT_VEC2));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGatherOffset(ctx, s_shaders[shaderNdx], FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP, glu::TYPE_SAMPLER_2D, glu::TYPE_FLOAT_VEC2, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGatherOffset(ctx, s_shaders[shaderNdx], FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP, glu::TYPE_SAMPLER_2D, glu::TYPE_FLOAT, glu::TYPE_INT_VEC2, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGatherOffset(ctx, s_shaders[shaderNdx], FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP, glu::TYPE_SAMPLER_2D, glu::TYPE_FLOAT_VEC2, glu::TYPE_INT, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGatherOffset(ctx, s_shaders[shaderNdx], FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP, glu::TYPE_SAMPLER_2D, glu::TYPE_FLOAT_VEC2, glu::TYPE_INT_VEC2, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGatherOffset(ctx, s_shaders[shaderNdx], FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP, glu::TYPE_INT_SAMPLER_2D, glu::TYPE_FLOAT, glu::TYPE_INT_VEC2));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGatherOffset(ctx, s_shaders[shaderNdx], FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP, glu::TYPE_INT_SAMPLER_2D, glu::TYPE_FLOAT_VEC2, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGatherOffset(ctx, s_shaders[shaderNdx], FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP, glu::TYPE_INT_SAMPLER_2D, glu::TYPE_FLOAT, glu::TYPE_INT_VEC2, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGatherOffset(ctx, s_shaders[shaderNdx], FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP, glu::TYPE_INT_SAMPLER_2D, glu::TYPE_FLOAT_VEC2, glu::TYPE_INT, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGatherOffset(ctx, s_shaders[shaderNdx], FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP, glu::TYPE_INT_SAMPLER_2D, glu::TYPE_FLOAT_VEC2, glu::TYPE_INT_VEC2, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGatherOffset(ctx, s_shaders[shaderNdx], FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP, glu::TYPE_UINT_SAMPLER_2D, glu::TYPE_FLOAT, glu::TYPE_INT_VEC2));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGatherOffset(ctx, s_shaders[shaderNdx], FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP, glu::TYPE_UINT_SAMPLER_2D, glu::TYPE_FLOAT_VEC2, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGatherOffset(ctx, s_shaders[shaderNdx], FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP, glu::TYPE_UINT_SAMPLER_2D, glu::TYPE_FLOAT, glu::TYPE_INT_VEC2, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGatherOffset(ctx, s_shaders[shaderNdx], FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP, glu::TYPE_UINT_SAMPLER_2D, glu::TYPE_FLOAT_VEC2, glu::TYPE_INT, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGatherOffset(ctx, s_shaders[shaderNdx], FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP, glu::TYPE_UINT_SAMPLER_2D, glu::TYPE_FLOAT_VEC2, glu::TYPE_INT_VEC2, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGatherOffset(ctx, s_shaders[shaderNdx], FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP, glu::TYPE_SAMPLER_3D, glu::TYPE_FLOAT_VEC2, glu::TYPE_INT_VEC2));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGatherOffset(ctx, s_shaders[shaderNdx], FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP, glu::TYPE_SAMPLER_3D, glu::TYPE_FLOAT_VEC2, glu::TYPE_INT_VEC2, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void texture_gather_offset_sampler_2d_array (NegativeTestContext& ctx)
{
	ctx.beginSection("textureGatherOffset - sampler2DArray");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			{
				const std::string shaderSource(genShaderSourceTextureGatherOffset(ctx, s_shaders[shaderNdx], FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP, glu::TYPE_SAMPLER_2D_ARRAY, glu::TYPE_FLOAT, glu::TYPE_INT_VEC2));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGatherOffset(ctx, s_shaders[shaderNdx], FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP, glu::TYPE_SAMPLER_2D_ARRAY, glu::TYPE_FLOAT_VEC3, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGatherOffset(ctx, s_shaders[shaderNdx], FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP, glu::TYPE_SAMPLER_2D_ARRAY, glu::TYPE_FLOAT, glu::TYPE_INT_VEC2, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGatherOffset(ctx, s_shaders[shaderNdx], FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP, glu::TYPE_SAMPLER_2D_ARRAY, glu::TYPE_FLOAT_VEC3, glu::TYPE_INT, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGatherOffset(ctx, s_shaders[shaderNdx], FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP, glu::TYPE_SAMPLER_2D_ARRAY, glu::TYPE_FLOAT_VEC3, glu::TYPE_INT, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGatherOffset(ctx, s_shaders[shaderNdx], FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP, glu::TYPE_INT_SAMPLER_2D_ARRAY, glu::TYPE_FLOAT, glu::TYPE_INT_VEC2));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGatherOffset(ctx, s_shaders[shaderNdx], FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP, glu::TYPE_INT_SAMPLER_2D_ARRAY, glu::TYPE_FLOAT_VEC3, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGatherOffset(ctx, s_shaders[shaderNdx], FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP, glu::TYPE_INT_SAMPLER_2D_ARRAY, glu::TYPE_FLOAT, glu::TYPE_INT_VEC2, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGatherOffset(ctx, s_shaders[shaderNdx], FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP, glu::TYPE_INT_SAMPLER_2D_ARRAY, glu::TYPE_FLOAT_VEC3, glu::TYPE_INT, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGatherOffset(ctx, s_shaders[shaderNdx], FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP, glu::TYPE_INT_SAMPLER_2D_ARRAY, glu::TYPE_FLOAT_VEC3, glu::TYPE_INT, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGatherOffset(ctx, s_shaders[shaderNdx], FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP, glu::TYPE_UINT_SAMPLER_2D_ARRAY, glu::TYPE_FLOAT, glu::TYPE_INT_VEC2));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGatherOffset(ctx, s_shaders[shaderNdx], FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP, glu::TYPE_UINT_SAMPLER_2D_ARRAY, glu::TYPE_FLOAT_VEC3, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGatherOffset(ctx, s_shaders[shaderNdx], FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP, glu::TYPE_UINT_SAMPLER_2D_ARRAY, glu::TYPE_FLOAT, glu::TYPE_INT_VEC2, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGatherOffset(ctx, s_shaders[shaderNdx], FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP, glu::TYPE_UINT_SAMPLER_2D_ARRAY, glu::TYPE_FLOAT_VEC3, glu::TYPE_INT, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGatherOffset(ctx, s_shaders[shaderNdx], FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP, glu::TYPE_UINT_SAMPLER_2D_ARRAY, glu::TYPE_FLOAT_VEC3, glu::TYPE_INT, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGatherOffset(ctx, s_shaders[shaderNdx], FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP, glu::TYPE_UINT_SAMPLER_3D, glu::TYPE_FLOAT_VEC3, glu::TYPE_INT_VEC2, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void texture_gather_offset_sampler_2d_shadow (NegativeTestContext& ctx)
{
	ctx.beginSection("textureGatherOffset - sampler2DShadow");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			{
				const std::string shaderSource(genShaderSourceTextureGatherOffset(ctx, s_shaders[shaderNdx], FUNCTION_TEXTURE_GATHER_OFFSET_MODE_REF_Z, glu::TYPE_SAMPLER_2D_SHADOW, glu::TYPE_FLOAT, glu::TYPE_INT_VEC2, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGatherOffset(ctx, s_shaders[shaderNdx], FUNCTION_TEXTURE_GATHER_OFFSET_MODE_REF_Z, glu::TYPE_SAMPLER_2D_SHADOW, glu::TYPE_FLOAT_VEC2, glu::TYPE_INT_VEC2, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGatherOffset(ctx, s_shaders[shaderNdx], FUNCTION_TEXTURE_GATHER_OFFSET_MODE_REF_Z, glu::TYPE_SAMPLER_2D_SHADOW, glu::TYPE_FLOAT_VEC2, glu::TYPE_INT, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGatherOffset(ctx, s_shaders[shaderNdx], FUNCTION_TEXTURE_GATHER_OFFSET_MODE_REF_Z, glu::TYPE_SAMPLER_3D, glu::TYPE_FLOAT_VEC2, glu::TYPE_INT_VEC2, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void texture_gather_offset_sampler_2d_array_shadow (NegativeTestContext& ctx)
{
	ctx.beginSection("textureGatherOffset - sampler2DShadow");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			{
				const std::string shaderSource(genShaderSourceTextureGatherOffset(ctx, s_shaders[shaderNdx], FUNCTION_TEXTURE_GATHER_OFFSET_MODE_REF_Z, glu::TYPE_SAMPLER_2D_ARRAY_SHADOW, glu::TYPE_FLOAT, glu::TYPE_INT_VEC2, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGatherOffset(ctx, s_shaders[shaderNdx], FUNCTION_TEXTURE_GATHER_OFFSET_MODE_REF_Z, glu::TYPE_SAMPLER_2D_ARRAY_SHADOW, glu::TYPE_FLOAT_VEC3, glu::TYPE_INT_VEC2, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGatherOffset(ctx, s_shaders[shaderNdx], FUNCTION_TEXTURE_GATHER_OFFSET_MODE_REF_Z, glu::TYPE_SAMPLER_2D_ARRAY_SHADOW, glu::TYPE_FLOAT_VEC3, glu::TYPE_INT, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceTextureGatherOffset(ctx, s_shaders[shaderNdx], FUNCTION_TEXTURE_GATHER_OFFSET_MODE_REF_Z, glu::TYPE_SAMPLER_3D, glu::TYPE_FLOAT_VEC3, glu::TYPE_INT_VEC2, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

// atomicAdd, atomicMin, atomicMax, atomicAnd, atomicOr, atomixXor, atomixExchange, atomicCompSwap
std::string genShaderSourceAtomicOperations (NegativeTestContext& ctx, glu::ShaderType shaderType, ShaderFunction function, glu::DataType memDataType, glu::DataType dataDataType, glu::DataType compareDataType)
{
	DE_ASSERT(SHADER_FUNCTION_ATOMIC_ADD <= function && function <= SHADER_FUNCTION_ATOMIC_COMP_SWAP);

	std::ostringstream source;

	source	<< (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) ? glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES)) << "\n"
			<< getShaderInitialization(ctx, shaderType)
			<< declareBuffer(memDataType, "mem")
			<< "void main()\n"
			<< "{\n"
			<< "	mediump " << declareAndInitializeShaderVariable(dataDataType, "data");

	switch (function)
	{
		case SHADER_FUNCTION_ATOMIC_ADD:		source << "    atomicAdd(mem, data);\n";		break;
		case SHADER_FUNCTION_ATOMIC_MIN:		source << "    atomicMin(mem, data);\n";		break;
		case SHADER_FUNCTION_ATOMIC_MAX:		source << "    atomicMax(mem, data);\n";		break;
		case SHADER_FUNCTION_ATOMIC_AND:		source << "    atomicAnd(mem, data);\n";		break;
		case SHADER_FUNCTION_ATOMIC_OR:			source << "    atomicOr(mem, data);\n";			break;
		case SHADER_FUNCTION_ATOMIC_XOR:		source << "    atomicXor(mem, data);\n";		break;
		case SHADER_FUNCTION_ATOMIC_EXCHANGE:	source << "    atomicExchange(mem, data);\n";	break;
		case SHADER_FUNCTION_ATOMIC_COMP_SWAP:
			source	<< "	mediump " << declareAndInitializeShaderVariable(compareDataType, "compare")
					<< "    atomicCompSwap(mem, compare, data);\n";
			break;

		default:
			DE_FATAL("Unsupported shader function.");
	}

	source << "}\n";

	return source.str();
}

std::string genShaderSourceAtomicOperations (NegativeTestContext& ctx, glu::ShaderType shaderType, ShaderFunction function, glu::DataType memDataType, glu::DataType dataDataType)
{
	DE_ASSERT(function != SHADER_FUNCTION_ATOMIC_COMP_SWAP);

	return genShaderSourceAtomicOperations(ctx, shaderType, function, memDataType, dataDataType, glu::TYPE_LAST);
}

void atomic_add (NegativeTestContext& ctx)
{
	ctx.beginSection("atomicAdd");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			{
				const std::string shaderSource(genShaderSourceAtomicOperations(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_ATOMIC_ADD, glu::TYPE_UINT, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceAtomicOperations(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_ATOMIC_ADD, glu::TYPE_INT, glu::TYPE_UINT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void atomic_min (NegativeTestContext& ctx)
{
	ctx.beginSection("atomicMin");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			{
				const std::string shaderSource(genShaderSourceAtomicOperations(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_ATOMIC_MIN, glu::TYPE_UINT, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceAtomicOperations(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_ATOMIC_MIN, glu::TYPE_INT, glu::TYPE_UINT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void atomic_max (NegativeTestContext& ctx)
{
	ctx.beginSection("atomicMax");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			{
				const std::string shaderSource(genShaderSourceAtomicOperations(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_ATOMIC_MAX, glu::TYPE_UINT, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceAtomicOperations(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_ATOMIC_MAX, glu::TYPE_INT, glu::TYPE_UINT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void atomic_and (NegativeTestContext& ctx)
{
	ctx.beginSection("atomicAnd");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			{
				const std::string shaderSource(genShaderSourceAtomicOperations(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_ATOMIC_AND, glu::TYPE_UINT, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceAtomicOperations(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_ATOMIC_AND, glu::TYPE_INT, glu::TYPE_UINT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void atomic_or (NegativeTestContext& ctx)
{
	ctx.beginSection("atomicOr");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			{
				const std::string shaderSource(genShaderSourceAtomicOperations(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_ATOMIC_OR, glu::TYPE_UINT, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceAtomicOperations(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_ATOMIC_OR, glu::TYPE_INT, glu::TYPE_UINT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void atomic_xor (NegativeTestContext& ctx)
{
	ctx.beginSection("atomicXor");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			{
				const std::string shaderSource(genShaderSourceAtomicOperations(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_ATOMIC_XOR, glu::TYPE_UINT, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceAtomicOperations(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_ATOMIC_XOR, glu::TYPE_INT, glu::TYPE_UINT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void atomic_exchange (NegativeTestContext& ctx)
{
	ctx.beginSection("atomicExchange");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			{
				const std::string shaderSource(genShaderSourceAtomicOperations(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_ATOMIC_EXCHANGE, glu::TYPE_UINT, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceAtomicOperations(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_ATOMIC_EXCHANGE, glu::TYPE_INT, glu::TYPE_UINT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void atomic_comp_swap (NegativeTestContext& ctx)
{
	ctx.beginSection("atomicCompSwap");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			{
				const std::string shaderSource(genShaderSourceAtomicOperations(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_ATOMIC_COMP_SWAP, glu::TYPE_UINT, glu::TYPE_INT, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceAtomicOperations(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_ATOMIC_COMP_SWAP, glu::TYPE_INT, glu::TYPE_UINT, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceAtomicOperations(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_ATOMIC_COMP_SWAP, glu::TYPE_INT, glu::TYPE_INT, glu::TYPE_UINT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

// interpolatedAtCentroid, interpolatedAtSample, interpolateAtOffset,
std::string genShaderSourceInterpolateAt (NegativeTestContext& ctx, glu::ShaderType shaderType, ShaderFunction function, glu::DataType interpolantDataType, glu::DataType secondArgumentDataType)
{
	DE_ASSERT(function >= SHADER_FUNCTION_INTERPOLATED_AT_CENTROID && function <= SHADER_FUNCTION_INTERPOLATED_AT_OFFSET);

	const bool			supportsES32 = contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2));
	std::ostringstream	source;

	source	<< (supportsES32 ? glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES)) << "\n"
			<< getShaderInitialization(ctx, shaderType)
			<< (supportsES32 ? "" : getShaderExtensionDeclaration("GL_OES_shader_multisample_interpolation"))
			<< declareShaderInput(interpolantDataType, "interpolant")
			<< "void main()\n"
			<< "{\n";

	switch (function)
	{
		case SHADER_FUNCTION_INTERPOLATED_AT_CENTROID:
			source << "    interpolateAtCentroid(interpolant);\n";
			break;

		case SHADER_FUNCTION_INTERPOLATED_AT_SAMPLE:
			source	<< "	mediump " << declareAndInitializeShaderVariable(secondArgumentDataType, "sample")
					<< "    interpolateAtSample(interpolant, sample);\n";
			break;

		case SHADER_FUNCTION_INTERPOLATED_AT_OFFSET:
			source	<< "	mediump " << declareAndInitializeShaderVariable(secondArgumentDataType, "offset")
					<< "    interpolateAtOffset(interpolant, offset);\n";
			break;

		default:
			DE_FATAL("Unsupported shader function.");
	}

	source << "}\n";

	return source.str();
}

std::string genShaderSourceInterpolateAt (NegativeTestContext& ctx, glu::ShaderType shaderType, ShaderFunction function, glu::DataType interpolantDataType)
{
	DE_ASSERT(function == SHADER_FUNCTION_INTERPOLATED_AT_CENTROID);

	return genShaderSourceInterpolateAt(ctx, shaderType, function, interpolantDataType, glu::TYPE_LAST);
}

void interpolate_at_centroid (NegativeTestContext& ctx)
{
	TCU_CHECK_AND_THROW(NotSupportedError,
		contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) || ctx.isExtensionSupported("GL_OES_shader_multisample_interpolation"),
		"This test requires a context version 3.2 or higher.");

	ctx.beginSection("interpolateAtCentroid");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			if (s_shaders[shaderNdx] == glu::SHADERTYPE_FRAGMENT)
			{
				const std::string shaderSource(genShaderSourceInterpolateAt(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_INTERPOLATED_AT_CENTROID, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			else
			{
				{
					const std::string shaderSource(genShaderSourceInterpolateAt(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_INTERPOLATED_AT_CENTROID, glu::TYPE_FLOAT));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceInterpolateAt(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_INTERPOLATED_AT_CENTROID, glu::TYPE_FLOAT_VEC2));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceInterpolateAt(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_INTERPOLATED_AT_CENTROID, glu::TYPE_FLOAT_VEC3));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceInterpolateAt(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_INTERPOLATED_AT_CENTROID, glu::TYPE_FLOAT_VEC4));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void interpolate_at_sample (NegativeTestContext& ctx)
{
	TCU_CHECK_AND_THROW(NotSupportedError,
		contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) || ctx.isExtensionSupported("GL_OES_shader_multisample_interpolation"),
		"This test requires a context version 3.2 or higher.");

	ctx.beginSection("interpolateAtSample");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			if (s_shaders[shaderNdx] == glu::SHADERTYPE_FRAGMENT)
			{
				{
					const std::string shaderSource(genShaderSourceInterpolateAt(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_INTERPOLATED_AT_SAMPLE, glu::TYPE_INT, glu::TYPE_INT));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceInterpolateAt(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_INTERPOLATED_AT_SAMPLE, glu::TYPE_FLOAT, glu::TYPE_FLOAT));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceInterpolateAt(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_INTERPOLATED_AT_SAMPLE, glu::TYPE_FLOAT_VEC2, glu::TYPE_FLOAT));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceInterpolateAt(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_INTERPOLATED_AT_SAMPLE, glu::TYPE_FLOAT_VEC3, glu::TYPE_FLOAT));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceInterpolateAt(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_INTERPOLATED_AT_SAMPLE, glu::TYPE_FLOAT_VEC4, glu::TYPE_FLOAT));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
			}
			else
			{
				{
					const std::string shaderSource(genShaderSourceInterpolateAt(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_INTERPOLATED_AT_SAMPLE, glu::TYPE_FLOAT, glu::TYPE_INT));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceInterpolateAt(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_INTERPOLATED_AT_SAMPLE, glu::TYPE_FLOAT_VEC2, glu::TYPE_INT));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceInterpolateAt(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_INTERPOLATED_AT_SAMPLE, glu::TYPE_FLOAT_VEC3, glu::TYPE_INT));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceInterpolateAt(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_INTERPOLATED_AT_SAMPLE, glu::TYPE_FLOAT_VEC4, glu::TYPE_INT));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

void interpolate_at_offset (NegativeTestContext& ctx)
{
	TCU_CHECK_AND_THROW(NotSupportedError,
		contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) || ctx.isExtensionSupported("GL_OES_shader_multisample_interpolation"),
		"This test requires a context version 3.2 or higher.");

	ctx.beginSection("interpolateAtOffset");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			if (s_shaders[shaderNdx] == glu::SHADERTYPE_FRAGMENT)
			{
				{
					const std::string shaderSource(genShaderSourceInterpolateAt(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_INTERPOLATED_AT_OFFSET, glu::TYPE_INT, glu::TYPE_FLOAT_VEC2));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceInterpolateAt(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_INTERPOLATED_AT_OFFSET, glu::TYPE_FLOAT, glu::TYPE_FLOAT));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceInterpolateAt(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_INTERPOLATED_AT_OFFSET, glu::TYPE_FLOAT_VEC2, glu::TYPE_FLOAT));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceInterpolateAt(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_INTERPOLATED_AT_OFFSET, glu::TYPE_FLOAT_VEC3, glu::TYPE_FLOAT));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceInterpolateAt(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_INTERPOLATED_AT_OFFSET, glu::TYPE_FLOAT_VEC4, glu::TYPE_FLOAT));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
			}
			else
			{
				{
					const std::string shaderSource(genShaderSourceInterpolateAt(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_INTERPOLATED_AT_OFFSET, glu::TYPE_FLOAT, glu::TYPE_FLOAT_VEC2));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceInterpolateAt(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_INTERPOLATED_AT_OFFSET, glu::TYPE_FLOAT_VEC2, glu::TYPE_FLOAT_VEC2));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceInterpolateAt(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_INTERPOLATED_AT_OFFSET, glu::TYPE_FLOAT_VEC3, glu::TYPE_FLOAT_VEC2));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
				{
					const std::string shaderSource(genShaderSourceInterpolateAt(ctx, s_shaders[shaderNdx], SHADER_FUNCTION_INTERPOLATED_AT_OFFSET, glu::TYPE_FLOAT_VEC4, glu::TYPE_FLOAT_VEC2));
					verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
				}
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}


// textureGatherOffsets
std::string genShaderSourceTextureGatherOffsets (NegativeTestContext& ctx, glu::ShaderType shaderType, const TextureGatherOffsetsTestSpec& spec)
{
	std::ostringstream source;

	source	<< (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) ? glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES)) << "\n"
			<< getShaderInitialization(ctx, shaderType)
			<< declareShaderUniform(spec.samplerDataType, "sampler")
			<< "void main(void)\n"
			<< "{\n"
			<< "	mediump " << declareAndInitializeShaderVariable(spec.pDataType, "P")
			<< "    mediump " << (spec.offsetIsConst ? "const " : "") << declareShaderArrayVariable(spec.offsetsDataType, "offsets", spec.offsetArraySize) << "\n";

	switch (spec.mode)
	{
		case FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP:
		{
			if (spec.fourthArgument != glu::TYPE_LAST)
				source	<< "	mediump " << declareAndInitializeShaderVariable(spec.fourthArgument, "comp")
						<< "	textureGatherOffsets(sampler, P, offsets, comp);\n";
			else
				source << "	textureGatherOffsets(sampler, P, offsets);\n";
			break;
		}

		case FUNCTION_TEXTURE_GATHER_OFFSET_MODE_REF_Z:
		{
			source	<< "	mediump " << declareAndInitializeShaderVariable(spec.fourthArgument, "refZ")
					<< "	textureGatherOffsets(sampler, P, refZ, offsets);\n";
			break;
		}

		default:
			DE_FATAL("Unsupported shader function overload.");
			break;
	}

	source << "}\n";
	return source.str();
}

void texture_gather_offsets (NegativeTestContext& ctx)
{
	TCU_CHECK_AND_THROW(NotSupportedError,
		contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) || ctx.isExtensionSupported("GL_EXT_gpu_shader5"),
		"This test requires a context version 3.2 or higher.");

	const struct TextureGatherOffsetsTestSpec testSpecs[] =
	{
			//mode										samplerDataType						pDataType				offsetsDataType			fourthArgument		offsetIsConst	offsetArraySize
		{	FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP,	glu::TYPE_SAMPLER_2D,				glu::TYPE_FLOAT_VEC2,	glu::TYPE_INT_VEC2,		glu::TYPE_LAST,		false,			4,		},
		{	FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP,	glu::TYPE_SAMPLER_2D,				glu::TYPE_FLOAT_VEC2,	glu::TYPE_INT_VEC2,		glu::TYPE_LAST,		true,			3,		},
		{	FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP,	glu::TYPE_SAMPLER_2D,				glu::TYPE_FLOAT,		glu::TYPE_INT_VEC2,		glu::TYPE_LAST,		true,			4,		},
		{	FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP,	glu::TYPE_SAMPLER_2D,				glu::TYPE_FLOAT_VEC2,	glu::TYPE_INT,			glu::TYPE_LAST,		true,			4,		},
		{	FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP,	glu::TYPE_SAMPLER_2D,				glu::TYPE_FLOAT_VEC2,	glu::TYPE_INT_VEC2,		glu::TYPE_INT,		false,			4,		},
		{	FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP,	glu::TYPE_SAMPLER_2D,				glu::TYPE_FLOAT_VEC2,	glu::TYPE_INT_VEC2,		glu::TYPE_FLOAT,	true,			4,		},
		{	FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP,	glu::TYPE_SAMPLER_2D,				glu::TYPE_FLOAT_VEC2,	glu::TYPE_INT_VEC2,		glu::TYPE_INT,		true,			3,		},
		{	FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP,	glu::TYPE_SAMPLER_2D,				glu::TYPE_FLOAT,		glu::TYPE_INT_VEC2,		glu::TYPE_INT,		true,			4,		},
		{	FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP,	glu::TYPE_SAMPLER_2D,				glu::TYPE_FLOAT_VEC2,	glu::TYPE_INT,			glu::TYPE_INT,		true,			4,		},

		{	FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP,	glu::TYPE_SAMPLER_2D_ARRAY,			glu::TYPE_FLOAT_VEC3,	glu::TYPE_INT_VEC2,		glu::TYPE_LAST,		false,			4,		},
		{	FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP,	glu::TYPE_SAMPLER_2D_ARRAY,			glu::TYPE_FLOAT_VEC3,	glu::TYPE_INT_VEC2,		glu::TYPE_LAST,		true,			3,		},
		{	FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP,	glu::TYPE_SAMPLER_2D_ARRAY,			glu::TYPE_FLOAT,		glu::TYPE_INT_VEC2,		glu::TYPE_LAST,		true,			4,		},
		{	FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP,	glu::TYPE_SAMPLER_2D_ARRAY,			glu::TYPE_FLOAT_VEC3,	glu::TYPE_INT,			glu::TYPE_LAST,		true,			4,		},
		{	FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP,	glu::TYPE_SAMPLER_2D_ARRAY,			glu::TYPE_FLOAT_VEC3,	glu::TYPE_INT_VEC2,		glu::TYPE_INT,		false,			4,		},
		{	FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP,	glu::TYPE_SAMPLER_2D_ARRAY,			glu::TYPE_FLOAT_VEC3,	glu::TYPE_INT_VEC2,		glu::TYPE_FLOAT,	true,			4,		},
		{	FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP,	glu::TYPE_SAMPLER_2D_ARRAY,			glu::TYPE_FLOAT_VEC3,	glu::TYPE_INT_VEC2,		glu::TYPE_INT,		true,			3,		},
		{	FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP,	glu::TYPE_SAMPLER_2D_ARRAY,			glu::TYPE_FLOAT,		glu::TYPE_INT_VEC2,		glu::TYPE_INT,		true,			4,		},
		{	FUNCTION_TEXTURE_GATHER_OFFSET_MODE_COMP,	glu::TYPE_SAMPLER_2D_ARRAY,			glu::TYPE_FLOAT_VEC3,	glu::TYPE_INT,			glu::TYPE_INT,		true,			4,		},

		{	FUNCTION_TEXTURE_GATHER_OFFSET_MODE_REF_Z,	glu::TYPE_SAMPLER_2D_SHADOW,		glu::TYPE_FLOAT_VEC2,	glu::TYPE_INT_VEC2,		glu::TYPE_FLOAT,	false,			4,		},
		{	FUNCTION_TEXTURE_GATHER_OFFSET_MODE_REF_Z,	glu::TYPE_SAMPLER_2D_SHADOW,		glu::TYPE_FLOAT_VEC2,	glu::TYPE_INT_VEC2,		glu::TYPE_FLOAT,	true,			3,		},
		{	FUNCTION_TEXTURE_GATHER_OFFSET_MODE_REF_Z,	glu::TYPE_SAMPLER_2D_SHADOW,		glu::TYPE_FLOAT,		glu::TYPE_INT_VEC2,		glu::TYPE_FLOAT,	true,			4,		},
		{	FUNCTION_TEXTURE_GATHER_OFFSET_MODE_REF_Z,	glu::TYPE_SAMPLER_2D_SHADOW,		glu::TYPE_FLOAT_VEC2,	glu::TYPE_INT,			glu::TYPE_FLOAT,	true,			4,		},
		{	FUNCTION_TEXTURE_GATHER_OFFSET_MODE_REF_Z,	glu::TYPE_SAMPLER_2D_SHADOW,		glu::TYPE_FLOAT_VEC2,	glu::TYPE_INT_VEC2,		glu::TYPE_INT,		true,			4,		},

		{	FUNCTION_TEXTURE_GATHER_OFFSET_MODE_REF_Z,	glu::TYPE_SAMPLER_2D_ARRAY_SHADOW,	glu::TYPE_FLOAT_VEC3,	glu::TYPE_INT_VEC2,		glu::TYPE_FLOAT,	false,			4,		},
		{	FUNCTION_TEXTURE_GATHER_OFFSET_MODE_REF_Z,	glu::TYPE_SAMPLER_2D_ARRAY_SHADOW,	glu::TYPE_FLOAT_VEC3,	glu::TYPE_INT_VEC2,		glu::TYPE_FLOAT,	true,			3,		},
		{	FUNCTION_TEXTURE_GATHER_OFFSET_MODE_REF_Z,	glu::TYPE_SAMPLER_2D_ARRAY_SHADOW,	glu::TYPE_FLOAT,		glu::TYPE_INT_VEC2,		glu::TYPE_FLOAT,	true,			4,		},
		{	FUNCTION_TEXTURE_GATHER_OFFSET_MODE_REF_Z,	glu::TYPE_SAMPLER_2D_ARRAY_SHADOW,	glu::TYPE_FLOAT_VEC3,	glu::TYPE_INT,			glu::TYPE_FLOAT,	true,			4,		},
		{	FUNCTION_TEXTURE_GATHER_OFFSET_MODE_REF_Z,	glu::TYPE_SAMPLER_2D_ARRAY_SHADOW,	glu::TYPE_FLOAT_VEC3,	glu::TYPE_INT_VEC2,		glu::TYPE_INT,		true,			4,		},
	};

	ctx.beginSection("textureGatherOffsets");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			for (int specNdx = 0; specNdx < DE_LENGTH_OF_ARRAY(testSpecs); ++specNdx)
			{
				const std::string shaderSource(genShaderSourceTextureGatherOffsets(ctx, s_shaders[shaderNdx], testSpecs[specNdx]));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

// fma
std::string genShaderSourceFma (NegativeTestContext& ctx, glu::ShaderType shaderType, glu::DataType aDataType, glu::DataType bDataType, glu::DataType cDataType)
{
	std::ostringstream source;

	source	<< (contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) ? glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES)) << "\n"
			<< getShaderInitialization(ctx, shaderType)
			<< "void main(void)\n"
			<< "{\n"
			<< "	mediump " << declareAndInitializeShaderVariable(aDataType, "a")
			<< "	mediump " << declareAndInitializeShaderVariable(bDataType, "b")
			<< "	mediump " << declareAndInitializeShaderVariable(cDataType, "c")
			<< "	fma(a, b, c);"
			<< "}\n";
	return source.str();
}

void fma (NegativeTestContext& ctx)
{
	TCU_CHECK_AND_THROW(NotSupportedError,
		contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)) || ctx.isExtensionSupported("GL_EXT_gpu_shader5"),
		"This test requires a context version 3.2 or higher.");

	ctx.beginSection("fma");
	for (int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(s_shaders); ++shaderNdx)
	{
		if (ctx.isShaderSupported(s_shaders[shaderNdx]))
		{
			ctx.beginSection("Verify shader: " + std::string(getShaderTypeName(s_shaders[shaderNdx])));
			{
				const std::string shaderSource(genShaderSourceFma(ctx, s_shaders[shaderNdx], glu::TYPE_FLOAT, glu::TYPE_FLOAT, glu::TYPE_INT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceFma(ctx, s_shaders[shaderNdx], glu::TYPE_FLOAT, glu::TYPE_INT, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			{
				const std::string shaderSource(genShaderSourceFma(ctx, s_shaders[shaderNdx], glu::TYPE_INT, glu::TYPE_FLOAT, glu::TYPE_FLOAT));
				verifyShader(ctx, s_shaders[shaderNdx], shaderSource);
			}
			ctx.endSection();
		}
	}
	ctx.endSection();
}

} // anonymous

std::vector<FunctionContainer> getNegativeShaderFunctionTestFunctions ()
{
	const FunctionContainer funcs[] =
	{
		{bitfield_extract_invalid_value_type,				"bitfield_extract_invalid_value_type",				"Invalid usage of bitfieldExtract."			},
		{bitfield_extract_invalid_offset_type,				"bitfield_extract_invalid_offset_type",				"Invalid usage of bitfieldExtract."			},
		{bitfield_extract_invalid_bits_type,				"bitfield_extract_invalid_bits_type",				"Invalid usage of bitfieldExtract."			},
		{bitfield_insert_invalid_base_type,					"bitfield_insert_invalid_base_type",				"Invalid usage of bitfieldInsert."			},
		{bitfield_insert_invalid_insert_type,				"bitfield_insert_invalid_insert_type",				"Invalid usage of bitfieldInsert."			},
		{bitfield_insert_invalid_offset_type,				"bitfield_insert_invalid_offset_type",				"Invalid usage of bitfieldInsert."			},
		{bitfield_insert_invalid_bits_type,					"bitfield_insert_invalid_bits_type",				"Invalid usage of bitfieldInsert."			},
		{bitfield_reverse,									"bitfield_reverse",									"Invalid usage of bitfieldReverse."			},
		{bit_count,											"bit_count",										"Invalid usage of bitCount."				},
		{find_msb,											"find_msb",											"Invalid usage of findMSB."					},
		{find_lsb,											"find_lsb",											"Invalid usage of findLSB."					},
		{uadd_carry_invalid_x,								"uadd_carry_invalid_x",								"Invalid usage of uaddCarry."				},
		{uadd_carry_invalid_y,								"uadd_carry_invalid_y",								"Invalid usage of uaddCarry."				},
		{uadd_carry_invalid_carry,							"uadd_carry_invalid_carry",							"Invalid usage of uaddCarry."				},
		{usub_borrow_invalid_x,								"usub_borrow_invalid_x",							"Invalid usage of usubBorrow."				},
		{usub_borrow_invalid_y,								"usub_borrow_invalid_y",							"Invalid usage of usubBorrow."				},
		{usub_borrow_invalid_borrow,						"usub_borrow_invalid_borrow",						"Invalid usage of usubBorrow."				},
		{umul_extended_invalid_x,							"umul_extended_invalid_x",							"Invalid usage of umulExtended."			},
		{umul_extended_invalid_y,							"umul_extended_invalid_y",							"Invalid usage of umulExtended."			},
		{umul_extended_invalid_msb,							"umul_extended_invalid_msb",						"Invalid usage of umulExtended."			},
		{umul_extended_invalid_lsb,							"umul_extended_invalid_lsb",						"Invalid usage of umulExtended."			},
		{imul_extended_invalid_x,							"imul_extended_invalid_x",							"Invalid usage of imulExtended."			},
		{imul_extended_invalid_y,							"imul_extended_invalid_y",							"Invalid usage of imulExtended."			},
		{imul_extended_invalid_msb,							"imul_extended_invalid_msb",						"Invalid usage of imulExtended."			},
		{imul_extended_invalid_lsb,							"imul_extended_invalid_lsb",						"Invalid usage of imulExtended."			},
		{frexp_invalid_x,									"frexp_invalid_x",									"Invalid usage of frexp."					},
		{frexp_invalid_exp,									"frexp_invalid_exp",								"Invalid usage of frexp."					},
		{ldexp_invalid_x,									"ldexp_invalid_x",									"Invalid usage of ldexp."					},
		{ldexp_invalid_exp,									"ldexp_invalid_exp",								"Invalid usage of ldexp."					},
		{pack_unorm_4x8,									"pack_unorm_4x8",									"Invalid usage of packUnorm4x8."			},
		{pack_snorm_4x8,									"pack_snorm_4x8",									"Invalid usage of packSnorm4x8."			},
		{unpack_snorm_4x8,									"unpack_snorm_4x8",									"Invalid usage of unpackSnorm4x8."			},
		{unpack_unorm_4x8,									"unpack_unorm_4x8",									"Invalid usage of unpackUnorm4x8."			},
		{texture_size_invalid_sampler,						"texture_size_invalid_sampler",						"Invalid usage of textureSize."				},
		{texture_size_invalid_lod,							"texture_size_invalid_lod",							"Invalid usage of textureSize."				},
		{texture_invalid_p,									"texture_invalid_p",								"Invalid usage of texture."					},
		{texture_invalid_bias_or_compare,					"texture_invalid_bias_or_compare",					"Invalid usage of texture."					},
		{texture_lod_invalid_p,								"texture_lod_invalid_p",							"Invalid usage of textureLod."				},
		{texture_lod_invalid_lod,							"texture_lod_invalid_lod",							"Invalid usage of textureLod."				},
		{texel_fetch_invalid_p,								"texel_fetch_invalid_p",							"Invalid usage of texelFetch."				},
		{texel_fetch_invalid_sample,						"texel_fetch_invalid_sample",						"Invalid usage of texelFetch."				},
		{emit_vertex,										"emit_vertex",										"Invalid usage of EmitVertex."				},
		{end_primitive,										"end_primitive",									"Invalid usage of EndPrimitive."			},
		{texture_grad,										"texture_grad",										"Invalid usage of textureGrad."				},
		{texture_gather_sampler_2d,							"texture_gather_sampler_2d",						"Invalid usage of textureGather."			},
		{texture_gather_sampler_2d_array,					"texture_gather_sampler_2d_array",					"Invalid usage of textureGather."			},
		{texture_gather_sampler_cube,						"texture_gather_sampler_cube",						"Invalid usage of textureGather."			},
		{texture_gather_sampler_2d_shadow,					"texture_gather_sampler_2d_shadow",					"Invalid usage of textureGather."			},
		{texture_gather_sampler_2d_array_shadow,			"texture_gather_sampler_2d_array_shadow",			"Invalid usage of textureGather."			},
		{texture_gather_sampler_cube_shadow,				"texture_gather_sampler_cube_shadow",				"Invalid usage of textureGather."			},
		{texture_gather_sampler_cube_array,					"texture_gather_sampler_cube_array",				"Invalid usage of textureGather."			},
		{texture_gather_sampler_cube_array_shadow,			"texture_gather_sampler_cube_array_shadow",			"Invalid usage of textureGather."			},
		{texture_gather_offset_sampler_2d,					"texture_gather_offset_sampler_2d",					"Invalid usage of textureGatherOffset."		},
		{texture_gather_offset_sampler_2d_array,			"texture_gather_offset_sampler_2d_array",			"Invalid usage of textureGatherOffset."		},
		{texture_gather_offset_sampler_2d_shadow,			"texture_gather_offset_sampler_2d_shadow",			"Invalid usage of textureGatherOffset."		},
		{texture_gather_offset_sampler_2d_array_shadow,		"texture_gather_offset_sampler_2d_array_shadow",	"Invalid usage of textureGatherOffset."		},
		{texture_gather_offsets,							"texture_gather_offsets",							"Invalid usage of textureGatherOffsets."	},
		{atomic_add,										"atomic_add",										"Invalid usage of atomicAdd."				},
		{atomic_min,										"atomic_min",										"Invalid usage of atomicMin."				},
		{atomic_max,										"atomic_max",										"Invalid usage of atomicMax."				},
		{atomic_and,										"atomic_and",										"Invalid usage of atomicAnd."				},
		{atomic_or,											"atomic_or",										"Invalid usage of atomicOr."				},
		{atomic_xor,										"atomic_xor",										"Invalid usage of atomicXor."				},
		{atomic_exchange,									"atomic_exchange",									"Invalid usage of atomicExchange."			},
		{atomic_comp_swap,									"atomic_comp_swap",									"Invalid usage of atomicCompSwap."			},
		{interpolate_at_centroid,							"interpolate_at_centroid",							"Invalid usage of interpolateAtCentroid."	},
		{interpolate_at_sample,								"interpolate_at_sample",							"Invalid usage of interpolateAtSample."		},
		{interpolate_at_offset,								"interpolate_at_offset",							"Invalid usage of interpolateAtOffset."		},
		{fma,												"fma",												"Invalid usage of fma."						},
	};

	return std::vector<FunctionContainer>(DE_ARRAY_BEGIN(funcs), DE_ARRAY_END(funcs));
}

} // NegativeTestShared
} // Functional
} // gles31
} // deqp
