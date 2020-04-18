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
 * \brief Multisample tests
 *//*--------------------------------------------------------------------*/

#include "es31fMultisampleTests.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuVector.hpp"
#include "tcuSurface.hpp"
#include "tcuImageCompare.hpp"
#include "tcuStringTemplate.hpp"
#include "gluPixelTransfer.hpp"
#include "gluRenderContext.hpp"
#include "gluCallLogWrapper.hpp"
#include "gluObjectWrapper.hpp"
#include "gluShaderProgram.hpp"
#include "glwFunctions.hpp"
#include "glwEnums.hpp"
#include "deRandom.hpp"
#include "deStringUtil.hpp"
#include "deString.h"
#include "deMath.h"

using namespace glw;

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

using std::map;
using std::string;

static std::string sampleMaskToString (const std::vector<deUint32>& bitfield, int numBits)
{
	std::string result(numBits, '0');

	// move from back to front and set chars to 1
	for (int wordNdx = 0; wordNdx < (int)bitfield.size(); ++wordNdx)
	{
		for (int bit = 0; bit < 32; ++bit)
		{
			const int targetCharNdx = numBits - (wordNdx*32+bit) - 1;

			// beginning of the string reached
			if (targetCharNdx < 0)
				return result;

			if ((bitfield[wordNdx] >> bit) & 0x01)
				result[targetCharNdx] = '1';
		}
	}

	return result;
}

/*--------------------------------------------------------------------*//*!
 * \brief Returns the number of words needed to represent mask of given length
 *//*--------------------------------------------------------------------*/
static int getEffectiveSampleMaskWordCount (int highestBitNdx)
{
	const int wordSize	= 32;
	const int maskLen	= highestBitNdx + 1;

	return ((maskLen - 1) / wordSize) + 1; // round_up(mask_len /  wordSize)
}

/*--------------------------------------------------------------------*//*!
 * \brief Creates sample mask with all less significant bits than nthBit set
 *//*--------------------------------------------------------------------*/
static std::vector<deUint32> genAllSetToNthBitSampleMask (int nthBit)
{
	const int				wordSize	= 32;
	const int				numWords	= getEffectiveSampleMaskWordCount(nthBit - 1);
	const deUint32			topWordBits	= (deUint32)(nthBit - (numWords - 1) * wordSize);
	std::vector<deUint32>	mask		(numWords);

	for (int ndx = 0; ndx < numWords - 1; ++ndx)
		mask[ndx] = 0xFFFFFFFF;

	mask[numWords - 1] = deBitMask32(0, (int)topWordBits);
	return mask;
}

class SamplePosQueryCase : public TestCase
{
public:
					SamplePosQueryCase (Context& context, const char* name, const char* desc);
private:
	void			init				(void);
	IterateResult	iterate				(void);
};

SamplePosQueryCase::SamplePosQueryCase (Context& context, const char* name, const char* desc)
	: TestCase(context, name, desc)
{
}

void SamplePosQueryCase::init (void)
{
	if (m_context.getRenderTarget().getNumSamples() == 0)
		throw tcu::NotSupportedError("No multisample buffers");
}

SamplePosQueryCase::IterateResult SamplePosQueryCase::iterate (void)
{
	glu::CallLogWrapper gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	bool				error	= false;

	gl.enableLogging(true);

	for (int ndx = 0; ndx < m_context.getRenderTarget().getNumSamples(); ++ndx)
	{
		tcu::Vec2 samplePos = tcu::Vec2(-1, -1);

		gl.glGetMultisamplefv(GL_SAMPLE_POSITION, ndx, samplePos.getPtr());
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "getMultisamplefv");

		// check value range
		if (samplePos.x() < 0.0f || samplePos.x() > 1.0f ||
			samplePos.y() < 0.0f || samplePos.y() > 1.0f)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Sample " << ndx << " is not in valid range [0,1], got " << samplePos << tcu::TestLog::EndMessage;
			error = true;
		}
	}

	if (!error)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid sample pos");

	return STOP;
}

/*--------------------------------------------------------------------*//*!
 * \brief Abstract base class handling common stuff for default fbo multisample cases.
 *//*--------------------------------------------------------------------*/
class DefaultFBOMultisampleCase : public TestCase
{
public:
								DefaultFBOMultisampleCase	(Context& context, const char* name, const char* desc, int desiredViewportSize);
	virtual						~DefaultFBOMultisampleCase	(void);

	virtual void				init						(void);
	virtual void				deinit						(void);

protected:
	void						renderTriangle				(const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec4& c0, const Vec4& c1, const Vec4& c2) const;
	void						renderTriangle				(const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec4& c0, const Vec4& c1, const Vec4& c2) const;
	void						renderTriangle				(const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec4& color) const;
	void						renderQuad					(const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec2& p3, const Vec4& c0, const Vec4& c1, const Vec4& c2, const Vec4& c3) const;
	void						renderQuad					(const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec2& p3, const Vec4& color) const;

	void						randomizeViewport			(void);
	void						readImage					(tcu::Surface& dst) const;

	int							m_numSamples;

	int							m_viewportSize;

private:
								DefaultFBOMultisampleCase	(const DefaultFBOMultisampleCase& other);
	DefaultFBOMultisampleCase&	operator=					(const DefaultFBOMultisampleCase& other);

	const int					m_desiredViewportSize;

	glu::ShaderProgram*			m_program;
	int							m_attrPositionLoc;
	int							m_attrColorLoc;

	int							m_viewportX;
	int							m_viewportY;
	de::Random					m_rnd;

	bool						m_initCalled;
};

