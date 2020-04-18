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
 * \brief Multisample shader render case
 *//*--------------------------------------------------------------------*/

#include "es31fMultisampleShaderRenderCase.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuSurface.hpp"
#include "tcuTestLog.hpp"
#include "tcuStringTemplate.hpp"
#include "gluShaderProgram.hpp"
#include "gluRenderContext.hpp"
#include "gluPixelTransfer.hpp"
#include "glwFunctions.hpp"
#include "glwEnums.hpp"
#include "deStringUtil.hpp"

namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace MultisampleShaderRenderUtil
{
using std::map;
using std::string;
namespace
{

static const char* const s_vertexSource =	"${GLSL_VERSION_DECL}\n"
											"in highp vec4 a_position;\n"
											"out highp vec4 v_position;\n"
											"void main (void)\n"
											"{\n"
											"	gl_Position = a_position;\n"
											"	v_position = a_position;\n"
											"}";

} // anonymous

QualityWarning::QualityWarning (const std::string& message)
	: tcu::Exception(message)
{
}

MultisampleRenderCase::MultisampleRenderCase (Context& context, const char* name, const char* desc, int numSamples, RenderTarget target, int renderSize, int flags)
	: TestCase						(context, name, desc)
	, m_numRequestedSamples			(numSamples)
	, m_renderTarget				(target)
	, m_renderSize					(renderSize)
	, m_perIterationShader			((flags & FLAG_PER_ITERATION_SHADER) != 0)
	, m_verifyTextureSampleBuffers	((flags & FLAG_VERIFY_MSAA_TEXTURE_SAMPLE_BUFFERS) != 0 && target == TARGET_TEXTURE)
	, m_numTargetSamples			(-1)
	, m_buffer						(0)
	, m_resolveBuffer				(0)
	, m_program						(DE_NULL)
	, m_fbo							(0)
	, m_fboTexture					(0)
	, m_textureSamplerProgram		(DE_NULL)
	, m_fboRbo						(0)
	, m_resolveFbo					(0)
	, m_resolveFboTexture			(0)
	, m_iteration					(0)
	, m_numIterations				(1)
	, m_renderMode					(0)
	, m_renderCount					(0)
	, m_renderVao					(0)
	, m_resolveVao					(0)
{
	DE_ASSERT(target < TARGET_LAST);
}

MultisampleRenderCase::~MultisampleRenderCase (void)
{
	MultisampleRenderCase::deinit();
}

void MultisampleRenderCase::init (void)
{
	const glw::Functions&	gl					= m_context.getRenderContext().getFunctions();
	deInt32					queriedSampleCount	= -1;
	const bool				supportsES32		= glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));
	map<string, string>		args;

	args["GLSL_VERSION_DECL"] = supportsES32 ? getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES);

	// requirements

	switch (m_renderTarget)
	{
		case TARGET_DEFAULT:
		{
			if (m_context.getRenderTarget().getWidth() < m_renderSize || m_context.getRenderTarget().getHeight() < m_renderSize)
				throw tcu::NotSupportedError("Test requires render target with size " + de::toString(m_renderSize) + "x" + de::toString(m_renderSize) + " or greater");
			break;
		}

		case TARGET_TEXTURE:
		{
			deInt32 maxTextureSamples = 0;
			gl.getInternalformativ(GL_TEXTURE_2D_MULTISAMPLE, GL_RGBA8, GL_SAMPLES, 1, &maxTextureSamples);

			if (m_numRequestedSamples > maxTextureSamples)
				throw tcu::NotSupportedError("Sample count not supported");
			break;
		}

		case TARGET_RENDERBUFFER:
		{
			deInt32 maxRboSamples = 0;
			gl.getInternalformativ(GL_RENDERBUFFER, GL_RGBA8, GL_SAMPLES, 1, &maxRboSamples);

			if (m_numRequestedSamples > maxRboSamples)
				throw tcu::NotSupportedError("Sample count not supported");
			break;
		}

		default:
			DE_ASSERT(false);
	}

	// resources

	{
		gl.genBuffers(1, &m_buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "gen buf");

		setupRenderData();
		GLU_EXPECT_NO_ERROR(gl.getError(), "setup data");

		gl.genVertexArrays(1, &m_renderVao);
		GLU_EXPECT_NO_ERROR(gl.getError(), "gen vao");

		// buffer for MSAA texture resolving
		{
			static const tcu::Vec4 fullscreenQuad[] =
			{
				tcu::Vec4( 1.0f, -1.0f, 0.0f, 1.0f),
				tcu::Vec4( 1.0f,  1.0f, 0.0f, 1.0f),
				tcu::Vec4(-1.0f, -1.0f, 0.0f, 1.0f),
				tcu::Vec4(-1.0f,  1.0f, 0.0f, 1.0f),
			};

			gl.genBuffers(1, &m_resolveBuffer);
			gl.bindBuffer(GL_ARRAY_BUFFER, m_resolveBuffer);
			gl.bufferData(GL_ARRAY_BUFFER, (int)sizeof(fullscreenQuad), fullscreenQuad, GL_STATIC_DRAW);
			GLU_EXPECT_NO_ERROR(gl.getError(), "setup data");
		}
	}

	// msaa targets

	if (m_renderTarget == TARGET_TEXTURE)
	{
		const deUint32 textureTarget = (m_numRequestedSamples == 0) ? (GL_TEXTURE_2D) : (GL_TEXTURE_2D_MULTISAMPLE);

		gl.genVertexArrays(1, &m_resolveVao);
		GLU_EXPECT_NO_ERROR(gl.getError(), "gen vao");

		gl.genTextures(1, &m_fboTexture);
		gl.bindTexture(textureTarget, m_fboTexture);
		if (m_numRequestedSamples == 0)
		{
			gl.texStorage2D(textureTarget, 1, GL_RGBA8, m_renderSize, m_renderSize);
			gl.texParameteri(textureTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			gl.texParameteri(textureTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		}
		else
			gl.texStorage2DMultisample(textureTarget, m_numRequestedSamples, GL_RGBA8, m_renderSize, m_renderSize, GL_FALSE);
		GLU_EXPECT_NO_ERROR(gl.getError(), "gen tex");

		gl.genFramebuffers(1, &m_fbo);
		gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo);
		gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, textureTarget, m_fboTexture, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "gen fbo");

		if (gl.checkFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			throw tcu::TestError("fbo not complete");

		if (m_numRequestedSamples != 0)
		{
			// for shader
			gl.getTexLevelParameteriv(GL_TEXTURE_2D_MULTISAMPLE, 0, GL_TEXTURE_SAMPLES, &queriedSampleCount);

			// logging
			m_testCtx.getLog() << tcu::TestLog::Message << "Asked for " << m_numRequestedSamples << " samples, got " << queriedSampleCount << " samples." << tcu::TestLog::EndMessage;

			// sanity
			if (queriedSampleCount < m_numRequestedSamples)
				throw tcu::TestError("Got less texture samples than asked for");
		}

		// texture sampler shader
		m_textureSamplerProgram = new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources()
			<< glu::VertexSource(tcu::StringTemplate(s_vertexSource).specialize(args))
			<< glu::FragmentSource(genMSSamplerSource(queriedSampleCount)));
		if (!m_textureSamplerProgram->isOk())
		{
			m_testCtx.getLog() << tcu::TestLog::Section("SamplerShader", "Sampler shader") << *m_textureSamplerProgram << tcu::TestLog::EndSection;
			throw tcu::TestError("could not build program");
		}
	}
	else if (m_renderTarget == TARGET_RENDERBUFFER)
	{
		gl.genRenderbuffers(1, &m_fboRbo);
		gl.bindRenderbuffer(GL_RENDERBUFFER, m_fboRbo);
		gl.renderbufferStorageMultisample(GL_RENDERBUFFER, m_numRequestedSamples, GL_RGBA8, m_renderSize, m_renderSize);
		GLU_EXPECT_NO_ERROR(gl.getError(), "gen rbo");

		gl.genFramebuffers(1, &m_fbo);
		gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo);
		gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_fboRbo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "gen fbo");

		if (gl.checkFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			throw tcu::TestError("fbo not complete");

		// logging
		gl.getRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_SAMPLES, &queriedSampleCount);
		m_testCtx.getLog() << tcu::TestLog::Message << "Asked for " << m_numRequestedSamples << " samples, got " << queriedSampleCount << " samples." << tcu::TestLog::EndMessage;

		// sanity
		if (queriedSampleCount < m_numRequestedSamples)
			throw tcu::TestError("Got less renderbuffer samples samples than asked for");
	}

	// fbo for resolving the multisample fbo
	if (m_renderTarget != TARGET_DEFAULT)
	{
		gl.genTextures(1, &m_resolveFboTexture);
		gl.bindTexture(GL_TEXTURE_2D, m_resolveFboTexture);
		gl.texStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, m_renderSize, m_renderSize);
		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		GLU_EXPECT_NO_ERROR(gl.getError(), "gen tex");

		gl.genFramebuffers(1, &m_resolveFbo);
		gl.bindFramebuffer(GL_FRAMEBUFFER, m_resolveFbo);
		gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_resolveFboTexture, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "gen fbo");

		if (gl.checkFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			throw tcu::TestError("resolve fbo not complete");
	}

	// create verifier shader and set targetSampleCount

	{
		int realSampleCount = -1;

		if (m_renderTarget == TARGET_TEXTURE)
		{
			if (m_numRequestedSamples == 0)
				realSampleCount = 1; // non msaa texture
			else
				realSampleCount = de::max(1, queriedSampleCount); // msaa texture
		}
		else if (m_renderTarget == TARGET_RENDERBUFFER)
		{
			realSampleCount = de::max(1, queriedSampleCount); // msaa rbo
		}
		else if (m_renderTarget == TARGET_DEFAULT)
		{
			realSampleCount = de::max(1, m_context.getRenderTarget().getNumSamples());
		}
		else
			DE_ASSERT(DE_FALSE);

		// is set and is valid
		DE_ASSERT(realSampleCount != -1);
		DE_ASSERT(realSampleCount != 0);
		m_numTargetSamples = realSampleCount;
	}

	if (!m_perIterationShader)
	{
		m_program = new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources() << glu::VertexSource(genVertexSource(m_numTargetSamples)) << glu::FragmentSource(genFragmentSource(m_numTargetSamples)));
		m_testCtx.getLog() << tcu::TestLog::Section("RenderShader", "Render shader") << *m_program << tcu::TestLog::EndSection;
		if (!m_program->isOk())
			throw tcu::TestError("could not build program");

	}
}

