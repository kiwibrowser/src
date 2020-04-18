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
 * \brief Framebuffer without attachments (GL_ARB_framebuffer_no_attachments) tests.
 *//*--------------------------------------------------------------------*/

#include "es31fFboNoAttachmentTests.hpp"

#include "glwDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"

#include "gluRenderContext.hpp"
#include "gluDefs.hpp"
#include "gluShaderProgram.hpp"

#include "tcuTestContext.hpp"
#include "tcuVectorType.hpp"
#include "tcuVectorUtil.hpp"
#include "tcuTestLog.hpp"
#include "tcuCommandLine.hpp"
#include "tcuResultCollector.hpp"

#include "deMemory.h"
#include "deRandom.hpp"
#include "deString.h"
#include "deStringUtil.hpp"

#include <string>
#include <vector>

namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace
{

using namespace glw;

using tcu::IVec2;
using tcu::TestLog;

using std::stringstream;
using std::string;
using std::vector;

bool checkFramebufferSize (TestLog& log, const glu::RenderContext& renderCtx, GLuint framebuffer, const IVec2& size)
{
	const glw::Functions&		gl				= renderCtx.getFunctions();

	const char* const			vertexSource	= "#version 310 es\n"
												  "in layout(location = 0) highp vec2 a_position;\n\n"
												  "void main()\n"
												  "{\n"
												  "	gl_Position = vec4(a_position, 0.0, 1.0);\n"
												  "}\n";

	const char* const			fragmentSource	= "#version 310 es\n"
												  "uniform layout(location = 0) highp ivec2 u_expectedSize;\n"
												  "out layout(location = 0) mediump vec4 f_color;\n\n"
												  "void main()\n"
												  "{\n"
												  "	if (ivec2(gl_FragCoord.xy) != u_expectedSize) discard;\n"
												  "	f_color = vec4(1.0, 0.5, 0.25, 1.0);\n"
												  "}\n";

	const glu::ShaderProgram	program			(renderCtx, glu::makeVtxFragSources(vertexSource, fragmentSource));
	GLuint						query			= 0;
	GLuint						insidePassed	= 0;
	GLuint						outsideXPassed	= 0;
	GLuint						outsideYPassed	= 0;

	if (!program.isOk())
		log << program;

	TCU_CHECK(program.isOk());

	gl.useProgram(program.getProgram());
	gl.enable(GL_DEPTH_TEST);
	gl.depthFunc(GL_ALWAYS);
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
	gl.viewport(0, 0, size.x()*2, size.y()*2); // Oversized viewport so that it will not accidentally limit us to the correct size

	log << TestLog::Message << "Using " << size.x()*2 << "x" << size.y()*2 << " viewport" << TestLog::EndMessage;
	log << TestLog::Message << "Discarding fragments outside pixel of interest" << TestLog::EndMessage;
	log << TestLog::Message << "Using occlusion query to check for rendered fragments" << TestLog::EndMessage;

	TCU_CHECK(gl.checkFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

	// Render
	{
		const float data[] =
		{
			 1.0f,  1.0f,
			 1.0f, -1.0f,
			-1.0f,  1.0f,
			-1.0f,  1.0f,
			 1.0f, -1.0f,
			-1.0f, -1.0f,
		};

		GLuint vertexArray	= 0;
		GLuint vertexBuffer	= 0;

		gl.genQueries(1, &query);
		gl.genVertexArrays(1, &vertexArray);
		gl.bindVertexArray(vertexArray);

		gl.genBuffers(1, &vertexBuffer);
		gl.bindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
		gl.bufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);

		gl.enableVertexAttribArray(0);
		gl.vertexAttribPointer(0, 2, GL_FLOAT, false, 0, DE_NULL);

		gl.uniform2i(0, size.x()-1, size.y()-1);
		gl.beginQuery(GL_ANY_SAMPLES_PASSED, query);
		gl.drawArrays(GL_TRIANGLES, 0, 6);
		gl.endQuery(GL_ANY_SAMPLES_PASSED);
		gl.getQueryObjectuiv(query, GL_QUERY_RESULT, &insidePassed);
		log << TestLog::Message << "A fragment was not discarded at (" << size.x()-1 << ", " << size.y()-1 << "). "
			<< "Occlusion query reports it was " << (insidePassed > 0 ? "rendered." : "not rendered") << TestLog::EndMessage;

		gl.uniform2i(0, size.x(), size.y()-1);
		gl.beginQuery(GL_ANY_SAMPLES_PASSED, query);
		gl.drawArrays(GL_TRIANGLES, 0, 6);
		gl.endQuery(GL_ANY_SAMPLES_PASSED);
		gl.getQueryObjectuiv(query, GL_QUERY_RESULT, &outsideXPassed);
		log << TestLog::Message << "A fragment was not discarded at (" << size.x() << ", " << size.y()-1 << "). "
			<< "Occlusion query reports it was " << (outsideXPassed > 0 ? "rendered." : "not rendered") << TestLog::EndMessage;

		gl.uniform2i(0, size.x()-1, size.y());
		gl.beginQuery(GL_ANY_SAMPLES_PASSED, query);
		gl.drawArrays(GL_TRIANGLES, 0, 6);
		gl.endQuery(GL_ANY_SAMPLES_PASSED);
		gl.getQueryObjectuiv(query, GL_QUERY_RESULT, &outsideYPassed);
		log << TestLog::Message << "A fragment was not discarded at (" << size.x()-1 << ", " << size.y() << "). "
			<< "Occlusion query reports it was " << (outsideYPassed > 0 ? "rendered." : "not rendered") << TestLog::EndMessage;

		gl.disableVertexAttribArray(0);
		gl.bindBuffer(GL_ARRAY_BUFFER, 0);
		gl.bindVertexArray(0);
		gl.deleteBuffers(1, &vertexBuffer);
		gl.deleteVertexArrays(1, &vertexArray);
	}

	gl.deleteQueries(1, &query);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Query failed");

	return insidePassed && !outsideXPassed && !outsideYPassed;
}

bool checkFramebufferRenderable (TestLog& log, const glu::RenderContext& renderCtx, GLuint framebuffer, const IVec2& size)
{
	const glw::Functions&		gl				= renderCtx.getFunctions();

	const char* const			vertexSource	= "#version 310 es\n"
												  "in layout(location = 0) highp vec2 a_position;\n\n"
												  "void main()\n"
												  "{\n"
												  "	gl_Position = vec4(a_position, 0.0, 1.0);\n"
												  "}\n";

	const char* const			fragmentSource	= "#version 310 es\n"
												  "out layout(location = 0) mediump vec4 f_color;\n\n"
												  "void main()\n"
												  "{\n"
												  "	f_color = vec4(1.0, 0.5, 0.25, 1.0);\n"
												  "}\n";

	const glu::ShaderProgram	program			(renderCtx, glu::makeVtxFragSources(vertexSource, fragmentSource));
	GLuint						query			= 0;

	if (!program.isOk())
		log << program;

	TCU_CHECK(program.isOk());

	gl.useProgram(program.getProgram());
	gl.enable(GL_DEPTH_TEST);
	gl.depthFunc(GL_ALWAYS);
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
	gl.viewport(0, 0, size.x(), size.y());

	TCU_CHECK(gl.checkFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

	log << TestLog::Message << "Rendering full framebuffer quad with color ouput, verifying output presence with occlusion query" << TestLog::EndMessage;

	// Render
	{
		const float data[] =
		{
			 1.0f,  1.0f,
			 1.0f, -1.0f,
			-1.0f,  1.0f,
			-1.0f,  1.0f,
			 1.0f, -1.0f,
			-1.0f, -1.0f,
		};

		GLuint vertexArray	= 0;
		GLuint vertexBuffer	= 0;

		gl.genQueries(1, &query);
		gl.genVertexArrays(1, &vertexArray);
		gl.bindVertexArray(vertexArray);

		gl.genBuffers(1, &vertexBuffer);
		gl.bindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
		gl.bufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);

		gl.enableVertexAttribArray(0);
		gl.vertexAttribPointer(0, 2, GL_FLOAT, false, 0, DE_NULL);

		gl.beginQuery(GL_ANY_SAMPLES_PASSED, query);
		gl.drawArrays(GL_TRIANGLES, 0, 6);
		gl.endQuery(GL_ANY_SAMPLES_PASSED);

		gl.disableVertexAttribArray(0);
		gl.bindBuffer(GL_ARRAY_BUFFER, 0);
		gl.bindVertexArray(0);
		gl.deleteBuffers(1, &vertexBuffer);
		gl.deleteVertexArrays(1, &vertexArray);
	}

	// Read
	{
		GLuint passed = 0;

		gl.getQueryObjectuiv(query, GL_QUERY_RESULT, &passed);
		gl.deleteQueries(1, &query);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Query failed");

		if (passed)
			log << TestLog::Message << "Query passed" << TestLog::EndMessage;
		else
			log << TestLog::Message << "Query did not pass" << TestLog::EndMessage;

		return passed != 0;
	}
}

class FramebufferCompletenessCase : public tcu::TestCase
{
public:
								FramebufferCompletenessCase		(tcu::TestContext&			testCtx,
																 const glu::RenderContext&	renderCtx,
																 const char*				name,
																 const char*				desc);
	virtual						~FramebufferCompletenessCase	 (void) {}
	virtual IterateResult		iterate							(void);

private:
	const glu::RenderContext&	m_renderCtx;
	tcu::ResultCollector		m_results;
};

FramebufferCompletenessCase::FramebufferCompletenessCase (tcu::TestContext&			testCtx,
														  const glu::RenderContext&	renderCtx,
														  const char*				name,
														  const char*				desc)
	: TestCase		(testCtx, name, desc)
	, m_renderCtx	(renderCtx)
{
}

FramebufferCompletenessCase::IterateResult FramebufferCompletenessCase::iterate (void)
{
	const glw::Functions&	gl			= m_renderCtx.getFunctions();
	GLuint					framebuffer	= 0;

	gl.genFramebuffers(1, &framebuffer);
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);

	m_results.check(gl.checkFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE, "Framebuffer was incorrectly reported as complete when it had no width, height or attachments");

	gl.framebufferParameteri(GL_DRAW_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_WIDTH, 16);
	m_results.check(gl.checkFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE, "Framebuffer was incorrectly reported as complete when it only had a width");

	gl.framebufferParameteri(GL_DRAW_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_HEIGHT, 16);
	m_results.check(gl.checkFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer not reported as complete when it had width and height set");

	gl.framebufferParameteri(GL_DRAW_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_WIDTH, 0);
	m_results.check(gl.checkFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE, "Framebuffer was incorrectly reported as complete when it only had a height");

	gl.deleteFramebuffers(1, &framebuffer);

	m_results.setTestContextResult(m_testCtx);
	return STOP;
}

struct FboSpec
{
	int width;
	int height;
	int samples;

	FboSpec(int width_, int height_, int samples_) : width(width_), height(height_), samples(samples_){}
};

class SizeCase : public tcu::TestCase
{
public:
								SizeCase	(tcu::TestContext&			testCtx,
											 const glu::RenderContext&	renderCtx,
											 const char*				name,
											 const char*				desc,
											 const FboSpec&				spec);
	virtual						~SizeCase	(void) {}

	virtual IterateResult		iterate		(void);

	enum
	{
		USE_MAXIMUM = -1
	};
private:
	int							getWidth	(void) const;
	int							getHeight	(void) const;
	int							getSamples	(void) const;

	const glu::RenderContext&	m_renderCtx;

	const FboSpec				m_spec;
};

SizeCase::SizeCase (tcu::TestContext&			testCtx,
					const glu::RenderContext&	renderCtx,
					const char*					name,
					const char*					desc,
					const FboSpec&				spec)
	: TestCase		(testCtx, name, desc)
	, m_renderCtx	(renderCtx)
	, m_spec		(spec)
{
}

SizeCase::IterateResult SizeCase::iterate (void)
{
	const glw::Functions&	gl			= m_renderCtx.getFunctions();
	TestLog&				log			= m_testCtx.getLog();
	GLuint					framebuffer	= 0;
	const int				width		= getWidth();
	const int				height		= getHeight();
	const int				samples		= getSamples();

	gl.genFramebuffers(1, &framebuffer);
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
	gl.framebufferParameteri(GL_DRAW_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_WIDTH, width);
	gl.framebufferParameteri(GL_DRAW_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_HEIGHT, height);
	gl.framebufferParameteri(GL_DRAW_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_SAMPLES, samples);

	log << TestLog::Message << "Verifying " << width << "x" << height << " framebuffer with " << samples << "x multisampling" << TestLog::EndMessage;

	if(checkFramebufferRenderable(log, m_renderCtx, framebuffer, IVec2(width, height)) && checkFramebufferSize(log, m_renderCtx, framebuffer, IVec2(width, height)))
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Framebuffer did not behave as expected");

	gl.deleteFramebuffers(1, &framebuffer);

	return STOP;
}

int SizeCase::getWidth (void) const
{
	if (m_spec.width != USE_MAXIMUM)
		return m_spec.width;
	else
	{
		const glw::Functions&	gl		= m_renderCtx.getFunctions();
		GLint					width	= 0;

		gl.getIntegerv(GL_MAX_FRAMEBUFFER_WIDTH, &width);

		return width;
	}
}

int SizeCase::getHeight (void) const
{
	if (m_spec.height != USE_MAXIMUM)
		return m_spec.height;
	else
	{
		const glw::Functions&	gl		= m_renderCtx.getFunctions();
		GLint					height	= 0;

		gl.getIntegerv(GL_MAX_FRAMEBUFFER_HEIGHT, &height);

		return height;
	}
}

int SizeCase::getSamples (void) const
{
	if (m_spec.samples != USE_MAXIMUM)
		return m_spec.samples;
	else
	{
		const glw::Functions&	gl		= m_renderCtx.getFunctions();
		GLint					samples	= 0;

		gl.getIntegerv(GL_MAX_FRAMEBUFFER_SAMPLES, &samples);

		return samples;
	}
}

class AttachmentInteractionCase : public tcu::TestCase
{
public:
								AttachmentInteractionCase	(tcu::TestContext&			testCtx,
															 const glu::RenderContext&	renderCtx,
															 const char*				name,
															 const char*				desc,
															 const FboSpec&				defaultSpec,
															 const FboSpec&				attachmentSpec);
	virtual						~AttachmentInteractionCase	(void) {}

	virtual IterateResult		iterate						(void);

private:
	const glu::RenderContext&	m_renderCtx;
	const FboSpec				m_defaultSpec;
	const FboSpec				m_attachmentSpec;
};

AttachmentInteractionCase::AttachmentInteractionCase (tcu::TestContext&			testCtx,
													  const glu::RenderContext&	renderCtx,
													  const char*				name,
													  const char*				desc,
													  const FboSpec&			defaultSpec,
													  const FboSpec&			attachmentSpec)
	: TestCase			(testCtx, name, desc)
	, m_renderCtx		(renderCtx)
	, m_defaultSpec		(defaultSpec)
	, m_attachmentSpec	(attachmentSpec)
{
}

AttachmentInteractionCase::IterateResult AttachmentInteractionCase::iterate (void)
{
	const glw::Functions&	gl			= m_renderCtx.getFunctions();
	TestLog&				log			= m_testCtx.getLog();
	GLuint					framebuffer	= 0;
	GLuint					renderbuffer= 0;

	gl.genFramebuffers(1, &framebuffer);
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
	gl.framebufferParameteri(GL_DRAW_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_WIDTH, m_defaultSpec.width);
	gl.framebufferParameteri(GL_DRAW_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_HEIGHT, m_defaultSpec.height);
	gl.framebufferParameteri(GL_DRAW_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_SAMPLES, m_defaultSpec.samples);

	gl.genRenderbuffers(1, &renderbuffer);
	gl.bindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
	gl.renderbufferStorageMultisample(GL_RENDERBUFFER, m_attachmentSpec.samples, GL_RGBA8, m_attachmentSpec.width, m_attachmentSpec.height);
	gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, renderbuffer);

	log << TestLog::Message << "Verifying " << m_attachmentSpec.width << "x" << m_attachmentSpec.height << " framebuffer with " << m_attachmentSpec.samples << "x multisampling"
		<< " and defaults set to " << m_defaultSpec.width << "x" << m_defaultSpec.height << " with " << m_defaultSpec.samples << "x multisampling" << TestLog::EndMessage;

	if(checkFramebufferRenderable(log, m_renderCtx, framebuffer, IVec2(m_attachmentSpec.width, m_attachmentSpec.height))
	   && checkFramebufferSize(log, m_renderCtx, framebuffer, IVec2(m_attachmentSpec.width, m_attachmentSpec.height)))
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Framebuffer did not behave as expected");

	gl.deleteRenderbuffers(1, &renderbuffer);
	gl.deleteFramebuffers(1, &framebuffer);

	return STOP;
}

} // Anonymous

