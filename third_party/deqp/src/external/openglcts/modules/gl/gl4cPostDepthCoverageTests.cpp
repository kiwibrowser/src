/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
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
 * \brief
 */ /*-------------------------------------------------------------------*/

/**
 */ /*!
 * \file  gl4cPostDepthCoverageTests.cpp
 * \brief Conformance tests for the GL_ARB_post_depth_coverage functionality.
 */ /*-------------------------------------------------------------------*/

#include "gl4cPostDepthCoverageTests.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "gluDrawUtil.hpp"
#include "gluShaderProgram.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"

using namespace glw;
using namespace glu;

namespace gl4cts
{
static const char* c_commonVertShader = "#version 450\n"
										"\n"
										"in vec3 vertex;\n"
										"in vec2 inTexCoord;\n"
										"out vec2 texCoord;\n"
										"\n"
										"void main()\n"
										"{\n"
										"    texCoord = inTexCoord;\n"
										"    gl_Position = vec4(vertex, 1);\n"
										"}\n";

/** Constructor.
 *
 *  @param context     Rendering context
 */
PostDepthShaderCase::PostDepthShaderCase(deqp::Context& context)
	: TestCase(context, "PostDepthShader",
			   "Verifies if shader functionality added in ARB_post_depth_coverage extension works as expected")
{
	/* Left blank intentionally */
}

/** Stub deinit method. */
void PostDepthShaderCase::deinit()
{
}

/** Stub init method */
void PostDepthShaderCase::init()
{
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_post_depth_coverage"))
		throw tcu::NotSupportedError("GL_ARB_post_depth_coverage not supported");

	m_vertShader = c_commonVertShader;

	m_fragShader1 = "#version 450\n"
					"\n"
					"#extension GL_ARB_post_depth_coverage : require\n"
					"\n"
					"out vec4 fragColor;\n"
					"\n"
					"void main()\n"
					"{\n"
					"    fragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
					"}\n";

	m_fragShader2 = "#version 450\n"
					"\n"
					"#extension GL_ARB_post_depth_coverage : enable\n"
					"\n"
					"#ifndef GL_ARB_post_depth_coverage\n"
					"  #error GL_ARB_post_depth_coverage not defined\n"
					"#else\n"
					"  #if (GL_ARB_post_depth_coverage != 1)\n"
					"    #error GL_ARB_post_depth_coverage wrong defined\n"
					"  #endif\n"
					"#endif // GL_ARB_post_depth_coverage\n"
					"\n"
					"out vec4 fragColor;\n"
					"\n"
					"void main()\n"
					"{\n"
					"    fragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
					"}\n";

	m_fragShader3 = "#version 450\n"
					"\n"
					"#extension GL_ARB_post_depth_coverage : enable\n"
					"\n"
					"layout(early_fragment_tests) in;\n"
					"layout(post_depth_coverage) in;\n"
					"\n"
					"out vec4 fragColor;\n"
					"\n"
					"void main()\n"
					"{\n"
					"    fragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
					"}\n";

	m_fragShader4 = "#version 450\n"
					"\n"
					"#extension GL_ARB_post_depth_coverage : enable\n"
					"\n"
					"layout(post_depth_coverage) in;\n"
					"\n"
					"out vec4 fragColor;\n"
					"\n"
					"void main()\n"
					"{\n"
					"    fragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
					"}\n";
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult PostDepthShaderCase::iterate()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	ProgramSources sources1 = makeVtxFragSources(m_vertShader, m_fragShader1);
	ShaderProgram  program1(gl, sources1);

	if (!program1.isOk())
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail checking extension in shader");
		m_testCtx.getLog() << tcu::TestLog::Message << "Vertex: " << program1.getShaderInfo(SHADERTYPE_VERTEX).infoLog
						   << "\n"
						   << "Fragment: " << program1.getShaderInfo(SHADERTYPE_FRAGMENT).infoLog << "\n"
						   << "Program: " << program1.getProgramInfo().infoLog << tcu::TestLog::EndMessage;

		return STOP;
	}

	ProgramSources sources2 = makeVtxFragSources(m_vertShader, m_fragShader2);
	ShaderProgram  program2(gl, sources2);

