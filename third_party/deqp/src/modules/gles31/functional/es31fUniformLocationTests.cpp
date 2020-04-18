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
 * \brief Explicit uniform location tests
 *//*--------------------------------------------------------------------*/

#include "es31fUniformLocationTests.hpp"

#include "tcuTestLog.hpp"
#include "tcuTextureUtil.hpp"
#include "tcuVectorUtil.hpp"
#include "tcuCommandLine.hpp"

#include "glsShaderLibrary.hpp"
#include "glsTextureTestUtil.hpp"

#include "gluShaderProgram.hpp"
#include "gluTexture.hpp"
#include "gluPixelTransfer.hpp"
#include "gluVarType.hpp"
#include "gluVarTypeUtil.hpp"

#include "glwFunctions.hpp"
#include "glwEnums.hpp"
#include "sglrContextUtil.hpp"

#include "deStringUtil.hpp"
#include "deUniquePtr.hpp"
#include "deString.h"
#include "deRandom.hpp"
#include "deInt32.h"

#include <set>
#include <map>

namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace
{

using std::string;
using std::vector;
using std::map;
using de::UniquePtr;
using glu::VarType;

struct UniformInfo
{
	enum ShaderStage
	{
		SHADERSTAGE_NONE	= 0,
		SHADERSTAGE_VERTEX	= (1<<0),
		SHADERSTAGE_FRAGMENT= (1<<1),
		SHADERSTAGE_BOTH	= (SHADERSTAGE_VERTEX | SHADERSTAGE_FRAGMENT),
	};

	VarType			type;
	ShaderStage		declareLocation; // support declarations with/without layout qualifiers, needed for linkage testing
	ShaderStage		layoutLocation;
	ShaderStage		checkLocation;
	int				location; // -1 for unset

	UniformInfo (VarType type_, ShaderStage declareLocation_, ShaderStage layoutLocation_, ShaderStage checkLocation_, int location_ = -1)
		: type				(type_)
		, declareLocation	(declareLocation_)
		, layoutLocation	(layoutLocation_)
		, checkLocation		(checkLocation_)
		, location			(location_)
	{
	}
};

class UniformLocationCase : public tcu::TestCase
{
public:
								UniformLocationCase		(tcu::TestContext&			context,
														 glu::RenderContext&		renderContext,
														 const char*				name,
														 const char*				desc,
														 const vector<UniformInfo>&	uniformInfo);
	virtual						~UniformLocationCase	(void) {}

	virtual IterateResult		iterate					(void);

protected:
	IterateResult				run						(const vector<UniformInfo>& uniformList);
	static glu::ProgramSources	genShaderSources		(const vector<UniformInfo>& uniformList);
	bool						verifyLocations			(const glu::ShaderProgram& program, const vector<UniformInfo>& uniformList);
	void						render					(const glu::ShaderProgram& program, const vector<UniformInfo>& uniformList);
	static bool					verifyResult			(const tcu::ConstPixelBufferAccess& access);

	static float				getExpectedValue		(glu::DataType type, int id, const char* name);

	de::MovePtr<glu::Texture2D>	createTexture			(glu::DataType samplerType, float redChannelValue, int binding);

	glu::RenderContext&			m_renderCtx;

	const vector<UniformInfo>	m_uniformInfo;

	enum
	{
		RENDER_SIZE = 16
	};
};

string getUniformName (int ndx, const glu::VarType& type, const glu::TypeComponentVector& path)
{
	std::ostringstream buff;
	buff << "uni" << ndx << glu::TypeAccessFormat(type, path);

	return buff.str();
}

string getFirstComponentName (const glu::VarType& type)
{
	std::ostringstream buff;
	if (glu::isDataTypeVector(type.getBasicType()))
		buff << glu::TypeAccessFormat(type, glu::SubTypeAccess(type).component(0).getPath());
	else if (glu::isDataTypeMatrix(type.getBasicType()))
		buff << glu::TypeAccessFormat(type, glu::SubTypeAccess(type).column(0).component(0).getPath());

	return buff.str();
}

UniformLocationCase::UniformLocationCase (tcu::TestContext&				context,
										  glu::RenderContext&			renderContext,
										  const char*					name,
										  const char*					desc,
										  const vector<UniformInfo>&	uniformInfo)
	: TestCase			(context, name, desc)
	, m_renderCtx		(renderContext)
	, m_uniformInfo		(uniformInfo)
{
}

// [from, to]
std::vector<int> shuffledRange (int from, int to, int seed)
{
	const int	count	= to - from;

	vector<int> retval	(count);
	de::Random	rng		(seed);

	DE_ASSERT(count > 0);

	for (int ndx = 0; ndx < count; ndx++)
		retval[ndx] = ndx + from;

	rng.shuffle(retval.begin(), retval.end());
	return retval;
}

glu::DataType getDataTypeSamplerSampleType (glu::DataType type)
{
	using namespace glu;

	if (type >= TYPE_SAMPLER_1D && type <= TYPE_SAMPLER_3D)
		return TYPE_FLOAT_VEC4;
	else if (type >= TYPE_INT_SAMPLER_1D && type <= TYPE_INT_SAMPLER_3D)
		return TYPE_INT_VEC4;
	else if (type >= TYPE_UINT_SAMPLER_1D && type <= TYPE_UINT_SAMPLER_3D)
		return TYPE_UINT_VEC4;
	else if (type >= TYPE_SAMPLER_1D_SHADOW && type <=	TYPE_SAMPLER_2D_ARRAY_SHADOW)
		return TYPE_FLOAT;
	else
		DE_FATAL("Unknown sampler type");

	return TYPE_INVALID;
}

// A (hopefully) unique value for a uniform. For multi-component types creates only one value. Values are in the range [0,1] for floats, [-128, 127] for ints, [0,255] for uints and 0/1 for booleans. Samplers are treated according to the types they return.
float UniformLocationCase::getExpectedValue (glu::DataType type, int id, const char* name)
{
	const deUint32	hash			= deStringHash(name) + deInt32Hash(id);

	glu::DataType	adjustedType	= type;

	if (glu::isDataTypeSampler(type))
		adjustedType = getDataTypeSamplerSampleType(type);

	if (glu::isDataTypeIntOrIVec(adjustedType))
		return float(hash%128);
	else if (glu::isDataTypeUintOrUVec(adjustedType))
		return float(hash%255);
	else if (glu::isDataTypeFloatOrVec(adjustedType))
		return float(hash%255)/255.0f;
	else if (glu::isDataTypeBoolOrBVec(adjustedType))
		return float(hash%2);
	else
		DE_FATAL("Unkown primitive type");

	return glu::TYPE_INVALID;
}

UniformLocationCase::IterateResult UniformLocationCase::iterate (void)
{
	return run(m_uniformInfo);
}

UniformLocationCase::IterateResult UniformLocationCase::run (const vector<UniformInfo>& uniformList)
{
	using gls::TextureTestUtil::RandomViewport;

	const glu::ProgramSources	sources		= genShaderSources(uniformList);
	const glu::ShaderProgram	program		(m_renderCtx, sources);
	const int					baseSeed	= m_testCtx.getCommandLine().getBaseSeed();
	const glw::Functions&		gl			= m_renderCtx.getFunctions();
	const RandomViewport		viewport	(m_renderCtx.getRenderTarget(), RENDER_SIZE, RENDER_SIZE, deStringHash(getName()) + baseSeed);

	tcu::Surface				rendered	(RENDER_SIZE, RENDER_SIZE);

	if (!verifyLocations(program, uniformList))
		return STOP;

	gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
	gl.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	gl.viewport(viewport.x, viewport.y, viewport.width, viewport.height);

	render(program, uniformList);

	glu::readPixels(m_renderCtx, viewport.x, viewport.y, rendered.getAccess());

	if (!verifyResult(rendered.getAccess()))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Shader produced incorrect result");
		return STOP;
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

glu::ProgramSources UniformLocationCase::genShaderSources (const vector<UniformInfo>& uniformList)
{
	std::ostringstream	vertDecl, vertMain, fragDecl, fragMain;

	vertDecl << "#version 310 es\n"
			 << "precision highp float;\n"
			 << "precision highp int;\n"
			 << "float verify(float val, float ref) { return float(abs(val-ref) < 0.05); }\n\n"
			 << "in highp vec4 a_position;\n"
			 << "out highp vec4 v_color;\n";
	fragDecl << "#version 310 es\n\n"
			 << "precision highp float;\n"
			 << "precision highp int;\n"
			 << "float verify(float val, float ref) { return float(abs(val-ref) < 0.05); }\n\n"
			 << "in highp vec4 v_color;\n"
			 << "layout(location = 0) out mediump vec4 o_color;\n\n";

	vertMain << "void main()\n{\n"
			 << "	gl_Position = a_position;\n"
			 << "	v_color = vec4(1.0);\n";

	fragMain << "void main()\n{\n"
			 << "	o_color = v_color;\n";

	std::set<const glu::StructType*> declaredStructs;

	// Declare uniforms
	for (int uniformNdx = 0; uniformNdx < int(uniformList.size()); uniformNdx++)
	{
		const UniformInfo&	uniformInfo = uniformList[uniformNdx];

		const bool			declareInVert	= (uniformInfo.declareLocation & UniformInfo::SHADERSTAGE_VERTEX)   != 0;
		const bool			declareInFrag	= (uniformInfo.declareLocation & UniformInfo::SHADERSTAGE_FRAGMENT) != 0;
		const bool			layoutInVert    = (uniformInfo.layoutLocation  & UniformInfo::SHADERSTAGE_VERTEX)   != 0;
		const bool			layoutInFrag    = (uniformInfo.layoutLocation  & UniformInfo::SHADERSTAGE_FRAGMENT) != 0;
		const bool			checkInVert		= (uniformInfo.checkLocation   & UniformInfo::SHADERSTAGE_VERTEX)   != 0;
		const bool			checkInFrag		= (uniformInfo.checkLocation   & UniformInfo::SHADERSTAGE_FRAGMENT) != 0;

		const string		layout			= uniformInfo.location >= 0 ? "layout(location = " + de::toString(uniformInfo.location) + ") " : "";
		const string		uniName			= "uni" + de::toString(uniformNdx);

		int					location		= uniformInfo.location;
		int					subTypeIndex	= 0;

		DE_ASSERT((declareInVert && layoutInVert) || !layoutInVert); // Cannot have layout without declaration
		DE_ASSERT((declareInFrag && layoutInFrag) || !layoutInFrag);
		DE_ASSERT(location<0 || (layoutInVert || layoutInFrag)); // Cannot have location without layout

		// struct definitions
		if (uniformInfo.type.isStructType())
		{
			const glu::StructType* const structType = uniformInfo.type.getStructPtr();
			if (!declaredStructs.count(structType))
			{
				if (declareInVert)
					vertDecl << glu::declare(structType, 0) << ";\n";

				if (declareInFrag)
					fragDecl << glu::declare(structType, 0) << ";\n";

				declaredStructs.insert(structType);
			}
		}

		if (declareInVert)
			vertDecl << "uniform " << (layoutInVert ? layout : "") << glu::declare(uniformInfo.type, uniName) << ";\n";

		if (declareInFrag)
			fragDecl << "uniform " << (layoutInFrag ? layout : "") << glu::declare(uniformInfo.type, uniName) << ";\n";

		// Anything that needs to be done for each enclosed primitive type
		for (glu::BasicTypeIterator subTypeIter = glu::BasicTypeIterator::begin(&uniformInfo.type); subTypeIter != glu::BasicTypeIterator::end(&uniformInfo.type); subTypeIter++, subTypeIndex++)
		{
			const glu::VarType	subType		= glu::getVarType(uniformInfo.type, subTypeIter.getPath());
			const glu::DataType	scalarType	= glu::getDataTypeScalarType(subType.getBasicType());
			const char* const	typeName	= glu::getDataTypeName(scalarType);
			const string		expectValue	= de::floatToString(getExpectedValue(scalarType, location >= 0 ? location+subTypeIndex : -1, typeName), 3);

			if (glu::isDataTypeSampler(scalarType))
			{
				if (checkInVert)
					vertMain << "	v_color.rgb *= verify(float( texture(" << uniName
							 << glu::TypeAccessFormat(uniformInfo.type, subTypeIter.getPath())
							 << ", vec2(0.5)).r), " << expectValue << ");\n";
				if (checkInFrag)
					fragMain << "	o_color.rgb *= verify(float( texture(" << uniName
							 << glu::TypeAccessFormat(uniformInfo.type, subTypeIter.getPath())
							 << ", vec2(0.5)).r), " << expectValue << ");\n";
			}
			else
			{
				if (checkInVert)
					vertMain << "	v_color.rgb *= verify(float(" << uniName
							 << glu::TypeAccessFormat(uniformInfo.type, subTypeIter.getPath())
							 << getFirstComponentName(subType) << "), " << expectValue << ");\n";
				if (checkInFrag)
					fragMain << "	o_color.rgb *= verify(float(" << uniName
							 << glu::TypeAccessFormat(uniformInfo.type, subTypeIter.getPath())
							 << getFirstComponentName(subType) << "), " << expectValue << ");\n";
			}
		}
	}

	vertMain << "}\n";
	fragMain << "}\n";

	return glu::makeVtxFragSources(vertDecl.str() + vertMain.str(), fragDecl.str() + fragMain.str());
}

bool UniformLocationCase::verifyLocations (const glu::ShaderProgram& program, const vector<UniformInfo>& uniformList)
{
	using tcu::TestLog;

	const glw::Functions&	gl			= m_renderCtx.getFunctions();
	const bool				vertexOk	= program.getShaderInfo(glu::SHADERTYPE_VERTEX).compileOk;
	const bool				fragmentOk	= program.getShaderInfo(glu::SHADERTYPE_FRAGMENT).compileOk;
	const bool				linkOk		= program.getProgramInfo().linkOk;
	const deUint32			programID	= program.getProgram();

	TestLog&				log			= m_testCtx.getLog();
	std::set<int>			usedLocations;

	log << program;

	if (!vertexOk || !fragmentOk || !linkOk)
	{
		log << TestLog::Message << "ERROR: shader failed to compile/link" << TestLog::EndMessage;
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Shader failed to compile/link");
		return false;
	}

	for (int uniformNdx = 0; uniformNdx < int(uniformList.size()); uniformNdx++)
	{
		const UniformInfo&	uniformInfo		= uniformList[uniformNdx];
		int					subTypeIndex	= 0;

		for (glu::BasicTypeIterator subTypeIter = glu::BasicTypeIterator::begin(&uniformInfo.type); subTypeIter != glu::BasicTypeIterator::end(&uniformInfo.type); subTypeIter++, subTypeIndex++)
		{
			const string		name		= getUniformName(uniformNdx, uniformInfo.type, subTypeIter.getPath());
			const int			gotLoc		= gl.getUniformLocation(programID, name.c_str());
			const int			expectLoc	= uniformInfo.location >= 0 ? uniformInfo.location+subTypeIndex : -1;

			if (expectLoc >= 0)
			{
				if (uniformInfo.checkLocation == 0 && gotLoc == -1)
					continue;

				if (gotLoc != expectLoc)
				{
					log << TestLog::Message << "ERROR: found uniform " << name << " in location " << gotLoc << " when it should have been in " << expectLoc << TestLog::EndMessage;
					m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Incorrect uniform location");
					return false;
				}

				if (usedLocations.find(expectLoc) != usedLocations.end())
				{
					log << TestLog::Message << "ERROR: expected uniform " << name << " in location " << gotLoc << " but it has already been used" << TestLog::EndMessage;
					m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Overlapping uniform location");
					return false;
				}

				usedLocations.insert(expectLoc);
			}
			else if (gotLoc >= 0)
			{
				if (usedLocations.count(gotLoc))
				{
					log << TestLog::Message << "ERROR: found uniform " << name << " in location " << gotLoc << " which has already been used" << TestLog::EndMessage;
					m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Overlapping uniform location");
					return false;
				}

				usedLocations.insert(gotLoc);
			}
		}
	}

	return true;
}

// Check that shader output is white (or very close to it)
bool UniformLocationCase::verifyResult (const tcu::ConstPixelBufferAccess& access)
{
	using tcu::Vec4;

	const Vec4 threshold (0.1f, 0.1f, 0.1f, 0.1f);
	const Vec4 reference (1.0f, 1.0f, 1.0f, 1.0f);

	for (int y = 0; y < access.getHeight(); y++)
	{
		for (int x = 0; x < access.getWidth(); x++)
		{
			const Vec4 diff = abs(access.getPixel(x, y) - reference);

			if (!boolAll(lessThanEqual(diff, threshold)))
				return false;
		}
	}

	return true;
}

// get a 4 channel 8 bits each texture format that is usable by the given sampler type
deUint32 getTextureFormat (glu::DataType samplerType)
{
	using namespace glu;

	switch (samplerType)
	{
		case TYPE_SAMPLER_1D:
		case TYPE_SAMPLER_2D:
		case TYPE_SAMPLER_CUBE:
		case TYPE_SAMPLER_2D_ARRAY:
		case TYPE_SAMPLER_3D:
			return GL_RGBA8;

		case TYPE_INT_SAMPLER_1D:
		case TYPE_INT_SAMPLER_2D:
		case TYPE_INT_SAMPLER_CUBE:
		case TYPE_INT_SAMPLER_2D_ARRAY:
		case TYPE_INT_SAMPLER_3D:
			return GL_RGBA8I;

		case TYPE_UINT_SAMPLER_1D:
		case TYPE_UINT_SAMPLER_2D:
		case TYPE_UINT_SAMPLER_CUBE:
		case TYPE_UINT_SAMPLER_2D_ARRAY:
		case TYPE_UINT_SAMPLER_3D:
			return GL_RGBA8UI;

		default:
			DE_FATAL("Unsupported (sampler) type");
			return 0;
	}
}

// create a texture suitable for sampling by the given sampler type and bind it
de::MovePtr<glu::Texture2D> UniformLocationCase::createTexture (glu::DataType samplerType, float redChannelValue, int binding)
{
	using namespace glu;

	const glw::Functions&	gl		 = m_renderCtx.getFunctions();

	const deUint32			format	 = getTextureFormat(samplerType);
	de::MovePtr<Texture2D>	tex;

	tex = de::MovePtr<Texture2D>(new Texture2D(m_renderCtx, format, 16, 16));

	tex->getRefTexture().allocLevel(0);

	if (format == GL_RGBA8I || format == GL_RGBA8UI)
		tcu::clear(tex->getRefTexture().getLevel(0), tcu::IVec4(int(redChannelValue), 0, 0, 0));
	else
		tcu::clear(tex->getRefTexture().getLevel(0), tcu::Vec4(redChannelValue, 0.0f, 0.0f, 1.0f));

	gl.activeTexture(GL_TEXTURE0 + binding);
	tex->upload();

	gl.bindTexture(GL_TEXTURE_2D, tex->getGLTexture());
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	GLU_EXPECT_NO_ERROR(gl.getError(), "UniformLocationCase: texture upload");

	return tex;
}

void UniformLocationCase::render (const glu::ShaderProgram& program, const vector<UniformInfo>& uniformList)
{
	using glu::Texture2D;
	using de::MovePtr;
	typedef vector<Texture2D*> TextureList;

	const glw::Functions&	gl				= m_renderCtx.getFunctions();
	const deUint32			programID		= program.getProgram();
	const deInt32			posLoc			= gl.getAttribLocation(programID, "a_position");

	// Vertex data.
	const float position[] =
	{
		-1.0f, -1.0f, 0.1f,	1.0f,
		-1.0f,  1.0f, 0.1f,	1.0f,
		 1.0f, -1.0f, 0.1f,	1.0f,
		 1.0f,  1.0f, 0.1f,	1.0f
	};
	const deUint16			indices[]		= { 0, 1, 2, 2, 1, 3 };

	// some buffers to feed to the GPU, only the first element is relevant since the others are never verified
	float					floatBuf[16]	= {0.0f};
	deInt32					intBuf[4]		= {0};
	deUint32				uintBuf[4]		= {0};

	TextureList				texList;

	TCU_CHECK(posLoc >= 0);
	gl.useProgram(programID);

	try
	{

		// Set uniforms
		for (unsigned int uniformNdx = 0; uniformNdx < uniformList.size(); uniformNdx++)
		{
			const UniformInfo&	uniformInfo			= uniformList[uniformNdx];
			int					expectedLocation	= uniformInfo.location;

			for (glu::BasicTypeIterator subTypeIter = glu::BasicTypeIterator::begin(&uniformInfo.type); subTypeIter != glu::BasicTypeIterator::end(&uniformInfo.type); subTypeIter++)
			{
				const glu::VarType	type			= glu::getVarType(uniformInfo.type, subTypeIter.getPath());
				const string		name			= getUniformName(uniformNdx, uniformInfo.type, subTypeIter.getPath());
				const int			gotLoc			= gl.getUniformLocation(programID, name.c_str());
				const glu::DataType	scalarType		= glu::getDataTypeScalarType(type.getBasicType());
				const char*	const	typeName		= glu::getDataTypeName(scalarType);
				const float			expectedValue	= getExpectedValue(scalarType, expectedLocation, typeName);

				if (glu::isDataTypeSampler(scalarType))
				{
					const int binding = (int)texList.size();

					texList.push_back(createTexture(scalarType, expectedValue, binding).release());
					gl.uniform1i(gotLoc, binding);
				}
				else if(gotLoc >= 0)
				{
					floatBuf[0] = expectedValue;
					intBuf[0]   = int(expectedValue);
					uintBuf[0]  = deUint32(expectedValue);

					m_testCtx.getLog() << tcu::TestLog::Message << "Set uniform " << name << " in location " << gotLoc << " to " << expectedValue << tcu::TestLog::EndMessage;

					switch (type.getBasicType())
					{
						case glu::TYPE_FLOAT:			gl.uniform1fv(gotLoc, 1, floatBuf);					break;
						case glu::TYPE_FLOAT_VEC2:		gl.uniform2fv(gotLoc, 1, floatBuf);					break;
						case glu::TYPE_FLOAT_VEC3:		gl.uniform3fv(gotLoc, 1, floatBuf);					break;
						case glu::TYPE_FLOAT_VEC4:		gl.uniform4fv(gotLoc, 1, floatBuf);					break;

						case glu::TYPE_INT:				gl.uniform1iv(gotLoc, 1, intBuf);					break;
						case glu::TYPE_INT_VEC2:		gl.uniform2iv(gotLoc, 1, intBuf);					break;
						case glu::TYPE_INT_VEC3:		gl.uniform3iv(gotLoc, 1, intBuf);					break;
						case glu::TYPE_INT_VEC4:		gl.uniform4iv(gotLoc, 1, intBuf);					break;

						case glu::TYPE_UINT:			gl.uniform1uiv(gotLoc, 1, uintBuf);					break;
						case glu::TYPE_UINT_VEC2:		gl.uniform2uiv(gotLoc, 1, uintBuf);					break;
						case glu::TYPE_UINT_VEC3:		gl.uniform3uiv(gotLoc, 1, uintBuf);					break;
						case glu::TYPE_UINT_VEC4:		gl.uniform4uiv(gotLoc, 1, uintBuf);					break;

						case glu::TYPE_BOOL:			gl.uniform1iv(gotLoc, 1, intBuf);					break;
						case glu::TYPE_BOOL_VEC2:		gl.uniform2iv(gotLoc, 1, intBuf);					break;
						case glu::TYPE_BOOL_VEC3:		gl.uniform3iv(gotLoc, 1, intBuf);					break;
						case glu::TYPE_BOOL_VEC4:		gl.uniform4iv(gotLoc, 1, intBuf);					break;

						case glu::TYPE_FLOAT_MAT2:		gl.uniformMatrix2fv(gotLoc, 1, false, floatBuf);	break;
						case glu::TYPE_FLOAT_MAT2X3:	gl.uniformMatrix2x3fv(gotLoc, 1, false, floatBuf);	break;
						case glu::TYPE_FLOAT_MAT2X4:	gl.uniformMatrix2x4fv(gotLoc, 1, false, floatBuf);	break;

						case glu::TYPE_FLOAT_MAT3X2:	gl.uniformMatrix3x2fv(gotLoc, 1, false, floatBuf);	break;
						case glu::TYPE_FLOAT_MAT3:		gl.uniformMatrix3fv(gotLoc, 1, false, floatBuf);	break;
						case glu::TYPE_FLOAT_MAT3X4:	gl.uniformMatrix3x4fv(gotLoc, 1, false, floatBuf);	break;

						case glu::TYPE_FLOAT_MAT4X2:	gl.uniformMatrix4x2fv(gotLoc, 1, false, floatBuf);	break;
						case glu::TYPE_FLOAT_MAT4X3:	gl.uniformMatrix4x3fv(gotLoc, 1, false, floatBuf);	break;
						case glu::TYPE_FLOAT_MAT4:		gl.uniformMatrix4fv(gotLoc, 1, false, floatBuf);	break;
						default:
							DE_ASSERT(false);
					}
				}

				expectedLocation += expectedLocation>=0;
			}
		}

		gl.enableVertexAttribArray(posLoc);
		gl.vertexAttribPointer(posLoc, 4, GL_FLOAT, GL_FALSE, 0, &position[0]);

		gl.drawElements(GL_TRIANGLES, DE_LENGTH_OF_ARRAY(indices), GL_UNSIGNED_SHORT, &indices[0]);

		gl.disableVertexAttribArray(posLoc);
	}
	catch(...)
	{
		for (int i = 0; i < int(texList.size()); i++)
			delete texList[i];

		throw;
	}

	for (int i = 0; i < int(texList.size()); i++)
		delete texList[i];
}

class MaxUniformLocationCase : public UniformLocationCase
{
public:
								MaxUniformLocationCase		(tcu::TestContext&			context,
															 glu::RenderContext&		renderContext,
															 const char*				name,
															 const char*				desc,
															 const vector<UniformInfo>&	uniformInfo);
	virtual						~MaxUniformLocationCase		(void) {}
	virtual IterateResult		iterate						(void);
};

MaxUniformLocationCase::MaxUniformLocationCase (tcu::TestContext&			context,
												glu::RenderContext&			renderContext,
												const char*					name,
												const char*					desc,
												const vector<UniformInfo>&	uniformInfo)
	: UniformLocationCase(context, renderContext, name, desc, uniformInfo)
{
	DE_ASSERT(!uniformInfo.empty());
}

UniformLocationCase::IterateResult MaxUniformLocationCase::iterate (void)
{
	int					maxLocation = 1024;
	vector<UniformInfo>	uniformInfo = m_uniformInfo;

	m_renderCtx.getFunctions().getIntegerv(GL_MAX_UNIFORM_LOCATIONS, &maxLocation);

	uniformInfo[0].location = maxLocation-1;

	return UniformLocationCase::run(uniformInfo);
}

} // Anonymous

UniformLocationTests::UniformLocationTests (Context& context)
	: TestCaseGroup(context, "uniform_location", "Explicit uniform locations")
{
}

UniformLocationTests::~UniformLocationTests (void)
{
	for (int i = 0; i < int(structTypes.size()); i++)
		delete structTypes[i];
}

glu::VarType createVarType (glu::DataType type)
{
	return glu::VarType(type, glu::isDataTypeBoolOrBVec(type) ? glu::PRECISION_LAST : glu::PRECISION_HIGHP);
}

void UniformLocationTests::init (void)
{
	using namespace glu;

	const UniformInfo::ShaderStage checkStages[]	= { UniformInfo::SHADERSTAGE_VERTEX, UniformInfo::SHADERSTAGE_FRAGMENT };
	const char*						stageNames[]	= {"vertex", "fragment"};
	const int						maxLocations	= 1024;
	const int						baseSeed		= m_context.getTestContext().getCommandLine().getBaseSeed();

	const DataType					primitiveTypes[] =
	{
		TYPE_FLOAT,
		TYPE_FLOAT_VEC2,
		TYPE_FLOAT_VEC3,
		TYPE_FLOAT_VEC4,

		TYPE_INT,
		TYPE_INT_VEC2,
		TYPE_INT_VEC3,
		TYPE_INT_VEC4,

		TYPE_UINT,
		TYPE_UINT_VEC2,
		TYPE_UINT_VEC3,
		TYPE_UINT_VEC4,

		TYPE_BOOL,
		TYPE_BOOL_VEC2,
		TYPE_BOOL_VEC3,
		TYPE_BOOL_VEC4,

		TYPE_FLOAT_MAT2,
		TYPE_FLOAT_MAT2X3,
		TYPE_FLOAT_MAT2X4,
		TYPE_FLOAT_MAT3X2,
		TYPE_FLOAT_MAT3,
		TYPE_FLOAT_MAT3X4,
		TYPE_FLOAT_MAT4X2,
		TYPE_FLOAT_MAT4X3,
		TYPE_FLOAT_MAT4,

		TYPE_SAMPLER_2D,
		TYPE_INT_SAMPLER_2D,
		TYPE_UINT_SAMPLER_2D,
	};

	const int maxPrimitiveTypeNdx = DE_LENGTH_OF_ARRAY(primitiveTypes) - 4;
	DE_ASSERT(primitiveTypes[maxPrimitiveTypeNdx] == TYPE_FLOAT_MAT4);

	// Primitive type cases with trivial linkage
	{
		tcu::TestCaseGroup* const	group	= new tcu::TestCaseGroup(m_testCtx, "basic", "Location specified with use, single shader stage");
		de::Random					rng		(baseSeed + 0x1001);
		addChild(group);

		for (int primitiveNdx = 0; primitiveNdx < DE_LENGTH_OF_ARRAY(primitiveTypes); primitiveNdx++)
		{
			const DataType		type	= primitiveTypes[primitiveNdx];

			for (int stageNdx = 0; stageNdx < DE_LENGTH_OF_ARRAY(checkStages); stageNdx++)
			{
				const string		name		= string(getDataTypeName(type)) + "_" + stageNames[stageNdx];

				vector<UniformInfo> config;

				UniformInfo			uniform	(createVarType(type),
											 checkStages[stageNdx],
											 checkStages[stageNdx],
											 checkStages[stageNdx],
											 rng.getInt(0, maxLocations-1));

				config.push_back(uniform);
				group->addChild(new UniformLocationCase (m_testCtx, m_context.getRenderContext(), name.c_str(), name.c_str(), config));
			}
		}
	}

	// Arrays
	{
		tcu::TestCaseGroup* const	group	= new tcu::TestCaseGroup(m_testCtx, "array", "Array location specified with use, single shader stage");
		de::Random					rng		(baseSeed + 0x2001);
		addChild(group);

		for (int primitiveNdx = 0; primitiveNdx < DE_LENGTH_OF_ARRAY(primitiveTypes); primitiveNdx++)
		{
			const DataType		type	= primitiveTypes[primitiveNdx];

			for (int stageNdx = 0; stageNdx < DE_LENGTH_OF_ARRAY(checkStages); stageNdx++)
			{

				const string		name	= string(getDataTypeName(type)) + "_" + stageNames[stageNdx];

				vector<UniformInfo> config;

				UniformInfo			uniform	(VarType(createVarType(type), 8),
												checkStages[stageNdx],
												checkStages[stageNdx],
												checkStages[stageNdx],
												rng.getInt(0, maxLocations-1-8));

				config.push_back(uniform);
				group->addChild(new UniformLocationCase (m_testCtx, m_context.getRenderContext(), name.c_str(), name.c_str(), config));
			}
		}
	}

	// Nested Arrays
	{
		tcu::TestCaseGroup* const	group	= new tcu::TestCaseGroup(m_testCtx, "nested_array", "Array location specified with use, single shader stage");
		de::Random					rng		(baseSeed + 0x3001);
		addChild(group);

		for (int primitiveNdx = 0; primitiveNdx < DE_LENGTH_OF_ARRAY(primitiveTypes); primitiveNdx++)
		{
			const DataType		type	= primitiveTypes[primitiveNdx];

			for (int stageNdx = 0; stageNdx < DE_LENGTH_OF_ARRAY(checkStages); stageNdx++)
			{
				const string		name		= string(getDataTypeName(type)) + "_" + stageNames[stageNdx];
				// stay comfortably within minimum max uniform component count (896 in fragment) and sampler count with all types
				const int			arraySize	= (getDataTypeScalarSize(type) > 4 || isDataTypeSampler(type)) ? 3 : 7;

				vector<UniformInfo> config;

				UniformInfo			uniform	(VarType(VarType(createVarType(type), arraySize), arraySize),
											 checkStages[stageNdx],
											 checkStages[stageNdx],
											 checkStages[stageNdx],
											 rng.getInt(0, maxLocations-1-arraySize*arraySize));

				config.push_back(uniform);
				group->addChild(new UniformLocationCase (m_testCtx, m_context.getRenderContext(), name.c_str(), name.c_str(), config));
			}
		}
	}

	// Structs
	{
		tcu::TestCaseGroup* const	group	= new tcu::TestCaseGroup(m_testCtx, "struct", "Struct location, random contents & declaration location");
		de::Random					rng		(baseSeed + 0x4001);
		addChild(group);

		for (int caseNdx = 0; caseNdx < 16; caseNdx++)
		{
			typedef UniformInfo::ShaderStage Stage;

			const string	name		= "case_" + de::toString(caseNdx);

			const Stage		layoutLoc	= Stage(rng.getUint32()&0x3);
			const Stage		declareLoc	= Stage((rng.getUint32()&0x3) | layoutLoc);
			const Stage		verifyLoc	= Stage((rng.getUint32()&0x3) & declareLoc);
			const int		location	= layoutLoc ? rng.getInt(0, maxLocations-1-5) : -1;

			StructType*		structProto = new StructType("S");

			structTypes.push_back(structProto);

			structProto->addMember("a", createVarType(primitiveTypes[rng.getInt(0, maxPrimitiveTypeNdx)]));
			structProto->addMember("b", createVarType(primitiveTypes[rng.getInt(0, maxPrimitiveTypeNdx)]));
			structProto->addMember("c", createVarType(primitiveTypes[rng.getInt(0, maxPrimitiveTypeNdx)]));
			structProto->addMember("d", createVarType(primitiveTypes[rng.getInt(0, maxPrimitiveTypeNdx)]));
			structProto->addMember("e", createVarType(primitiveTypes[rng.getInt(0, maxPrimitiveTypeNdx)]));

			{
				vector<UniformInfo> config;

				config.push_back(UniformInfo(VarType(structProto),
											 declareLoc,
											 layoutLoc,
											 verifyLoc,
											 location));
				group->addChild(new UniformLocationCase (m_testCtx, m_context.getRenderContext(), name.c_str(), name.c_str(), config));
			}
		}
	}

	// Nested Structs
	{
		tcu::TestCaseGroup* const	group	= new tcu::TestCaseGroup(m_testCtx, "nested_struct", "Struct location specified with use, single shader stage");
		de::Random					rng		(baseSeed + 0x5001);

		addChild(group);

		for (int caseNdx = 0; caseNdx < 16; caseNdx++)
		{
			typedef UniformInfo::ShaderStage Stage;

			const string	name		= "case_" + de::toString(caseNdx);
			const int		baseLoc		= rng.getInt(0, maxLocations-1-60);

			// Structs need to be added in the order of their declaration
			const Stage		layoutLocs[]=
			{
				Stage(rng.getUint32()&0x3),
				Stage(rng.getUint32()&0x3),
				Stage(rng.getUint32()&0x3),
				Stage(rng.getUint32()&0x3),
			};

			const deUint32	tempDecl[] =
			{
				(rng.getUint32()&0x3) | layoutLocs[0],
				(rng.getUint32()&0x3) | layoutLocs[1],
				(rng.getUint32()&0x3) | layoutLocs[2],
				(rng.getUint32()&0x3) | layoutLocs[3],
			};

			// Component structs need to be declared if anything using them is declared
			const Stage		declareLocs[] =
			{
				Stage(tempDecl[0] | tempDecl[1] | tempDecl[2] | tempDecl[3]),
				Stage(tempDecl[1] | tempDecl[2] | tempDecl[3]),
				Stage(tempDecl[2] | tempDecl[3]),
				Stage(tempDecl[3]),
			};

			const Stage		verifyLocs[] =
			{
				Stage(rng.getUint32()&0x3 & declareLocs[0]),
				Stage(rng.getUint32()&0x3 & declareLocs[1]),
				Stage(rng.getUint32()&0x3 & declareLocs[2]),
				Stage(rng.getUint32()&0x3 & declareLocs[3]),
			};

			StructType*		testTypes[]	=
			{
				new StructType("Type0"),
				new StructType("Type1"),
				new StructType("Type2"),
				new StructType("Type3"),
			};

			structTypes.push_back(testTypes[0]);
			structTypes.push_back(testTypes[1]);
			structTypes.push_back(testTypes[2]);
			structTypes.push_back(testTypes[3]);

			testTypes[0]->addMember("a", createVarType(primitiveTypes[rng.getInt(0, maxPrimitiveTypeNdx)]));
			testTypes[0]->addMember("b", createVarType(primitiveTypes[rng.getInt(0, maxPrimitiveTypeNdx)]));
			testTypes[0]->addMember("c", createVarType(primitiveTypes[rng.getInt(0, maxPrimitiveTypeNdx)]));
			testTypes[0]->addMember("d", createVarType(primitiveTypes[rng.getInt(0, maxPrimitiveTypeNdx)]));
			testTypes[0]->addMember("e", createVarType(primitiveTypes[rng.getInt(0, maxPrimitiveTypeNdx)]));

			testTypes[1]->addMember("a", createVarType(primitiveTypes[rng.getInt(0, maxPrimitiveTypeNdx)]));
			testTypes[1]->addMember("b", createVarType(primitiveTypes[rng.getInt(0, maxPrimitiveTypeNdx)]));
			testTypes[1]->addMember("c", createVarType(primitiveTypes[rng.getInt(0, maxPrimitiveTypeNdx)]));
			testTypes[1]->addMember("d", createVarType(primitiveTypes[rng.getInt(0, maxPrimitiveTypeNdx)]));
			testTypes[1]->addMember("e", createVarType(primitiveTypes[rng.getInt(0, maxPrimitiveTypeNdx)]));

			testTypes[2]->addMember("a", VarType(testTypes[0]));
			testTypes[2]->addMember("b", VarType(testTypes[1]));
			testTypes[2]->addMember("c", createVarType(primitiveTypes[rng.getInt(0, maxPrimitiveTypeNdx)]));

			testTypes[3]->addMember("a", VarType(testTypes[2]));

			{
				vector<UniformInfo> config;

				config.push_back(UniformInfo(VarType(testTypes[0]),
											 declareLocs[0],
											 layoutLocs[0],
											 verifyLocs[0],
											 layoutLocs[0] ? baseLoc : -1));

				config.push_back(UniformInfo(VarType(testTypes[1]),
											 declareLocs[1],
											 layoutLocs[1],
											 verifyLocs[1],
											 layoutLocs[1] ? baseLoc+5 : -1));

				config.push_back(UniformInfo(VarType(testTypes[2]),
											 declareLocs[2],
											 layoutLocs[2],
											 verifyLocs[2],
											 layoutLocs[2] ? baseLoc+16 : -1));

				config.push_back(UniformInfo(VarType(testTypes[3]),
											 declareLocs[3],
											 layoutLocs[3],
											 verifyLocs[3],
											 layoutLocs[3] ? baseLoc+27 : -1));

				group->addChild(new UniformLocationCase (m_testCtx, m_context.getRenderContext(), name.c_str(), name.c_str(), config));
			}
		}
	}

	// Min/Max location
	{
		tcu::TestCaseGroup* const	group		= new tcu::TestCaseGroup(m_testCtx, "min_max", "Maximum & minimum location");

		addChild(group);

		for (int primitiveNdx = 0; primitiveNdx < DE_LENGTH_OF_ARRAY(primitiveTypes); primitiveNdx++)
		{
			const DataType		type	= primitiveTypes[primitiveNdx];

			for (int stageNdx = 0; stageNdx < DE_LENGTH_OF_ARRAY(checkStages); stageNdx++)
			{
				const string		name		= string(getDataTypeName(type)) + "_" + stageNames[stageNdx];
				vector<UniformInfo> config;

				config.push_back(UniformInfo(createVarType(type),
											 checkStages[stageNdx],
											 checkStages[stageNdx],
											 checkStages[stageNdx],
											 0));

				group->addChild(new UniformLocationCase (m_testCtx, m_context.getRenderContext(), (name+"_min").c_str(), (name+"_min").c_str(), config));

				group->addChild(new MaxUniformLocationCase (m_testCtx, m_context.getRenderContext(), (name+"_max").c_str(), (name+"_max").c_str(), config));
			}
		}
	}

	// Link
	{
		tcu::TestCaseGroup* const	group	= new tcu::TestCaseGroup(m_testCtx, "link", "Location specified independently from use");
		de::Random					rng		(baseSeed + 0x82e1);

		addChild(group);

		for (int caseNdx = 0; caseNdx < 10; caseNdx++)
		{
			const string		name		= "case_" + de::toString(caseNdx);
			vector<UniformInfo> config;

			vector<int>			locations	= shuffledRange(0, maxLocations, 0x1234 + caseNdx*100);

			for (int count = 0; count < 32; count++)
			{
				typedef UniformInfo::ShaderStage Stage;

				const Stage			layoutLoc	= Stage(rng.getUint32()&0x3);
				const Stage			declareLoc	= Stage((rng.getUint32()&0x3) | layoutLoc);
				const Stage			verifyLoc	= Stage((rng.getUint32()&0x3) & declareLoc);

				const UniformInfo	uniform		(createVarType(primitiveTypes[rng.getInt(0, maxPrimitiveTypeNdx)]),
												 declareLoc,
												 layoutLoc,
												 verifyLoc,
												 (layoutLoc!=0) ? locations.back() : -1);

				config.push_back(uniform);
				locations.pop_back();
			}
			group->addChild(new UniformLocationCase (m_testCtx, m_context.getRenderContext(), name.c_str(), name.c_str(), config));
		}
	}

	// Negative
	{
		de::MovePtr<tcu::TestCaseGroup>	negativeGroup			(new tcu::TestCaseGroup(m_testCtx, "negative", "Negative tests"));

		{
			de::MovePtr<tcu::TestCaseGroup>	es31Group		(new tcu::TestCaseGroup(m_testCtx, "es31", "GLSL ES 3.1 Negative tests"));
			gls::ShaderLibrary				shaderLibrary   (m_testCtx, m_context.getRenderContext(), m_context.getContextInfo());
			const vector<TestNode*>			negativeCases    = shaderLibrary.loadShaderFile("shaders/es31/uniform_location.test");

			for (int ndx = 0; ndx < int(negativeCases.size()); ndx++)
				es31Group->addChild(negativeCases[ndx]);

			negativeGroup->addChild(es31Group.release());
		}

		{
			de::MovePtr<tcu::TestCaseGroup>	es32Group		(new tcu::TestCaseGroup(m_testCtx, "es32", "GLSL ES 3.2 Negative tests"));
			gls::ShaderLibrary				shaderLibrary   (m_testCtx, m_context.getRenderContext(), m_context.getContextInfo());
			const vector<TestNode*>			negativeCases    = shaderLibrary.loadShaderFile("shaders/es32/uniform_location.test");

			for (int ndx = 0; ndx < int(negativeCases.size()); ndx++)
				es32Group->addChild(negativeCases[ndx]);

			negativeGroup->addChild(es32Group.release());
		}

		addChild(negativeGroup.release());
	}
}

} // Functional
} // gles31
} // deqp
