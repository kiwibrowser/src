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
 * \brief Basic Layout Binding Tests.
 *//*--------------------------------------------------------------------*/

#include "es31fLayoutBindingTests.hpp"

#include "gluShaderProgram.hpp"
#include "gluPixelTransfer.hpp"
#include "gluTextureUtil.hpp"
#include "gluContextInfo.hpp"

#include "glwFunctions.hpp"
#include "glwEnums.hpp"

#include "tcuSurface.hpp"
#include "tcuTestLog.hpp"
#include "tcuTexture.hpp"
#include "tcuTextureUtil.hpp"
#include "tcuImageCompare.hpp"
#include "tcuStringTemplate.hpp"
#include "tcuRenderTarget.hpp"

#include "deString.h"
#include "deStringUtil.hpp"
#include "deRandom.hpp"

using tcu::TestLog;
using tcu::Vec2;
using tcu::Vec3;
using tcu::Vec4;

namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace
{

enum TestType
{
	TESTTYPE_BINDING_SINGLE = 0,
	TESTTYPE_BINDING_MAX,
	TESTTYPE_BINDING_MULTIPLE,
	TESTTYPE_BINDING_ARRAY,
	TESTTYPE_BINDING_MAX_ARRAY,

	TESTTYPE_BINDING_LAST,
};

enum ShaderType
{
	SHADERTYPE_VERTEX = 0,
	SHADERTYPE_FRAGMENT,
	SHADERTYPE_TESS_CONTROL,
	SHADERTYPE_TESS_EVALUATION,
	SHADERTYPE_ALL,

	SHADERTYPE_LAST,
};

enum
{
	MAX_UNIFORM_MULTIPLE_INSTANCES	= 7,
	MAX_UNIFORM_ARRAY_SIZE			= 7,
};

std::string generateVertexShader (ShaderType shaderType, const std::string& shaderUniformDeclarations, const std::string& shaderBody)
{
	static const char* const s_simpleVertexShaderSource	=	"#version 310 es\n"
															"in highp vec4 a_position;\n"
															"void main (void)\n"
															"{\n"
															"	gl_Position = a_position;\n"
															"}\n";

	switch (shaderType)
	{
		case SHADERTYPE_VERTEX:
		case SHADERTYPE_ALL:
		{
			std::ostringstream vertexShaderSource;
			vertexShaderSource	<<	"#version 310 es\n"
								<<	"in highp vec4 a_position;\n"
								<<	"out highp vec4 v_color;\n"
								<<	"uniform highp int u_arrayNdx;\n\n"
								<<	shaderUniformDeclarations << "\n"
								<<	"void main (void)\n"
								<<	"{\n"
								<<	"	highp vec4 color;\n\n"
								<<	shaderBody << "\n"
								<<	"	v_color = color;\n"
								<<	"	gl_Position = a_position;\n"
								<<	"}\n";

			return vertexShaderSource.str();
		}

		case SHADERTYPE_FRAGMENT:
		case SHADERTYPE_TESS_CONTROL:
		case SHADERTYPE_TESS_EVALUATION:
			return s_simpleVertexShaderSource;

		default:
			DE_ASSERT(false);
			return "";
	}
}

std::string generateFragmentShader (ShaderType shaderType, const std::string& shaderUniformDeclarations, const std::string& shaderBody)
{
	static const char* const s_simpleFragmentShaderSource = "#version 310 es\n"
															"in highp vec4 v_color;\n"
															"layout(location = 0) out highp vec4 fragColor;\n"
															"void main (void)\n"
															"{\n"
															"	fragColor = v_color;\n"
															"}\n";

	switch (shaderType)
	{
		case SHADERTYPE_VERTEX:
		case SHADERTYPE_TESS_CONTROL:
		case SHADERTYPE_TESS_EVALUATION:
			return s_simpleFragmentShaderSource;

		case SHADERTYPE_FRAGMENT:
		{
			std::ostringstream fragmentShaderSource;
			fragmentShaderSource	<<	"#version 310 es\n"
									<<	"layout(location = 0) out highp vec4 fragColor;\n"
									<<	"uniform highp int u_arrayNdx;\n\n"
									<<	shaderUniformDeclarations << "\n"
									<<	"void main (void)\n"
									<<	"{\n"
									<<	"	highp vec4 color;\n\n"
									<<	shaderBody << "\n"
									<<	"	fragColor = color;\n"
									<<	"}\n";

			return fragmentShaderSource.str();
		}
		case SHADERTYPE_ALL:
		{
			std::ostringstream fragmentShaderSource;
			fragmentShaderSource	<<	"#version 310 es\n"
									<<	"in highp vec4 v_color;\n"
									<<	"layout(location = 0) out highp vec4 fragColor;\n"
									<<	"uniform highp int u_arrayNdx;\n\n"
									<<	shaderUniformDeclarations << "\n"
									<<	"void main (void)\n"
									<<	"{\n"
									<<	"	if (v_color.x > 2.0) discard;\n"
									<<	"	highp vec4 color;\n\n"
									<<	shaderBody << "\n"
									<<	"	fragColor = color;\n"
									<<	"}\n";

			return fragmentShaderSource.str();
		}

		default:
			DE_ASSERT(false);
			return "";
	}
}

std::string generateTessControlShader (ShaderType shaderType, const std::string& shaderUniformDeclarations, const std::string& shaderBody)
{
	static const char* const s_simpleTessContorlShaderSource =	"#version 310 es\n"
																"#extension GL_EXT_tessellation_shader : require\n"
																"layout (vertices=3) out;\n"
																"\n"
																"void main (void)\n"
																"{\n"
																"	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
																"}\n";

	switch (shaderType)
	{
		case SHADERTYPE_VERTEX:
		case SHADERTYPE_FRAGMENT:
		case SHADERTYPE_TESS_EVALUATION:
			return s_simpleTessContorlShaderSource;

		case SHADERTYPE_TESS_CONTROL:
		case SHADERTYPE_ALL:
		{
			std::ostringstream tessControlShaderSource;
			tessControlShaderSource <<	"#version 310 es\n"
									<<	"#extension GL_EXT_tessellation_shader : require\n"
									<<	"layout (vertices=3) out;\n"
									<<	"\n"
									<<	"uniform highp int u_arrayNdx;\n\n"
									<<	shaderUniformDeclarations << "\n"
									<<	"void main (void)\n"
									<<	"{\n"
									<<	"	highp vec4 color;\n\n"
									<<	shaderBody << "\n"
									<<	"	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
									<<	"}\n";

			return tessControlShaderSource.str();
		}

		default:
			DE_ASSERT(false);
			return "";
	}
}

std::string generateTessEvaluationShader (ShaderType shaderType, const std::string& shaderUniformDeclarations, const std::string& shaderBody)
{
	static const char* const s_simpleTessEvaluationShaderSource =	"#version 310 es\n"
																	"#extension GL_EXT_tessellation_shader : require\n"
																	"layout (triangles) in;\n"
																	"\n"
																	"void main (void)\n"
																	"{\n"
																	"	gl_Position = gl_TessCoord[0] * gl_in[0].gl_Position + gl_TessCoord[1] * gl_in[1].gl_Position + gl_TessCoord[2] * gl_in[2].gl_Position;\n"
																	"}\n";

	switch (shaderType)
	{
		case SHADERTYPE_VERTEX:
		case SHADERTYPE_FRAGMENT:
		case SHADERTYPE_TESS_CONTROL:
			return s_simpleTessEvaluationShaderSource;

		case SHADERTYPE_TESS_EVALUATION:
		case SHADERTYPE_ALL:
		{
			std::ostringstream tessEvaluationShaderSource;
			tessEvaluationShaderSource	<< "#version 310 es\n"
										<< "#extension GL_EXT_tessellation_shader : require\n"
										<< "layout (triangles) in;\n"
										<< "\n"
										<< "uniform highp int u_arrayNdx;\n\n"
										<< shaderUniformDeclarations << "\n"
										<< "out mediump vec4 v_color;\n"
										<< "void main (void)\n"
										<< "{\n"
										<< "	highp vec4 color;\n\n"
										<<	shaderBody << "\n"
										<< "	v_color = color;\n"
										<< "	gl_Position = gl_TessCoord[0] * gl_in[0].gl_Position + gl_TessCoord[1] * gl_in[1].gl_Position + gl_TessCoord[2] * gl_in[2].gl_Position;\n"
										<< "}\n";

			return tessEvaluationShaderSource.str();
		}

		default:
			DE_ASSERT(false);
			return "";
	}
}

std::string getUniformName (const std::string& name, int declNdx)
{
	return name + de::toString(declNdx);
}

std::string getUniformName (const std::string& name, int declNdx, int arrNdx)
{
	return name + de::toString(declNdx) + "[" + de::toString(arrNdx) + "]";
}

Vec4 getRandomColor (de::Random& rnd)
{
	const float r = rnd.getFloat(0.2f, 0.9f);
	const float g = rnd.getFloat(0.2f, 0.9f);
	const float b = rnd.getFloat(0.2f, 0.9f);
	return Vec4(r, g, b, 1.0f);
}

class LayoutBindingRenderCase : public TestCase
{
public:
	enum
	{
		MAX_TEST_RENDER_WIDTH	= 256,
		MAX_TEST_RENDER_HEIGHT	= 256,
		TEST_TEXTURE_SIZE	= 1,
	};

										LayoutBindingRenderCase			(Context&			context,
																		 const char*		name,
																		 const char*		desc,
																		 ShaderType			shaderType,
																		 TestType			testType,
																		 glw::GLenum		maxBindingPointEnum,
																		 glw::GLenum		maxVertexUnitsEnum,
																		 glw::GLenum		maxFragmentUnitsEnum,
																		 glw::GLenum		maxCombinedUnitsEnum,
																		 const std::string& uniformName);
	virtual								~LayoutBindingRenderCase		(void);

	virtual void						init							(void);
	virtual void						deinit							(void);

	int									getRenderWidth					(void) const { return de::min((int)MAX_TEST_RENDER_WIDTH, m_context.getRenderTarget().getWidth()); }
	int									getRenderHeight					(void) const { return de::min((int)MAX_TEST_RENDER_HEIGHT, m_context.getRenderTarget().getHeight()); }
protected:
	virtual glu::ShaderProgram*			generateShaders					(void) const = 0;

	void								initRenderState					(void);
	bool								drawAndVerifyResult				(const Vec4& expectedColor);
	void								setTestResult					(bool queryTestPassed, bool imageTestPassed);

	const glu::ShaderProgram*			m_program;
	const ShaderType					m_shaderType;
	const TestType						m_testType;
	const std::string					m_uniformName;

	const glw::GLenum					m_maxBindingPointEnum;
	const glw::GLenum					m_maxVertexUnitsEnum;
	const glw::GLenum					m_maxFragmentUnitsEnum;
	const glw::GLenum					m_maxCombinedUnitsEnum;

	glw::GLuint							m_vertexBuffer;
	glw::GLuint							m_indexBuffer;
	glw::GLint							m_shaderProgramLoc;
	glw::GLint							m_shaderProgramPosLoc;
	glw::GLint							m_shaderProgramArrayNdxLoc;
	glw::GLint							m_numBindings;

	std::vector<glw::GLint>				m_bindings;

private:
	void								initBindingPoints				(int minBindingPoint, int numBindingPoints);
};

LayoutBindingRenderCase::LayoutBindingRenderCase (Context&				context,
												  const char*			name,
												  const char*			desc,
												  ShaderType			shaderType,
												  TestType				testType,
												  glw::GLenum			maxBindingPointEnum,
												  glw::GLenum			maxVertexUnitsEnum,
												  glw::GLenum			maxFragmentUnitsEnum,
												  glw::GLenum			maxCombinedUnitsEnum,
												  const std::string&	uniformName)
	: TestCase						(context, name, desc)
	, m_program						(DE_NULL)
	, m_shaderType					(shaderType)
	, m_testType					(testType)
	, m_uniformName					(uniformName)
	, m_maxBindingPointEnum			(maxBindingPointEnum)
	, m_maxVertexUnitsEnum			(maxVertexUnitsEnum)
	, m_maxFragmentUnitsEnum		(maxFragmentUnitsEnum)
	, m_maxCombinedUnitsEnum		(maxCombinedUnitsEnum)
	, m_vertexBuffer				(0)
	, m_indexBuffer					(0)
	, m_shaderProgramLoc			(0)
	, m_shaderProgramPosLoc			(0)
	, m_shaderProgramArrayNdxLoc	(0)
	, m_numBindings					(0)
{
}

LayoutBindingRenderCase::~LayoutBindingRenderCase (void)
{
	deinit();
}

void LayoutBindingRenderCase::init (void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	{
		de::Random				rnd					(deStringHash(getName()) ^ 0xff23a4);
		glw::GLint				numBindingPoints	= 0;	// Number of available binding points
		glw::GLint				maxVertexUnits		= 0;	// Available uniforms in the vertex shader
		glw::GLint				maxFragmentUnits	= 0;	// Available uniforms in the fragment shader
		glw::GLint				maxCombinedUnits	= 0;	// Available uniforms in all the shader stages combined
		glw::GLint				maxUnits			= 0;	// Maximum available uniforms for this test

		gl.getIntegerv(m_maxVertexUnitsEnum, &maxVertexUnits);
		gl.getIntegerv(m_maxFragmentUnitsEnum, &maxFragmentUnits);
		gl.getIntegerv(m_maxCombinedUnitsEnum, &maxCombinedUnits);
		gl.getIntegerv(m_maxBindingPointEnum, &numBindingPoints);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Querying available uniform numbers failed");

		m_testCtx.getLog() << tcu::TestLog::Message << "Maximum units for uniform type in the vertex shader: " << maxVertexUnits << tcu::TestLog::EndMessage;
		m_testCtx.getLog() << tcu::TestLog::Message << "Maximum units for uniform type in the fragment shader: " << maxFragmentUnits << tcu::TestLog::EndMessage;
		m_testCtx.getLog() << tcu::TestLog::Message << "Maximum combined units for uniform type: " << maxCombinedUnits << tcu::TestLog::EndMessage;
		m_testCtx.getLog() << tcu::TestLog::Message << "Maximum binding point for uniform type: " << numBindingPoints-1 << tcu::TestLog::EndMessage;

		// Select maximum number of uniforms used for the test
		switch (m_shaderType)
		{
			case SHADERTYPE_VERTEX:
				maxUnits = maxVertexUnits;
				break;

			case SHADERTYPE_FRAGMENT:
				maxUnits = maxFragmentUnits;
				break;

			case SHADERTYPE_ALL:
				maxUnits = maxCombinedUnits/2;
				break;

			default:
				DE_ASSERT(false);
		}

		// Select the number of uniforms (= bindings) used for this test
		switch (m_testType)
		{
			case TESTTYPE_BINDING_SINGLE:
			case TESTTYPE_BINDING_MAX:
				m_numBindings = 1;
				break;

			case TESTTYPE_BINDING_MULTIPLE:
				if (maxUnits < 2)
					throw tcu::NotSupportedError("Not enough uniforms available for test");
				m_numBindings = rnd.getInt(2, deMin32(MAX_UNIFORM_MULTIPLE_INSTANCES, maxUnits));
				break;

			case TESTTYPE_BINDING_ARRAY:
			case TESTTYPE_BINDING_MAX_ARRAY:
				if (maxUnits < 2)
					throw tcu::NotSupportedError("Not enough uniforms available for test");
				m_numBindings = rnd.getInt(2, deMin32(MAX_UNIFORM_ARRAY_SIZE, maxUnits));
				break;

			default:
				DE_ASSERT(false);
		}

		// Check that we have enough uniforms in different shaders to perform the tests
		if ( ((m_shaderType == SHADERTYPE_VERTEX) || (m_shaderType == SHADERTYPE_ALL)) && (maxVertexUnits < m_numBindings) )
			throw tcu::NotSupportedError("Vertex shader: not enough uniforms available for test");
		if ( ((m_shaderType == SHADERTYPE_FRAGMENT) || (m_shaderType == SHADERTYPE_ALL)) && (maxFragmentUnits < m_numBindings) )
			throw tcu::NotSupportedError("Fragment shader: not enough uniforms available for test");
		if ( (m_shaderType == SHADERTYPE_ALL) && (maxCombinedUnits < m_numBindings*2) )
			throw tcu::NotSupportedError("Not enough uniforms available for test");

		// Check that we have enough binding points to perform the tests
		if (numBindingPoints < m_numBindings)
			throw tcu::NotSupportedError("Not enough binding points available for test");

		// Initialize the binding points i.e. populate the two binding point vectors
		initBindingPoints(0, numBindingPoints);
	}

	// Generate the shader program - note: this must be done after deciding the binding points
	DE_ASSERT(!m_program);
	m_testCtx.getLog() << tcu::TestLog::Message << "Creating test shaders" << tcu::TestLog::EndMessage;
	m_program = generateShaders();
	m_testCtx.getLog() << *m_program;

	if (!m_program->isOk())
		throw tcu::TestError("Shader compile failed");

	// Setup vertex and index buffers
	{
		// Get attribute and uniform locations
		const deUint32	program	= m_program->getProgram();

		m_shaderProgramPosLoc		= gl.getAttribLocation(program, "a_position");
		m_shaderProgramArrayNdxLoc	= gl.getUniformLocation(program, "u_arrayNdx");
		m_vertexBuffer				= 0;
		m_indexBuffer				= 0;

		// Setup buffers so that we render one quad covering the whole viewport
		const Vec3 vertices[] =
		{
			Vec3(-1.0f, -1.0f, +1.0f),
			Vec3(+1.0f, -1.0f, +1.0f),
			Vec3(+1.0f, +1.0f, +1.0f),
			Vec3(-1.0f, +1.0f, +1.0f),
		};

		const deUint16 indices[] =
		{
			0, 1, 2,
			0, 2, 3,
		};

		TCU_CHECK((m_shaderProgramPosLoc >= 0) && (m_shaderProgramArrayNdxLoc >= 0));

		// Generate and bind index buffer
		gl.genBuffers(1, &m_indexBuffer);
		gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
		gl.bufferData(GL_ELEMENT_ARRAY_BUFFER, (DE_LENGTH_OF_ARRAY(indices)*(glw::GLsizeiptr)sizeof(indices[0])), &indices[0], GL_STATIC_DRAW);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Index buffer setup failed");

		// Generate and bind vertex buffer
		gl.genBuffers(1, &m_vertexBuffer);
		gl.bindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
		gl.bufferData(GL_ARRAY_BUFFER, (DE_LENGTH_OF_ARRAY(vertices)*(glw::GLsizeiptr)sizeof(vertices[0])), &vertices[0], GL_STATIC_DRAW);
		gl.enableVertexAttribArray(m_shaderProgramPosLoc);
		gl.vertexAttribPointer(m_shaderProgramPosLoc, 3, GL_FLOAT, GL_FALSE, 0, DE_NULL);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Vertex buffer setup failed");
	}
}

void LayoutBindingRenderCase::deinit (void)
{
	if (m_program)
	{
		delete m_program;
		m_program = DE_NULL;
	}

	if (m_shaderProgramPosLoc)
		m_context.getRenderContext().getFunctions().disableVertexAttribArray(m_shaderProgramPosLoc);

	if (m_vertexBuffer)
	{
		m_context.getRenderContext().getFunctions().deleteBuffers(1, &m_vertexBuffer);
		m_context.getRenderContext().getFunctions().bindBuffer(GL_ARRAY_BUFFER, 0);
	}

	if (m_indexBuffer)
	{
		m_context.getRenderContext().getFunctions().deleteBuffers(1, &m_indexBuffer);
		m_context.getRenderContext().getFunctions().bindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
}

void LayoutBindingRenderCase::initBindingPoints (int minBindingPoint, int numBindingPoints)
{
	de::Random rnd(deStringHash(getName()) ^ 0xff23a4);

	switch (m_testType)
	{
		case TESTTYPE_BINDING_SINGLE:
		{
			const int bpoint = rnd.getInt(minBindingPoint, numBindingPoints-1);
			m_bindings.push_back(bpoint);
			break;
		}

		case TESTTYPE_BINDING_MAX:
			m_bindings.push_back(numBindingPoints-1);
			break;

		case TESTTYPE_BINDING_MULTIPLE:
		{
			// Choose multiple unique binding points from the low and high end of available binding points
			std::vector<deUint32> lowBindingPoints;
			std::vector<deUint32> highBindingPoints;

			for (int bpoint = 0; bpoint < numBindingPoints/2; ++bpoint)
				lowBindingPoints.push_back(bpoint);
			for (int bpoint = numBindingPoints/2; bpoint < numBindingPoints; ++bpoint)
				highBindingPoints.push_back(bpoint);

			rnd.shuffle(lowBindingPoints.begin(), lowBindingPoints.end());
			rnd.shuffle(highBindingPoints.begin(), highBindingPoints.end());

			for (int ndx = 0; ndx < m_numBindings; ++ndx)
			{
				if (ndx%2 == 0)
				{
					const int bpoint = lowBindingPoints.back();
					lowBindingPoints.pop_back();
					m_bindings.push_back(bpoint);
				}
				else
				{
					const int bpoint = highBindingPoints.back();
					highBindingPoints.pop_back();
					m_bindings.push_back(bpoint);
				}

			}
			break;
		}

		case TESTTYPE_BINDING_ARRAY:
		{
			const glw::GLint binding = rnd.getInt(minBindingPoint, numBindingPoints-m_numBindings);
			for (int ndx = 0; ndx < m_numBindings; ++ndx)
				m_bindings.push_back(binding+ndx);
			break;
		}

		case TESTTYPE_BINDING_MAX_ARRAY:
		{
			const glw::GLint binding = numBindingPoints-m_numBindings;
			for (int ndx = 0; ndx < m_numBindings; ++ndx)
				m_bindings.push_back(binding+ndx);
			break;
		}

		default:
			DE_ASSERT(false);
	}
}

void LayoutBindingRenderCase::initRenderState (void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.useProgram(m_program->getProgram());
	gl.viewport(0, 0, getRenderWidth(), getRenderHeight());
	gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to set render state");
}

bool LayoutBindingRenderCase::drawAndVerifyResult (const Vec4& expectedColor)
{
	const glw::Functions&	gl					= m_context.getRenderContext().getFunctions();
	tcu::Surface			reference			(getRenderWidth(), getRenderHeight());

	// the point of these test is to check layout_binding. For this purpose, we can use quite
	// large thresholds.
	const tcu::RGBA			surfaceThreshold	= m_context.getRenderContext().getRenderTarget().getPixelFormat().getColorThreshold();
	const tcu::RGBA			compareThreshold	= tcu::RGBA(de::clamp(2 * surfaceThreshold.getRed(),   0, 255),
															de::clamp(2 * surfaceThreshold.getGreen(), 0, 255),
															de::clamp(2 * surfaceThreshold.getBlue(),  0, 255),
															de::clamp(2 * surfaceThreshold.getAlpha(), 0, 255));

	gl.clear(GL_COLOR_BUFFER_BIT);

	// Draw
	gl.drawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, DE_NULL);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Drawing failed");

	// Verify
	tcu::Surface result(getRenderWidth(), getRenderHeight());
	m_testCtx.getLog() << TestLog::Message << "Reading pixels" << TestLog::EndMessage;
	glu::readPixels(m_context.getRenderContext(), 0, 0, result.getAccess());
	GLU_EXPECT_NO_ERROR(gl.getError(), "Read pixels failed");

	tcu::clear(reference.getAccess(), expectedColor);
	m_testCtx.getLog() << tcu::TestLog::Message << "Verifying output image, fragment output color is " << expectedColor << tcu::TestLog::EndMessage;

	return tcu::pixelThresholdCompare(m_testCtx.getLog(), "Render result", "Result verification", reference, result, compareThreshold, tcu::COMPARE_LOG_RESULT);
}

void LayoutBindingRenderCase::setTestResult (bool queryTestPassed, bool imageTestPassed)
{
	if (queryTestPassed && imageTestPassed)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else if (!queryTestPassed && !imageTestPassed)
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "One or more binding point queries and image comparisons failed");
	else if (!queryTestPassed)
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "One or more binding point queries failed");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "One or more image comparisons failed");
}