DefaultFBOMultisampleCase::DefaultFBOMultisampleCase (Context& context, const char* name, const char* desc, int desiredViewportSize)
	: TestCase				(context, name, desc)
	, m_numSamples			(0)
	, m_viewportSize		(0)
	, m_desiredViewportSize	(desiredViewportSize)
	, m_program				(DE_NULL)
	, m_attrPositionLoc		(-1)
	, m_attrColorLoc		(-1)
	, m_viewportX			(0)
	, m_viewportY			(0)
	, m_rnd					(deStringHash(name))
	, m_initCalled			(false)
{
}

DefaultFBOMultisampleCase::~DefaultFBOMultisampleCase (void)
{
	DefaultFBOMultisampleCase::deinit();
}

void DefaultFBOMultisampleCase::init (void)
{
	const bool					supportsES32 = glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));
	map<string, string>			args;
	args["GLSL_VERSION_DECL"] = supportsES32 ? getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES);

	static const char* vertShaderSource =
		"${GLSL_VERSION_DECL}\n"
		"in highp vec4 a_position;\n"
		"in mediump vec4 a_color;\n"
		"out mediump vec4 v_color;\n"
		"void main()\n"
		"{\n"
		"	gl_Position = a_position;\n"
		"	v_color = a_color;\n"
		"}\n";

	static const char* fragShaderSource =
		"${GLSL_VERSION_DECL}\n"
		"in mediump vec4 v_color;\n"
		"layout(location = 0) out mediump vec4 o_color;\n"
		"void main()\n"
		"{\n"
		"	o_color = v_color;\n"
		"}\n";

	TestLog&				log	= m_testCtx.getLog();
	const glw::Functions&	gl	= m_context.getRenderContext().getFunctions();

	if (m_context.getRenderTarget().getNumSamples() <= 1)
		throw tcu::NotSupportedError("No multisample buffers");

	m_initCalled = true;

	// Query and log number of samples per pixel.

	gl.getIntegerv(GL_SAMPLES, &m_numSamples);
	GLU_EXPECT_NO_ERROR(gl.getError(), "getIntegerv(GL_SAMPLES)");
	log << TestLog::Message << "GL_SAMPLES = " << m_numSamples << TestLog::EndMessage;

	// Prepare program.

	DE_ASSERT(!m_program);

	m_program = new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources()
		<< glu::VertexSource(tcu::StringTemplate(vertShaderSource).specialize(args))
		<< glu::FragmentSource(tcu::StringTemplate(fragShaderSource).specialize(args)));
	if (!m_program->isOk())
		throw tcu::TestError("Failed to compile program", DE_NULL, __FILE__, __LINE__);

	m_attrPositionLoc	= gl.getAttribLocation(m_program->getProgram(), "a_position");
	m_attrColorLoc		= gl.getAttribLocation(m_program->getProgram(), "a_color");
	GLU_EXPECT_NO_ERROR(gl.getError(), "getAttribLocation");

	if (m_attrPositionLoc < 0 || m_attrColorLoc < 0)
	{
		delete m_program;
		throw tcu::TestError("Invalid attribute locations", DE_NULL, __FILE__, __LINE__);
	}

	// Get suitable viewport size.

	m_viewportSize = de::min<int>(m_desiredViewportSize, de::min(m_context.getRenderTarget().getWidth(), m_context.getRenderTarget().getHeight()));
	randomizeViewport();
}

void DefaultFBOMultisampleCase::deinit (void)
{
	// Do not try to call GL functions during case list creation
	if (!m_initCalled)
		return;

	delete m_program;
	m_program = DE_NULL;
}

