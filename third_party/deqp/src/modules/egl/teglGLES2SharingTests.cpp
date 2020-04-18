/*-------------------------------------------------------------------------
 * drawElements Quality Program EGL Module
 * ---------------------------------------
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
 * \brief EGL gles2 sharing tests
 *//*--------------------------------------------------------------------*/

#include "teglGLES2SharingTests.hpp"

#include "teglGLES2SharingThreadedTests.hpp"

#include "egluNativeWindow.hpp"
#include "egluUtil.hpp"
#include "egluUnique.hpp"

#include "eglwLibrary.hpp"
#include "eglwEnums.hpp"

#include "tcuCommandLine.hpp"
#include "tcuImageCompare.hpp"
#include "tcuSurface.hpp"
#include "tcuTestLog.hpp"
#include "tcuTexture.hpp"
#include "tcuTextureUtil.hpp"

#include "deUniquePtr.hpp"
#include "deRandom.hpp"

#include "deMath.h"
#include "deMemory.h"
#include "deString.h"

#include "gluDefs.hpp"
#include "gluShaderProgram.hpp"

#include "glwFunctions.hpp"
#include "glwEnums.hpp"

#include <memory>
#include <sstream>
#include <vector>

using std::vector;

namespace deqp
{
namespace egl
{

using namespace glw;
using namespace eglw;

class GLES2SharingTest : public TestCase
{
public:
	enum ResourceType
	{
		BUFFER = 0,
		TEXTURE,
		RENDERBUFFER,
		SHADER_PROGRAM
	};

	struct TestSpec
	{
		ResourceType	type;
		bool			destroyContextBFirst;
		bool			useResource;
		bool			destroyOnContexB;
		bool			initializeData;
		bool			renderOnContexA;
		bool			renderOnContexB;
		bool			verifyOnContexA;
		bool			verifyOnContexB;
	};

					GLES2SharingTest	(EglTestContext& eglTestCtx, const char* name , const char* desc, const TestSpec& spec);

	void			init				(void);

	IterateResult	iterate				(void);

private:
	TestSpec		m_spec;

	EGLContext		createContext		(EGLDisplay display, EGLContext share, EGLConfig config);
	void			makeCurrent			(EGLDisplay display, EGLContext context, EGLSurface surface);

protected:
	de::Random		m_random;
	tcu::TestLog&	m_log;
	glw::Functions	m_gl;