class LayoutBindingNegativeCase : public TestCase
{
public:
	enum ErrorType
	{
		ERRORTYPE_OVER_MAX_UNITS = 0,
		ERRORTYPE_LESS_THAN_ZERO,
		ERRORTYPE_CONTRADICTORY,

		ERRORTYPE_LAST,
	};

										LayoutBindingNegativeCase		(Context&			context,
																		 const char*		name,
																		 const char*		desc,
																		 ShaderType			shaderType,
																		 TestType			testType,
																		 ErrorType			errorType,
																		 glw::GLenum		maxBindingPointEnum,
																		 glw::GLenum		maxVertexUnitsEnum,
																		 glw::GLenum		maxFragmentUnitsEnum,
																		 glw::GLenum		maxTessCtrlUnitsEnum,
																		 glw::GLenum		maxTessEvalUnitsEnum,
																		 glw::GLenum		maxCombinedUnitsEnum,
																		 const std::string& uniformName);
	virtual								~LayoutBindingNegativeCase		(void);

	virtual void						init							(void);
	virtual void						deinit							(void);
	virtual IterateResult				iterate							(void);

protected:
	virtual glu::ShaderProgram*			generateShaders					(void) const = 0;

	const glu::ShaderProgram*			m_program;
	const ShaderType					m_shaderType;
	const TestType						m_testType;
	const ErrorType						m_errorType;
	const glw::GLenum					m_maxBindingPointEnum;
	const glw::GLenum					m_maxVertexUnitsEnum;
	const glw::GLenum					m_maxFragmentUnitsEnum;
	const glw::GLenum					m_maxTessCtrlUnitsEnum;
	const glw::GLenum					m_maxTessEvalUnitsEnum;
	const glw::GLenum					m_maxCombinedUnitsEnum;
	const std::string					m_uniformName;
	glw::GLint							m_numBindings;
	std::vector<glw::GLint>				m_vertexShaderBinding;
	std::vector<glw::GLint>				m_fragmentShaderBinding;
	std::vector<glw::GLint>				m_tessCtrlShaderBinding;
	std::vector<glw::GLint>				m_tessEvalShaderBinding;
	bool								m_tessSupport;

private:
	void								initBindingPoints				(int minBindingPoint, int numBindingPoints);
};

