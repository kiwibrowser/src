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
 * \brief gl_FragDepth tests.
 */ /*-------------------------------------------------------------------*/

#include "glcFragDepthTests.hpp"
#include "deMath.h"
#include "deRandom.hpp"
#include "deString.h"
#include "gluDrawUtil.hpp"
#include "gluPixelTransfer.hpp"
#include "gluShaderProgram.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuImageCompare.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuStringTemplate.hpp"
#include "tcuSurface.hpp"
#include "tcuTestLog.hpp"
#include "tcuVector.hpp"

// For setupDefaultUniforms()
#include "glcShaderRenderCase.hpp"

namespace deqp
{

using tcu::Vec2;
using tcu::Vec3;
using tcu::Vec4;
using tcu::TestLog;
using std::string;
using std::vector;

typedef float (*EvalFragDepthFunc)(const Vec2& coord);

static const char* s_vertexShaderSrc = "${VERSION_DECL}\n"
									   "in highp vec4 a_position;\n"
									   "in highp vec2 a_coord;\n"
									   "out highp vec2 v_coord;\n"
									   "void main (void)\n"
									   "{\n"
									   "   gl_Position = a_position;\n"
									   "   v_coord = a_coord;\n"
									   "}\n";
static const char* s_defaultFragmentShaderSrc = "${VERSION_DECL}\n"
												"uniform highp vec4 u_color;\n"
												"layout(location = 0) out mediump vec4 o_color;\n"
												"void main (void)\n"
												"{\n"
												"   o_color = u_color;\n"
												"}\n";

template <typename T>
static inline bool compare(deUint32 func, T a, T b)
{
	switch (func)
	{
	case GL_NEVER:
		return false;
	case GL_ALWAYS:
		return true;
	case GL_LESS:
		return a < b;
	case GL_LEQUAL:
		return a <= b;
	case GL_EQUAL:
		return a == b;
	case GL_NOTEQUAL:
		return a != b;
	case GL_GEQUAL:
		return a >= b;
	case GL_GREATER:
		return a > b;
	default:
		DE_ASSERT(DE_FALSE);
		return false;
	}
}

static std::string specializeVersion(const std::string& source, glu::GLSLVersion version)
{
	DE_ASSERT(version == glu::GLSL_VERSION_300_ES || version == glu::GLSL_VERSION_310_ES ||
			  version >= glu::GLSL_VERSION_330);
	std::map<std::string, std::string> args;
	args["VERSION_DECL"] = glu::getGLSLVersionDeclaration(version);
	return tcu::StringTemplate(source.c_str()).specialize(args);
}

class FragDepthCompareCase : public TestCase
{
public:
	FragDepthCompareCase(Context& context, const char* name, const char* desc, glu::GLSLVersion glslVersion,
						 const char* fragSrc, EvalFragDepthFunc evalFunc, deUint32 compareFunc);
	~FragDepthCompareCase(void);