	virtual void	createResource		(void)  { DE_ASSERT(false); }
	virtual void	destroyResource		(void)	{ DE_ASSERT(false); }
	virtual void	renderResource		(tcu::Surface* screen, tcu::Surface* reference) { DE_UNREF(screen); DE_UNREF(reference); DE_ASSERT(false); }
};

GLES2SharingTest::GLES2SharingTest (EglTestContext& eglTestCtx, const char* name , const char* desc, const TestSpec& spec)
	: TestCase	(eglTestCtx, name, desc)
	, m_spec	(spec)
	, m_random	(deStringHash(name))
	, m_log		(eglTestCtx.getTestContext().getLog())
{
}

void GLES2SharingTest::init (void)
{
	m_eglTestCtx.initGLFunctions(&m_gl, glu::ApiType::es(2,0));
}

EGLContext GLES2SharingTest::createContext (EGLDisplay display, EGLContext share, EGLConfig config)
{
	const Library& egl = m_eglTestCtx.getLibrary();
	EGLContext context = EGL_NO_CONTEXT;
	const EGLint attriblist[] =
	{
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};

	EGLU_CHECK_CALL(egl, bindAPI(EGL_OPENGL_ES_API));

	context = egl.createContext(display, config, share, attriblist);
	EGLU_CHECK_MSG(egl, "Failed to create GLES2 context");
	TCU_CHECK(context != EGL_NO_CONTEXT);

	return context;
}

void GLES2SharingTest::makeCurrent (EGLDisplay display, EGLContext context, EGLSurface surface)
{
	const Library& egl = m_eglTestCtx.getLibrary();
	EGLU_CHECK_CALL(egl, makeCurrent(display, surface, surface, context));
}

TestCase::IterateResult GLES2SharingTest::iterate (void)
{
	const Library&						egl				= m_eglTestCtx.getLibrary();
	tcu::TestLog&						log				= m_testCtx.getLog();
	eglu::UniqueDisplay					display			(egl, eglu::getAndInitDisplay(m_eglTestCtx.getNativeDisplay()));
	const eglu::NativeWindowFactory&	windowFactory	= eglu::selectNativeWindowFactory(m_eglTestCtx.getNativeDisplayFactory(), m_testCtx.getCommandLine());
	EGLConfig							config;
	bool								isOk			= true;
	EGLContext							contextA		= EGL_NO_CONTEXT;
	EGLContext							contextB		= EGL_NO_CONTEXT;

	{
		const EGLint attribList[] =
		{
			EGL_RENDERABLE_TYPE,	EGL_OPENGL_ES2_BIT,
			EGL_SURFACE_TYPE,		EGL_WINDOW_BIT,
			EGL_ALPHA_SIZE,			1,
			EGL_NONE
		};

		config = eglu::chooseSingleConfig(egl, *display, attribList);
	}

	try
	{
		de::UniquePtr<eglu::NativeWindow>	window	(windowFactory.createWindow(&m_eglTestCtx.getNativeDisplay(), *display, config, DE_NULL, eglu::WindowParams(480, 480, eglu::parseWindowVisibility(m_testCtx.getCommandLine()))));
		eglu::UniqueSurface					surface	(egl, *display, eglu::createWindowSurface(m_eglTestCtx.getNativeDisplay(), *window, *display, config, DE_NULL));

		m_log << tcu::TestLog::Message << "Create context A (share_context = EGL_NO_CONTEXT)" << tcu::TestLog::EndMessage;
		contextA = createContext(*display, EGL_NO_CONTEXT, config);

		m_log << tcu::TestLog::Message << "Create context B (share_context = context A)" << tcu::TestLog::EndMessage;
		contextB = createContext(*display, contextA, config);

		if (m_spec.useResource)
		{
			m_log << tcu::TestLog::Message << "Make current context A" << tcu::TestLog::EndMessage;
			makeCurrent(*display, contextA, *surface);
			m_log << tcu::TestLog::Message << "Creating resource" << tcu::TestLog::EndMessage;
			createResource();

			int		width	= 240;
			int		height	= 240;

			if (m_spec.renderOnContexA)
			{
				m_log << tcu::TestLog::Message << "Render resource" << tcu::TestLog::EndMessage;
				if (m_spec.verifyOnContexA)
				{
					tcu::Surface screen	(width, height);
					tcu::Surface ref	(width, height);
					renderResource(&screen, &ref);

					if (!fuzzyCompare(log, "Rendered image", "Rendering result comparision", ref, screen, 0.05f, tcu::COMPARE_LOG_RESULT))
						isOk = false;
				}
				else
				{
					renderResource(DE_NULL, DE_NULL);
				}
			}

			if (m_spec.renderOnContexB)
			{
				m_log << tcu::TestLog::Message << "Make current context B" << tcu::TestLog::EndMessage;
				makeCurrent(*display, contextB, *surface);
				m_log << tcu::TestLog::Message << "Render resource" << tcu::TestLog::EndMessage;
				if (m_spec.verifyOnContexB)
				{
					tcu::Surface screen	(width, height);
					tcu::Surface ref	(width, height);
					renderResource(&screen, &ref);

					if (!fuzzyCompare(log, "Rendered image", "Rendering result comparision", ref, screen, 0.05f, tcu::COMPARE_LOG_RESULT))
						isOk = false;
				}
				else
				{
					renderResource(DE_NULL, DE_NULL);
				}
			}

			if (m_spec.destroyOnContexB)
			{
				m_log << tcu::TestLog::Message << "Make current context B" << tcu::TestLog::EndMessage;
				makeCurrent(*display, contextB, *surface);
				m_log << tcu::TestLog::Message << "Destroy resource" << tcu::TestLog::EndMessage;
				destroyResource();
			}
			else
			{
				m_log << tcu::TestLog::Message << "Make current context A" << tcu::TestLog::EndMessage;
				makeCurrent(*display, contextA, *surface);
				m_log << tcu::TestLog::Message << "Destroy resource" << tcu::TestLog::EndMessage;
				destroyResource();
			}
		}

		makeCurrent(*display, EGL_NO_CONTEXT, EGL_NO_SURFACE);

		if (m_spec.destroyContextBFirst)
		{
			m_log << tcu::TestLog::Message << "Destroy context B" << tcu::TestLog::EndMessage;
			egl.destroyContext(*display, contextB);
			contextB = EGL_NO_CONTEXT;

			m_log << tcu::TestLog::Message << "Destroy context A" << tcu::TestLog::EndMessage;
			egl.destroyContext(*display, contextA);
			contextA = EGL_NO_CONTEXT;
		}
		else
		{
			m_log << tcu::TestLog::Message << "Destroy context A" << tcu::TestLog::EndMessage;
			egl.destroyContext(*display, contextA);
			contextA = EGL_NO_CONTEXT;

			m_log << tcu::TestLog::Message << "Destroy context B" << tcu::TestLog::EndMessage;
			egl.destroyContext(*display, contextB);
			contextB = EGL_NO_CONTEXT;
		}

		EGLU_CHECK(egl);
	}
	catch (...)
	{
		egl.makeCurrent(*display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		if (contextA != EGL_NO_CONTEXT)
			egl.destroyContext(*display, contextA);
		if (contextB != EGL_NO_CONTEXT)
			egl.destroyContext(*display, contextB);
		throw;
	}

	if (isOk)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Image comparison failed");

	return STOP;
}

class GLES2BufferSharingTest : public GLES2SharingTest
{
public:
							GLES2BufferSharingTest	(EglTestContext& eglTestCtx, const char* name, const char* desc, const GLES2SharingTest::TestSpec& spec);

private:
	GLuint					m_glBuffer;
	std::vector<GLubyte>	m_buffer;

	virtual void	createResource		(void);
	virtual void	destroyResource		(void);
	virtual void	renderResource		(tcu::Surface* screen, tcu::Surface* reference);

};

GLES2BufferSharingTest::GLES2BufferSharingTest (EglTestContext& eglTestCtx, const char* name, const char* desc, const GLES2SharingTest::TestSpec& spec)
	: GLES2SharingTest	(eglTestCtx, name, desc, spec)
	, m_glBuffer		(0)
{
}

void GLES2BufferSharingTest::createResource (void)
{
	int						size	= 16*16*4;

	m_buffer.reserve(size);

	for (int i = 0; i < size; i++)
		m_buffer.push_back((GLubyte)m_random.getInt(0, 255));

	GLU_CHECK_GLW_CALL(m_gl, genBuffers(1, &m_glBuffer));
	GLU_CHECK_GLW_CALL(m_gl, bindBuffer(GL_ARRAY_BUFFER, m_glBuffer));
	GLU_CHECK_GLW_CALL(m_gl, bufferData(GL_ARRAY_BUFFER, (GLsizei)(m_buffer.size() * sizeof(GLubyte)), &(m_buffer[0]), GL_DYNAMIC_DRAW));
	GLU_CHECK_GLW_CALL(m_gl, bindBuffer(GL_ARRAY_BUFFER, 0));
}

void GLES2BufferSharingTest::destroyResource (void)
{
	GLU_CHECK_GLW_CALL(m_gl, deleteBuffers(1, &m_glBuffer));
	m_buffer.clear();
}

void GLES2BufferSharingTest::renderResource (tcu::Surface* screen, tcu::Surface* reference)
{
	DE_ASSERT((screen && reference) || (!screen && !reference));

	const char* vertexShader = ""
	"attribute mediump vec2 a_pos;\n"
	"attribute mediump float a_color;\n"
	"varying mediump float v_color;\n"
	"void main(void)\n"
	"{\n"
	"\tv_color = a_color;\n"
	"\tgl_Position = vec4(a_pos, 0.0, 1.0);\n"
	"}\n";

	const char* fragmentShader = ""
	"varying mediump float v_color;\n"
	"void main(void)\n"
	"{\n"
	"\tgl_FragColor = vec4(v_color, v_color, v_color, 1.0);\n"
	"}\n";

	glu::ShaderProgram program(m_gl, glu::makeVtxFragSources(vertexShader, fragmentShader));

	if (!program.isOk())
		TCU_FAIL("Failed to compile shader program");

	std::vector<deUint16>	indices;
	std::vector<float>		coords;

	DE_ASSERT(m_buffer.size() % 4 == 0);

	for (int i = 0; i < (int)m_buffer.size() / 4; i++)
	{
		indices.push_back((deUint16)(i*4));
		indices.push_back((deUint16)(i*4 + 1));
		indices.push_back((deUint16)(i*4 + 2));
		indices.push_back((deUint16)(i*4 + 2));
		indices.push_back((deUint16)(i*4 + 3));
		indices.push_back((deUint16)(i*4));

		coords.push_back(0.125f * (float)(i % 16) - 1.0f);
		coords.push_back(0.125f * (float)((int)((float)i / 16.0f)) - 1.0f);

		coords.push_back(0.125f * (float)(i % 16) - 1.0f);
		coords.push_back(0.125f * (float)((int)((float)i / 16.0f) + 1) - 1.0f);

		coords.push_back(0.125f * (float)((i % 16) + 1) - 1.0f);
		coords.push_back(0.125f * (float)((int)((float)i / 16.0f) + 1) - 1.0f);

		coords.push_back(0.125f * (float)((i % 16) + 1) - 1.0f);
		coords.push_back(0.125f * (float)((int)((float)i / 16.0f)) - 1.0f);
	}

	int width = 240;
	int height = 240;

	if (screen)
	{
		width = screen->getWidth();
		height = screen->getHeight();
	}

	GLU_CHECK_GLW_CALL(m_gl, viewport(0, 0, width, height));

	GLU_CHECK_GLW_CALL(m_gl, clearColor(1.0f, 0.0f, 0.0f, 1.0f));
	GLU_CHECK_GLW_CALL(m_gl, clear(GL_COLOR_BUFFER_BIT));

	GLU_CHECK_GLW_CALL(m_gl, useProgram(program.getProgram()));

	GLuint gridLocation = m_gl.getAttribLocation(program.getProgram(), "a_pos");
	GLU_CHECK_GLW_MSG(m_gl, "glGetAttribLocation()");
	TCU_CHECK(gridLocation != (GLuint)-1);

	GLuint colorLocation = m_gl.getAttribLocation(program.getProgram(), "a_color");
	GLU_CHECK_GLW_MSG(m_gl, "glGetAttribLocation()");
	TCU_CHECK(colorLocation != (GLuint)-1);

	GLU_CHECK_GLW_CALL(m_gl, enableVertexAttribArray(colorLocation));
	GLU_CHECK_GLW_CALL(m_gl, enableVertexAttribArray(gridLocation));

	GLU_CHECK_GLW_CALL(m_gl, bindBuffer(GL_ARRAY_BUFFER, m_glBuffer));
	GLU_CHECK_GLW_CALL(m_gl, vertexAttribPointer(colorLocation, 1, GL_UNSIGNED_BYTE, GL_TRUE, 0, DE_NULL));
	GLU_CHECK_GLW_CALL(m_gl, bindBuffer(GL_ARRAY_BUFFER, 0));

	GLU_CHECK_GLW_CALL(m_gl, vertexAttribPointer(gridLocation, 2, GL_FLOAT, GL_FALSE, 0, &(coords[0])));

	GLU_CHECK_GLW_CALL(m_gl, drawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_SHORT, &(indices[0])));
	GLU_CHECK_GLW_CALL(m_gl, disableVertexAttribArray(colorLocation));
	GLU_CHECK_GLW_CALL(m_gl, disableVertexAttribArray(gridLocation));

	GLU_CHECK_GLW_CALL(m_gl, useProgram(0));

	if (screen)
	{
		tcu::clear(reference->getAccess(), tcu::IVec4(0xff, 0, 0, 0xff));
		m_gl.readPixels(0, 0, screen->getWidth(), screen->getHeight(), GL_RGBA, GL_UNSIGNED_BYTE, screen->getAccess().getDataPtr());
		for (int i = 0; i < (int)m_buffer.size() / 4; i++)
		{
			float fx1 = 0.125f * (float)(i % 16) - 1.0f;
			float fy1 = 0.125f * (float)((int)((float)i / 16.0f)) - 1.0f;
			float fx2 = 0.125f * (float)((i % 16) + 1) - 1.0f;
			float fy2 = 0.125f * (float)((int)((float)i / 16.0f) + 1) - 1.0f;

			int ox = deRoundFloatToInt32((float)width	/ 2.0f);
			int oy = deRoundFloatToInt32((float)height	/ 2.0f);
			int x1 = deRoundFloatToInt32(((float)width	 * fx1 / 2.0f) + (float)ox);
			int y1 = deRoundFloatToInt32(((float)height	 * fy1 / 2.0f) + (float)oy);
			int x2 = deRoundFloatToInt32(((float)width	 * fx2 / 2.0f) + (float)ox);
			int y2 = deRoundFloatToInt32(((float)height	 * fy2 / 2.0f) + (float)oy);

			for (int x = x1; x < x2; x++)
			{
				for (int y = y1; y < y2; y++)
				{
					float		xf		= ((float)(x-x1) + 0.5f) / (float)(x2 - x1);
					float		yf		= ((float)(y-y1) + 0.5f) / (float)(y2 - y1);
					bool		tri		= yf >= xf;
					deUint8		a		= m_buffer[i*4 + (tri ? 1 : 3)];
					deUint8		b		= m_buffer[i*4 + (tri ? 2 : 0)];
					deUint8		c		= m_buffer[i*4 + (tri ? 0 : 2)];
					float		s		= tri ? xf : 1.0f-xf;
					float		t		= tri ? 1.0f-yf : yf;
					float		val		= (float)a + (float)(b-a)*s + (float)(c-a)*t;

					reference->setPixel(x, y, tcu::RGBA((deUint8)val, (deUint8)val, (deUint8)val, 255));
				}
			}
		}
	}
}

class GLES2TextureSharingTest : public GLES2SharingTest
{
public:
							GLES2TextureSharingTest	(EglTestContext& eglTestCtx, const char* name, const char* desc, const GLES2SharingTest::TestSpec& spec);

private:
	GLuint					m_glTexture;
	tcu::Texture2D			m_texture;