LayoutBindingNegativeCase::LayoutBindingNegativeCase (Context&				context,
													  const char*			name,
													  const char*			desc,
													  ShaderType			shaderType,
													  TestType				testType,
													  ErrorType				errorType,
													  glw::GLenum			maxBindingPointEnum,
													  glw::GLenum			maxVertexUnitsEnum,
													  glw::GLenum			maxTessCtrlUnitsEnum,
													  glw::GLenum			maxTessEvalUnitsEnum,
													  glw::GLenum			maxFragmentUnitsEnum,
													  glw::GLenum			maxCombinedUnitsEnum,
													  const std::string&	uniformName)
	: TestCase					(context, name, desc)
	, m_program					(DE_NULL)
	, m_shaderType				(shaderType)
	, m_testType				(testType)
	, m_errorType				(errorType)
	, m_maxBindingPointEnum		(maxBindingPointEnum)
	, m_maxVertexUnitsEnum		(maxVertexUnitsEnum)
	, m_maxFragmentUnitsEnum	(maxFragmentUnitsEnum)
	, m_maxTessCtrlUnitsEnum	(maxTessCtrlUnitsEnum)
	, m_maxTessEvalUnitsEnum	(maxTessEvalUnitsEnum)
	, m_maxCombinedUnitsEnum	(maxCombinedUnitsEnum)
	, m_uniformName				(uniformName)
	, m_numBindings				(0)
	, m_tessSupport				(false)
{
}

LayoutBindingNegativeCase::~LayoutBindingNegativeCase (void)
{
	deinit();
}

void LayoutBindingNegativeCase::init (void)
{
	// Decide appropriate binding points for the vertex and fragment shaders
	const glw::Functions&	gl					= m_context.getRenderContext().getFunctions();
	de::Random				rnd					(deStringHash(getName()) ^ 0xff23a4);
	glw::GLint				numBindingPoints	= 0;	// Number of binding points
	glw::GLint				maxVertexUnits		= 0;	// Available uniforms in the vertex shader
	glw::GLint				maxFragmentUnits	= 0;	// Available uniforms in the fragment shader
	glw::GLint				maxCombinedUnits	= 0;	// Available uniforms in all the shader stages combined
	glw::GLint				maxTessCtrlUnits	= 0;	// Available uniforms in tessellation control shader
	glw::GLint				maxTessEvalUnits	= 0;	// Available uniforms in tessellation evaluation shader
	glw::GLint				maxUnits			= 0;	// Maximum available uniforms for this test

	m_tessSupport = m_context.getContextInfo().isExtensionSupported("GL_EXT_tessellation_shader")
					|| contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));

	if (!m_tessSupport && (m_shaderType == SHADERTYPE_TESS_EVALUATION || m_shaderType == SHADERTYPE_TESS_CONTROL))
		TCU_THROW(NotSupportedError, "Tesselation shaders not supported");

	int numShaderStages = m_tessSupport ? 4 : 2;

	gl.getIntegerv(m_maxVertexUnitsEnum, &maxVertexUnits);
	gl.getIntegerv(m_maxFragmentUnitsEnum, &maxFragmentUnits);

	if (m_tessSupport)
	{
		gl.getIntegerv(m_maxTessCtrlUnitsEnum, &maxTessCtrlUnits);
		gl.getIntegerv(m_maxTessEvalUnitsEnum, &maxTessEvalUnits);
	}

	gl.getIntegerv(m_maxCombinedUnitsEnum, &maxCombinedUnits);
	gl.getIntegerv(m_maxBindingPointEnum, &numBindingPoints);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Querying available uniform numbers failed");

	m_testCtx.getLog() << tcu::TestLog::Message << "Maximum units for uniform type in the vertex shader: " << maxVertexUnits << tcu::TestLog::EndMessage;
	m_testCtx.getLog() << tcu::TestLog::Message << "Maximum units for uniform type in the fragment shader: " << maxFragmentUnits << tcu::TestLog::EndMessage;

	if (m_tessSupport)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Maximum units for uniform type in the tessellation control shader: " << maxTessCtrlUnits << tcu::TestLog::EndMessage;
		m_testCtx.getLog() << tcu::TestLog::Message << "Maximum units for uniform type in the tessellation evaluation shader: " << maxTessCtrlUnits << tcu::TestLog::EndMessage;
	}

	m_testCtx.getLog() << tcu::TestLog::Message << "Maximum combined units for uniform type: " << maxCombinedUnits << tcu::TestLog::EndMessage;
	m_testCtx.getLog() << tcu::TestLog::Message << "Maximum binding point for uniform type: " << numBindingPoints-1 << tcu::TestLog::EndMessage;

	// Select maximum number of uniforms used for the test
	switch (m_shaderType)
	{
		case SHADERTYPE_VERTEX:
			maxUnits = maxVertexUnits;
			break;

		case SHADERTYPE_FRAGMENT:
			maxUnits = maxFragmentUnits;
			break;

		case SHADERTYPE_ALL:
			maxUnits = de::min(de::min(de::min(maxVertexUnits, maxFragmentUnits), de::min(maxTessCtrlUnits, maxTessEvalUnits)), maxCombinedUnits/numShaderStages);
			break;

		case SHADERTYPE_TESS_CONTROL:
			maxUnits = maxTessCtrlUnits;
			break;

		case SHADERTYPE_TESS_EVALUATION:
			maxUnits = maxTessEvalUnits;
			break;

		default:
			DE_ASSERT(false);
	}

	// Select the number of uniforms (= bindings) used for this test
	switch (m_testType)
	{
		case TESTTYPE_BINDING_SINGLE:
		case TESTTYPE_BINDING_MAX:
			m_numBindings = 1;
			break;

		case TESTTYPE_BINDING_MULTIPLE:
		case TESTTYPE_BINDING_ARRAY:
		case TESTTYPE_BINDING_MAX_ARRAY:
			if (m_errorType == ERRORTYPE_CONTRADICTORY)
			{
				// leave room for contradictory case
				if (maxUnits < 3)
					TCU_THROW(NotSupportedError, "Not enough uniforms available for test");
				m_numBindings = rnd.getInt(2, deMin32(MAX_UNIFORM_ARRAY_SIZE, maxUnits-1));
			}
			else
			{
				if (maxUnits < 2)
					TCU_THROW(NotSupportedError, "Not enough uniforms available for test");
				m_numBindings = rnd.getInt(2, deMin32(MAX_UNIFORM_ARRAY_SIZE, maxUnits));
			}
			break;

		default:
			DE_ASSERT(false);
	}

	// Check that we have enough uniforms in different shaders to perform the tests
	if (((m_shaderType == SHADERTYPE_VERTEX) || (m_shaderType == SHADERTYPE_ALL)) && (maxVertexUnits < m_numBindings) )
		TCU_THROW(NotSupportedError, "Vertex shader: not enough uniforms available for test");

	if (((m_shaderType == SHADERTYPE_FRAGMENT) || (m_shaderType == SHADERTYPE_ALL)) && (maxFragmentUnits < m_numBindings) )
		TCU_THROW(NotSupportedError, "Fragment shader: not enough uniforms available for test");

	if (m_tessSupport && ((m_shaderType == SHADERTYPE_TESS_CONTROL) || (m_shaderType == SHADERTYPE_ALL)) && (maxTessCtrlUnits < m_numBindings) )
		TCU_THROW(NotSupportedError, "Tessellation control shader: not enough uniforms available for test");

	if (m_tessSupport && ((m_shaderType == SHADERTYPE_TESS_EVALUATION) || (m_shaderType == SHADERTYPE_ALL)) && (maxTessEvalUnits < m_numBindings) )
		TCU_THROW(NotSupportedError, "Tessellation evaluation shader: not enough uniforms available for test");

	if ((m_shaderType == SHADERTYPE_ALL) && (maxCombinedUnits < m_numBindings*numShaderStages) )
		TCU_THROW(NotSupportedError, "Not enough uniforms available for test");

	// Check that we have enough binding points to perform the tests
	if (numBindingPoints < m_numBindings)
		TCU_THROW(NotSupportedError, "Not enough binding points available for test");

	if (m_errorType == ERRORTYPE_CONTRADICTORY && numBindingPoints == m_numBindings)
		TCU_THROW(NotSupportedError, "Not enough binding points available for test");

	// Initialize the binding points i.e. populate the two binding point vectors
	initBindingPoints(0, numBindingPoints);

	// Generate the shader program - note: this must be done after deciding the binding points
	DE_ASSERT(!m_program);
	m_testCtx.getLog() << tcu::TestLog::Message << "Creating test shaders" << tcu::TestLog::EndMessage;
	m_program = generateShaders();
	m_testCtx.getLog() << *m_program;
}

void LayoutBindingNegativeCase::deinit (void)
{
	if (m_program)
	{
		delete m_program;
		m_program = DE_NULL;
	}
}

TestCase::IterateResult LayoutBindingNegativeCase::iterate (void)
{
	bool pass = false;
	std::string failMessage;

	switch (m_errorType)
	{
		case ERRORTYPE_CONTRADICTORY:		// Contradictory binding points should cause a link-time error
			if (!(m_program->getProgramInfo()).linkOk)
				pass = true;
			failMessage = "Test failed - expected a link-time error";
			break;

		case ERRORTYPE_LESS_THAN_ZERO:		// Out of bounds binding points should cause a compile-time error
		case ERRORTYPE_OVER_MAX_UNITS:
			if (m_tessSupport)
			{
				if (!(m_program->getShaderInfo(glu::SHADERTYPE_VERTEX)).compileOk
					|| !(m_program->getShaderInfo(glu::SHADERTYPE_FRAGMENT).compileOk)
					|| !(m_program->getShaderInfo(glu::SHADERTYPE_TESSELLATION_CONTROL).compileOk)
					|| !(m_program->getShaderInfo(glu::SHADERTYPE_TESSELLATION_EVALUATION)).compileOk)
					pass = true;
			}
			else
			{
				if (!(m_program->getShaderInfo(glu::SHADERTYPE_VERTEX)).compileOk
					|| !(m_program->getShaderInfo(glu::SHADERTYPE_FRAGMENT).compileOk))
					pass = true;
			}

			failMessage = "Test failed - expected a compile-time error";
			break;

		default:
			DE_ASSERT(false);
	}

	if (pass)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, failMessage.c_str());

	return STOP;
}

void LayoutBindingNegativeCase::initBindingPoints (int minBindingPoint, int numBindingPoints)
{
	de::Random rnd(deStringHash(getName()) ^ 0xff23a4);

	switch (m_errorType)
	{
		case ERRORTYPE_OVER_MAX_UNITS:	// Select a binding point that is 1 over the maximum
		{
			m_vertexShaderBinding.push_back(numBindingPoints+1-m_numBindings);
			m_fragmentShaderBinding.push_back(numBindingPoints+1-m_numBindings);
			m_tessCtrlShaderBinding.push_back(numBindingPoints+1-m_numBindings);
			m_tessEvalShaderBinding.push_back(numBindingPoints+1-m_numBindings);
			break;
		}

		case ERRORTYPE_LESS_THAN_ZERO:	// Select a random negative binding point
		{
			const glw::GLint binding = -rnd.getInt(1, m_numBindings);
			m_vertexShaderBinding.push_back(binding);
			m_fragmentShaderBinding.push_back(binding);
			m_tessCtrlShaderBinding.push_back(binding);
			m_tessEvalShaderBinding.push_back(binding);
			break;
		}

		case ERRORTYPE_CONTRADICTORY:	// Select two valid, but contradictory binding points
		{
			m_vertexShaderBinding.push_back(minBindingPoint);
			m_fragmentShaderBinding.push_back((minBindingPoint+1)%numBindingPoints);
			m_tessCtrlShaderBinding.push_back((minBindingPoint+2)%numBindingPoints);
			m_tessEvalShaderBinding.push_back((minBindingPoint+3)%numBindingPoints);

			DE_ASSERT(m_vertexShaderBinding.back()		!= m_fragmentShaderBinding.back());
			DE_ASSERT(m_fragmentShaderBinding.back()	!= m_tessEvalShaderBinding.back());
			DE_ASSERT(m_tessEvalShaderBinding.back()	!= m_tessCtrlShaderBinding.back());
			DE_ASSERT(m_tessCtrlShaderBinding.back()	!= m_vertexShaderBinding.back());
			DE_ASSERT(m_vertexShaderBinding.back()		!= m_tessEvalShaderBinding.back());
			DE_ASSERT(m_tessCtrlShaderBinding.back()	!= m_fragmentShaderBinding.back());
			break;
		}

		default:
			DE_ASSERT(false);
	}

	// In case we are testing with multiple uniforms populate the rest of the binding points
	for (int ndx = 1; ndx < m_numBindings; ++ndx)
	{
		m_vertexShaderBinding.push_back(m_vertexShaderBinding.front()+ndx);
		m_fragmentShaderBinding.push_back(m_fragmentShaderBinding.front()+ndx);
		m_tessCtrlShaderBinding.push_back(m_tessCtrlShaderBinding.front()+ndx);
		m_tessEvalShaderBinding.push_back(m_tessCtrlShaderBinding.front()+ndx);
	}
}

class SamplerBindingRenderCase : public LayoutBindingRenderCase
{
public:
									SamplerBindingRenderCase		(Context& context, const char* name, const char* desc, ShaderType shaderType, TestType testType, glw::GLenum samplerType, glw::GLenum textureType);
									~SamplerBindingRenderCase		(void);

