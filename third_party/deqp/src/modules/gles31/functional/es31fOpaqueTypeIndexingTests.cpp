/*-------------------------------------------------------------------------
 * drawElements Quality Program OpenGL ES 3.1 Module
 * -------------------------------------------------
 *
 * Copyright 2014 The Android Open Source Project
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
 * \brief Opaque type (sampler, buffer, atomic counter, ...) indexing tests.
 *
 * \todo [2014-03-05 pyry] Extend with following:
 *  + sampler: different filtering modes, multiple sizes, incomplete textures
 *  + SSBO: write, atomic op, unsized array .length()
 *//*--------------------------------------------------------------------*/

#include "es31fOpaqueTypeIndexingTests.hpp"
#include "tcuTexture.hpp"
#include "tcuTestLog.hpp"
#include "tcuFormatUtil.hpp"
#include "tcuVectorUtil.hpp"
#include "gluShaderUtil.hpp"
#include "gluShaderProgram.hpp"
#include "gluObjectWrapper.hpp"
#include "gluTextureUtil.hpp"
#include "gluRenderContext.hpp"
#include "gluProgramInterfaceQuery.hpp"
#include "gluContextInfo.hpp"
#include "glsShaderExecUtil.hpp"
#include "glwFunctions.hpp"
#include "glwEnums.hpp"
#include "deUniquePtr.hpp"
#include "deStringUtil.hpp"
#include "deRandom.hpp"

#include <sstream>

namespace deqp
{
namespace gles31
{
namespace Functional
{

namespace
{

using namespace gls::ShaderExecUtil;
using namespace glu;
using std::string;
using std::vector;
using tcu::TextureFormat;
using tcu::TestLog;

typedef de::UniquePtr<ShaderExecutor> ShaderExecutorPtr;

enum IndexExprType
{
	INDEX_EXPR_TYPE_CONST_LITERAL	= 0,
	INDEX_EXPR_TYPE_CONST_EXPRESSION,
	INDEX_EXPR_TYPE_UNIFORM,
	INDEX_EXPR_TYPE_DYNAMIC_UNIFORM,

	INDEX_EXPR_TYPE_LAST
};

enum TextureType
{
	TEXTURE_TYPE_1D = 0,
	TEXTURE_TYPE_2D,
	TEXTURE_TYPE_CUBE,
	TEXTURE_TYPE_2D_ARRAY,
	TEXTURE_TYPE_3D,
	TEXTURE_TYPE_CUBE_ARRAY,

	TEXTURE_TYPE_LAST
};

static void declareUniformIndexVars (std::ostream& str, const char* varPrefix, int numVars)
{
	for (int varNdx = 0; varNdx < numVars; varNdx++)
		str << "uniform highp int " << varPrefix << varNdx << ";\n";
}

static void uploadUniformIndices (const glw::Functions& gl, deUint32 program, const char* varPrefix, int numIndices, const int* indices)
{
	for (int varNdx = 0; varNdx < numIndices; varNdx++)
	{
		const string	varName		= varPrefix + de::toString(varNdx);
		const int		loc			= gl.getUniformLocation(program, varName.c_str());
		TCU_CHECK_MSG(loc >= 0, ("No location assigned for uniform '" + varName + "'").c_str());

		gl.uniform1i(loc, indices[varNdx]);
	}
}

template<typename T>
static T maxElement (const std::vector<T>& elements)
{
	T maxElem = elements[0];

	for (size_t ndx = 1; ndx < elements.size(); ndx++)
		maxElem = de::max(maxElem, elements[ndx]);

	return maxElem;
}

static TextureType getTextureType (glu::DataType samplerType)
{
	switch (samplerType)
	{
		case glu::TYPE_SAMPLER_1D:
		case glu::TYPE_INT_SAMPLER_1D:
		case glu::TYPE_UINT_SAMPLER_1D:
		case glu::TYPE_SAMPLER_1D_SHADOW:
			return TEXTURE_TYPE_1D;

		case glu::TYPE_SAMPLER_2D:
		case glu::TYPE_INT_SAMPLER_2D:
		case glu::TYPE_UINT_SAMPLER_2D:
		case glu::TYPE_SAMPLER_2D_SHADOW:
			return TEXTURE_TYPE_2D;

		case glu::TYPE_SAMPLER_CUBE:
		case glu::TYPE_INT_SAMPLER_CUBE:
		case glu::TYPE_UINT_SAMPLER_CUBE:
		case glu::TYPE_SAMPLER_CUBE_SHADOW:
			return TEXTURE_TYPE_CUBE;

		case glu::TYPE_SAMPLER_2D_ARRAY:
		case glu::TYPE_INT_SAMPLER_2D_ARRAY:
		case glu::TYPE_UINT_SAMPLER_2D_ARRAY:
		case glu::TYPE_SAMPLER_2D_ARRAY_SHADOW:
			return TEXTURE_TYPE_2D_ARRAY;

		case glu::TYPE_SAMPLER_3D:
		case glu::TYPE_INT_SAMPLER_3D:
		case glu::TYPE_UINT_SAMPLER_3D:
			return TEXTURE_TYPE_3D;

		case glu::TYPE_SAMPLER_CUBE_ARRAY:
		case glu::TYPE_SAMPLER_CUBE_ARRAY_SHADOW:
		case glu::TYPE_INT_SAMPLER_CUBE_ARRAY:
		case glu::TYPE_UINT_SAMPLER_CUBE_ARRAY:
			return TEXTURE_TYPE_CUBE_ARRAY;

		default:
			TCU_THROW(InternalError, "Invalid sampler type");
	}
}

static bool isShadowSampler (glu::DataType samplerType)
{
	return samplerType == glu::TYPE_SAMPLER_1D_SHADOW		||
		   samplerType == glu::TYPE_SAMPLER_2D_SHADOW		||
		   samplerType == glu::TYPE_SAMPLER_2D_ARRAY_SHADOW	||
		   samplerType == glu::TYPE_SAMPLER_CUBE_SHADOW		||
		   samplerType == glu::TYPE_SAMPLER_CUBE_ARRAY_SHADOW;
}

static glu::DataType getSamplerOutputType (glu::DataType samplerType)
{
	switch (samplerType)
	{
		case glu::TYPE_SAMPLER_1D:
		case glu::TYPE_SAMPLER_2D:
		case glu::TYPE_SAMPLER_CUBE:
		case glu::TYPE_SAMPLER_2D_ARRAY:
		case glu::TYPE_SAMPLER_3D:
		case glu::TYPE_SAMPLER_CUBE_ARRAY:
			return glu::TYPE_FLOAT_VEC4;

		case glu::TYPE_SAMPLER_1D_SHADOW:
		case glu::TYPE_SAMPLER_2D_SHADOW:
		case glu::TYPE_SAMPLER_CUBE_SHADOW:
		case glu::TYPE_SAMPLER_2D_ARRAY_SHADOW:
		case glu::TYPE_SAMPLER_CUBE_ARRAY_SHADOW:
			return glu::TYPE_FLOAT;

		case glu::TYPE_INT_SAMPLER_1D:
		case glu::TYPE_INT_SAMPLER_2D:
		case glu::TYPE_INT_SAMPLER_CUBE:
		case glu::TYPE_INT_SAMPLER_2D_ARRAY:
		case glu::TYPE_INT_SAMPLER_3D:
		case glu::TYPE_INT_SAMPLER_CUBE_ARRAY:
			return glu::TYPE_INT_VEC4;

		case glu::TYPE_UINT_SAMPLER_1D:
		case glu::TYPE_UINT_SAMPLER_2D:
		case glu::TYPE_UINT_SAMPLER_CUBE:
		case glu::TYPE_UINT_SAMPLER_2D_ARRAY:
		case glu::TYPE_UINT_SAMPLER_3D:
		case glu::TYPE_UINT_SAMPLER_CUBE_ARRAY:
			return glu::TYPE_UINT_VEC4;

		default:
			TCU_THROW(InternalError, "Invalid sampler type");
	}
}

static tcu::TextureFormat getSamplerTextureFormat (glu::DataType samplerType)
{
	const glu::DataType		outType			= getSamplerOutputType(samplerType);
	const glu::DataType		outScalarType	= glu::getDataTypeScalarType(outType);

	switch (outScalarType)
	{
		case glu::TYPE_FLOAT:
			if (isShadowSampler(samplerType))
				return tcu::TextureFormat(tcu::TextureFormat::D, tcu::TextureFormat::UNORM_INT16);
			else
				return tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNORM_INT8);

		case glu::TYPE_INT:		return tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::SIGNED_INT8);
		case glu::TYPE_UINT:	return tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNSIGNED_INT8);