	virtual void	createResource		(void);
	virtual void	destroyResource		(void);
	virtual void	renderResource		(tcu::Surface* screen, tcu::Surface* reference);

};

GLES2TextureSharingTest::GLES2TextureSharingTest (EglTestContext& eglTestCtx, const char* name, const char* desc, const GLES2SharingTest::TestSpec& spec)
	: GLES2SharingTest	(eglTestCtx, name, desc, spec)
	, m_glTexture		(0)
	, m_texture			(tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNORM_INT8), 1, 1)
{
}

void GLES2TextureSharingTest::createResource (void)
{
	int width	= 128;
	int	height	= 128;
	m_texture = tcu::Texture2D(tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNORM_INT8), width, height);
	m_texture.allocLevel(0);

	tcu::fillWithComponentGradients(m_texture.getLevel(0), tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f), tcu::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
	GLU_CHECK_GLW_CALL(m_gl, genTextures(1, &m_glTexture));
	GLU_CHECK_GLW_CALL(m_gl, bindTexture(GL_TEXTURE_2D, m_glTexture));
	GLU_CHECK_GLW_CALL(m_gl, texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
	GLU_CHECK_GLW_CALL(m_gl, texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
	GLU_CHECK_GLW_CALL(m_gl, texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GLU_CHECK_GLW_CALL(m_gl, texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GLU_CHECK_GLW_CALL(m_gl, texImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_texture.getLevel(0).getDataPtr()));
	GLU_CHECK_GLW_CALL(m_gl, bindTexture(GL_TEXTURE_2D, 0));
}

void GLES2TextureSharingTest::destroyResource (void)
{
	GLU_CHECK_GLW_CALL(m_gl, deleteTextures(1, &m_glTexture));
}

void GLES2TextureSharingTest::renderResource (tcu::Surface* screen, tcu::Surface* reference)
{
	DE_ASSERT((screen && reference) || (!screen && !reference));

	const char* vertexShader = ""
	"attribute mediump vec2 a_pos;\n"
	"attribute mediump vec2 a_texCorod;\n"
	"varying mediump vec2 v_texCoord;\n"
	"void main(void)\n"
	"{\n"
	"\tv_texCoord = a_texCorod;\n"
	"\tgl_Position = vec4(a_pos, 0.0, 1.0);\n"
	"}\n";

	const char* fragmentShader = ""
	"varying mediump vec2 v_texCoord;\n"
	"uniform sampler2D u_sampler;\n"
	"void main(void)\n"
	"{\n"
	"\tgl_FragColor = texture2D(u_sampler, v_texCoord);\n"
	"}\n";

	glu::ShaderProgram program(m_gl, glu::makeVtxFragSources(vertexShader, fragmentShader));

	if (!program.isOk())
		TCU_FAIL("Failed to compile shader program");

	int width = 240;
	int height = 240;

	if (screen)
	{
		width = screen->getWidth();
		height = screen->getHeight();
	}

	static const GLfloat coords[] = {
		-1.0f, -1.0f,
		 1.0f, -1.0f,
		 1.0f,  1.0f,
		-1.0f,  1.0f
	};

	static const GLfloat texCoords[] = {
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f
	};

	static const GLushort indices[] = {
		0, 1, 2,
		2, 3, 0
	};

	GLU_CHECK_GLW_CALL(m_gl, viewport(0, 0, width, height));

	GLU_CHECK_GLW_CALL(m_gl, clearColor(1.0f, 0.0f, 0.0f, 1.0f));
	GLU_CHECK_GLW_CALL(m_gl, clear(GL_COLOR_BUFFER_BIT));

	GLU_CHECK_GLW_CALL(m_gl, useProgram(program.getProgram()));

	GLuint coordLocation = m_gl.getAttribLocation(program.getProgram(), "a_pos");
	GLU_CHECK_GLW_MSG(m_gl, "glGetAttribLocation()");
	TCU_CHECK(coordLocation != (GLuint)-1);

	GLuint texCoordLocation = m_gl.getAttribLocation(program.getProgram(), "a_texCorod");
	GLU_CHECK_GLW_MSG(m_gl, "glGetAttribLocation()");
	TCU_CHECK(texCoordLocation != (GLuint)-1);


	GLuint samplerLocation = m_gl.getUniformLocation(program.getProgram(), "u_sampler");
	GLU_CHECK_GLW_MSG(m_gl, "glGetUniformLocation()");
	TCU_CHECK(samplerLocation != (GLuint)-1);

	GLU_CHECK_GLW_CALL(m_gl, activeTexture(GL_TEXTURE0));
	GLU_CHECK_GLW_CALL(m_gl, bindTexture(GL_TEXTURE_2D, m_glTexture));

	GLU_CHECK_GLW_CALL(m_gl, uniform1i(samplerLocation, 0));

	GLU_CHECK_GLW_CALL(m_gl, enableVertexAttribArray(texCoordLocation));
	GLU_CHECK_GLW_CALL(m_gl, enableVertexAttribArray(coordLocation));

	GLU_CHECK_GLW_CALL(m_gl, vertexAttribPointer(texCoordLocation, 2, GL_FLOAT, GL_FALSE, 0, texCoords));
	GLU_CHECK_GLW_CALL(m_gl, vertexAttribPointer(coordLocation, 2, GL_FLOAT, GL_FALSE, 0, coords));

	GLU_CHECK_GLW_CALL(m_gl, drawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices));
	GLU_CHECK_GLW_CALL(m_gl, disableVertexAttribArray(coordLocation));
	GLU_CHECK_GLW_CALL(m_gl, disableVertexAttribArray(texCoordLocation));

	GLU_CHECK_GLW_CALL(m_gl, bindTexture(GL_TEXTURE_2D, 0));
	GLU_CHECK_GLW_CALL(m_gl, useProgram(0));

	if (screen)
	{
		m_gl.readPixels(0, 0, screen->getWidth(), screen->getHeight(), GL_RGBA, GL_UNSIGNED_BYTE, screen->getAccess().getDataPtr());

		for (int x = 0; x < width; x++)
		{
			for (int y = 0; y < height; y++)
			{
				float t = ((float)x / ((float)width - 1.0f));
				float s = ((float)y / ((float)height - 1.0f));
				float lod = 0.0f;

				tcu::Vec4 color = m_texture.sample(tcu::Sampler(tcu::Sampler::REPEAT_GL, tcu::Sampler::REPEAT_GL, tcu::Sampler::REPEAT_GL, tcu::Sampler::LINEAR, tcu::Sampler::LINEAR), t, s, lod);

				int r = deClamp32((int)(255.0f * color.x()), 0, 255);
				int g = deClamp32((int)(255.0f * color.y()), 0, 255);
				int b = deClamp32((int)(255.0f * color.z()), 0, 255);
				int a = deClamp32((int)(255.0f * color.w()), 0, 255);

				reference->setPixel(x, y, tcu::RGBA(r, g, b, a));
			}
		}
	}
}

class GLES2ProgramSharingTest : public GLES2SharingTest
{
public:
						GLES2ProgramSharingTest	(EglTestContext& eglTestCtx, const char* name, const char* desc, const GLES2SharingTest::TestSpec& spec);

private:
	glu::ShaderProgram*	m_program;

	virtual void		createResource			(void);
	virtual void		destroyResource			(void);
	virtual void		renderResource			(tcu::Surface* screen, tcu::Surface* reference);

};

GLES2ProgramSharingTest::GLES2ProgramSharingTest (EglTestContext& eglTestCtx, const char* name, const char* desc, const GLES2SharingTest::TestSpec& spec)
	: GLES2SharingTest	(eglTestCtx, name, desc, spec)
	, m_program			(DE_NULL)
{
}

void GLES2ProgramSharingTest::createResource (void)
{
	const char* vertexShader = ""
	"attribute mediump vec2 a_pos;\n"
	"attribute mediump vec4 a_color;\n"
	"varying mediump vec4 v_color;\n"
	"void main(void)\n"
	"{\n"
	"\tv_color = a_color;\n"
	"\tgl_Position = vec4(a_pos, 0.0, 1.0);\n"
	"}\n";

	const char* fragmentShader = ""
	"varying mediump vec4 v_color;\n"
	"void main(void)\n"
	"{\n"
	"\tgl_FragColor = v_color;\n"
	"}\n";

	m_program = new glu::ShaderProgram(m_gl, glu::makeVtxFragSources(vertexShader, fragmentShader));

	if (!m_program->isOk())
		TCU_FAIL("Failed to compile shader program");
}

void GLES2ProgramSharingTest::destroyResource (void)
{
	delete m_program;
}

void GLES2ProgramSharingTest::renderResource (tcu::Surface* screen, tcu::Surface* reference)
{
	DE_ASSERT((screen && reference) || (!screen && !reference));

	int width = 240;
	int height = 240;

	if (screen)
	{
		width = screen->getWidth();
		height = screen->getHeight();
	}

	static const GLfloat coords[] = {
		-0.9f, -0.9f,
		 0.9f, -0.9f,
		 0.9f,  0.9f,
		-0.9f,  0.9f
	};

	static const GLfloat colors [] = {
		0.0f, 0.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 0.0f, 1.0f,
		0.0f, 1.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f, 1.0f
	};

	static const GLushort indices[] = {
		0, 1, 2,
		2, 3, 0
	};

	GLU_CHECK_GLW_CALL(m_gl, viewport(0, 0, width, height));

	GLU_CHECK_GLW_CALL(m_gl, clearColor(1.0f, 0.0f, 0.0f, 1.0f));
	GLU_CHECK_GLW_CALL(m_gl, clear(GL_COLOR_BUFFER_BIT));

	GLU_CHECK_GLW_CALL(m_gl, useProgram(m_program->getProgram()));

	GLuint coordLocation = m_gl.getAttribLocation(m_program->getProgram(), "a_pos");
	GLU_CHECK_GLW_MSG(m_gl, "glGetAttribLocation()");
	TCU_CHECK(coordLocation != (GLuint)-1);

	GLuint colorLocation = m_gl.getAttribLocation(m_program->getProgram(), "a_color");
	GLU_CHECK_GLW_MSG(m_gl, "glGetAttribLocation()");
	TCU_CHECK(colorLocation != (GLuint)-1);

	GLU_CHECK_GLW_CALL(m_gl, enableVertexAttribArray(colorLocation));
	GLU_CHECK_GLW_CALL(m_gl, enableVertexAttribArray(coordLocation));

	GLU_CHECK_GLW_CALL(m_gl, vertexAttribPointer(colorLocation, 4, GL_FLOAT, GL_FALSE, 0, colors));
	GLU_CHECK_GLW_CALL(m_gl, vertexAttribPointer(coordLocation, 2, GL_FLOAT, GL_FALSE, 0, coords));

	GLU_CHECK_GLW_CALL(m_gl, drawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices));
	GLU_CHECK_GLW_CALL(m_gl, disableVertexAttribArray(coordLocation));
	GLU_CHECK_GLW_CALL(m_gl, disableVertexAttribArray(colorLocation));
	GLU_CHECK_GLW_CALL(m_gl, useProgram(0));

	if (screen)
	{
		m_gl.readPixels(0, 0, screen->getWidth(), screen->getHeight(), GL_RGBA, GL_UNSIGNED_BYTE, screen->getAccess().getDataPtr());

		tcu::clear(reference->getAccess(), tcu::IVec4(0xff, 0, 0, 0xff));

		int x1 = (int)(((float)width/2.0f)  * (-0.9f) + ((float)width/2.0f));
		int x2 = (int)(((float)width/2.0f)  *   0.9f  + ((float)width/2.0f));
		int y1 = (int)(((float)height/2.0f) * (-0.9f) + ((float)height/2.0f));
		int y2 = (int)(((float)height/2.0f) *   0.9f  + ((float)height/2.0f));

		for (int x = x1; x <= x2; x++)
		{
			for (int y = y1; y <= y2; y++)
			{
				float t = ((float)(x-x1) / (float)(x2-x1));
				float s = ((float)(y-y1) / (float)(y2-y1));
				bool isUpper = t > s;

				tcu::Vec4 a(colors[0],		colors[1],		colors[2],		colors[3]);
				tcu::Vec4 b(colors[4 + 0],	colors[4 + 1],	colors[4 + 2],	colors[4 + 3]);
				tcu::Vec4 c(colors[8 + 0],	colors[8 + 1],	colors[8 + 2],	colors[8 + 3]);
				tcu::Vec4 d(colors[12 + 0],	colors[12 + 1],	colors[12 + 2],	colors[12 + 3]);


				tcu::Vec4 color;

				if (isUpper)
					color = a * (1.0f - t)  + b * (t - s) + s * c;
				else
					color = a * (1.0f - s)  + d * (s - t) + t * c;

				int red		= deClamp32((int)(255.0f * color.x()), 0, 255);
				int green	= deClamp32((int)(255.0f * color.y()), 0, 255);
				int blue	= deClamp32((int)(255.0f * color.z()), 0, 255);
				int alpha	= deClamp32((int)(255.0f * color.w()), 0, 255);

				reference->setPixel(x, y, tcu::RGBA(red, green, blue, alpha));
			}
		}
	}
}

class GLES2ShaderSharingTest : public GLES2SharingTest
{
public:
					GLES2ShaderSharingTest	(EglTestContext& eglTestCtx, const char* name, const char* desc, GLenum shaderType, const GLES2SharingTest::TestSpec& spec);

private:
	GLuint			m_shader;
	GLenum			m_shaderType;