	if (!program2.isOk())
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail checking preprocessor directives in shader");
		m_testCtx.getLog() << tcu::TestLog::Message << "Vertex: " << program2.getShaderInfo(SHADERTYPE_VERTEX).infoLog
						   << "\n"
						   << "Fragment: " << program2.getShaderInfo(SHADERTYPE_FRAGMENT).infoLog << "\n"
						   << "Program: " << program2.getProgramInfo().infoLog << tcu::TestLog::EndMessage;
		return STOP;
	}

	ProgramSources sources3 = makeVtxFragSources(m_vertShader, m_fragShader3);
	ShaderProgram  program3(gl, sources3);

	if (!program3.isOk())
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail checking first layout setup in shader");
		m_testCtx.getLog() << tcu::TestLog::Message << "Vertex: " << program3.getShaderInfo(SHADERTYPE_VERTEX).infoLog
						   << "\n"
						   << "Fragment: " << program3.getShaderInfo(SHADERTYPE_FRAGMENT).infoLog << "\n"
						   << "Program: " << program3.getProgramInfo().infoLog << tcu::TestLog::EndMessage;
		return STOP;
	}

	ProgramSources sources4 = makeVtxFragSources(m_vertShader, m_fragShader4);
	ShaderProgram  program4(gl, sources4);

	if (!program4.isOk())
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail checking second layout setup in shader");
		m_testCtx.getLog() << tcu::TestLog::Message << "Vertex: " << program4.getShaderInfo(SHADERTYPE_VERTEX).infoLog
						   << "\n"
						   << "Fragment: " << program4.getShaderInfo(SHADERTYPE_FRAGMENT).infoLog << "\n"
						   << "Program: " << program4.getProgramInfo().infoLog << tcu::TestLog::EndMessage;
		return STOP;
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Constructor.
 *
 *  @param context     Rendering context
 */
PostDepthSampleMaskCase::PostDepthSampleMaskCase(deqp::Context& context)
	: TestCase(context, "PostDepthSampleMask", "Verifies multisampling with depth test and stencil test functionality "
											   "added in ARB_post_depth_coverage extension")
	, m_framebufferMS(0)
	, m_framebuffer(0)
	, m_textureMS(0)
	, m_texture(0)
	, m_depthStencilRenderbuffer(0)
{
	/* Left blank intentionally */
}

/** Stub deinit method. */
void PostDepthSampleMaskCase::deinit()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindFramebuffer(GL_FRAMEBUFFER, 0);

	if (m_framebufferMS)
		gl.deleteFramebuffers(1, &m_framebufferMS);
	if (m_framebuffer)
		gl.deleteFramebuffers(1, &m_framebuffer);

	if (m_textureMS)
		gl.deleteTextures(1, &m_textureMS);
	if (m_texture)
		gl.deleteTextures(1, &m_texture);

	if (m_depthStencilRenderbuffer)
		gl.deleteRenderbuffers(1, &m_depthStencilRenderbuffer);
}

