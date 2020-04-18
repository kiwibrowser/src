/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2016 Google Inc.
 * Copyright (c) 2016 The Khronos Group Inc.
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
 */ /*!
 * \file
 * \brief Compiler test case.
 */ /*-------------------------------------------------------------------*/

#include "glcShaderLibraryCase.hpp"

#include "tcuRenderTarget.hpp"
#include "tcuTestLog.hpp"

#include "gluDrawUtil.hpp"
#include "gluPixelTransfer.hpp"
#include "gluShaderProgram.hpp"
#include "tcuStringTemplate.hpp"

#include "glwEnums.hpp"
#include "glwFunctions.hpp"

#include "deInt32.h"
#include "deMath.h"
#include "deRandom.hpp"
#include "deString.h"

#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
using namespace tcu;
using namespace glu;

namespace deqp
{
namespace sl
{

enum
{
	VIEWPORT_WIDTH  = 128,
	VIEWPORT_HEIGHT = 128
};

static inline bool usesShaderInoutQualifiers(glu::GLSLVersion version)
{
	switch (version)
	{
	case glu::GLSL_VERSION_100_ES:
	case glu::GLSL_VERSION_130:
	case glu::GLSL_VERSION_140:
	case glu::GLSL_VERSION_150:
		return false;

	default:
		return true;
	}
}

// ShaderCase.

ShaderCase::ShaderCase(tcu::TestContext& testCtx, RenderContext& renderCtx, const char* name, const char* description,
					   ExpectResult expectResult, const std::vector<ValueBlock>& valueBlocks, GLSLVersion targetVersion,
					   const char* vertexSource, const char* fragmentSource)
	: tcu::TestCase(testCtx, name, description)
	, m_renderCtx(renderCtx)
	, m_expectResult(expectResult)
	, m_valueBlocks(valueBlocks)
	, m_targetVersion(targetVersion)
{
	// If no value blocks given, use an empty one.
	if (m_valueBlocks.size() == 0)
		m_valueBlocks.push_back(ValueBlock());

	// Use first value block to specialize shaders.
	const ValueBlock& valueBlock = m_valueBlocks[0];

	// \todo [2010-04-01 petri] Check that all value blocks have matching values.

	// Generate specialized shader sources.
	if (vertexSource && fragmentSource)
	{
		m_caseType = CASETYPE_COMPLETE;
		specializeShaders(vertexSource, fragmentSource, m_vertexSource, m_fragmentSource, valueBlock);
	}
	else if (vertexSource)
	{
		m_caseType		 = CASETYPE_VERTEX_ONLY;
		m_vertexSource   = specializeVertexShader(vertexSource, valueBlock);
		m_fragmentSource = genFragmentShader(valueBlock);
	}
	else
	{
		DE_ASSERT(fragmentSource);
		m_caseType		 = CASETYPE_FRAGMENT_ONLY;
		m_vertexSource   = genVertexShader(valueBlock);
		m_fragmentSource = specializeFragmentShader(fragmentSource, valueBlock);
	}
}

ShaderCase::~ShaderCase(void)
{
}

static void setUniformValue(const glw::Functions& gl, deUint32 programID, const std::string& name,
							const ShaderCase::Value& val, int arrayNdx)
{
	int scalarSize = getDataTypeScalarSize(val.dataType);
	int loc		   = gl.getUniformLocation(programID, name.c_str());

	TCU_CHECK_MSG(loc != -1, "uniform location not found");

	DE_STATIC_ASSERT(sizeof(ShaderCase::Value::Element) == sizeof(glw::GLfloat));
	DE_STATIC_ASSERT(sizeof(ShaderCase::Value::Element) == sizeof(glw::GLint));

	int elemNdx = (val.arrayLength == 1) ? 0 : (arrayNdx * scalarSize);

	switch (val.dataType)
	{
	case TYPE_FLOAT:
		gl.uniform1fv(loc, 1, &val.elements[elemNdx].float32);
		break;
	case TYPE_FLOAT_VEC2:
		gl.uniform2fv(loc, 1, &val.elements[elemNdx].float32);
		break;
	case TYPE_FLOAT_VEC3:
		gl.uniform3fv(loc, 1, &val.elements[elemNdx].float32);
		break;
	case TYPE_FLOAT_VEC4:
		gl.uniform4fv(loc, 1, &val.elements[elemNdx].float32);
		break;
	case TYPE_FLOAT_MAT2:
		gl.uniformMatrix2fv(loc, 1, GL_FALSE, &val.elements[elemNdx].float32);
		break;
	case TYPE_FLOAT_MAT3:
		gl.uniformMatrix3fv(loc, 1, GL_FALSE, &val.elements[elemNdx].float32);
		break;
	case TYPE_FLOAT_MAT4:
		gl.uniformMatrix4fv(loc, 1, GL_FALSE, &val.elements[elemNdx].float32);
		break;
	case TYPE_INT:
		gl.uniform1iv(loc, 1, &val.elements[elemNdx].int32);
		break;
	case TYPE_INT_VEC2:
		gl.uniform2iv(loc, 1, &val.elements[elemNdx].int32);
		break;
	case TYPE_INT_VEC3:
		gl.uniform3iv(loc, 1, &val.elements[elemNdx].int32);
		break;
	case TYPE_INT_VEC4:
		gl.uniform4iv(loc, 1, &val.elements[elemNdx].int32);
		break;
	case TYPE_BOOL:
		gl.uniform1iv(loc, 1, &val.elements[elemNdx].int32);
		break;
	case TYPE_BOOL_VEC2:
		gl.uniform2iv(loc, 1, &val.elements[elemNdx].int32);
		break;
	case TYPE_BOOL_VEC3:
		gl.uniform3iv(loc, 1, &val.elements[elemNdx].int32);
		break;
	case TYPE_BOOL_VEC4:
		gl.uniform4iv(loc, 1, &val.elements[elemNdx].int32);
		break;
	case TYPE_UINT:
		gl.uniform1uiv(loc, 1, (const deUint32*)&val.elements[elemNdx].int32);
		break;
	case TYPE_UINT_VEC2:
		gl.uniform2uiv(loc, 1, (const deUint32*)&val.elements[elemNdx].int32);
		break;
	case TYPE_UINT_VEC3:
		gl.uniform3uiv(loc, 1, (const deUint32*)&val.elements[elemNdx].int32);
		break;
	case TYPE_UINT_VEC4:
		gl.uniform4uiv(loc, 1, (const deUint32*)&val.elements[elemNdx].int32);
		break;
	case TYPE_FLOAT_MAT2X3:
		gl.uniformMatrix2x3fv(loc, 1, GL_FALSE, &val.elements[elemNdx].float32);
		break;
	case TYPE_FLOAT_MAT2X4:
		gl.uniformMatrix2x4fv(loc, 1, GL_FALSE, &val.elements[elemNdx].float32);
		break;
	case TYPE_FLOAT_MAT3X2:
		gl.uniformMatrix3x2fv(loc, 1, GL_FALSE, &val.elements[elemNdx].float32);
		break;
	case TYPE_FLOAT_MAT3X4:
		gl.uniformMatrix3x4fv(loc, 1, GL_FALSE, &val.elements[elemNdx].float32);
		break;
	case TYPE_FLOAT_MAT4X2:
		gl.uniformMatrix4x2fv(loc, 1, GL_FALSE, &val.elements[elemNdx].float32);
		break;
	case TYPE_FLOAT_MAT4X3:
		gl.uniformMatrix4x3fv(loc, 1, GL_FALSE, &val.elements[elemNdx].float32);
		break;

	case TYPE_SAMPLER_2D:
	case TYPE_SAMPLER_CUBE:
		DE_ASSERT(DE_FALSE && "implement!");
		break;

	default:
		DE_ASSERT(false);
	}
}

bool ShaderCase::checkPixels(Surface& surface, int minX, int maxX, int minY, int maxY)
{
	TestLog& log		   = m_testCtx.getLog();
	bool	 allWhite	  = true;
	bool	 allBlack	  = true;
	bool	 anyUnexpected = false;

	DE_ASSERT((maxX > minX) && (maxY > minY));

	for (int y = minY; y <= maxY; y++)
	{
		for (int x = minX; x <= maxX; x++)
		{
			RGBA pixel = surface.getPixel(x, y);
			// Note: we really do not want to involve alpha in the check comparison
			// \todo [2010-09-22 kalle] Do we know that alpha would be one? If yes, could use color constants white and black.
			bool isWhite = (pixel.getRed() == 255) && (pixel.getGreen() == 255) && (pixel.getBlue() == 255);
			bool isBlack = (pixel.getRed() == 0) && (pixel.getGreen() == 0) && (pixel.getBlue() == 0);

			allWhite	  = allWhite && isWhite;
			allBlack	  = allBlack && isBlack;
			anyUnexpected = anyUnexpected || (!isWhite && !isBlack);
		}
	}

	if (!allWhite)
	{
		if (anyUnexpected)
			log << TestLog::Message
				<< "WARNING: expecting all rendered pixels to be white or black, but got other colors as well!"
				<< TestLog::EndMessage;
		else if (!allBlack)
			log << TestLog::Message
				<< "WARNING: got inconsistent results over the image, when all pixels should be the same color!"
				<< TestLog::EndMessage;

		return false;
	}
	return true;
}

bool ShaderCase::execute(void)
{
	TestLog&			  log = m_testCtx.getLog();
	const glw::Functions& gl  = m_renderCtx.getFunctions();

	// Compute viewport.
	const tcu::RenderTarget& renderTarget = m_renderCtx.getRenderTarget();
	de::Random				 rnd(deStringHash(getName()));
	int						 width				= deMin32(renderTarget.getWidth(), VIEWPORT_WIDTH);
	int						 height				= deMin32(renderTarget.getHeight(), VIEWPORT_HEIGHT);
	int						 viewportX			= rnd.getInt(0, renderTarget.getWidth() - width);
	int						 viewportY			= rnd.getInt(0, renderTarget.getHeight() - height);
	const int				 numVerticesPerDraw = 4;

	GLU_EXPECT_NO_ERROR(gl.getError(), "ShaderCase::execute(): start");

	// Setup viewport.
	gl.viewport(viewportX, viewportY, width, height);

	const float		   quadSize			  = 1.0f;
	static const float s_positions[4 * 4] = { -quadSize, -quadSize, 0.0f, 1.0f, -quadSize, +quadSize, 0.0f, 1.0f,
											  +quadSize, -quadSize, 0.0f, 1.0f, +quadSize, +quadSize, 0.0f, 1.0f };

	static const deUint16 s_indices[2 * 3] = { 0, 1, 2, 1, 3, 2 };

	// Setup program.
	glu::ShaderProgram program(m_renderCtx, glu::makeVtxFragSources(m_vertexSource.c_str(), m_fragmentSource.c_str()));

	// Check that compile/link results are what we expect.
	bool		vertexOk   = program.getShaderInfo(SHADERTYPE_VERTEX).compileOk;
	bool		fragmentOk = program.getShaderInfo(SHADERTYPE_FRAGMENT).compileOk;
	bool		linkOk	 = program.getProgramInfo().linkOk;
	const char* failReason = DE_NULL;

	log << program;

	switch (m_expectResult)
	{
	case EXPECT_PASS:
		if (!vertexOk || !fragmentOk)
			failReason = "expected shaders to compile and link properly, but failed to compile.";
		else if (!linkOk)
			failReason = "expected shaders to compile and link properly, but failed to link.";
		break;

	case EXPECT_COMPILE_FAIL:
		if (vertexOk && fragmentOk && !linkOk)
			failReason = "expected compilation to fail, but both shaders compiled and link failed.";
		else if (vertexOk && fragmentOk)
			failReason = "expected compilation to fail, but both shaders compiled correctly.";
		break;

	case EXPECT_LINK_FAIL:
		if (!vertexOk || !fragmentOk)
			failReason = "expected linking to fail, but unable to compile.";
		else if (linkOk)
			failReason = "expected linking to fail, but passed.";
		break;

	default:
		DE_ASSERT(false);
		return false;
	}

	if (failReason != DE_NULL)
	{
		// \todo [2010-06-07 petri] These should be handled in the test case?
		log << TestLog::Message << "ERROR: " << failReason << TestLog::EndMessage;

		// If implementation parses shader at link time, report it as quality warning.
		if (m_expectResult == EXPECT_COMPILE_FAIL && vertexOk && fragmentOk && !linkOk)
			m_testCtx.setTestResult(QP_TEST_RESULT_QUALITY_WARNING, failReason);
		else
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, failReason);
		return false;
	}

