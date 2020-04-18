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
 * \brief Vertex attribute binding stress tests.
 *//*--------------------------------------------------------------------*/

#include "es31sVertexAttributeBindingTests.hpp"
#include "tcuVector.hpp"
#include "tcuTestLog.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuSurface.hpp"
#include "gluCallLogWrapper.hpp"
#include "gluObjectWrapper.hpp"
#include "gluPixelTransfer.hpp"
#include "gluRenderContext.hpp"
#include "gluShaderProgram.hpp"
#include "gluStrUtil.hpp"
#include "glwFunctions.hpp"
#include "glwEnums.hpp"
#include "deStringUtil.hpp"

namespace deqp
{
namespace gles31
{
namespace Stress
{
namespace
{

static const char* const s_vertexSource =				"#version 310 es\n"
														"in highp vec4 a_position;\n"
														"void main (void)\n"
														"{\n"
														"	gl_Position = a_position;\n"
														"}\n";

static const char* const s_fragmentSource =				"#version 310 es\n"
														"layout(location = 0) out mediump vec4 fragColor;\n"
														"void main (void)\n"
														"{\n"
														"	fragColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
														"}\n";

static const char* const s_colorFragmentShader =		"#version 310 es\n"
														"in mediump vec4 v_color;\n"
														"layout(location = 0) out mediump vec4 fragColor;\n"
														"void main (void)\n"
														"{\n"
														"	fragColor = v_color;\n"
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

	DE_ASSERT(isDataUnaligned(flags));
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

class BindVertexBufferCase : public TestCase
{
public:
						BindVertexBufferCase	(Context& ctx, const char* name, const char* desc, int offset, int drawCount);
						~BindVertexBufferCase	(void);