	virtual void	createResource		(void);
	virtual void	destroyResource		(void);
	virtual void	renderResource		(tcu::Surface* screen, tcu::Surface* reference);

};

GLES2ShaderSharingTest::GLES2ShaderSharingTest (EglTestContext& eglTestCtx, const char* name, const char* desc, GLenum shaderType, const GLES2SharingTest::TestSpec& spec)
	: GLES2SharingTest	(eglTestCtx, name, desc, spec)
	, m_shader			(0)
	, m_shaderType		(shaderType)
{
}

void GLES2ShaderSharingTest::createResource (void)
{
	const char* vertexShader = ""
	"attribute mediump vec2 a_pos;\n"
	"attribute mediump vec4 a_color;\n"
	"varying mediump vec4 v_color;\n"
	"void main(void)\n"
	"{\n"
	"\tv_color = a_color;\n"
	"\tgl_Position = vec4(a_pos, 0.0, 1.0);\n"
	"}\n";

	const char* fragmentShader = ""
	"varying mediump vec4 v_color;\n"
	"void main(void)\n"
	"{\n"
	"\tgl_FragColor = v_color;\n"
	"}\n";


	m_shader = m_gl.createShader(m_shaderType);
	GLU_CHECK_GLW_MSG(m_gl, "glCreateShader()");

	switch (m_shaderType)
	{
		case GL_VERTEX_SHADER:
			GLU_CHECK_GLW_CALL(m_gl, shaderSource(m_shader, 1, &vertexShader, DE_NULL));
			break;

		case GL_FRAGMENT_SHADER:
			GLU_CHECK_GLW_CALL(m_gl, shaderSource(m_shader, 1, &fragmentShader, DE_NULL));
			break;

		default:
			DE_ASSERT(false);
	}

	GLU_CHECK_GLW_CALL(m_gl, compileShader(m_shader));

	GLint status = 0;
	GLU_CHECK_GLW_CALL(m_gl, getShaderiv(m_shader, GL_COMPILE_STATUS, &status));

	if (!status)
	{
		char buffer[256];
		GLU_CHECK_GLW_CALL(m_gl, getShaderInfoLog(m_shader, 256, DE_NULL, buffer));

		m_log << tcu::TestLog::Message << "Failed to compile shader" << tcu::TestLog::EndMessage;

		switch (m_shaderType)
		{
			case GL_VERTEX_SHADER:
				m_log << tcu::TestLog::Message << vertexShader << tcu::TestLog::EndMessage;
				break;

			case GL_FRAGMENT_SHADER:
				m_log << tcu::TestLog::Message << fragmentShader << tcu::TestLog::EndMessage;
				break;

			default:
				DE_ASSERT(false);
		}

		m_log << tcu::TestLog::Message << buffer << tcu::TestLog::EndMessage;
		TCU_FAIL("Failed to compile shader");
	}
}

void GLES2ShaderSharingTest::destroyResource (void)
{
	GLU_CHECK_GLW_CALL(m_gl, deleteShader(m_shader));
}

void GLES2ShaderSharingTest::renderResource (tcu::Surface* screen, tcu::Surface* reference)
{
	DE_ASSERT((screen && reference) || (!screen && !reference));

	int width = 240;
	int height = 240;

	const char* vertexShader = ""
	"attribute mediump vec2 a_pos;\n"
	"attribute mediump vec4 a_color;\n"
	"varying mediump vec4 v_color;\n"
	"void main(void)\n"
	"{\n"
	"\tv_color = a_color;\n"
	"\tgl_Position = vec4(a_pos, 0.0, 1.0);\n"
	"}\n";

	const char* fragmentShader = ""
	"varying mediump vec4 v_color;\n"
	"void main(void)\n"
	"{\n"
	"\tgl_FragColor = v_color;\n"
	"}\n";


	GLuint otherShader = (GLuint)-1;

	switch (m_shaderType)
	{
		case GL_VERTEX_SHADER:
			otherShader = m_gl.createShader(GL_FRAGMENT_SHADER);
			GLU_CHECK_GLW_MSG(m_gl, "glCreateShader()");
			GLU_CHECK_GLW_CALL(m_gl, shaderSource(otherShader, 1, &fragmentShader, DE_NULL));
			break;

		case GL_FRAGMENT_SHADER:
			otherShader = m_gl.createShader(GL_VERTEX_SHADER);
			GLU_CHECK_GLW_MSG(m_gl, "glCreateShader()");
			GLU_CHECK_GLW_CALL(m_gl, shaderSource(otherShader, 1, &vertexShader, DE_NULL));
			break;

		default:
			DE_ASSERT(false);
	}

	GLU_CHECK_GLW_CALL(m_gl, compileShader(otherShader));

	GLint status = 0;
	GLU_CHECK_GLW_CALL(m_gl, getShaderiv(otherShader, GL_COMPILE_STATUS, &status));

	if (!status)
	{
		char buffer[256];
		GLU_CHECK_GLW_CALL(m_gl, getShaderInfoLog(otherShader, 256, DE_NULL, buffer));

		m_log << tcu::TestLog::Message << "Failed to compile shader" << tcu::TestLog::EndMessage;

		switch (m_shaderType)
		{
			case GL_FRAGMENT_SHADER:
				m_log << tcu::TestLog::Message << vertexShader << tcu::TestLog::EndMessage;
				break;

			case GL_VERTEX_SHADER:
				m_log << tcu::TestLog::Message << fragmentShader << tcu::TestLog::EndMessage;
				break;

			default:
				DE_ASSERT(false);
		}

		m_log << tcu::TestLog::Message << buffer << tcu::TestLog::EndMessage;
		TCU_FAIL("Failed to compile shader");
	}

	GLuint program = m_gl.createProgram();
	GLU_CHECK_GLW_MSG(m_gl, "glCreateProgram()");

	GLU_CHECK_GLW_CALL(m_gl, attachShader(program, m_shader));
	GLU_CHECK_GLW_CALL(m_gl, attachShader(program, otherShader));

	GLU_CHECK_GLW_CALL(m_gl, linkProgram(program));
	GLU_CHECK_GLW_CALL(m_gl, deleteShader(otherShader));

	status = 0;
	GLU_CHECK_GLW_CALL(m_gl, getProgramiv(program, GL_LINK_STATUS, &status));

	if (!status)
	{
		char buffer[256];
		GLU_CHECK_GLW_CALL(m_gl, getProgramInfoLog(program, 256, DE_NULL, buffer));

		m_log << tcu::TestLog::Message << "Failed to link program" << tcu::TestLog::EndMessage;

		m_log << tcu::TestLog::Message << vertexShader << tcu::TestLog::EndMessage;
		m_log << tcu::TestLog::Message << fragmentShader << tcu::TestLog::EndMessage;
		m_log << tcu::TestLog::Message << buffer << tcu::TestLog::EndMessage;
		TCU_FAIL("Failed to link program");
	}

	if (screen)
	{
		width = screen->getWidth();
		height = screen->getHeight();
	}

	static const GLfloat coords[] = {
		-0.9f, -0.9f,
		 0.9f, -0.9f,
		 0.9f,  0.9f,
		-0.9f,  0.9f
	};

	static const GLfloat colors [] = {
		0.0f, 0.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 0.0f, 1.0f,
		0.0f, 1.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f, 1.0f
	};

	static const GLushort indices[] = {
		0, 1, 2,
		2, 3, 0
	};

	GLU_CHECK_GLW_CALL(m_gl, viewport(0, 0, width, height));

	GLU_CHECK_GLW_CALL(m_gl, clearColor(1.0f, 0.0f, 0.0f, 1.0f));
	GLU_CHECK_GLW_CALL(m_gl, clear(GL_COLOR_BUFFER_BIT));

	GLU_CHECK_GLW_CALL(m_gl, useProgram(program));

	GLuint coordLocation = m_gl.getAttribLocation(program, "a_pos");
	GLU_CHECK_GLW_MSG(m_gl, "glGetAttribLocation()");
	TCU_CHECK(coordLocation != (GLuint)-1);

	GLuint colorLocation = m_gl.getAttribLocation(program, "a_color");
	GLU_CHECK_GLW_MSG(m_gl, "glGetAttribLocation()");
	TCU_CHECK(colorLocation != (GLuint)-1);

	GLU_CHECK_GLW_CALL(m_gl, enableVertexAttribArray(colorLocation));
	GLU_CHECK_GLW_CALL(m_gl, enableVertexAttribArray(coordLocation));

	GLU_CHECK_GLW_CALL(m_gl, vertexAttribPointer(colorLocation, 4, GL_FLOAT, GL_FALSE, 0, colors));
	GLU_CHECK_GLW_CALL(m_gl, vertexAttribPointer(coordLocation, 2, GL_FLOAT, GL_FALSE, 0, coords));

	GLU_CHECK_GLW_CALL(m_gl, drawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices));
	GLU_CHECK_GLW_CALL(m_gl, disableVertexAttribArray(coordLocation));
	GLU_CHECK_GLW_CALL(m_gl, disableVertexAttribArray(colorLocation));
	GLU_CHECK_GLW_CALL(m_gl, useProgram(0));

