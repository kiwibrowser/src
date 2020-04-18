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
 * \brief Multisample texture test
 *//*--------------------------------------------------------------------*/

#include "es31fTextureMultisampleTests.hpp"
#include "tcuTestLog.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuSurface.hpp"
#include "tcuStringTemplate.hpp"
#include "tcuTextureUtil.hpp"
#include "glsStateQueryUtil.hpp"
#include "tcuRasterizationVerifier.hpp"
#include "gluRenderContext.hpp"
#include "gluCallLogWrapper.hpp"
#include "gluObjectWrapper.hpp"
#include "gluShaderProgram.hpp"
#include "gluPixelTransfer.hpp"
#include "gluStrUtil.hpp"
#include "gluContextInfo.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "deStringUtil.hpp"
#include "deRandom.hpp"

using namespace glw;

namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace
{

using tcu::RasterizationArguments;
using tcu::TriangleSceneSpec;

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

	mask[numWords - 1] = (deUint32)((1ULL << topWordBits) - (deUint32)1);
	return mask;
}

/*--------------------------------------------------------------------*//*!
 * \brief Creates sample mask with nthBit set
 *//*--------------------------------------------------------------------*/
static std::vector<deUint32> genSetNthBitSampleMask (int nthBit)
{
	const int				wordSize	= 32;
	const int				numWords	= getEffectiveSampleMaskWordCount(nthBit);
	const deUint32			topWordBits	= (deUint32)(nthBit - (numWords - 1) * wordSize);
	std::vector<deUint32>	mask		(numWords);

	for (int ndx = 0; ndx < numWords - 1; ++ndx)
		mask[ndx] = 0;

	mask[numWords - 1] = (deUint32)(1ULL << topWordBits);
	return mask;
}

std::string specializeShader (Context& context, const char* code)
{
	const glu::ContextType				contextType		= context.getRenderContext().getType();
	const glu::GLSLVersion				glslVersion		= glu::getContextTypeGLSLVersion(contextType);
	std::map<std::string, std::string> specializationMap;

	specializationMap["GLSL_VERSION_DECL"] = glu::getGLSLVersionDeclaration(glslVersion);

	return tcu::StringTemplate(code).specialize(specializationMap);
}

class SamplePosRasterizationTest : public TestCase
{
public:
								SamplePosRasterizationTest	(Context& context, const char* name, const char* desc, int samples);
								~SamplePosRasterizationTest	(void);

private:
	void						init						(void);
	void						deinit						(void);
	IterateResult				iterate						(void);
	void						genMultisampleTexture		(void);
	void						genSamplerProgram			(void);
	bool						testMultisampleTexture		(int sampleNdx);
	void						drawSample					(tcu::Surface& dst, int sampleNdx);
	void						convertToSceneSpec			(TriangleSceneSpec& scene, const tcu::Vec2& samplePos) const;

	struct Triangle
	{
		tcu::Vec4 p1;
		tcu::Vec4 p2;
		tcu::Vec4 p3;
	};

	const int					m_samples;
	const int					m_canvasSize;
	std::vector<Triangle>		m_testTriangles;

	int							m_iteration;
	bool						m_allIterationsOk;

	GLuint						m_texID;
	GLuint						m_vaoID;
	GLuint						m_vboID;
	std::vector<tcu::Vec2>		m_samplePositions;
	int							m_subpixelBits;

	const glu::ShaderProgram*	m_samplerProgram;
	GLint						m_samplerProgramPosLoc;
	GLint						m_samplerProgramSamplerLoc;
	GLint						m_samplerProgramSampleNdxLoc;
};

SamplePosRasterizationTest::SamplePosRasterizationTest (Context& context, const char* name, const char* desc, int samples)
	: TestCase						(context, name, desc)
	, m_samples						(samples)
	, m_canvasSize					(256)
	, m_iteration					(0)
	, m_allIterationsOk				(true)
	, m_texID						(0)
	, m_vaoID						(0)
	, m_vboID						(0)
	, m_subpixelBits				(0)
	, m_samplerProgram				(DE_NULL)
	, m_samplerProgramPosLoc		(-1)
	, m_samplerProgramSamplerLoc	(-1)
	, m_samplerProgramSampleNdxLoc	(-1)
{
}

SamplePosRasterizationTest::~SamplePosRasterizationTest (void)
{
	deinit();
}

void SamplePosRasterizationTest::init (void)
{
	const glw::Functions&	gl			= m_context.getRenderContext().getFunctions();
	GLint					maxSamples	= 0;

	// requirements

	if (m_context.getRenderTarget().getWidth() < m_canvasSize || m_context.getRenderTarget().getHeight() < m_canvasSize)
		throw tcu::NotSupportedError("render target size must be at least " + de::toString(m_canvasSize) + "x" + de::toString(m_canvasSize));

	gl.getIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &maxSamples);
	if (m_samples > maxSamples)
		throw tcu::NotSupportedError("Requested sample count is greater than GL_MAX_COLOR_TEXTURE_SAMPLES");

	m_testCtx.getLog() << tcu::TestLog::Message << "GL_MAX_COLOR_TEXTURE_SAMPLES = " << maxSamples << tcu::TestLog::EndMessage;

	gl.getIntegerv(GL_SUBPIXEL_BITS, &m_subpixelBits);
	m_testCtx.getLog() << tcu::TestLog::Message << "GL_SUBPIXEL_BITS = " << m_subpixelBits << tcu::TestLog::EndMessage;

	// generate textures & other gl stuff

	m_testCtx.getLog() << tcu::TestLog::Message << "Creating multisample texture" << tcu::TestLog::EndMessage;

	gl.genTextures				(1, &m_texID);
	gl.bindTexture				(GL_TEXTURE_2D_MULTISAMPLE, m_texID);
	gl.texStorage2DMultisample	(GL_TEXTURE_2D_MULTISAMPLE, m_samples, GL_RGBA8, m_canvasSize, m_canvasSize, GL_TRUE);
	GLU_EXPECT_NO_ERROR			(gl.getError(), "texStorage2DMultisample");

	gl.genVertexArrays		(1, &m_vaoID);
	gl.bindVertexArray		(m_vaoID);
	GLU_EXPECT_NO_ERROR		(gl.getError(), "bindVertexArray");

	gl.genBuffers			(1, &m_vboID);
	gl.bindBuffer			(GL_ARRAY_BUFFER, m_vboID);
	GLU_EXPECT_NO_ERROR		(gl.getError(), "bindBuffer");

	// generate test scene
	for (int i = 0; i < 20; ++i)
	{
		// vertical spikes
		Triangle tri;
		tri.p1 = tcu::Vec4(((float)i        + 1.0f / (float)(i + 1)) / 20.0f,	 0.0f,	0.0f,	1.0f);
		tri.p2 = tcu::Vec4(((float)i + 0.3f + 1.0f / (float)(i + 1)) / 20.0f,	 0.0f,	0.0f,	1.0f);
		tri.p3 = tcu::Vec4(((float)i        + 1.0f / (float)(i + 1)) / 20.0f,	-1.0f,	0.0f,	1.0f);
		m_testTriangles.push_back(tri);
	}
	for (int i = 0; i < 20; ++i)
	{
		// horisontal spikes
		Triangle tri;
		tri.p1 = tcu::Vec4(-1.0f,	((float)i        + 1.0f / (float)(i + 1)) / 20.0f,	0.0f,	1.0f);
		tri.p2 = tcu::Vec4(-1.0f,	((float)i + 0.3f + 1.0f / (float)(i + 1)) / 20.0f,	0.0f,	1.0f);
		tri.p3 = tcu::Vec4( 0.0f,	((float)i        + 1.0f / (float)(i + 1)) / 20.0f,	0.0f,	1.0f);
		m_testTriangles.push_back(tri);
	}

	for (int i = 0; i < 20; ++i)
	{
		// fan
		const tcu::Vec2 p = tcu::Vec2(deFloatCos(((float)i)/20.0f*DE_PI*2) * 0.5f + 0.5f, deFloatSin(((float)i)/20.0f*DE_PI*2) * 0.5f + 0.5f);
		const tcu::Vec2 d = tcu::Vec2(0.1f, 0.02f);

		Triangle tri;
		tri.p1 = tcu::Vec4(0.4f,			0.4f,			0.0f,	1.0f);
		tri.p2 = tcu::Vec4(p.x(),			p.y(),			0.0f,	1.0f);
		tri.p3 = tcu::Vec4(p.x() + d.x(),	p.y() + d.y(),	0.0f,	1.0f);
		m_testTriangles.push_back(tri);
	}
	{
		Triangle tri;
		tri.p1 = tcu::Vec4(-0.202f, -0.202f, 0.0f, 1.0f);
		tri.p2 = tcu::Vec4(-0.802f, -0.202f, 0.0f, 1.0f);
		tri.p3 = tcu::Vec4(-0.802f, -0.802f, 0.0f, 1.0f);
		m_testTriangles.push_back(tri);
	}

	// generate multisample texture (and query the sample positions in it)
	genMultisampleTexture();

	// verify queried samples are in a valid range
	for (int sampleNdx = 0; sampleNdx < m_samples; ++sampleNdx)
	{
		if (m_samplePositions[sampleNdx].x() < 0.0f || m_samplePositions[sampleNdx].x() > 1.0f ||
			m_samplePositions[sampleNdx].y() < 0.0f || m_samplePositions[sampleNdx].y() > 1.0f)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "// ERROR: Sample position of sample " << sampleNdx << " should be in range ([0, 1], [0, 1]). Got " << m_samplePositions[sampleNdx] << tcu::TestLog::EndMessage;
			throw tcu::TestError("invalid sample position");
		}
	}

	// generate sampler program
	genSamplerProgram();
}

void SamplePosRasterizationTest::deinit (void)
{
	if (m_vboID)
	{
		m_context.getRenderContext().getFunctions().deleteBuffers(1, &m_vboID);
		m_vboID = 0;
	}

	if (m_vaoID)
	{
		m_context.getRenderContext().getFunctions().deleteVertexArrays(1, &m_vaoID);
		m_vaoID = 0;
	}

	if (m_texID)
	{
		m_context.getRenderContext().getFunctions().deleteTextures(1, &m_texID);
		m_texID = 0;
	}

	if (m_samplerProgram)
	{
		delete m_samplerProgram;
		m_samplerProgram = DE_NULL;
	}
}

SamplePosRasterizationTest::IterateResult SamplePosRasterizationTest::iterate (void)
{
	m_allIterationsOk &= testMultisampleTexture(m_iteration);
	m_iteration++;

	if (m_iteration < m_samples)
		return CONTINUE;

	// End result
	if (m_allIterationsOk)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Pixel comparison failed");

	return STOP;
}

