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
 * \brief Vertex attribute binding tests.
 *//*--------------------------------------------------------------------*/

#include "es31fVertexAttributeBindingTests.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuSurface.hpp"
#include "gluCallLogWrapper.hpp"
#include "gluRenderContext.hpp"
#include "gluPixelTransfer.hpp"
#include "gluShaderProgram.hpp"
#include "gluObjectWrapper.hpp"
#include "gluStrUtil.hpp"
#include "glwFunctions.hpp"
#include "glwEnums.hpp"
#include "deStringUtil.hpp"
#include "deInt32.h"

namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace
{

static const char* const s_colorFragmentShader =		"#version 310 es\n"
														"in mediump vec4 v_color;\n"
														"layout(location = 0) out mediump vec4 fragColor;\n"
														"void main (void)\n"
														"{\n"
														"	fragColor = v_color;\n"
														"}\n";

static const char* const s_positionColorShader =		"#version 310 es\n"
														"in highp vec4 a_position;\n"
														"in highp vec4 a_color;\n"
														"out highp vec4 v_color;\n"
														"void main (void)\n"
														"{\n"
														"	gl_Position = a_position;\n"
														"	v_color = a_color;\n"
														"}\n";

static const char* const s_positionColorOffsetShader =	"#version 310 es\n"
														"in highp vec4 a_position;\n"
														"in highp vec4 a_offset;\n"
														"in highp vec4 a_color;\n"
														"out highp vec4 v_color;\n"
														"void main (void)\n"
														"{\n"
														"	gl_Position = a_position + a_offset;\n"
														"	v_color = a_color;\n"
														"}\n";

// Verifies image contains only yellow or greeen, or a linear combination
// of these colors.
static bool verifyImageYellowGreen (const tcu::Surface& image, tcu::TestLog& log, bool logImageOnSuccess)
{
	using tcu::TestLog;

	const int colorThreshold	= 20;

	tcu::Surface error			(image.getWidth(), image.getHeight());
	bool isOk					= true;

	log << TestLog::Message << "Verifying image contents." << TestLog::EndMessage;

	for (int y = 0; y < image.getHeight(); y++)
	for (int x = 0; x < image.getWidth(); x++)
	{
		const tcu::RGBA pixel = image.getPixel(x, y);
		bool pixelOk = true;

		// Any pixel with !(G ~= 255) is faulty (not a linear combinations of green and yellow)
		if (de::abs(pixel.getGreen() - 255) > colorThreshold)
			pixelOk = false;

		// Any pixel with !(B ~= 0) is faulty (not a linear combinations of green and yellow)
		if (de::abs(pixel.getBlue() - 0) > colorThreshold)
			pixelOk = false;

		error.setPixel(x, y, (pixelOk) ? (tcu::RGBA(0, 255, 0, 255)) : (tcu::RGBA(255, 0, 0, 255)));
		isOk = isOk && pixelOk;
	}

	if (!isOk)
	{
		log << TestLog::Message << "Image verification failed." << TestLog::EndMessage;
		log << TestLog::ImageSet("Verfication result", "Result of rendering")
			<< TestLog::Image("Result",		"Result",		image)
			<< TestLog::Image("ErrorMask",	"Error mask",	error)
			<< TestLog::EndImageSet;
	}
	else
	{
		log << TestLog::Message << "Image verification passed." << TestLog::EndMessage;

		if (logImageOnSuccess)
			log << TestLog::ImageSet("Verfication result", "Result of rendering")
				<< TestLog::Image("Result", "Result", image)
				<< TestLog::EndImageSet;
	}

	return isOk;
}

class BindingRenderCase : public TestCase
{
public:
	enum
	{
		TEST_RENDER_SIZE = 64
	};

						BindingRenderCase	(Context& ctx, const char* name, const char* desc, bool unalignedData);
	virtual				~BindingRenderCase	(void);

	virtual void		init				(void);
	virtual void		deinit				(void);
	IterateResult		iterate				(void);

private:
	virtual void		renderTo			(tcu::Surface& dst) = 0;
	virtual void		createBuffers		(void) = 0;
	virtual void		createShader		(void) = 0;

protected:
	const bool			m_unalignedData;
	glw::GLuint			m_vao;
	glu::ShaderProgram*	m_program;
};

BindingRenderCase::BindingRenderCase (Context& ctx, const char* name, const char* desc, bool unalignedData)
	: TestCase			(ctx, name, desc)
	, m_unalignedData	(unalignedData)
	, m_vao				(0)
	, m_program			(DE_NULL)
{
}

BindingRenderCase::~BindingRenderCase (void)
{
	deinit();
}

void BindingRenderCase::init (void)
{
	// check requirements
	if (m_context.getRenderTarget().getWidth() < TEST_RENDER_SIZE || m_context.getRenderTarget().getHeight() < TEST_RENDER_SIZE)
		throw tcu::NotSupportedError("Test requires at least " + de::toString<int>(TEST_RENDER_SIZE) + "x" + de::toString<int>(TEST_RENDER_SIZE) + " render target");

	// resources
	m_context.getRenderContext().getFunctions().genVertexArrays(1, &m_vao);
	if (m_context.getRenderContext().getFunctions().getError() != GL_NO_ERROR)
		throw tcu::TestError("could not gen vao");

	createBuffers();
	createShader();
}

void BindingRenderCase::deinit (void)
{
	if (m_vao)
	{
		m_context.getRenderContext().getFunctions().deleteVertexArrays(1, &m_vao);
		m_vao = 0;
	}

	delete m_program;
	m_program = DE_NULL;
}

BindingRenderCase::IterateResult BindingRenderCase::iterate (void)
{
	tcu::Surface surface(TEST_RENDER_SIZE, TEST_RENDER_SIZE);

	// draw pattern

	renderTo(surface);

	// verify results

	if (verifyImageYellowGreen(surface, m_testCtx.getLog(), false))
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else if (m_unalignedData)
		m_testCtx.setTestResult(QP_TEST_RESULT_COMPATIBILITY_WARNING, "Failed to draw with unaligned data");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Image verification failed");

	return STOP;
}

class SingleBindingCase : public BindingRenderCase
{
public:

	enum CaseFlag
	{
		FLAG_ATTRIB_UNALIGNED			= (1<<0),		// !< unalign attributes with relativeOffset
		FLAG_ATTRIB_ALIGNED				= (1<<1),		// !< align attributes with relativeOffset to the buffer begin (and not buffer offset)
		FLAG_ATTRIBS_MULTIPLE_ELEMS		= (1<<2),		// !< use multiple attribute elements
		FLAG_ATTRIBS_SHARED_ELEMS		= (1<<3),		// !< use multiple shared attribute elements. xyzw & rgba stored as (x, y, zr, wg, b, a)

		FLAG_BUF_ALIGNED_OFFSET			= (1<<4),		// !< use aligned offset to the buffer object
		FLAG_BUF_UNALIGNED_OFFSET		= (1<<5),		// !< use unaligned offset to the buffer object
		FLAG_BUF_UNALIGNED_STRIDE		= (1<<6),		// !< unalign buffer elements
	};
						SingleBindingCase	(Context& ctx, const char* name, int flags);
						~SingleBindingCase	(void);

	void				init				(void);
	void				deinit				(void);

private:
	struct TestSpec
	{
		int		bufferOffset;
		int		bufferStride;
		int		positionAttrOffset;
		int		colorAttrOffset;
		bool	hasColorAttr;
	};

	enum
	{
		GRID_SIZE = 20
	};

	void				renderTo			(tcu::Surface& dst);

	static TestSpec		genTestSpec			(int flags);
	static std::string	genTestDescription	(int flags);
	static bool			isDataUnaligned		(int flags);

	void				createBuffers		(void);
	void				createShader		(void);
	std::string			genVertexSource		(void);

	const TestSpec		m_spec;
	glw::GLuint			m_buf;
};

SingleBindingCase::SingleBindingCase (Context& ctx, const char* name, int flags)
	: BindingRenderCase	(ctx, name, genTestDescription(flags).c_str(), isDataUnaligned(flags))
	, m_spec			(genTestSpec(flags))
	, m_buf				(0)
{
	DE_ASSERT(!((flags & FLAG_ATTRIB_UNALIGNED) && (flags & FLAG_ATTRIB_ALIGNED)));
	DE_ASSERT(!((flags & FLAG_ATTRIB_ALIGNED) && (flags & FLAG_BUF_UNALIGNED_STRIDE)));

	DE_ASSERT(!isDataUnaligned(flags));
}

SingleBindingCase::~SingleBindingCase (void)
{
	deinit();
}

void SingleBindingCase::init (void)
{
	// log what we are trying to do

	m_testCtx.getLog()	<< tcu::TestLog::Message
						<< "Rendering " << (int)GRID_SIZE << "x" << (int)GRID_SIZE << " grid.\n"
						<< "Buffer format:\n"
						<< "	bufferOffset: " << m_spec.bufferOffset << "\n"
						<< "	bufferStride: " << m_spec.bufferStride << "\n"
						<< "Vertex position format:\n"
						<< "	type: float4\n"
						<< "	offset: " << m_spec.positionAttrOffset << "\n"
						<< "	total offset: " << m_spec.bufferOffset + m_spec.positionAttrOffset << "\n"
						<< tcu::TestLog::EndMessage;

	if (m_spec.hasColorAttr)
		m_testCtx.getLog()	<< tcu::TestLog::Message
							<< "Color:\n"
							<< "	type: float4\n"
							<< "	offset: " << m_spec.colorAttrOffset << "\n"
							<< "	total offset: " << m_spec.bufferOffset + m_spec.colorAttrOffset << "\n"
							<< tcu::TestLog::EndMessage;
	// init

	BindingRenderCase::init();
}

void SingleBindingCase::deinit (void)
{
	if (m_buf)
	{
		m_context.getRenderContext().getFunctions().deleteBuffers(1, &m_buf);
		m_buf = 0;
	}

	BindingRenderCase::deinit();
}

void SingleBindingCase::renderTo (tcu::Surface& dst)
{
	glu::CallLogWrapper gl				(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	const int			positionLoc		= gl.glGetAttribLocation(m_program->getProgram(), "a_position");
	const int			colorLoc		= gl.glGetAttribLocation(m_program->getProgram(), "a_color");
	const int			colorUniformLoc	= gl.glGetUniformLocation(m_program->getProgram(), "u_color");

	gl.enableLogging(true);

	gl.glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	gl.glClear(GL_COLOR_BUFFER_BIT);
	gl.glViewport(0, 0, dst.getWidth(), dst.getHeight());
	gl.glBindVertexArray(m_vao);
	GLU_EXPECT_NO_ERROR(gl.glGetError(), "set vao");

	gl.glUseProgram(m_program->getProgram());
	GLU_EXPECT_NO_ERROR(gl.glGetError(), "use program");

	if (m_spec.hasColorAttr)
	{
		gl.glBindVertexBuffer(3, m_buf, m_spec.bufferOffset, m_spec.bufferStride);

		gl.glVertexAttribBinding(positionLoc, 3);
		gl.glVertexAttribFormat(positionLoc, 4, GL_FLOAT, GL_FALSE, m_spec.positionAttrOffset);
		gl.glEnableVertexAttribArray(positionLoc);

		gl.glVertexAttribBinding(colorLoc, 3);
		gl.glVertexAttribFormat(colorLoc, 4, GL_FLOAT, GL_FALSE, m_spec.colorAttrOffset);
		gl.glEnableVertexAttribArray(colorLoc);

		GLU_EXPECT_NO_ERROR(gl.glGetError(), "set va");

		gl.glDrawArrays(GL_TRIANGLES, 0, GRID_SIZE*GRID_SIZE*6);
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "draw");
	}
	else
	{
		gl.glBindVertexBuffer(3, m_buf, m_spec.bufferOffset, m_spec.bufferStride);
		gl.glVertexAttribBinding(positionLoc, 3);
		gl.glVertexAttribFormat(positionLoc, 4, GL_FLOAT, GL_FALSE, m_spec.positionAttrOffset);
		gl.glEnableVertexAttribArray(positionLoc);

		GLU_EXPECT_NO_ERROR(gl.glGetError(), "set va");
		gl.glUniform4f(colorUniformLoc, 0.0f, 1.0f, 0.0f, 1.0f);

		gl.glDrawArrays(GL_TRIANGLES, 0, GRID_SIZE*GRID_SIZE*6);
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "draw");
	}

	gl.glFinish();
	gl.glBindVertexArray(0);
	gl.glUseProgram(0);
	GLU_EXPECT_NO_ERROR(gl.glGetError(), "clean");

	glu::readPixels(m_context.getRenderContext(), 0, 0, dst.getAccess());
}