tcu::TestCaseGroup* createFboNoAttachmentTests(Context& context)
{
	const glu::RenderContext&	renderCtx	= context.getRenderContext();
	tcu::TestContext&			testCtx		= context.getTestContext();

	const int					maxWidth	= 2048; // MAX_FRAMEBUFFER_WIDTH in ES 3.1
	const int					maxHeight	= 2048; // MAX_FRAMEBUFFER_HEIGHT in ES 3.1
	const int					maxSamples	= 4;

	tcu::TestCaseGroup* const	root		= new tcu::TestCaseGroup(testCtx, "no_attachments", "Framebuffer without attachments");

	// Size
	{
		tcu::TestCaseGroup* const group = new tcu::TestCaseGroup(testCtx, "size", "Basic functionality tests with varying default size");

		root->addChild(group);

		for (int width = 16; width <= maxWidth; width *= 4)
		{
			for (int height = 16; height <= maxHeight; height *= 4)
			{
				const FboSpec	spec (width, height, 0);
				stringstream	name;

				name << width << "x" << height;

				group->addChild(new SizeCase(testCtx, renderCtx, name.str().c_str(), name.str().c_str(), spec));
			}
		}
	}

	// NPOT size
	{
		const FboSpec specs[] =
		{
			// Square
			FboSpec(1,    1,    0),
			FboSpec(3,    3,    0),
			FboSpec(15,   15,   0),
			FboSpec(17,   17,   0),
			FboSpec(31,   31,   0),
			FboSpec(33,   33,   0),
			FboSpec(63,   63,   0),
			FboSpec(65,   65,   0),
			FboSpec(127,  127,  0),
			FboSpec(129,  129,  0),
			FboSpec(255,  255,  0),
			FboSpec(257,  257,  0),
			FboSpec(511,  511,  0),
			FboSpec(513,  513,  0),
			FboSpec(1023, 1023, 0),
			FboSpec(1025, 1025, 0),
			FboSpec(2047, 2047, 0),

			// Non-square
			FboSpec(15,   511,  0),
			FboSpec(127,  15,   0),
			FboSpec(129,  127,  0),
			FboSpec(511,  127,  0),
			FboSpec(2047, 1025, 0),
		};
		tcu::TestCaseGroup* const group = new tcu::TestCaseGroup(testCtx, "npot_size", "Basic functionality with Non-power-of-two size");

		root->addChild(group);

		for (int caseNdx = 0; caseNdx < DE_LENGTH_OF_ARRAY(specs); caseNdx++)
		{
			const FboSpec&	spec = specs[caseNdx];
			stringstream	name;

			name << spec.width << "x" << spec.height;

			group->addChild(new SizeCase(testCtx, renderCtx, name.str().c_str(), name.str().c_str(), spec));
		}
	}

	// Multisample
	{
		tcu::TestCaseGroup* const group = new tcu::TestCaseGroup(testCtx, "multisample", "Basic functionality with multisampled fbo");

		root->addChild(group);

		for (int samples = 0; samples <= maxSamples; samples++)
		{
			const FboSpec	spec (128, 128, samples);
			stringstream	name;

			name << "samples" << samples;

			group->addChild(new SizeCase(testCtx, renderCtx, name.str().c_str(), name.str().c_str(), spec));
		}
	}

	// Randomized
	{
		tcu::TestCaseGroup* const	group	= new tcu::TestCaseGroup(testCtx, "random", "Randomized size & multisampling");
		de::Random					rng		(0xF0E1E2D3 ^ testCtx.getCommandLine().getBaseSeed());

		root->addChild(group);

		for (int caseNdx = 0; caseNdx < 16; caseNdx++)
		{
			const int		width	= rng.getInt(1, maxWidth);
			const int		height	= rng.getInt(1, maxHeight);
			const int		samples = rng.getInt(0, maxSamples);
			const FboSpec	spec	(width, height, samples);
			const string	name	= de::toString(caseNdx);

			group->addChild(new SizeCase(testCtx, renderCtx, name.c_str(), name.c_str(), spec));
		}
	}

	// Normal fbo with defaults set
	{
		tcu::TestCaseGroup* const group = new tcu::TestCaseGroup(testCtx, "interaction", "Interaction of default parameters with normal fbo");

		root->addChild(group);

		const FboSpec specs[][2] =
		{
			{ FboSpec(256,  256,  0), FboSpec(128,  128,  1) },
			{ FboSpec(256,  256,  1), FboSpec(128,  128,  0) },
			{ FboSpec(256,  256,  0), FboSpec(512,  512,  2) },
			{ FboSpec(256,  256,  2), FboSpec(128,  512,  0) },
			{ FboSpec(127,  127,  0), FboSpec(129,  129,  0) },
			{ FboSpec(17,   512,  4), FboSpec(16,   16,   2) },
			{ FboSpec(2048, 2048, 4), FboSpec(1,    1,    0) },
			{ FboSpec(1,    1,    0), FboSpec(2048, 2048, 4) },
		};

		for (int specNdx = 0; specNdx < DE_LENGTH_OF_ARRAY(specs); specNdx++)
		{
			const FboSpec& baseSpec = specs[specNdx][0];
			const FboSpec& altSpec	= specs[specNdx][1];
			stringstream baseSpecName, altSpecName;

			baseSpecName << baseSpec.width << "x" << baseSpec.height << "ms" << baseSpec.samples;
			altSpecName << altSpec.width << "x" << altSpec.height << "ms" << altSpec.samples;

			{
				const string name = baseSpecName.str() + "_default_" + altSpecName.str();

				group->addChild(new AttachmentInteractionCase(testCtx, renderCtx, name.c_str(), name.c_str(), altSpec, baseSpec));
			}
		}
	}

	// Maximums
	{
		tcu::TestCaseGroup* const	group	= new tcu::TestCaseGroup(testCtx, "maximums", "Maximum dimensions");

		root->addChild(group);
		group->addChild(new SizeCase(testCtx, renderCtx, "width",	"Maximum width",		  FboSpec(SizeCase::USE_MAXIMUM,	128,					0)));
		group->addChild(new SizeCase(testCtx, renderCtx, "height",	"Maximum height",		  FboSpec(128,						SizeCase::USE_MAXIMUM,  0)));
		group->addChild(new SizeCase(testCtx, renderCtx, "size",	"Maximum size",			  FboSpec(SizeCase::USE_MAXIMUM,	SizeCase::USE_MAXIMUM,  0)));
		group->addChild(new SizeCase(testCtx, renderCtx, "samples", "Maximum samples",		  FboSpec(128,						128,					SizeCase::USE_MAXIMUM)));
		group->addChild(new SizeCase(testCtx, renderCtx, "all",		"Maximum size & samples", FboSpec(SizeCase::USE_MAXIMUM,	SizeCase::USE_MAXIMUM,  SizeCase::USE_MAXIMUM)));
	}

	return root;
}

tcu::TestCaseGroup* createFboNoAttachmentCompletenessTests(Context& context)
{
	TestCaseGroup* const group = new TestCaseGroup(context, "completeness", "Completeness tests");

	group->addChild(new FramebufferCompletenessCase(context.getTestContext(), context.getRenderContext(), "no_attachments", "No attachments completeness"));

	return group;
}

} // Functional
} // gles31
} // deqp