void SamplePosRasterizationTest::genMultisampleTexture (void)
{
	const char* const vertexShaderSource	=	"${GLSL_VERSION_DECL}\n"
												"in highp vec4 a_position;\n"
												"void main (void)\n"
												"{\n"
												"	gl_Position = a_position;\n"
												"}\n";
	const char* const fragmentShaderSource	=	"${GLSL_VERSION_DECL}\n"
												"layout(location = 0) out highp vec4 fragColor;\n"
												"void main (void)\n"
												"{\n"
												"	fragColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
												"}\n";

	const glw::Functions&		gl				= m_context.getRenderContext().getFunctions();
	const glu::ShaderProgram	program			(m_context.getRenderContext(), glu::ProgramSources()
																			<< glu::VertexSource(specializeShader(m_context, vertexShaderSource))
																			<< glu::FragmentSource(specializeShader(m_context, fragmentShaderSource)));
	const GLuint				posLoc			= gl.getAttribLocation(program.getProgram(), "a_position");
	GLuint						fboID			= 0;

	if (!program.isOk())
	{
		m_testCtx.getLog() << program;
		throw tcu::TestError("Failed to build shader.");
	}

	gl.bindTexture			(GL_TEXTURE_2D_MULTISAMPLE, m_texID);
	gl.bindVertexArray		(m_vaoID);
	gl.bindBuffer			(GL_ARRAY_BUFFER, m_vboID);

	// Setup fbo for drawing and for sample position query

	m_testCtx.getLog() << tcu::TestLog::Message << "Attaching texture to FBO" << tcu::TestLog::EndMessage;

	gl.genFramebuffers		(1, &fboID);
	gl.bindFramebuffer		(GL_FRAMEBUFFER, fboID);
	gl.framebufferTexture2D	(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, m_texID, 0);
	GLU_EXPECT_NO_ERROR		(gl.getError(), "framebufferTexture2D");

	// Query sample positions of the multisample texture by querying the sample positions
	// from an fbo which has the multisample texture as attachment.

	m_testCtx.getLog() << tcu::TestLog::Message << "Sample locations:" << tcu::TestLog::EndMessage;

	for (int sampleNdx = 0; sampleNdx < m_samples; ++sampleNdx)
	{
		gls::StateQueryUtil::StateQueryMemoryWriteGuard<float[2]> position;

		gl.getMultisamplefv(GL_SAMPLE_POSITION, (deUint32)sampleNdx, position);
		if (!position.verifyValidity(m_testCtx))
			throw tcu::TestError("Error while querying sample positions");

		m_testCtx.getLog() << tcu::TestLog::Message << "\t" << sampleNdx << ": (" << position[0] << ", " << position[1] << ")" << tcu::TestLog::EndMessage;
		m_samplePositions.push_back(tcu::Vec2(position[0], position[1]));
	}

	// Draw test pattern to texture

	m_testCtx.getLog() << tcu::TestLog::Message << "Drawing test pattern to the texture" << tcu::TestLog::EndMessage;

	gl.bufferData				(GL_ARRAY_BUFFER, (glw::GLsizeiptr)(m_testTriangles.size() * sizeof(Triangle)), &m_testTriangles[0], GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR			(gl.getError(), "bufferData");

	gl.viewport					(0, 0, m_canvasSize, m_canvasSize);
	gl.clearColor				(0, 0, 0, 1);
	gl.clear					(GL_COLOR_BUFFER_BIT);
	gl.vertexAttribPointer		(posLoc, 4, GL_FLOAT, GL_FALSE, 0, DE_NULL);
	gl.enableVertexAttribArray	(posLoc);
	GLU_EXPECT_NO_ERROR			(gl.getError(), "vertexAttribPointer");

	gl.useProgram				(program.getProgram());
	gl.drawArrays				(GL_TRIANGLES, 0, (glw::GLsizei)(m_testTriangles.size() * 3));
	GLU_EXPECT_NO_ERROR			(gl.getError(), "drawArrays");

	gl.disableVertexAttribArray	(posLoc);
	gl.useProgram				(0);
	gl.deleteFramebuffers		(1, &fboID);
	GLU_EXPECT_NO_ERROR			(gl.getError(), "cleanup");
}

void SamplePosRasterizationTest::genSamplerProgram (void)
{
	const char* const	vertexShaderSource	=	"${GLSL_VERSION_DECL}\n"
												"in highp vec4 a_position;\n"
												"void main (void)\n"
												"{\n"
												"	gl_Position = a_position;\n"
												"}\n";
	const char* const	fragShaderSource	=	"${GLSL_VERSION_DECL}\n"
												"layout(location = 0) out highp vec4 fragColor;\n"
												"uniform highp sampler2DMS u_sampler;\n"
												"uniform highp int u_sample;\n"
												"void main (void)\n"
												"{\n"
												"	fragColor = texelFetch(u_sampler, ivec2(int(floor(gl_FragCoord.x)), int(floor(gl_FragCoord.y))), u_sample);\n"
												"}\n";
	const tcu::ScopedLogSection	section			(m_testCtx.getLog(), "Generate sampler shader", "Generate sampler shader");
	const glw::Functions&		gl			=	m_context.getRenderContext().getFunctions();

	m_samplerProgram = new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources() << glu::VertexSource(specializeShader(m_context, vertexShaderSource)) << glu::FragmentSource(specializeShader(m_context, fragShaderSource)));
	m_testCtx.getLog() << *m_samplerProgram;

	if (!m_samplerProgram->isOk())
		throw tcu::TestError("Could not create sampler program.");

	m_samplerProgramPosLoc			= gl.getAttribLocation(m_samplerProgram->getProgram(), "a_position");
	m_samplerProgramSamplerLoc		= gl.getUniformLocation(m_samplerProgram->getProgram(), "u_sampler");
	m_samplerProgramSampleNdxLoc	= gl.getUniformLocation(m_samplerProgram->getProgram(), "u_sample");
}

bool SamplePosRasterizationTest::testMultisampleTexture (int sampleNdx)
{
	tcu::Surface		glSurface(m_canvasSize, m_canvasSize);
	TriangleSceneSpec	scene;

	// Draw sample
	drawSample(glSurface, sampleNdx);

	// Draw reference(s)
	convertToSceneSpec(scene, m_samplePositions[sampleNdx]);

	// Compare
	{
		RasterizationArguments args;
		args.redBits		= m_context.getRenderTarget().getPixelFormat().redBits;
		args.greenBits		= m_context.getRenderTarget().getPixelFormat().greenBits;
		args.blueBits		= m_context.getRenderTarget().getPixelFormat().blueBits;
		args.numSamples		= 0;
		args.subpixelBits	= m_subpixelBits;

		return tcu::verifyTriangleGroupRasterization(glSurface, scene, args, m_testCtx.getLog(), tcu::VERIFICATIONMODE_STRICT);
	}
}

void SamplePosRasterizationTest::drawSample (tcu::Surface& dst, int sampleNdx)
{
	// Downsample using only one sample
	static const tcu::Vec4 fullscreenQuad[] =
	{
		tcu::Vec4(-1.0f,  1.0f, 0.0f, 1.0f),
		tcu::Vec4(-1.0f, -1.0f, 0.0f, 1.0f),
		tcu::Vec4( 1.0f,  1.0f, 0.0f, 1.0f),
		tcu::Vec4( 1.0f, -1.0f, 0.0f, 1.0f)
	};

	const tcu::ScopedLogSection section	(m_testCtx.getLog(), "Test sample position " + de::toString(sampleNdx+1) + "/" + de::toString(m_samples), "Test sample position " + de::toString(sampleNdx+1) + "/" + de::toString(m_samples));
	const glw::Functions&		gl		= m_context.getRenderContext().getFunctions();

	gl.bindTexture				(GL_TEXTURE_2D_MULTISAMPLE, m_texID);
	gl.bindVertexArray			(m_vaoID);
	gl.bindBuffer				(GL_ARRAY_BUFFER, m_vboID);

	gl.bufferData				(GL_ARRAY_BUFFER, sizeof(fullscreenQuad), &fullscreenQuad[0], GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR			(gl.getError(), "bufferData");

	gl.viewport					(0, 0, m_canvasSize, m_canvasSize);
	gl.clearColor				(0, 0, 0, 1);
	gl.clear					(GL_COLOR_BUFFER_BIT);
	gl.vertexAttribPointer		(m_samplerProgramPosLoc, 4, GL_FLOAT, GL_FALSE, 0, DE_NULL);
	gl.enableVertexAttribArray	(m_samplerProgramPosLoc);
	GLU_EXPECT_NO_ERROR			(gl.getError(), "vertexAttribPointer");

	gl.useProgram				(m_samplerProgram->getProgram());
	gl.uniform1i				(m_samplerProgramSamplerLoc, 0);
	gl.uniform1i				(m_samplerProgramSampleNdxLoc, (deInt32)sampleNdx);
	GLU_EXPECT_NO_ERROR			(gl.getError(), "useprogram");

	m_testCtx.getLog() << tcu::TestLog::Message << "Reading from texture with sample index " << sampleNdx << tcu::TestLog::EndMessage;

	gl.drawArrays				(GL_TRIANGLE_STRIP, 0, 4);
	GLU_EXPECT_NO_ERROR			(gl.getError(), "drawArrays");

	gl.disableVertexAttribArray	(m_samplerProgramPosLoc);
	gl.useProgram				(0);
	GLU_EXPECT_NO_ERROR			(gl.getError(), "cleanup");

	gl.finish					();
	glu::readPixels				(m_context.getRenderContext(), 0, 0, dst.getAccess());
	GLU_EXPECT_NO_ERROR			(gl.getError(), "readPixels");
}

void SamplePosRasterizationTest::convertToSceneSpec (TriangleSceneSpec& scene, const tcu::Vec2& samplePos) const
{
	// Triangles are offset from the pixel center by "offset". Move the triangles back to take this into account.
	const tcu::Vec4 offset = tcu::Vec4(samplePos.x() - 0.5f, samplePos.y() - 0.5f, 0.0f, 0.0f) / tcu::Vec4((float)m_canvasSize, (float)m_canvasSize, 1.0f, 1.0f) * 2.0f;

	for (int triangleNdx = 0; triangleNdx < (int)m_testTriangles.size(); ++triangleNdx)
	{
		TriangleSceneSpec::SceneTriangle triangle;

		triangle.positions[0] = m_testTriangles[triangleNdx].p1 - offset;
		triangle.positions[1] = m_testTriangles[triangleNdx].p2 - offset;
		triangle.positions[2] = m_testTriangles[triangleNdx].p3 - offset;

		triangle.sharedEdge[0] = false;
		triangle.sharedEdge[1] = false;
		triangle.sharedEdge[2] = false;

		scene.triangles.push_back(triangle);
	}
}

class SampleMaskCase : public TestCase
{
public:
	enum CaseFlags
	{
		FLAGS_NONE					= 0,
		FLAGS_ALPHA_TO_COVERAGE		= (1ULL << 0),
		FLAGS_SAMPLE_COVERAGE		= (1ULL << 1),
		FLAGS_HIGH_BITS				= (1ULL << 2),
	};

								SampleMaskCase				(Context& context, const char* name, const char* desc, int samples, int flags);
								~SampleMaskCase				(void);

private:
	void						init						(void);
	void						deinit						(void);
	IterateResult				iterate						(void);

	void						genSamplerProgram			(void);
	void						genAlphaProgram				(void);
	void						updateTexture				(int sample);
	bool						verifyTexture				(int sample);
	void						drawSample					(tcu::Surface& dst, int sample);

	const int					m_samples;
	const int					m_canvasSize;
	const int					m_gridsize;
	const int					m_effectiveSampleMaskWordCount;

	int							m_flags;
	int							m_currentSample;
	int							m_allIterationsOk;

	glw::GLuint					m_texID;
	glw::GLuint					m_vaoID;
	glw::GLuint					m_vboID;
	glw::GLuint					m_fboID;

	const glu::ShaderProgram*	m_samplerProgram;
	glw::GLint					m_samplerProgramPosLoc;
	glw::GLint					m_samplerProgramSamplerLoc;
	glw::GLint					m_samplerProgramSampleNdxLoc;

	const glu::ShaderProgram*	m_alphaProgram;
	glw::GLint					m_alphaProgramPosLoc;
};

SampleMaskCase::SampleMaskCase (Context& context, const char* name, const char* desc, int samples, int flags)
	: TestCase						(context, name, desc)
	, m_samples						(samples)
	, m_canvasSize					(256)
	, m_gridsize					(16)
	, m_effectiveSampleMaskWordCount(getEffectiveSampleMaskWordCount(samples - 1))
	, m_flags						(flags)
	, m_currentSample				(-1)
	, m_allIterationsOk				(true)
	, m_texID						(0)
	, m_vaoID						(0)
	, m_vboID						(0)
	, m_fboID						(0)
	, m_samplerProgram				(DE_NULL)
	, m_samplerProgramPosLoc		(-1)
	, m_samplerProgramSamplerLoc	(-1)
	, m_samplerProgramSampleNdxLoc	(-1)
	, m_alphaProgram				(DE_NULL)
	, m_alphaProgramPosLoc			(-1)
{
}

SampleMaskCase::~SampleMaskCase (void)
{
	deinit();
}

void SampleMaskCase::init (void)
{
	const glw::Functions&	gl					= m_context.getRenderContext().getFunctions();
	glw::GLint				maxSamples			= 0;
	glw::GLint				maxSampleMaskWords	= 0;

	// requirements

	if (m_context.getRenderTarget().getWidth() < m_canvasSize || m_context.getRenderTarget().getHeight() < m_canvasSize)
		throw tcu::NotSupportedError("render target size must be at least " + de::toString(m_canvasSize) + "x" + de::toString(m_canvasSize));

	gl.getIntegerv(GL_MAX_SAMPLE_MASK_WORDS, &maxSampleMaskWords);
	if (m_effectiveSampleMaskWordCount > maxSampleMaskWords)
		throw tcu::NotSupportedError("Test requires larger GL_MAX_SAMPLE_MASK_WORDS");

	gl.getIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &maxSamples);
	if (m_samples > maxSamples)
		throw tcu::NotSupportedError("Requested sample count is greater than GL_MAX_COLOR_TEXTURE_SAMPLES");

	m_testCtx.getLog() << tcu::TestLog::Message << "GL_MAX_COLOR_TEXTURE_SAMPLES = " << maxSamples << tcu::TestLog::EndMessage;

	// Don't even try to test high bits if there are none

	if ((m_flags & FLAGS_HIGH_BITS) && (m_samples % 32 == 0))
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Sample count is multiple of word size. No unused high bits in sample mask.\nSkipping." << tcu::TestLog::EndMessage;
		throw tcu::NotSupportedError("Test requires unused high bits (sample count not multiple of 32)");
	}

	// generate textures

	m_testCtx.getLog() << tcu::TestLog::Message << "Creating multisample texture with sample count " << m_samples << tcu::TestLog::EndMessage;

	gl.genTextures				(1, &m_texID);
	gl.bindTexture				(GL_TEXTURE_2D_MULTISAMPLE, m_texID);
	gl.texStorage2DMultisample	(GL_TEXTURE_2D_MULTISAMPLE, m_samples, GL_RGBA8, m_canvasSize, m_canvasSize, GL_FALSE);
	GLU_EXPECT_NO_ERROR			(gl.getError(), "texStorage2DMultisample");

	// attach texture to fbo

	m_testCtx.getLog() << tcu::TestLog::Message << "Attaching texture to FBO" << tcu::TestLog::EndMessage;

	gl.genFramebuffers		(1, &m_fboID);
	gl.bindFramebuffer		(GL_FRAMEBUFFER, m_fboID);
	gl.framebufferTexture2D	(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, m_texID, 0);
	GLU_EXPECT_NO_ERROR		(gl.getError(), "framebufferTexture2D");

	// buffers

	gl.genVertexArrays		(1, &m_vaoID);
	GLU_EXPECT_NO_ERROR		(gl.getError(), "genVertexArrays");

	gl.genBuffers			(1, &m_vboID);
	gl.bindBuffer			(GL_ARRAY_BUFFER, m_vboID);
	GLU_EXPECT_NO_ERROR		(gl.getError(), "genBuffers");

	// generate grid pattern
	{
		std::vector<tcu::Vec4> gridData(m_gridsize*m_gridsize*6);

		for (int y = 0; y < m_gridsize; ++y)
		for (int x = 0; x < m_gridsize; ++x)
		{
			gridData[(y * m_gridsize + x)*6 + 0] = tcu::Vec4(((float)(x+0) / (float)m_gridsize) * 2.0f - 1.0f, ((float)(y+0) / (float)m_gridsize) * 2.0f - 1.0f, 0.0f, 1.0f);
			gridData[(y * m_gridsize + x)*6 + 1] = tcu::Vec4(((float)(x+0) / (float)m_gridsize) * 2.0f - 1.0f, ((float)(y+1) / (float)m_gridsize) * 2.0f - 1.0f, 0.0f, 1.0f);
			gridData[(y * m_gridsize + x)*6 + 2] = tcu::Vec4(((float)(x+1) / (float)m_gridsize) * 2.0f - 1.0f, ((float)(y+1) / (float)m_gridsize) * 2.0f - 1.0f, 0.0f, 1.0f);
			gridData[(y * m_gridsize + x)*6 + 3] = tcu::Vec4(((float)(x+0) / (float)m_gridsize) * 2.0f - 1.0f, ((float)(y+0) / (float)m_gridsize) * 2.0f - 1.0f, 0.0f, 1.0f);
			gridData[(y * m_gridsize + x)*6 + 4] = tcu::Vec4(((float)(x+1) / (float)m_gridsize) * 2.0f - 1.0f, ((float)(y+1) / (float)m_gridsize) * 2.0f - 1.0f, 0.0f, 1.0f);
			gridData[(y * m_gridsize + x)*6 + 5] = tcu::Vec4(((float)(x+1) / (float)m_gridsize) * 2.0f - 1.0f, ((float)(y+0) / (float)m_gridsize) * 2.0f - 1.0f, 0.0f, 1.0f);
		}

		gl.bufferData			(GL_ARRAY_BUFFER, (int)(gridData.size() * sizeof(tcu::Vec4)), gridData[0].getPtr(), GL_STATIC_DRAW);
		GLU_EXPECT_NO_ERROR		(gl.getError(), "bufferData");
	}

	// generate programs

	genSamplerProgram();
	genAlphaProgram();
}