/** Stub init method */
void PostDepthSampleMaskCase::init()
{
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_post_depth_coverage"))
		throw tcu::NotSupportedError("GL_ARB_post_depth_coverage not supported");

	m_vertShader = c_commonVertShader;

	m_fragShader1a = "#version 450\n"
					 "\n"
					 "#extension GL_ARB_post_depth_coverage : enable\n"
					 "\n"
					 "in vec2 texCoord;\n"
					 "out vec4 fragColor;\n"
					 "\n"
					 "void main()\n"
					 "{\n"
					 "    int samp1 = (gl_SampleMaskIn[0] & 0x1);\n"
					 "    int samp2 = (gl_SampleMaskIn[0] & 0x2) / 2;\n"
					 "    int samp3 = (gl_SampleMaskIn[0] & 0x4) / 4;\n"
					 "    int samp4 = (gl_SampleMaskIn[0] & 0x8) / 8;\n"
					 "    fragColor = vec4(samp1, samp2, samp3, samp4);\n"
					 "}\n";

	m_fragShader1b = "#version 450\n"
					 "\n"
					 "#extension GL_ARB_post_depth_coverage : enable\n"
					 "layout(post_depth_coverage) in;\n"
					 "\n"
					 "in vec2 texCoord;\n"
					 "out vec4 fragColor;\n"
					 "\n"
					 "void main()\n"
					 "{\n"
					 "    int samp1 = (gl_SampleMaskIn[0] & 0x1);\n"
					 "    int samp2 = (gl_SampleMaskIn[0] & 0x2) / 2;\n"
					 "    int samp3 = (gl_SampleMaskIn[0] & 0x4) / 4;\n"
					 "    int samp4 = (gl_SampleMaskIn[0] & 0x8) / 8;\n"
					 "    fragColor = vec4(samp1, samp2, samp3, samp4);\n"
					 "}\n";

	m_fragShader2 = "#version 450\n"
					"\n"
					"in vec2 texCoord;\n"
					"out vec4 fragColor;\n"
					"\n"
					"uniform sampler2DMS texture;\n"
					"\n"
					"void main()\n"
					"{\n"
					"    int samp = int(texCoord.y * 4);\n"
					"    fragColor = texelFetch(texture, ivec2(0, 0), samp);\n"
					"}\n";

	const Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genRenderbuffers(1, &m_depthStencilRenderbuffer);
	gl.bindRenderbuffer(GL_RENDERBUFFER, m_depthStencilRenderbuffer);
	gl.renderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_STENCIL, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorageMultisample");

	gl.genTextures(1, &m_textureMS);
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_textureMS);
	gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA8, 1, 1, GL_TRUE);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2DMultisample");

	gl.genFramebuffers(1, &m_framebufferMS);
	gl.bindFramebuffer(GL_FRAMEBUFFER, m_framebufferMS);
	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, m_textureMS, 0);
	gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthStencilRenderbuffer);
	gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_depthStencilRenderbuffer);

	gl.genTextures(1, &m_texture);
	gl.bindTexture(GL_TEXTURE_2D, m_texture);
	gl.texStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 1, 4);

	gl.genFramebuffers(1, &m_framebuffer);
	gl.bindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture, 0);
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult PostDepthSampleMaskCase::iterate()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	const GLfloat texCoord[] = {
		0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
	};

	const GLfloat verticesNear[] = {
		-1.0f, -1.0f, 0.25f, 0.4f, -1.0f, 0.25f, -1.0f, 0.4f, 0.25f,
	};

	const GLfloat verticesFar[] = {
		-1.0f, -1.0f, 0.75f, 3.0f, -1.0f, 0.75f, -1.0f, 3.0f, 0.75f,
	};

	const GLfloat verticesPost[] = {
		-1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, -1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
	};

	const GLuint indicesPre[] = { 0, 1, 2 };

	const GLuint indicesPost[] = { 0, 1, 2, 1, 2, 3 };

	glu::VertexArrayBinding vertexArraysNear[] = { glu::va::Float("vertex", 3, 3, 0, verticesNear),
												   glu::va::Float("inTexCoord", 2, 3, 0, texCoord) };

	glu::VertexArrayBinding vertexArraysFar[] = { glu::va::Float("vertex", 3, 3, 0, verticesFar),
												  glu::va::Float("inTexCoord", 2, 3, 0, texCoord) };

	glu::VertexArrayBinding vertexArraysPost[] = { glu::va::Float("vertex", 3, 4, 0, verticesPost),
												   glu::va::Float("inTexCoord", 2, 4, 0, texCoord) };

	//Prepare shaders
	ProgramSources sources1a = makeVtxFragSources(m_vertShader, m_fragShader1a);
	ShaderProgram  program1a(gl, sources1a);

	if (!program1a.isOk())
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Shader program fail (non post_depth_coverage).");
		m_testCtx.getLog() << tcu::TestLog::Message << "Vertex: " << program1a.getShaderInfo(SHADERTYPE_VERTEX).infoLog
						   << "\n"
						   << "Fragment: " << program1a.getShaderInfo(SHADERTYPE_FRAGMENT).infoLog << "\n"
						   << "Program: " << program1a.getProgramInfo().infoLog << tcu::TestLog::EndMessage;
		return STOP;
	}

	ProgramSources sources1b = makeVtxFragSources(m_vertShader, m_fragShader1b);
	ShaderProgram  program1b(gl, sources1b);

	if (!program1b.isOk())
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Shader program fail (post_depth_coverage).");
		m_testCtx.getLog() << tcu::TestLog::Message << "PostDepthCoverage: enabled\n"
						   << "Vertex: " << program1b.getShaderInfo(SHADERTYPE_VERTEX).infoLog << "\n"
						   << "Fragment: " << program1b.getShaderInfo(SHADERTYPE_FRAGMENT).infoLog << "\n"
						   << "Program: " << program1b.getProgramInfo().infoLog << tcu::TestLog::EndMessage;
		return STOP;
	}

	ProgramSources sources2 = makeVtxFragSources(m_vertShader, m_fragShader2);
	ShaderProgram  program2(gl, sources2);

	if (!program2.isOk())
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Shader program fail (samples reader).");
		m_testCtx.getLog() << tcu::TestLog::Message << "Vertex: " << program2.getShaderInfo(SHADERTYPE_VERTEX).infoLog
						   << "\n"
						   << "Fragment: " << program2.getShaderInfo(SHADERTYPE_FRAGMENT).infoLog << "\n"
						   << "Program: " << program2.getProgramInfo().infoLog << tcu::TestLog::EndMessage;
		return STOP;
	}

	gl.clearColor(0.0f, 0.0f, 0.0f, 0.0f);
	gl.clearStencil(0);

	//Iterate through all cases
	for (int bufferCase = BUFFERCASE_FIRST; bufferCase <= BUFFERCASE_LAST; ++bufferCase)
		for (int pdcCase = PDCCASE_FIRST; pdcCase <= PDCCASE_LAST; ++pdcCase)
		{
			GLint program = 0;
			if (pdcCase == PDCCASE_DISABLED)
				program = program1a.getProgram();
			else if (pdcCase == PDCCASE_ENABLED)
				program = program1b.getProgram();

			gl.useProgram(program);

			//Firstible use multisampled framebuffer
			gl.bindFramebuffer(GL_FRAMEBUFFER, m_framebufferMS);
			gl.viewport(0, 0, 1, 1);

			gl.enable(GL_MULTISAMPLE);

			//Check which samples should be covered by first draw - calculate expected sample mask
			GLboolean expectedMask[4];

			for (GLint i = 0; i < 4; ++i)
			{
				GLfloat samplePos[2];
				gl.getMultisamplefv(GL_SAMPLE_POSITION, i, &samplePos[0]);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glGetMultisamplefv");
				if (pdcCase == PDCCASE_ENABLED && (samplePos[0] + samplePos[1]) < 0.7f)
					expectedMask[i] = false;
				else
					expectedMask[i] = true;
			}

			if (bufferCase == BUFFERCASE_DEPTH)
				gl.enable(GL_DEPTH_TEST);
			else if (bufferCase == BUFFERCASE_STENCIL)
				gl.enable(GL_STENCIL_TEST);

			gl.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

			if (bufferCase == BUFFERCASE_STENCIL)
			{
				gl.stencilFunc(GL_ALWAYS, 1, 0xFF);
				gl.stencilOp(GL_ZERO, GL_REPLACE, GL_REPLACE);
				gl.stencilMask(0xFF);
			}

			//Draw near primitive
			glu::draw(m_context.getRenderContext(), program, DE_LENGTH_OF_ARRAY(vertexArraysNear), vertexArraysNear,
					  glu::pr::TriangleStrip(DE_LENGTH_OF_ARRAY(indicesPre), indicesPre));

			gl.clear(GL_COLOR_BUFFER_BIT);

			if (bufferCase == BUFFERCASE_STENCIL)
			{
				gl.stencilFunc(GL_NOTEQUAL, 1, 0xFF);
				gl.stencilMask(0x00);
			}

			//Draw far primitive
			glu::draw(m_context.getRenderContext(), program, DE_LENGTH_OF_ARRAY(vertexArraysFar), vertexArraysFar,
					  glu::pr::TriangleStrip(DE_LENGTH_OF_ARRAY(indicesPre), indicesPre));

			gl.disable(GL_DEPTH_TEST);
			if (bufferCase == BUFFERCASE_DEPTH)
				gl.disable(GL_DEPTH_TEST);
			else if (bufferCase == BUFFERCASE_STENCIL)
				gl.disable(GL_STENCIL_TEST);

			gl.bindFramebuffer(GL_FRAMEBUFFER, 0);

			gl.useProgram(program2.getProgram());

			gl.bindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
			gl.viewport(0, 0, 1, 4);

			//Pass multisampled texture as a uniform
			gl.activeTexture(GL_TEXTURE0);
			gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_textureMS);
			gl.uniform1i(gl.getUniformLocation(program2.getProgram(), "texture"), 0);

			gl.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

			//Draw multisampled texture to framebuffer
			glu::draw(m_context.getRenderContext(), program2.getProgram(), DE_LENGTH_OF_ARRAY(vertexArraysPost),
					  vertexArraysPost, glu::pr::TriangleStrip(DE_LENGTH_OF_ARRAY(indicesPost), indicesPost));

			gl.disable(GL_MULTISAMPLE);

			//Read data from framebuffer
			GLubyte pixels[4 * 4];
			deMemset(pixels, 0, 4 * 4);

			gl.readPixels(0, 0, 1, 4, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels");

			//Verify the result
			GLboolean compiledMask[4];
			deMemset(compiledMask, 0, 4);
			for (int i = 0; i < 4; ++i)
			{
				for (int n			= 0; n < 4; ++n)
					compiledMask[n] = compiledMask[n] || (pixels[4 * i + n] != 0);
			}

			for (int n = 0; n < 4; ++n)
			{
				if (expectedMask[n] != compiledMask[n])
				{
					m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
					m_testCtx.getLog() << tcu::TestLog::Message << "Wrong results for "
									   << (bufferCase == BUFFERCASE_DEPTH ? "BUFFERCASE_DEPTH" :
																			"BUFFERCASE_DEPTH_STENCIL")
									   << " / "
									   << (pdcCase == PDCCASE_DISABLED ? "PDCCASE_DISABLED" : "PDCCASE_ENABLED")
									   << tcu::TestLog::EndMessage;
					return STOP;
				}
			}
		}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context.
 */
PostDepthCoverage::PostDepthCoverage(deqp::Context& context)
	: TestCaseGroup(context, "post_depth_coverage_tests",
					"Verify conformance of CTS_ARB_post_depth_coverage implementation")
{
}

/** Initializes the test group contents. */
void PostDepthCoverage::init()
{
	addChild(new PostDepthShaderCase(m_context));
	addChild(new PostDepthSampleMaskCase(m_context));
}

} /* gl4cts namespace */