void MultisampleRenderCase::deinit (void)
{
	if (m_buffer)
	{
		m_context.getRenderContext().getFunctions().deleteBuffers(1, &m_buffer);
		m_buffer = 0;
	}

	if (m_resolveBuffer)
	{
		m_context.getRenderContext().getFunctions().deleteBuffers(1, &m_resolveBuffer);
		m_resolveBuffer = 0;
	}

	delete m_program;
	m_program = DE_NULL;

	if (m_fbo)
	{
		m_context.getRenderContext().getFunctions().deleteFramebuffers(1, &m_fbo);
		m_fbo = 0;
	}

	if (m_fboTexture)
	{
		m_context.getRenderContext().getFunctions().deleteTextures(1, &m_fboTexture);
		m_fboTexture = 0;
	}

	delete m_textureSamplerProgram;
	m_textureSamplerProgram = DE_NULL;

	if (m_fboRbo)
	{
		m_context.getRenderContext().getFunctions().deleteRenderbuffers(1, &m_fboRbo);
		m_fboRbo = 0;
	}

	if (m_resolveFbo)
	{
		m_context.getRenderContext().getFunctions().deleteFramebuffers(1, &m_resolveFbo);
		m_resolveFbo = 0;
	}

	if (m_resolveFboTexture)
	{
		m_context.getRenderContext().getFunctions().deleteTextures(1, &m_resolveFboTexture);
		m_resolveFboTexture = 0;
	}

	if (m_renderVao)
	{
		m_context.getRenderContext().getFunctions().deleteVertexArrays(1, &m_renderVao);
		m_renderVao = 0;
	}

	if (m_resolveVao)
	{
		m_context.getRenderContext().getFunctions().deleteVertexArrays(1, &m_resolveVao);
		m_resolveVao = 0;
	}
}