SingleBindingCase::TestSpec SingleBindingCase::genTestSpec (int flags)
{
	const int	datumSize				= 4;
	const int	bufferOffset			= (flags & FLAG_BUF_ALIGNED_OFFSET) ? (32) : (flags & FLAG_BUF_UNALIGNED_OFFSET) ? (19) : (0);
	const int	attrBufAlignment		= ((bufferOffset % datumSize) == 0) ? (0) : (datumSize - (bufferOffset % datumSize));
	const int	positionAttrOffset		= (flags & FLAG_ATTRIB_UNALIGNED) ? (3) : (flags & FLAG_ATTRIB_ALIGNED) ? (attrBufAlignment) : (0);
	const bool	hasColorAttr			= (flags & FLAG_ATTRIBS_SHARED_ELEMS) || (flags & FLAG_ATTRIBS_MULTIPLE_ELEMS);
	const int	colorAttrOffset			= (flags & FLAG_ATTRIBS_SHARED_ELEMS) ? (2 * datumSize) : (flags & FLAG_ATTRIBS_MULTIPLE_ELEMS) ? (4 * datumSize) : (-1);

	const int	bufferStrideBase		= de::max(positionAttrOffset + 4 * datumSize, colorAttrOffset + 4 * datumSize);
	const int	bufferStrideAlignment	= ((bufferStrideBase % datumSize) == 0) ? (0) : (datumSize - (bufferStrideBase % datumSize));
	const int	bufferStridePadding		= ((flags & FLAG_BUF_UNALIGNED_STRIDE) && deIsAligned32(bufferStrideBase, datumSize)) ? (13) : (!(flags & FLAG_BUF_UNALIGNED_STRIDE) && !deIsAligned32(bufferStrideBase, datumSize)) ? (bufferStrideAlignment) : (0);

	TestSpec spec;

	spec.bufferOffset			= bufferOffset;
	spec.bufferStride			= bufferStrideBase + bufferStridePadding;
	spec.positionAttrOffset		= positionAttrOffset;
	spec.colorAttrOffset		= colorAttrOffset;
	spec.hasColorAttr			= hasColorAttr;

	if (flags & FLAG_ATTRIB_UNALIGNED)
		DE_ASSERT(!deIsAligned32(spec.bufferOffset + spec.positionAttrOffset, datumSize));
	else if (flags & FLAG_ATTRIB_ALIGNED)
		DE_ASSERT(deIsAligned32(spec.bufferOffset + spec.positionAttrOffset, datumSize));

	if (flags & FLAG_BUF_UNALIGNED_STRIDE)
		DE_ASSERT(!deIsAligned32(spec.bufferStride, datumSize));
	else
		DE_ASSERT(deIsAligned32(spec.bufferStride, datumSize));

	return spec;
}

std::string SingleBindingCase::genTestDescription (int flags)
{
	std::ostringstream buf;
	buf << "draw test pattern";

	if (flags & FLAG_ATTRIB_UNALIGNED)
		buf << ", attribute offset (unaligned)";
	if (flags & FLAG_ATTRIB_ALIGNED)
		buf << ", attribute offset (aligned)";

	if (flags & FLAG_ATTRIBS_MULTIPLE_ELEMS)
		buf << ", 2 attributes";
	if (flags & FLAG_ATTRIBS_SHARED_ELEMS)
		buf << ", 2 attributes (some components shared)";

	if (flags & FLAG_BUF_ALIGNED_OFFSET)
		buf << ", buffer offset aligned";
	if (flags & FLAG_BUF_UNALIGNED_OFFSET)
		buf << ", buffer offset unaligned";
	if (flags & FLAG_BUF_UNALIGNED_STRIDE)
		buf << ", buffer stride unaligned";

	return buf.str();
}

bool SingleBindingCase::isDataUnaligned (int flags)
{
	if (flags & FLAG_ATTRIB_UNALIGNED)
		return true;
	if (flags & FLAG_ATTRIB_ALIGNED)
		return false;

	return (flags & FLAG_BUF_UNALIGNED_OFFSET) || (flags & FLAG_BUF_UNALIGNED_STRIDE);
}