void SampleMaskCase::deinit (void)
{
	if (m_texID)
	{
		m_context.getRenderContext().getFunctions().deleteTextures(1, &m_texID);
		m_texID = 0;
	}
	if (m_vaoID)
	{
		m_context.getRenderContext().getFunctions().deleteVertexArrays(1, &m_vaoID);
		m_vaoID = 0;
	}
	if (m_vboID)
	{
		m_context.getRenderContext().getFunctions().deleteBuffers(1, &m_vboID);
		m_vboID = 0;
	}
	if (m_fboID)
	{
		m_context.getRenderContext().getFunctions().deleteFramebuffers(1, &m_fboID);
		m_fboID = 0;
	}

	if (m_samplerProgram)
	{
		delete m_samplerProgram;
		m_samplerProgram = DE_NULL;
	}
	if (m_alphaProgram)
	{
		delete m_alphaProgram;
		m_alphaProgram = DE_NULL;
	}
}

SampleMaskCase::IterateResult SampleMaskCase::iterate (void)
{
	const tcu::ScopedLogSection	section(m_testCtx.getLog(), "Iteration", (m_currentSample == -1) ? ("Verifying with zero mask") : (std::string() + "Verifying sample " + de::toString(m_currentSample + 1) + "/" + de::toString(m_samples)));

	bool iterationOk;

	// Mask only one sample, clear rest

	updateTexture(m_currentSample);

	// Verify only one sample set is in the texture

	iterationOk = verifyTexture(m_currentSample);
	if (!iterationOk)
		m_allIterationsOk = false;

	m_currentSample++;
	if (m_currentSample < m_samples)
		return CONTINUE;

	// End result

	if (m_allIterationsOk)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else if (m_flags & FLAGS_HIGH_BITS)
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Unused mask bits have effect");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Sample test failed");

	return STOP;
}

void SampleMaskCase::genSamplerProgram (void)
{
	const char* const	vertexShaderSource			= "${GLSL_VERSION_DECL}\n"
													  "in highp vec4 a_position;\n"
													  "void main (void)\n"
													  "{\n"
													  "	gl_Position = a_position;\n"
													  "}\n";
	const char* const	fragShaderSource			= "${GLSL_VERSION_DECL}\n"
													  "layout(location = 0) out highp vec4 fragColor;\n"
													  "uniform highp sampler2DMS u_sampler;\n"
													  "uniform highp int u_sample;\n"
													  "void main (void)\n"
													  "{\n"
													  "	highp float correctCoverage = 0.0;\n"
													  "	highp float incorrectCoverage = 0.0;\n"
													  "	highp ivec2 texelPos = ivec2(int(floor(gl_FragCoord.x)), int(floor(gl_FragCoord.y)));\n"
													  "\n"
													  "	for (int sampleNdx = 0; sampleNdx < ${NUMSAMPLES}; ++sampleNdx)\n"
													  "	{\n"
													  "		highp float sampleColor = texelFetch(u_sampler, texelPos, sampleNdx).r;\n"
													  "		if (sampleNdx == u_sample)\n"
													  "			correctCoverage += sampleColor;\n"
													  "		else\n"
													  "			incorrectCoverage += sampleColor;\n"
													  "	}\n"
													  "	fragColor = vec4(correctCoverage, incorrectCoverage, 0.0, 1.0);\n"
													  "}\n";
	const tcu::ScopedLogSection			section		(m_testCtx.getLog(), "GenerateSamplerShader", "Generate sampler shader");
	const glw::Functions&				gl			= m_context.getRenderContext().getFunctions();
	std::map<std::string, std::string>	args;
	const glu::GLSLVersion				glslVersion = glu::getContextTypeGLSLVersion(m_context.getRenderContext().getType());

	args["GLSL_VERSION_DECL"] = glu::getGLSLVersionDeclaration(glslVersion);
	args["NUMSAMPLES"] = de::toString(m_samples);

	m_samplerProgram = new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources() << glu::VertexSource(specializeShader(m_context, vertexShaderSource)) << glu::FragmentSource(tcu::StringTemplate(fragShaderSource).specialize(args)));
	m_testCtx.getLog() << *m_samplerProgram;

	if (!m_samplerProgram->isOk())
		throw tcu::TestError("Could not create sampler program.");

	m_samplerProgramPosLoc			= gl.getAttribLocation(m_samplerProgram->getProgram(), "a_position");
	m_samplerProgramSamplerLoc		= gl.getUniformLocation(m_samplerProgram->getProgram(), "u_sampler");
	m_samplerProgramSampleNdxLoc	= gl.getUniformLocation(m_samplerProgram->getProgram(), "u_sample");
}

void SampleMaskCase::genAlphaProgram (void)
{
	const char* const	vertexShaderSource	=	"${GLSL_VERSION_DECL}\n"
												"in highp vec4 a_position;\n"
												"out highp float v_alpha;\n"
												"void main (void)\n"
												"{\n"
												"	gl_Position = a_position;\n"
												"	v_alpha = (a_position.x * 0.5 + 0.5)*(a_position.y * 0.5 + 0.5);\n"
												"}\n";
	const char* const	fragShaderSource	=	"${GLSL_VERSION_DECL}\n"
												"layout(location = 0) out highp vec4 fragColor;\n"
												"in mediump float v_alpha;\n"
												"void main (void)\n"
												"{\n"
												"	fragColor = vec4(1.0, 1.0, 1.0, v_alpha);\n"
												"}\n";
	const glw::Functions&		gl			=	m_context.getRenderContext().getFunctions();

	m_alphaProgram = new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources() << glu::VertexSource(specializeShader(m_context, vertexShaderSource)) << glu::FragmentSource(specializeShader(m_context, fragShaderSource)));

	if (!m_alphaProgram->isOk())
	{
		m_testCtx.getLog() << *m_alphaProgram;
		throw tcu::TestError("Could not create aplha program.");
	}

	m_alphaProgramPosLoc = gl.getAttribLocation(m_alphaProgram->getProgram(), "a_position");
}