MultisampleRenderCase::IterateResult MultisampleRenderCase::iterate (void)
{
	// default value
	if (m_iteration == 0)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		preTest();
	}

	drawOneIteration();

	// next iteration
	++m_iteration;
	if (m_iteration < m_numIterations)
		return CONTINUE;
	else
	{
		postTest();
		return STOP;
	}
}

void MultisampleRenderCase::preDraw (void)
{
}

void MultisampleRenderCase::postDraw (void)
{
}

void MultisampleRenderCase::preTest (void)
{
}

void MultisampleRenderCase::postTest (void)
{
}

void MultisampleRenderCase::verifyResultImageAndSetResult (const tcu::Surface& resultImage)
{
	// verify using case-specific verification

	try
	{
		if (!verifyImage(resultImage))
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Image verification failed");
	}
	catch (const QualityWarning& ex)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Quality warning, error = " << ex.what() << tcu::TestLog::EndMessage;

		// Failures are more important than warnings
		if (m_testCtx.getTestResult() == QP_TEST_RESULT_PASS)
			m_testCtx.setTestResult(QP_TEST_RESULT_QUALITY_WARNING, ex.what());
	}
}

void MultisampleRenderCase::verifyResultBuffersAndSetResult (const std::vector<tcu::Surface>& resultBuffers)
{
	// verify using case-specific verification

	try
	{
		if (!verifySampleBuffers(resultBuffers))
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Image verification failed");
	}
	catch (const QualityWarning& ex)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Quality warning, error = " << ex.what() << tcu::TestLog::EndMessage;

		// Failures are more important than warnings
		if (m_testCtx.getTestResult() == QP_TEST_RESULT_PASS)
			m_testCtx.setTestResult(QP_TEST_RESULT_QUALITY_WARNING, ex.what());
	}
}