void DefaultFBOMultisampleCase::renderTriangle (const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec4& c0, const Vec4& c1, const Vec4& c2) const
{
	const float vertexPositions[] =
	{
		p0.x(), p0.y(), p0.z(), 1.0f,
		p1.x(), p1.y(), p1.z(), 1.0f,
		p2.x(), p2.y(), p2.z(), 1.0f
	};
	const float vertexColors[] =
	{
		c0.x(), c0.y(), c0.z(), c0.w(),
		c1.x(), c1.y(), c1.z(), c1.w(),
		c2.x(), c2.y(), c2.z(), c2.w(),
	};

	const glw::Functions&	gl		= m_context.getRenderContext().getFunctions();
	glu::Buffer				vtxBuf	(m_context.getRenderContext());
	glu::Buffer				colBuf	(m_context.getRenderContext());
	glu::VertexArray		vao		(m_context.getRenderContext());

	gl.bindVertexArray(*vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindVertexArray");

	gl.bindBuffer(GL_ARRAY_BUFFER, *vtxBuf);
	gl.bufferData(GL_ARRAY_BUFFER, sizeof(vertexPositions), &vertexPositions[0], GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "vtx buf");

	gl.enableVertexAttribArray(m_attrPositionLoc);
	gl.vertexAttribPointer(m_attrPositionLoc, 4, GL_FLOAT, false, 0, DE_NULL);
	GLU_EXPECT_NO_ERROR(gl.getError(), "vtx vertexAttribPointer");

	gl.bindBuffer(GL_ARRAY_BUFFER, *colBuf);
	gl.bufferData(GL_ARRAY_BUFFER, sizeof(vertexColors), &vertexColors[0], GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "col buf");

	gl.enableVertexAttribArray(m_attrColorLoc);
	gl.vertexAttribPointer(m_attrColorLoc, 4, GL_FLOAT, false, 0, DE_NULL);
	GLU_EXPECT_NO_ERROR(gl.getError(), "col vertexAttribPointer");

	gl.useProgram(m_program->getProgram());
	gl.drawArrays(GL_TRIANGLES, 0, 3);
	GLU_EXPECT_NO_ERROR(gl.getError(), "drawArrays");
}

void DefaultFBOMultisampleCase::renderTriangle (const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec4& c0, const Vec4& c1, const Vec4& c2) const
{
	renderTriangle(Vec3(p0.x(), p0.y(), 0.0f),
				   Vec3(p1.x(), p1.y(), 0.0f),
				   Vec3(p2.x(), p2.y(), 0.0f),
				   c0, c1, c2);
}

void DefaultFBOMultisampleCase::renderTriangle (const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec4& color) const
{
	renderTriangle(p0, p1, p2, color, color, color);
}

void DefaultFBOMultisampleCase::renderQuad (const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec2& p3, const Vec4& c0, const Vec4& c1, const Vec4& c2, const Vec4& c3) const
{
	renderTriangle(p0, p1, p2, c0, c1, c2);
	renderTriangle(p2, p1, p3, c2, c1, c3);
}

void DefaultFBOMultisampleCase::renderQuad (const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec2& p3, const Vec4& color) const
{
	renderQuad(p0, p1, p2, p3, color, color, color, color);
}

void DefaultFBOMultisampleCase::randomizeViewport (void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	m_viewportX = m_rnd.getInt(0, m_context.getRenderTarget().getWidth()  - m_viewportSize);
	m_viewportY = m_rnd.getInt(0, m_context.getRenderTarget().getHeight() - m_viewportSize);

	gl.viewport(m_viewportX, m_viewportY, m_viewportSize, m_viewportSize);
	GLU_EXPECT_NO_ERROR(gl.getError(), "viewport");
}

void DefaultFBOMultisampleCase::readImage (tcu::Surface& dst) const
{
	glu::readPixels(m_context.getRenderContext(), m_viewportX, m_viewportY, dst.getAccess());
}

/*--------------------------------------------------------------------*//*!
 * \brief Tests coverage mask inversion validity.
 *
 * Tests that the coverage masks obtained by masks set with glSampleMaski(mask)
 * and glSampleMaski(~mask) are indeed each others' inverses.
 *
 * This is done by drawing a pattern, with varying coverage values,
 * overlapped by a pattern that has inverted masks and is otherwise
 * identical. The resulting image is compared to one obtained by drawing
 * the same pattern but with all-ones coverage masks.
 *//*--------------------------------------------------------------------*/
class MaskInvertCase : public DefaultFBOMultisampleCase
{
public:
					MaskInvertCase				(Context& context, const char* name, const char* description);
					~MaskInvertCase				(void) {}

	void			init						(void);
	IterateResult	iterate						(void);

private:
	void			drawPattern					(bool invert) const;
};

MaskInvertCase::MaskInvertCase (Context& context, const char* name, const char* description)
	: DefaultFBOMultisampleCase	(context, name, description, 256)
{
}

void MaskInvertCase::init (void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	// check the test is even possible

	GLint maxSampleMaskWords = 0;
	gl.getIntegerv(GL_MAX_SAMPLE_MASK_WORDS, &maxSampleMaskWords);
	if (getEffectiveSampleMaskWordCount(m_numSamples - 1) > maxSampleMaskWords)
		throw tcu::NotSupportedError("Test requires larger GL_MAX_SAMPLE_MASK_WORDS");

	// normal init
	DefaultFBOMultisampleCase::init();
}

MaskInvertCase::IterateResult MaskInvertCase::iterate (void)
{
	const glw::Functions&	gl = m_context.getRenderContext().getFunctions();
	TestLog&				log								= m_testCtx.getLog();
	tcu::Surface			renderedImgNoSampleCoverage		(m_viewportSize, m_viewportSize);
	tcu::Surface			renderedImgSampleCoverage		(m_viewportSize, m_viewportSize);

	randomizeViewport();

	gl.enable(GL_BLEND);
	gl.blendEquation(GL_FUNC_ADD);
	gl.blendFunc(GL_ONE, GL_ONE);
	GLU_EXPECT_NO_ERROR(gl.getError(), "set blend");
	log << TestLog::Message << "Additive blending enabled in order to detect (erroneously) overlapping samples" << TestLog::EndMessage;

	log << TestLog::Message << "Clearing color to all-zeros" << TestLog::EndMessage;
	gl.clearColor(0.0f, 0.0f, 0.0f, 0.0f);
	gl.clear(GL_COLOR_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "clear");

	log << TestLog::Message << "Drawing the pattern with GL_SAMPLE_MASK disabled" << TestLog::EndMessage;
	drawPattern(false);
	readImage(renderedImgNoSampleCoverage);

	log << TestLog::Image("RenderedImageNoSampleMask", "Rendered image with GL_SAMPLE_MASK disabled", renderedImgNoSampleCoverage, QP_IMAGE_COMPRESSION_MODE_PNG);

	log << TestLog::Message << "Clearing color to all-zeros" << TestLog::EndMessage;
	gl.clear(GL_COLOR_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "clear");

	gl.enable(GL_SAMPLE_MASK);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable(GL_SAMPLE_MASK)");

	log << TestLog::Message << "Drawing the pattern with GL_SAMPLE_MASK enabled, using non-inverted sample masks" << TestLog::EndMessage;
	drawPattern(false);
	log << TestLog::Message << "Drawing the pattern with GL_SAMPLE_MASK enabled, using inverted sample masks" << TestLog::EndMessage;
	drawPattern(true);

	readImage(renderedImgSampleCoverage);

	log << TestLog::Image("RenderedImageSampleMask", "Rendered image with GL_SAMPLE_MASK enabled", renderedImgSampleCoverage, QP_IMAGE_COMPRESSION_MODE_PNG);

	bool passed = tcu::pixelThresholdCompare(log,
											 "CoverageVsNoCoverage",
											 "Comparison of same pattern with GL_SAMPLE_MASK disabled and enabled",
											 renderedImgNoSampleCoverage,
											 renderedImgSampleCoverage,
											 tcu::RGBA(0),
											 tcu::COMPARE_LOG_ON_ERROR);

	if (passed)
		log << TestLog::Message << "Success: The two images rendered are identical" << TestLog::EndMessage;

	m_context.getTestContext().setTestResult(passed ? QP_TEST_RESULT_PASS	: QP_TEST_RESULT_FAIL,
											 passed ? "Passed"				: "Failed");

	return STOP;
}

void MaskInvertCase::drawPattern (bool invert) const
{
	const int				numTriangles	= 25;
	const glw::Functions&	gl				= m_context.getRenderContext().getFunctions();

	for (int triNdx = 0; triNdx < numTriangles; triNdx++)
	{
		const float	angle0	= 2.0f*DE_PI * (float)triNdx			/ (float)numTriangles;
		const float	angle1	= 2.0f*DE_PI * ((float)triNdx + 0.5f)	/ (float)numTriangles;
		const Vec4	color	= Vec4(0.4f + (float)triNdx/(float)numTriangles*0.6f,
		                           0.5f + (float)triNdx/(float)numTriangles*0.3f,
		                           0.6f - (float)triNdx/(float)numTriangles*0.5f,
		                           0.7f - (float)triNdx/(float)numTriangles*0.7f);


		const int			wordCount		= getEffectiveSampleMaskWordCount(m_numSamples - 1);
		const GLbitfield	finalWordBits	= m_numSamples - 32 * ((m_numSamples-1) / 32);
		const GLbitfield	finalWordMask	= (GLbitfield)deBitMask32(0, (int)finalWordBits);

		for (int wordNdx = 0; wordNdx < wordCount; ++wordNdx)
		{
			const GLbitfield	rawMask		= (GLbitfield)deUint32Hash(wordNdx * 32 + triNdx);
			const GLbitfield	mask		= (invert) ? (~rawMask) : (rawMask);
			const bool			isFinalWord	= (wordNdx + 1) == wordCount;
			const GLbitfield	maskMask	= (isFinalWord) ? (finalWordMask) : (0xFFFFFFFFUL); // maskMask prevents setting coverage bits higher than sample count

			gl.sampleMaski(wordNdx, mask & maskMask);
		}
		GLU_EXPECT_NO_ERROR(gl.getError(), "glSampleMaski");

		renderTriangle(Vec2(0.0f, 0.0f),
					   Vec2(deFloatCos(angle0)*0.95f, deFloatSin(angle0)*0.95f),
					   Vec2(deFloatCos(angle1)*0.95f, deFloatSin(angle1)*0.95f),
					   color);
	}
}

/*--------------------------------------------------------------------*//*!
 * \brief Tests coverage mask generation proportionality property.
 *
 * Tests that the number of coverage bits in a coverage mask set with
 * glSampleMaski is, on average, proportional to the number of set bits.
 * Draws multiple frames, each time increasing the number of mask bits set
 * and checks that the average color is changing appropriately.
 *//*--------------------------------------------------------------------*/
class MaskProportionalityCase : public DefaultFBOMultisampleCase
{
public:
					MaskProportionalityCase				(Context& context, const char* name, const char* description);
					~MaskProportionalityCase			(void) {}

	void			init								(void);

	IterateResult	iterate								(void);

private:
	int				m_numIterations;
	int				m_currentIteration;

	deInt32			m_previousIterationColorSum;
};

MaskProportionalityCase::MaskProportionalityCase (Context& context, const char* name, const char* description)
	: DefaultFBOMultisampleCase		(context, name, description, 32)
	, m_numIterations				(-1)
	, m_currentIteration			(0)
	, m_previousIterationColorSum	(-1)
{
}

void MaskProportionalityCase::init (void)
{
	const glw::Functions&	gl	= m_context.getRenderContext().getFunctions();
	TestLog&				log	= m_testCtx.getLog();

	// check the test is even possible
	GLint maxSampleMaskWords = 0;
	gl.getIntegerv(GL_MAX_SAMPLE_MASK_WORDS, &maxSampleMaskWords);
	if (getEffectiveSampleMaskWordCount(m_numSamples - 1) > maxSampleMaskWords)
		throw tcu::NotSupportedError("Test requires larger GL_MAX_SAMPLE_MASK_WORDS");

	DefaultFBOMultisampleCase::init();

	// set state
	gl.enable(GL_SAMPLE_MASK);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable(GL_SAMPLE_MASK)");
	log << TestLog::Message << "GL_SAMPLE_MASK is enabled" << TestLog::EndMessage;

	m_numIterations = m_numSamples + 1;

	randomizeViewport(); // \note Using the same viewport for every iteration since coverage mask may depend on window-relative pixel coordinate.
}

MaskProportionalityCase::IterateResult MaskProportionalityCase::iterate (void)
{
	const glw::Functions&	gl				= m_context.getRenderContext().getFunctions();
	TestLog&				log				= m_testCtx.getLog();
	tcu::Surface			renderedImg		(m_viewportSize, m_viewportSize);
	deInt32					numPixels		= (deInt32)renderedImg.getWidth()*(deInt32)renderedImg.getHeight();

	DE_ASSERT(m_numIterations >= 0);

	log << TestLog::Message << "Clearing color to black" << TestLog::EndMessage;
	gl.colorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
	gl.clear(GL_COLOR_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "clear");

	// Draw quad.

	{
		const Vec2					pt0						(-1.0f, -1.0f);
		const Vec2					pt1						( 1.0f, -1.0f);
		const Vec2					pt2						(-1.0f,  1.0f);
		const Vec2					pt3						( 1.0f,  1.0f);
		Vec4						quadColor				(1.0f, 0.0f, 0.0f, 1.0f);
		const std::vector<deUint32>	sampleMask				= genAllSetToNthBitSampleMask(m_currentIteration);

		DE_ASSERT(m_currentIteration <= m_numSamples + 1);

		log << TestLog::Message << "Drawing a red quad using sample mask 0b" << sampleMaskToString(sampleMask, m_numSamples) << TestLog::EndMessage;

		for (int wordNdx = 0; wordNdx < getEffectiveSampleMaskWordCount(m_numSamples - 1); ++wordNdx)
		{
			const GLbitfield mask = (wordNdx < (int)sampleMask.size()) ? ((GLbitfield)(sampleMask[wordNdx])) : (0);

			gl.sampleMaski(wordNdx, mask);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glSampleMaski");
		}

		renderQuad(pt0, pt1, pt2, pt3, quadColor);
	}

	// Read ang log image.

	readImage(renderedImg);

	log << TestLog::Image("RenderedImage", "Rendered image", renderedImg, QP_IMAGE_COMPRESSION_MODE_PNG);

	// Compute average red component in rendered image.

	deInt32 sumRed = 0;

	for (int y = 0; y < renderedImg.getHeight(); y++)
	for (int x = 0; x < renderedImg.getWidth(); x++)
		sumRed += renderedImg.getPixel(x, y).getRed();

	log << TestLog::Message << "Average red color component: " << de::floatToString((float)sumRed / 255.0f / (float)numPixels, 2) << TestLog::EndMessage;

	// Check if average color has decreased from previous frame's color.

	if (sumRed < m_previousIterationColorSum)
	{
		log << TestLog::Message << "Failure: Current average red color component is lower than previous" << TestLog::EndMessage;
		m_context.getTestContext().setTestResult(QP_TEST_RESULT_FAIL, "Failed");
		return STOP;
	}

	// Check if coverage mask is not all-zeros if alpha or coverage value is 0 (or 1, if inverted).

	if (m_currentIteration == 0 && sumRed != 0)
	{
		log << TestLog::Message << "Failure: Image should be completely black" << TestLog::EndMessage;
		m_context.getTestContext().setTestResult(QP_TEST_RESULT_FAIL, "Failed");
		return STOP;
	}

	if (m_currentIteration == m_numIterations-1 && sumRed != 0xff*numPixels)
	{
		log << TestLog::Message << "Failure: Image should be completely red" << TestLog::EndMessage;

		m_context.getTestContext().setTestResult(QP_TEST_RESULT_FAIL, "Failed");
		return STOP;
	}

	m_previousIterationColorSum = sumRed;

	m_currentIteration++;

	if (m_currentIteration >= m_numIterations)
	{
		log << TestLog::Message << "Success: Number of coverage mask bits set appears to be, on average, proportional to the number of set sample mask bits" << TestLog::EndMessage;
		m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Passed");
		return STOP;
	}
	else
		return CONTINUE;
}

/*--------------------------------------------------------------------*//*!
 * \brief Tests coverage mask generation constancy property.
 *
 * Tests that the coverage mask created by GL_SAMPLE_MASK is constant at
 * given pixel coordinates. Draws two quads, with the second one fully
 * overlapping the first one such that at any given pixel, both quads have
 * the same coverage mask value. This way, if the constancy property is
 * fulfilled, only the second quad should be visible.
 *//*--------------------------------------------------------------------*/
class MaskConstancyCase : public DefaultFBOMultisampleCase
{
public:
	enum CaseBits
	{
		CASEBIT_ALPHA_TO_COVERAGE			= 1,	//!< Use alpha-to-coverage.
		CASEBIT_SAMPLE_COVERAGE				= 2,	//!< Use sample coverage.
		CASEBIT_SAMPLE_COVERAGE_INVERTED	= 4,	//!< Inverted sample coverage.
		CASEBIT_SAMPLE_MASK					= 8,	//!< Use sample mask.
	};

					MaskConstancyCase			(Context& context, const char* name, const char* description, deUint32 typeBits);
					~MaskConstancyCase			(void) {}

	void			init						(void);
	IterateResult	iterate						(void);

private:
	const bool		m_isAlphaToCoverageCase;
	const bool		m_isSampleCoverageCase;
	const bool		m_isInvertedSampleCoverageCase;
	const bool		m_isSampleMaskCase;
};

MaskConstancyCase::MaskConstancyCase (Context& context, const char* name, const char* description, deUint32 typeBits)
	: DefaultFBOMultisampleCase			(context, name, description, 256)
	, m_isAlphaToCoverageCase			(0 != (typeBits & CASEBIT_ALPHA_TO_COVERAGE))
	, m_isSampleCoverageCase			(0 != (typeBits & CASEBIT_SAMPLE_COVERAGE))
	, m_isInvertedSampleCoverageCase	(0 != (typeBits & CASEBIT_SAMPLE_COVERAGE_INVERTED))
	, m_isSampleMaskCase				(0 != (typeBits & CASEBIT_SAMPLE_MASK))
{
	// CASEBIT_SAMPLE_COVERAGE_INVERT => CASEBIT_SAMPLE_COVERAGE
	DE_ASSERT((typeBits & CASEBIT_SAMPLE_COVERAGE) || ~(typeBits & CASEBIT_SAMPLE_COVERAGE_INVERTED));
	DE_ASSERT(m_isSampleMaskCase); // no point testing non-sample-mask cases, they are checked already in gles3
}

void MaskConstancyCase::init (void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	// check the test is even possible
	if (m_isSampleMaskCase)
	{
		GLint maxSampleMaskWords = 0;
		gl.getIntegerv(GL_MAX_SAMPLE_MASK_WORDS, &maxSampleMaskWords);
		if (getEffectiveSampleMaskWordCount(m_numSamples - 1) > maxSampleMaskWords)
			throw tcu::NotSupportedError("Test requires larger GL_MAX_SAMPLE_MASK_WORDS");
	}

	// normal init
	DefaultFBOMultisampleCase::init();
}

MaskConstancyCase::IterateResult MaskConstancyCase::iterate (void)
{
	const glw::Functions&	gl				= m_context.getRenderContext().getFunctions();
	TestLog&				log				= m_testCtx.getLog();
	tcu::Surface			renderedImg		(m_viewportSize, m_viewportSize);

	randomizeViewport();

	log << TestLog::Message << "Clearing color to black" << TestLog::EndMessage;
	gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
	gl.clear(GL_COLOR_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "clear");

	if (m_isAlphaToCoverageCase)
	{
		gl.enable(GL_SAMPLE_ALPHA_TO_COVERAGE);
		gl.colorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
		GLU_EXPECT_NO_ERROR(gl.getError(), "enable GL_SAMPLE_ALPHA_TO_COVERAGE");

		log << TestLog::Message << "GL_SAMPLE_ALPHA_TO_COVERAGE is enabled" << TestLog::EndMessage;
		log << TestLog::Message << "Color mask is TRUE, TRUE, TRUE, FALSE" << TestLog::EndMessage;
	}

	if (m_isSampleCoverageCase)
	{
		gl.enable(GL_SAMPLE_COVERAGE);
		GLU_EXPECT_NO_ERROR(gl.getError(), "enable GL_SAMPLE_COVERAGE");

		log << TestLog::Message << "GL_SAMPLE_COVERAGE is enabled" << TestLog::EndMessage;
	}

	if (m_isSampleMaskCase)
	{
		gl.enable(GL_SAMPLE_MASK);
		GLU_EXPECT_NO_ERROR(gl.getError(), "enable GL_SAMPLE_MASK");

		log << TestLog::Message << "GL_SAMPLE_MASK is enabled" << TestLog::EndMessage;
	}

	log << TestLog::Message
		<< "Drawing several green quads, each fully overlapped by a red quad with the same "
		<< (m_isAlphaToCoverageCase ? "alpha" : "")
		<< (m_isAlphaToCoverageCase && (m_isSampleCoverageCase || m_isSampleMaskCase) ? " and " : "")
		<< (m_isInvertedSampleCoverageCase ? "inverted " : "")
		<< (m_isSampleCoverageCase ? "sample coverage" : "")
		<< (m_isSampleCoverageCase && m_isSampleMaskCase ? " and " : "")
		<< (m_isSampleMaskCase ? "sample mask" : "")
		<< " values"
		<< TestLog::EndMessage;

	const int numQuadRowsCols = m_numSamples*4;

	for (int row = 0; row < numQuadRowsCols; row++)
	{
		for (int col = 0; col < numQuadRowsCols; col++)
		{
			float		x0			= (float)(col+0) / (float)numQuadRowsCols * 2.0f - 1.0f;
			float		x1			= (float)(col+1) / (float)numQuadRowsCols * 2.0f - 1.0f;
			float		y0			= (float)(row+0) / (float)numQuadRowsCols * 2.0f - 1.0f;
			float		y1			= (float)(row+1) / (float)numQuadRowsCols * 2.0f - 1.0f;
			const Vec4	baseGreen	(0.0f, 1.0f, 0.0f, 0.0f);
			const Vec4	baseRed		(1.0f, 0.0f, 0.0f, 0.0f);
			Vec4		alpha0		(0.0f, 0.0f, 0.0f, m_isAlphaToCoverageCase ? (float)col / (float)(numQuadRowsCols-1) : 1.0f);
			Vec4		alpha1		(0.0f, 0.0f, 0.0f, m_isAlphaToCoverageCase ? (float)row / (float)(numQuadRowsCols-1) : 1.0f);

			if (m_isSampleCoverageCase)
			{
				float value = (float)(row*numQuadRowsCols + col) / (float)(numQuadRowsCols*numQuadRowsCols-1);
				gl.sampleCoverage(m_isInvertedSampleCoverageCase ? 1.0f - value : value, m_isInvertedSampleCoverageCase ? GL_TRUE : GL_FALSE);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glSampleCoverage");
			}

			if (m_isSampleMaskCase)
			{
				const int			wordCount		= getEffectiveSampleMaskWordCount(m_numSamples - 1);
				const GLbitfield	finalWordBits	= m_numSamples - 32 * ((m_numSamples-1) / 32);
				const GLbitfield	finalWordMask	= (GLbitfield)deBitMask32(0, (int)finalWordBits);

				for (int wordNdx = 0; wordNdx < wordCount; ++wordNdx)
				{
					const GLbitfield	mask		= (GLbitfield)deUint32Hash((col << (m_numSamples / 2)) ^ row);
					const bool			isFinalWord	= (wordNdx + 1) == wordCount;
					const GLbitfield	maskMask	= (isFinalWord) ? (finalWordMask) : (0xFFFFFFFFUL); // maskMask prevents setting coverage bits higher than sample count

					gl.sampleMaski(wordNdx, mask & maskMask);
					GLU_EXPECT_NO_ERROR(gl.getError(), "glSampleMaski");
				}
			}

			renderQuad(Vec2(x0, y0), Vec2(x1, y0), Vec2(x0, y1), Vec2(x1, y1), baseGreen + alpha0,	baseGreen + alpha1,	baseGreen + alpha0,	baseGreen + alpha1);
			renderQuad(Vec2(x0, y0), Vec2(x1, y0), Vec2(x0, y1), Vec2(x1, y1), baseRed + alpha0,	baseRed + alpha1,	baseRed + alpha0,	baseRed + alpha1);
		}
	}

	readImage(renderedImg);

	log << TestLog::Image("RenderedImage", "Rendered image", renderedImg, QP_IMAGE_COMPRESSION_MODE_PNG);

	for (int y = 0; y < renderedImg.getHeight(); y++)
	for (int x = 0; x < renderedImg.getWidth(); x++)
	{
		if (renderedImg.getPixel(x, y).getGreen() > 0)
		{
			log << TestLog::Message << "Failure: Non-zero green color component detected - should have been completely overwritten by red quad" << TestLog::EndMessage;
			m_context.getTestContext().setTestResult(QP_TEST_RESULT_FAIL, "Failed");
			return STOP;
		}
	}

	log << TestLog::Message
		<< "Success: Coverage mask appears to be constant at a given pixel coordinate with a given "
		<< (m_isAlphaToCoverageCase ? "alpha" : "")
		<< (m_isAlphaToCoverageCase && m_isSampleCoverageCase ? " and " : "")
		<< (m_isSampleCoverageCase ? "coverage value" : "")
		<< TestLog::EndMessage;

	m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Passed");

	return STOP;
}

/*--------------------------------------------------------------------*//*!
 * \brief Tests that unused bits of a sample mask have no effect
 *
 * Tests that the bits in the sample mask with positions higher than
 * the number of samples do not have effect. In multisample fragment
 * operations the sample mask is ANDed with the fragment coverage value.
 * The coverage value cannot have the corresponding bits set.
 *
 * This is done by drawing a quads with varying sample masks and then
 * redrawing the quads with identical masks but with the mask's high bits
 * having different values. Only the latter quad pattern should be visible.
 *//*--------------------------------------------------------------------*/
class SampleMaskHighBitsCase : public DefaultFBOMultisampleCase
{
public:
					SampleMaskHighBitsCase		(Context& context, const char* name, const char* description);
					~SampleMaskHighBitsCase		(void) {}

	void			init						(void);
	IterateResult	iterate						(void);
};

SampleMaskHighBitsCase::SampleMaskHighBitsCase (Context& context, const char* name, const char* description)
	: DefaultFBOMultisampleCase(context, name, description, 256)
{
}

void SampleMaskHighBitsCase::init (void)
{
	const glw::Functions&	gl					= m_context.getRenderContext().getFunctions();
	GLint					 maxSampleMaskWords	= 0;

	// check the test is even possible
	gl.getIntegerv(GL_MAX_SAMPLE_MASK_WORDS, &maxSampleMaskWords);
	if (getEffectiveSampleMaskWordCount(m_numSamples - 1) > maxSampleMaskWords)
		throw tcu::NotSupportedError("Test requires larger GL_MAX_SAMPLE_MASK_WORDS");

	// normal init
	DefaultFBOMultisampleCase::init();
}

SampleMaskHighBitsCase::IterateResult SampleMaskHighBitsCase::iterate (void)
{
	const glw::Functions&	gl				= m_context.getRenderContext().getFunctions();
	TestLog&				log				= m_testCtx.getLog();
	tcu::Surface			renderedImg		(m_viewportSize, m_viewportSize);
	de::Random				rnd				(12345);

	if (m_numSamples % 32 == 0)
	{
		log << TestLog::Message << "Sample count is multiple of word size. No unused high bits in sample mask.\nSkipping." << TestLog::EndMessage;
		m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Skipped");
		return STOP;
	}

	randomizeViewport();

	log << TestLog::Message << "Clearing color to black" << TestLog::EndMessage;
	gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
	gl.clear(GL_COLOR_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "clear");

	gl.enable(GL_SAMPLE_MASK);
	GLU_EXPECT_NO_ERROR(gl.getError(), "enable GL_SAMPLE_MASK");
	log << TestLog::Message << "GL_SAMPLE_MASK is enabled" << TestLog::EndMessage;
	log << TestLog::Message << "Drawing several green quads, each fully overlapped by a red quad with the same effective sample mask values" << TestLog::EndMessage;

	const int numQuadRowsCols = m_numSamples*4;

	for (int row = 0; row < numQuadRowsCols; row++)
	{
		for (int col = 0; col < numQuadRowsCols; col++)
		{
			float				x0				= (float)(col+0) / (float)numQuadRowsCols * 2.0f - 1.0f;
			float				x1				= (float)(col+1) / (float)numQuadRowsCols * 2.0f - 1.0f;
			float				y0				= (float)(row+0) / (float)numQuadRowsCols * 2.0f - 1.0f;
			float				y1				= (float)(row+1) / (float)numQuadRowsCols * 2.0f - 1.0f;
			const Vec4			baseGreen		(0.0f, 1.0f, 0.0f, 1.0f);
			const Vec4			baseRed			(1.0f, 0.0f, 0.0f, 1.0f);

			const int			wordCount		= getEffectiveSampleMaskWordCount(m_numSamples - 1);
			const GLbitfield	finalWordBits	= m_numSamples - 32 * ((m_numSamples-1) / 32);
			const GLbitfield	finalWordMask	= (GLbitfield)deBitMask32(0, (int)finalWordBits);

			for (int wordNdx = 0; wordNdx < wordCount; ++wordNdx)
			{
				const GLbitfield	mask		= (GLbitfield)deUint32Hash((col << (m_numSamples / 2)) ^ row);
				const bool			isFinalWord	= (wordNdx + 1) == wordCount;
				const GLbitfield	maskMask	= (isFinalWord) ? (finalWordMask) : (0xFFFFFFFFUL); // maskMask is 1 on bits in lower positions than sample count
				const GLbitfield	highBits	= rnd.getUint32();

				gl.sampleMaski(wordNdx, (mask & maskMask) | (highBits & ~maskMask));
				GLU_EXPECT_NO_ERROR(gl.getError(), "glSampleMaski");
			}
			renderQuad(Vec2(x0, y0), Vec2(x1, y0), Vec2(x0, y1), Vec2(x1, y1), baseGreen, baseGreen, baseGreen, baseGreen);

			for (int wordNdx = 0; wordNdx < wordCount; ++wordNdx)
			{
				const GLbitfield	mask		= (GLbitfield)deUint32Hash((col << (m_numSamples / 2)) ^ row);
				const bool			isFinalWord	= (wordNdx + 1) == wordCount;
				const GLbitfield	maskMask	= (isFinalWord) ? (finalWordMask) : (0xFFFFFFFFUL); // maskMask is 1 on bits in lower positions than sample count
				const GLbitfield	highBits	= rnd.getUint32();

				gl.sampleMaski(wordNdx, (mask & maskMask) | (highBits & ~maskMask));
				GLU_EXPECT_NO_ERROR(gl.getError(), "glSampleMaski");
			}
			renderQuad(Vec2(x0, y0), Vec2(x1, y0), Vec2(x0, y1), Vec2(x1, y1), baseRed, baseRed, baseRed, baseRed);
		}
	}

	readImage(renderedImg);

	log << TestLog::Image("RenderedImage", "Rendered image", renderedImg, QP_IMAGE_COMPRESSION_MODE_PNG);

	for (int y = 0; y < renderedImg.getHeight(); y++)
	for (int x = 0; x < renderedImg.getWidth(); x++)
	{
		if (renderedImg.getPixel(x, y).getGreen() > 0)
		{
			log << TestLog::Message << "Failure: Non-zero green color component detected - should have been completely overwritten by red quad. Mask unused bits have effect." << TestLog::EndMessage;
			m_context.getTestContext().setTestResult(QP_TEST_RESULT_FAIL, "Unused mask bits modified mask");
			return STOP;
		}
	}

	log << TestLog::Message << "Success: Coverage mask high bits appear to have no effect." << TestLog::EndMessage;
	m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Passed");

	return STOP;
}

} // anonymous