	if (screen)
	{
		m_gl.readPixels(0, 0, screen->getWidth(), screen->getHeight(), GL_RGBA, GL_UNSIGNED_BYTE, screen->getAccess().getDataPtr());

		tcu::clear(reference->getAccess(), tcu::IVec4(0xff, 0, 0, 0xff));

		int x1 = (int)(((float)width/2.0f)  * (-0.9f) + ((float)width/2.0f));
		int x2 = (int)(((float)width/2.0f)  *   0.9f  + ((float)width/2.0f));
		int y1 = (int)(((float)height/2.0f) * (-0.9f) + ((float)height/2.0f));
		int y2 = (int)(((float)height/2.0f) *   0.9f  + ((float)height/2.0f));

		for (int x = x1; x <= x2; x++)
		{
			for (int y = y1; y <= y2; y++)
			{
				float t = ((float)(x-x1) / (float)(x2-x1));
				float s = ((float)(y-y1) / (float)(y2-y1));
				bool isUpper = t > s;

				tcu::Vec4 a(colors[0],		colors[1],		colors[2],		colors[3]);
				tcu::Vec4 b(colors[4 + 0],	colors[4 + 1],	colors[4 + 2],	colors[4 + 3]);
				tcu::Vec4 c(colors[8 + 0],	colors[8 + 1],	colors[8 + 2],	colors[8 + 3]);
				tcu::Vec4 d(colors[12 + 0],	colors[12 + 1],	colors[12 + 2],	colors[12 + 3]);


				tcu::Vec4 color;

				if (isUpper)
					color = a * (1.0f - t)  + b * (t - s) + s * c;
				else
					color = a * (1.0f - s)  + d * (s - t) + t * c;

				int red		= deClamp32((int)(255.0f * color.x()), 0, 255);
				int green	= deClamp32((int)(255.0f * color.y()), 0, 255);
				int blue	= deClamp32((int)(255.0f * color.z()), 0, 255);
				int alpha	= deClamp32((int)(255.0f * color.w()), 0, 255);

				reference->setPixel(x, y, tcu::RGBA(red, green, blue, alpha));
			}
		}
	}
}