		default:
			TCU_THROW(InternalError, "Invalid sampler type");
	}
}

static glu::DataType getSamplerCoordType (glu::DataType samplerType)
{
	const TextureType	texType		= getTextureType(samplerType);
	int					numCoords	= 0;

	switch (texType)
	{
		case TEXTURE_TYPE_1D:			numCoords = 1;	break;
		case TEXTURE_TYPE_2D:			numCoords = 2;	break;
		case TEXTURE_TYPE_2D_ARRAY:		numCoords = 3;	break;
		case TEXTURE_TYPE_CUBE:			numCoords = 3;	break;
		case TEXTURE_TYPE_3D:			numCoords = 3;	break;
		case TEXTURE_TYPE_CUBE_ARRAY:	numCoords = 4;	break;
		default:
			TCU_THROW(InternalError, "Invalid texture type");
	}

	if (isShadowSampler(samplerType) && samplerType != TYPE_SAMPLER_CUBE_ARRAY_SHADOW)
		numCoords += 1;

	DE_ASSERT(de::inRange(numCoords, 1, 4));

	return numCoords == 1 ? glu::TYPE_FLOAT : glu::getDataTypeFloatVec(numCoords);
}

static deUint32 getGLTextureTarget (TextureType texType)
{
	switch (texType)
	{
		case TEXTURE_TYPE_1D:			return GL_TEXTURE_1D;
		case TEXTURE_TYPE_2D:			return GL_TEXTURE_2D;
		case TEXTURE_TYPE_2D_ARRAY:		return GL_TEXTURE_2D_ARRAY;
		case TEXTURE_TYPE_CUBE:			return GL_TEXTURE_CUBE_MAP;
		case TEXTURE_TYPE_3D:			return GL_TEXTURE_3D;
		case TEXTURE_TYPE_CUBE_ARRAY:	return GL_TEXTURE_CUBE_MAP_ARRAY;
		default:
			TCU_THROW(InternalError, "Invalid texture type");
	}
}

static void setupTexture (const glw::Functions&	gl,
						  deUint32				texture,
						  glu::DataType			samplerType,
						  tcu::TextureFormat	texFormat,
						  const void*			color)
{
	const TextureType			texType		= getTextureType(samplerType);
	const deUint32				texTarget	= getGLTextureTarget(texType);
	const deUint32				intFormat	= glu::getInternalFormat(texFormat);
	const glu::TransferFormat	transferFmt	= glu::getTransferFormat(texFormat);

	// \todo [2014-03-04 pyry] Use larger than 1x1 textures?

	gl.bindTexture(texTarget, texture);

	switch (texType)
	{
		case TEXTURE_TYPE_1D:
			gl.texStorage1D(texTarget, 1, intFormat, 1);
			gl.texSubImage1D(texTarget, 0, 0, 1, transferFmt.format, transferFmt.dataType, color);
			break;

		case TEXTURE_TYPE_2D:
			gl.texStorage2D(texTarget, 1, intFormat, 1, 1);
			gl.texSubImage2D(texTarget, 0, 0, 0, 1, 1, transferFmt.format, transferFmt.dataType, color);
			break;

		case TEXTURE_TYPE_2D_ARRAY:
		case TEXTURE_TYPE_3D:
			gl.texStorage3D(texTarget, 1, intFormat, 1, 1, 1);
			gl.texSubImage3D(texTarget, 0, 0, 0, 0, 1, 1, 1, transferFmt.format, transferFmt.dataType, color);
			break;

		case TEXTURE_TYPE_CUBE_ARRAY:
			gl.texStorage3D(texTarget, 1, intFormat, 1, 1, 6);
			for (int zoffset = 0; zoffset < 6; ++zoffset)
				for (int face = 0; face < tcu::CUBEFACE_LAST; face++)
					gl.texSubImage3D(texTarget, 0, 0, 0, zoffset, 1, 1, 1, transferFmt.format, transferFmt.dataType, color);
			break;

		case TEXTURE_TYPE_CUBE:
			gl.texStorage2D(texTarget, 1, intFormat, 1, 1);
			for (int face = 0; face < tcu::CUBEFACE_LAST; face++)
				gl.texSubImage2D(glu::getGLCubeFace((tcu::CubeFace)face), 0, 0, 0, 1, 1, transferFmt.format, transferFmt.dataType, color);
			break;

		default:
			TCU_THROW(InternalError, "Invalid texture type");
	}

	gl.texParameteri(texTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gl.texParameteri(texTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	if (isShadowSampler(samplerType))
		gl.texParameteri(texTarget, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Texture setup failed");
}

class SamplerIndexingCase : public TestCase
{
public:
							SamplerIndexingCase			(Context& context, const char* name, const char* description, glu::ShaderType shaderType, glu::DataType samplerType, IndexExprType indexExprType);
							~SamplerIndexingCase		(void);

	void					init						(void);
	IterateResult			iterate						(void);

private:
							SamplerIndexingCase			(const SamplerIndexingCase&);
	SamplerIndexingCase&	operator=					(const SamplerIndexingCase&);

	void					getShaderSpec				(ShaderSpec* spec, int numSamplers, int numLookups, const int* lookupIndices, const RenderContext& renderContext) const;

	const glu::ShaderType	m_shaderType;
	const glu::DataType		m_samplerType;
	const IndexExprType		m_indexExprType;
};

SamplerIndexingCase::SamplerIndexingCase (Context& context, const char* name, const char* description, glu::ShaderType shaderType, glu::DataType samplerType, IndexExprType indexExprType)
	: TestCase			(context, name, description)
	, m_shaderType		(shaderType)
	, m_samplerType		(samplerType)
	, m_indexExprType	(indexExprType)
{
}

SamplerIndexingCase::~SamplerIndexingCase (void)
{
}

void SamplerIndexingCase::init (void)
{
	if (!contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)))
	{
		if (m_shaderType == SHADERTYPE_GEOMETRY)
			TCU_CHECK_AND_THROW(NotSupportedError,
				m_context.getContextInfo().isExtensionSupported("GL_EXT_geometry_shader"),
				"GL_EXT_geometry_shader extension is required to run geometry shader tests.");

		if (m_shaderType == SHADERTYPE_TESSELLATION_CONTROL || m_shaderType == SHADERTYPE_TESSELLATION_EVALUATION)
			TCU_CHECK_AND_THROW(NotSupportedError,
				m_context.getContextInfo().isExtensionSupported("GL_EXT_tessellation_shader"),
				"GL_EXT_tessellation_shader extension is required to run tessellation shader tests.");

		if (m_indexExprType != INDEX_EXPR_TYPE_CONST_LITERAL && m_indexExprType != INDEX_EXPR_TYPE_CONST_EXPRESSION)
			TCU_CHECK_AND_THROW(NotSupportedError,
				m_context.getContextInfo().isExtensionSupported("GL_EXT_gpu_shader5"),
				"GL_EXT_gpu_shader5 extension is required for dynamic indexing of sampler arrays.");

		if (m_samplerType == TYPE_SAMPLER_CUBE_ARRAY
			|| m_samplerType == TYPE_SAMPLER_CUBE_ARRAY_SHADOW
			|| m_samplerType == TYPE_INT_SAMPLER_CUBE_ARRAY
			|| m_samplerType == TYPE_UINT_SAMPLER_CUBE_ARRAY)
		{
			TCU_CHECK_AND_THROW(NotSupportedError,
				m_context.getContextInfo().isExtensionSupported("GL_EXT_texture_cube_map_array"),
				"GL_EXT_texture_cube_map_array extension is required for cube map arrays.");
		}
	}
}

void SamplerIndexingCase::getShaderSpec (ShaderSpec* spec, int numSamplers, int numLookups, const int* lookupIndices, const RenderContext& renderContext) const
{
	const char*			samplersName	= "sampler";
	const char*			coordsName		= "coords";
	const char*			indicesPrefix	= "index";
	const char*			resultPrefix	= "result";
	const DataType		coordType		= getSamplerCoordType(m_samplerType);
	const DataType		outType			= getSamplerOutputType(m_samplerType);
	const bool			supportsES32	= contextSupports(renderContext.getType(), glu::ApiType::es(3, 2));
	std::ostringstream	global;
	std::ostringstream	code;

	spec->inputs.push_back(Symbol(coordsName, VarType(coordType, PRECISION_HIGHP)));

	if (!supportsES32 && m_indexExprType != INDEX_EXPR_TYPE_CONST_LITERAL && m_indexExprType != INDEX_EXPR_TYPE_CONST_EXPRESSION)
		global << "#extension GL_EXT_gpu_shader5 : require\n";

	if (!supportsES32
		&& (m_samplerType == TYPE_SAMPLER_CUBE_ARRAY
			|| m_samplerType == TYPE_SAMPLER_CUBE_ARRAY_SHADOW
			|| m_samplerType == TYPE_INT_SAMPLER_CUBE_ARRAY
			|| m_samplerType == TYPE_UINT_SAMPLER_CUBE_ARRAY))
	{
		global << "#extension GL_EXT_texture_cube_map_array: require\n";
	}

	if (m_indexExprType == INDEX_EXPR_TYPE_CONST_EXPRESSION)
		global << "const highp int indexBase = 1;\n";

	global <<
		"uniform highp " << getDataTypeName(m_samplerType) << " " << samplersName << "[" << numSamplers << "];\n";

	if (m_indexExprType == INDEX_EXPR_TYPE_DYNAMIC_UNIFORM)
	{
		for (int lookupNdx = 0; lookupNdx < numLookups; lookupNdx++)
		{
			const string varName = indicesPrefix + de::toString(lookupNdx);
			spec->inputs.push_back(Symbol(varName, VarType(TYPE_INT, PRECISION_HIGHP)));
		}
	}
	else if (m_indexExprType == INDEX_EXPR_TYPE_UNIFORM)
		declareUniformIndexVars(global, indicesPrefix, numLookups);

	for (int lookupNdx = 0; lookupNdx < numLookups; lookupNdx++)
	{
		const string varName = resultPrefix + de::toString(lookupNdx);
		spec->outputs.push_back(Symbol(varName, VarType(outType, PRECISION_HIGHP)));
	}

	for (int lookupNdx = 0; lookupNdx < numLookups; lookupNdx++)
	{
		code << resultPrefix << "" << lookupNdx << " = texture(" << samplersName << "[";

		if (m_indexExprType == INDEX_EXPR_TYPE_CONST_LITERAL)
			code << lookupIndices[lookupNdx];
		else if (m_indexExprType == INDEX_EXPR_TYPE_CONST_EXPRESSION)
			code << "indexBase + " << (lookupIndices[lookupNdx]-1);
		else
			code << indicesPrefix << lookupNdx;


		code << "], " << coordsName << (m_samplerType == TYPE_SAMPLER_CUBE_ARRAY_SHADOW ? ", 0.0" : "") << ");\n";
	}

	spec->version				= supportsES32 ? GLSL_VERSION_320_ES : GLSL_VERSION_310_ES;
	spec->globalDeclarations	= global.str();
	spec->source				= code.str();
}

static void fillTextureData (const tcu::PixelBufferAccess& access, de::Random& rnd)
{
	DE_ASSERT(access.getHeight() == 1 && access.getDepth() == 1);

	if (access.getFormat().order == TextureFormat::D)
	{
		// \note Texture uses odd values, lookup even values to avoid precision issues.
		const float values[] = { 0.1f, 0.3f, 0.5f, 0.7f, 0.9f };

		for (int ndx = 0; ndx < access.getWidth(); ndx++)
			access.setPixDepth(rnd.choose<float>(DE_ARRAY_BEGIN(values), DE_ARRAY_END(values)), ndx, 0);
	}
	else
	{
		TCU_CHECK_INTERNAL(access.getFormat().order == TextureFormat::RGBA && access.getFormat().getPixelSize() == 4);

		for (int ndx = 0; ndx < access.getWidth(); ndx++)
			*((deUint32*)access.getDataPtr() + ndx) = rnd.getUint32();
	}
}

SamplerIndexingCase::IterateResult SamplerIndexingCase::iterate (void)
{
	const int						numInvocations		= 64;
	const int						numSamplers			= 8;
	const int						numLookups			= 4;
	const DataType					coordType			= getSamplerCoordType(m_samplerType);
	const DataType					outputType			= getSamplerOutputType(m_samplerType);
	const TextureFormat				texFormat			= getSamplerTextureFormat(m_samplerType);
	const int						outLookupStride		= numInvocations*getDataTypeScalarSize(outputType);
	vector<int>						lookupIndices		(numLookups);
	vector<float>					coords;
	vector<deUint32>				outData;
	vector<deUint8>					texData				(numSamplers * texFormat.getPixelSize());
	const tcu::PixelBufferAccess	refTexAccess		(texFormat, numSamplers, 1, 1, &texData[0]);

	ShaderSpec						shaderSpec;
	de::Random						rnd					(deInt32Hash(m_samplerType) ^ deInt32Hash(m_shaderType) ^ deInt32Hash(m_indexExprType));

	for (int ndx = 0; ndx < numLookups; ndx++)
		lookupIndices[ndx] = rnd.getInt(0, numSamplers-1);

	getShaderSpec(&shaderSpec, numSamplers, numLookups, &lookupIndices[0], m_context.getRenderContext());

	coords.resize(numInvocations * getDataTypeScalarSize(coordType));

	if (m_samplerType != TYPE_SAMPLER_CUBE_ARRAY_SHADOW && isShadowSampler(m_samplerType))
	{
		// Use different comparison value per invocation.
		// \note Texture uses odd values, comparison even values.
		const int	numCoordComps	= getDataTypeScalarSize(coordType);
		const float	cmpValues[]		= { 0.0f, 0.2f, 0.4f, 0.6f, 0.8f, 1.0f };

		for (int invocationNdx = 0; invocationNdx < numInvocations; invocationNdx++)
			coords[invocationNdx*numCoordComps + (numCoordComps-1)] = rnd.choose<float>(DE_ARRAY_BEGIN(cmpValues), DE_ARRAY_END(cmpValues));
	}

	fillTextureData(refTexAccess, rnd);

	outData.resize(numLookups*outLookupStride);

	{
		const RenderContext&	renderCtx		= m_context.getRenderContext();
		const glw::Functions&	gl				= renderCtx.getFunctions();
		ShaderExecutorPtr		executor		(createExecutor(m_context.getRenderContext(), m_shaderType, shaderSpec));
		TextureVector			textures		(renderCtx, numSamplers);
		vector<void*>			inputs;
		vector<void*>			outputs;
		vector<int>				expandedIndices;
		const int				maxIndex		= maxElement(lookupIndices);

		m_testCtx.getLog() << *executor;

		if (!executor->isOk())
			TCU_FAIL("Compile failed");

		executor->useProgram();

		// \todo [2014-03-05 pyry] Do we want to randomize tex unit assignments?
		for (int samplerNdx = 0; samplerNdx < numSamplers; samplerNdx++)
		{
			const string	samplerName	= string("sampler[") + de::toString(samplerNdx) + "]";
			const int		samplerLoc	= gl.getUniformLocation(executor->getProgram(), samplerName.c_str());

			if (samplerNdx > maxIndex && samplerLoc < 0)
				continue; // Unused uniform eliminated by compiler

			TCU_CHECK_MSG(samplerLoc >= 0, (string("No location for uniform '") + samplerName + "' found").c_str());

			gl.activeTexture(GL_TEXTURE0 + samplerNdx);
			setupTexture(gl, textures[samplerNdx], m_samplerType, texFormat, &texData[samplerNdx*texFormat.getPixelSize()]);

			gl.uniform1i(samplerLoc, samplerNdx);
		}

		inputs.push_back(&coords[0]);

		if (m_indexExprType == INDEX_EXPR_TYPE_DYNAMIC_UNIFORM)
		{
			expandedIndices.resize(numInvocations * lookupIndices.size());
			for (int lookupNdx = 0; lookupNdx < numLookups; lookupNdx++)
			{
				for (int invNdx = 0; invNdx < numInvocations; invNdx++)
					expandedIndices[lookupNdx*numInvocations + invNdx] = lookupIndices[lookupNdx];
			}

			for (int lookupNdx = 0; lookupNdx < numLookups; lookupNdx++)
				inputs.push_back(&expandedIndices[lookupNdx*numInvocations]);
		}
		else if (m_indexExprType == INDEX_EXPR_TYPE_UNIFORM)
			uploadUniformIndices(gl, executor->getProgram(), "index", numLookups, &lookupIndices[0]);

		for (int lookupNdx = 0; lookupNdx < numLookups; lookupNdx++)
			outputs.push_back(&outData[outLookupStride*lookupNdx]);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Setup failed");

		executor->execute(numInvocations, &inputs[0], &outputs[0]);
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	if (isShadowSampler(m_samplerType))
	{
		const tcu::Sampler	refSampler		(tcu::Sampler::CLAMP_TO_EDGE, tcu::Sampler::CLAMP_TO_EDGE, tcu::Sampler::CLAMP_TO_EDGE,
											 tcu::Sampler::NEAREST, tcu::Sampler::NEAREST, 0.0f, false /* non-normalized */,
											 tcu::Sampler::COMPAREMODE_LESS);
		const int			numCoordComps	= getDataTypeScalarSize(coordType);

		TCU_CHECK_INTERNAL(getDataTypeScalarSize(outputType) == 1);

		// Each invocation may have different results.
		for (int invocationNdx = 0; invocationNdx < numInvocations; invocationNdx++)
		{
			const float	coord	= coords[invocationNdx*numCoordComps + (numCoordComps-1)];

			for (int lookupNdx = 0; lookupNdx < numLookups; lookupNdx++)
			{
				const int		texNdx		= lookupIndices[lookupNdx];
				const float		result		= *((const float*)(const deUint8*)&outData[lookupNdx*outLookupStride + invocationNdx]);
				const float		reference	= refTexAccess.sample2DCompare(refSampler, tcu::Sampler::NEAREST, coord, (float)texNdx, 0.0f, tcu::IVec3(0));

				if (de::abs(result-reference) > 0.005f)
				{
					m_testCtx.getLog() << TestLog::Message << "ERROR: at invocation " << invocationNdx << ", lookup " << lookupNdx << ": expected "
														   << reference << ", got " << result
									   << TestLog::EndMessage;

					if (m_testCtx.getTestResult() == QP_TEST_RESULT_PASS)
						m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Got invalid lookup result");
				}
			}
		}
	}
	else
	{
		TCU_CHECK_INTERNAL(getDataTypeScalarSize(outputType) == 4);

		// Validate results from first invocation
		for (int lookupNdx = 0; lookupNdx < numLookups; lookupNdx++)
		{
			const int		texNdx	= lookupIndices[lookupNdx];
			const deUint8*	resPtr	= (const deUint8*)&outData[lookupNdx*outLookupStride];
			bool			isOk;

			if (outputType == TYPE_FLOAT_VEC4)
			{
				const float			threshold		= 1.0f / 256.0f;
				const tcu::Vec4		reference		= refTexAccess.getPixel(texNdx, 0);
				const float*		floatPtr		= (const float*)resPtr;
				const tcu::Vec4		result			(floatPtr[0], floatPtr[1], floatPtr[2], floatPtr[3]);

				isOk = boolAll(lessThanEqual(abs(reference-result), tcu::Vec4(threshold)));

				if (!isOk)
				{
					m_testCtx.getLog() << TestLog::Message << "ERROR: at lookup " << lookupNdx << ": expected "
														   << reference << ", got " << result
									   << TestLog::EndMessage;
				}
			}
			else
			{
				const tcu::UVec4	reference		= refTexAccess.getPixelUint(texNdx, 0);
				const deUint32*		uintPtr			= (const deUint32*)resPtr;
				const tcu::UVec4	result			(uintPtr[0], uintPtr[1], uintPtr[2], uintPtr[3]);

				isOk = boolAll(equal(reference, result));

				if (!isOk)
				{
					m_testCtx.getLog() << TestLog::Message << "ERROR: at lookup " << lookupNdx << ": expected "
														   << reference << ", got " << result
									   << TestLog::EndMessage;
				}
			}

			if (!isOk && m_testCtx.getTestResult() == QP_TEST_RESULT_PASS)
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Got invalid lookup result");
		}

		// Check results of other invocations against first one
		for (int invocationNdx = 1; invocationNdx < numInvocations; invocationNdx++)
		{
			for (int lookupNdx = 0; lookupNdx < numLookups; lookupNdx++)
			{
				const deUint32*		refPtr		= &outData[lookupNdx*outLookupStride];
				const deUint32*		resPtr		= refPtr + invocationNdx*4;
				bool				isOk		= true;

				for (int ndx = 0; ndx < 4; ndx++)
					isOk = isOk && (refPtr[ndx] == resPtr[ndx]);

				if (!isOk)
				{
					m_testCtx.getLog() << TestLog::Message << "ERROR: invocation " << invocationNdx << " result "
														   << tcu::formatArray(tcu::Format::HexIterator<deUint32>(resPtr), tcu::Format::HexIterator<deUint32>(resPtr+4))
														   << " for lookup " << lookupNdx << " doesn't match result from first invocation "
														   << tcu::formatArray(tcu::Format::HexIterator<deUint32>(refPtr), tcu::Format::HexIterator<deUint32>(refPtr+4))
									   << TestLog::EndMessage;

					if (m_testCtx.getTestResult() == QP_TEST_RESULT_PASS)
						m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Inconsistent lookup results");
				}
			}
		}
	}

	return STOP;
}

class BlockArrayIndexingCase : public TestCase
{
public:
	enum BlockType
	{
		BLOCKTYPE_UNIFORM = 0,
		BLOCKTYPE_BUFFER,

		BLOCKTYPE_LAST
	};
								BlockArrayIndexingCase		(Context& context, const char* name, const char* description, BlockType blockType, IndexExprType indexExprType, ShaderType shaderType);
								~BlockArrayIndexingCase		(void);

	void						init						(void);
	IterateResult				iterate						(void);

private:
								BlockArrayIndexingCase		(const BlockArrayIndexingCase&);
	BlockArrayIndexingCase&		operator=					(const BlockArrayIndexingCase&);

	void						getShaderSpec				(ShaderSpec* spec, int numInstances, int numReads, const int* readIndices, const RenderContext& renderContext) const;

	const BlockType				m_blockType;
	const IndexExprType			m_indexExprType;
	const ShaderType			m_shaderType;

	const int					m_numInstances;
};

BlockArrayIndexingCase::BlockArrayIndexingCase (Context& context, const char* name, const char* description, BlockType blockType, IndexExprType indexExprType, ShaderType shaderType)
	: TestCase			(context, name, description)
	, m_blockType		(blockType)
	, m_indexExprType	(indexExprType)
	, m_shaderType		(shaderType)
	, m_numInstances	(4)
{
}

BlockArrayIndexingCase::~BlockArrayIndexingCase (void)
{
}

void BlockArrayIndexingCase::init (void)
{
	if (!contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)))
	{
		if (m_shaderType == SHADERTYPE_GEOMETRY)
			TCU_CHECK_AND_THROW(NotSupportedError,
				m_context.getContextInfo().isExtensionSupported("GL_EXT_geometry_shader"),
				"GL_EXT_geometry_shader extension is required to run geometry shader tests.");

		if (m_shaderType == SHADERTYPE_TESSELLATION_CONTROL || m_shaderType == SHADERTYPE_TESSELLATION_EVALUATION)
			TCU_CHECK_AND_THROW(NotSupportedError,
				m_context.getContextInfo().isExtensionSupported("GL_EXT_tessellation_shader"),
				"GL_EXT_tessellation_shader extension is required to run tessellation shader tests.");

		if (m_indexExprType != INDEX_EXPR_TYPE_CONST_LITERAL && m_indexExprType != INDEX_EXPR_TYPE_CONST_EXPRESSION)
			TCU_CHECK_AND_THROW(NotSupportedError,
				m_context.getContextInfo().isExtensionSupported("GL_EXT_gpu_shader5"),
				"GL_EXT_gpu_shader5 extension is required for dynamic indexing of interface blocks.");
	}

	if (m_blockType == BLOCKTYPE_BUFFER)
	{
		const deUint32 limitPnames[] =
		{
			GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS,
			GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS,
			GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS,
			GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS,
			GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS,
			GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS
		};

		const glw::Functions&	gl			= m_context.getRenderContext().getFunctions();
		int						maxBlocks	= 0;

		gl.getIntegerv(limitPnames[m_shaderType], &maxBlocks);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv()");

		if (maxBlocks < 2 + m_numInstances)
			throw tcu::NotSupportedError("Not enough shader storage blocks supported for shader type");
	}
}

void BlockArrayIndexingCase::getShaderSpec (ShaderSpec* spec, int numInstances, int numReads, const int* readIndices, const RenderContext& renderContext) const
{
	const int			binding			= 2;
	const char*			blockName		= "Block";
	const char*			instanceName	= "block";
	const char*			indicesPrefix	= "index";
	const char*			resultPrefix	= "result";
	const char*			interfaceName	= m_blockType == BLOCKTYPE_UNIFORM ? "uniform" : "buffer";
	const char*			layout			= m_blockType == BLOCKTYPE_UNIFORM ? "std140" : "std430";
	const bool			supportsES32	= contextSupports(renderContext.getType(), glu::ApiType::es(3, 2));
	std::ostringstream	global;
	std::ostringstream	code;

	if (!supportsES32 && m_indexExprType != INDEX_EXPR_TYPE_CONST_LITERAL && m_indexExprType != INDEX_EXPR_TYPE_CONST_EXPRESSION)
		global << "#extension GL_EXT_gpu_shader5 : require\n";

	if (m_indexExprType == INDEX_EXPR_TYPE_CONST_EXPRESSION)
		global << "const highp int indexBase = 1;\n";

	global <<
		"layout(" << layout << ", binding = " << binding << ") " << interfaceName << " " << blockName << "\n"
		"{\n"
		"	uint value;\n"
		"} " << instanceName << "[" << numInstances << "];\n";

	if (m_indexExprType == INDEX_EXPR_TYPE_DYNAMIC_UNIFORM)
	{
		for (int readNdx = 0; readNdx < numReads; readNdx++)
		{
			const string varName = indicesPrefix + de::toString(readNdx);
			spec->inputs.push_back(Symbol(varName, VarType(TYPE_INT, PRECISION_HIGHP)));
		}
	}
	else if (m_indexExprType == INDEX_EXPR_TYPE_UNIFORM)
		declareUniformIndexVars(global, indicesPrefix, numReads);

	for (int readNdx = 0; readNdx < numReads; readNdx++)
	{
		const string varName = resultPrefix + de::toString(readNdx);
		spec->outputs.push_back(Symbol(varName, VarType(TYPE_UINT, PRECISION_HIGHP)));
	}

	for (int readNdx = 0; readNdx < numReads; readNdx++)
	{
		code << resultPrefix << readNdx << " = " << instanceName << "[";

		if (m_indexExprType == INDEX_EXPR_TYPE_CONST_LITERAL)
			code << readIndices[readNdx];
		else if (m_indexExprType == INDEX_EXPR_TYPE_CONST_EXPRESSION)
			code << "indexBase + " << (readIndices[readNdx]-1);
		else
			code << indicesPrefix << readNdx;

		code << "].value;\n";
	}

	spec->version				= supportsES32 ? GLSL_VERSION_320_ES : GLSL_VERSION_310_ES;
	spec->globalDeclarations	= global.str();
	spec->source				= code.str();
}

BlockArrayIndexingCase::IterateResult BlockArrayIndexingCase::iterate (void)
{
	const int			numInvocations		= 32;
	const int			numInstances		= m_numInstances;
	const int			numReads			= 4;
	vector<int>			readIndices			(numReads);
	vector<deUint32>	inValues			(numInstances);
	vector<deUint32>	outValues			(numInvocations*numReads);
	ShaderSpec			shaderSpec;
	de::Random			rnd					(deInt32Hash(m_shaderType) ^ deInt32Hash(m_blockType) ^ deInt32Hash(m_indexExprType));

	for (int readNdx = 0; readNdx < numReads; readNdx++)
		readIndices[readNdx] = rnd.getInt(0, numInstances-1);

	for (int instanceNdx = 0; instanceNdx < numInstances; instanceNdx++)
		inValues[instanceNdx] = rnd.getUint32();

	getShaderSpec(&shaderSpec, numInstances, numReads, &readIndices[0], m_context.getRenderContext());

	{
		const RenderContext&	renderCtx		= m_context.getRenderContext();
		const glw::Functions&	gl				= renderCtx.getFunctions();
		const int				baseBinding		= 2;
		const BufferVector		buffers			(renderCtx, numInstances);
		const deUint32			bufTarget		= m_blockType == BLOCKTYPE_BUFFER ? GL_SHADER_STORAGE_BUFFER : GL_UNIFORM_BUFFER;
		ShaderExecutorPtr		shaderExecutor	(createExecutor(renderCtx, m_shaderType, shaderSpec));
		vector<int>				expandedIndices;
		vector<void*>			inputs;
		vector<void*>			outputs;

		m_testCtx.getLog() << *shaderExecutor;

		if (!shaderExecutor->isOk())
			TCU_FAIL("Compile failed");

		shaderExecutor->useProgram();

		for (int instanceNdx = 0; instanceNdx < numInstances; instanceNdx++)
		{
			gl.bindBuffer(bufTarget, buffers[instanceNdx]);
			gl.bufferData(bufTarget, (glw::GLsizeiptr)sizeof(deUint32), &inValues[instanceNdx], GL_STATIC_DRAW);
			gl.bindBufferBase(bufTarget, baseBinding+instanceNdx, buffers[instanceNdx]);
		}

		if (m_indexExprType == INDEX_EXPR_TYPE_DYNAMIC_UNIFORM)
		{
			expandedIndices.resize(numInvocations * readIndices.size());

			for (int readNdx = 0; readNdx < numReads; readNdx++)
			{
				int* dst = &expandedIndices[numInvocations*readNdx];
				std::fill(dst, dst+numInvocations, readIndices[readNdx]);
			}

			for (int readNdx = 0; readNdx < numReads; readNdx++)
				inputs.push_back(&expandedIndices[readNdx*numInvocations]);
		}
		else if (m_indexExprType == INDEX_EXPR_TYPE_UNIFORM)
			uploadUniformIndices(gl, shaderExecutor->getProgram(), "index", numReads, &readIndices[0]);

		for (int readNdx = 0; readNdx < numReads; readNdx++)
			outputs.push_back(&outValues[readNdx*numInvocations]);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Setup failed");

		shaderExecutor->execute(numInvocations, inputs.empty() ? DE_NULL : &inputs[0], &outputs[0]);
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	for (int invocationNdx = 0; invocationNdx < numInvocations; invocationNdx++)
	{
		for (int readNdx = 0; readNdx < numReads; readNdx++)
		{
			const deUint32	refValue	= inValues[readIndices[readNdx]];
			const deUint32	resValue	= outValues[readNdx*numInvocations + invocationNdx];

			if (refValue != resValue)
			{
				m_testCtx.getLog() << TestLog::Message << "ERROR: at invocation " << invocationNdx
													   << ", read " << readNdx << ": expected "
													   << tcu::toHex(refValue) << ", got " << tcu::toHex(resValue)
								   << TestLog::EndMessage;

				if (m_testCtx.getTestResult() == QP_TEST_RESULT_PASS)
					m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid result value");
			}
		}
	}

	return STOP;
}

class AtomicCounterIndexingCase : public TestCase
{
public:
								AtomicCounterIndexingCase		(Context& context, const char* name, const char* description, IndexExprType indexExprType, ShaderType shaderType);
								~AtomicCounterIndexingCase		(void);

	void						init							(void);
	IterateResult				iterate							(void);

private:
								AtomicCounterIndexingCase		(const AtomicCounterIndexingCase&);
	AtomicCounterIndexingCase&	operator=						(const AtomicCounterIndexingCase&);

	void						getShaderSpec					(ShaderSpec* spec, int numCounters, int numOps, const int* opIndices, const RenderContext& renderContext) const;

	const IndexExprType			m_indexExprType;
	const glu::ShaderType		m_shaderType;
	deInt32						m_numCounters;
};

AtomicCounterIndexingCase::AtomicCounterIndexingCase (Context& context, const char* name, const char* description, IndexExprType indexExprType, ShaderType shaderType)
	: TestCase			(context, name, description)
	, m_indexExprType	(indexExprType)
	, m_shaderType		(shaderType)
	, m_numCounters		(0)
{
}

AtomicCounterIndexingCase::~AtomicCounterIndexingCase (void)
{
}

deUint32 getMaxAtomicCounterEnum (glu::ShaderType type)
{
	switch (type)
	{
		case glu::SHADERTYPE_VERTEX:					return GL_MAX_VERTEX_ATOMIC_COUNTERS;
		case glu::SHADERTYPE_FRAGMENT:					return GL_MAX_FRAGMENT_ATOMIC_COUNTERS;
		case glu::SHADERTYPE_GEOMETRY:					return GL_MAX_GEOMETRY_ATOMIC_COUNTERS;
		case glu::SHADERTYPE_COMPUTE:					return GL_MAX_COMPUTE_ATOMIC_COUNTERS;
		case glu::SHADERTYPE_TESSELLATION_CONTROL:		return GL_MAX_TESS_CONTROL_ATOMIC_COUNTERS;
		case glu::SHADERTYPE_TESSELLATION_EVALUATION:	return GL_MAX_TESS_EVALUATION_ATOMIC_COUNTERS;

		default:
			DE_FATAL("Unknown shader type");
			return -1;
	}
}

void AtomicCounterIndexingCase::init (void)
{
	if (!contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)))
	{
		if (m_shaderType == SHADERTYPE_GEOMETRY)
			TCU_CHECK_AND_THROW(NotSupportedError,
				m_context.getContextInfo().isExtensionSupported("GL_EXT_geometry_shader"),
				"GL_EXT_geometry_shader extension is required to run geometry shader tests.");

		if (m_shaderType == SHADERTYPE_TESSELLATION_CONTROL || m_shaderType == SHADERTYPE_TESSELLATION_EVALUATION)
			TCU_CHECK_AND_THROW(NotSupportedError,
				m_context.getContextInfo().isExtensionSupported("GL_EXT_tessellation_shader"),
				"GL_EXT_tessellation_shader extension is required to run tessellation shader tests.");

		if (m_indexExprType != INDEX_EXPR_TYPE_CONST_LITERAL && m_indexExprType != INDEX_EXPR_TYPE_CONST_EXPRESSION)
			TCU_CHECK_AND_THROW(NotSupportedError,
				m_context.getContextInfo().isExtensionSupported("GL_EXT_gpu_shader5"),
				"GL_EXT_gpu_shader5 extension is required for dynamic indexing of atomic counters.");
	}