void SingleBindingCase::createBuffers (void)
{
	const glw::Functions&	gl		= m_context.getRenderContext().getFunctions();
	std::vector<deUint8>	dataBuf	(m_spec.bufferOffset + m_spec.bufferStride * GRID_SIZE * GRID_SIZE * 6);

	// In interleaved mode color rg and position zw are the same. Select "good" values for r and g
	const tcu::Vec4			colorA	(0.0f, 1.0f, 0.0f, 1.0f);
	const tcu::Vec4			colorB	(0.5f, 1.0f, 0.0f, 1.0f);

	for (int y = 0; y < GRID_SIZE; ++y)
	for (int x = 0; x < GRID_SIZE; ++x)
	{
		const tcu::Vec4&	color = ((x + y) % 2 == 0) ? (colorA) : (colorB);
		const tcu::Vec4		positions[6] =
		{
			tcu::Vec4(float(x+0) / float(GRID_SIZE) * 2.0f - 1.0f, float(y+0) / float(GRID_SIZE) * 2.0f - 1.0f, 0.0f, 1.0f),
			tcu::Vec4(float(x+0) / float(GRID_SIZE) * 2.0f - 1.0f, float(y+1) / float(GRID_SIZE) * 2.0f - 1.0f, 0.0f, 1.0f),
			tcu::Vec4(float(x+1) / float(GRID_SIZE) * 2.0f - 1.0f, float(y+1) / float(GRID_SIZE) * 2.0f - 1.0f, 0.0f, 1.0f),
			tcu::Vec4(float(x+0) / float(GRID_SIZE) * 2.0f - 1.0f, float(y+0) / float(GRID_SIZE) * 2.0f - 1.0f, 0.0f, 1.0f),
			tcu::Vec4(float(x+1) / float(GRID_SIZE) * 2.0f - 1.0f, float(y+1) / float(GRID_SIZE) * 2.0f - 1.0f, 0.0f, 1.0f),
			tcu::Vec4(float(x+1) / float(GRID_SIZE) * 2.0f - 1.0f, float(y+0) / float(GRID_SIZE) * 2.0f - 1.0f, 0.0f, 1.0f),
		};

		// copy cell vertices to the buffer.
		for (int v = 0; v < 6; ++v)
			memcpy(&dataBuf[m_spec.bufferOffset + m_spec.positionAttrOffset + m_spec.bufferStride * ((y * GRID_SIZE + x) * 6 + v)], positions[v].getPtr(), sizeof(positions[v]));

		// copy color to buffer
		if (m_spec.hasColorAttr)
			for (int v = 0; v < 6; ++v)
				memcpy(&dataBuf[m_spec.bufferOffset + m_spec.colorAttrOffset + m_spec.bufferStride * ((y * GRID_SIZE + x) * 6 + v)], color.getPtr(), sizeof(color));
	}

	gl.genBuffers(1, &m_buf);
	gl.bindBuffer(GL_ARRAY_BUFFER, m_buf);
	gl.bufferData(GL_ARRAY_BUFFER, (glw::GLsizeiptr)dataBuf.size(), &dataBuf[0], GL_STATIC_DRAW);
	gl.bindBuffer(GL_ARRAY_BUFFER, 0);

	if (gl.getError() != GL_NO_ERROR)
		throw tcu::TestError("could not init buffer");
}

void SingleBindingCase::createShader (void)
{
	m_program = new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources() << glu::VertexSource(genVertexSource()) << glu::FragmentSource(s_colorFragmentShader));
	m_testCtx.getLog() << *m_program;

	if (!m_program->isOk())
		throw tcu::TestError("could not build shader");
}

std::string SingleBindingCase::genVertexSource (void)
{
	const bool			useUniformColor = !m_spec.hasColorAttr;
	std::ostringstream	buf;

	buf <<	"#version 310 es\n"
			"in highp vec4 a_position;\n";

	if (!useUniformColor)
		buf << "in highp vec4 a_color;\n";
	else
		buf << "uniform highp vec4 u_color;\n";

	buf <<	"out highp vec4 v_color;\n"
			"void main (void)\n"
			"{\n"
			"	gl_Position = a_position;\n"
			"	v_color = " << ((useUniformColor) ? ("u_color") : ("a_color")) << ";\n"
			"}\n";

	return buf.str();
}

class MultipleBindingCase : public BindingRenderCase
{
public:

	enum CaseFlag
	{
		FLAG_ZERO_STRIDE		= (1<<0),	// !< set a buffer stride to zero
		FLAG_INSTANCED			= (1<<1),	// !< set a buffer instance divisor to non-zero
		FLAG_ALIASING_BUFFERS	= (1<<2),	// !< bind buffer to multiple binding points
	};

						MultipleBindingCase		(Context& ctx, const char* name, int flags);
						~MultipleBindingCase	(void);

	void				init					(void);
	void				deinit					(void);

private:
	struct TestSpec
	{
		bool zeroStride;
		bool instanced;
		bool aliasingBuffers;
	};

	enum
	{
		GRID_SIZE = 20
	};

	void				renderTo				(tcu::Surface& dst);

	TestSpec			genTestSpec				(int flags) const;
	std::string			genTestDescription		(int flags) const;
	void				createBuffers			(void);
	void				createShader			(void);

	const TestSpec		m_spec;
	glw::GLuint			m_primitiveBuf;
	glw::GLuint			m_colorOffsetBuf;
};

MultipleBindingCase::MultipleBindingCase (Context& ctx, const char* name, int flags)
	: BindingRenderCase	(ctx, name, genTestDescription(flags).c_str(), false)
	, m_spec			(genTestSpec(flags))
	, m_primitiveBuf	(0)
	, m_colorOffsetBuf	(0)
{
	DE_ASSERT(!(m_spec.instanced && m_spec.zeroStride));
}

MultipleBindingCase::~MultipleBindingCase (void)
{
	deinit();
}

void MultipleBindingCase::init (void)
{
	BindingRenderCase::init();

	// log what we are trying to do

	m_testCtx.getLog()	<< tcu::TestLog::Message
						<< "Rendering " << (int)GRID_SIZE << "x" << (int)GRID_SIZE << " grid.\n"
						<< "Vertex positions:\n"
						<< "	binding point: 1\n"
						<< "Vertex offsets:\n"
						<< "	binding point: 2\n"
						<< "Vertex colors:\n"
						<< "	binding point: 2\n"
						<< "Binding point 1:\n"
						<< "	buffer object: " << m_primitiveBuf << "\n"
						<< "Binding point 2:\n"
						<< "	buffer object: " << ((m_spec.aliasingBuffers) ? (m_primitiveBuf) : (m_colorOffsetBuf)) << "\n"
						<< "	instance divisor: " << ((m_spec.instanced) ? (1) : (0)) << "\n"
						<< "	stride: " << ((m_spec.zeroStride) ? (0) : (4*4*2)) << "\n"
						<< tcu::TestLog::EndMessage;
}

void MultipleBindingCase::deinit (void)
{
	if (m_primitiveBuf)
	{
		m_context.getRenderContext().getFunctions().deleteBuffers(1, &m_primitiveBuf);
		m_primitiveBuf = DE_NULL;
	}

	if (m_colorOffsetBuf)
	{
		m_context.getRenderContext().getFunctions().deleteBuffers(1, &m_colorOffsetBuf);
		m_colorOffsetBuf = DE_NULL;
	}

	BindingRenderCase::deinit();
}