	void				init					(void);
	void				deinit					(void);
	IterateResult		iterate					(void);

private:
	const int			m_offset;
	const int			m_drawCount;
	deUint32			m_buffer;
	glu::ShaderProgram*	m_program;
};

BindVertexBufferCase::BindVertexBufferCase (Context& ctx, const char* name, const char* desc, int offset, int drawCount)
	: TestCase		(ctx, name, desc)
	, m_offset		(offset)
	, m_drawCount	(drawCount)
	, m_buffer		(0)
	, m_program		(DE_NULL)
{
}

BindVertexBufferCase::~BindVertexBufferCase (void)
{
	deinit();
}

void BindVertexBufferCase::init (void)
{
	const glw::Functions&	gl		= m_context.getRenderContext().getFunctions();
	std::vector<tcu::Vec4>	data	(m_drawCount); // !< some junk data to make sure buffer is really allocated

	gl.genBuffers(1, &m_buffer);
	gl.bindBuffer(GL_ARRAY_BUFFER, m_buffer);
	gl.bufferData(GL_ARRAY_BUFFER, int(m_drawCount * sizeof(tcu::Vec4)), &data[0], GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "buffer gen");

	m_program = new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources() << glu::VertexSource(s_vertexSource) << glu::FragmentSource(s_fragmentSource));
	if (!m_program->isOk())
	{
		m_testCtx.getLog() << *m_program;
		throw tcu::TestError("could not build program");
	}
}

void BindVertexBufferCase::deinit (void)
{
	if (m_buffer)
	{
		m_context.getRenderContext().getFunctions().deleteBuffers(1, &m_buffer);
		m_buffer = 0;
	}

	delete m_program;
	m_program = DE_NULL;
}

BindVertexBufferCase::IterateResult BindVertexBufferCase::iterate (void)
{
	glu::CallLogWrapper		gl			(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	const deInt32			positionLoc = gl.glGetAttribLocation(m_program->getProgram(), "a_position");
	tcu::Surface			dst			(m_context.getRenderTarget().getWidth(), m_context.getRenderTarget().getHeight());
	glu::VertexArray		vao			(m_context.getRenderContext());

	gl.enableLogging(true);

	gl.glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	gl.glClear(GL_COLOR_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.glGetError(), "setup");

	gl.glUseProgram(m_program->getProgram());
	GLU_EXPECT_NO_ERROR(gl.glGetError(), "use program");

	gl.glBindVertexArray(*vao);
	gl.glEnableVertexAttribArray(positionLoc);
	gl.glVertexAttribFormat(positionLoc, 4, GL_FLOAT, GL_FALSE, 0);
	gl.glVertexAttribBinding(positionLoc, 0);
	gl.glBindVertexBuffer(0, m_buffer, m_offset, int(sizeof(tcu::Vec4)));
	GLU_EXPECT_NO_ERROR(gl.glGetError(), "set buffer");

	gl.glDrawArrays(GL_POINTS, 0, m_drawCount);

	// allow errors after attempted out-of-bounds memory access
	{
		const deUint32 error = gl.glGetError();

		if (error != GL_NO_ERROR)
			m_testCtx.getLog() << tcu::TestLog::Message << "Got error: " << glu::getErrorStr(error) << ", ignoring..." << tcu::TestLog::EndMessage;
	}

	// read pixels to wait for rendering
	gl.glFinish();
	glu::readPixels(m_context.getRenderContext(), 0, 0, dst.getAccess());

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

} // anonymous

VertexAttributeBindingTests::VertexAttributeBindingTests (Context& context)
	: TestCaseGroup(context, "vertex_attribute_binding", "Test vertex attribute binding stress tests")
{
}

VertexAttributeBindingTests::~VertexAttributeBindingTests (void)
{
}

void VertexAttributeBindingTests::init (void)
{
	tcu::TestCaseGroup* const unalignedGroup	= new tcu::TestCaseGroup(m_testCtx, "unaligned",		"Unaligned access");
	tcu::TestCaseGroup* const bufferRangeGroup	= new tcu::TestCaseGroup(m_testCtx, "buffer_bounds",	"Source data over buffer bounds");

	addChild(unalignedGroup);
	addChild(bufferRangeGroup);

	// .unaligned
	{
		unalignedGroup->addChild(new SingleBindingCase(m_context, "elements_1_unaligned",																		  SingleBindingCase::FLAG_ATTRIB_UNALIGNED));
		unalignedGroup->addChild(new SingleBindingCase(m_context, "offset_elements_1_unaligned",				SingleBindingCase::FLAG_BUF_ALIGNED_OFFSET		| SingleBindingCase::FLAG_ATTRIB_UNALIGNED));

		unalignedGroup->addChild(new SingleBindingCase(m_context, "unaligned_offset_elements_1",				SingleBindingCase::FLAG_BUF_UNALIGNED_OFFSET	| 0));
		unalignedGroup->addChild(new SingleBindingCase(m_context, "unaligned_offset_elements_1_unaligned",		SingleBindingCase::FLAG_BUF_UNALIGNED_OFFSET	| SingleBindingCase::FLAG_ATTRIB_UNALIGNED));
		unalignedGroup->addChild(new SingleBindingCase(m_context, "unaligned_offset_elements_2",				SingleBindingCase::FLAG_BUF_UNALIGNED_OFFSET	| SingleBindingCase::FLAG_ATTRIBS_MULTIPLE_ELEMS));
		unalignedGroup->addChild(new SingleBindingCase(m_context, "unaligned_offset_elements_2_share_elements",	SingleBindingCase::FLAG_BUF_UNALIGNED_OFFSET	| SingleBindingCase::FLAG_ATTRIBS_SHARED_ELEMS));

		unalignedGroup->addChild(new SingleBindingCase(m_context, "unaligned_stride_elements_1",				SingleBindingCase::FLAG_BUF_UNALIGNED_STRIDE	| 0));
		unalignedGroup->addChild(new SingleBindingCase(m_context, "unaligned_stride_elements_2",				SingleBindingCase::FLAG_BUF_UNALIGNED_STRIDE	| SingleBindingCase::FLAG_ATTRIBS_MULTIPLE_ELEMS));
		unalignedGroup->addChild(new SingleBindingCase(m_context, "unaligned_stride_elements_2_share_elements",	SingleBindingCase::FLAG_BUF_UNALIGNED_STRIDE	| SingleBindingCase::FLAG_ATTRIBS_SHARED_ELEMS));
	}

	// .buffer_bounds
	{
		// bind buffer offset cases
		bufferRangeGroup->addChild(new BindVertexBufferCase(m_context, "bind_vertex_buffer_offset_over_bounds_10",		"Offset over buffer bounds",				0x00210000, 10));
		bufferRangeGroup->addChild(new BindVertexBufferCase(m_context, "bind_vertex_buffer_offset_over_bounds_1000",	"Offset over buffer bounds",				0x00210000, 1000));
		bufferRangeGroup->addChild(new BindVertexBufferCase(m_context, "bind_vertex_buffer_offset_near_wrap_10",		"Offset over buffer bounds, near wrapping",	0x7FFFFFF0, 10));
		bufferRangeGroup->addChild(new BindVertexBufferCase(m_context, "bind_vertex_buffer_offset_near_wrap_1000",		"Offset over buffer bounds, near wrapping",	0x7FFFFFF0, 1000));
	}
}

} // Stress
} // gles31
} // deqp