void SampleMaskCase::updateTexture (int sample)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	// prepare draw

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fboID);
	gl.viewport(0, 0, m_canvasSize, m_canvasSize);
	gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);

	// clear all samples

	m_testCtx.getLog() << tcu::TestLog::Message << "Clearing image" << tcu::TestLog::EndMessage;
	gl.clear(GL_COLOR_BUFFER_BIT);

	// set mask state

	if (m_flags & FLAGS_HIGH_BITS)
	{
		const std::vector<deUint32> bitmask			= genSetNthBitSampleMask(sample);
		const std::vector<deUint32>	effectiveMask	= genAllSetToNthBitSampleMask(m_samples);
		std::vector<deUint32>		totalBitmask	(effectiveMask.size());

		DE_ASSERT((int)totalBitmask.size() == m_effectiveSampleMaskWordCount);

		// set some arbitrary high bits to non-effective bits
		for (int wordNdx = 0; wordNdx < (int)effectiveMask.size(); ++wordNdx)
		{
			const deUint32 randomMask	= (deUint32)deUint32Hash(wordNdx << 2 ^ sample);
			const deUint32 sampleMask	= (wordNdx < (int)bitmask.size()) ? (bitmask[wordNdx]) : (0);
			const deUint32 maskMask		= effectiveMask[wordNdx];

			totalBitmask[wordNdx] = (sampleMask & maskMask) | (randomMask & ~maskMask);
		}

		m_testCtx.getLog() << tcu::TestLog::Message << "Setting sample mask to 0b" << sampleMaskToString(totalBitmask, (int)totalBitmask.size() * 32) << tcu::TestLog::EndMessage;

		gl.enable(GL_SAMPLE_MASK);
		for (int wordNdx = 0; wordNdx < m_effectiveSampleMaskWordCount; ++wordNdx)
		{
			const GLbitfield wordmask = (wordNdx < (int)totalBitmask.size()) ? ((GLbitfield)(totalBitmask[wordNdx])) : (0);
			gl.sampleMaski((deUint32)wordNdx, wordmask);
		}
	}
	else
	{
		const std::vector<deUint32> bitmask = genSetNthBitSampleMask(sample);
		DE_ASSERT((int)bitmask.size() <= m_effectiveSampleMaskWordCount);

		m_testCtx.getLog() << tcu::TestLog::Message << "Setting sample mask to 0b" << sampleMaskToString(bitmask, m_samples) << tcu::TestLog::EndMessage;

		gl.enable(GL_SAMPLE_MASK);
		for (int wordNdx = 0; wordNdx < m_effectiveSampleMaskWordCount; ++wordNdx)
		{
			const GLbitfield wordmask = (wordNdx < (int)bitmask.size()) ? ((GLbitfield)(bitmask[wordNdx])) : (0);
			gl.sampleMaski((deUint32)wordNdx, wordmask);
		}
	}
	if (m_flags & FLAGS_ALPHA_TO_COVERAGE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Enabling alpha to coverage." << tcu::TestLog::EndMessage;
		gl.enable(GL_SAMPLE_ALPHA_TO_COVERAGE);
	}
	if (m_flags & FLAGS_SAMPLE_COVERAGE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Enabling sample coverage. Varying sample coverage for grid cells." << tcu::TestLog::EndMessage;
		gl.enable(GL_SAMPLE_COVERAGE);
	}

	// draw test pattern

	m_testCtx.getLog() << tcu::TestLog::Message << "Drawing test grid" << tcu::TestLog::EndMessage;

	gl.bindVertexArray			(m_vaoID);
	gl.bindBuffer				(GL_ARRAY_BUFFER, m_vboID);
	gl.vertexAttribPointer		(m_alphaProgramPosLoc, 4, GL_FLOAT, GL_FALSE, 0, DE_NULL);
	gl.enableVertexAttribArray	(m_alphaProgramPosLoc);
	GLU_EXPECT_NO_ERROR			(gl.getError(), "vertexAttribPointer");

	gl.useProgram				(m_alphaProgram->getProgram());

	for (int y = 0; y < m_gridsize; ++y)
	for (int x = 0; x < m_gridsize; ++x)
	{
		if (m_flags & FLAGS_SAMPLE_COVERAGE)
			gl.sampleCoverage((float)(y*m_gridsize + x) / float(m_gridsize*m_gridsize), GL_FALSE);

		gl.drawArrays				(GL_TRIANGLES, (y*m_gridsize + x) * 6, 6);
		GLU_EXPECT_NO_ERROR			(gl.getError(), "drawArrays");
	}

	// clean state

	gl.disableVertexAttribArray	(m_alphaProgramPosLoc);
	gl.useProgram				(0);
	gl.bindFramebuffer			(GL_FRAMEBUFFER, 0);
	gl.disable					(GL_SAMPLE_MASK);
	gl.disable					(GL_SAMPLE_ALPHA_TO_COVERAGE);
	gl.disable					(GL_SAMPLE_COVERAGE);
	GLU_EXPECT_NO_ERROR			(gl.getError(), "clean");
}

bool SampleMaskCase::verifyTexture (int sample)
{
	tcu::Surface	result		(m_canvasSize, m_canvasSize);
	tcu::Surface	errorMask	(m_canvasSize, m_canvasSize);
	bool			error		= false;

	tcu::clear(errorMask.getAccess(), tcu::RGBA::green().toVec());

	// Draw sample:
	//	Sample sampleNdx is set to red channel
	//	Other samples are set to green channel
	drawSample(result, sample);

	// Check surface contains only sampleNdx
	for (int y = 0; y < m_canvasSize; ++y)
	for (int x = 0; x < m_canvasSize; ++x)
	{
		const tcu::RGBA color					= result.getPixel(x, y);

		// Allow missing coverage with FLAGS_ALPHA_TO_COVERAGE and FLAGS_SAMPLE_COVERAGE, or if there are no samples enabled
		const bool		allowMissingCoverage	= ((m_flags & (FLAGS_ALPHA_TO_COVERAGE | FLAGS_SAMPLE_COVERAGE)) != 0) || (sample == -1);

		// disabled sample was written to
		if (color.getGreen() != 0)
		{
			error = true;
			errorMask.setPixel(x, y, tcu::RGBA::red());
		}
		// enabled sample was not written to
		else if (color.getRed() != 255 && !allowMissingCoverage)
		{
			error = true;
			errorMask.setPixel(x, y, tcu::RGBA::red());
		}
	}

	if (error)
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message << "Image verification failed, disabled samples found." << tcu::TestLog::EndMessage
			<< tcu::TestLog::ImageSet("VerificationResult", "Result of rendering")
			<< tcu::TestLog::Image("Result",	"Result",		result)
			<< tcu::TestLog::Image("ErrorMask",	"Error Mask",	errorMask)
			<< tcu::TestLog::EndImageSet;
		return false;
	}
	else
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Image verification ok, no disabled samples found." << tcu::TestLog::EndMessage;
		return true;
	}
}

void SampleMaskCase::drawSample (tcu::Surface& dst, int sample)
{
	// Downsample using only one sample
	static const tcu::Vec4 fullscreenQuad[] =
	{
		tcu::Vec4(-1.0f,  1.0f, 0.0f, 1.0f),
		tcu::Vec4(-1.0f, -1.0f, 0.0f, 1.0f),
		tcu::Vec4( 1.0f,  1.0f, 0.0f, 1.0f),
		tcu::Vec4( 1.0f, -1.0f, 0.0f, 1.0f)
	};

	const glw::Functions&	gl				= m_context.getRenderContext().getFunctions();
	glu::Buffer				vertexBuffer	(m_context.getRenderContext());

	gl.bindTexture				(GL_TEXTURE_2D_MULTISAMPLE, m_texID);
	gl.bindVertexArray			(m_vaoID);

	gl.bindBuffer				(GL_ARRAY_BUFFER, *vertexBuffer);
	gl.bufferData				(GL_ARRAY_BUFFER, sizeof(fullscreenQuad), &fullscreenQuad[0], GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR			(gl.getError(), "bufferData");

	gl.viewport					(0, 0, m_canvasSize, m_canvasSize);
	gl.clearColor				(0, 0, 0, 1);
	gl.clear					(GL_COLOR_BUFFER_BIT);
	gl.vertexAttribPointer		(m_samplerProgramPosLoc, 4, GL_FLOAT, GL_FALSE, 0, DE_NULL);
	gl.enableVertexAttribArray	(m_samplerProgramPosLoc);
	GLU_EXPECT_NO_ERROR			(gl.getError(), "vertexAttribPointer");

	gl.useProgram				(m_samplerProgram->getProgram());
	gl.uniform1i				(m_samplerProgramSamplerLoc, 0);
	gl.uniform1i				(m_samplerProgramSampleNdxLoc, (deInt32)sample);
	GLU_EXPECT_NO_ERROR			(gl.getError(), "useprogram");

	m_testCtx.getLog() << tcu::TestLog::Message << "Reading from texture with sampler shader, u_sample = " << sample << tcu::TestLog::EndMessage;

	gl.drawArrays				(GL_TRIANGLE_STRIP, 0, 4);
	GLU_EXPECT_NO_ERROR			(gl.getError(), "drawArrays");

	gl.disableVertexAttribArray	(m_samplerProgramPosLoc);
	gl.useProgram				(0);
	GLU_EXPECT_NO_ERROR			(gl.getError(), "cleanup");

	gl.finish					();
	glu::readPixels				(m_context.getRenderContext(), 0, 0, dst.getAccess());
	GLU_EXPECT_NO_ERROR			(gl.getError(), "readPixels");
}

class MultisampleTextureUsageCase : public TestCase
{
public:

	enum TextureType
	{
		TEXTURE_COLOR_2D = 0,
		TEXTURE_COLOR_2D_ARRAY,
		TEXTURE_INT_2D,
		TEXTURE_INT_2D_ARRAY,
		TEXTURE_UINT_2D,
		TEXTURE_UINT_2D_ARRAY,
		TEXTURE_DEPTH_2D,
		TEXTURE_DEPTH_2D_ARRAY,

		TEXTURE_LAST
	};

						MultisampleTextureUsageCase		(Context& ctx, const char* name, const char* desc, int samples, TextureType type);
						~MultisampleTextureUsageCase	(void);

private:
	void				init							(void);
	void				deinit							(void);
	IterateResult		iterate							(void);

	void				genDrawShader					(void);
	void				genSamplerShader				(void);

	void				renderToTexture					(float value);
	void				sampleTexture					(tcu::Surface& dst, float value);
	bool				verifyImage						(const tcu::Surface& dst);

	static const int	s_textureSize					= 256;
	static const int	s_textureArraySize				= 8;
	static const int	s_textureLayer					= 3;

	const TextureType	m_type;
	const int			m_numSamples;

	glw::GLuint			m_fboID;
	glw::GLuint			m_textureID;

	glu::ShaderProgram*	m_drawShader;
	glu::ShaderProgram*	m_samplerShader;

	const bool			m_isColorFormat;
	const bool			m_isSignedFormat;
	const bool			m_isUnsignedFormat;
	const bool			m_isDepthFormat;
	const bool			m_isArrayType;
};

MultisampleTextureUsageCase::MultisampleTextureUsageCase (Context& ctx, const char* name, const char* desc, int samples, TextureType type)
	: TestCase			(ctx, name, desc)
	, m_type			(type)
	, m_numSamples		(samples)
	, m_fboID			(0)
	, m_textureID		(0)
	, m_drawShader		(DE_NULL)
	, m_samplerShader	(DE_NULL)
	, m_isColorFormat	(m_type == TEXTURE_COLOR_2D	|| m_type == TEXTURE_COLOR_2D_ARRAY)
	, m_isSignedFormat	(m_type == TEXTURE_INT_2D	|| m_type == TEXTURE_INT_2D_ARRAY)
	, m_isUnsignedFormat(m_type == TEXTURE_UINT_2D	|| m_type == TEXTURE_UINT_2D_ARRAY)
	, m_isDepthFormat	(m_type == TEXTURE_DEPTH_2D	|| m_type == TEXTURE_DEPTH_2D_ARRAY)
	, m_isArrayType		(m_type == TEXTURE_COLOR_2D_ARRAY || m_type == TEXTURE_INT_2D_ARRAY || m_type == TEXTURE_UINT_2D_ARRAY || m_type == TEXTURE_DEPTH_2D_ARRAY)
{
	DE_ASSERT(m_type < TEXTURE_LAST);
}

MultisampleTextureUsageCase::~MultisampleTextureUsageCase (void)
{
	deinit();
}

void MultisampleTextureUsageCase::init (void)
{
	const glw::Functions&	gl				= m_context.getRenderContext().getFunctions();
	const glw::GLenum		internalFormat	= (m_isColorFormat) ? (GL_R8) : (m_isSignedFormat) ? (GL_R8I) : (m_isUnsignedFormat) ? (GL_R8UI) : (m_isDepthFormat) ? (GL_DEPTH_COMPONENT32F) : (0);
	const glw::GLenum		textureTarget	= (m_isArrayType) ? (GL_TEXTURE_2D_MULTISAMPLE_ARRAY) : (GL_TEXTURE_2D_MULTISAMPLE);
	const glw::GLenum		fboAttachment	= (m_isDepthFormat) ? (GL_DEPTH_ATTACHMENT) : (GL_COLOR_ATTACHMENT0);
	const bool				supportsES32	= glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));

	DE_ASSERT(internalFormat);

	// requirements

	if (m_isArrayType && !supportsES32 && !m_context.getContextInfo().isExtensionSupported("GL_OES_texture_storage_multisample_2d_array"))
		throw tcu::NotSupportedError("Test requires OES_texture_storage_multisample_2d_array extension");
	if (m_context.getRenderTarget().getWidth() < s_textureSize || m_context.getRenderTarget().getHeight() < s_textureSize)
		throw tcu::NotSupportedError("render target size must be at least " + de::toString(static_cast<int>(s_textureSize)) + "x" + de::toString(static_cast<int>(s_textureSize)));

	{
		glw::GLint maxSamples = 0;
		gl.getInternalformativ(textureTarget, internalFormat, GL_SAMPLES, 1, &maxSamples);

		if (m_numSamples > maxSamples)
			throw tcu::NotSupportedError("Requested sample count is greater than supported");

		m_testCtx.getLog() << tcu::TestLog::Message << "Max sample count for " << glu::getTextureFormatStr(internalFormat) << ": " << maxSamples << tcu::TestLog::EndMessage;
	}

	{
		GLint maxTextureSize = 0;
		gl.getIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);

		if (s_textureSize > maxTextureSize)
			throw tcu::NotSupportedError("Larger GL_MAX_TEXTURE_SIZE is required");
	}

	if (m_isArrayType)
	{
		GLint maxTextureLayers = 0;
		gl.getIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &maxTextureLayers);

		if (s_textureArraySize > maxTextureLayers)
			throw tcu::NotSupportedError("Larger GL_MAX_ARRAY_TEXTURE_LAYERS is required");
	}

	// create texture

	m_testCtx.getLog() << tcu::TestLog::Message << "Creating multisample " << ((m_isDepthFormat) ? ("depth") : ("")) << " texture" << ((m_isArrayType) ? (" array") : ("")) << tcu::TestLog::EndMessage;

	gl.genTextures(1, &m_textureID);
	gl.bindTexture(textureTarget, m_textureID);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bind texture");

	if (m_isArrayType)
		gl.texStorage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, m_numSamples, internalFormat, s_textureSize, s_textureSize, s_textureArraySize, GL_FALSE);
	else
		gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_numSamples, internalFormat, s_textureSize, s_textureSize, GL_FALSE);
	GLU_EXPECT_NO_ERROR(gl.getError(), "texstorage");

	// create fbo for drawing

	gl.genFramebuffers(1, &m_fboID);
	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fboID);

	if (m_isArrayType)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Attaching multisample texture array layer " << static_cast<int>(s_textureLayer) << " to fbo" << tcu::TestLog::EndMessage;
		gl.framebufferTextureLayer(GL_FRAMEBUFFER, fboAttachment, m_textureID, 0, s_textureLayer);
	}
	else
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Attaching multisample texture to fbo" << tcu::TestLog::EndMessage;
		gl.framebufferTexture2D(GL_FRAMEBUFFER, fboAttachment, textureTarget, m_textureID, 0);
	}
	GLU_EXPECT_NO_ERROR(gl.getError(), "gen fbo");

	// create shader for rendering to fbo
	genDrawShader();

	// create shader for sampling the texture rendered to
	genSamplerShader();
}