void MultipleBindingCase::renderTo (tcu::Surface& dst)
{
	glu::CallLogWrapper gl					(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	const int			positionLoc			= gl.glGetAttribLocation(m_program->getProgram(), "a_position");
	const int			colorLoc			= gl.glGetAttribLocation(m_program->getProgram(), "a_color");
	const int			offsetLoc			= gl.glGetAttribLocation(m_program->getProgram(), "a_offset");

	const int			positionBinding		= 1;
	const int			colorOffsetBinding	= 2;

	gl.enableLogging(true);

	gl.glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	gl.glClear(GL_COLOR_BUFFER_BIT);
	gl.glViewport(0, 0, dst.getWidth(), dst.getHeight());
	gl.glBindVertexArray(m_vao);
	GLU_EXPECT_NO_ERROR(gl.glGetError(), "set vao");

	gl.glUseProgram(m_program->getProgram());
	GLU_EXPECT_NO_ERROR(gl.glGetError(), "use program");

	// Setup format & binding

	gl.glEnableVertexAttribArray(positionLoc);
	gl.glEnableVertexAttribArray(colorLoc);
	gl.glEnableVertexAttribArray(offsetLoc);

	gl.glVertexAttribFormat(positionLoc, 4, GL_FLOAT, GL_FALSE, 0);
	gl.glVertexAttribFormat(colorLoc, 4, GL_FLOAT, GL_FALSE, 0);
	gl.glVertexAttribFormat(offsetLoc, 4, GL_FLOAT, GL_FALSE, sizeof(tcu::Vec4));

	gl.glVertexAttribBinding(positionLoc, positionBinding);
	gl.glVertexAttribBinding(colorLoc, colorOffsetBinding);
	gl.glVertexAttribBinding(offsetLoc, colorOffsetBinding);

	GLU_EXPECT_NO_ERROR(gl.glGetError(), "setup attribs");

	// setup binding points

	gl.glVertexBindingDivisor(positionBinding, 0);
	gl.glBindVertexBuffer(positionBinding, m_primitiveBuf, 0, sizeof(tcu::Vec4));

	{
		const int			stride	= (m_spec.zeroStride) ? (0) : (2 * (int)sizeof(tcu::Vec4));
		const int			offset	= (!m_spec.aliasingBuffers) ? (0) : (m_spec.instanced) ? (6 * (int)sizeof(tcu::Vec4)) : (6 * GRID_SIZE * GRID_SIZE * (int)sizeof(tcu::Vec4));
		const glw::GLuint	buffer	= (m_spec.aliasingBuffers) ? (m_primitiveBuf) : (m_colorOffsetBuf);
		const int			divisor	= (m_spec.instanced) ? (1) : (0);

		gl.glVertexBindingDivisor(colorOffsetBinding, divisor);
		gl.glBindVertexBuffer(colorOffsetBinding, buffer, offset, (glw::GLsizei)stride);
	}

	GLU_EXPECT_NO_ERROR(gl.glGetError(), "set binding points");

	if (m_spec.instanced)
		gl.glDrawArraysInstanced(GL_TRIANGLES, 0, 6, GRID_SIZE*GRID_SIZE);
	else
		gl.glDrawArrays(GL_TRIANGLES, 0, GRID_SIZE*GRID_SIZE*6);
	GLU_EXPECT_NO_ERROR(gl.glGetError(), "draw");

	gl.glFinish();
	gl.glBindVertexArray(0);
	gl.glUseProgram(0);
	GLU_EXPECT_NO_ERROR(gl.glGetError(), "clean");

	glu::readPixels(m_context.getRenderContext(), 0, 0, dst.getAccess());
}

MultipleBindingCase::TestSpec MultipleBindingCase::genTestSpec (int flags) const
{
	MultipleBindingCase::TestSpec spec;

	spec.zeroStride			= !!(flags & FLAG_ZERO_STRIDE);
	spec.instanced			= !!(flags & FLAG_INSTANCED);
	spec.aliasingBuffers	= !!(flags & FLAG_ALIASING_BUFFERS);

	return spec;
}

std::string MultipleBindingCase::genTestDescription (int flags) const
{
	std::ostringstream buf;
	buf << "draw test pattern";

	if (flags & FLAG_ZERO_STRIDE)
		buf << ", zero stride";
	if (flags & FLAG_INSTANCED)
		buf << ", instanced binding point";
	if (flags & FLAG_ALIASING_BUFFERS)
		buf << ", binding points share buffer object";

	return buf.str();
}

void MultipleBindingCase::createBuffers (void)
{
	const glw::Functions&	gl					= m_context.getRenderContext().getFunctions();
	const tcu::Vec4			green				= tcu::Vec4(0.0f, 1.0f, 0.0f, 1.0f);
	const tcu::Vec4			yellow				= tcu::Vec4(1.0f, 1.0f, 0.0f, 1.0f);

	const int				vertexDataSize		= (m_spec.instanced) ? (6) : (6 * GRID_SIZE * GRID_SIZE);
	const int				offsetColorSize		= (m_spec.zeroStride) ? (2) : (m_spec.instanced) ? (2 * GRID_SIZE * GRID_SIZE) : (2 * 6 * GRID_SIZE * GRID_SIZE);
	const int				primitiveBufSize	= (m_spec.aliasingBuffers) ? (vertexDataSize + offsetColorSize) : (vertexDataSize);
	const int				colorOffsetBufSize	= (m_spec.aliasingBuffers) ? (0) : (offsetColorSize);

	std::vector<tcu::Vec4>	primitiveData		(primitiveBufSize);
	std::vector<tcu::Vec4>	colorOffsetData		(colorOffsetBufSize);
	tcu::Vec4*				colorOffsetWritePtr = DE_NULL;

	if (m_spec.aliasingBuffers)
	{
		if (m_spec.instanced)
			colorOffsetWritePtr = &primitiveData[6];
		else
			colorOffsetWritePtr = &primitiveData[GRID_SIZE*GRID_SIZE*6];
	}
	else
		colorOffsetWritePtr = &colorOffsetData[0];

	// write vertex position

	if (m_spec.instanced)
	{
		// store single basic primitive
		primitiveData[0] = tcu::Vec4(0.0f,				0.0f,				0.0f, 1.0f);
		primitiveData[1] = tcu::Vec4(0.0f,				2.0f / GRID_SIZE,	0.0f, 1.0f);
		primitiveData[2] = tcu::Vec4(2.0f / GRID_SIZE,	2.0f / GRID_SIZE,	0.0f, 1.0f);
		primitiveData[3] = tcu::Vec4(0.0f,				0.0f,				0.0f, 1.0f);
		primitiveData[4] = tcu::Vec4(2.0f / GRID_SIZE,	2.0f / GRID_SIZE,	0.0f, 1.0f);
		primitiveData[5] = tcu::Vec4(2.0f / GRID_SIZE,	0.0f,				0.0f, 1.0f);
	}
	else
	{
		// store whole grid
		for (int y = 0; y < GRID_SIZE; ++y)
		for (int x = 0; x < GRID_SIZE; ++x)
		{
			primitiveData[(y * GRID_SIZE + x) * 6 + 0] = tcu::Vec4(float(x+0) / float(GRID_SIZE) * 2.0f - 1.0f, float(y+0) / float(GRID_SIZE) * 2.0f - 1.0f, 0.0f, 1.0f);
			primitiveData[(y * GRID_SIZE + x) * 6 + 1] = tcu::Vec4(float(x+0) / float(GRID_SIZE) * 2.0f - 1.0f, float(y+1) / float(GRID_SIZE) * 2.0f - 1.0f, 0.0f, 1.0f);
			primitiveData[(y * GRID_SIZE + x) * 6 + 2] = tcu::Vec4(float(x+1) / float(GRID_SIZE) * 2.0f - 1.0f, float(y+1) / float(GRID_SIZE) * 2.0f - 1.0f, 0.0f, 1.0f);
			primitiveData[(y * GRID_SIZE + x) * 6 + 3] = tcu::Vec4(float(x+0) / float(GRID_SIZE) * 2.0f - 1.0f, float(y+0) / float(GRID_SIZE) * 2.0f - 1.0f, 0.0f, 1.0f);
			primitiveData[(y * GRID_SIZE + x) * 6 + 4] = tcu::Vec4(float(x+1) / float(GRID_SIZE) * 2.0f - 1.0f, float(y+1) / float(GRID_SIZE) * 2.0f - 1.0f, 0.0f, 1.0f);
			primitiveData[(y * GRID_SIZE + x) * 6 + 5] = tcu::Vec4(float(x+1) / float(GRID_SIZE) * 2.0f - 1.0f, float(y+0) / float(GRID_SIZE) * 2.0f - 1.0f, 0.0f, 1.0f);
		}
	}

	// store color&offset

	if (m_spec.zeroStride)
	{
		colorOffsetWritePtr[0] = green;
		colorOffsetWritePtr[1] = tcu::Vec4(0.0f);
	}
	else if (m_spec.instanced)
	{
		for (int y = 0; y < GRID_SIZE; ++y)
		for (int x = 0; x < GRID_SIZE; ++x)
		{
			const tcu::Vec4& color = ((x + y) % 2 == 0) ? (green) : (yellow);

			colorOffsetWritePtr[(y * GRID_SIZE + x) * 2 + 0] = color;
			colorOffsetWritePtr[(y * GRID_SIZE + x) * 2 + 1] = tcu::Vec4(float(x) / float(GRID_SIZE) * 2.0f - 1.0f, float(y) / float(GRID_SIZE) * 2.0f - 1.0f, 0.0f, 0.0f);
		}
	}
	else
	{
		for (int y = 0; y < GRID_SIZE; ++y)
		for (int x = 0; x < GRID_SIZE; ++x)
		for (int v = 0; v < 6; ++v)
		{
			const tcu::Vec4& color = ((x + y) % 2 == 0) ? (green) : (yellow);

			colorOffsetWritePtr[((y * GRID_SIZE + x) * 6 + v) * 2 + 0] = color;
			colorOffsetWritePtr[((y * GRID_SIZE + x) * 6 + v) * 2 + 1] = tcu::Vec4(0.0f);
		}
	}

	// upload vertex data

	gl.genBuffers(1, &m_primitiveBuf);
	gl.bindBuffer(GL_ARRAY_BUFFER, m_primitiveBuf);
	gl.bufferData(GL_ARRAY_BUFFER, (int)(primitiveData.size() * sizeof(tcu::Vec4)), primitiveData[0].getPtr(), GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "upload data");

	if (!m_spec.aliasingBuffers)
	{
		// upload color & offset data

		gl.genBuffers(1, &m_colorOffsetBuf);
		gl.bindBuffer(GL_ARRAY_BUFFER, m_colorOffsetBuf);
		gl.bufferData(GL_ARRAY_BUFFER, (int)(colorOffsetData.size() * sizeof(tcu::Vec4)), colorOffsetData[0].getPtr(), GL_STATIC_DRAW);
		GLU_EXPECT_NO_ERROR(gl.getError(), "upload colordata");
	}
}

void MultipleBindingCase::createShader (void)
{
	m_program = new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources() << glu::VertexSource(s_positionColorOffsetShader) << glu::FragmentSource(s_colorFragmentShader));
	m_testCtx.getLog() << *m_program;

	if (!m_program->isOk())
		throw tcu::TestError("could not build shader");
}

class MixedBindingCase : public BindingRenderCase
{
public:

	enum CaseType
	{
		CASE_BASIC = 0,
		CASE_INSTANCED_BINDING,
		CASE_INSTANCED_ATTRIB,

		CASE_LAST
	};

						MixedBindingCase		(Context& ctx, const char* name, const char* desc, CaseType caseType);
						~MixedBindingCase		(void);

	void				init					(void);
	void				deinit					(void);

private:
	enum
	{
		GRID_SIZE = 20
	};

	void				renderTo				(tcu::Surface& dst);
	void				createBuffers			(void);
	void				createShader			(void);

	const CaseType		m_case;
	glw::GLuint			m_posBuffer;
	glw::GLuint			m_colorOffsetBuffer;
};

MixedBindingCase::MixedBindingCase (Context& ctx, const char* name, const char* desc, CaseType caseType)
	: BindingRenderCase		(ctx, name, desc, false)
	, m_case				(caseType)
	, m_posBuffer			(0)
	, m_colorOffsetBuffer	(0)
{
	DE_ASSERT(caseType < CASE_LAST);
}

MixedBindingCase::~MixedBindingCase (void)
{
	deinit();
}

void MixedBindingCase::init (void)
{
	BindingRenderCase::init();
}