	// Return if compile/link expected to fail.
	if (m_expectResult != EXPECT_PASS)
		return (failReason == DE_NULL);

	// Start using program.
	deUint32 programID = program.getProgram();
	gl.useProgram(programID);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram()");

	// Fetch location for positions positions.
	int positionLoc = gl.getAttribLocation(programID, "dEQP_Position");
	if (positionLoc == -1)
	{
		string errStr = string("no location found for attribute 'dEQP_Position'");
		TCU_FAIL(errStr.c_str());
	}

	// Iterate all value blocks.
	for (int blockNdx = 0; blockNdx < (int)m_valueBlocks.size(); blockNdx++)
	{
		const ValueBlock& valueBlock = m_valueBlocks[blockNdx];

		// Iterate all array sub-cases.
		for (int arrayNdx = 0; arrayNdx < valueBlock.arrayLength; arrayNdx++)
		{
			int						   numValues = (int)valueBlock.values.size();
			vector<VertexArrayBinding> vertexArrays;

			int					   attribValueNdx = 0;
			vector<vector<float> > attribValues(numValues);

			vertexArrays.push_back(va::Float(positionLoc, 4, numVerticesPerDraw, 0, &s_positions[0]));

			// Collect VA pointer for inputs and set uniform values for outputs (refs).
			for (int valNdx = 0; valNdx < numValues; valNdx++)
			{
				const ShaderCase::Value& val		= valueBlock.values[valNdx];
				const char*				 valueName  = val.valueName.c_str();
				DataType				 dataType   = val.dataType;
				int						 scalarSize = getDataTypeScalarSize(val.dataType);

				GLU_EXPECT_NO_ERROR(gl.getError(), "before set uniforms");

				if (val.storageType == ShaderCase::Value::STORAGE_INPUT)
				{
					// Replicate values four times.
					std::vector<float>& scalars = attribValues[attribValueNdx++];
					scalars.resize(numVerticesPerDraw * scalarSize);
					if (isDataTypeFloatOrVec(dataType) || isDataTypeMatrix(dataType))
					{
						for (int repNdx = 0; repNdx < numVerticesPerDraw; repNdx++)
							for (int ndx						   = 0; ndx < scalarSize; ndx++)
								scalars[repNdx * scalarSize + ndx] = val.elements[arrayNdx * scalarSize + ndx].float32;
					}
					else
					{
						// convert to floats.
						for (int repNdx = 0; repNdx < numVerticesPerDraw; repNdx++)
						{
							for (int ndx = 0; ndx < scalarSize; ndx++)
							{
								float v = (float)val.elements[arrayNdx * scalarSize + ndx].int32;
								DE_ASSERT(val.elements[arrayNdx * scalarSize + ndx].int32 == (int)v);
								scalars[repNdx * scalarSize + ndx] = v;
							}
						}
					}

					// Attribute name prefix.
					string attribPrefix = "";
					// \todo [2010-05-27 petri] Should latter condition only apply for vertex cases (or actually non-fragment cases)?
					if ((m_caseType == CASETYPE_FRAGMENT_ONLY) || (getDataTypeScalarType(dataType) != TYPE_FLOAT))
						attribPrefix = "a_";

					// Input always given as attribute.
					string attribName = attribPrefix + valueName;
					int	attribLoc  = gl.getAttribLocation(programID, attribName.c_str());
					if (attribLoc == -1)
					{
						log << TestLog::Message << "Warning: no location found for attribute '" << attribName << "'"
							<< TestLog::EndMessage;
						continue;
					}

					if (isDataTypeMatrix(dataType))
					{
						int numCols = getDataTypeMatrixNumColumns(dataType);
						int numRows = getDataTypeMatrixNumRows(dataType);
						DE_ASSERT(scalarSize == numCols * numRows);

						for (int i = 0; i < numCols; i++)
							vertexArrays.push_back(va::Float(attribLoc + i, numRows, numVerticesPerDraw,
															 static_cast<int>(scalarSize * sizeof(float)),
															 &scalars[i * numRows]));
					}
					else
					{
						DE_ASSERT(isDataTypeFloatOrVec(dataType) || isDataTypeIntOrIVec(dataType) ||
								  isDataTypeUintOrUVec(dataType) || isDataTypeBoolOrBVec(dataType));
						vertexArrays.push_back(va::Float(attribLoc, scalarSize, numVerticesPerDraw, 0, &scalars[0]));
					}

					GLU_EXPECT_NO_ERROR(gl.getError(), "set vertex attrib array");
				}
				else if (val.storageType == ShaderCase::Value::STORAGE_OUTPUT)
				{
					// Set reference value.
					string refName = string("ref_") + valueName;
					setUniformValue(gl, programID, refName, val, arrayNdx);
					GLU_EXPECT_NO_ERROR(gl.getError(), "set reference uniforms");
				}
				else
				{
					DE_ASSERT(val.storageType == ShaderCase::Value::STORAGE_UNIFORM);
					setUniformValue(gl, programID, valueName, val, arrayNdx);
					GLU_EXPECT_NO_ERROR(gl.getError(), "set uniforms");
				}
			}

			// Clear.
			gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
			gl.clear(GL_COLOR_BUFFER_BIT);
			GLU_EXPECT_NO_ERROR(gl.getError(), "clear buffer");

			// Draw.
			draw(m_renderCtx, program.getProgram(), (int)vertexArrays.size(), &vertexArrays[0],
				 pr::Triangles(DE_LENGTH_OF_ARRAY(s_indices), &s_indices[0]));
			GLU_EXPECT_NO_ERROR(gl.getError(), "draw");

			// Read back results.
			Surface surface(width, height);
			glu::readPixels(m_renderCtx, viewportX, viewportY, surface.getAccess());
			GLU_EXPECT_NO_ERROR(gl.getError(), "read pixels");

			float w	= s_positions[3];
			int   minY = deCeilFloatToInt32(((-quadSize / w) * 0.5f + 0.5f) * (float)height + 1.0f);
			int   maxY = deFloorFloatToInt32(((+quadSize / w) * 0.5f + 0.5f) * (float)height - 0.5f);
			int   minX = deCeilFloatToInt32(((-quadSize / w) * 0.5f + 0.5f) * (float)width + 1.0f);
			int   maxX = deFloorFloatToInt32(((+quadSize / w) * 0.5f + 0.5f) * (float)width - 0.5f);

			if (!checkPixels(surface, minX, maxX, minY, maxY))
			{
				log << TestLog::Message << "INCORRECT RESULT for (value block " << (blockNdx + 1) << " of "
					<< (int)m_valueBlocks.size() << ", sub-case " << arrayNdx + 1 << " of " << valueBlock.arrayLength
					<< "):" << TestLog::EndMessage;

				log << TestLog::Message << "Failing shader input/output values:" << TestLog::EndMessage;
				dumpValues(valueBlock, arrayNdx);

				// Dump image on failure.
				log << TestLog::Image("Result", "Rendered result image", surface);

				gl.useProgram(0);
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Image comparison failed");
				return false;
			}
		}
	}