	void							init							(void);
	void							deinit							(void);
	IterateResult					iterate							(void);

private:
	glu::ShaderProgram*				generateShaders					(void) const;
	glu::DataType					getSamplerTexCoordType			(void) const;
	void							initializeTexture				(glw::GLint bindingPoint, glw::GLint textureName, const Vec4& color) const;

	const glw::GLenum				m_samplerType;
	const glw::GLenum				m_textureType;

	std::vector<glw::GLuint>		m_textures;
	std::vector<Vec4>				m_textureColors;
};


SamplerBindingRenderCase::SamplerBindingRenderCase (Context&		context,
													const char*		name,
													const char*		desc,
													ShaderType		shaderType,
													TestType		testType,
													glw::GLenum		samplerType,
													glw::GLenum		textureType)
	: LayoutBindingRenderCase	(context, name, desc, shaderType, testType, GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, GL_MAX_TEXTURE_IMAGE_UNITS, GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, "u_sampler")
	, m_samplerType				(samplerType)
	, m_textureType				(textureType)
{
}

SamplerBindingRenderCase::~SamplerBindingRenderCase (void)
{
	deinit();
}

void SamplerBindingRenderCase::init (void)
{
	LayoutBindingRenderCase::init();
	const glw::Functions&	gl		= m_context.getRenderContext().getFunctions();
	de::Random				rnd		(deStringHash(getName()) ^ 0xff23a4);


	// Initialize texture resources
	m_textures = std::vector<glw::GLuint>(m_numBindings,  0);

	// Texture colors
	for (int texNdx = 0; texNdx < (int)m_textures.size(); ++texNdx)
		m_textureColors.push_back(getRandomColor(rnd));

	// Textures
	gl.genTextures((glw::GLsizei)m_textures.size(), &m_textures[0]);

	for (int texNdx = 0; texNdx < (int)m_textures.size(); ++texNdx)
		initializeTexture(m_bindings[texNdx], m_textures[texNdx], m_textureColors[texNdx]);

	gl.activeTexture(GL_TEXTURE0);
}

void SamplerBindingRenderCase::deinit(void)
{
	LayoutBindingRenderCase::deinit();

	// Clean up texture data
	for (int i = 0; i < (int)m_textures.size(); ++i)
	{
		if (m_textures[i])
		{
			m_context.getRenderContext().getFunctions().deleteTextures(1, &m_textures[i]);
			m_context.getRenderContext().getFunctions().bindTexture(m_textureType, 0);
		}
	}
}

TestCase::IterateResult SamplerBindingRenderCase::iterate (void)
{
	const glw::Functions&	gl				= m_context.getRenderContext().getFunctions();
	const int				iterations		= m_numBindings;
	const bool				arrayInstance	= (m_testType == TESTTYPE_BINDING_ARRAY || m_testType == TESTTYPE_BINDING_MAX_ARRAY);
	bool					imageTestPassed	= true;
	bool					queryTestPassed	= true;

	// Set the viewport and enable the shader program
	initRenderState();

	for (int iterNdx = 0; iterNdx < iterations; ++iterNdx)
	{
		// Set the uniform value indicating the current array index
		gl.uniform1i(m_shaderProgramArrayNdxLoc, iterNdx);

		// Query binding point
		const std::string	name	= arrayInstance ? getUniformName(m_uniformName, 0, iterNdx) : getUniformName(m_uniformName, iterNdx);
		const glw::GLint	binding = m_bindings[iterNdx];
		glw::GLint			val		= -1;

		gl.getUniformiv(m_program->getProgram(), gl.getUniformLocation(m_program->getProgram(), name.c_str()), &val);
		m_testCtx.getLog() << tcu::TestLog::Message << "Querying binding point for " << name << ": " << val << " == " << binding << tcu::TestLog::EndMessage;
		GLU_EXPECT_NO_ERROR(gl.getError(), "Binding point query failed");

		// Draw and verify
		if (val != binding)
			queryTestPassed = false;
		if (!drawAndVerifyResult(m_textureColors[iterNdx]))
			imageTestPassed = false;
	}

	setTestResult(queryTestPassed, imageTestPassed);
	return STOP;
}

glu::ShaderProgram* SamplerBindingRenderCase::generateShaders (void) const
{
	std::ostringstream		shaderUniformDecl;
	std::ostringstream		shaderBody;

	const std::string		texCoordType	= glu::getDataTypeName(getSamplerTexCoordType());
	const std::string		samplerType		= glu::getDataTypeName(glu::getDataTypeFromGLType(m_samplerType));
	const bool				arrayInstance	= (m_testType == TESTTYPE_BINDING_ARRAY || m_testType == TESTTYPE_BINDING_MAX_ARRAY) ? true : false;
	const int				numDeclarations =  arrayInstance ? 1 : m_numBindings;

	// Generate the uniform declarations for the vertex and fragment shaders
	for (int declNdx = 0; declNdx < numDeclarations; ++declNdx)
	{
		shaderUniformDecl << "layout(binding = " << m_bindings[declNdx] << ") uniform highp " << samplerType << " "
			<< (arrayInstance ? getUniformName(m_uniformName, declNdx, m_numBindings) : getUniformName(m_uniformName, declNdx)) << ";\n";
	}

	// Generate the shader body for the vertex and fragment shaders
	for (int bindNdx = 0; bindNdx < m_numBindings; ++bindNdx)
	{
		shaderBody	<< "	" << (bindNdx == 0 ? "if" : "else if") << " (u_arrayNdx == " << de::toString(bindNdx) << ")\n"
					<< "	{\n"
					<< "		color = texture(" << (arrayInstance ? getUniformName(m_uniformName, 0, bindNdx) : getUniformName(m_uniformName, bindNdx)) << ", " << texCoordType << "(0.5));\n"
					<< "	}\n";
	}

	shaderBody	<< "	else\n"
				<< "	{\n"
				<< "		color = vec4(0.0, 0.0, 0.0, 1.0);\n"
				<< "	}\n";

	return new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources()
					<< glu::VertexSource(generateVertexShader(m_shaderType, shaderUniformDecl.str(), shaderBody.str()))
					<< glu::FragmentSource(generateFragmentShader(m_shaderType, shaderUniformDecl.str(), shaderBody.str())));
}

void SamplerBindingRenderCase::initializeTexture (glw::GLint bindingPoint, glw::GLint textureName, const Vec4& color) const
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.activeTexture(GL_TEXTURE0 + bindingPoint);
	gl.bindTexture(m_textureType, textureName);
	gl.texParameteri(m_textureType, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	switch (m_textureType)
	{
		case GL_TEXTURE_2D:
		{
			tcu::TextureLevel level(tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNORM_INT8), TEST_TEXTURE_SIZE, TEST_TEXTURE_SIZE);
			tcu::clear(level.getAccess(), color);
			glu::texImage2D(m_context.getRenderContext(), m_textureType, 0, GL_RGBA8, level.getAccess());
			break;
		}

		case GL_TEXTURE_3D:
		{
			tcu::TextureLevel level(tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNORM_INT8), TEST_TEXTURE_SIZE, TEST_TEXTURE_SIZE, TEST_TEXTURE_SIZE);
			tcu::clear(level.getAccess(), color);
			glu::texImage3D(m_context.getRenderContext(), m_textureType, 0, GL_RGBA8, level.getAccess());
			break;
		}

		default:
			DE_ASSERT(false);
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "Texture initialization failed");
}

glu::DataType SamplerBindingRenderCase::getSamplerTexCoordType (void) const
{
	switch (m_samplerType)
	{
		case GL_SAMPLER_2D:
			return glu::TYPE_FLOAT_VEC2;

		case GL_SAMPLER_3D:
			return glu::TYPE_FLOAT_VEC3;

		default:
			DE_ASSERT(false);
			return glu::TYPE_INVALID;
	}
}


class SamplerBindingNegativeCase : public LayoutBindingNegativeCase
{
public:
									SamplerBindingNegativeCase		(Context&		context,
																	 const char*	name,
																	 const char*	desc,
																	 ShaderType		shaderType,
																	 TestType		testType,
																	 ErrorType		errorType,
																	 glw::GLenum	samplerType);
									~SamplerBindingNegativeCase		(void);

private:
	glu::ShaderProgram*				generateShaders					(void) const;
	glu::DataType					getSamplerTexCoordType			(void) const;

	const glw::GLenum				m_samplerType;
};

SamplerBindingNegativeCase::SamplerBindingNegativeCase (Context&		context,
														const char*		name,
														const char*		desc,
														ShaderType		shaderType,
														TestType		testType,
														ErrorType		errorType,
														glw::GLenum		samplerType)
	: LayoutBindingNegativeCase		(context,
									 name,
									 desc,
									 shaderType,
									 testType,
									 errorType,
									 GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS,
									 GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS,
									 GL_MAX_TESS_CONTROL_TEXTURE_IMAGE_UNITS,
									 GL_MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS,
									 GL_MAX_TEXTURE_IMAGE_UNITS,
									 GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS,
									 "u_sampler")
	, m_samplerType					(samplerType)
{
}

SamplerBindingNegativeCase::~SamplerBindingNegativeCase (void)
{
	LayoutBindingNegativeCase::deinit();
}

glu::ShaderProgram*	SamplerBindingNegativeCase::generateShaders	(void) const
{
	std::ostringstream		vertexUniformDecl;
	std::ostringstream		fragmentUniformDecl;
	std::ostringstream		tessCtrlUniformDecl;
	std::ostringstream		tessEvalUniformDecl;
	std::ostringstream		shaderBody;

	const std::string		texCoordType	= glu::getDataTypeName(getSamplerTexCoordType());
	const std::string		samplerType		= glu::getDataTypeName(glu::getDataTypeFromGLType(m_samplerType));
	const bool				arrayInstance	= (m_testType == TESTTYPE_BINDING_ARRAY || m_testType == TESTTYPE_BINDING_MAX_ARRAY);
	const int				numDeclarations = arrayInstance ? 1 : m_numBindings;

	// Generate the uniform declarations for the vertex and fragment shaders
	for (int declNdx = 0; declNdx < numDeclarations; ++declNdx)
	{
		vertexUniformDecl << "layout(binding = " << m_vertexShaderBinding[declNdx] << ") uniform highp " << samplerType
			<< " " << (arrayInstance ? getUniformName(m_uniformName, declNdx, m_numBindings) : getUniformName(m_uniformName, declNdx)) << ";\n";
		fragmentUniformDecl << "layout(binding = " << m_fragmentShaderBinding[declNdx] << ") uniform highp " << samplerType
			<< " " << (arrayInstance ? getUniformName(m_uniformName, declNdx, m_numBindings) : getUniformName(m_uniformName, declNdx)) << ";\n";
		tessCtrlUniformDecl << "layout(binding = " << m_tessCtrlShaderBinding[declNdx] << ") uniform highp " << samplerType
			<< " " << (arrayInstance ? getUniformName(m_uniformName, declNdx, m_numBindings) : getUniformName(m_uniformName, declNdx)) << ";\n";
		tessEvalUniformDecl << "layout(binding = " << m_tessEvalShaderBinding[declNdx] << ") uniform highp " << samplerType
			<< " " << (arrayInstance ? getUniformName(m_uniformName, declNdx, m_numBindings) : getUniformName(m_uniformName, declNdx)) << ";\n";
	}

	// Generate the shader body for the vertex and fragment shaders
	for (int bindNdx = 0; bindNdx < m_numBindings; ++bindNdx)
	{
		shaderBody	<< "	" << (bindNdx == 0 ? "if" : "else if") << " (u_arrayNdx == " << de::toString(bindNdx) << ")\n"
					<< "	{\n"
					<< "		color = texture(" << (arrayInstance ? getUniformName(m_uniformName, 0, bindNdx) : getUniformName(m_uniformName, bindNdx)) << ", " << texCoordType << "(0.5));\n"
					<< "	}\n";
	}

	shaderBody	<< "	else\n"
				<< "	{\n"
				<< "		color = vec4(0.0, 0.0, 0.0, 1.0);\n"
				<< "	}\n";

	glu::ProgramSources sources = glu::ProgramSources()
				<< glu::VertexSource(generateVertexShader(m_shaderType, vertexUniformDecl.str(), shaderBody.str()))
				<< glu::FragmentSource(generateFragmentShader(m_shaderType, fragmentUniformDecl.str(), shaderBody.str()));

	if (m_tessSupport)
		sources << glu::TessellationControlSource(generateTessControlShader(m_shaderType, tessCtrlUniformDecl.str(), shaderBody.str()))
				<< glu::TessellationEvaluationSource(generateTessEvaluationShader(m_shaderType, tessEvalUniformDecl.str(), shaderBody.str()));

	return new glu::ShaderProgram(m_context.getRenderContext(), sources);

}

glu::DataType SamplerBindingNegativeCase::getSamplerTexCoordType(void) const
{
	switch (m_samplerType)
	{
		case GL_SAMPLER_2D:
			return glu::TYPE_FLOAT_VEC2;

		case GL_SAMPLER_3D:
			return glu::TYPE_FLOAT_VEC3;

		default:
			DE_ASSERT(false);
			return glu::TYPE_INVALID;
	}
}

class ImageBindingRenderCase : public LayoutBindingRenderCase
{
public:
											ImageBindingRenderCase			(Context&		context,
																			 const char*	name,
																			 const char*	desc,
																			 ShaderType		shaderType,
																			 TestType		testType,
																			 glw::GLenum	imageType,
																			 glw::GLenum	textureType);
											~ImageBindingRenderCase			(void);