void MixedBindingCase::deinit (void)
{
	if (m_posBuffer)
	{
		m_context.getRenderContext().getFunctions().deleteBuffers(1, &m_posBuffer);
		m_posBuffer = DE_NULL;
	}

	if (m_colorOffsetBuffer)
	{
		m_context.getRenderContext().getFunctions().deleteBuffers(1, &m_colorOffsetBuffer);
		m_colorOffsetBuffer = DE_NULL;
	}

	BindingRenderCase::deinit();
}

void MixedBindingCase::renderTo (tcu::Surface& dst)
{
	glu::CallLogWrapper gl				(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	const int			positionLoc		= gl.glGetAttribLocation(m_program->getProgram(), "a_position");
	const int			colorLoc		= gl.glGetAttribLocation(m_program->getProgram(), "a_color");
	const int			offsetLoc		= gl.glGetAttribLocation(m_program->getProgram(), "a_offset");

	gl.enableLogging(true);

	gl.glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	gl.glClear(GL_COLOR_BUFFER_BIT);
	gl.glViewport(0, 0, dst.getWidth(), dst.getHeight());
	gl.glBindVertexArray(m_vao);
	GLU_EXPECT_NO_ERROR(gl.glGetError(), "set vao");

	gl.glUseProgram(m_program->getProgram());
	GLU_EXPECT_NO_ERROR(gl.glGetError(), "use program");

	switch (m_case)
	{
		case CASE_BASIC:
		{
			// bind position using vertex_attrib_binding api

			gl.glBindVertexBuffer(positionLoc, m_posBuffer, 0, (glw::GLsizei)sizeof(tcu::Vec4));
			gl.glVertexAttribBinding(positionLoc, positionLoc);
			gl.glVertexAttribFormat(positionLoc, 4, GL_FLOAT, GL_FALSE, 0);
			GLU_EXPECT_NO_ERROR(gl.glGetError(), "set binding");

			// bind color using old api

			gl.glBindBuffer(GL_ARRAY_BUFFER, m_colorOffsetBuffer);
			gl.glVertexAttribPointer(colorLoc, 4, GL_FLOAT, GL_FALSE, glw::GLsizei(2 * sizeof(tcu::Vec4)), DE_NULL);
			gl.glVertexAttribPointer(offsetLoc, 4, GL_FLOAT, GL_FALSE, glw::GLsizei(2 * sizeof(tcu::Vec4)), (deUint8*)DE_NULL + sizeof(tcu::Vec4));
			GLU_EXPECT_NO_ERROR(gl.glGetError(), "set va");

			// draw
			gl.glEnableVertexAttribArray(positionLoc);
			gl.glEnableVertexAttribArray(colorLoc);
			gl.glEnableVertexAttribArray(offsetLoc);
			gl.glDrawArrays(GL_TRIANGLES, 0, 6*GRID_SIZE*GRID_SIZE);
			break;
		}

		case CASE_INSTANCED_BINDING:
		{
			// bind position using old api
			gl.glBindBuffer(GL_ARRAY_BUFFER, m_posBuffer);
			gl.glVertexAttribPointer(positionLoc, 4, GL_FLOAT, GL_FALSE, 0, DE_NULL);
			GLU_EXPECT_NO_ERROR(gl.glGetError(), "set va");

			// bind color using vertex_attrib_binding api
			gl.glBindVertexBuffer(colorLoc, m_colorOffsetBuffer, 0, (glw::GLsizei)(2 * sizeof(tcu::Vec4)));
			gl.glVertexBindingDivisor(colorLoc, 1);

			gl.glVertexAttribBinding(colorLoc, colorLoc);
			gl.glVertexAttribBinding(offsetLoc, colorLoc);

			gl.glVertexAttribFormat(colorLoc, 4, GL_FLOAT, GL_FALSE, 0);
			gl.glVertexAttribFormat(offsetLoc, 4, GL_FLOAT, GL_FALSE, sizeof(tcu::Vec4));

			GLU_EXPECT_NO_ERROR(gl.glGetError(), "set binding");

			// draw
			gl.glEnableVertexAttribArray(positionLoc);
			gl.glEnableVertexAttribArray(colorLoc);
			gl.glEnableVertexAttribArray(offsetLoc);
			gl.glDrawArraysInstanced(GL_TRIANGLES, 0, 6, GRID_SIZE*GRID_SIZE);
			break;
		}

		case CASE_INSTANCED_ATTRIB:
		{
			// bind position using vertex_attrib_binding api
			gl.glBindVertexBuffer(positionLoc, m_posBuffer, 0, (glw::GLsizei)sizeof(tcu::Vec4));
			gl.glVertexAttribBinding(positionLoc, positionLoc);
			gl.glVertexAttribFormat(positionLoc, 4, GL_FLOAT, GL_FALSE, 0);
			GLU_EXPECT_NO_ERROR(gl.glGetError(), "set binding");

			// bind color using old api
			gl.glBindBuffer(GL_ARRAY_BUFFER, m_colorOffsetBuffer);
			gl.glVertexAttribPointer(colorLoc, 4, GL_FLOAT, GL_FALSE, glw::GLsizei(2 * sizeof(tcu::Vec4)), DE_NULL);
			gl.glVertexAttribPointer(offsetLoc, 4, GL_FLOAT, GL_FALSE, glw::GLsizei(2 * sizeof(tcu::Vec4)), (deUint8*)DE_NULL + sizeof(tcu::Vec4));
			gl.glVertexAttribDivisor(colorLoc, 1);
			gl.glVertexAttribDivisor(offsetLoc, 1);
			GLU_EXPECT_NO_ERROR(gl.glGetError(), "set va");

			// draw
			gl.glEnableVertexAttribArray(positionLoc);
			gl.glEnableVertexAttribArray(colorLoc);
			gl.glEnableVertexAttribArray(offsetLoc);
			gl.glDrawArraysInstanced(GL_TRIANGLES, 0, 6, GRID_SIZE*GRID_SIZE);
			break;
		}

		default:
			DE_ASSERT(DE_FALSE);
	}

	gl.glFinish();
	gl.glBindVertexArray(0);
	gl.glUseProgram(0);
	GLU_EXPECT_NO_ERROR(gl.glGetError(), "clean");

	glu::readPixels(m_context.getRenderContext(), 0, 0, dst.getAccess());
}

void MixedBindingCase::createBuffers (void)
{
	const glw::Functions&	gl								= m_context.getRenderContext().getFunctions();
	const tcu::Vec4			green							= tcu::Vec4(0.0f, 1.0f, 0.0f, 1.0f);
	const tcu::Vec4			yellow							= tcu::Vec4(1.0f, 1.0f, 0.0f, 1.0f);

	// draw grid. In instanced mode, each cell is an instance
	const bool				instanced						= (m_case == CASE_INSTANCED_BINDING) || (m_case == CASE_INSTANCED_ATTRIB);
	const int				numCells						= GRID_SIZE*GRID_SIZE;
	const int				numPositionCells				= (instanced) ? (1) : (numCells);
	const int				numPositionElements				= 6 * numPositionCells;
	const int				numInstanceElementsPerCell		= (instanced) ? (1) : (6);
	const int				numColorOffsetElements			= numInstanceElementsPerCell * numCells;

	std::vector<tcu::Vec4>	positionData					(numPositionElements);
	std::vector<tcu::Vec4>	colorOffsetData					(2 * numColorOffsetElements);

	// positions

	for (int primNdx = 0; primNdx < numPositionCells; ++primNdx)
	{
		positionData[primNdx*6 + 0] =  tcu::Vec4(0.0f,				0.0f,				0.0f, 1.0f);
		positionData[primNdx*6 + 1] =  tcu::Vec4(0.0f,				2.0f / GRID_SIZE,	0.0f, 1.0f);
		positionData[primNdx*6 + 2] =  tcu::Vec4(2.0f / GRID_SIZE,	2.0f / GRID_SIZE,	0.0f, 1.0f);
		positionData[primNdx*6 + 3] =  tcu::Vec4(0.0f,				0.0f,				0.0f, 1.0f);
		positionData[primNdx*6 + 4] =  tcu::Vec4(2.0f / GRID_SIZE,	2.0f / GRID_SIZE,	0.0f, 1.0f);
		positionData[primNdx*6 + 5] =  tcu::Vec4(2.0f / GRID_SIZE,	0.0f,				0.0f, 1.0f);
	}

	// color & offset

	for (int y = 0; y < GRID_SIZE; ++y)
	for (int x = 0; x < GRID_SIZE; ++x)
	{
		for (int v = 0; v < numInstanceElementsPerCell; ++v)
		{
			const tcu::Vec4& color = ((x + y) % 2 == 0) ? (green) : (yellow);

			colorOffsetData[((y * GRID_SIZE + x) * numInstanceElementsPerCell + v) * 2 + 0] = color;
			colorOffsetData[((y * GRID_SIZE + x) * numInstanceElementsPerCell + v) * 2 + 1] = tcu::Vec4(float(x) / float(GRID_SIZE) * 2.0f - 1.0f, float(y) / float(GRID_SIZE) * 2.0f - 1.0f, 0.0f, 0.0f);
		}
	}

	// upload vertex data

	gl.genBuffers(1, &m_posBuffer);
	gl.bindBuffer(GL_ARRAY_BUFFER, m_posBuffer);
	gl.bufferData(GL_ARRAY_BUFFER, (int)(positionData.size() * sizeof(tcu::Vec4)), positionData[0].getPtr(), GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "upload position data");

	gl.genBuffers(1, &m_colorOffsetBuffer);
	gl.bindBuffer(GL_ARRAY_BUFFER, m_colorOffsetBuffer);
	gl.bufferData(GL_ARRAY_BUFFER, (int)(colorOffsetData.size() * sizeof(tcu::Vec4)), colorOffsetData[0].getPtr(), GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "upload position data");
}

void MixedBindingCase::createShader (void)
{
	m_program = new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources() << glu::VertexSource(s_positionColorOffsetShader) << glu::FragmentSource(s_colorFragmentShader));
	m_testCtx.getLog() << *m_program;

	if (!m_program->isOk())
		throw tcu::TestError("could not build shader");
}