	{
		m_context.getRenderContext().getFunctions().getIntegerv(getMaxAtomicCounterEnum(m_shaderType),
																&m_numCounters);

		if (m_numCounters < 1)
		{
			const string message =  "Atomic counters not supported in " + string(glu::getShaderTypeName(m_shaderType)) + " shader";
			TCU_THROW(NotSupportedError, message.c_str());
		}
	}
}

void AtomicCounterIndexingCase::getShaderSpec (ShaderSpec* spec, int numCounters, int numOps, const int* opIndices, const RenderContext& renderContext) const
{
	const char*			indicesPrefix	= "index";
	const char*			resultPrefix	= "result";
	const bool			supportsES32	= contextSupports(renderContext.getType(), glu::ApiType::es(3, 2));
	std::ostringstream	global;
	std::ostringstream	code;

	if (!supportsES32 && m_indexExprType != INDEX_EXPR_TYPE_CONST_LITERAL && m_indexExprType != INDEX_EXPR_TYPE_CONST_EXPRESSION)
		global << "#extension GL_EXT_gpu_shader5 : require\n";

	if (m_indexExprType == INDEX_EXPR_TYPE_CONST_EXPRESSION)
		global << "const highp int indexBase = 1;\n";

	global <<
		"layout(binding = 0) uniform atomic_uint counter[" << numCounters << "];\n";

	if (m_indexExprType == INDEX_EXPR_TYPE_DYNAMIC_UNIFORM)
	{
		for (int opNdx = 0; opNdx < numOps; opNdx++)
		{
			const string varName = indicesPrefix + de::toString(opNdx);
			spec->inputs.push_back(Symbol(varName, VarType(TYPE_INT, PRECISION_HIGHP)));
		}
	}
	else if (m_indexExprType == INDEX_EXPR_TYPE_UNIFORM)
		declareUniformIndexVars(global, indicesPrefix, numOps);

	for (int opNdx = 0; opNdx < numOps; opNdx++)
	{
		const string varName = resultPrefix + de::toString(opNdx);
		spec->outputs.push_back(Symbol(varName, VarType(TYPE_UINT, PRECISION_HIGHP)));
	}

	for (int opNdx = 0; opNdx < numOps; opNdx++)
	{
		code << resultPrefix << opNdx << " = atomicCounterIncrement(counter[";

		if (m_indexExprType == INDEX_EXPR_TYPE_CONST_LITERAL)
			code << opIndices[opNdx];
		else if (m_indexExprType == INDEX_EXPR_TYPE_CONST_EXPRESSION)
			code << "indexBase + " << (opIndices[opNdx]-1);
		else
			code << indicesPrefix << opNdx;

		code << "]);\n";
	}

	spec->version				= supportsES32 ? GLSL_VERSION_320_ES : GLSL_VERSION_310_ES;
	spec->globalDeclarations	= global.str();
	spec->source				= code.str();
}