void MultisampleTextureUsageCase::deinit (void)
{
	if (m_textureID)
	{
		m_context.getRenderContext().getFunctions().deleteTextures(1, &m_textureID);
		m_textureID = 0;
	}

	if (m_fboID)
	{
		m_context.getRenderContext().getFunctions().deleteFramebuffers(1, &m_fboID);
		m_fboID = 0;
	}

	if (m_drawShader)
	{
		delete m_drawShader;
		m_drawShader = DE_NULL;
	}

	if (m_samplerShader)
	{
		delete m_samplerShader;
		m_samplerShader = DE_NULL;
	}
}

MultisampleTextureUsageCase::IterateResult MultisampleTextureUsageCase::iterate (void)
{
	const tcu::ScopedLogSection	section			(m_testCtx.getLog(), "Sample", "Render to texture and sample texture");
	tcu::Surface				result			(s_textureSize, s_textureSize);
	const float					minValue		= (m_isColorFormat || m_isDepthFormat) ? (0.0f) : (m_isSignedFormat) ? (-100.0f) : (m_isUnsignedFormat) ? (0.0f)	: ( 1.0f);
	const float					maxValue		= (m_isColorFormat || m_isDepthFormat) ? (1.0f) : (m_isSignedFormat) ? ( 100.0f) : (m_isUnsignedFormat) ? (200.0f)	: (-1.0f);
	de::Random					rnd				(deUint32Hash((deUint32)m_type));
	const float					rawValue		= rnd.getFloat(minValue, maxValue);
	const float					preparedValue	= (m_isSignedFormat || m_isUnsignedFormat) ? (deFloatFloor(rawValue)) : (rawValue);

	// draw to fbo with a random value

	renderToTexture(preparedValue);

	// draw from texture to front buffer

	sampleTexture(result, preparedValue);

	// result is ok?

	if (verifyImage(result))
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Image verification failed");

	return STOP;
}

void MultisampleTextureUsageCase::genDrawShader (void)
{
	const tcu::ScopedLogSection section(m_testCtx.getLog(), "RenderShader", "Generate render-to-texture shader");

	static const char* const	vertexShaderSource =		"${GLSL_VERSION_DECL}\n"
															"in highp vec4 a_position;\n"
															"void main (void)\n"
															"{\n"
															"	gl_Position = a_position;\n"
															"}\n";
	static const char* const	fragmentShaderSourceColor =	"${GLSL_VERSION_DECL}\n"
															"layout(location = 0) out highp ${OUTTYPE} fragColor;\n"
															"uniform highp float u_writeValue;\n"
															"void main (void)\n"
															"{\n"
															"	fragColor = ${OUTTYPE}(vec4(u_writeValue, 1.0, 1.0, 1.0));\n"
															"}\n";
	static const char* const	fragmentShaderSourceDepth =	"${GLSL_VERSION_DECL}\n"
															"layout(location = 0) out highp vec4 fragColor;\n"
															"uniform highp float u_writeValue;\n"
															"void main (void)\n"
															"{\n"
															"	fragColor = vec4(0.0, 0.0, 0.0, 1.0);\n"
															"	gl_FragDepth = u_writeValue;\n"
															"}\n";
	const char* const			fragmentSource =			(m_isDepthFormat) ? (fragmentShaderSourceDepth) : (fragmentShaderSourceColor);

	std::map<std::string, std::string> fragmentArguments;

	const glu::GLSLVersion glslVersion = glu::getContextTypeGLSLVersion(m_context.getRenderContext().getType());
	fragmentArguments["GLSL_VERSION_DECL"] = glu::getGLSLVersionDeclaration(glslVersion);

	if (m_isColorFormat || m_isDepthFormat)
		fragmentArguments["OUTTYPE"] = "vec4";
	else if (m_isSignedFormat)
		fragmentArguments["OUTTYPE"] = "ivec4";
	else if (m_isUnsignedFormat)
		fragmentArguments["OUTTYPE"] = "uvec4";
	else
		DE_ASSERT(DE_FALSE);

	m_drawShader = new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources() << glu::VertexSource(specializeShader(m_context, vertexShaderSource)) << glu::FragmentSource(tcu::StringTemplate(fragmentSource).specialize(fragmentArguments)));
	m_testCtx.getLog() << *m_drawShader;

	if (!m_drawShader->isOk())
		throw tcu::TestError("could not build shader");
}

void MultisampleTextureUsageCase::genSamplerShader (void)
{
	const tcu::ScopedLogSection section(m_testCtx.getLog(), "SamplerShader", "Generate texture sampler shader");

	static const char* const vertexShaderSource =	"${GLSL_VERSION_DECL}\n"
													"in highp vec4 a_position;\n"
													"out highp float v_gradient;\n"
													"void main (void)\n"
													"{\n"
													"	gl_Position = a_position;\n"
													"	v_gradient = a_position.x * 0.5 + 0.5;\n"
													"}\n";
	static const char* const fragmentShaderSource =	"${GLSL_VERSION_DECL}\n"
													"${EXTENSION_STATEMENT}"
													"layout(location = 0) out highp vec4 fragColor;\n"
													"uniform highp ${SAMPLERTYPE} u_sampler;\n"
													"uniform highp int u_maxSamples;\n"
													"uniform highp int u_layer;\n"
													"uniform highp float u_cmpValue;\n"
													"in highp float v_gradient;\n"
													"void main (void)\n"
													"{\n"
													"	const highp vec4 okColor = vec4(0.0, 1.0, 0.0, 1.0);\n"
													"	const highp vec4 failColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
													"	const highp float epsilon = ${EPSILON};\n"
													"\n"
													"	highp int sampleNdx = clamp(int(floor(v_gradient * float(u_maxSamples))), 0, u_maxSamples-1);\n"
													"	highp float value = float(texelFetch(u_sampler, ${FETCHPOS}, sampleNdx).r);\n"
													"	fragColor = (abs(u_cmpValue - value) < epsilon) ? (okColor) : (failColor);\n"
													"}\n";

	std::map<std::string, std::string> fragmentArguments;

	const glu::GLSLVersion glslVersion = glu::getContextTypeGLSLVersion(m_context.getRenderContext().getType());
	fragmentArguments["GLSL_VERSION_DECL"] = glu::getGLSLVersionDeclaration(glslVersion);

	if (m_isArrayType)
		fragmentArguments["FETCHPOS"] = "ivec3(floor(gl_FragCoord.xy), u_layer)";
	else
		fragmentArguments["FETCHPOS"] = "ivec2(floor(gl_FragCoord.xy))";

	if (m_isColorFormat || m_isDepthFormat)
		fragmentArguments["EPSILON"] = "0.1";
	else
		fragmentArguments["EPSILON"] = "1.0";

	if (m_isArrayType && !glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)))
		fragmentArguments["EXTENSION_STATEMENT"] = "#extension GL_OES_texture_storage_multisample_2d_array : require\n";
	else
		fragmentArguments["EXTENSION_STATEMENT"] = "";

	switch (m_type)
	{
		case TEXTURE_COLOR_2D:			fragmentArguments["SAMPLERTYPE"] = "sampler2DMS";		break;
		case TEXTURE_COLOR_2D_ARRAY:	fragmentArguments["SAMPLERTYPE"] = "sampler2DMSArray";	break;
		case TEXTURE_INT_2D:			fragmentArguments["SAMPLERTYPE"] = "isampler2DMS";		break;
		case TEXTURE_INT_2D_ARRAY:		fragmentArguments["SAMPLERTYPE"] = "isampler2DMSArray";	break;
		case TEXTURE_UINT_2D:			fragmentArguments["SAMPLERTYPE"] = "usampler2DMS";		break;
		case TEXTURE_UINT_2D_ARRAY:		fragmentArguments["SAMPLERTYPE"] = "usampler2DMSArray";	break;
		case TEXTURE_DEPTH_2D:			fragmentArguments["SAMPLERTYPE"] = "sampler2DMS";		break;
		case TEXTURE_DEPTH_2D_ARRAY:	fragmentArguments["SAMPLERTYPE"] = "sampler2DMSArray";	break;

		default:
			DE_ASSERT(DE_FALSE);
	}

	m_samplerShader = new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources() << glu::VertexSource(specializeShader(m_context, vertexShaderSource)) << glu::FragmentSource(tcu::StringTemplate(fragmentShaderSource).specialize(fragmentArguments)));
	m_testCtx.getLog() << *m_samplerShader;

	if (!m_samplerShader->isOk())
		throw tcu::TestError("could not build shader");
}