class MixedApiCase : public BindingRenderCase
{
public:

	enum CaseType
	{
		CASE_CHANGE_BUFFER = 0,
		CASE_CHANGE_BUFFER_OFFSET,
		CASE_CHANGE_BUFFER_STRIDE,
		CASE_CHANGE_BINDING_POINT,

		CASE_LAST
	};

						MixedApiCase			(Context& ctx, const char* name, const char* desc, CaseType caseType);
						~MixedApiCase			(void);

	void				init					(void);
	void				deinit					(void);

private:
	enum
	{
		GRID_SIZE = 20
	};

	void				renderTo				(tcu::Surface& dst);
	void				createBuffers			(void);
	void				createShader			(void);

	const CaseType		m_case;
	glw::GLuint			m_buffer;
};


MixedApiCase::MixedApiCase (Context& ctx, const char* name, const char* desc, CaseType caseType)
	: BindingRenderCase		(ctx, name, desc, false)
	, m_case				(caseType)
	, m_buffer				(0)
{
	DE_ASSERT(caseType < CASE_LAST);
}

MixedApiCase::~MixedApiCase (void)
{
	deinit();
}

void MixedApiCase::init (void)
{
	BindingRenderCase::init();
}

void MixedApiCase::deinit (void)
{
	if (m_buffer)
	{
		m_context.getRenderContext().getFunctions().deleteBuffers(1, &m_buffer);
		m_buffer = DE_NULL;
	}

	BindingRenderCase::deinit();
}

void MixedApiCase::renderTo (tcu::Surface& dst)
{
	glu::CallLogWrapper		gl				(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	const int				positionLoc		= gl.glGetAttribLocation(m_program->getProgram(), "a_position");
	const int				colorLoc		= gl.glGetAttribLocation(m_program->getProgram(), "a_color");
	glu::Buffer				dummyBuffer		(m_context.getRenderContext());

	gl.enableLogging(true);

	gl.glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	gl.glClear(GL_COLOR_BUFFER_BIT);
	gl.glViewport(0, 0, dst.getWidth(), dst.getHeight());
	gl.glBindVertexArray(m_vao);
	GLU_EXPECT_NO_ERROR(gl.glGetError(), "set vao");

	gl.glUseProgram(m_program->getProgram());
	GLU_EXPECT_NO_ERROR(gl.glGetError(), "use program");

	switch (m_case)
	{
		case CASE_CHANGE_BUFFER:
		{
			// bind data using old api

			gl.glBindBuffer(GL_ARRAY_BUFFER, *dummyBuffer);
			gl.glVertexAttribPointer(positionLoc, 4, GL_FLOAT, GL_FALSE, (glw::GLsizei)(2 * sizeof(tcu::Vec4)), (const deUint8*)DE_NULL);
			gl.glVertexAttribPointer(colorLoc,    4, GL_FLOAT, GL_FALSE, (glw::GLsizei)(2 * sizeof(tcu::Vec4)), (const deUint8*)DE_NULL + sizeof(tcu::Vec4));

			// change buffer with vertex_attrib_binding

			gl.glBindVertexBuffer(positionLoc, m_buffer, 0,                 (glw::GLsizei)(2 * sizeof(tcu::Vec4)));
			gl.glBindVertexBuffer(colorLoc,    m_buffer, sizeof(tcu::Vec4), (glw::GLsizei)(2 * sizeof(tcu::Vec4)));

			GLU_EXPECT_NO_ERROR(gl.glGetError(), "");
			break;
		}

		case CASE_CHANGE_BUFFER_OFFSET:
		{
			// bind data using old api

			gl.glBindBuffer(GL_ARRAY_BUFFER, m_buffer);
			gl.glVertexAttribPointer(positionLoc, 4, GL_FLOAT, GL_FALSE, (glw::GLsizei)(2 * sizeof(tcu::Vec4)), (const deUint8*)DE_NULL);
			gl.glVertexAttribPointer(colorLoc,    4, GL_FLOAT, GL_FALSE, (glw::GLsizei)(2 * sizeof(tcu::Vec4)), (const deUint8*)DE_NULL);

			// change buffer offset with vertex_attrib_binding

			gl.glBindVertexBuffer(positionLoc, m_buffer, 0,                 (glw::GLsizei)(2 * sizeof(tcu::Vec4)));
			gl.glBindVertexBuffer(colorLoc,    m_buffer, sizeof(tcu::Vec4), (glw::GLsizei)(2 * sizeof(tcu::Vec4)));

			GLU_EXPECT_NO_ERROR(gl.glGetError(), "");
			break;
		}

		case CASE_CHANGE_BUFFER_STRIDE:
		{
			// bind data using old api

			gl.glBindBuffer(GL_ARRAY_BUFFER, m_buffer);
			gl.glVertexAttribPointer(positionLoc, 4, GL_FLOAT, GL_FALSE, 8, (const deUint8*)DE_NULL);
			gl.glVertexAttribPointer(colorLoc,    4, GL_FLOAT, GL_FALSE, 4, (const deUint8*)DE_NULL);

			// change buffer stride with vertex_attrib_binding

			gl.glBindVertexBuffer(positionLoc, m_buffer, 0,                 (glw::GLsizei)(2 * sizeof(tcu::Vec4)));
			gl.glBindVertexBuffer(colorLoc,    m_buffer, sizeof(tcu::Vec4), (glw::GLsizei)(2 * sizeof(tcu::Vec4)));

			GLU_EXPECT_NO_ERROR(gl.glGetError(), "");
			break;
		}

		case CASE_CHANGE_BINDING_POINT:
		{
			const int maxUsedLocation	= de::max(positionLoc, colorLoc);
			const int bindingPoint1		= maxUsedLocation + 1;
			const int bindingPoint2		= maxUsedLocation + 2;

			// bind data using old api

			gl.glBindBuffer(GL_ARRAY_BUFFER, m_buffer);
			gl.glVertexAttribPointer(bindingPoint1, 4, GL_FLOAT, GL_FALSE, (glw::GLsizei)(2 * sizeof(tcu::Vec4)), (const deUint8*)DE_NULL);
			gl.glVertexAttribPointer(bindingPoint2, 4, GL_FLOAT, GL_FALSE, (glw::GLsizei)(2 * sizeof(tcu::Vec4)), (const deUint8*)DE_NULL + sizeof(tcu::Vec4));

			// change buffer binding point with vertex_attrib_binding

			gl.glVertexAttribFormat(positionLoc, 4, GL_FLOAT, GL_FALSE, 0);
			gl.glVertexAttribFormat(colorLoc, 4, GL_FLOAT, GL_FALSE, 0);

			gl.glVertexAttribBinding(positionLoc, bindingPoint1);
			gl.glVertexAttribBinding(colorLoc, bindingPoint2);

			GLU_EXPECT_NO_ERROR(gl.glGetError(), "");
			break;
		}

		default:
			DE_ASSERT(DE_FALSE);
	}

	// draw
	gl.glEnableVertexAttribArray(positionLoc);
	gl.glEnableVertexAttribArray(colorLoc);
	gl.glDrawArrays(GL_TRIANGLES, 0, 6*GRID_SIZE*GRID_SIZE);

	gl.glFinish();
	gl.glBindVertexArray(0);
	gl.glUseProgram(0);
	GLU_EXPECT_NO_ERROR(gl.glGetError(), "clean");

	glu::readPixels(m_context.getRenderContext(), 0, 0, dst.getAccess());
}

void MixedApiCase::createBuffers (void)
{
	const tcu::Vec4			green							= tcu::Vec4(0.0f, 1.0f, 0.0f, 1.0f);
	const tcu::Vec4			yellow							= tcu::Vec4(1.0f, 1.0f, 0.0f, 1.0f);

	const glw::Functions&	gl								= m_context.getRenderContext().getFunctions();
	std::vector<tcu::Vec4>	vertexData						(12 * GRID_SIZE * GRID_SIZE);

	for (int y = 0; y < GRID_SIZE; ++y)
	for (int x = 0; x < GRID_SIZE; ++x)
	{
		const tcu::Vec4& color = ((x + y) % 2 == 0) ? (green) : (yellow);

		vertexData[(y * GRID_SIZE + x) * 12 +  0] = tcu::Vec4(float(x+0) / float(GRID_SIZE) * 2.0f - 1.0f, float(y+0) / float(GRID_SIZE) * 2.0f - 1.0f, 0.0f, 1.0f);
		vertexData[(y * GRID_SIZE + x) * 12 +  1] = color;
		vertexData[(y * GRID_SIZE + x) * 12 +  2] = tcu::Vec4(float(x+0) / float(GRID_SIZE) * 2.0f - 1.0f, float(y+1) / float(GRID_SIZE) * 2.0f - 1.0f, 0.0f, 1.0f);
		vertexData[(y * GRID_SIZE + x) * 12 +  3] = color;
		vertexData[(y * GRID_SIZE + x) * 12 +  4] = tcu::Vec4(float(x+1) / float(GRID_SIZE) * 2.0f - 1.0f, float(y+1) / float(GRID_SIZE) * 2.0f - 1.0f, 0.0f, 1.0f);
		vertexData[(y * GRID_SIZE + x) * 12 +  5] = color;
		vertexData[(y * GRID_SIZE + x) * 12 +  6] = tcu::Vec4(float(x+0) / float(GRID_SIZE) * 2.0f - 1.0f, float(y+0) / float(GRID_SIZE) * 2.0f - 1.0f, 0.0f, 1.0f);
		vertexData[(y * GRID_SIZE + x) * 12 +  7] = color;
		vertexData[(y * GRID_SIZE + x) * 12 +  8] = tcu::Vec4(float(x+1) / float(GRID_SIZE) * 2.0f - 1.0f, float(y+1) / float(GRID_SIZE) * 2.0f - 1.0f, 0.0f, 1.0f);
		vertexData[(y * GRID_SIZE + x) * 12 +  9] = color;
		vertexData[(y * GRID_SIZE + x) * 12 + 10] = tcu::Vec4(float(x+1) / float(GRID_SIZE) * 2.0f - 1.0f, float(y+0) / float(GRID_SIZE) * 2.0f - 1.0f, 0.0f, 1.0f);
		vertexData[(y * GRID_SIZE + x) * 12 + 11] = color;
	}

	// upload vertex data

	gl.genBuffers(1, &m_buffer);
	gl.bindBuffer(GL_ARRAY_BUFFER, m_buffer);
	gl.bufferData(GL_ARRAY_BUFFER, (int)(vertexData.size() * sizeof(tcu::Vec4)), vertexData[0].getPtr(), GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "upload data");
}

void MixedApiCase::createShader (void)
{
	m_program = new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources() << glu::VertexSource(s_positionColorShader) << glu::FragmentSource(s_colorFragmentShader));
	m_testCtx.getLog() << *m_program;

	if (!m_program->isOk())
		throw tcu::TestError("could not build shader");
}