AtomicCounterIndexingCase::IterateResult AtomicCounterIndexingCase::iterate (void)
{
	const RenderContext&	renderCtx			= m_context.getRenderContext();
	const glw::Functions&	gl					= renderCtx.getFunctions();
	const Buffer			counterBuffer		(renderCtx);

	const int				numInvocations		= 32;
	const int				numOps				= 4;
	vector<int>				opIndices			(numOps);
	vector<deUint32>		outValues			(numInvocations*numOps);
	ShaderSpec				shaderSpec;
	de::Random				rnd					(deInt32Hash(m_shaderType) ^ deInt32Hash(m_indexExprType));

	for (int opNdx = 0; opNdx < numOps; opNdx++)
		opIndices[opNdx] = rnd.getInt(0, numOps-1);

	getShaderSpec(&shaderSpec, m_numCounters, numOps, &opIndices[0], m_context.getRenderContext());

	{
		const BufferVector		buffers			(renderCtx, m_numCounters);
		ShaderExecutorPtr		shaderExecutor	(createExecutor(renderCtx, m_shaderType, shaderSpec));
		vector<int>				expandedIndices;
		vector<void*>			inputs;
		vector<void*>			outputs;

		m_testCtx.getLog() << *shaderExecutor;

		if (!shaderExecutor->isOk())
			TCU_FAIL("Compile failed");

		{
			const int				bufSize		= getProgramResourceInt(gl, shaderExecutor->getProgram(), GL_ATOMIC_COUNTER_BUFFER, 0, GL_BUFFER_DATA_SIZE);
			const int				maxNdx		= maxElement(opIndices);
			std::vector<deUint8>	emptyData	(m_numCounters*4, 0);

			if (bufSize < (maxNdx+1)*4)
				TCU_FAIL((string("GL reported invalid buffer size " + de::toString(bufSize)).c_str()));

			gl.bindBuffer(GL_ATOMIC_COUNTER_BUFFER, *counterBuffer);
			gl.bufferData(GL_ATOMIC_COUNTER_BUFFER, (glw::GLsizeiptr)emptyData.size(), &emptyData[0], GL_STATIC_DRAW);
			gl.bindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, *counterBuffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Atomic counter buffer initialization failed");
		}

		shaderExecutor->useProgram();

		if (m_indexExprType == INDEX_EXPR_TYPE_DYNAMIC_UNIFORM)
		{
			expandedIndices.resize(numInvocations * opIndices.size());

			for (int opNdx = 0; opNdx < numOps; opNdx++)
			{
				int* dst = &expandedIndices[numInvocations*opNdx];
				std::fill(dst, dst+numInvocations, opIndices[opNdx]);
			}

			for (int opNdx = 0; opNdx < numOps; opNdx++)
				inputs.push_back(&expandedIndices[opNdx*numInvocations]);
		}
		else if (m_indexExprType == INDEX_EXPR_TYPE_UNIFORM)
			uploadUniformIndices(gl, shaderExecutor->getProgram(), "index", numOps, &opIndices[0]);

		for (int opNdx = 0; opNdx < numOps; opNdx++)
			outputs.push_back(&outValues[opNdx*numInvocations]);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Setup failed");

		shaderExecutor->execute(numInvocations, inputs.empty() ? DE_NULL : &inputs[0], &outputs[0]);
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	{
		vector<int>				numHits			(m_numCounters, 0);	// Number of hits per counter.
		vector<deUint32>		counterValues	(m_numCounters);
		vector<vector<bool> >	counterMasks	(m_numCounters);

		for (int opNdx = 0; opNdx < numOps; opNdx++)
			numHits[opIndices[opNdx]] += 1;

		// Read counter values
		{
			const void* mapPtr = DE_NULL;

			try
			{
				mapPtr = gl.mapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, m_numCounters*4, GL_MAP_READ_BIT);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER)");
				TCU_CHECK(mapPtr);
				std::copy((const deUint32*)mapPtr, (const deUint32*)mapPtr + m_numCounters, &counterValues[0]);
				gl.unmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
			}
			catch (...)
			{
				if (mapPtr)
					gl.unmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
				throw;
			}
		}

		// Verify counter values
		for (int counterNdx = 0; counterNdx < m_numCounters; counterNdx++)
		{
			const deUint32		refCount	= (deUint32)(numHits[counterNdx]*numInvocations);
			const deUint32		resCount	= counterValues[counterNdx];

			if (refCount != resCount)
			{
				m_testCtx.getLog() << TestLog::Message << "ERROR: atomic counter " << counterNdx << " has value " << resCount
													   << ", expected " << refCount
								   << TestLog::EndMessage;

				if (m_testCtx.getTestResult() == QP_TEST_RESULT_PASS)
					m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid atomic counter value");
			}
		}

		// Allocate bitmasks - one bit per each valid result value
		for (int counterNdx = 0; counterNdx < m_numCounters; counterNdx++)
		{
			const int	counterValue	= numHits[counterNdx]*numInvocations;
			counterMasks[counterNdx].resize(counterValue, false);
		}

		// Verify result values from shaders
		for (int invocationNdx = 0; invocationNdx < numInvocations; invocationNdx++)
		{
			for (int opNdx = 0; opNdx < numOps; opNdx++)
			{
				const int		counterNdx	= opIndices[opNdx];
				const deUint32	resValue	= outValues[opNdx*numInvocations + invocationNdx];
				const bool		rangeOk		= de::inBounds(resValue, 0u, (deUint32)counterMasks[counterNdx].size());
				const bool		notSeen		= rangeOk && !counterMasks[counterNdx][resValue];
				const bool		isOk		= rangeOk && notSeen;

				if (!isOk)
				{
					m_testCtx.getLog() << TestLog::Message << "ERROR: at invocation " << invocationNdx
														   << ", op " << opNdx << ": got invalid result value "
														   << resValue
									   << TestLog::EndMessage;

					if (m_testCtx.getTestResult() == QP_TEST_RESULT_PASS)
						m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid result value");
				}
				else
				{
					// Mark as used - no other invocation should see this value from same counter.
					counterMasks[counterNdx][resValue] = true;
				}
			}
		}

		if (m_testCtx.getTestResult() == QP_TEST_RESULT_PASS)
		{
			// Consistency check - all masks should be 1 now
			for (int counterNdx = 0; counterNdx < m_numCounters; counterNdx++)
			{
				for (vector<bool>::const_iterator i = counterMasks[counterNdx].begin(); i != counterMasks[counterNdx].end(); i++)
					TCU_CHECK_INTERNAL(*i);
			}
		}
	}

	return STOP;
}

} // anonymous