std::string	MultisampleRenderCase::getIterationDescription (int iteration) const
{
	DE_UNREF(iteration);
	DE_ASSERT(false);
	return "";
}

void MultisampleRenderCase::drawOneIteration (void)
{
	const glw::Functions&		gl					= m_context.getRenderContext().getFunctions();
	const std::string			sectionDescription	= (m_numIterations > 1) ? ("Iteration " + de::toString(m_iteration+1) + "/" + de::toString(m_numIterations) + ": " + getIterationDescription(m_iteration)) : ("Test");
	const tcu::ScopedLogSection	section				(m_testCtx.getLog(), "Iteration" + de::toString(m_iteration), sectionDescription);

	// Per iteration shader?
	if (m_perIterationShader)
	{
		delete m_program;
		m_program = DE_NULL;

		m_program = new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources()
			<< glu::VertexSource(genVertexSource(m_numTargetSamples))
			<< glu::FragmentSource(genFragmentSource(m_numTargetSamples)));
		m_testCtx.getLog() << tcu::TestLog::Section("RenderShader", "Render shader") << *m_program << tcu::TestLog::EndSection;
		if (!m_program->isOk())
			throw tcu::TestError("could not build program");

	}

	// render
	{
		if (m_renderTarget == TARGET_TEXTURE || m_renderTarget == TARGET_RENDERBUFFER)
		{
			gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo);
			GLU_EXPECT_NO_ERROR(gl.getError(), "bind fbo");

			m_testCtx.getLog() << tcu::TestLog::Message << "Rendering " << m_renderSceneDescription << " with render shader to fbo." << tcu::TestLog::EndMessage;
		}
		else
			m_testCtx.getLog() << tcu::TestLog::Message << "Rendering " << m_renderSceneDescription << " with render shader to default framebuffer." << tcu::TestLog::EndMessage;

		gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
		gl.clear(GL_COLOR_BUFFER_BIT);
		gl.viewport(0, 0, m_renderSize, m_renderSize);
		GLU_EXPECT_NO_ERROR(gl.getError(), "clear");

		gl.bindVertexArray(m_renderVao);
		gl.bindBuffer(GL_ARRAY_BUFFER, m_buffer);

		// set attribs
		DE_ASSERT(!m_renderAttribs.empty());
		for (std::map<std::string, Attrib>::const_iterator it = m_renderAttribs.begin(); it != m_renderAttribs.end(); ++it)
		{
			const deInt32 location = gl.getAttribLocation(m_program->getProgram(), it->first.c_str());

			if (location != -1)
			{
				gl.vertexAttribPointer(location, 4, GL_FLOAT, GL_FALSE, it->second.stride, (deUint8*)DE_NULL + it->second.offset);
				gl.enableVertexAttribArray(location);
			}
		}
		GLU_EXPECT_NO_ERROR(gl.getError(), "set attrib");

		gl.useProgram(m_program->getProgram());
		preDraw();
		gl.drawArrays(m_renderMode, 0, m_renderCount);
		postDraw();
		gl.useProgram(0);
		gl.bindVertexArray(0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "draw");

		if (m_renderTarget == TARGET_TEXTURE || m_renderTarget == TARGET_RENDERBUFFER)
			gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	// read
	{
		if (m_renderTarget == TARGET_DEFAULT)
		{
			tcu::Surface resultImage(m_renderSize, m_renderSize);

			m_testCtx.getLog() << tcu::TestLog::Message << "Reading pixels from default framebuffer." << tcu::TestLog::EndMessage;

			// default directly
			glu::readPixels(m_context.getRenderContext(), 0, 0, resultImage.getAccess());
			GLU_EXPECT_NO_ERROR(gl.getError(), "read pixels");

			// set test result
			verifyResultImageAndSetResult(resultImage);
		}
		else if (m_renderTarget == TARGET_RENDERBUFFER)
		{
			tcu::Surface resultImage(m_renderSize, m_renderSize);

			// rbo by blitting to non-multisample fbo

			m_testCtx.getLog() << tcu::TestLog::Message << "Blitting result from fbo to single sample fbo. (Resolve multisample)" << tcu::TestLog::EndMessage;

			gl.bindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo);
			gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, m_resolveFbo);
			gl.blitFramebuffer(0, 0, m_renderSize, m_renderSize, 0, 0, m_renderSize, m_renderSize, GL_COLOR_BUFFER_BIT, GL_NEAREST);
			GLU_EXPECT_NO_ERROR(gl.getError(), "blit resolve");

			m_testCtx.getLog() << tcu::TestLog::Message << "Reading pixels from single sample framebuffer." << tcu::TestLog::EndMessage;

			gl.bindFramebuffer(GL_READ_FRAMEBUFFER, m_resolveFbo);
			glu::readPixels(m_context.getRenderContext(), 0, 0, resultImage.getAccess());
			GLU_EXPECT_NO_ERROR(gl.getError(), "read pixels");

			gl.bindFramebuffer(GL_FRAMEBUFFER, 0);

			// set test result
			verifyResultImageAndSetResult(resultImage);
		}
		else if (m_renderTarget == TARGET_TEXTURE && !m_verifyTextureSampleBuffers)
		{
			const deInt32	posLocation		= gl.getAttribLocation(m_textureSamplerProgram->getProgram(), "a_position");
			const deInt32	samplerLocation	= gl.getUniformLocation(m_textureSamplerProgram->getProgram(), "u_sampler");
			const deUint32	textureTarget	= (m_numRequestedSamples == 0) ? (GL_TEXTURE_2D) : (GL_TEXTURE_2D_MULTISAMPLE);
			tcu::Surface	resultImage		(m_renderSize, m_renderSize);

			if (m_numRequestedSamples)
				m_testCtx.getLog() << tcu::TestLog::Message << "Using sampler shader to sample the multisample texture to single sample framebuffer." << tcu::TestLog::EndMessage;
			else
				m_testCtx.getLog() << tcu::TestLog::Message << "Drawing texture to single sample framebuffer. Using sampler shader." << tcu::TestLog::EndMessage;

			if (samplerLocation == -1)
				throw tcu::TestError("Location u_sampler was -1.");

			// resolve multisample texture by averaging
			gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
			gl.clear(GL_COLOR_BUFFER_BIT);
			gl.viewport(0, 0, m_renderSize, m_renderSize);
			GLU_EXPECT_NO_ERROR(gl.getError(), "clear");

			gl.bindVertexArray(m_resolveVao);
			gl.bindBuffer(GL_ARRAY_BUFFER, m_resolveBuffer);
			gl.vertexAttribPointer(posLocation, 4, GL_FLOAT, GL_FALSE, 0, DE_NULL);
			gl.enableVertexAttribArray(posLocation);
			GLU_EXPECT_NO_ERROR(gl.getError(), "set attrib");

			gl.activeTexture(GL_TEXTURE0);
			gl.bindTexture(textureTarget, m_fboTexture);
			GLU_EXPECT_NO_ERROR(gl.getError(), "bind tex");

			gl.useProgram(m_textureSamplerProgram->getProgram());
			gl.uniform1i(samplerLocation, 0);

			gl.bindFramebuffer(GL_FRAMEBUFFER, m_resolveFbo);
			gl.drawArrays(GL_TRIANGLE_STRIP, 0, 4);

			gl.useProgram(0);
			gl.bindVertexArray(0);
			GLU_EXPECT_NO_ERROR(gl.getError(), "draw");

			m_testCtx.getLog() << tcu::TestLog::Message << "Reading pixels from single sample framebuffer." << tcu::TestLog::EndMessage;

			glu::readPixels(m_context.getRenderContext(), 0, 0, resultImage.getAccess());
			GLU_EXPECT_NO_ERROR(gl.getError(), "read pixels");

			gl.bindFramebuffer(GL_FRAMEBUFFER, 0);

			// set test result
			verifyResultImageAndSetResult(resultImage);
		}
		else if (m_renderTarget == TARGET_TEXTURE && m_verifyTextureSampleBuffers)
		{
			const deInt32				posLocation		= gl.getAttribLocation(m_textureSamplerProgram->getProgram(), "a_position");
			const deInt32				samplerLocation	= gl.getUniformLocation(m_textureSamplerProgram->getProgram(), "u_sampler");
			const deInt32				sampleLocation	= gl.getUniformLocation(m_textureSamplerProgram->getProgram(), "u_sampleNdx");
			const deUint32				textureTarget	= (m_numRequestedSamples == 0) ? (GL_TEXTURE_2D) : (GL_TEXTURE_2D_MULTISAMPLE);
			std::vector<tcu::Surface>	resultBuffers	(m_numTargetSamples);

			if (m_numRequestedSamples)
				m_testCtx.getLog() << tcu::TestLog::Message << "Reading multisample texture sample buffers." << tcu::TestLog::EndMessage;
			else
				m_testCtx.getLog() << tcu::TestLog::Message << "Reading texture." << tcu::TestLog::EndMessage;

			if (samplerLocation == -1)
				throw tcu::TestError("Location u_sampler was -1.");
			if (sampleLocation == -1)
				throw tcu::TestError("Location u_sampleNdx was -1.");

			for (int sampleNdx = 0; sampleNdx < m_numTargetSamples; ++sampleNdx)
				resultBuffers[sampleNdx].setSize(m_renderSize, m_renderSize);

			// read sample buffers to different surfaces
			gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
			gl.clear(GL_COLOR_BUFFER_BIT);
			gl.viewport(0, 0, m_renderSize, m_renderSize);
			GLU_EXPECT_NO_ERROR(gl.getError(), "clear");

			gl.bindVertexArray(m_resolveVao);
			gl.bindBuffer(GL_ARRAY_BUFFER, m_resolveBuffer);
			gl.vertexAttribPointer(posLocation, 4, GL_FLOAT, GL_FALSE, 0, DE_NULL);
			gl.enableVertexAttribArray(posLocation);
			GLU_EXPECT_NO_ERROR(gl.getError(), "set attrib");

			gl.activeTexture(GL_TEXTURE0);
			gl.bindTexture(textureTarget, m_fboTexture);
			GLU_EXPECT_NO_ERROR(gl.getError(), "bind tex");

			gl.bindFramebuffer(GL_FRAMEBUFFER, m_resolveFbo);
			gl.useProgram(m_textureSamplerProgram->getProgram());
			gl.uniform1i(samplerLocation, 0);

			m_testCtx.getLog() << tcu::TestLog::Message << "Reading sample buffers" << tcu::TestLog::EndMessage;

			for (int sampleNdx = 0; sampleNdx < m_numTargetSamples; ++sampleNdx)
			{
				gl.uniform1i(sampleLocation, sampleNdx);
				gl.drawArrays(GL_TRIANGLE_STRIP, 0, 4);
				GLU_EXPECT_NO_ERROR(gl.getError(), "draw");

				glu::readPixels(m_context.getRenderContext(), 0, 0, resultBuffers[sampleNdx].getAccess());
				GLU_EXPECT_NO_ERROR(gl.getError(), "read pixels");
			}

			gl.useProgram(0);
			gl.bindVertexArray(0);
			gl.bindFramebuffer(GL_FRAMEBUFFER, 0);

			// verify sample buffers
			verifyResultBuffersAndSetResult(resultBuffers);
		}
		else
			DE_ASSERT(false);
	}
}