	void									init							(void);
	void									deinit							(void);
	IterateResult							iterate							(void);

private:
	glu::ShaderProgram*						generateShaders					(void) const;
	void									initializeImage					(glw::GLint imageBindingPoint, glw::GLint textureBindingPoint, glw::GLint textureName, const Vec4& color) const;
	glu::DataType							getImageTexCoordType			(void) const;

	const glw::GLenum						m_imageType;
	const glw::GLenum						m_textureType;

	std::vector<glw::GLuint>				m_textures;
	std::vector<Vec4>						m_textureColors;
};


ImageBindingRenderCase::ImageBindingRenderCase (Context&		context,
												const char*		name,
												const char*		desc,
												ShaderType		shaderType,
												TestType		testType,
												glw::GLenum		imageType,
												glw::GLenum		textureType)
	: LayoutBindingRenderCase		(context, name, desc, shaderType, testType, GL_MAX_IMAGE_UNITS, GL_MAX_VERTEX_IMAGE_UNIFORMS, GL_MAX_FRAGMENT_IMAGE_UNIFORMS, GL_MAX_COMBINED_IMAGE_UNIFORMS, "u_image")
	, m_imageType					(imageType)
	, m_textureType					(textureType)
{
}

ImageBindingRenderCase::~ImageBindingRenderCase (void)
{
	deinit();
}

void ImageBindingRenderCase::init (void)
{
	LayoutBindingRenderCase::init();

	const glw::Functions&	gl		= m_context.getRenderContext().getFunctions();
	de::Random				rnd		(deStringHash(getName()) ^ 0xff23a4);

	// Initialize image / texture resources
	m_textures = std::vector<glw::GLuint>(m_numBindings,  0);

	// Texture colors
	for (int texNdx = 0; texNdx < (int)m_textures.size(); ++texNdx)
		m_textureColors.push_back(getRandomColor(rnd));

	// Image textures
	gl.genTextures(m_numBindings, &m_textures[0]);

	for (int texNdx = 0; texNdx < (int)m_textures.size(); ++texNdx)
		initializeImage(m_bindings[texNdx], texNdx, m_textures[texNdx], m_textureColors[texNdx]);
}

void ImageBindingRenderCase::deinit (void)
{
	LayoutBindingRenderCase::deinit();

	// Clean up texture data
	for (int texNdx = 0; texNdx < (int)m_textures.size(); ++texNdx)
	{
		if (m_textures[texNdx])
		{
			m_context.getRenderContext().getFunctions().deleteTextures(1, &m_textures[texNdx]);
			m_context.getRenderContext().getFunctions().bindTexture(m_textureType, 0);
		}
	}
}

TestCase::IterateResult ImageBindingRenderCase::iterate	(void)
{
	const glw::Functions&	gl				= m_context.getRenderContext().getFunctions();
	const int				iterations		= m_numBindings;
	const bool				arrayInstance	= (m_testType == TESTTYPE_BINDING_ARRAY || m_testType == TESTTYPE_BINDING_MAX_ARRAY);
	bool					queryTestPassed	= true;
	bool					imageTestPassed = true;

	// Set the viewport and enable the shader program
	initRenderState();

	for (int iterNdx = 0; iterNdx < iterations; ++iterNdx)
	{
		// Set the uniform value indicating the current array index
		gl.uniform1i(m_shaderProgramArrayNdxLoc, iterNdx);

		const std::string	name	= (arrayInstance ? getUniformName(m_uniformName, 0, iterNdx) : getUniformName(m_uniformName, iterNdx));
		const glw::GLint	binding = m_bindings[iterNdx];
		glw::GLint			val		= -1;

		gl.getUniformiv(m_program->getProgram(), gl.getUniformLocation(m_program->getProgram(), name.c_str()), &val);
		m_testCtx.getLog() << tcu::TestLog::Message << "Querying binding point for " << name << ": " << val << " == " << binding << tcu::TestLog::EndMessage;
		GLU_EXPECT_NO_ERROR(gl.getError(), "Binding point query failed");

		// Draw and verify
		if (val != binding)
			queryTestPassed = false;
		if (!drawAndVerifyResult(m_textureColors[iterNdx]))
			imageTestPassed = false;
	}

	setTestResult(queryTestPassed, imageTestPassed);
	return STOP;
}

void ImageBindingRenderCase::initializeImage (glw::GLint imageBindingPoint, glw::GLint textureBindingPoint, glw::GLint textureName, const Vec4& color) const
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.activeTexture(GL_TEXTURE0 + textureBindingPoint);
	gl.bindTexture(m_textureType, textureName);
	gl.texParameteri(m_textureType, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	switch (m_textureType)
	{
		case GL_TEXTURE_2D:
		{
			tcu::TextureLevel level(tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNORM_INT8), TEST_TEXTURE_SIZE, TEST_TEXTURE_SIZE);
			tcu::clear(level.getAccess(), color);
			gl.texStorage2D(m_textureType, 1, GL_RGBA8, TEST_TEXTURE_SIZE, TEST_TEXTURE_SIZE);
			gl.texSubImage2D(m_textureType, 0, 0, 0, TEST_TEXTURE_SIZE, TEST_TEXTURE_SIZE, GL_RGBA, GL_UNSIGNED_BYTE, level.getAccess().getDataPtr());
			break;
		}

		case GL_TEXTURE_3D:
		{
			tcu::TextureLevel level(tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNORM_INT8), TEST_TEXTURE_SIZE, TEST_TEXTURE_SIZE, TEST_TEXTURE_SIZE);
			tcu::clear(level.getAccess(), color);
			gl.texStorage3D(m_textureType, 1, GL_RGBA8, TEST_TEXTURE_SIZE, TEST_TEXTURE_SIZE, TEST_TEXTURE_SIZE);
			gl.texSubImage3D(m_textureType, 0, 0, 0, 0, TEST_TEXTURE_SIZE, TEST_TEXTURE_SIZE, TEST_TEXTURE_SIZE, GL_RGBA, GL_UNSIGNED_BYTE, level.getAccess().getDataPtr());
			break;
		}

		default:
			DE_ASSERT(false);
	}

	gl.bindTexture(m_textureType, 0);
	gl.bindImageTexture(imageBindingPoint, textureName, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA8);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Image initialization failed");
}

glu::ShaderProgram* ImageBindingRenderCase::generateShaders (void) const
{
	std::ostringstream		shaderUniformDecl;
	std::ostringstream		shaderBody;

	const std::string		texCoordType	= glu::getDataTypeName(getImageTexCoordType());
	const std::string		imageType		= glu::getDataTypeName(glu::getDataTypeFromGLType(m_imageType));
	const bool				arrayInstance	= (m_testType == TESTTYPE_BINDING_ARRAY || m_testType == TESTTYPE_BINDING_MAX_ARRAY) ? true : false;
	const int				numDeclarations = (arrayInstance ? 1 : m_numBindings);

	// Generate the uniform declarations for the vertex and fragment shaders
	for (int declNdx = 0; declNdx < numDeclarations; ++declNdx)
	{
		shaderUniformDecl << "layout(rgba8, binding = " << m_bindings[declNdx] << ") uniform readonly highp " << imageType
			<< " " << (arrayInstance ? getUniformName(m_uniformName, declNdx, m_numBindings) : getUniformName(m_uniformName, declNdx)) << ";\n";
	}

	// Generate the shader body for the vertex and fragment shaders
	for (int bindNdx = 0; bindNdx < m_numBindings; ++bindNdx)
	{
		shaderBody	<< "	" << (bindNdx == 0 ? "if" : "else if") << " (u_arrayNdx == " << de::toString(bindNdx) << ")\n"
					<< "	{\n"
					<< "		color = imageLoad(" << (arrayInstance ? getUniformName(m_uniformName, 0, bindNdx) : getUniformName(m_uniformName, bindNdx)) << ", " << texCoordType << "(0));\n"
					<< "	}\n";
	}

	shaderBody	<< "	else\n"
				<< "	{\n"
				<< "		color = vec4(0.0, 0.0, 0.0, 1.0);\n"
				<< "	}\n";

	return new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources()
					<< glu::VertexSource(generateVertexShader(m_shaderType, shaderUniformDecl.str(), shaderBody.str()))
					<< glu::FragmentSource(generateFragmentShader(m_shaderType, shaderUniformDecl.str(), shaderBody.str())));
}

glu::DataType ImageBindingRenderCase::getImageTexCoordType(void) const
{
	switch (m_imageType)
	{
		case GL_IMAGE_2D:
			return glu::TYPE_INT_VEC2;

		case GL_IMAGE_3D:
			return glu::TYPE_INT_VEC3;

		default:
			DE_ASSERT(false);
			return glu::TYPE_INVALID;
	}
}


class ImageBindingNegativeCase : public LayoutBindingNegativeCase
{
public:
											ImageBindingNegativeCase		(Context&		context,
																			 const char*	name,
																			 const char*	desc,
																			 ShaderType		shaderType,
																			 TestType		testType,
																			 ErrorType		errorType,
																			 glw::GLenum	imageType);
											~ImageBindingNegativeCase		(void);

private:
	glu::ShaderProgram*						generateShaders					(void) const;
	glu::DataType							getImageTexCoordType			(void) const;

	const glw::GLenum						m_imageType;
};

ImageBindingNegativeCase::ImageBindingNegativeCase (Context&		context,
													const char*		name,
													const char*		desc,
													ShaderType		shaderType,
													TestType		testType,
													ErrorType		errorType,
													glw::GLenum		imageType)
	: LayoutBindingNegativeCase		(context,
									 name,
									 desc,
									 shaderType,
									 testType,
									 errorType,
									 GL_MAX_IMAGE_UNITS,
									 GL_MAX_VERTEX_IMAGE_UNIFORMS,
									 GL_MAX_TESS_CONTROL_IMAGE_UNIFORMS,
									 GL_MAX_TESS_EVALUATION_IMAGE_UNIFORMS,
									 GL_MAX_FRAGMENT_IMAGE_UNIFORMS,
									 GL_MAX_COMBINED_IMAGE_UNIFORMS,
									 "u_image")
	, m_imageType					(imageType)
{
}

ImageBindingNegativeCase::~ImageBindingNegativeCase (void)
{
	deinit();
}

glu::ShaderProgram* ImageBindingNegativeCase::generateShaders (void) const
{
	std::ostringstream		vertexUniformDecl;
	std::ostringstream		fragmentUniformDecl;
	std::ostringstream		tessCtrlUniformDecl;
	std::ostringstream		tessEvalUniformDecl;
	std::ostringstream		shaderBody;

	const std::string		texCoordType	= glu::getDataTypeName(getImageTexCoordType());
	const std::string		imageType		= glu::getDataTypeName(glu::getDataTypeFromGLType(m_imageType));
	const bool				arrayInstance	= (m_testType == TESTTYPE_BINDING_ARRAY || m_testType == TESTTYPE_BINDING_MAX_ARRAY);
	const int				numDeclarations = (arrayInstance ? 1 : m_numBindings);

	// Generate the uniform declarations for the vertex and fragment shaders
	for (int declNdx = 0; declNdx < numDeclarations; ++declNdx)
	{
		vertexUniformDecl << "layout(rgba8, binding = " << m_vertexShaderBinding[declNdx] << ") uniform readonly highp " << imageType
			<< " " << (arrayInstance ? getUniformName(m_uniformName, declNdx, m_numBindings) : getUniformName(m_uniformName, declNdx)) << ";\n";
		fragmentUniformDecl << "layout(rgba8, binding = " << m_fragmentShaderBinding[declNdx] << ") uniform readonly highp " << imageType
			<< " " << (arrayInstance ? getUniformName(m_uniformName, declNdx, m_numBindings) : getUniformName(m_uniformName, declNdx)) << ";\n";
		tessCtrlUniformDecl << "layout(rgba8, binding = " << m_tessCtrlShaderBinding[declNdx] << ") uniform readonly highp " << imageType
			<< " " << (arrayInstance ? getUniformName(m_uniformName, declNdx, m_numBindings) : getUniformName(m_uniformName, declNdx)) << ";\n";
		tessEvalUniformDecl << "layout(rgba8, binding = " << m_tessEvalShaderBinding[declNdx] << ") uniform readonly highp " << imageType
			<< " " << (arrayInstance ? getUniformName(m_uniformName, declNdx, m_numBindings) : getUniformName(m_uniformName, declNdx)) << ";\n";
	}

	// Generate the shader body for the vertex and fragment shaders
	for (int bindNdx = 0; bindNdx < m_numBindings; ++bindNdx)
	{
		shaderBody	<< "	" << (bindNdx == 0 ? "if" : "else if") << " (u_arrayNdx == " << de::toString(bindNdx) << ")\n"
					<< "	{\n"
					<< "		color = imageLoad(" << (arrayInstance ? getUniformName(m_uniformName, 0, bindNdx) : getUniformName(m_uniformName, bindNdx)) << ", " << texCoordType << "(0));\n"
					<< "	}\n";
	}

	shaderBody	<< "	else\n"
				<< "	{\n"
				<< "		color = vec4(0.0, 0.0, 0.0, 1.0);\n"
				<< "	}\n";

	glu::ProgramSources sources = glu::ProgramSources()
				<< glu::VertexSource(generateVertexShader(m_shaderType, vertexUniformDecl.str(), shaderBody.str()))
				<< glu::FragmentSource(generateFragmentShader(m_shaderType, fragmentUniformDecl.str(), shaderBody.str()));

	if (m_tessSupport)
		sources << glu::TessellationControlSource(generateTessControlShader(m_shaderType, tessCtrlUniformDecl.str(), shaderBody.str()))
				<< glu::TessellationEvaluationSource(generateTessEvaluationShader(m_shaderType, tessEvalUniformDecl.str(), shaderBody.str()));

	return new glu::ShaderProgram(m_context.getRenderContext(), sources);
}