void MultisampleTextureUsageCase::renderToTexture (float value)
{
	static const tcu::Vec4 fullscreenQuad[] =
	{
		tcu::Vec4(-1.0f, -1.0f, 0.0f, 1.0f),
		tcu::Vec4(-1.0f,  1.0f, 0.0f, 1.0f),
		tcu::Vec4( 1.0f, -1.0f, 0.0f, 1.0f),
		tcu::Vec4( 1.0f,  1.0f, 0.0f, 1.0f),
	};

	const glw::Functions&	gl					= m_context.getRenderContext().getFunctions();
	const int				posLocation			= gl.getAttribLocation(m_drawShader->getProgram(), "a_position");
	const int				valueLocation		= gl.getUniformLocation(m_drawShader->getProgram(), "u_writeValue");
	glu::Buffer				vertexAttibBuffer	(m_context.getRenderContext());

	m_testCtx.getLog() << tcu::TestLog::Message << "Filling multisample texture with value " << value  << tcu::TestLog::EndMessage;

	// upload data

	gl.bindBuffer(GL_ARRAY_BUFFER, *vertexAttibBuffer);
	gl.bufferData(GL_ARRAY_BUFFER, sizeof(fullscreenQuad), fullscreenQuad[0].getPtr(), GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bufferdata");

	// clear buffer

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fboID);
	gl.viewport(0, 0, s_textureSize, s_textureSize);

	if (m_isColorFormat)
	{
		const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		gl.clearBufferfv(GL_COLOR, 0, clearColor);
	}
	else if (m_isSignedFormat)
	{
		const deInt32 clearColor[4] = { 0, 0, 0, 0 };
		gl.clearBufferiv(GL_COLOR, 0, clearColor);
	}
	else if (m_isUnsignedFormat)
	{
		const deUint32 clearColor[4] = { 0, 0, 0, 0 };
		gl.clearBufferuiv(GL_COLOR, 0, clearColor);
	}
	else if (m_isDepthFormat)
	{
		const float clearDepth = 0.5f;
		gl.clearBufferfv(GL_DEPTH, 0, &clearDepth);
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "clear buffer");

	// setup shader and draw

	gl.vertexAttribPointer(posLocation, 4, GL_FLOAT, GL_FALSE, 0, DE_NULL);
	gl.enableVertexAttribArray(posLocation);

	gl.useProgram(m_drawShader->getProgram());
	gl.uniform1f(valueLocation, value);

	GLU_EXPECT_NO_ERROR(gl.getError(), "setup draw");

	if (m_isDepthFormat)
	{
		gl.enable(GL_DEPTH_TEST);
		gl.depthFunc(GL_ALWAYS);
	}

	gl.drawArrays(GL_TRIANGLE_STRIP, 0, 4);
	GLU_EXPECT_NO_ERROR(gl.getError(), "draw");

	// clean state

	if (m_isDepthFormat)
		gl.disable(GL_DEPTH_TEST);

	gl.disableVertexAttribArray(posLocation);
	gl.useProgram(0);
	gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "clean");
}

void MultisampleTextureUsageCase::sampleTexture (tcu::Surface& dst, float value)
{
	static const tcu::Vec4 fullscreenQuad[] =
	{
		tcu::Vec4(-1.0f, -1.0f, 0.0f, 1.0f),
		tcu::Vec4(-1.0f,  1.0f, 0.0f, 1.0f),
		tcu::Vec4( 1.0f, -1.0f, 0.0f, 1.0f),
		tcu::Vec4( 1.0f,  1.0f, 0.0f, 1.0f),
	};

	const glw::Functions&	gl					= m_context.getRenderContext().getFunctions();
	const int				posLocation			= gl.getAttribLocation(m_samplerShader->getProgram(), "a_position");
	const int				samplerLocation		= gl.getUniformLocation(m_samplerShader->getProgram(), "u_sampler");
	const int				maxSamplesLocation	= gl.getUniformLocation(m_samplerShader->getProgram(), "u_maxSamples");
	const int				layerLocation		= gl.getUniformLocation(m_samplerShader->getProgram(), "u_layer");
	const int				valueLocation		= gl.getUniformLocation(m_samplerShader->getProgram(), "u_cmpValue");
	const glw::GLenum		textureTarget		= (m_isArrayType) ? (GL_TEXTURE_2D_MULTISAMPLE_ARRAY) : (GL_TEXTURE_2D_MULTISAMPLE);
	glu::Buffer				vertexAttibBuffer	(m_context.getRenderContext());

	m_testCtx.getLog() << tcu::TestLog::Message << "Sampling from texture." << tcu::TestLog::EndMessage;

	// upload data

	gl.bindBuffer(GL_ARRAY_BUFFER, *vertexAttibBuffer);
	gl.bufferData(GL_ARRAY_BUFFER, sizeof(fullscreenQuad), fullscreenQuad[0].getPtr(), GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bufferdata");

	// clear

	gl.viewport(0, 0, s_textureSize, s_textureSize);
	gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
	gl.clear(GL_COLOR_BUFFER_BIT);

	// setup shader and draw

	gl.vertexAttribPointer(posLocation, 4, GL_FLOAT, GL_FALSE, 0, DE_NULL);
	gl.enableVertexAttribArray(posLocation);

	gl.useProgram(m_samplerShader->getProgram());
	gl.uniform1i(samplerLocation, 0);
	gl.uniform1i(maxSamplesLocation, m_numSamples);
	if (m_isArrayType)
		gl.uniform1i(layerLocation, s_textureLayer);
	gl.uniform1f(valueLocation, value);
	gl.bindTexture(textureTarget, m_textureID);
	GLU_EXPECT_NO_ERROR(gl.getError(), "setup draw");

	gl.drawArrays(GL_TRIANGLE_STRIP, 0, 4);
	GLU_EXPECT_NO_ERROR(gl.getError(), "draw");

	// clean state

	gl.disableVertexAttribArray(posLocation);
	gl.useProgram(0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "clean");

	// read results
	gl.finish();
	glu::readPixels(m_context.getRenderContext(), 0, 0, dst.getAccess());
}

bool MultisampleTextureUsageCase::verifyImage (const tcu::Surface& dst)
{
	bool error = false;

	m_testCtx.getLog() << tcu::TestLog::Message << "Verifying image." << tcu::TestLog::EndMessage;

	for (int y = 0; y < dst.getHeight(); ++y)
	for (int x = 0; x < dst.getWidth(); ++x)
	{
		const tcu::RGBA color				= dst.getPixel(x, y);
		const int		colorThresholdRed	= 1 << (8 - m_context.getRenderTarget().getPixelFormat().redBits);
		const int		colorThresholdGreen	= 1 << (8 - m_context.getRenderTarget().getPixelFormat().greenBits);
		const int		colorThresholdBlue	= 1 << (8 - m_context.getRenderTarget().getPixelFormat().blueBits);

		// only green is accepted
		if (color.getRed() > colorThresholdRed || color.getGreen() < 255 - colorThresholdGreen || color.getBlue() > colorThresholdBlue)
			error = true;
	}

	if (error)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Invalid pixels found." << tcu::TestLog::EndMessage;
		m_testCtx.getLog()
			<< tcu::TestLog::ImageSet("Verification result", "Result of rendering")
			<< tcu::TestLog::Image("Result", "Result", dst)
			<< tcu::TestLog::EndImageSet;

		return false;
	}
	else
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "No invalid pixels found." << tcu::TestLog::EndMessage;
		return true;
	}
}

class NegativeFramebufferCase : public TestCase
{
public:
	enum CaseType
	{
		CASE_DIFFERENT_N_SAMPLES_TEX = 0,
		CASE_DIFFERENT_N_SAMPLES_RBO,
		CASE_DIFFERENT_FIXED_TEX,
		CASE_DIFFERENT_FIXED_RBO,
		CASE_NON_ZERO_LEVEL,

		CASE_LAST
	};

						NegativeFramebufferCase		(Context& context, const char* name, const char* desc, CaseType caseType);
						~NegativeFramebufferCase	(void);

private:
	void				init						(void);
	void				deinit						(void);
	IterateResult		iterate						(void);

	void				getFormatSamples			(glw::GLenum target, std::vector<int>& samples);

	const CaseType		m_caseType;
	const int			m_fboSize;
	const glw::GLenum	m_internalFormat;

	int					m_numSamples0;	// !< samples for attachment 0
	int					m_numSamples1;	// !< samples for attachment 1
};

NegativeFramebufferCase::NegativeFramebufferCase (Context& context, const char* name, const char* desc, CaseType caseType)
	: TestCase			(context, name, desc)
	, m_caseType		(caseType)
	, m_fboSize			(64)
	, m_internalFormat	(GL_RGBA8)
	, m_numSamples0		(-1)
	, m_numSamples1		(-1)
{
}

NegativeFramebufferCase::~NegativeFramebufferCase (void)
{
	deinit();
}

void NegativeFramebufferCase::init (void)
{
	const bool				colorAttachmentTexture	= (m_caseType == CASE_DIFFERENT_N_SAMPLES_TEX) || (m_caseType == CASE_DIFFERENT_FIXED_TEX);
	const bool				colorAttachmentRbo		= (m_caseType == CASE_DIFFERENT_N_SAMPLES_RBO) || (m_caseType == CASE_DIFFERENT_FIXED_RBO);
	const bool				useDifferentSampleCounts= (m_caseType == CASE_DIFFERENT_N_SAMPLES_TEX) || (m_caseType == CASE_DIFFERENT_N_SAMPLES_RBO);
	std::vector<int>		textureSamples;
	std::vector<int>		rboSamples;

	getFormatSamples(GL_TEXTURE_2D_MULTISAMPLE, textureSamples);
	getFormatSamples(GL_RENDERBUFFER, rboSamples);

	TCU_CHECK(!textureSamples.empty());
	TCU_CHECK(!rboSamples.empty());

	// select sample counts

	if (useDifferentSampleCounts)
	{
		if (colorAttachmentTexture)
		{
			m_numSamples0 = textureSamples[0];

			if (textureSamples.size() >= 2)
				m_numSamples1 = textureSamples[1];
			else
				throw tcu::NotSupportedError("Test requires multiple supported sample counts");
		}
		else if (colorAttachmentRbo)
		{
			for (int texNdx = 0; texNdx < (int)textureSamples.size(); ++texNdx)
			for (int rboNdx = 0; rboNdx < (int)rboSamples.size(); ++rboNdx)
			{
				if (textureSamples[texNdx] != rboSamples[rboNdx])
				{
					m_numSamples0 = textureSamples[texNdx];
					m_numSamples1 = rboSamples[rboNdx];
					return;
				}
			}

			throw tcu::NotSupportedError("Test requires multiple supported sample counts");
		}
		else
			DE_ASSERT(DE_FALSE);
	}
	else
	{
		if (colorAttachmentTexture)
		{
			m_numSamples0 = textureSamples[0];
			m_numSamples1 = textureSamples[0];
		}
		else if (colorAttachmentRbo)
		{
			for (int texNdx = 0; texNdx < (int)textureSamples.size(); ++texNdx)
			for (int rboNdx = 0; rboNdx < (int)rboSamples.size(); ++rboNdx)
			{
				if (textureSamples[texNdx] == rboSamples[rboNdx])
				{
					m_numSamples0 = textureSamples[texNdx];
					m_numSamples1 = rboSamples[rboNdx];
					return;
				}
			}

			throw tcu::NotSupportedError("Test requires a sample count supported in both rbo and texture");
		}
		else
		{
			m_numSamples0 = textureSamples[0];
		}
	}
}