std::string	MultisampleRenderCase::genVertexSource (int numTargetSamples) const
{
	const bool				supportsES32	= glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));
	map<string, string>		args;

	args["GLSL_VERSION_DECL"] = supportsES32 ? getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES);

	DE_UNREF(numTargetSamples);
	return std::string(tcu::StringTemplate(s_vertexSource).specialize(args));
}

std::string MultisampleRenderCase::genMSSamplerSource (int numTargetSamples) const
{
	if (m_verifyTextureSampleBuffers)
		return genMSTextureLayerFetchSource(numTargetSamples);
	else
		return genMSTextureResolverSource(numTargetSamples);
}

std::string	MultisampleRenderCase::genMSTextureResolverSource (int numTargetSamples) const
{
	// default behavior: average

	const bool				supportsES32			= glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));
	map<string, string>		args;
	const bool				isSingleSampleTarget	= (m_numRequestedSamples == 0);
	std::ostringstream		buf;

	args["GLSL_VERSION_DECL"] = supportsES32 ? getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES);

	buf <<	"${GLSL_VERSION_DECL}\n"
			"in mediump vec4 v_position;\n"
			"layout(location = 0) out mediump vec4 fragColor;\n"
			"uniform mediump " << ((isSingleSampleTarget) ? ("sampler2D") : ("sampler2DMS")) << " u_sampler;\n"
			"void main (void)\n"
			"{\n"
			"	mediump vec2 relPosition = (v_position.xy + vec2(1.0, 1.0)) / 2.0;\n"
			"	mediump ivec2 fetchPos = ivec2(floor(relPosition * " << m_renderSize << ".0));\n"
			"	mediump vec4 colorSum = vec4(0.0, 0.0, 0.0, 0.0);\n"
			"\n";

	if (isSingleSampleTarget)
		buf <<	"	colorSum = texelFetch(u_sampler, fetchPos, 0);\n"
				"\n";
	else
		buf <<	"	for (int sampleNdx = 0; sampleNdx < " << numTargetSamples << "; ++sampleNdx)\n"
				"		colorSum += texelFetch(u_sampler, fetchPos, sampleNdx);\n"
				"	colorSum /= " << numTargetSamples << ".0;\n"
				"\n";

	buf <<	"	fragColor = vec4(colorSum.xyz, 1.0);\n"
			"}\n";

	return tcu::StringTemplate(buf.str()).specialize(args);
}