glu::DataType ImageBindingNegativeCase::getImageTexCoordType(void) const
{
	switch (m_imageType)
	{
		case GL_IMAGE_2D:
			return glu::TYPE_INT_VEC2;

		case GL_IMAGE_3D:
			return glu::TYPE_INT_VEC3;

		default:
			DE_ASSERT(false);
			return glu::TYPE_INVALID;
	}
}


class UBOBindingRenderCase : public LayoutBindingRenderCase
{
public:
											UBOBindingRenderCase		(Context&		context,
																		 const char*	name,
																		 const char*	desc,
																		 ShaderType		shaderType,
																		 TestType		testType);
											~UBOBindingRenderCase		(void);

	void									init						(void);
	void									deinit						(void);
	IterateResult							iterate						(void);

private:
	glu::ShaderProgram*						generateShaders				(void) const;

	std::vector<deUint32>					m_buffers;
	std::vector<Vec4>						m_expectedColors;
};

UBOBindingRenderCase::UBOBindingRenderCase (Context&		context,
											const char*		name,
											const char*		desc,
											ShaderType		shaderType,
											TestType		testType)
	: LayoutBindingRenderCase (context, name, desc, shaderType, testType, GL_MAX_UNIFORM_BUFFER_BINDINGS, GL_MAX_VERTEX_UNIFORM_BLOCKS, GL_MAX_FRAGMENT_UNIFORM_BLOCKS, GL_MAX_COMBINED_UNIFORM_BLOCKS, "ColorBlock")
{
}

UBOBindingRenderCase::~UBOBindingRenderCase (void)
{
	deinit();
}

void UBOBindingRenderCase::init (void)
{
	LayoutBindingRenderCase::init();

	const glw::Functions&	gl		= m_context.getRenderContext().getFunctions();
	de::Random				rnd		(deStringHash(getName()) ^ 0xff23a4);

	// Initialize UBOs and related data
	m_buffers = std::vector<glw::GLuint>(m_numBindings,  0);
	gl.genBuffers((glw::GLsizei)m_buffers.size(), &m_buffers[0]);

	for (int bufNdx = 0; bufNdx < (int)m_buffers.size(); ++bufNdx)
	{
			m_expectedColors.push_back(getRandomColor(rnd));
			m_expectedColors.push_back(getRandomColor(rnd));
	}

	for (int bufNdx = 0; bufNdx < (int)m_buffers.size(); ++bufNdx)
	{
		gl.bindBuffer(GL_UNIFORM_BUFFER, m_buffers[bufNdx]);
		gl.bufferData(GL_UNIFORM_BUFFER, 2*sizeof(Vec4), &(m_expectedColors[2*bufNdx]), GL_STATIC_DRAW);
		gl.bindBufferBase(GL_UNIFORM_BUFFER, m_bindings[bufNdx], m_buffers[bufNdx]);
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "UBO setup failed");
}

void UBOBindingRenderCase::deinit (void)
{
	LayoutBindingRenderCase::deinit();

	// Clean up UBO data
	for (int bufNdx = 0; bufNdx < (int)m_buffers.size(); ++bufNdx)
	{
		if (m_buffers[bufNdx])
		{
			m_context.getRenderContext().getFunctions().deleteBuffers(1, &m_buffers[bufNdx]);
			m_context.getRenderContext().getFunctions().bindBuffer(GL_UNIFORM_BUFFER, 0);
		}
	}
}

TestCase::IterateResult UBOBindingRenderCase::iterate (void)
{
	const glw::Functions&	gl				= m_context.getRenderContext().getFunctions();
	const int				iterations		= m_numBindings;
	const glw::GLenum		prop			= GL_BUFFER_BINDING;
	const bool				arrayInstance	= (m_testType == TESTTYPE_BINDING_ARRAY || m_testType == TESTTYPE_BINDING_MAX_ARRAY);
	bool					queryTestPassed	= true;
	bool					imageTestPassed = true;

	// Set the viewport and enable the shader program
	initRenderState();

	for (int iterNdx = 0; iterNdx < iterations; ++iterNdx)
	{
		// Query binding point
		const std::string	name	= (arrayInstance ? getUniformName(m_uniformName, 0, iterNdx) : getUniformName(m_uniformName, iterNdx));
		const glw::GLint	binding = m_bindings[iterNdx];
		glw::GLint			val		= -1;

		gl.getProgramResourceiv(m_program->getProgram(), GL_UNIFORM_BLOCK, gl.getProgramResourceIndex(m_program->getProgram(), GL_UNIFORM_BLOCK, name.c_str() ), 1, &prop, 1, DE_NULL, &val);
		m_testCtx.getLog() << tcu::TestLog::Message << "Querying binding point for " << name << ": " << val << " == " << binding << tcu::TestLog::EndMessage;
		GLU_EXPECT_NO_ERROR(gl.getError(), "Binding point query failed");

		if (val != binding)
			queryTestPassed = false;

		// Draw twice to render both colors within the UBO
		for (int drawCycle = 0; drawCycle < 2; ++drawCycle)
		{
			// Set the uniform indicating the array index to be used and set the expected color
			const int arrayNdx = iterNdx*2 + drawCycle;
			gl.uniform1i(m_shaderProgramArrayNdxLoc, arrayNdx);

			if (!drawAndVerifyResult(m_expectedColors[arrayNdx]))
				imageTestPassed = false;
		}
	}

	setTestResult(queryTestPassed, imageTestPassed);
	return STOP;
}

glu::ShaderProgram* UBOBindingRenderCase::generateShaders (void) const
{
	std::ostringstream		shaderUniformDecl;
	std::ostringstream		shaderBody;
	const bool				arrayInstance	= (m_testType == TESTTYPE_BINDING_ARRAY || m_testType == TESTTYPE_BINDING_MAX_ARRAY);
	const int				numDeclarations = (arrayInstance ? 1 : m_numBindings);

	// Generate the uniform declarations for the vertex and fragment shaders
	for (int declNdx = 0; declNdx < numDeclarations; ++declNdx)
	{
		shaderUniformDecl << "layout(std140, binding = " << m_bindings[declNdx] << ") uniform "
			<< getUniformName(m_uniformName, declNdx) << "\n"
			<< "{\n"
			<< "	highp vec4 color1;\n"
			<< "	highp vec4 color2;\n"
			<< "} " << (arrayInstance ? getUniformName("colors", declNdx, m_numBindings) : getUniformName("colors", declNdx)) << ";\n";
	}

	// Generate the shader body for the vertex and fragment shaders
	for (int bindNdx = 0; bindNdx < m_numBindings*2; ++bindNdx)	// Multiply by two to cover cases for both colors for each UBO
	{
		const std::string uname = (arrayInstance ? getUniformName("colors", 0, bindNdx/2) : getUniformName("colors", bindNdx/2));
		shaderBody	<< "	" << (bindNdx == 0 ? "if" : "else if") << " (u_arrayNdx == " << de::toString(bindNdx) << ")\n"
					<< "	{\n"
					<< "		color = " << uname << (bindNdx%2 == 0 ? ".color1" : ".color2") << ";\n"
					<< "	}\n";
	}

	shaderBody	<< "	else\n"
				<< "	{\n"
				<< "		color = vec4(0.0, 0.0, 0.0, 1.0);\n"
				<< "	}\n";

	return new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources()
					<< glu::VertexSource(generateVertexShader(m_shaderType, shaderUniformDecl.str(), shaderBody.str()))
					<< glu::FragmentSource(generateFragmentShader(m_shaderType, shaderUniformDecl.str(), shaderBody.str())));
}


class UBOBindingNegativeCase : public LayoutBindingNegativeCase
{
public:
											UBOBindingNegativeCase			(Context&		context,
																			 const char*	name,
																			 const char*	desc,
																			 ShaderType		shaderType,
																			 TestType		testType,
																			 ErrorType		errorType);
											~UBOBindingNegativeCase			(void);

private:
	glu::ShaderProgram*						generateShaders					(void) const;
};

UBOBindingNegativeCase::UBOBindingNegativeCase (Context&		context,
												const char*		name,
												const char*		desc,
												ShaderType		shaderType,
												TestType		testType,
												ErrorType		errorType)
	: LayoutBindingNegativeCase(context,
								name,
								desc,
								shaderType,
								testType,
								errorType,
								GL_MAX_UNIFORM_BUFFER_BINDINGS,
								GL_MAX_VERTEX_UNIFORM_BLOCKS,
								GL_MAX_TESS_CONTROL_UNIFORM_BLOCKS,
								GL_MAX_TESS_EVALUATION_UNIFORM_BLOCKS,
								GL_MAX_FRAGMENT_UNIFORM_BLOCKS,
								GL_MAX_COMBINED_UNIFORM_BLOCKS,
								"ColorBlock")
{
}

UBOBindingNegativeCase::~UBOBindingNegativeCase (void)
{
	deinit();
}

glu::ShaderProgram* UBOBindingNegativeCase::generateShaders (void) const
{
	std::ostringstream		vertexUniformDecl;
	std::ostringstream		fragmentUniformDecl;
	std::ostringstream		tessCtrlUniformDecl;
	std::ostringstream		tessEvalUniformDecl;
	std::ostringstream		shaderBody;
	const bool				arrayInstance	= (m_testType == TESTTYPE_BINDING_ARRAY || m_testType == TESTTYPE_BINDING_MAX_ARRAY);
	const int				numDeclarations = (arrayInstance ? 1 : m_numBindings);

	// Generate the uniform declarations for the vertex and fragment shaders
	for (int declNdx = 0; declNdx < numDeclarations; ++declNdx)
	{
		vertexUniformDecl << "layout(std140, binding = " << m_vertexShaderBinding[declNdx] << ") uniform "
			<< getUniformName(m_uniformName, declNdx) << "\n"
			<< "{\n"
			<< "	highp vec4 color1;\n"
			<< "	highp vec4 color2;\n"
			<< "} " << (arrayInstance ? getUniformName("colors", declNdx, m_numBindings) : getUniformName("colors", declNdx)) << ";\n";

		fragmentUniformDecl << "layout(std140, binding = " << m_fragmentShaderBinding[declNdx] << ") uniform "
			<< getUniformName(m_uniformName, declNdx) << "\n"
			<< "{\n"
			<< "	highp vec4 color1;\n"
			<< "	highp vec4 color2;\n"
			<< "} " << (arrayInstance ? getUniformName("colors", declNdx, m_numBindings) : getUniformName("colors", declNdx)) << ";\n";

		tessCtrlUniformDecl << "layout(std140, binding = " << m_tessCtrlShaderBinding[declNdx] << ") uniform "
			<< getUniformName(m_uniformName, declNdx) << "\n"
			<< "{\n"
			<< "	highp vec4 color1;\n"
			<< "	highp vec4 color2;\n"
			<< "} " << (arrayInstance ? getUniformName("colors", declNdx, m_numBindings) : getUniformName("colors", declNdx)) << ";\n";

		tessEvalUniformDecl << "layout(std140, binding = " << m_tessCtrlShaderBinding[declNdx] << ") uniform "
			<< getUniformName(m_uniformName, declNdx) << "\n"
			<< "{\n"
			<< "	highp vec4 color1;\n"
			<< "	highp vec4 color2;\n"
			<< "} " << (arrayInstance ? getUniformName("colors", declNdx, m_numBindings) : getUniformName("colors", declNdx)) << ";\n";
	}

	// Generate the shader body for the vertex and fragment shaders
	for (int bindNdx = 0; bindNdx < m_numBindings*2; ++bindNdx)	// Multiply by two to cover cases for both colors for each UBO
	{
		const std::string uname = (arrayInstance ? getUniformName("colors", 0, bindNdx/2) : getUniformName("colors", bindNdx/2));
		shaderBody	<< "	" << (bindNdx == 0 ? "if" : "else if") << " (u_arrayNdx == " << de::toString(bindNdx) << ")\n"
					<< "	{\n"
					<< "		color = " << uname << (bindNdx%2 == 0 ? ".color1" : ".color2") << ";\n"
					<< "	}\n";
	}

	shaderBody	<< "	else\n"
				<< "	{\n"
				<< "		color = vec4(0.0, 0.0, 0.0, 1.0);\n"
				<< "	}\n";

	glu::ProgramSources sources = glu::ProgramSources()
				<< glu::VertexSource(generateVertexShader(m_shaderType, vertexUniformDecl.str(), shaderBody.str()))
				<< glu::FragmentSource(generateFragmentShader(m_shaderType, fragmentUniformDecl.str(), shaderBody.str()));

	if (m_tessSupport)
		sources << glu::TessellationControlSource(generateTessControlShader(m_shaderType, tessCtrlUniformDecl.str(), shaderBody.str()))
				<< glu::TessellationEvaluationSource(generateTessEvaluationShader(m_shaderType, tessEvalUniformDecl.str(), shaderBody.str()));

	return new glu::ShaderProgram(m_context.getRenderContext(), sources);
}