void NegativeFramebufferCase::deinit (void)
{
}

NegativeFramebufferCase::IterateResult NegativeFramebufferCase::iterate (void)
{
	const bool				colorAttachmentTexture	= (m_caseType == CASE_DIFFERENT_N_SAMPLES_TEX) || (m_caseType == CASE_DIFFERENT_FIXED_TEX);
	const bool				colorAttachmentRbo		= (m_caseType == CASE_DIFFERENT_N_SAMPLES_RBO) || (m_caseType == CASE_DIFFERENT_FIXED_RBO);
	const glw::GLboolean	fixedSampleLocations0	= (m_caseType == CASE_DIFFERENT_N_SAMPLES_RBO) ? (GL_TRUE) : (GL_FALSE);
	const glw::GLboolean	fixedSampleLocations1	= ((m_caseType == CASE_DIFFERENT_FIXED_TEX) || (m_caseType == CASE_DIFFERENT_FIXED_RBO)) ? (GL_TRUE) : (GL_FALSE);
	glu::CallLogWrapper		gl						(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	glw::GLuint				fboId					= 0;
	glw::GLuint				rboId					= 0;
	glw::GLuint				tex0Id					= 0;
	glw::GLuint				tex1Id					= 0;

	bool					testFailed				= false;

	gl.enableLogging(true);

	try
	{
		gl.glGenFramebuffers(1, &fboId);
		gl.glBindFramebuffer(GL_FRAMEBUFFER, fboId);
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "gen fbo");

		gl.glGenTextures(1, &tex0Id);
		gl.glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, tex0Id);
		gl.glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_numSamples0, m_internalFormat, m_fboSize, m_fboSize, fixedSampleLocations0);
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "gen texture 0");

		if (m_caseType == CASE_NON_ZERO_LEVEL)
		{
			glw::GLenum error;

			// attaching non-zero level generates invalid value
			gl.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, tex0Id, 5);
			error = gl.glGetError();

			if (error != GL_INVALID_VALUE)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "ERROR! Expected GL_INVALID_VALUE, got " << glu::getErrorStr(error) << tcu::TestLog::EndMessage;
				testFailed = true;
			}
		}
		else
		{
			gl.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, tex0Id, 0);
			GLU_EXPECT_NO_ERROR(gl.glGetError(), "attach to c0");

			if (colorAttachmentTexture)
			{
				gl.glGenTextures(1, &tex1Id);
				gl.glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, tex1Id);
				gl.glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_numSamples1, m_internalFormat, m_fboSize, m_fboSize, fixedSampleLocations1);
				GLU_EXPECT_NO_ERROR(gl.glGetError(), "gen texture 1");

				gl.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D_MULTISAMPLE, tex1Id, 0);
				GLU_EXPECT_NO_ERROR(gl.glGetError(), "attach to c1");
			}
			else if (colorAttachmentRbo)
			{
				gl.glGenRenderbuffers(1, &rboId);
				gl.glBindRenderbuffer(GL_RENDERBUFFER, rboId);
				gl.glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_numSamples1, m_internalFormat, m_fboSize, m_fboSize);
				GLU_EXPECT_NO_ERROR(gl.glGetError(), "gen rb");

				gl.glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_RENDERBUFFER, rboId);
				GLU_EXPECT_NO_ERROR(gl.glGetError(), "attach to c1");
			}
			else
				DE_ASSERT(DE_FALSE);

			// should not be complete
			{
				glw::GLenum status = gl.glCheckFramebufferStatus(GL_FRAMEBUFFER);

				if (status != GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE)
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "ERROR! Expected GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE, got " << glu::getFramebufferStatusName(status) << tcu::TestLog::EndMessage;
					testFailed = true;
				}
			}
		}
	}
	catch (...)
	{
		gl.glDeleteFramebuffers(1, &fboId);
		gl.glDeleteRenderbuffers(1, &rboId);
		gl.glDeleteTextures(1, &tex0Id);
		gl.glDeleteTextures(1, &tex1Id);
		throw;
	}

	gl.glDeleteFramebuffers(1, &fboId);
	gl.glDeleteRenderbuffers(1, &rboId);
	gl.glDeleteTextures(1, &tex0Id);
	gl.glDeleteTextures(1, &tex1Id);

	if (testFailed)
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Got wrong error code");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

void NegativeFramebufferCase::getFormatSamples (glw::GLenum target, std::vector<int>& samples)
{
	const glw::Functions	gl			= m_context.getRenderContext().getFunctions();
	int						sampleCount	= 0;

	gl.getInternalformativ(target, m_internalFormat, GL_NUM_SAMPLE_COUNTS, 1, &sampleCount);
	samples.resize(sampleCount);

	if (sampleCount > 0)
	{
		gl.getInternalformativ(target, m_internalFormat, GL_SAMPLES, sampleCount, &samples[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "get max samples");
	}
}

class NegativeTexParameterCase : public TestCase
{
public:
	enum TexParam
	{
		TEXTURE_MIN_FILTER = 0,
		TEXTURE_MAG_FILTER,
		TEXTURE_WRAP_S,
		TEXTURE_WRAP_T,
		TEXTURE_WRAP_R,
		TEXTURE_MIN_LOD,
		TEXTURE_MAX_LOD,
		TEXTURE_COMPARE_MODE,
		TEXTURE_COMPARE_FUNC,
		TEXTURE_BASE_LEVEL,

		TEXTURE_LAST
	};

					NegativeTexParameterCase	(Context& context, const char* name, const char* desc, TexParam param);
					~NegativeTexParameterCase	(void);

private:
	void			init						(void);
	void			deinit						(void);
	IterateResult	iterate						(void);

	glw::GLenum		getParamGLEnum				(void) const;
	glw::GLint		getParamValue				(void) const;
	glw::GLenum		getExpectedError			(void) const;

	const TexParam	m_texParam;
	int				m_iteration;
};

NegativeTexParameterCase::NegativeTexParameterCase (Context& context, const char* name, const char* desc, TexParam param)
	: TestCase		(context, name, desc)
	, m_texParam	(param)
	, m_iteration	(0)
{
	DE_ASSERT(param < TEXTURE_LAST);
}

NegativeTexParameterCase::~NegativeTexParameterCase	(void)
{
	deinit();
}

void NegativeTexParameterCase::init (void)
{
	// default value
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
}

void NegativeTexParameterCase::deinit (void)
{
}

NegativeTexParameterCase::IterateResult NegativeTexParameterCase::iterate (void)
{
	static const struct TextureType
	{
		const char*	name;
		glw::GLenum	target;
		glw::GLenum	internalFormat;
		bool		isArrayType;
	} types[] =
	{
		{ "color",					GL_TEXTURE_2D_MULTISAMPLE,			GL_RGBA8,	false	},
		{ "color array",			GL_TEXTURE_2D_MULTISAMPLE_ARRAY,	GL_RGBA8,	true	},
		{ "signed integer",			GL_TEXTURE_2D_MULTISAMPLE,			GL_R8I,		false	},
		{ "signed integer array",	GL_TEXTURE_2D_MULTISAMPLE_ARRAY,	GL_R8I,		true	},
		{ "unsigned integer",		GL_TEXTURE_2D_MULTISAMPLE,			GL_R8UI,	false	},
		{ "unsigned integer array",	GL_TEXTURE_2D_MULTISAMPLE_ARRAY,	GL_R8UI,	true	},
	};

	const tcu::ScopedLogSection scope(m_testCtx.getLog(), "Iteration", std::string() + "Testing parameter with " + types[m_iteration].name + " texture");
	const bool					supportsES32 = glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));

	if (types[m_iteration].isArrayType && !supportsES32 && !m_context.getContextInfo().isExtensionSupported("GL_OES_texture_storage_multisample_2d_array"))
		m_testCtx.getLog() << tcu::TestLog::Message << "GL_OES_texture_storage_multisample_2d_array not supported, skipping target" << tcu::TestLog::EndMessage;
	else
	{
		glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
		glu::Texture			texture	(m_context.getRenderContext());
		glw::GLenum				error;

		gl.enableLogging(true);

		// gen texture

		gl.glBindTexture(types[m_iteration].target, *texture);

		if (types[m_iteration].isArrayType)
			gl.glTexStorage3DMultisample(types[m_iteration].target, 1, types[m_iteration].internalFormat, 16, 16, 16, GL_FALSE);
		else
			gl.glTexStorage2DMultisample(types[m_iteration].target, 1, types[m_iteration].internalFormat, 16, 16, GL_FALSE);
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "setup texture");

		// set param

		gl.glTexParameteri(types[m_iteration].target, getParamGLEnum(), getParamValue());
		error = gl.glGetError();

		// expect failure

		if (error != getExpectedError())
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Got unexpected error: " << glu::getErrorStr(error) << ", expected: " << glu::getErrorStr(getExpectedError()) << tcu::TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Got wrong error code");
		}
	}

	if (++m_iteration < DE_LENGTH_OF_ARRAY(types))
		return CONTINUE;
	return STOP;
}

glw::GLenum NegativeTexParameterCase::getParamGLEnum (void) const
{
	switch (m_texParam)
	{
		case TEXTURE_MIN_FILTER:	return GL_TEXTURE_MIN_FILTER;
		case TEXTURE_MAG_FILTER:	return GL_TEXTURE_MAG_FILTER;
		case TEXTURE_WRAP_S:		return GL_TEXTURE_WRAP_S;
		case TEXTURE_WRAP_T:		return GL_TEXTURE_WRAP_T;
		case TEXTURE_WRAP_R:		return GL_TEXTURE_WRAP_R;
		case TEXTURE_MIN_LOD:		return GL_TEXTURE_MIN_LOD;
		case TEXTURE_MAX_LOD:		return GL_TEXTURE_MAX_LOD;
		case TEXTURE_COMPARE_MODE:	return GL_TEXTURE_COMPARE_MODE;
		case TEXTURE_COMPARE_FUNC:	return GL_TEXTURE_COMPARE_FUNC;
		case TEXTURE_BASE_LEVEL:	return GL_TEXTURE_BASE_LEVEL;
		default:
			DE_ASSERT(DE_FALSE);
			return 0;
	}
}