SharingTests::SharingTests (EglTestContext& eglTestCtx)
	: TestCaseGroup	(eglTestCtx, "sharing", "Sharing test cases")
{
}

void SharingTests::init (void)
{
	TestCaseGroup* gles2 = new TestCaseGroup(m_eglTestCtx, "gles2", "OpenGL ES 2 sharing test");

	TestCaseGroup* context = new TestCaseGroup(m_eglTestCtx, "context", "Context creation and destruction tests");

	{
		GLES2SharingTest::TestSpec spec;
		spec.destroyContextBFirst	= false;
		spec.useResource			= false;
		spec.destroyOnContexB		= false;
		spec.initializeData			= true;
		spec.renderOnContexA		= true;
		spec.renderOnContexB		= true;
		spec.verifyOnContexA		= true;
		spec.verifyOnContexB		= true;

		context->addChild(new GLES2SharingTest(m_eglTestCtx, "create_destroy", "Simple context creation and destruction", spec));
	}
	{
		GLES2SharingTest::TestSpec spec;
		spec.destroyContextBFirst	= true;
		spec.useResource			= false;
		spec.destroyOnContexB		= false;
		spec.initializeData			= false;
		spec.renderOnContexA		= false;
		spec.renderOnContexB		= false;
		spec.verifyOnContexA		= false;
		spec.verifyOnContexB		= false;

		context->addChild(new GLES2SharingTest(m_eglTestCtx, "create_destroy_mixed", "Simple context creation and destruction test with different destruction order", spec));
	}

	gles2->addChild(context);

	TestCaseGroup* buffer = new TestCaseGroup(m_eglTestCtx, "buffer", "Buffer creation, destruction and rendering test");

	{
		GLES2SharingTest::TestSpec spec;
		spec.destroyContextBFirst	= false;
		spec.useResource			= true;
		spec.destroyOnContexB		= false;
		spec.initializeData			= true;
		spec.renderOnContexA		= false;
		spec.renderOnContexB		= false;
		spec.verifyOnContexA		= false;
		spec.verifyOnContexB		= false;

		buffer->addChild(new GLES2BufferSharingTest(m_eglTestCtx, "create_delete", "Create and delete on shared context", spec));
	}
	{
		GLES2SharingTest::TestSpec spec;
		spec.destroyContextBFirst	= false;
		spec.useResource			= true;
		spec.destroyOnContexB		= true;
		spec.initializeData			= true;
		spec.renderOnContexA		= false;
		spec.renderOnContexB		= false;
		spec.verifyOnContexA		= false;
		spec.verifyOnContexB		= false;

		buffer->addChild(new GLES2BufferSharingTest(m_eglTestCtx, "create_delete_mixed", "Create and delet on different contexts", spec));
	}
	{
		GLES2SharingTest::TestSpec spec;
		spec.destroyContextBFirst	= false;
		spec.useResource			= true;
		spec.destroyOnContexB		= false;
		spec.initializeData			= true;
		spec.renderOnContexA		= true;
		spec.renderOnContexB		= true;
		spec.verifyOnContexA		= true;
		spec.verifyOnContexB		= true;

		buffer->addChild(new GLES2BufferSharingTest(m_eglTestCtx, "render", "Create, rendering on two different contexts and delete", spec));
	}

	gles2->addChild(buffer);

	TestCaseGroup* texture = new TestCaseGroup(m_eglTestCtx, "texture", "Texture creation, destruction and rendering tests");

	{
		GLES2SharingTest::TestSpec spec;
		spec.destroyContextBFirst	= false;
		spec.useResource			= true;
		spec.destroyOnContexB		= false;
		spec.initializeData			= true;
		spec.renderOnContexA		= false;
		spec.renderOnContexB		= false;
		spec.verifyOnContexA		= false;
		spec.verifyOnContexB		= false;

		texture->addChild(new GLES2TextureSharingTest(m_eglTestCtx, "create_delete", "Create and delete on shared context", spec));
	}
	{
		GLES2SharingTest::TestSpec spec;
		spec.destroyContextBFirst	= false;
		spec.useResource			= true;
		spec.destroyOnContexB		= true;
		spec.initializeData			= true;
		spec.renderOnContexA		= false;
		spec.renderOnContexB		= false;
		spec.verifyOnContexA		= false;
		spec.verifyOnContexB		= false;

		texture->addChild(new GLES2TextureSharingTest(m_eglTestCtx, "create_delete_mixed", "Create and delete on different contexts", spec));
	}
	{
		GLES2SharingTest::TestSpec spec;
		spec.destroyContextBFirst	= false;
		spec.useResource			= true;
		spec.destroyOnContexB		= false;
		spec.initializeData			= true;
		spec.renderOnContexA		= true;
		spec.renderOnContexB		= true;
		spec.verifyOnContexA		= true;
		spec.verifyOnContexB		= true;

		texture->addChild(new GLES2TextureSharingTest(m_eglTestCtx, "render", "Create, render in two contexts and delete", spec));
	}

	gles2->addChild(texture);

	TestCaseGroup* program = new TestCaseGroup(m_eglTestCtx, "program", "Program creation, destruction and rendering test");

	{
		GLES2SharingTest::TestSpec spec;
		spec.destroyContextBFirst	= false;
		spec.useResource			= true;
		spec.destroyOnContexB		= false;
		spec.initializeData			= true;
		spec.renderOnContexA		= false;
		spec.renderOnContexB		= false;
		spec.verifyOnContexA		= false;
		spec.verifyOnContexB		= false;

		program->addChild(new GLES2ProgramSharingTest(m_eglTestCtx, "create_delete", "Create and delete on shared context", spec));
	}
	{
		GLES2SharingTest::TestSpec spec;
		spec.destroyContextBFirst	= false;
		spec.useResource			= true;
		spec.destroyOnContexB		= true;
		spec.initializeData			= true;
		spec.renderOnContexA		= false;
		spec.renderOnContexB		= false;
		spec.verifyOnContexA		= false;
		spec.verifyOnContexB		= false;

		program->addChild(new GLES2ProgramSharingTest(m_eglTestCtx, "create_delete_mixed", "Create and delete on different contexts", spec));
	}
	{
		GLES2SharingTest::TestSpec spec;
		spec.destroyContextBFirst	= false;
		spec.useResource			= true;
		spec.destroyOnContexB		= false;
		spec.initializeData			= true;
		spec.renderOnContexA		= true;
		spec.renderOnContexB		= true;
		spec.verifyOnContexA		= true;
		spec.verifyOnContexB		= true;

		program->addChild(new GLES2ProgramSharingTest(m_eglTestCtx, "render", "Create, render in two contexts and delete", spec));
	}

	gles2->addChild(program);

	TestCaseGroup* shader = new TestCaseGroup(m_eglTestCtx, "shader", "Shader creation, destruction and rendering test");

	{
		GLES2SharingTest::TestSpec spec;
		spec.destroyContextBFirst	= false;
		spec.useResource			= true;
		spec.destroyOnContexB		= false;
		spec.initializeData			= true;
		spec.renderOnContexA		= false;
		spec.renderOnContexB		= false;
		spec.verifyOnContexA		= false;
		spec.verifyOnContexB		= false;

		shader->addChild(new GLES2ShaderSharingTest(m_eglTestCtx, "create_delete_vert", "Create and delete on shared context", GL_VERTEX_SHADER, spec));
	}
	{
		GLES2SharingTest::TestSpec spec;
		spec.destroyContextBFirst	= false;
		spec.useResource			= true;
		spec.destroyOnContexB		= true;
		spec.initializeData			= true;
		spec.renderOnContexA		= false;
		spec.renderOnContexB		= false;
		spec.verifyOnContexA		= false;
		spec.verifyOnContexB		= false;

		shader->addChild(new GLES2ShaderSharingTest(m_eglTestCtx, "create_delete_mixed_vert", "Create and delete on different contexts", GL_VERTEX_SHADER, spec));
	}
	{
		GLES2SharingTest::TestSpec spec;
		spec.destroyContextBFirst	= false;
		spec.useResource			= true;
		spec.destroyOnContexB		= false;
		spec.initializeData			= true;
		spec.renderOnContexA		= true;
		spec.renderOnContexB		= true;
		spec.verifyOnContexA		= true;
		spec.verifyOnContexB		= true;

		shader->addChild(new GLES2ShaderSharingTest(m_eglTestCtx, "render_vert", "Create, render on two contexts and delete", GL_VERTEX_SHADER, spec));
	}
	{
		GLES2SharingTest::TestSpec spec;
		spec.destroyContextBFirst	= false;
		spec.useResource			= true;
		spec.destroyOnContexB		= false;
		spec.initializeData			= true;
		spec.renderOnContexA		= false;
		spec.renderOnContexB		= false;
		spec.verifyOnContexA		= false;
		spec.verifyOnContexB		= false;

		shader->addChild(new GLES2ShaderSharingTest(m_eglTestCtx, "create_delete_frag", "Create and delete on shared context", GL_FRAGMENT_SHADER, spec));
	}
	{
		GLES2SharingTest::TestSpec spec;
		spec.destroyContextBFirst	= false;
		spec.useResource			= true;
		spec.destroyOnContexB		= true;
		spec.initializeData			= true;
		spec.renderOnContexA		= false;
		spec.renderOnContexB		= false;
		spec.verifyOnContexA		= false;
		spec.verifyOnContexB		= false;

		shader->addChild(new GLES2ShaderSharingTest(m_eglTestCtx, "create_delete_mixed_frag", "Create and delete on different contexts", GL_FRAGMENT_SHADER, spec));
	}
	{
		GLES2SharingTest::TestSpec spec;
		spec.destroyContextBFirst	= false;
		spec.useResource			= true;
		spec.destroyOnContexB		= false;
		spec.initializeData			= true;
		spec.renderOnContexA		= true;
		spec.renderOnContexB		= true;
		spec.verifyOnContexA		= true;
		spec.verifyOnContexB		= true;

		shader->addChild(new GLES2ShaderSharingTest(m_eglTestCtx, "render_frag", "Create, render on two contexts and delete", GL_FRAGMENT_SHADER, spec));
	}


	gles2->addChild(shader);


	gles2->addChild(new GLES2SharingThreadedTests(m_eglTestCtx));

	addChild(gles2);
}

} // egl
} // deqp