	IterateResult iterate(void);

private:
	glu::GLSLVersion  m_glslVersion;
	string			  m_fragSrc;
	EvalFragDepthFunc m_evalFunc;
	deUint32		  m_compareFunc;
};

FragDepthCompareCase::FragDepthCompareCase(Context& context, const char* name, const char* desc,
										   glu::GLSLVersion glslVersion, const char* fragSrc,
										   EvalFragDepthFunc evalFunc, deUint32 compareFunc)
	: TestCase(context, name, desc)
	, m_glslVersion(glslVersion)
	, m_fragSrc(fragSrc)
	, m_evalFunc(evalFunc)
	, m_compareFunc(compareFunc)
{
}

FragDepthCompareCase::~FragDepthCompareCase(void)
{
}

FragDepthCompareCase::IterateResult FragDepthCompareCase::iterate(void)
{
	TestLog&				 log = m_testCtx.getLog();
	const glw::Functions&	gl  = m_context.getRenderContext().getFunctions();
	de::Random				 rnd(deStringHash(getName()));
	const tcu::RenderTarget& renderTarget = m_context.getRenderContext().getRenderTarget();
	int						 viewportW	= de::min(128, renderTarget.getWidth());
	int						 viewportH	= de::min(128, renderTarget.getHeight());
	int						 viewportX	= rnd.getInt(0, renderTarget.getWidth() - viewportW);
	int						 viewportY	= rnd.getInt(0, renderTarget.getHeight() - viewportH);
	tcu::Surface			 renderedFrame(viewportW, viewportH);
	tcu::Surface			 referenceFrame(viewportW, viewportH);
	const float				 constDepth = 0.1f;

	if (renderTarget.getDepthBits() == 0)
		throw tcu::NotSupportedError("Depth buffer is required", "", __FILE__, __LINE__);

	gl.viewport(viewportX, viewportY, viewportW, viewportH);
	gl.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	gl.enable(GL_DEPTH_TEST);

	static const deUint16 quadIndices[] = { 0, 1, 2, 2, 1, 3 };

	// Fill viewport with 2 quads - one with constant depth and another with d = [-1..1]
	{
		glu::ShaderProgram basicQuadProgram(
			m_context.getRenderContext(),
			glu::makeVtxFragSources(specializeVersion(s_vertexShaderSrc, m_glslVersion).c_str(),
									specializeVersion(s_defaultFragmentShaderSrc, m_glslVersion).c_str()));

		if (!basicQuadProgram.isOk())
		{
			log << basicQuadProgram;
			TCU_FAIL("Compile failed");
		}

		const float constDepthCoord[] = { -1.0f, -1.0f, constDepth, 1.0f, -1.0f, +1.0f, constDepth, 1.0f,
										  0.0f,  -1.0f, constDepth, 1.0f, 0.0f,  +1.0f, constDepth, 1.0f };
		const float varyingDepthCoord[] = { 0.0f,  -1.0f, +1.0f, 1.0f, 0.0f,  +1.0f, 0.0f,  1.0f,
											+1.0f, -1.0f, 0.0f,  1.0f, +1.0f, +1.0f, -1.0f, 1.0f };

		gl.useProgram(basicQuadProgram.getProgram());
		gl.uniform4f(gl.getUniformLocation(basicQuadProgram.getProgram(), "u_color"), 0.0f, 0.0f, 1.0f, 1.0f);
		gl.depthFunc(GL_ALWAYS);

		{
			glu::VertexArrayBinding posBinding = glu::va::Float("a_position", 4, 4, 0, &constDepthCoord[0]);
			glu::draw(m_context.getRenderContext(), basicQuadProgram.getProgram(), 1, &posBinding,
					  glu::pr::Triangles(DE_LENGTH_OF_ARRAY(quadIndices), &quadIndices[0]));
		}

		{
			glu::VertexArrayBinding posBinding = glu::va::Float("a_position", 4, 4, 0, &varyingDepthCoord[0]);
			glu::draw(m_context.getRenderContext(), basicQuadProgram.getProgram(), 1, &posBinding,
					  glu::pr::Triangles(DE_LENGTH_OF_ARRAY(quadIndices), &quadIndices[0]));
		}

		GLU_EXPECT_NO_ERROR(gl.getError(), "Draw base quads");
	}

	// Render with depth test.
	{
		glu::ShaderProgram program(m_context.getRenderContext(),
								   glu::makeVtxFragSources(specializeVersion(s_vertexShaderSrc, m_glslVersion).c_str(),
														   specializeVersion(m_fragSrc, m_glslVersion).c_str()));
		log << program;

		if (!program.isOk())
			TCU_FAIL("Compile failed");

		const float coord[]	= { 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f };
		const float position[] = { -1.0f, -1.0f, +1.0f, 1.0f, -1.0f, +1.0f, 0.0f,  1.0f,
								   +1.0f, -1.0f, 0.0f,  1.0f, +1.0f, +1.0f, -1.0f, 1.0f };

		gl.useProgram(program.getProgram());
		gl.depthFunc(m_compareFunc);
		gl.uniform4f(gl.getUniformLocation(program.getProgram(), "u_color"), 0.0f, 1.0f, 0.0f, 1.0f);

		// Setup default helper uniforms.
		setupDefaultUniforms(m_context.getRenderContext(), program.getProgram());

		{
			glu::VertexArrayBinding vertexArrays[] = { glu::va::Float("a_position", 4, 4, 0, &position[0]),
													   glu::va::Float("a_coord", 2, 4, 0, &coord[0]) };
			glu::draw(m_context.getRenderContext(), program.getProgram(), DE_LENGTH_OF_ARRAY(vertexArrays),
					  &vertexArrays[0], glu::pr::Triangles(DE_LENGTH_OF_ARRAY(quadIndices), &quadIndices[0]));
		}

		GLU_EXPECT_NO_ERROR(gl.getError(), "Draw test quad");
	}

	glu::readPixels(m_context.getRenderContext(), viewportX, viewportY, renderedFrame.getAccess());

	// Render reference.
	for (int y = 0; y < referenceFrame.getHeight(); y++)
	{
		float yf   = ((float)y + 0.5f) / (float)referenceFrame.getHeight();
		int   half = de::clamp((int)((float)referenceFrame.getWidth() * 0.5f + 0.5f), 0, referenceFrame.getWidth());

		// Fill left half - comparison to constant 0.5
		for (int x = 0; x < half; x++)
		{
			float xf	= ((float)x + 0.5f) / (float)referenceFrame.getWidth();
			float d		= m_evalFunc(Vec2(xf, yf));
			bool  dpass = compare(m_compareFunc, d, constDepth * 0.5f + 0.5f);

			referenceFrame.setPixel(x, y, dpass ? tcu::RGBA::green() : tcu::RGBA::blue());
		}

		// Fill right half - comparison to interpolated depth
		for (int x = half; x < referenceFrame.getWidth(); x++)
		{
			float xf	= ((float)x + 0.5f) / (float)referenceFrame.getWidth();
			float xh	= ((float)x - (float)half + 0.5f) / (float)(referenceFrame.getWidth() - half);
			float rd	= 1.0f - (xh + yf) * 0.5f;
			float d		= m_evalFunc(Vec2(xf, yf));
			bool  dpass = compare(m_compareFunc, d, rd);

			referenceFrame.setPixel(x, y, dpass ? tcu::RGBA::green() : tcu::RGBA::blue());
		}
	}

	bool isOk = tcu::fuzzyCompare(log, "Result", "Image comparison result", referenceFrame, renderedFrame, 0.05f,
								  tcu::COMPARE_LOG_RESULT);
	m_testCtx.setTestResult(isOk ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL, isOk ? "Pass" : "Fail");
	return STOP;
}

class FragDepthWriteCase : public TestCase
{
public:
	FragDepthWriteCase(Context& context, const char* name, const char* desc, glu::GLSLVersion glslVersion,
					   const char* fragSrc, EvalFragDepthFunc evalFunc);
	~FragDepthWriteCase(void);