std::string MultisampleRenderCase::genMSTextureLayerFetchSource (int numTargetSamples) const
{
	DE_UNREF(numTargetSamples);

	const bool				supportsES32			= glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));
	map<string, string>		args;
	const bool				isSingleSampleTarget	= (m_numRequestedSamples == 0);
	std::ostringstream		buf;

	args["GLSL_VERSION_DECL"] = supportsES32 ? getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES);

	buf <<	"${GLSL_VERSION_DECL}\n"
			"in mediump vec4 v_position;\n"
			"layout(location = 0) out mediump vec4 fragColor;\n"
			"uniform mediump " << ((isSingleSampleTarget) ? ("sampler2D") : ("sampler2DMS")) << " u_sampler;\n"
			"uniform mediump int u_sampleNdx;\n"
			"void main (void)\n"
			"{\n"
			"	mediump vec2 relPosition = (v_position.xy + vec2(1.0, 1.0)) / 2.0;\n"
			"	mediump ivec2 fetchPos = ivec2(floor(relPosition * " << m_renderSize << ".0));\n"
			"\n"
			"	mediump vec4 color = texelFetch(u_sampler, fetchPos, u_sampleNdx);\n"
			"	fragColor = vec4(color.rgb, 1.0);\n"
			"}\n";

	return tcu::StringTemplate(buf.str()).specialize(args);
}