class DefaultVAOCase : public TestCase
{
public:
	enum CaseType
	{
		CASE_BIND_VERTEX_BUFFER,
		CASE_VERTEX_ATTRIB_FORMAT,
		CASE_VERTEX_ATTRIB_I_FORMAT,
		CASE_VERTEX_ATTRIB_BINDING,
		CASE_VERTEX_BINDING_DIVISOR,

		CASE_LAST
	};

					DefaultVAOCase		(Context& ctx, const char* name, const char* desc, CaseType caseType);
					~DefaultVAOCase		(void);

	IterateResult	iterate				(void);

private:
	const CaseType	m_caseType;
};

DefaultVAOCase::DefaultVAOCase (Context& ctx, const char* name, const char* desc, CaseType caseType)
	: TestCase		(ctx, name, desc)
	, m_caseType	(caseType)
{
	DE_ASSERT(caseType < CASE_LAST);
}

DefaultVAOCase::~DefaultVAOCase (void)
{
}

DefaultVAOCase::IterateResult DefaultVAOCase::iterate (void)
{
	glw::GLenum			error	= 0;
	glu::CallLogWrapper gl		(m_context.getRenderContext().getFunctions(), m_context.getTestContext().getLog());

	gl.enableLogging(true);

	switch (m_caseType)
	{
		case CASE_BIND_VERTEX_BUFFER:
		{
			glu::Buffer buffer(m_context.getRenderContext());
			gl.glBindVertexBuffer(0, *buffer, 0, 0);
			break;
		}

		case CASE_VERTEX_ATTRIB_FORMAT:
			gl.glVertexAttribFormat(0, 4, GL_FLOAT, GL_FALSE, 0);
			break;

		case CASE_VERTEX_ATTRIB_I_FORMAT:
			gl.glVertexAttribIFormat(0, 4, GL_INT, 0);
			break;

		case CASE_VERTEX_ATTRIB_BINDING:
			gl.glVertexAttribBinding(0, 0);
			break;

		case CASE_VERTEX_BINDING_DIVISOR:
			gl.glVertexBindingDivisor(0, 1);
			break;

		default:
			DE_ASSERT(false);
	}

	error = gl.glGetError();

	if (error != GL_INVALID_OPERATION)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "ERROR! Expected GL_INVALID_OPERATION, got " << glu::getErrorStr(error) << tcu::TestLog::EndMessage;
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid error");
	}
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

class BindToCreateCase : public TestCase
{
public:
					BindToCreateCase	(Context& ctx, const char* name, const char* desc);
					~BindToCreateCase	(void);

	IterateResult	iterate				(void);
};

BindToCreateCase::BindToCreateCase (Context& ctx, const char* name, const char* desc)
	: TestCase(ctx, name, desc)
{
}

BindToCreateCase::~BindToCreateCase (void)
{
}

BindToCreateCase::IterateResult BindToCreateCase::iterate (void)
{
	glw::GLuint			buffer	= 0;
	glw::GLenum			error;
	glu::CallLogWrapper gl		(m_context.getRenderContext().getFunctions(), m_context.getTestContext().getLog());
	glu::VertexArray	vao		(m_context.getRenderContext());

	gl.enableLogging(true);

	gl.glGenBuffers(1, &buffer);
	gl.glDeleteBuffers(1, &buffer);
	GLU_EXPECT_NO_ERROR(gl.glGetError(), "");

	gl.glBindVertexArray(*vao);
	gl.glBindVertexBuffer(0, buffer, 0, 0);

	error = gl.glGetError();

	if (error != GL_INVALID_OPERATION)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "ERROR! Expected GL_INVALID_OPERATION, got " << glu::getErrorStr(error) << tcu::TestLog::EndMessage;
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid error");
	}
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

class NegativeApiCase : public TestCase
{
public:
	enum CaseType
	{
		CASE_LARGE_OFFSET,
		CASE_LARGE_STRIDE,
		CASE_NEGATIVE_STRIDE,
		CASE_NEGATIVE_OFFSET,
		CASE_INVALID_ATTR,
		CASE_INVALID_BINDING,

		CASE_LAST
	};
					NegativeApiCase		(Context& ctx, const char* name, const char* desc, CaseType caseType);
					~NegativeApiCase	(void);

	IterateResult	iterate				(void);

private:
	const CaseType	m_caseType;
};

NegativeApiCase::NegativeApiCase (Context& ctx, const char* name, const char* desc, CaseType caseType)
	: TestCase		(ctx, name, desc)
	, m_caseType	(caseType)
{
}

NegativeApiCase::~NegativeApiCase (void)
{
}

NegativeApiCase::IterateResult NegativeApiCase::iterate (void)
{
	glw::GLenum			error;
	glu::CallLogWrapper gl		(m_context.getRenderContext().getFunctions(), m_context.getTestContext().getLog());
	glu::VertexArray	vao		(m_context.getRenderContext());

	gl.enableLogging(true);
	gl.glBindVertexArray(*vao);

	switch (m_caseType)
	{
		case CASE_LARGE_OFFSET:
		{
			glw::GLint	maxOffset	= -1;
			glw::GLint	largeOffset;

			gl.glGetIntegerv(GL_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET, &maxOffset);
			GLU_EXPECT_NO_ERROR(gl.glGetError(), "");

			largeOffset = maxOffset + 1;

			// skip if maximum unsigned or signed values
			if (maxOffset == -1 || maxOffset == 0x7FFFFFFF)
				throw tcu::NotSupportedError("Implementation supports all offsets");

			gl.glVertexAttribFormat(0, 4, GL_FLOAT, GL_FALSE, largeOffset);
			break;
		}

		case CASE_LARGE_STRIDE:
		{
			glu::Buffer buffer		(m_context.getRenderContext());
			glw::GLint	maxStride	= -1;
			glw::GLint	largeStride;

			gl.glGetIntegerv(GL_MAX_VERTEX_ATTRIB_STRIDE, &maxStride);
			GLU_EXPECT_NO_ERROR(gl.glGetError(), "");

			largeStride = maxStride + 1;

			// skip if maximum unsigned or signed values
			if (maxStride == -1 || maxStride == 0x7FFFFFFF)
				throw tcu::NotSupportedError("Implementation supports all strides");

			gl.glBindVertexBuffer(0, *buffer, 0, largeStride);
			break;
		}

		case CASE_NEGATIVE_STRIDE:
		{
			glu::Buffer buffer(m_context.getRenderContext());
			gl.glBindVertexBuffer(0, *buffer, 0, -20);
			break;
		}

		case CASE_NEGATIVE_OFFSET:
		{
			glu::Buffer buffer(m_context.getRenderContext());
			gl.glBindVertexBuffer(0, *buffer, -20, 0);
			break;
		}

		case CASE_INVALID_ATTR:
		{
			glw::GLint maxIndex = -1;
			glw::GLint largeIndex;

			gl.glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxIndex);
			GLU_EXPECT_NO_ERROR(gl.glGetError(), "");

			largeIndex = maxIndex + 1;

			// skip if maximum unsigned or signed values
			if (maxIndex == -1 || maxIndex == 0x7FFFFFFF)
				throw tcu::NotSupportedError("Implementation supports any attribute index");

			gl.glVertexAttribBinding(largeIndex, 0);
			break;
		}

		case CASE_INVALID_BINDING:
		{
			glw::GLint maxBindings = -1;
			glw::GLint largeBinding;

			gl.glGetIntegerv(GL_MAX_VERTEX_ATTRIB_BINDINGS, &maxBindings);
			GLU_EXPECT_NO_ERROR(gl.glGetError(), "");

			largeBinding = maxBindings + 1;

			// skip if maximum unsigned or signed values
			if (maxBindings == -1 || maxBindings == 0x7FFFFFFF)
				throw tcu::NotSupportedError("Implementation supports any binding");

			gl.glVertexAttribBinding(0, largeBinding);
			break;
		}

		default:
			DE_ASSERT(false);
	}

	error = gl.glGetError();

	if (error != GL_INVALID_VALUE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "ERROR! Expected GL_INVALID_VALUE, got " << glu::getErrorStr(error) << tcu::TestLog::EndMessage;
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid error");
	}
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

} // anonymous