	IterateResult iterate(void);

private:
	glu::GLSLVersion  m_glslVersion;
	string			  m_fragSrc;
	EvalFragDepthFunc m_evalFunc;
};

FragDepthWriteCase::FragDepthWriteCase(Context& context, const char* name, const char* desc,
									   glu::GLSLVersion glslVersion, const char* fragSrc, EvalFragDepthFunc evalFunc)
	: TestCase(context, name, desc), m_glslVersion(glslVersion), m_fragSrc(fragSrc), m_evalFunc(evalFunc)
{
}

FragDepthWriteCase::~FragDepthWriteCase(void)
{
}

FragDepthWriteCase::IterateResult FragDepthWriteCase::iterate(void)
{
	TestLog&				 log = m_testCtx.getLog();
	const glw::Functions&	gl  = m_context.getRenderContext().getFunctions();
	de::Random				 rnd(deStringHash(getName()));
	const tcu::RenderTarget& renderTarget = m_context.getRenderContext().getRenderTarget();
	int						 viewportW	= de::min(128, renderTarget.getWidth());
	int						 viewportH	= de::min(128, renderTarget.getHeight());
	int						 viewportX	= rnd.getInt(0, renderTarget.getWidth() - viewportW);
	int						 viewportY	= rnd.getInt(0, renderTarget.getHeight() - viewportH);
	tcu::Surface			 renderedFrame(viewportW, viewportH);
	tcu::Surface			 referenceFrame(viewportW, viewportH);
	const int				 numDepthSteps = 16;
	const float				 depthStep	 = 1.0f / (float)(numDepthSteps - 1);

	if (renderTarget.getDepthBits() == 0)
		throw tcu::NotSupportedError("Depth buffer is required", "", __FILE__, __LINE__);

	gl.viewport(viewportX, viewportY, viewportW, viewportH);
	gl.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	gl.enable(GL_DEPTH_TEST);
	gl.depthFunc(GL_LESS);

	static const deUint16 quadIndices[] = { 0, 1, 2, 2, 1, 3 };

	// Render with given shader.
	{
		glu::ShaderProgram program(m_context.getRenderContext(),
								   glu::makeVtxFragSources(specializeVersion(s_vertexShaderSrc, m_glslVersion).c_str(),
														   specializeVersion(m_fragSrc, m_glslVersion).c_str()));
		log << program;

		if (!program.isOk())
			TCU_FAIL("Compile failed");

		const float coord[]	= { 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f };
		const float position[] = { -1.0f, -1.0f, +1.0f, 1.0f, -1.0f, +1.0f, 0.0f,  1.0f,
								   +1.0f, -1.0f, 0.0f,  1.0f, +1.0f, +1.0f, -1.0f, 1.0f };

		gl.useProgram(program.getProgram());
		gl.uniform4f(gl.getUniformLocation(program.getProgram(), "u_color"), 0.0f, 1.0f, 0.0f, 1.0f);

		// Setup default helper uniforms.
		setupDefaultUniforms(m_context.getRenderContext(), program.getProgram());

		{
			glu::VertexArrayBinding vertexArrays[] = { glu::va::Float("a_position", 4, 4, 0, &position[0]),
													   glu::va::Float("a_coord", 2, 4, 0, &coord[0]) };
			glu::draw(m_context.getRenderContext(), program.getProgram(), DE_LENGTH_OF_ARRAY(vertexArrays),
					  &vertexArrays[0], glu::pr::Triangles(DE_LENGTH_OF_ARRAY(quadIndices), &quadIndices[0]));
		}

		GLU_EXPECT_NO_ERROR(gl.getError(), "Draw test quad");
	}

	// Visualize by rendering full-screen quads with increasing depth and color.
	{
		glu::ShaderProgram program(
			m_context.getRenderContext(),
			glu::makeVtxFragSources(specializeVersion(s_vertexShaderSrc, m_glslVersion).c_str(),
									specializeVersion(s_defaultFragmentShaderSrc, m_glslVersion).c_str()));
		if (!program.isOk())
		{
			log << program;
			TCU_FAIL("Compile failed");
		}

		int posLoc   = gl.getAttribLocation(program.getProgram(), "a_position");
		int colorLoc = gl.getUniformLocation(program.getProgram(), "u_color");

		gl.useProgram(program.getProgram());
		gl.depthMask(GL_FALSE);

		for (int stepNdx = 0; stepNdx < numDepthSteps; stepNdx++)
		{
			float f		= (float)stepNdx * depthStep;
			float depth = f * 2.0f - 1.0f;
			Vec4  color = Vec4(f, f, f, 1.0f);

			const float position[] = { -1.0f, -1.0f, depth, 1.0f, -1.0f, +1.0f, depth, 1.0f,
									   +1.0f, -1.0f, depth, 1.0f, +1.0f, +1.0f, depth, 1.0f };
			glu::VertexArrayBinding posBinding = glu::va::Float(posLoc, 4, 4, 0, &position[0]);

			gl.uniform4fv(colorLoc, 1, color.getPtr());
			glu::draw(m_context.getRenderContext(), program.getProgram(), 1, &posBinding,
					  glu::pr::Triangles(DE_LENGTH_OF_ARRAY(quadIndices), &quadIndices[0]));
		}

		GLU_EXPECT_NO_ERROR(gl.getError(), "Visualization draw");
	}

	glu::readPixels(m_context.getRenderContext(), viewportX, viewportY, renderedFrame.getAccess());

	// Render reference.
	for (int y = 0; y < referenceFrame.getHeight(); y++)
	{
		for (int x = 0; x < referenceFrame.getWidth(); x++)
		{
			float xf   = ((float)x + 0.5f) / (float)referenceFrame.getWidth();
			float yf   = ((float)y + 0.5f) / (float)referenceFrame.getHeight();
			float d	= m_evalFunc(Vec2(xf, yf));
			int   step = (int)deFloatFloor(d / depthStep);
			int   col  = de::clamp(deRoundFloatToInt32((float)step * depthStep * 255.0f), 0, 255);

			referenceFrame.setPixel(x, y, tcu::RGBA(col, col, col, 0xff));
		}
	}

	bool isOk = tcu::fuzzyCompare(log, "Result", "Image comparison result", referenceFrame, renderedFrame, 0.05f,
								  tcu::COMPARE_LOG_RESULT);
	m_testCtx.setTestResult(isOk ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL, isOk ? "Pass" : "Fail");
	return STOP;
}

FragDepthTests::FragDepthTests(Context& context, glu::GLSLVersion glslVersion)
	: TestCaseGroup(context, "fragdepth", "gl_FragDepth tests"), m_glslVersion(glslVersion)
{
}

FragDepthTests::~FragDepthTests(void)
{
}

static float evalConstDepth(const Vec2& coord)
{
	DE_UNREF(coord);
	return 0.5f;
}
static float evalDynamicDepth(const Vec2& coord)
{
	return (coord.x() + coord.y()) * 0.5f;
}
static float evalNoWrite(const Vec2& coord)
{
	return 1.0f - (coord.x() + coord.y()) * 0.5f;
}

static float evalDynamicConditionalDepth(const Vec2& coord)
{
	float d = (coord.x() + coord.y()) * 0.5f;
	if (coord.y() < 0.5f)
		return d;
	else
		return 1.0f - d;
}

void FragDepthTests::init(void)
{
	static const struct
	{
		const char*		  name;
		const char*		  desc;
		EvalFragDepthFunc evalFunc;
		const char*		  fragSrc;
	} cases[] = {
		{ "no_write", "No gl_FragDepth write", evalNoWrite, "${VERSION_DECL}\n"
															"uniform highp vec4 u_color;\n"
															"layout(location = 0) out mediump vec4 o_color;\n"
															"void main (void)\n"
															"{\n"
															"   o_color = u_color;\n"
															"}\n" },
		{ "const", "Const depth write", evalConstDepth, "${VERSION_DECL}\n"
														"uniform highp vec4 u_color;\n"
														"layout(location = 0) out mediump vec4 o_color;\n"
														"void main (void)\n"
														"{\n"
														"   o_color = u_color;\n"
														"   gl_FragDepth = 0.5;\n"
														"}\n" },
		{ "uniform", "Uniform depth write", evalConstDepth, "${VERSION_DECL}\n"
															"uniform highp vec4 u_color;\n"
															"uniform highp float uf_half;\n"
															"layout(location = 0) out mediump vec4 o_color;\n"
															"void main (void)\n"
															"{\n"
															"   o_color = u_color;\n"
															"   gl_FragDepth = uf_half;\n"
															"}\n" },
		{ "dynamic", "Dynamic depth write", evalDynamicDepth, "${VERSION_DECL}\n"
															  "uniform highp vec4 u_color;\n"
															  "in highp vec2 v_coord;\n"
															  "layout(location = 0) out mediump vec4 o_color;\n"
															  "void main (void)\n"
															  "{\n"
															  "   o_color = u_color;\n"
															  "   gl_FragDepth = (v_coord.x+v_coord.y)*0.5;\n"
															  "}\n" },
		{ "fragcoord_z", "gl_FragDepth write from gl_FragCoord.z", evalNoWrite,
		  "${VERSION_DECL}\n"
		  "uniform highp vec4 u_color;\n"
		  "layout(location = 0) out mediump vec4 o_color;\n"
		  "void main (void)\n"
		  "{\n"
		  "   o_color = u_color;\n"
		  "   gl_FragDepth = gl_FragCoord.z;\n"
		  "}\n" },
		{ "uniform_conditional_write", "Uniform conditional write", evalDynamicDepth,
		  "${VERSION_DECL}\n"
		  "uniform highp vec4 u_color;\n"
		  "uniform bool ub_true;\n"
		  "in highp vec2 v_coord;\n"
		  "layout(location = 0) out mediump vec4 o_color;\n"
		  "void main (void)\n"
		  "{\n"
		  "   o_color = u_color;\n"
		  "   if (ub_true)\n"
		  "       gl_FragDepth = (v_coord.x+v_coord.y)*0.5;\n"
		  "}\n" },
		{ "dynamic_conditional_write", "Uniform conditional write", evalDynamicConditionalDepth,
		  "${VERSION_DECL}\n"
		  "uniform highp vec4 u_color;\n"
		  "uniform bool ub_true;\n"
		  "in highp vec2 v_coord;\n"
		  "layout(location = 0) out mediump vec4 o_color;\n"
		  "void main (void)\n"
		  "{\n"
		  "   o_color = u_color;\n"
		  "   mediump float d = (v_coord.x+v_coord.y)*0.5f;\n"
		  "   if (v_coord.y < 0.5)\n"
		  "       gl_FragDepth = d;\n"
		  "   else\n"
		  "       gl_FragDepth = 1.0 - d;\n"
		  "}\n" },
		{ "uniform_loop_write", "Uniform loop write", evalConstDepth, "${VERSION_DECL}\n"
																	  "uniform highp vec4 u_color;\n"
																	  "uniform int ui_two;\n"
																	  "uniform highp float uf_fourth;\n"
																	  "in highp vec2 v_coord;\n"
																	  "layout(location = 0) out mediump vec4 o_color;\n"
																	  "void main (void)\n"
																	  "{\n"
																	  "   o_color = u_color;\n"
																	  "   gl_FragDepth = 0.0;\n"
																	  "   for (int i = 0; i < ui_two; i++)\n"
																	  "       gl_FragDepth += uf_fourth;\n"
																	  "}\n" },
		{ "write_in_function", "Uniform loop write", evalDynamicDepth,
		  "${VERSION_DECL}\n"
		  "uniform highp vec4 u_color;\n"
		  "uniform highp float uf_half;\n"
		  "in highp vec2 v_coord;\n"
		  "layout(location = 0) out mediump vec4 o_color;\n"
		  "void myfunc (highp vec2 coord)\n"
		  "{\n"
		  "   gl_FragDepth = (coord.x+coord.y)*0.5;\n"
		  "}\n"
		  "void main (void)\n"
		  "{\n"
		  "   o_color = u_color;\n"
		  "   myfunc(v_coord);\n"
		  "}\n" }
	};

	// .write
	tcu::TestCaseGroup* writeGroup = new tcu::TestCaseGroup(m_testCtx, "write", "gl_FragDepth write tests");
	addChild(writeGroup);
	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(cases); ndx++)
		writeGroup->addChild(new FragDepthWriteCase(m_context, cases[ndx].name, cases[ndx].desc, m_glslVersion,
													cases[ndx].fragSrc, cases[ndx].evalFunc));

	// .compare
	tcu::TestCaseGroup* compareGroup =
		new tcu::TestCaseGroup(m_testCtx, "compare", "gl_FragDepth used with depth comparison");
	addChild(compareGroup);
	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(cases); ndx++)
		compareGroup->addChild(new FragDepthCompareCase(m_context, cases[ndx].name, cases[ndx].desc, m_glslVersion,
														cases[ndx].fragSrc, cases[ndx].evalFunc, GL_LESS));
}

} // deqp