glw::GLint NegativeTexParameterCase::getParamValue (void) const
{
	switch (m_texParam)
	{
		case TEXTURE_MIN_FILTER:	return GL_LINEAR;
		case TEXTURE_MAG_FILTER:	return GL_LINEAR;
		case TEXTURE_WRAP_S:		return GL_CLAMP_TO_EDGE;
		case TEXTURE_WRAP_T:		return GL_CLAMP_TO_EDGE;
		case TEXTURE_WRAP_R:		return GL_CLAMP_TO_EDGE;
		case TEXTURE_MIN_LOD:		return 1;
		case TEXTURE_MAX_LOD:		return 5;
		case TEXTURE_COMPARE_MODE:	return GL_NONE;
		case TEXTURE_COMPARE_FUNC:	return GL_NOTEQUAL;
		case TEXTURE_BASE_LEVEL:	return 2;
		default:
			DE_ASSERT(DE_FALSE);
			return 0;
	}
}

glw::GLenum NegativeTexParameterCase::getExpectedError (void) const
{
	switch (m_texParam)
	{
		case TEXTURE_MIN_FILTER:	return GL_INVALID_ENUM;
		case TEXTURE_MAG_FILTER:	return GL_INVALID_ENUM;
		case TEXTURE_WRAP_S:		return GL_INVALID_ENUM;
		case TEXTURE_WRAP_T:		return GL_INVALID_ENUM;
		case TEXTURE_WRAP_R:		return GL_INVALID_ENUM;
		case TEXTURE_MIN_LOD:		return GL_INVALID_ENUM;
		case TEXTURE_MAX_LOD:		return GL_INVALID_ENUM;
		case TEXTURE_COMPARE_MODE:	return GL_INVALID_ENUM;
		case TEXTURE_COMPARE_FUNC:	return GL_INVALID_ENUM;
		case TEXTURE_BASE_LEVEL:	return GL_INVALID_OPERATION;
		default:
			DE_ASSERT(DE_FALSE);
			return 0;
	}
}

class NegativeTexureSampleCase : public TestCase
{
public:
	enum SampleCountParam
	{
		SAMPLECOUNT_HIGH = 0,
		SAMPLECOUNT_ZERO,

		SAMPLECOUNT_LAST
	};

							NegativeTexureSampleCase	(Context& context, const char* name, const char* desc, SampleCountParam param);
private:
	IterateResult			iterate						(void);

	const SampleCountParam	m_sampleParam;
};

NegativeTexureSampleCase::NegativeTexureSampleCase (Context& context, const char* name, const char* desc, SampleCountParam param)
	: TestCase		(context, name, desc)
	, m_sampleParam	(param)
{
	DE_ASSERT(param < SAMPLECOUNT_LAST);
}

NegativeTexureSampleCase::IterateResult NegativeTexureSampleCase::iterate (void)
{
	const glw::GLenum		expectedError	= (m_sampleParam == SAMPLECOUNT_HIGH) ? (GL_INVALID_OPERATION) : (GL_INVALID_VALUE);
	glu::CallLogWrapper		gl				(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	glu::Texture			texture			(m_context.getRenderContext());
	glw::GLenum				error;
	int						samples			= -1;

	gl.enableLogging(true);

	// calc samples

	if (m_sampleParam == SAMPLECOUNT_HIGH)
	{
		int maxSamples = 0;

		gl.glGetInternalformativ(GL_TEXTURE_2D_MULTISAMPLE, GL_RGBA8, GL_SAMPLES, 1, &maxSamples);
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "glGetInternalformativ");

		samples = maxSamples + 1;
	}
	else if (m_sampleParam == SAMPLECOUNT_ZERO)
		samples = 0;
	else
		DE_ASSERT(DE_FALSE);

	// create texture with bad values

	gl.glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, *texture);
	gl.glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGBA8, 64, 64, GL_FALSE);
	error = gl.glGetError();

	// expect failure

	if (error == expectedError)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Got unexpected error: " << glu::getErrorStr(error) << ", expected: " << glu::getErrorStr(expectedError) << tcu::TestLog::EndMessage;
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Got wrong error code");
	}

	return STOP;
}


} // anonymous

TextureMultisampleTests::TextureMultisampleTests (Context& context)
	: TestCaseGroup(context, "multisample", "Multisample texture tests")
{
}

TextureMultisampleTests::~TextureMultisampleTests (void)
{
}

void TextureMultisampleTests::init (void)
{
	static const int sampleCounts[] = { 1, 2, 3, 4, 8, 10, 12, 13, 16, 64 };

	static const struct TextureType
	{
		const char*									name;
		MultisampleTextureUsageCase::TextureType	type;
	} textureTypes[] =
	{
		{ "texture_color_2d",		MultisampleTextureUsageCase::TEXTURE_COLOR_2D		},
		{ "texture_color_2d_array",	MultisampleTextureUsageCase::TEXTURE_COLOR_2D_ARRAY	},
		{ "texture_int_2d",			MultisampleTextureUsageCase::TEXTURE_INT_2D			},
		{ "texture_int_2d_array",	MultisampleTextureUsageCase::TEXTURE_INT_2D_ARRAY	},
		{ "texture_uint_2d",		MultisampleTextureUsageCase::TEXTURE_UINT_2D		},
		{ "texture_uint_2d_array",	MultisampleTextureUsageCase::TEXTURE_UINT_2D_ARRAY	},
		{ "texture_depth_2d",		MultisampleTextureUsageCase::TEXTURE_DEPTH_2D		},
		{ "texture_depth_2d_array",	MultisampleTextureUsageCase::TEXTURE_DEPTH_2D_ARRAY	},
	};

	// .samples_x
	for (int sampleNdx = 0; sampleNdx < DE_LENGTH_OF_ARRAY(sampleCounts); ++sampleNdx)
	{
		tcu::TestCaseGroup* const sampleGroup = new tcu::TestCaseGroup(m_testCtx, (std::string("samples_") + de::toString(sampleCounts[sampleNdx])).c_str(), "Test with N samples");
		addChild(sampleGroup);

		// position query works
		sampleGroup->addChild(new SamplePosRasterizationTest(m_context, "sample_position", "test SAMPLE_POSITION", sampleCounts[sampleNdx]));

		// sample mask is ANDed properly
		sampleGroup->addChild(new SampleMaskCase(m_context, "sample_mask_only",											"Test with SampleMask only",									sampleCounts[sampleNdx],	SampleMaskCase::FLAGS_NONE));
		sampleGroup->addChild(new SampleMaskCase(m_context, "sample_mask_and_alpha_to_coverage",						"Test with SampleMask and alpha to coverage",					sampleCounts[sampleNdx],	SampleMaskCase::FLAGS_ALPHA_TO_COVERAGE));
		sampleGroup->addChild(new SampleMaskCase(m_context, "sample_mask_and_sample_coverage",							"Test with SampleMask and sample coverage",						sampleCounts[sampleNdx],	SampleMaskCase::FLAGS_SAMPLE_COVERAGE));
		sampleGroup->addChild(new SampleMaskCase(m_context, "sample_mask_and_sample_coverage_and_alpha_to_coverage",	"Test with SampleMask, sample coverage, and alpha to coverage",	sampleCounts[sampleNdx],	SampleMaskCase::FLAGS_ALPHA_TO_COVERAGE | SampleMaskCase::FLAGS_SAMPLE_COVERAGE));

		// high bits cause no unexpected behavior
		sampleGroup->addChild(new SampleMaskCase(m_context, "sample_mask_non_effective_bits",							"Test with SampleMask, set higher bits than sample count",		sampleCounts[sampleNdx],	SampleMaskCase::FLAGS_HIGH_BITS));

		// usage
		for (int typeNdx = 0; typeNdx < DE_LENGTH_OF_ARRAY(textureTypes); ++typeNdx)
			sampleGroup->addChild(new MultisampleTextureUsageCase(m_context, (std::string("use_") + textureTypes[typeNdx].name).c_str(), textureTypes[typeNdx].name, sampleCounts[sampleNdx], textureTypes[typeNdx].type));
	}

	// .negative
	{
		tcu::TestCaseGroup* const negativeGroup = new tcu::TestCaseGroup(m_testCtx, "negative", "Negative tests");
		addChild(negativeGroup);

		negativeGroup->addChild(new NegativeFramebufferCase	(m_context, "fbo_attach_different_sample_count_tex_tex",	"Attach different sample counts",	NegativeFramebufferCase::CASE_DIFFERENT_N_SAMPLES_TEX));
		negativeGroup->addChild(new NegativeFramebufferCase	(m_context, "fbo_attach_different_sample_count_tex_rbo",	"Attach different sample counts",	NegativeFramebufferCase::CASE_DIFFERENT_N_SAMPLES_RBO));
		negativeGroup->addChild(new NegativeFramebufferCase	(m_context, "fbo_attach_different_fixed_state_tex_tex",		"Attach fixed and non fixed",		NegativeFramebufferCase::CASE_DIFFERENT_FIXED_TEX));
		negativeGroup->addChild(new NegativeFramebufferCase	(m_context, "fbo_attach_different_fixed_state_tex_rbo",		"Attach fixed and non fixed",		NegativeFramebufferCase::CASE_DIFFERENT_FIXED_RBO));
		negativeGroup->addChild(new NegativeFramebufferCase	(m_context, "fbo_attach_non_zero_level",					"Attach non-zero level",			NegativeFramebufferCase::CASE_NON_ZERO_LEVEL));
		negativeGroup->addChild(new NegativeTexParameterCase(m_context, "texture_min_filter",							"set TEXTURE_MIN_FILTER",			NegativeTexParameterCase::TEXTURE_MIN_FILTER));
		negativeGroup->addChild(new NegativeTexParameterCase(m_context, "texture_mag_filter",							"set TEXTURE_MAG_FILTER",			NegativeTexParameterCase::TEXTURE_MAG_FILTER));
		negativeGroup->addChild(new NegativeTexParameterCase(m_context, "texture_wrap_s",								"set TEXTURE_WRAP_S",				NegativeTexParameterCase::TEXTURE_WRAP_S));
		negativeGroup->addChild(new NegativeTexParameterCase(m_context, "texture_wrap_t",								"set TEXTURE_WRAP_T",				NegativeTexParameterCase::TEXTURE_WRAP_T));
		negativeGroup->addChild(new NegativeTexParameterCase(m_context, "texture_wrap_r",								"set TEXTURE_WRAP_R",				NegativeTexParameterCase::TEXTURE_WRAP_R));
		negativeGroup->addChild(new NegativeTexParameterCase(m_context, "texture_min_lod",								"set TEXTURE_MIN_LOD",				NegativeTexParameterCase::TEXTURE_MIN_LOD));
		negativeGroup->addChild(new NegativeTexParameterCase(m_context, "texture_max_lod",								"set TEXTURE_MAX_LOD",				NegativeTexParameterCase::TEXTURE_MAX_LOD));
		negativeGroup->addChild(new NegativeTexParameterCase(m_context, "texture_compare_mode",							"set TEXTURE_COMPARE_MODE",			NegativeTexParameterCase::TEXTURE_COMPARE_MODE));
		negativeGroup->addChild(new NegativeTexParameterCase(m_context, "texture_compare_func",							"set TEXTURE_COMPARE_FUNC",			NegativeTexParameterCase::TEXTURE_COMPARE_FUNC));
		negativeGroup->addChild(new NegativeTexParameterCase(m_context, "texture_base_level",							"set TEXTURE_BASE_LEVEL",			NegativeTexParameterCase::TEXTURE_BASE_LEVEL));
		negativeGroup->addChild(new NegativeTexureSampleCase(m_context, "texture_high_sample_count",					"TexStorage with high numSamples",	NegativeTexureSampleCase::SAMPLECOUNT_HIGH));
		negativeGroup->addChild(new NegativeTexureSampleCase(m_context, "texture_zero_sample_count",					"TexStorage with zero numSamples",	NegativeTexureSampleCase::SAMPLECOUNT_ZERO));
	}
}

} // Functional
} // gles31
} // deqp