OpaqueTypeIndexingTests::OpaqueTypeIndexingTests (Context& context)
	: TestCaseGroup(context, "opaque_type_indexing", "Opaque Type Indexing Tests")
{
}

OpaqueTypeIndexingTests::~OpaqueTypeIndexingTests (void)
{
}

void OpaqueTypeIndexingTests::init (void)
{
	static const struct
	{
		IndexExprType	type;
		const char*		name;
		const char*		description;
	} indexingTypes[] =
	{
		{ INDEX_EXPR_TYPE_CONST_LITERAL,	"const_literal",		"Indexing by constant literal"					},
		{ INDEX_EXPR_TYPE_CONST_EXPRESSION,	"const_expression",		"Indexing by constant expression"				},
		{ INDEX_EXPR_TYPE_UNIFORM,			"uniform",				"Indexing by uniform value"						},
		{ INDEX_EXPR_TYPE_DYNAMIC_UNIFORM,	"dynamically_uniform",	"Indexing by dynamically uniform expression"	}
	};

	static const struct
	{
		ShaderType		type;
		const char*		name;
	} shaderTypes[] =
	{
		{ SHADERTYPE_VERTEX,					"vertex"					},
		{ SHADERTYPE_FRAGMENT,					"fragment"					},
		{ SHADERTYPE_COMPUTE,					"compute"					},
		{ SHADERTYPE_GEOMETRY,					"geometry"					},
		{ SHADERTYPE_TESSELLATION_CONTROL,		"tessellation_control"		},
		{ SHADERTYPE_TESSELLATION_EVALUATION,	"tessellation_evaluation"	}
	};

	// .sampler
	{
		static const DataType samplerTypes[] =
		{
			// \note 1D images will be added by a later extension.
//			TYPE_SAMPLER_1D,
			TYPE_SAMPLER_2D,
			TYPE_SAMPLER_CUBE,
			TYPE_SAMPLER_2D_ARRAY,
			TYPE_SAMPLER_3D,
//			TYPE_SAMPLER_1D_SHADOW,
			TYPE_SAMPLER_2D_SHADOW,
			TYPE_SAMPLER_CUBE_SHADOW,
			TYPE_SAMPLER_2D_ARRAY_SHADOW,
//			TYPE_INT_SAMPLER_1D,
			TYPE_INT_SAMPLER_2D,
			TYPE_INT_SAMPLER_CUBE,
			TYPE_INT_SAMPLER_2D_ARRAY,
			TYPE_INT_SAMPLER_3D,
//			TYPE_UINT_SAMPLER_1D,
			TYPE_UINT_SAMPLER_2D,
			TYPE_UINT_SAMPLER_CUBE,
			TYPE_UINT_SAMPLER_2D_ARRAY,
			TYPE_UINT_SAMPLER_3D,
			TYPE_SAMPLER_CUBE_ARRAY,
			TYPE_SAMPLER_CUBE_ARRAY_SHADOW,
			TYPE_INT_SAMPLER_CUBE_ARRAY,
			TYPE_UINT_SAMPLER_CUBE_ARRAY
		};

		tcu::TestCaseGroup* const samplerGroup = new tcu::TestCaseGroup(m_testCtx, "sampler", "Sampler Array Indexing Tests");
		addChild(samplerGroup);

		for (int indexTypeNdx = 0; indexTypeNdx < DE_LENGTH_OF_ARRAY(indexingTypes); indexTypeNdx++)
		{
			const IndexExprType			indexExprType	= indexingTypes[indexTypeNdx].type;
			tcu::TestCaseGroup* const	indexGroup		= new tcu::TestCaseGroup(m_testCtx, indexingTypes[indexTypeNdx].name, indexingTypes[indexTypeNdx].description);
			samplerGroup->addChild(indexGroup);

			for (int shaderTypeNdx = 0; shaderTypeNdx < DE_LENGTH_OF_ARRAY(shaderTypes); shaderTypeNdx++)
			{
				const ShaderType			shaderType		= shaderTypes[shaderTypeNdx].type;
				tcu::TestCaseGroup* const	shaderGroup		= new tcu::TestCaseGroup(m_testCtx, shaderTypes[shaderTypeNdx].name, "");
				indexGroup->addChild(shaderGroup);

				for (int samplerTypeNdx = 0; samplerTypeNdx < DE_LENGTH_OF_ARRAY(samplerTypes); samplerTypeNdx++)
				{
					const DataType	samplerType	= samplerTypes[samplerTypeNdx];
					const char*		samplerName	= getDataTypeName(samplerType);
					const string	caseName	= de::toLower(samplerName);

					shaderGroup->addChild(new SamplerIndexingCase(m_context, caseName.c_str(), "", shaderType, samplerType, indexExprType));
				}
			}
		}
	}

	// .ubo / .ssbo / .atomic_counter
	{
		tcu::TestCaseGroup* const	uboGroup	= new tcu::TestCaseGroup(m_testCtx, "ubo",				"Uniform Block Instance Array Indexing Tests");
		tcu::TestCaseGroup* const	ssboGroup	= new tcu::TestCaseGroup(m_testCtx, "ssbo",				"Buffer Block Instance Array Indexing Tests");
		tcu::TestCaseGroup* const	acGroup		= new tcu::TestCaseGroup(m_testCtx, "atomic_counter",	"Atomic Counter Array Indexing Tests");
		addChild(uboGroup);
		addChild(ssboGroup);
		addChild(acGroup);

		for (int indexTypeNdx = 0; indexTypeNdx < DE_LENGTH_OF_ARRAY(indexingTypes); indexTypeNdx++)
		{
			const IndexExprType		indexExprType		= indexingTypes[indexTypeNdx].type;
			const char*				indexExprName		= indexingTypes[indexTypeNdx].name;
			const char*				indexExprDesc		= indexingTypes[indexTypeNdx].description;

			for (int shaderTypeNdx = 0; shaderTypeNdx < DE_LENGTH_OF_ARRAY(shaderTypes); shaderTypeNdx++)
			{
				const ShaderType		shaderType		= shaderTypes[shaderTypeNdx].type;
				const string			name			= string(indexExprName) + "_" + shaderTypes[shaderTypeNdx].name;

				uboGroup->addChild	(new BlockArrayIndexingCase		(m_context, name.c_str(), indexExprDesc, BlockArrayIndexingCase::BLOCKTYPE_UNIFORM,	indexExprType, shaderType));
				acGroup->addChild	(new AtomicCounterIndexingCase	(m_context, name.c_str(), indexExprDesc, indexExprType, shaderType));

				if (indexExprType == INDEX_EXPR_TYPE_CONST_LITERAL || indexExprType == INDEX_EXPR_TYPE_CONST_EXPRESSION)
					ssboGroup->addChild	(new BlockArrayIndexingCase		(m_context, name.c_str(), indexExprDesc, BlockArrayIndexingCase::BLOCKTYPE_BUFFER,	indexExprType, shaderType));
			}
		}
	}
}

} // Functional
} // gles31
} // deqp