VertexAttributeBindingTests::VertexAttributeBindingTests (Context& context)
	: TestCaseGroup(context, "vertex_attribute_binding", "Test vertex attribute binding")
{
}

VertexAttributeBindingTests::~VertexAttributeBindingTests (void)
{
}

void VertexAttributeBindingTests::init (void)
{
	tcu::TestCaseGroup* const usageGroup	= new tcu::TestCaseGroup(m_testCtx, "usage", "Test using binding points");
	tcu::TestCaseGroup* const negativeGroup	= new tcu::TestCaseGroup(m_testCtx, "negative", "Negative test");

	addChild(usageGroup);
	addChild(negativeGroup);

	// .usage
	{
		tcu::TestCaseGroup* const singleGroup	= new tcu::TestCaseGroup(m_testCtx, "single_binding", "Test using single binding point");
		tcu::TestCaseGroup* const multipleGroup	= new tcu::TestCaseGroup(m_testCtx, "multiple_bindings", "Test using multiple binding points");
		tcu::TestCaseGroup* const mixedGroup	= new tcu::TestCaseGroup(m_testCtx, "mixed_usage", "Test using binding point and non binding point api variants");

		usageGroup->addChild(singleGroup);
		usageGroup->addChild(multipleGroup);
		usageGroup->addChild(mixedGroup);

		// single binding

		singleGroup->addChild(new SingleBindingCase(m_context, "elements_1",																					  0));
		singleGroup->addChild(new SingleBindingCase(m_context, "elements_2",																					  SingleBindingCase::FLAG_ATTRIBS_MULTIPLE_ELEMS));
		singleGroup->addChild(new SingleBindingCase(m_context, "elements_2_share_elements",																		  SingleBindingCase::FLAG_ATTRIBS_SHARED_ELEMS));
		singleGroup->addChild(new SingleBindingCase(m_context, "offset_elements_1",								SingleBindingCase::FLAG_BUF_ALIGNED_OFFSET		| 0));
		singleGroup->addChild(new SingleBindingCase(m_context, "offset_elements_2",								SingleBindingCase::FLAG_BUF_ALIGNED_OFFSET		| SingleBindingCase::FLAG_ATTRIBS_MULTIPLE_ELEMS));
		singleGroup->addChild(new SingleBindingCase(m_context, "offset_elements_2_share_elements",				SingleBindingCase::FLAG_BUF_ALIGNED_OFFSET		| SingleBindingCase::FLAG_ATTRIBS_SHARED_ELEMS));
		singleGroup->addChild(new SingleBindingCase(m_context, "unaligned_offset_elements_1_aligned_elements",	SingleBindingCase::FLAG_BUF_UNALIGNED_OFFSET	| SingleBindingCase::FLAG_ATTRIB_ALIGNED));			// !< total offset is aligned

		// multiple bindings

		multipleGroup->addChild(new MultipleBindingCase(m_context, "basic",									0));
		multipleGroup->addChild(new MultipleBindingCase(m_context, "zero_stride",							MultipleBindingCase::FLAG_ZERO_STRIDE));
		multipleGroup->addChild(new MultipleBindingCase(m_context, "instanced",								MultipleBindingCase::FLAG_INSTANCED));
		multipleGroup->addChild(new MultipleBindingCase(m_context, "aliasing_buffer_zero_stride",			MultipleBindingCase::FLAG_ALIASING_BUFFERS	| MultipleBindingCase::FLAG_ZERO_STRIDE));
		multipleGroup->addChild(new MultipleBindingCase(m_context, "aliasing_buffer_instanced",				MultipleBindingCase::FLAG_ALIASING_BUFFERS	| MultipleBindingCase::FLAG_INSTANCED));

		// mixed cases
		mixedGroup->addChild(new MixedBindingCase(m_context,		"mixed_attribs_basic",					"Use different api for different attributes",			MixedBindingCase::CASE_BASIC));
		mixedGroup->addChild(new MixedBindingCase(m_context,		"mixed_attribs_instanced_binding",		"Use different api for different attributes",			MixedBindingCase::CASE_INSTANCED_BINDING));
		mixedGroup->addChild(new MixedBindingCase(m_context,		"mixed_attribs_instanced_attrib",		"Use different api for different attributes",			MixedBindingCase::CASE_INSTANCED_ATTRIB));

		mixedGroup->addChild(new MixedApiCase(m_context,			"mixed_api_change_buffer",				"change buffer with vertex_attrib_binding api",			MixedApiCase::CASE_CHANGE_BUFFER));
		mixedGroup->addChild(new MixedApiCase(m_context,			"mixed_api_change_buffer_offset",		"change buffer offset with vertex_attrib_binding api",	MixedApiCase::CASE_CHANGE_BUFFER_OFFSET));
		mixedGroup->addChild(new MixedApiCase(m_context,			"mixed_api_change_buffer_stride",		"change buffer stride with vertex_attrib_binding api",	MixedApiCase::CASE_CHANGE_BUFFER_STRIDE));
		mixedGroup->addChild(new MixedApiCase(m_context,			"mixed_api_change_binding_point",		"change binding point with vertex_attrib_binding api",	MixedApiCase::CASE_CHANGE_BINDING_POINT));
	}

	// negative
	{
		negativeGroup->addChild(new DefaultVAOCase(m_context,	"default_vao_bind_vertex_buffer",			"use with default vao",	DefaultVAOCase::CASE_BIND_VERTEX_BUFFER));
		negativeGroup->addChild(new DefaultVAOCase(m_context,	"default_vao_vertex_attrib_format",			"use with default vao",	DefaultVAOCase::CASE_VERTEX_ATTRIB_FORMAT));
		negativeGroup->addChild(new DefaultVAOCase(m_context,	"default_vao_vertex_attrib_i_format",		"use with default vao",	DefaultVAOCase::CASE_VERTEX_ATTRIB_I_FORMAT));
		negativeGroup->addChild(new DefaultVAOCase(m_context,	"default_vao_vertex_attrib_binding",		"use with default vao",	DefaultVAOCase::CASE_VERTEX_ATTRIB_BINDING));
		negativeGroup->addChild(new DefaultVAOCase(m_context,	"default_vao_vertex_binding_divisor",		"use with default vao",	DefaultVAOCase::CASE_VERTEX_BINDING_DIVISOR));

		negativeGroup->addChild(new BindToCreateCase(m_context,	"bind_create_new_buffer",					"bind not existing buffer"));

		negativeGroup->addChild(new NegativeApiCase(m_context, "vertex_attrib_format_large_offset",			"large relative offset",	NegativeApiCase::CASE_LARGE_OFFSET));
		negativeGroup->addChild(new NegativeApiCase(m_context, "bind_vertex_buffer_large_stride",			"large stride",				NegativeApiCase::CASE_LARGE_STRIDE));
		negativeGroup->addChild(new NegativeApiCase(m_context, "bind_vertex_buffer_negative_stride",		"negative stride",			NegativeApiCase::CASE_NEGATIVE_STRIDE));
		negativeGroup->addChild(new NegativeApiCase(m_context, "bind_vertex_buffer_negative_offset",		"negative offset",			NegativeApiCase::CASE_NEGATIVE_OFFSET));
		negativeGroup->addChild(new NegativeApiCase(m_context, "vertex_attrib_binding_invalid_attr",		"bind invalid attr",		NegativeApiCase::CASE_INVALID_ATTR));
		negativeGroup->addChild(new NegativeApiCase(m_context, "vertex_attrib_binding_invalid_binding",		"bind invalid binding",		NegativeApiCase::CASE_INVALID_BINDING));
	}
}

} // Functional
} // gles31
} // deqp