	gl.useProgram(0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "ShaderCase::execute(): end");
	return true;
}

TestCase::IterateResult ShaderCase::iterate(void)
{
	// Initialize state to pass.
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	bool executeOk = execute();

	DE_ASSERT(executeOk ? m_testCtx.getTestResult() == QP_TEST_RESULT_PASS :
						  m_testCtx.getTestResult() != QP_TEST_RESULT_PASS);
	(void)executeOk;
	return TestCase::STOP;
}

// This functions builds a matching vertex shader for a 'both' case, when
// the fragment shader is being tested.
// We need to build attributes and varyings for each 'input'.
string ShaderCase::genVertexShader(const ValueBlock& valueBlock)
{
	ostringstream res;
	const bool	usesInout = usesShaderInoutQualifiers(m_targetVersion);
	const char*   vtxIn		= usesInout ? "in" : "attribute";
	const char*   vtxOut	= usesInout ? "out" : "varying";

	res << glu::getGLSLVersionDeclaration(m_targetVersion) << "\n";

	// Declarations (position + attribute/varying for each input).
	res << "precision highp float;\n";
	res << "precision highp int;\n";
	res << "\n";
	res << vtxIn << " highp vec4 dEQP_Position;\n";
	for (int ndx = 0; ndx < (int)valueBlock.values.size(); ndx++)
	{
		const ShaderCase::Value& val = valueBlock.values[ndx];
		if (val.storageType == ShaderCase::Value::STORAGE_INPUT)
		{
			DataType	floatType = getDataTypeFloatScalars(val.dataType);
			const char* typeStr   = getDataTypeName(floatType);
			res << vtxIn << " " << typeStr << " a_" << val.valueName << ";\n";

			if (getDataTypeScalarType(val.dataType) == TYPE_FLOAT)
				res << vtxOut << " " << typeStr << " " << val.valueName << ";\n";
			else
				res << vtxOut << " " << typeStr << " v_" << val.valueName << ";\n";
		}
	}
	res << "\n";

	// Main function.
	// - gl_Position = dEQP_Position;
	// - for each input: write attribute directly to varying
	res << "void main()\n";
	res << "{\n";
	res << "    gl_Position = dEQP_Position;\n";
	for (int ndx = 0; ndx < (int)valueBlock.values.size(); ndx++)
	{
		const ShaderCase::Value& val = valueBlock.values[ndx];
		if (val.storageType == ShaderCase::Value::STORAGE_INPUT)
		{
			const string& name = val.valueName;
			if (getDataTypeScalarType(val.dataType) == TYPE_FLOAT)
				res << "    " << name << " = a_" << name << ";\n";
			else
				res << "    v_" << name << " = a_" << name << ";\n";
		}
	}

	res << "}\n";
	return res.str();
}

static void genCompareFunctions(ostringstream& stream, const ShaderCase::ValueBlock& valueBlock, bool useFloatTypes)
{
	bool cmpTypeFound[TYPE_LAST];
	for (int i			= 0; i < TYPE_LAST; i++)
		cmpTypeFound[i] = false;

	for (int valueNdx = 0; valueNdx < (int)valueBlock.values.size(); valueNdx++)
	{
		const ShaderCase::Value& val = valueBlock.values[valueNdx];
		if (val.storageType == ShaderCase::Value::STORAGE_OUTPUT)
			cmpTypeFound[(int)val.dataType] = true;
	}

	if (useFloatTypes)
	{
		if (cmpTypeFound[TYPE_BOOL])
			stream << "bool isOk (float a, bool b) { return ((a > 0.5) == b); }\n";
		if (cmpTypeFound[TYPE_BOOL_VEC2])
			stream << "bool isOk (vec2 a, bvec2 b) { return (greaterThan(a, vec2(0.5)) == b); }\n";
		if (cmpTypeFound[TYPE_BOOL_VEC3])
			stream << "bool isOk (vec3 a, bvec3 b) { return (greaterThan(a, vec3(0.5)) == b); }\n";
		if (cmpTypeFound[TYPE_BOOL_VEC4])
			stream << "bool isOk (vec4 a, bvec4 b) { return (greaterThan(a, vec4(0.5)) == b); }\n";
		if (cmpTypeFound[TYPE_INT])
			stream << "bool isOk (float a, int b)  { float atemp = a+0.5; return (float(b) <= atemp && atemp <= "
					  "float(b+1)); }\n";
		if (cmpTypeFound[TYPE_INT_VEC2])
			stream << "bool isOk (vec2 a, ivec2 b) { return (ivec2(floor(a + 0.5)) == b); }\n";
		if (cmpTypeFound[TYPE_INT_VEC3])
			stream << "bool isOk (vec3 a, ivec3 b) { return (ivec3(floor(a + 0.5)) == b); }\n";
		if (cmpTypeFound[TYPE_INT_VEC4])
			stream << "bool isOk (vec4 a, ivec4 b) { return (ivec4(floor(a + 0.5)) == b); }\n";
		if (cmpTypeFound[TYPE_UINT])
			stream << "bool isOk (float a, uint b) { float atemp = a+0.5; return (float(b) <= atemp && atemp <= "
					  "float(b+1)); }\n";
		if (cmpTypeFound[TYPE_UINT_VEC2])
			stream << "bool isOk (vec2 a, uvec2 b) { return (uvec2(floor(a + 0.5)) == b); }\n";
		if (cmpTypeFound[TYPE_UINT_VEC3])
			stream << "bool isOk (vec3 a, uvec3 b) { return (uvec3(floor(a + 0.5)) == b); }\n";
		if (cmpTypeFound[TYPE_UINT_VEC4])
			stream << "bool isOk (vec4 a, uvec4 b) { return (uvec4(floor(a + 0.5)) == b); }\n";
	}
	else
	{
		if (cmpTypeFound[TYPE_BOOL])
			stream << "bool isOk (bool a, bool b)   { return (a == b); }\n";
		if (cmpTypeFound[TYPE_BOOL_VEC2])
			stream << "bool isOk (bvec2 a, bvec2 b) { return (a == b); }\n";
		if (cmpTypeFound[TYPE_BOOL_VEC3])
			stream << "bool isOk (bvec3 a, bvec3 b) { return (a == b); }\n";
		if (cmpTypeFound[TYPE_BOOL_VEC4])
			stream << "bool isOk (bvec4 a, bvec4 b) { return (a == b); }\n";
		if (cmpTypeFound[TYPE_INT])
			stream << "bool isOk (int a, int b)     { return (a == b); }\n";
		if (cmpTypeFound[TYPE_INT_VEC2])
			stream << "bool isOk (ivec2 a, ivec2 b) { return (a == b); }\n";
		if (cmpTypeFound[TYPE_INT_VEC3])
			stream << "bool isOk (ivec3 a, ivec3 b) { return (a == b); }\n";
		if (cmpTypeFound[TYPE_INT_VEC4])
			stream << "bool isOk (ivec4 a, ivec4 b) { return (a == b); }\n";
		if (cmpTypeFound[TYPE_UINT])
			stream << "bool isOk (uint a, uint b)   { return (a == b); }\n";
		if (cmpTypeFound[TYPE_UINT_VEC2])
			stream << "bool isOk (uvec2 a, uvec2 b) { return (a == b); }\n";
		if (cmpTypeFound[TYPE_UINT_VEC3])
			stream << "bool isOk (uvec3 a, uvec3 b) { return (a == b); }\n";
		if (cmpTypeFound[TYPE_UINT_VEC4])
			stream << "bool isOk (uvec4 a, uvec4 b) { return (a == b); }\n";
	}

	if (cmpTypeFound[TYPE_FLOAT])
		stream << "bool isOk (float a, float b, float eps) { return (abs(a-b) <= (eps*abs(b) + eps)); }\n";
	if (cmpTypeFound[TYPE_FLOAT_VEC2])
		stream
			<< "bool isOk (vec2 a, vec2 b, float eps) { return all(lessThanEqual(abs(a-b), (eps*abs(b) + eps))); }\n";
	if (cmpTypeFound[TYPE_FLOAT_VEC3])
		stream
			<< "bool isOk (vec3 a, vec3 b, float eps) { return all(lessThanEqual(abs(a-b), (eps*abs(b) + eps))); }\n";
	if (cmpTypeFound[TYPE_FLOAT_VEC4])
		stream
			<< "bool isOk (vec4 a, vec4 b, float eps) { return all(lessThanEqual(abs(a-b), (eps*abs(b) + eps))); }\n";

	if (cmpTypeFound[TYPE_FLOAT_MAT2])
		stream << "bool isOk (mat2 a, mat2 b, float eps) { vec2 diff = max(abs(a[0]-b[0]), abs(a[1]-b[1])); return "
				  "all(lessThanEqual(diff, vec2(eps))); }\n";
	if (cmpTypeFound[TYPE_FLOAT_MAT2X3])
		stream << "bool isOk (mat2x3 a, mat2x3 b, float eps) { vec3 diff = max(abs(a[0]-b[0]), abs(a[1]-b[1])); return "
				  "all(lessThanEqual(diff, vec3(eps))); }\n";
	if (cmpTypeFound[TYPE_FLOAT_MAT2X4])
		stream << "bool isOk (mat2x4 a, mat2x4 b, float eps) { vec4 diff = max(abs(a[0]-b[0]), abs(a[1]-b[1])); return "
				  "all(lessThanEqual(diff, vec4(eps))); }\n";
	if (cmpTypeFound[TYPE_FLOAT_MAT3X2])
		stream << "bool isOk (mat3x2 a, mat3x2 b, float eps) { vec2 diff = max(max(abs(a[0]-b[0]), abs(a[1]-b[1])), "
				  "abs(a[2]-b[2])); return all(lessThanEqual(diff, vec2(eps))); }\n";
	if (cmpTypeFound[TYPE_FLOAT_MAT3])
		stream << "bool isOk (mat3 a, mat3 b, float eps) { vec3 diff = max(max(abs(a[0]-b[0]), abs(a[1]-b[1])), "
				  "abs(a[2]-b[2])); return all(lessThanEqual(diff, vec3(eps))); }\n";
	if (cmpTypeFound[TYPE_FLOAT_MAT3X4])
		stream << "bool isOk (mat3x4 a, mat3x4 b, float eps) { vec4 diff = max(max(abs(a[0]-b[0]), abs(a[1]-b[1])), "
				  "abs(a[2]-b[2])); return all(lessThanEqual(diff, vec4(eps))); }\n";
	if (cmpTypeFound[TYPE_FLOAT_MAT4X2])
		stream << "bool isOk (mat4x2 a, mat4x2 b, float eps) { vec2 diff = max(max(abs(a[0]-b[0]), abs(a[1]-b[1])), "
				  "max(abs(a[2]-b[2]), abs(a[3]-b[3]))); return all(lessThanEqual(diff, vec2(eps))); }\n";
	if (cmpTypeFound[TYPE_FLOAT_MAT4X3])
		stream << "bool isOk (mat4x3 a, mat4x3 b, float eps) { vec3 diff = max(max(abs(a[0]-b[0]), abs(a[1]-b[1])), "
				  "max(abs(a[2]-b[2]), abs(a[3]-b[3]))); return all(lessThanEqual(diff, vec3(eps))); }\n";
	if (cmpTypeFound[TYPE_FLOAT_MAT4])
		stream << "bool isOk (mat4 a, mat4 b, float eps) { vec4 diff = max(max(abs(a[0]-b[0]), abs(a[1]-b[1])), "
				  "max(abs(a[2]-b[2]), abs(a[3]-b[3]))); return all(lessThanEqual(diff, vec4(eps))); }\n";
}

static void genCompareOp(ostringstream& output, const char* dstVec4Var, const ShaderCase::ValueBlock& valueBlock,
						 const char* nonFloatNamePrefix, const char* checkVarName)
{
	bool isFirstOutput = true;

	for (int ndx = 0; ndx < (int)valueBlock.values.size(); ndx++)
	{
		const ShaderCase::Value& val	   = valueBlock.values[ndx];
		const char*				 valueName = val.valueName.c_str();

		if (val.storageType == ShaderCase::Value::STORAGE_OUTPUT)
		{
			// Check if we're only interested in one variable (then skip if not the right one).
			if (checkVarName && !deStringEqual(valueName, checkVarName))
				continue;

			// Prefix.
			if (isFirstOutput)
			{
				output << "bool RES = ";
				isFirstOutput = false;
			}
			else
				output << "RES = RES && ";

			// Generate actual comparison.
			if (getDataTypeScalarType(val.dataType) == TYPE_FLOAT)
				output << "isOk(" << valueName << ", ref_" << valueName << ", 0.05);\n";
			else
				output << "isOk(" << nonFloatNamePrefix << valueName << ", ref_" << valueName << ");\n";
		}
		// \note Uniforms are already declared in shader.
	}

	if (isFirstOutput)
		output << dstVec4Var << " = vec4(1.0);\n"; // \todo [petri] Should we give warning if not expect-failure case?
	else
		output << dstVec4Var << " = vec4(RES, RES, RES, 1.0);\n";
}

string ShaderCase::genFragmentShader(const ValueBlock& valueBlock)
{
	ostringstream shader;
	const bool	usesInout		 = usesShaderInoutQualifiers(m_targetVersion);
	const bool	customColorOut = usesInout;
	const char*   fragIn		 = usesInout ? "in" : "varying";

	shader << glu::getGLSLVersionDeclaration(m_targetVersion) << "\n";

	shader << "precision mediump float;\n";
	shader << "precision mediump int;\n";
	shader << "\n";

	if (customColorOut)
	{
		shader << "layout(location = 0) out mediump vec4 dEQP_FragColor;\n";
		shader << "\n";
	}

	genCompareFunctions(shader, valueBlock, true);
	shader << "\n";

	// Declarations (varying, reference for each output).
	for (int ndx = 0; ndx < (int)valueBlock.values.size(); ndx++)
	{
		const ShaderCase::Value& val		  = valueBlock.values[ndx];
		DataType				 floatType	= getDataTypeFloatScalars(val.dataType);
		const char*				 floatTypeStr = getDataTypeName(floatType);
		const char*				 refTypeStr   = getDataTypeName(val.dataType);

		if (val.storageType == ShaderCase::Value::STORAGE_OUTPUT)
		{
			if (getDataTypeScalarType(val.dataType) == TYPE_FLOAT)
				shader << fragIn << " " << floatTypeStr << " " << val.valueName << ";\n";
			else
				shader << fragIn << " " << floatTypeStr << " v_" << val.valueName << ";\n";

			shader << "uniform " << refTypeStr << " ref_" << val.valueName << ";\n";
		}
	}

	shader << "\n";
	shader << "void main()\n";
	shader << "{\n";

	shader << " ";
	genCompareOp(shader, customColorOut ? "dEQP_FragColor" : "gl_FragColor", valueBlock, "v_", DE_NULL);

	shader << "}\n";
	return shader.str();
}

// Specialize a shader for the vertex shader test case.
string ShaderCase::specializeVertexShader(const char* src, const ValueBlock& valueBlock)
{
	ostringstream decl;
	ostringstream setup;
	ostringstream output;
	const bool	usesInout = usesShaderInoutQualifiers(m_targetVersion);
	const char*   vtxIn		= usesInout ? "in" : "attribute";
	const char*   vtxOut	= usesInout ? "out" : "varying";

	// Output (write out position).
	output << "gl_Position = dEQP_Position;\n";

	// Declarations (position + attribute for each input, varying for each output).
	decl << vtxIn << " highp vec4 dEQP_Position;\n";
	for (int ndx = 0; ndx < (int)valueBlock.values.size(); ndx++)
	{
		const ShaderCase::Value& val		  = valueBlock.values[ndx];
		const char*				 valueName	= val.valueName.c_str();
		DataType				 floatType	= getDataTypeFloatScalars(val.dataType);
		const char*				 floatTypeStr = getDataTypeName(floatType);
		const char*				 refTypeStr   = getDataTypeName(val.dataType);

		if (val.storageType == ShaderCase::Value::STORAGE_INPUT)
		{
			if (getDataTypeScalarType(val.dataType) == TYPE_FLOAT)
			{
				decl << vtxIn << " " << floatTypeStr << " " << valueName << ";\n";
			}
			else
			{
				decl << vtxIn << " " << floatTypeStr << " a_" << valueName << ";\n";
				setup << refTypeStr << " " << valueName << " = " << refTypeStr << "(a_" << valueName << ");\n";
			}
		}
		else if (val.storageType == ShaderCase::Value::STORAGE_OUTPUT)
		{
			if (getDataTypeScalarType(val.dataType) == TYPE_FLOAT)
				decl << vtxOut << " " << floatTypeStr << " " << valueName << ";\n";
			else
			{
				decl << vtxOut << " " << floatTypeStr << " v_" << valueName << ";\n";
				decl << refTypeStr << " " << valueName << ";\n";

				output << "v_" << valueName << " = " << floatTypeStr << "(" << valueName << ");\n";
			}
		}
	}

	// Shader specialization.
	map<string, string> params;
	params.insert(pair<string, string>("DECLARATIONS", decl.str()));
	params.insert(pair<string, string>("SETUP", setup.str()));
	params.insert(pair<string, string>("OUTPUT", output.str()));
	params.insert(pair<string, string>("POSITION_FRAG_COLOR", "gl_Position"));

	StringTemplate tmpl(src);
	return tmpl.specialize(params);
}

// Specialize a shader for the fragment shader test case.
string ShaderCase::specializeFragmentShader(const char* src, const ValueBlock& valueBlock)
{
	ostringstream decl;
	ostringstream setup;
	ostringstream output;

	const bool  usesInout	  = usesShaderInoutQualifiers(m_targetVersion);
	const bool  customColorOut = usesInout;
	const char* fragIn		   = usesInout ? "in" : "varying";
	const char* fragColor	  = customColorOut ? "dEQP_FragColor" : "gl_FragColor";

	genCompareFunctions(decl, valueBlock, false);
	genCompareOp(output, fragColor, valueBlock, "", DE_NULL);

	if (customColorOut)
		decl << "layout(location = 0) out mediump vec4 dEQP_FragColor;\n";

	for (int ndx = 0; ndx < (int)valueBlock.values.size(); ndx++)
	{
		const ShaderCase::Value& val		  = valueBlock.values[ndx];
		const char*				 valueName	= val.valueName.c_str();
		DataType				 floatType	= getDataTypeFloatScalars(val.dataType);
		const char*				 floatTypeStr = getDataTypeName(floatType);
		const char*				 refTypeStr   = getDataTypeName(val.dataType);

		if (val.storageType == ShaderCase::Value::STORAGE_INPUT)
		{
			if (getDataTypeScalarType(val.dataType) == TYPE_FLOAT)
				decl << fragIn << " " << floatTypeStr << " " << valueName << ";\n";
			else
			{
				decl << fragIn << " " << floatTypeStr << " v_" << valueName << ";\n";
				std::string offset =
					isDataTypeIntOrIVec(val.dataType) ?
						" * 1.0025" :
						""; // \todo [petri] bit of a hack to avoid errors in chop() due to varying interpolation
				setup << refTypeStr << " " << valueName << " = " << refTypeStr << "(v_" << valueName << offset
					  << ");\n";
			}
		}
		else if (val.storageType == ShaderCase::Value::STORAGE_OUTPUT)
		{
			decl << "uniform " << refTypeStr << " ref_" << valueName << ";\n";
			decl << refTypeStr << " " << valueName << ";\n";
		}
	}

	/* \todo [2010-04-01 petri] Check all outputs. */

	// Shader specialization.
	map<string, string> params;
	params.insert(pair<string, string>("DECLARATIONS", decl.str()));
	params.insert(pair<string, string>("SETUP", setup.str()));
	params.insert(pair<string, string>("OUTPUT", output.str()));
	params.insert(pair<string, string>("POSITION_FRAG_COLOR", fragColor));

	StringTemplate tmpl(src);
	return tmpl.specialize(params);
}

void ShaderCase::specializeShaders(const char* vertexSource, const char* fragmentSource, string& outVertexSource,
								   string& outFragmentSource, const ValueBlock& valueBlock)
{
	const bool usesInout	  = usesShaderInoutQualifiers(m_targetVersion);
	const bool customColorOut = usesInout;

	// Vertex shader specialization.
	{
		ostringstream decl;
		ostringstream setup;
		const char*   vtxIn = usesInout ? "in" : "attribute";

		decl << vtxIn << " highp vec4 dEQP_Position;\n";

		for (int ndx = 0; ndx < (int)valueBlock.values.size(); ndx++)
		{
			const ShaderCase::Value& val	 = valueBlock.values[ndx];
			const char*				 typeStr = getDataTypeName(val.dataType);

			if (val.storageType == ShaderCase::Value::STORAGE_INPUT)
			{
				if (getDataTypeScalarType(val.dataType) == TYPE_FLOAT)
				{
					decl << vtxIn << " " << typeStr << " " << val.valueName << ";\n";
				}
				else
				{
					DataType	floatType	= getDataTypeFloatScalars(val.dataType);
					const char* floatTypeStr = getDataTypeName(floatType);

					decl << vtxIn << " " << floatTypeStr << " a_" << val.valueName << ";\n";
					setup << typeStr << " " << val.valueName << " = " << typeStr << "(a_" << val.valueName << ");\n";
				}
			}
			else if (val.storageType == ShaderCase::Value::STORAGE_UNIFORM && val.valueName.find('.') == string::npos)
			{
				decl << "uniform " << typeStr << " " << val.valueName << ";\n";
			}
		}

		map<string, string> params;
		params.insert(pair<string, string>("VERTEX_DECLARATIONS", decl.str()));
		params.insert(pair<string, string>("VERTEX_SETUP", setup.str()));
		params.insert(pair<string, string>("VERTEX_OUTPUT", string("gl_Position = dEQP_Position;\n")));
		StringTemplate tmpl(vertexSource);
		outVertexSource = tmpl.specialize(params);
	}

	// Fragment shader specialization.
	{
		ostringstream decl;
		ostringstream output;
		const char*   fragColor = customColorOut ? "dEQP_FragColor" : "gl_FragColor";

		genCompareFunctions(decl, valueBlock, false);
		genCompareOp(output, fragColor, valueBlock, "", DE_NULL);

		if (customColorOut)
			decl << "layout(location = 0) out mediump vec4 dEQP_FragColor;\n";

		for (int ndx = 0; ndx < (int)valueBlock.values.size(); ndx++)
		{
			const ShaderCase::Value& val		= valueBlock.values[ndx];
			const char*				 valueName  = val.valueName.c_str();
			const char*				 refTypeStr = getDataTypeName(val.dataType);

			if (val.storageType == ShaderCase::Value::STORAGE_OUTPUT)
			{
				decl << "uniform " << refTypeStr << " ref_" << valueName << ";\n";
				decl << refTypeStr << " " << valueName << ";\n";
			}
			else if (val.storageType == ShaderCase::Value::STORAGE_UNIFORM && val.valueName.find('.') == string::npos)
			{
				decl << "uniform " << refTypeStr << " " << valueName << ";\n";
			}
		}

		map<string, string> params;
		params.insert(pair<string, string>("FRAGMENT_DECLARATIONS", decl.str()));
		params.insert(pair<string, string>("FRAGMENT_OUTPUT", output.str()));
		params.insert(pair<string, string>("FRAG_COLOR", fragColor));
		StringTemplate tmpl(fragmentSource);
		outFragmentSource = tmpl.specialize(params);
	}
}

void ShaderCase::dumpValues(const ValueBlock& valueBlock, int arrayNdx)
{
	vector<vector<float> > attribValues;

	int numValues = (int)valueBlock.values.size();
	for (int valNdx = 0; valNdx < numValues; valNdx++)
	{
		const ShaderCase::Value& val		= valueBlock.values[valNdx];
		const char*				 valueName  = val.valueName.c_str();
		DataType				 dataType   = val.dataType;
		int						 scalarSize = getDataTypeScalarSize(val.dataType);
		ostringstream			 result;

		result << "    ";
		if (val.storageType == Value::STORAGE_INPUT)
			result << "input ";
		else if (val.storageType == Value::STORAGE_UNIFORM)
			result << "uniform ";
		else if (val.storageType == Value::STORAGE_OUTPUT)
			result << "expected ";

		result << getDataTypeName(dataType) << " " << valueName << ":";

		if (isDataTypeScalar(dataType))
			result << " ";
		if (isDataTypeVector(dataType))
			result << " [ ";
		else if (isDataTypeMatrix(dataType))
			result << "\n";

		if (isDataTypeScalarOrVector(dataType))
		{
			for (int scalarNdx = 0; scalarNdx < scalarSize; scalarNdx++)
			{
				int					  elemNdx = (val.arrayLength == 1) ? 0 : arrayNdx;
				const Value::Element& e		  = val.elements[elemNdx * scalarSize + scalarNdx];
				result << ((scalarNdx != 0) ? ", " : "");

				if (isDataTypeFloatOrVec(dataType))
					result << e.float32;
				else if (isDataTypeIntOrIVec(dataType))
					result << e.int32;
				else if (isDataTypeBoolOrBVec(dataType))
					result << (e.bool32 ? "true" : "false");
			}
		}
		else if (isDataTypeMatrix(dataType))
		{
			int numRows = getDataTypeMatrixNumRows(dataType);
			int numCols = getDataTypeMatrixNumColumns(dataType);
			for (int rowNdx = 0; rowNdx < numRows; rowNdx++)
			{
				result << "       [ ";
				for (int colNdx = 0; colNdx < numCols; colNdx++)
				{
					int   elemNdx = (val.arrayLength == 1) ? 0 : arrayNdx;
					float v		  = val.elements[elemNdx * scalarSize + rowNdx * numCols + colNdx].float32;
					result << ((colNdx == 0) ? "" : ", ") << v;
				}
				result << " ]\n";
			}
		}

		if (isDataTypeScalar(dataType))
			result << "\n";
		else if (isDataTypeVector(dataType))
			result << " ]\n";

		m_testCtx.getLog() << TestLog::Message << result.str() << TestLog::EndMessage;
	}
}

} // sl
} // deqp