class SSBOBindingRenderCase : public LayoutBindingRenderCase
{
public:
											SSBOBindingRenderCase		(Context&		context,
																		 const char*	name,
																		 const char*	desc,
																		 ShaderType		shaderType,
																		 TestType		testType);
											~SSBOBindingRenderCase		(void);

	void									init						(void);
	void									deinit						(void);
	IterateResult							iterate						(void);

private:
	glu::ShaderProgram*						generateShaders				(void) const;

	std::vector<glw::GLuint>				m_buffers;
	std::vector<Vec4>						m_expectedColors;
};

SSBOBindingRenderCase::SSBOBindingRenderCase (Context&		context,
											  const char*	name,
											  const char*	desc,
											  ShaderType	shaderType,
											  TestType		testType)
	: LayoutBindingRenderCase (context, name, desc, shaderType, testType, GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS, GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS, GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS, "ColorBuffer")
{
}

SSBOBindingRenderCase::~SSBOBindingRenderCase (void)
{
	deinit();
}

void SSBOBindingRenderCase::init (void)
{
	LayoutBindingRenderCase::init();

	const glw::Functions&	gl		= m_context.getRenderContext().getFunctions();
	de::Random				rnd		(deStringHash(getName()) ^ 0xff23a4);

	// Initialize SSBOs and related data
	m_buffers = std::vector<glw::GLuint>(m_numBindings, 0);
	gl.genBuffers((glw::GLsizei)m_buffers.size(), &m_buffers[0]);

	for (int bufNdx = 0; bufNdx < (int)m_buffers.size(); ++bufNdx)
	{
		m_expectedColors.push_back(getRandomColor(rnd));
		m_expectedColors.push_back(getRandomColor(rnd));
	}

	for (int bufNdx = 0; bufNdx < (int)m_buffers.size(); ++bufNdx)
	{
		gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffers[bufNdx]);
		gl.bufferData(GL_SHADER_STORAGE_BUFFER, 2*sizeof(Vec4), &(m_expectedColors[2*bufNdx]), GL_STATIC_DRAW);
		gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, m_bindings[bufNdx], m_buffers[bufNdx]);
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "SSBO setup failed");
}

void SSBOBindingRenderCase::deinit (void)
{
	LayoutBindingRenderCase::deinit();

	// Clean up SSBO data
	for (int bufNdx = 0; bufNdx < (int)m_buffers.size(); ++bufNdx)
	{
		if (m_buffers[bufNdx])
		{
			m_context.getRenderContext().getFunctions().deleteBuffers(1, &m_buffers[bufNdx]);
			m_context.getRenderContext().getFunctions().bindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
			m_buffers[bufNdx] = 0;
		}
	}
}

TestCase::IterateResult SSBOBindingRenderCase::iterate (void)
{
	const glw::Functions&	gl				= m_context.getRenderContext().getFunctions();
	const int				iterations		= m_numBindings;
	const glw::GLenum		prop			= GL_BUFFER_BINDING;
	const bool				arrayInstance	= (m_testType == TESTTYPE_BINDING_ARRAY || m_testType == TESTTYPE_BINDING_MAX_ARRAY);
	bool					queryTestPassed	= true;
	bool					imageTestPassed = true;

	initRenderState();

	for (int iterNdx = 0; iterNdx < iterations; ++iterNdx)
	{
		// Query binding point
		const std::string	name	= (arrayInstance ? getUniformName(m_uniformName, 0, iterNdx) : getUniformName(m_uniformName, iterNdx));
		const glw::GLint	binding = m_bindings[iterNdx];
		glw::GLint			val		= -1;

		gl.getProgramResourceiv(m_program->getProgram(), GL_SHADER_STORAGE_BLOCK, gl.getProgramResourceIndex(m_program->getProgram(), GL_SHADER_STORAGE_BLOCK, name.c_str() ), 1, &prop, 1, DE_NULL, &val);
		m_testCtx.getLog() << tcu::TestLog::Message << "Querying binding point for " << name << ": " << val << " == " << binding << tcu::TestLog::EndMessage;
		GLU_EXPECT_NO_ERROR(gl.getError(), "Binding point query failed");

		if (val != binding)
			queryTestPassed = false;

		// Draw twice to render both colors within the SSBO
		for (int drawCycle = 0; drawCycle < 2; ++drawCycle)
		{
			// Set the uniform indicating the array index to be used and set the expected color
			const int arrayNdx = iterNdx*2 + drawCycle;
			gl.uniform1i(m_shaderProgramArrayNdxLoc, arrayNdx);

			if (!drawAndVerifyResult(m_expectedColors[arrayNdx]))
				imageTestPassed = false;
		}
	}

	setTestResult(queryTestPassed, imageTestPassed);
	return STOP;
}

glu::ShaderProgram* SSBOBindingRenderCase::generateShaders (void) const
{
	std::ostringstream		shaderUniformDecl;
	std::ostringstream		shaderBody;
	const bool				arrayInstance	= (m_testType == TESTTYPE_BINDING_ARRAY || m_testType == TESTTYPE_BINDING_MAX_ARRAY);
	const int				numDeclarations = (arrayInstance ? 1 : m_numBindings);

	// Generate the uniform declarations for the vertex and fragment shaders
	for (int declNdx = 0; declNdx < numDeclarations; ++declNdx)
	{
		shaderUniformDecl << "layout(std140, binding = " << m_bindings[declNdx] << ") buffer "
			<< getUniformName(m_uniformName, declNdx) << "\n"
			<< "{\n"
			<< "	highp vec4 color1;\n"
			<< "	highp vec4 color2;\n"
			<< "} " << (arrayInstance ? getUniformName("colors", declNdx, m_numBindings) : getUniformName("colors", declNdx)) << ";\n";
	}

	// Generate the shader body for the vertex and fragment shaders
	for (int bindNdx = 0; bindNdx < m_numBindings*2; ++bindNdx)	// Multiply by two to cover cases for both colors for each UBO
	{
		const std::string uname = (arrayInstance ? getUniformName("colors", 0, bindNdx/2) : getUniformName("colors", bindNdx/2));
		shaderBody	<< "	" << (bindNdx == 0 ? "if" : "else if") << " (u_arrayNdx == " << de::toString(bindNdx) << ")\n"
					<< "	{\n"
					<< "		color = " << uname << (bindNdx%2 == 0 ? ".color1" : ".color2") << ";\n"
					<< "	}\n";
	}

	shaderBody	<< "	else\n"
				<< "	{\n"
				<< "		color = vec4(0.0, 0.0, 0.0, 1.0);\n"
				<< "	}\n";

	return new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources()
					<< glu::VertexSource(generateVertexShader(m_shaderType, shaderUniformDecl.str(), shaderBody.str()))
					<< glu::FragmentSource(generateFragmentShader(m_shaderType, shaderUniformDecl.str(), shaderBody.str())));
}


class SSBOBindingNegativeCase : public LayoutBindingNegativeCase
{
public:
											SSBOBindingNegativeCase			(Context&		context,
																			 const char*	name,
																			 const char*	desc,
																			 ShaderType		shaderType,
																			 TestType		testType,
																			 ErrorType		errorType);
											~SSBOBindingNegativeCase		(void);

private:
	glu::ShaderProgram*						generateShaders					(void) const;
};

SSBOBindingNegativeCase::SSBOBindingNegativeCase (Context& context,
												  const char* name,
												  const char* desc,
												  ShaderType shaderType,
												  TestType testType,
												  ErrorType errorType)
	: LayoutBindingNegativeCase(context,
								name,
								desc,
								shaderType,
								testType,
								errorType,
								GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS,
								GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS,
								GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS,
								GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS,
								GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS,
								GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS,
								"ColorBuffer")
{
}

SSBOBindingNegativeCase::~SSBOBindingNegativeCase (void)
{
	deinit();
}

glu::ShaderProgram* SSBOBindingNegativeCase::generateShaders (void) const
{
	std::ostringstream		vertexUniformDecl;
	std::ostringstream		fragmentUniformDecl;
	std::ostringstream		tessCtrlUniformDecl;
	std::ostringstream		tessEvalUniformDecl;
	std::ostringstream		shaderBody;
	const bool				arrayInstance	= (m_testType == TESTTYPE_BINDING_ARRAY || m_testType == TESTTYPE_BINDING_MAX_ARRAY);
	const int				numDeclarations = (arrayInstance ? 1 : m_numBindings);

	// Generate the uniform declarations for the vertex and fragment shaders
	for (int declNdx = 0; declNdx < numDeclarations; ++declNdx)
	{
		vertexUniformDecl << "layout(std140, binding = " << m_vertexShaderBinding[declNdx] << ") buffer "
			<< getUniformName(m_uniformName, declNdx) << "\n"
			<< "{\n"
			<< "	highp vec4 color1;\n"
			<< "	highp vec4 color2;\n"
			<< "} " << (arrayInstance ? getUniformName("colors", declNdx, m_numBindings) : getUniformName("colors", declNdx)) << ";\n";

		fragmentUniformDecl << "layout(std140, binding = " << m_fragmentShaderBinding[declNdx] << ") buffer "
			<< getUniformName(m_uniformName, declNdx) << "\n"
			<< "{\n"
			<< "	highp vec4 color1;\n"
			<< "	highp vec4 color2;\n"
			<< "} " << (arrayInstance ? getUniformName("colors", declNdx, m_numBindings) : getUniformName("colors", declNdx)) << ";\n";

		tessCtrlUniformDecl << "layout(std140, binding = " << m_tessCtrlShaderBinding[declNdx] << ") buffer "
			<< getUniformName(m_uniformName, declNdx) << "\n"
			<< "{\n"
			<< "	highp vec4 color1;\n"
			<< "	highp vec4 color2;\n"
			<< "} " << (arrayInstance ? getUniformName("colors", declNdx, m_numBindings) : getUniformName("colors", declNdx)) << ";\n";

		tessEvalUniformDecl << "layout(std140, binding = " << m_tessEvalShaderBinding[declNdx] << ") buffer "
			<< getUniformName(m_uniformName, declNdx) << "\n"
			<< "{\n"
			<< "	highp vec4 color1;\n"
			<< "	highp vec4 color2;\n"
			<< "} " << (arrayInstance ? getUniformName("colors", declNdx, m_numBindings) : getUniformName("colors", declNdx)) << ";\n";
	}

	// Generate the shader body for the vertex and fragment shaders
	for (int bindNdx = 0; bindNdx < m_numBindings*2; ++bindNdx)	// Multiply by two to cover cases for both colors for each UBO
	{
		const std::string uname = (arrayInstance ? getUniformName("colors", 0, bindNdx/2) : getUniformName("colors", bindNdx/2));
		shaderBody	<< "	" << (bindNdx == 0 ? "if" : "else if") << " (u_arrayNdx == " << de::toString(bindNdx) << ")\n"
					<< "	{\n"
					<< "		color = " << uname << (bindNdx%2 == 0 ? ".color1" : ".color2") << ";\n"
					<< "	}\n";
	}

	shaderBody	<< "	else\n"
				<< "	{\n"
				<< "		color = vec4(0.0, 0.0, 0.0, 1.0);\n"
				<< "	}\n";

	glu::ProgramSources sources = glu::ProgramSources()
				<< glu::VertexSource(generateVertexShader(m_shaderType, vertexUniformDecl.str(), shaderBody.str()))
				<< glu::FragmentSource(generateFragmentShader(m_shaderType, fragmentUniformDecl.str(), shaderBody.str()));

	if (m_tessSupport)
		sources << glu::TessellationControlSource(generateTessControlShader(m_shaderType, tessCtrlUniformDecl.str(), shaderBody.str()))
				<< glu::TessellationEvaluationSource(generateTessEvaluationShader(m_shaderType, tessEvalUniformDecl.str(), shaderBody.str()));

	return new glu::ShaderProgram(m_context.getRenderContext(), sources);
}


} // Anonymous

LayoutBindingTests::LayoutBindingTests (Context& context)
	: TestCaseGroup (context, "layout_binding", "Layout binding tests")
{
}

LayoutBindingTests::~LayoutBindingTests (void)
{
}