MultisampleTests::MultisampleTests (Context& context)
	: TestCaseGroup(context, "multisample", "Multisample tests")
{
}

MultisampleTests::~MultisampleTests (void)
{
}

void MultisampleTests::init (void)
{
	tcu::TestCaseGroup* const group = new tcu::TestCaseGroup(m_testCtx, "default_framebuffer", "Test with default framebuffer");

	addChild(group);

	// .default_framebuffer
	{
		// sample positions
		group->addChild(new SamplePosQueryCase			(m_context, "sample_position", "test SAMPLE_POSITION"));

		// sample mask
		group->addChild(new MaskInvertCase				(m_context, "sample_mask_sum_of_inverses",	"Test that mask and its negation's sum equal the fully set mask"));
		group->addChild(new MaskProportionalityCase		(m_context, "proportionality_sample_mask",	"Test the proportionality property of GL_SAMPLE_MASK"));

		group->addChild(new MaskConstancyCase			(m_context, "constancy_sample_mask",
																	"Test that coverage mask is constant at given coordinates with a given alpha or coverage value, using GL_SAMPLE_MASK",
																	MaskConstancyCase::CASEBIT_SAMPLE_MASK));
		group->addChild(new MaskConstancyCase			(m_context, "constancy_alpha_to_coverage_sample_mask",
																	"Test that coverage mask is constant at given coordinates with a given alpha or coverage value, using GL_SAMPLE_ALPHA_TO_COVERAGE and GL_SAMPLE_MASK",
																	MaskConstancyCase::CASEBIT_ALPHA_TO_COVERAGE | MaskConstancyCase::CASEBIT_SAMPLE_MASK));
		group->addChild(new MaskConstancyCase			(m_context, "constancy_sample_coverage_sample_mask",
																	"Test that coverage mask is constant at given coordinates with a given alpha or coverage value, using GL_SAMPLE_COVERAGE and GL_SAMPLE_MASK",
																	MaskConstancyCase::CASEBIT_SAMPLE_COVERAGE | MaskConstancyCase::CASEBIT_SAMPLE_MASK));
		group->addChild(new MaskConstancyCase			(m_context, "constancy_alpha_to_coverage_sample_coverage_sample_mask",
																	"Test that coverage mask is constant at given coordinates with a given alpha or coverage value, using GL_SAMPLE_ALPHA_TO_COVERAGE, GL_SAMPLE_COVERAGE and GL_SAMPLE_MASK",
																	MaskConstancyCase::CASEBIT_ALPHA_TO_COVERAGE | MaskConstancyCase::CASEBIT_SAMPLE_COVERAGE | MaskConstancyCase::CASEBIT_SAMPLE_MASK));
		group->addChild(new SampleMaskHighBitsCase		(m_context, "sample_mask_non_effective_bits",
																	"Test that values of unused bits of a sample mask (bit index > sample count) have no effect"));
	}
}

} // Functional
} // gles31
} // deqp
