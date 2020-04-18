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
 * \brief gl_HelperInvocation tests.
 *//*--------------------------------------------------------------------*/

#include "es31fShaderHelperInvocationTests.hpp"

#include "gluObjectWrapper.hpp"
#include "gluShaderProgram.hpp"
#include "gluDrawUtil.hpp"
#include "gluPixelTransfer.hpp"

#include "glwFunctions.hpp"
#include "glwEnums.hpp"

#include "tcuTestLog.hpp"
#include "tcuVector.hpp"
#include "tcuSurface.hpp"

#include "deUniquePtr.hpp"
#include "deStringUtil.hpp"
#include "deRandom.hpp"
#include "deString.h"

namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace
{

using glu::ShaderProgram;
using tcu::TestLog;
using tcu::Vec2;
using tcu::IVec2;
using de::MovePtr;
using std::string;
using std::vector;

enum PrimitiveType
{
	PRIMITIVETYPE_TRIANGLE = 0,
	PRIMITIVETYPE_LINE,
	PRIMITIVETYPE_WIDE_LINE,
	PRIMITIVETYPE_POINT,
	PRIMITIVETYPE_WIDE_POINT,

	PRIMITIVETYPE_LAST
};

static int getNumVerticesPerPrimitive (PrimitiveType primType)
{
	switch (primType)
	{
		case PRIMITIVETYPE_TRIANGLE:	return 3;
		case PRIMITIVETYPE_LINE:		return 2;
		case PRIMITIVETYPE_WIDE_LINE:	return 2;
		case PRIMITIVETYPE_POINT:		return 1;
		case PRIMITIVETYPE_WIDE_POINT:	return 1;
		default:
			DE_ASSERT(false);
			return 0;
	}
}

static glu::PrimitiveType getGluPrimitiveType (PrimitiveType primType)
{
	switch (primType)
	{
		case PRIMITIVETYPE_TRIANGLE:	return glu::PRIMITIVETYPE_TRIANGLES;
		case PRIMITIVETYPE_LINE:		return glu::PRIMITIVETYPE_LINES;
		case PRIMITIVETYPE_WIDE_LINE:	return glu::PRIMITIVETYPE_LINES;
		case PRIMITIVETYPE_POINT:		return glu::PRIMITIVETYPE_POINTS;
		case PRIMITIVETYPE_WIDE_POINT:	return glu::PRIMITIVETYPE_POINTS;
		default:
			DE_ASSERT(false);
			return glu::PRIMITIVETYPE_LAST;
	}
}

static void genVertices (PrimitiveType primType, int numPrimitives, de::Random* rnd, vector<Vec2>* dst)
{
	const bool	isTri					= primType == PRIMITIVETYPE_TRIANGLE;
	const float	minCoord				= isTri ? -1.5f : -1.0f;
	const float	maxCoord				= isTri ? +1.5f : +1.0f;
	const int	numVerticesPerPrimitive	= getNumVerticesPerPrimitive(primType);
	const int	numVert					= numVerticesPerPrimitive*numPrimitives;

	dst->resize(numVert);

	for (size_t ndx = 0; ndx < dst->size(); ndx++)
	{
		(*dst)[ndx][0] = rnd->getFloat(minCoord, maxCoord);
		(*dst)[ndx][1] = rnd->getFloat(minCoord, maxCoord);
	}

	// Don't produce completely or almost completely discardable primitives.
	// \note: This doesn't guarantee that resulting primitives are visible or
	//        produce any fragments. This just removes trivially discardable
	//        primitives.
	for (int primitiveNdx = 0; primitiveNdx < numPrimitives; ++primitiveNdx)
	for (int component = 0; component < 2; ++component)
	{
		bool negativeClip = true;
		bool positiveClip = true;

		for (int vertexNdx = 0; vertexNdx < numVerticesPerPrimitive; ++vertexNdx)
		{
			const float p = (*dst)[primitiveNdx * numVerticesPerPrimitive + vertexNdx][component];
			// \note 0.9 instead of 1.0 to avoid just barely visible primitives
			if (p > -0.9f)
				negativeClip = false;
			if (p < +0.9f)
				positiveClip = false;
		}

		// if discardable, just mirror first vertex along center
		if (negativeClip || positiveClip)
		{
			(*dst)[primitiveNdx * numVerticesPerPrimitive + 0][0] *= -1.0f;
			(*dst)[primitiveNdx * numVerticesPerPrimitive + 0][1] *= -1.0f;
		}
	}
}

static int getInteger (const glw::Functions& gl, deUint32 pname)
{
	int v = 0;
	gl.getIntegerv(pname, &v);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv()");
	return v;
}

static Vec2 getRange (const glw::Functions& gl, deUint32 pname)
{
	Vec2 v(0.0f);
	gl.getFloatv(pname, v.getPtr());
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetFloatv()");
	return v;
}

static void drawRandomPrimitives (const glu::RenderContext& renderCtx, deUint32 program, PrimitiveType primType, int numPrimitives, de::Random* rnd)
{
	const glw::Functions&			gl				= renderCtx.getFunctions();
	const float						minPointSize	= 16.0f;
	const float						maxPointSize	= 32.0f;
	const float						minLineWidth	= 16.0f;
	const float						maxLineWidth	= 32.0f;
	vector<Vec2>					vertices;
	vector<glu::VertexArrayBinding>	vertexArrays;

	genVertices(primType, numPrimitives, rnd, &vertices);

	vertexArrays.push_back(glu::va::Float("a_position", 2, (int)vertices.size(), 0, (const float*)&vertices[0]));

	gl.useProgram(program);

	// Special state for certain primitives
	if (primType == PRIMITIVETYPE_POINT || primType == PRIMITIVETYPE_WIDE_POINT)
	{
		const Vec2		range			= getRange(gl, GL_ALIASED_POINT_SIZE_RANGE);
		const bool		isWidePoint		= primType == PRIMITIVETYPE_WIDE_POINT;
		const float		pointSize		= isWidePoint ? de::min(rnd->getFloat(minPointSize, maxPointSize), range.y()) : 1.0f;
		const int		pointSizeLoc	= gl.getUniformLocation(program, "u_pointSize");

		gl.uniform1f(pointSizeLoc, pointSize);
	}
	else if (primType == PRIMITIVETYPE_WIDE_LINE)
	{
		const Vec2		range			= getRange(gl, GL_ALIASED_LINE_WIDTH_RANGE);
		const float		lineWidth		= de::min(rnd->getFloat(minLineWidth, maxLineWidth), range.y());

		gl.lineWidth(lineWidth);
	}

	glu::draw(renderCtx, program, (int)vertexArrays.size(), &vertexArrays[0],
			  glu::PrimitiveList(getGluPrimitiveType(primType), (int)vertices.size()));
}

class FboHelper
{
public:
								FboHelper			(const glu::RenderContext& renderCtx, int width, int height, deUint32 format, int numSamples);
								~FboHelper			(void);

	void						bindForRendering	(void);
	void						readPixels			(int x, int y, const tcu::PixelBufferAccess& dst);

private:
	const glu::RenderContext&	m_renderCtx;
	const int					m_numSamples;
	const IVec2					m_size;

	glu::Renderbuffer			m_colorbuffer;
	glu::Framebuffer			m_framebuffer;
	glu::Renderbuffer			m_resolveColorbuffer;
	glu::Framebuffer			m_resolveFramebuffer;
};

FboHelper::FboHelper (const glu::RenderContext& renderCtx, int width, int height, deUint32 format, int numSamples)
	: m_renderCtx			(renderCtx)
	, m_numSamples			(numSamples)
	, m_size				(width, height)
	, m_colorbuffer			(renderCtx)
	, m_framebuffer			(renderCtx)
	, m_resolveColorbuffer	(renderCtx)
	, m_resolveFramebuffer	(renderCtx)
{
	const glw::Functions&	gl			= m_renderCtx.getFunctions();
	const int				maxSamples	= getInteger(gl, GL_MAX_SAMPLES);

	gl.bindRenderbuffer(GL_RENDERBUFFER, *m_colorbuffer);
	gl.renderbufferStorageMultisample(GL_RENDERBUFFER, m_numSamples, format, width, height);
	gl.bindFramebuffer(GL_FRAMEBUFFER, *m_framebuffer);
	gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, *m_colorbuffer);

	if (m_numSamples > maxSamples && gl.checkFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		throw tcu::NotSupportedError("Sample count exceeds GL_MAX_SAMPLES");

	TCU_CHECK(gl.checkFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

	if (m_numSamples != 0)
	{
		gl.bindRenderbuffer(GL_RENDERBUFFER, *m_resolveColorbuffer);
		gl.renderbufferStorage(GL_RENDERBUFFER, format, width, height);
		gl.bindFramebuffer(GL_FRAMEBUFFER, *m_resolveFramebuffer);
		gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, *m_resolveColorbuffer);
		TCU_CHECK(gl.checkFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to create framebuffer");
}

FboHelper::~FboHelper (void)
{
}

void FboHelper::bindForRendering (void)
{
	const glw::Functions& gl = m_renderCtx.getFunctions();
	gl.bindFramebuffer(GL_FRAMEBUFFER, *m_framebuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer()");
	gl.viewport(0, 0, m_size.x(), m_size.y());
	GLU_EXPECT_NO_ERROR(gl.getError(), "viewport()");
}

void FboHelper::readPixels (int x, int y, const tcu::PixelBufferAccess& dst)
{
	const glw::Functions&	gl		= m_renderCtx.getFunctions();
	const int				width	= dst.getWidth();
	const int				height	= dst.getHeight();

	if (m_numSamples != 0)
	{
		gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, *m_resolveFramebuffer);
		gl.blitFramebuffer(x, y, width, height, x, y, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		gl.bindFramebuffer(GL_READ_FRAMEBUFFER, *m_resolveFramebuffer);
	}

	glu::readPixels(m_renderCtx, x, y, dst);
}

enum
{
	FRAMEBUFFER_WIDTH	= 256,
	FRAMEBUFFER_HEIGHT	= 256,
	FRAMEBUFFER_FORMAT	= GL_RGBA8,
	NUM_SAMPLES_MAX		= -1
};

//! Verifies that gl_HelperInvocation is false in all rendered pixels.
class HelperInvocationValueCase : public TestCase
{
public:
							HelperInvocationValueCase	(Context& context, const char* name, const char* description, PrimitiveType primType, int numSamples);
							~HelperInvocationValueCase	(void);

	void					init						(void);
	void					deinit						(void);
	IterateResult			iterate						(void);

private:
	const PrimitiveType		m_primitiveType;
	const int				m_numSamples;

	const int				m_numIters;
	const int				m_numPrimitivesPerIter;

	MovePtr<ShaderProgram>	m_program;
	MovePtr<FboHelper>		m_fbo;
	int						m_iterNdx;
};

HelperInvocationValueCase::HelperInvocationValueCase (Context& context, const char* name, const char* description, PrimitiveType primType, int numSamples)
	: TestCase					(context, name, description)
	, m_primitiveType			(primType)
	, m_numSamples				(numSamples)
	, m_numIters				(5)
	, m_numPrimitivesPerIter	(10)
	, m_iterNdx					(0)
{
}

HelperInvocationValueCase::~HelperInvocationValueCase (void)
{
	deinit();
}

void HelperInvocationValueCase::init (void)
{
	const glu::RenderContext&	renderCtx		= m_context.getRenderContext();
	const glw::Functions&		gl				= renderCtx.getFunctions();
	const int					maxSamples		= getInteger(gl, GL_MAX_SAMPLES);
	const int					actualSamples	= m_numSamples == NUM_SAMPLES_MAX ? maxSamples : m_numSamples;

	m_program = MovePtr<ShaderProgram>(new ShaderProgram(m_context.getRenderContext(),
		glu::ProgramSources()
			<< glu::VertexSource(
				"#version 310 es\n"
				"in highp vec2 a_position;\n"
				"uniform highp float u_pointSize;\n"
				"void main (void)\n"
				"{\n"
				"	gl_Position = vec4(a_position, 0.0, 1.0);\n"
				"	gl_PointSize = u_pointSize;\n"
				"}\n")
			<< glu::FragmentSource(
				"#version 310 es\n"
				"out mediump vec4 o_color;\n"
				"void main (void)\n"
				"{\n"
				"	if (gl_HelperInvocation)\n"
				"		o_color = vec4(1.0, 0.0, 0.0, 1.0);\n"
				"	else\n"
				"		o_color = vec4(0.0, 1.0, 0.0, 1.0);\n"
				"}\n")));

	m_testCtx.getLog() << *m_program;

	if (!m_program->isOk())
	{
		m_program.clear();
		TCU_FAIL("Compile failed");
	}

	m_testCtx.getLog() << TestLog::Message << "Using GL_RGBA8 framebuffer with "
					   << actualSamples << " samples" << TestLog::EndMessage;

	m_fbo = MovePtr<FboHelper>(new FboHelper(renderCtx, FRAMEBUFFER_WIDTH, FRAMEBUFFER_HEIGHT,
											 FRAMEBUFFER_FORMAT, actualSamples));

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
}

void HelperInvocationValueCase::deinit (void)
{
	m_program.clear();
	m_fbo.clear();
}

static bool verifyHelperInvocationValue (TestLog& log, const tcu::Surface& result, bool isMultiSample)
{
	const tcu::RGBA		bgRef				(0, 0, 0, 255);
	const tcu::RGBA		fgRef				(0, 255, 0, 255);
	const tcu::RGBA		threshold			(1, isMultiSample ? 254 : 1, 1, 1);
	int					numInvalidPixels	= 0;
	bool				renderedSomething	= false;

	for (int y = 0; y < result.getHeight(); ++y)
	{
		for (int x = 0; x < result.getWidth(); ++x)
		{
			const tcu::RGBA	resPix	= result.getPixel(x, y);
			const bool		isBg	= tcu::compareThreshold(resPix, bgRef, threshold);
			const bool		isFg	= tcu::compareThreshold(resPix, fgRef, threshold);

			if (!isBg && !isFg)
				numInvalidPixels += 1;

			if (isFg)
				renderedSomething = true;
		}
	}

	if (numInvalidPixels > 0)
	{
		log << TestLog::Image("Result", "Result image", result);
		log << TestLog::Message << "ERROR: Found " << numInvalidPixels << " invalid result pixels!" << TestLog::EndMessage;
		return false;
	}
	else if (!renderedSomething)
	{
		log << TestLog::Image("Result", "Result image", result);
		log << TestLog::Message << "ERROR: Result image was empty!" << TestLog::EndMessage;
		return false;
	}
	else
	{
		log << TestLog::Message << "All result pixels are valid" << TestLog::EndMessage;
		return true;
	}
}

HelperInvocationValueCase::IterateResult HelperInvocationValueCase::iterate (void)
{
	const glu::RenderContext&		renderCtx	= m_context.getRenderContext();
	const glw::Functions&			gl			= renderCtx.getFunctions();
	const string					sectionName	= string("Iteration ") + de::toString(m_iterNdx+1) + " / " + de::toString(m_numIters);
	const tcu::ScopedLogSection		section		(m_testCtx.getLog(), (string("Iter") + de::toString(m_iterNdx)), sectionName);
	de::Random						rnd			(deStringHash(getName()) ^ deInt32Hash(m_iterNdx));
	tcu::Surface					result		(FRAMEBUFFER_WIDTH, FRAMEBUFFER_HEIGHT);

	m_fbo->bindForRendering();
	gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
	gl.clear(GL_COLOR_BUFFER_BIT);

	drawRandomPrimitives(renderCtx, m_program->getProgram(), m_primitiveType, m_numPrimitivesPerIter, &rnd);

	m_fbo->readPixels(0, 0, result.getAccess());

	if (!verifyHelperInvocationValue(m_testCtx.getLog(), result, m_numSamples != 0))
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid pixels found");

	m_iterNdx += 1;
	return (m_iterNdx < m_numIters) ? CONTINUE : STOP;
}

//! Checks derivates when value depends on gl_HelperInvocation.
class HelperInvocationDerivateCase : public TestCase
{
public:
							HelperInvocationDerivateCase	(Context& context, const char* name, const char* description, PrimitiveType primType, int numSamples, const char* derivateFunc, bool checkAbsoluteValue);
							~HelperInvocationDerivateCase	(void);

	void					init							(void);
	void					deinit							(void);
	IterateResult			iterate							(void);

private:
	const PrimitiveType		m_primitiveType;
	const int				m_numSamples;
	const std::string		m_derivateFunc;
	const bool				m_checkAbsoluteValue;

	const int				m_numIters;

	MovePtr<ShaderProgram>	m_program;
	MovePtr<FboHelper>		m_fbo;
	int						m_iterNdx;
};

HelperInvocationDerivateCase::HelperInvocationDerivateCase (Context& context, const char* name, const char* description, PrimitiveType primType, int numSamples, const char* derivateFunc, bool checkAbsoluteValue)
	: TestCase					(context, name, description)
	, m_primitiveType			(primType)
	, m_numSamples				(numSamples)
	, m_derivateFunc			(derivateFunc)
	, m_checkAbsoluteValue		(checkAbsoluteValue)
	, m_numIters				(16)
	, m_iterNdx					(0)
{
}

HelperInvocationDerivateCase::~HelperInvocationDerivateCase (void)
{
	deinit();
}

void HelperInvocationDerivateCase::init (void)
{
	const glu::RenderContext&	renderCtx		= m_context.getRenderContext();
	const glw::Functions&		gl				= renderCtx.getFunctions();
	const int					maxSamples		= getInteger(gl, GL_MAX_SAMPLES);
	const int					actualSamples	= m_numSamples == NUM_SAMPLES_MAX ? maxSamples : m_numSamples;
	const std::string			funcSource		= (m_checkAbsoluteValue) ? ("abs(" + m_derivateFunc + "(value))") : (m_derivateFunc + "(value)");

	m_program = MovePtr<ShaderProgram>(new ShaderProgram(m_context.getRenderContext(),
		glu::ProgramSources()
			<< glu::VertexSource(
				"#version 310 es\n"
				"in highp vec2 a_position;\n"
				"uniform highp float u_pointSize;\n"
				"void main (void)\n"
				"{\n"
				"	gl_Position = vec4(a_position, 0.0, 1.0);\n"
				"	gl_PointSize = u_pointSize;\n"
				"}\n")
			<< glu::FragmentSource(string(
				"#version 310 es\n"
				"out mediump vec4 o_color;\n"
				"void main (void)\n"
				"{\n"
				"	highp float value		= gl_HelperInvocation ? 1.0 : 0.0;\n"
				"	highp float derivate	= ") + funcSource + ";\n"
				"	if (gl_HelperInvocation)\n"
				"		o_color = vec4(1.0, 0.0, derivate, 1.0);\n"
				"	else\n"
				"		o_color = vec4(0.0, 1.0, derivate, 1.0);\n"
				"}\n")));

	m_testCtx.getLog() << *m_program;

	if (!m_program->isOk())
	{
		m_program.clear();
		TCU_FAIL("Compile failed");
	}

	m_testCtx.getLog() << TestLog::Message << "Using GL_RGBA8 framebuffer with "
					   << actualSamples << " samples" << TestLog::EndMessage;

	m_fbo = MovePtr<FboHelper>(new FboHelper(renderCtx, FRAMEBUFFER_WIDTH, FRAMEBUFFER_HEIGHT,
											 FRAMEBUFFER_FORMAT, actualSamples));

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
}

void HelperInvocationDerivateCase::deinit (void)
{
	m_program.clear();
	m_fbo.clear();
}

static bool hasNeighborWithColor (const tcu::Surface& surface, int x, int y, tcu::RGBA color, tcu::RGBA threshold)
{
	const int	w	= surface.getWidth();
	const int	h	= surface.getHeight();

	for (int dx = -1; dx < 2; dx++)
	for (int dy = -1; dy < 2; dy++)
	{
		const IVec2	pos	= IVec2(x + dx, y + dy);

		if (dx == 0 && dy == 0)
			continue;

		if (de::inBounds(pos.x(), 0, w) && de::inBounds(pos.y(), 0, h))
		{
			const tcu::RGBA neighborColor = surface.getPixel(pos.x(), pos.y());

			if (tcu::compareThreshold(color, neighborColor, threshold))
				return true;
		}
		else
			return true; // Can't know for certain
	}

	return false;
}

static bool verifyHelperInvocationDerivate (TestLog& log, const tcu::Surface& result, bool isMultiSample)
{
	const tcu::RGBA		bgRef				(0, 0, 0, 255);
	const tcu::RGBA		fgRef				(0, 255, 0, 255);
	const tcu::RGBA		isBgThreshold		(1, isMultiSample ? 254 : 1, 0, 1);
	const tcu::RGBA		isFgThreshold		(1, isMultiSample ? 254 : 1, 255, 1);
	int					numInvalidPixels	= 0;
	int					numNonZeroDeriv		= 0;
	bool				renderedSomething	= false;

	for (int y = 0; y < result.getHeight(); ++y)
	{
		for (int x = 0; x < result.getWidth(); ++x)
		{
			const tcu::RGBA	resPix			= result.getPixel(x, y);
			const bool		isBg			= tcu::compareThreshold(resPix, bgRef, isBgThreshold);
			const bool		isFg			= tcu::compareThreshold(resPix, fgRef, isFgThreshold);
			const bool		nonZeroDeriv	= resPix.getBlue() > 0;
			const bool		neighborBg		= nonZeroDeriv ? hasNeighborWithColor(result, x, y, bgRef, isBgThreshold) : false;

			if (nonZeroDeriv)
				numNonZeroDeriv	+= 1;

			if ((!isBg && !isFg) ||							// Neither of valid colors (ignoring blue channel that has derivate)
				(nonZeroDeriv && !neighborBg && !isFg))		// Has non-zero derivate, but sample not at primitive edge or inside primitive
				numInvalidPixels += 1;

			if (isFg)
				renderedSomething = true;
		}
	}

	log << TestLog::Message << "Found " << numNonZeroDeriv << " pixels with non-zero derivate (neighbor sample has gl_HelperInvocation = true)" << TestLog::EndMessage;

	if (numInvalidPixels > 0)
	{
		log << TestLog::Image("Result", "Result image", result);
		log << TestLog::Message << "ERROR: Found " << numInvalidPixels << " invalid result pixels!" << TestLog::EndMessage;
		return false;
	}
	else if (!renderedSomething)
	{
		log << TestLog::Image("Result", "Result image", result);
		log << TestLog::Message << "ERROR: Result image was empty!" << TestLog::EndMessage;
		return false;
	}
	else
	{
		log << TestLog::Message << "All result pixels are valid" << TestLog::EndMessage;
		return true;
	}
}

HelperInvocationDerivateCase::IterateResult HelperInvocationDerivateCase::iterate (void)
{
	const glu::RenderContext&		renderCtx	= m_context.getRenderContext();
	const glw::Functions&			gl			= renderCtx.getFunctions();
	const string					sectionName	= string("Iteration ") + de::toString(m_iterNdx+1) + " / " + de::toString(m_numIters);
	const tcu::ScopedLogSection		section		(m_testCtx.getLog(), (string("Iter") + de::toString(m_iterNdx)), sectionName);
	de::Random						rnd			(deStringHash(getName()) ^ deInt32Hash(m_iterNdx));
	tcu::Surface					result		(FRAMEBUFFER_WIDTH, FRAMEBUFFER_HEIGHT);

	m_fbo->bindForRendering();
	gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
	gl.clear(GL_COLOR_BUFFER_BIT);

	drawRandomPrimitives(renderCtx, m_program->getProgram(), m_primitiveType, 1, &rnd);

	m_fbo->readPixels(0, 0, result.getAccess());

	if (!verifyHelperInvocationDerivate(m_testCtx.getLog(), result, m_numSamples != 0))
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid pixels found");

	m_iterNdx += 1;
	return (m_iterNdx < m_numIters) ? CONTINUE : STOP;
}

} // anonymous

ShaderHelperInvocationTests::ShaderHelperInvocationTests (Context& context)
	: TestCaseGroup(context, "helper_invocation", "gl_HelperInvocation tests")
{
}

ShaderHelperInvocationTests::~ShaderHelperInvocationTests (void)
{
}

void ShaderHelperInvocationTests::init (void)
{
	static const struct
	{
		const char*		caseName;
		PrimitiveType	primType;
	} s_primTypes[] =
	{
		{ "triangles",		PRIMITIVETYPE_TRIANGLE		},
		{ "lines",			PRIMITIVETYPE_LINE			},
		{ "wide_lines",		PRIMITIVETYPE_WIDE_LINE		},
		{ "points",			PRIMITIVETYPE_POINT			},
		{ "wide_points",	PRIMITIVETYPE_WIDE_POINT	}
	};

	static const struct
	{
		const char*		suffix;
		int				numSamples;
	} s_sampleCounts[] =
	{
		{ "",					0				},
		{ "_4_samples",			4				},
		{ "_8_samples",			8				},
		{ "_max_samples",		NUM_SAMPLES_MAX	}
	};

	// value
	{
		tcu::TestCaseGroup* const valueGroup = new tcu::TestCaseGroup(m_testCtx, "value", "gl_HelperInvocation value in rendered pixels");
		addChild(valueGroup);

		for (int sampleCountNdx = 0; sampleCountNdx < DE_LENGTH_OF_ARRAY(s_sampleCounts); sampleCountNdx++)
		{
			for (int primTypeNdx = 0; primTypeNdx < DE_LENGTH_OF_ARRAY(s_primTypes); primTypeNdx++)
			{
				const string		name		= string(s_primTypes[primTypeNdx].caseName) + s_sampleCounts[sampleCountNdx].suffix;
				const PrimitiveType	primType	= s_primTypes[primTypeNdx].primType;
				const int			numSamples	= s_sampleCounts[sampleCountNdx].numSamples;

				valueGroup->addChild(new HelperInvocationValueCase(m_context, name.c_str(), "", primType, numSamples));
			}
		}
	}

	// derivate
	{
		tcu::TestCaseGroup* const derivateGroup = new tcu::TestCaseGroup(m_testCtx, "derivate", "Derivate of gl_HelperInvocation-dependent value");
		addChild(derivateGroup);

		for (int sampleCountNdx = 0; sampleCountNdx < DE_LENGTH_OF_ARRAY(s_sampleCounts); sampleCountNdx++)
		{
			for (int primTypeNdx = 0; primTypeNdx < DE_LENGTH_OF_ARRAY(s_primTypes); primTypeNdx++)
			{
				const string		name		= string(s_primTypes[primTypeNdx].caseName) + s_sampleCounts[sampleCountNdx].suffix;
				const PrimitiveType	primType	= s_primTypes[primTypeNdx].primType;
				const int			numSamples	= s_sampleCounts[sampleCountNdx].numSamples;

				derivateGroup->addChild(new HelperInvocationDerivateCase(m_context, (name + "_dfdx").c_str(),	"", primType, numSamples, "dFdx",	true));
				derivateGroup->addChild(new HelperInvocationDerivateCase(m_context, (name + "_dfdy").c_str(),	"", primType, numSamples, "dFdy",	true));
				derivateGroup->addChild(new HelperInvocationDerivateCase(m_context, (name + "_fwidth").c_str(),	"", primType, numSamples, "fwidth",	false));
			}
		}
	}
}

} // Functional
} // gles31
} // deqp