void LayoutBindingTests::init (void)
{
	// Render test groups
	tcu::TestCaseGroup* const samplerBindingTestGroup			= new tcu::TestCaseGroup(m_testCtx, "sampler",		"Test sampler layout binding");
	tcu::TestCaseGroup* const sampler2dBindingTestGroup			= new tcu::TestCaseGroup(m_testCtx, "sampler2d",	"Test sampler2d layout binding");
	tcu::TestCaseGroup* const sampler3dBindingTestGroup			= new tcu::TestCaseGroup(m_testCtx, "sampler3d",	"Test sampler3d layout binding");

	tcu::TestCaseGroup* const imageBindingTestGroup				= new tcu::TestCaseGroup(m_testCtx, "image",		"Test image layout binding");
	tcu::TestCaseGroup* const image2dBindingTestGroup			= new tcu::TestCaseGroup(m_testCtx, "image2d",		"Test image2d layout binding");
	tcu::TestCaseGroup* const image3dBindingTestGroup			= new tcu::TestCaseGroup(m_testCtx, "image3d",		"Test image3d layout binding");

	tcu::TestCaseGroup* const UBOBindingTestGroup				= new tcu::TestCaseGroup(m_testCtx, "ubo",			"Test UBO layout binding");
	tcu::TestCaseGroup* const SSBOBindingTestGroup				= new tcu::TestCaseGroup(m_testCtx, "ssbo",			"Test SSBO layout binding");

	// Negative test groups
	tcu::TestCaseGroup* const negativeBindingTestGroup			= new tcu::TestCaseGroup(m_testCtx, "negative",		"Test layout binding with invalid bindings");

	tcu::TestCaseGroup* const negativeSamplerBindingTestGroup	= new tcu::TestCaseGroup(m_testCtx, "sampler",		"Test sampler layout binding with invalid bindings");
	tcu::TestCaseGroup* const negativeSampler2dBindingTestGroup	= new tcu::TestCaseGroup(m_testCtx, "sampler2d",	"Test sampler2d layout binding with invalid bindings");
	tcu::TestCaseGroup* const negativeSampler3dBindingTestGroup	= new tcu::TestCaseGroup(m_testCtx, "sampler3d",	"Test sampler3d layout binding with invalid bindings");

	tcu::TestCaseGroup* const negativeImageBindingTestGroup		= new tcu::TestCaseGroup(m_testCtx, "image",		"Test image layout binding with invalid bindings");
	tcu::TestCaseGroup* const negativeImage2dBindingTestGroup	= new tcu::TestCaseGroup(m_testCtx, "image2d",		"Test image2d layout binding with invalid bindings");
	tcu::TestCaseGroup* const negativeImage3dBindingTestGroup	= new tcu::TestCaseGroup(m_testCtx, "image3d",		"Test image3d layout binding with invalid bindings");

	tcu::TestCaseGroup* const negativeUBOBindingTestGroup		= new tcu::TestCaseGroup(m_testCtx, "ubo",			"Test UBO layout binding with invalid bindings");
	tcu::TestCaseGroup* const negativeSSBOBindingTestGroup		= new tcu::TestCaseGroup(m_testCtx, "ssbo",			"Test SSBO layout binding with invalid bindings");

	static const struct RenderTestType
	{
		ShaderType				shaderType;
		TestType				testType;
		std::string				name;
		std::string				descPostfix;
	} s_renderTestTypes[] =
	{
		{ SHADERTYPE_VERTEX,	TESTTYPE_BINDING_SINGLE,		"vertex_binding_single",		"a single instance" },
		{ SHADERTYPE_VERTEX,	TESTTYPE_BINDING_MAX,			"vertex_binding_max",			"maximum binding point"	},
		{ SHADERTYPE_VERTEX,	TESTTYPE_BINDING_MULTIPLE,		"vertex_binding_multiple",		"multiple instances"},
		{ SHADERTYPE_VERTEX,	TESTTYPE_BINDING_ARRAY,			"vertex_binding_array",			"an array instance" },
		{ SHADERTYPE_VERTEX,	TESTTYPE_BINDING_MAX_ARRAY,		"vertex_binding_max_array",		"an array instance with maximum binding point" },

		{ SHADERTYPE_FRAGMENT,	TESTTYPE_BINDING_SINGLE,		"fragment_binding_single",		"a single instance" },
		{ SHADERTYPE_FRAGMENT,	TESTTYPE_BINDING_MAX,			"fragment_binding_max",			"maximum binding point"	},
		{ SHADERTYPE_FRAGMENT,	TESTTYPE_BINDING_MULTIPLE,		"fragment_binding_multiple",	"multiple instances"},
		{ SHADERTYPE_FRAGMENT,	TESTTYPE_BINDING_ARRAY,			"fragment_binding_array",		"an array instance" },
		{ SHADERTYPE_FRAGMENT,	TESTTYPE_BINDING_MAX_ARRAY,		"fragment_binding_max_array",	"an array instance with maximum binding point" },
	};

	static const struct NegativeTestType
	{
		ShaderType								shaderType;
		TestType								testType;
		LayoutBindingNegativeCase::ErrorType	errorType;
		std::string								name;
		std::string								descPostfix;
	} s_negativeTestTypes[] =
	{
		{ SHADERTYPE_VERTEX,			TESTTYPE_BINDING_SINGLE,		LayoutBindingNegativeCase::ERRORTYPE_OVER_MAX_UNITS,	"vertex_binding_over_max",					"over maximum binding point"   },
		{ SHADERTYPE_FRAGMENT,			TESTTYPE_BINDING_SINGLE,		LayoutBindingNegativeCase::ERRORTYPE_OVER_MAX_UNITS,	"fragment_binding_over_max",				"over maximum binding point"   },
		{ SHADERTYPE_TESS_CONTROL,		TESTTYPE_BINDING_SINGLE,		LayoutBindingNegativeCase::ERRORTYPE_OVER_MAX_UNITS,	"tess_control_binding_over_max",			"over maximum binding point"   },
		{ SHADERTYPE_TESS_EVALUATION,	TESTTYPE_BINDING_SINGLE,		LayoutBindingNegativeCase::ERRORTYPE_OVER_MAX_UNITS,	"tess_evaluation_binding_over_max",			"over maximum binding point"   },
		{ SHADERTYPE_VERTEX,			TESTTYPE_BINDING_SINGLE,		LayoutBindingNegativeCase::ERRORTYPE_LESS_THAN_ZERO,	"vertex_binding_neg",						"negative binding point"	   },
		{ SHADERTYPE_FRAGMENT,			TESTTYPE_BINDING_SINGLE,		LayoutBindingNegativeCase::ERRORTYPE_LESS_THAN_ZERO,	"fragment_binding_neg",						"negative binding point"	   },
		{ SHADERTYPE_TESS_CONTROL,		TESTTYPE_BINDING_SINGLE,		LayoutBindingNegativeCase::ERRORTYPE_LESS_THAN_ZERO,	"tess_control_binding_neg",					"negative binding point"	   },
		{ SHADERTYPE_TESS_EVALUATION,	TESTTYPE_BINDING_SINGLE,		LayoutBindingNegativeCase::ERRORTYPE_LESS_THAN_ZERO,	"tess_evaluation_binding_neg",				"negative binding point"	   },

		{ SHADERTYPE_VERTEX,			TESTTYPE_BINDING_ARRAY,			LayoutBindingNegativeCase::ERRORTYPE_OVER_MAX_UNITS,	"vertex_binding_over_max_array",			"over maximum binding point"   },
		{ SHADERTYPE_FRAGMENT,			TESTTYPE_BINDING_ARRAY,			LayoutBindingNegativeCase::ERRORTYPE_OVER_MAX_UNITS,	"fragment_binding_over_max_array",			"over maximum binding point"   },
		{ SHADERTYPE_TESS_CONTROL,		TESTTYPE_BINDING_ARRAY,			LayoutBindingNegativeCase::ERRORTYPE_OVER_MAX_UNITS,	"tess_control_binding_over_max_array",		"over maximum binding point"   },
		{ SHADERTYPE_TESS_EVALUATION,	TESTTYPE_BINDING_ARRAY,			LayoutBindingNegativeCase::ERRORTYPE_OVER_MAX_UNITS,	"tess_evaluation_binding_over_max_array",	"over maximum binding point"   },
		{ SHADERTYPE_VERTEX,			TESTTYPE_BINDING_ARRAY,			LayoutBindingNegativeCase::ERRORTYPE_LESS_THAN_ZERO,	"vertex_binding_neg_array",					"negative binding point"	   },
		{ SHADERTYPE_FRAGMENT,			TESTTYPE_BINDING_ARRAY,			LayoutBindingNegativeCase::ERRORTYPE_LESS_THAN_ZERO,	"fragment_binding_neg_array",				"negative binding point"	   },
		{ SHADERTYPE_TESS_CONTROL,		TESTTYPE_BINDING_ARRAY,			LayoutBindingNegativeCase::ERRORTYPE_LESS_THAN_ZERO,	"tess_control_binding_neg_array",			"negative binding point"	   },
		{ SHADERTYPE_TESS_EVALUATION,	TESTTYPE_BINDING_ARRAY,			LayoutBindingNegativeCase::ERRORTYPE_LESS_THAN_ZERO,	"tess_evaluation_binding_neg_array",		"negative binding point"	   },

		{ SHADERTYPE_ALL,				TESTTYPE_BINDING_SINGLE,		LayoutBindingNegativeCase::ERRORTYPE_CONTRADICTORY,		"binding_contradictory",					"contradictory binding points" },
		{ SHADERTYPE_ALL,				TESTTYPE_BINDING_ARRAY,			LayoutBindingNegativeCase::ERRORTYPE_CONTRADICTORY,		"binding_contradictory_array",				"contradictory binding points" },
	};

	// Render tests
	for (int testNdx = 0; testNdx < DE_LENGTH_OF_ARRAY(s_renderTestTypes); ++testNdx)
	{
		const RenderTestType& test = s_renderTestTypes[testNdx];

		// Render sampler binding tests
		sampler2dBindingTestGroup->addChild(new SamplerBindingRenderCase(m_context, test.name.c_str(), ("Sampler2D layout binding with " + test.descPostfix).c_str(), test.shaderType, test.testType, GL_SAMPLER_2D, GL_TEXTURE_2D));
		sampler3dBindingTestGroup->addChild(new SamplerBindingRenderCase(m_context, test.name.c_str(), ("Sampler3D layout binding with " + test.descPostfix).c_str(), test.shaderType, test.testType, GL_SAMPLER_3D, GL_TEXTURE_3D));

		// Render image binding tests
		image2dBindingTestGroup->addChild(new ImageBindingRenderCase(m_context, test.name.c_str(), ("Image2D layout binding with " + test.descPostfix).c_str(), test.shaderType, test.testType, GL_IMAGE_2D, GL_TEXTURE_2D));
		image3dBindingTestGroup->addChild(new ImageBindingRenderCase(m_context, test.name.c_str(), ("Image3D layout binding with " + test.descPostfix).c_str(), test.shaderType, test.testType, GL_IMAGE_3D, GL_TEXTURE_3D));

		// Render UBO binding tests
		UBOBindingTestGroup->addChild(new UBOBindingRenderCase(m_context, test.name.c_str(), ("UBO layout binding with " + test.descPostfix).c_str(), test.shaderType, test.testType));

		// Render SSBO binding tests
		SSBOBindingTestGroup->addChild(new SSBOBindingRenderCase(m_context, test.name.c_str(), ("SSBO layout binding with " + test.descPostfix).c_str(), test.shaderType, test.testType));
	}

	// Negative binding tests
	for (int testNdx = 0; testNdx < DE_LENGTH_OF_ARRAY(s_negativeTestTypes); ++testNdx)
	{
		const NegativeTestType& test = s_negativeTestTypes[testNdx];

		// Negative sampler binding tests
		negativeSampler2dBindingTestGroup->addChild(new SamplerBindingNegativeCase(m_context, test.name.c_str(), ("Invalid sampler2d layout binding using " + test.descPostfix).c_str(), test.shaderType, test.testType, test.errorType, GL_SAMPLER_2D));
		negativeSampler3dBindingTestGroup->addChild(new SamplerBindingNegativeCase(m_context, test.name.c_str(), ("Invalid sampler3d layout binding using " + test.descPostfix).c_str(), test.shaderType, test.testType, test.errorType, GL_SAMPLER_3D));

		// Negative image binding tests
		negativeImage2dBindingTestGroup->addChild(new ImageBindingNegativeCase(m_context, test.name.c_str(), ("Invalid image2d layout binding using " + test.descPostfix).c_str(), test.shaderType, test.testType, test.errorType, GL_IMAGE_2D));
		negativeImage3dBindingTestGroup->addChild(new ImageBindingNegativeCase(m_context, test.name.c_str(), ("Invalid image3d layout binding using " + test.descPostfix).c_str(), test.shaderType, test.testType, test.errorType, GL_IMAGE_3D));

		// Negative UBO binding tests
		negativeUBOBindingTestGroup->addChild(new UBOBindingNegativeCase(m_context, test.name.c_str(), ("Invalid UBO layout binding using " + test.descPostfix).c_str(), test.shaderType, test.testType, test.errorType));

		// Negative SSBO binding tests
		negativeSSBOBindingTestGroup->addChild(new SSBOBindingNegativeCase(m_context, test.name.c_str(), ("Invalid SSBO layout binding using " + test.descPostfix).c_str(), test.shaderType, test.testType, test.errorType));
	}

	samplerBindingTestGroup->addChild(sampler2dBindingTestGroup);
	samplerBindingTestGroup->addChild(sampler3dBindingTestGroup);

	imageBindingTestGroup->addChild(image2dBindingTestGroup);
	imageBindingTestGroup->addChild(image3dBindingTestGroup);

	negativeSamplerBindingTestGroup->addChild(negativeSampler2dBindingTestGroup);
	negativeSamplerBindingTestGroup->addChild(negativeSampler3dBindingTestGroup);

	negativeImageBindingTestGroup->addChild(negativeImage2dBindingTestGroup);
	negativeImageBindingTestGroup->addChild(negativeImage3dBindingTestGroup);

	negativeBindingTestGroup->addChild(negativeSamplerBindingTestGroup);
	negativeBindingTestGroup->addChild(negativeUBOBindingTestGroup);
	negativeBindingTestGroup->addChild(negativeSSBOBindingTestGroup);
	negativeBindingTestGroup->addChild(negativeImageBindingTestGroup);

	addChild(samplerBindingTestGroup);
	addChild(UBOBindingTestGroup);
	addChild(SSBOBindingTestGroup);
	addChild(imageBindingTestGroup);
	addChild(negativeBindingTestGroup);
}

} // Functional
} // gles31
} // deqp