bool MultisampleRenderCase::verifySampleBuffers (const std::vector<tcu::Surface>& resultBuffers)
{
	DE_UNREF(resultBuffers);
	DE_ASSERT(false);
	return false;
}

void MultisampleRenderCase::setupRenderData (void)
{
	static const tcu::Vec4 fullscreenQuad[] =
	{
		tcu::Vec4( 1.0f, -1.0f, 0.0f, 1.0f),
		tcu::Vec4( 1.0f,  1.0f, 0.0f, 1.0f),
		tcu::Vec4(-1.0f, -1.0f, 0.0f, 1.0f),
		tcu::Vec4(-1.0f,  1.0f, 0.0f, 1.0f),
	};

	const glw::Functions&	gl				= m_context.getRenderContext().getFunctions();

	m_renderMode = GL_TRIANGLE_STRIP;
	m_renderCount = 4;
	m_renderSceneDescription = "quad";

	m_renderAttribs["a_position"].offset = 0;
	m_renderAttribs["a_position"].stride = sizeof(float[4]);

	gl.bindBuffer(GL_ARRAY_BUFFER, m_buffer);
	gl.bufferData(GL_ARRAY_BUFFER, (int)sizeof(fullscreenQuad), fullscreenQuad, GL_STATIC_DRAW);
}

} // MultisampleShaderRenderUtil
} // Functional
} // gles31
} // deqp
