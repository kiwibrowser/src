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
 * \brief GLES2 resource sharing performnace tests.
 *//*--------------------------------------------------------------------*/

#include "teglGLES2SharedRenderingPerfTests.hpp"

#include "tcuTestLog.hpp"

#include "egluUtil.hpp"
#include "eglwLibrary.hpp"
#include "eglwEnums.hpp"

#include "gluDefs.hpp"
#include "glwDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"

#include "deThread.hpp"
#include "deClock.h"
#include "deStringUtil.hpp"
#include "deSTLUtil.hpp"

#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

namespace deqp
{
namespace egl
{

using tcu::TestLog;
using std::vector;
using std::string;

using namespace glw;
using namespace eglw;

namespace
{

struct TestConfig
{
	enum TextureType
	{
		TEXTURETYPE_TEXTURE = 0,
		TEXTURETYPE_SHARED_TEXTURE,
		TEXTURETYPE_IMAGE,
		TEXTURETYPE_SHARED_IMAGE,
		TEXTURETYPE_SHARED_IMAGE_TEXTURE
	};

	int			threadCount;
	int			perThreadContextCount;

	int			frameCount;
	int			drawCallCount;
	int			triangleCount;

	bool		sharedContexts;

	bool		useCoordBuffer;
	bool		sharedCoordBuffer;

	bool		useIndices;
	bool		useIndexBuffer;
	bool		sharedIndexBuffer;

	bool		useTexture;
	TextureType	textureType;

	bool		sharedProgram;

	int			textureWidth;
	int			textureHeight;

	int			surfaceWidth;
	int			surfaceHeight;
};

class TestContext
{
public:
						TestContext		(EglTestContext& eglTestCtx, EGLDisplay display, EGLConfig eglConfig, const TestConfig& config, bool share, TestContext* parent);
						~TestContext	(void);

	void				render			(void);

	EGLContext			getEGLContext	(void) { return m_eglContext; }

	GLuint				getCoordBuffer	(void) const { return m_coordBuffer;	}
	GLuint				getIndexBuffer	(void) const { return m_indexBuffer;	}
	GLuint				getTexture		(void) const { return m_texture;		}
	GLuint				getProgram		(void) const { return m_program;		}
	EGLImageKHR			getEGLImage		(void) const { return m_eglImage;		}

private:
	TestContext*		m_parent;
	EglTestContext&		m_testCtx;
	TestConfig			m_config;

	EGLDisplay			m_eglDisplay;
	EGLContext			m_eglContext;
	EGLSurface			m_eglSurface;

	glw::Functions		m_gl;

	GLuint				m_coordBuffer;
	GLuint				m_indexBuffer;
	GLuint				m_texture;
	GLuint				m_program;

	EGLImageKHR			m_eglImage;

	GLuint				m_coordLoc;
	GLuint				m_textureLoc;

	vector<float>		m_coordData;
	vector<deUint16>	m_indexData;

	EGLImageKHR			createEGLImage			(void);
	GLuint				createTextureFromImage	(EGLImageKHR image);

	// Not supported
	TestContext&		operator=				(const TestContext&);
						TestContext				(const TestContext&);
};

namespace
{

void createCoordData (vector<float>& data, const TestConfig& config)
{
	if (config.useIndices)
	{
		for (int triangleNdx = 0; triangleNdx < 2; triangleNdx++)
		{
			const float x1 = -1.0f;
			const float y1 = -1.0f;

			const float x2 = 1.0f;
			const float y2 = 1.0f;

			const float side = ((triangleNdx % 2) == 0 ? 1.0f : -1.0f);

			data.push_back(side * x1);
			data.push_back(side * y1);

			data.push_back(side * x2);
			data.push_back(side * y1);

			data.push_back(side * x2);
			data.push_back(side * y2);
		}
	}
	else
	{
		data.reserve(config.triangleCount * 3 * 2);

		for (int triangleNdx = 0; triangleNdx < config.triangleCount; triangleNdx++)
		{
			const float x1 = -1.0f;
			const float y1 = -1.0f;

			const float x2 = 1.0f;
			const float y2 = 1.0f;

			const float side = ((triangleNdx % 2) == 0 ? 1.0f : -1.0f);

			data.push_back(side * x1);
			data.push_back(side * y1);

			data.push_back(side * x2);
			data.push_back(side * y1);

			data.push_back(side * x2);
			data.push_back(side * y2);
		}
	}
}

GLuint createCoordBuffer (const glw::Functions& gl, const TestConfig& config)
{
	GLuint			buffer;
	vector<float>	data;

	createCoordData(data, config);

	gl.genBuffers(1, &buffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers()");
	gl.bindBuffer(GL_ARRAY_BUFFER, buffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer()");
	gl.bufferData(GL_ARRAY_BUFFER, (GLsizei)(data.size() * sizeof(float)), &(data[0]), GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData()");
	gl.bindBuffer(GL_ARRAY_BUFFER, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer()");

	return buffer;
}

void createIndexData (vector<deUint16>& data, const TestConfig& config)
{
	for (int triangleNdx = 0; triangleNdx < config.triangleCount; triangleNdx++)
	{
		if ((triangleNdx % 2) == 0)
		{
			data.push_back(0);
			data.push_back(1);
			data.push_back(2);
		}
		else
		{
			data.push_back(2);
			data.push_back(3);
			data.push_back(0);
		}
	}
}

GLuint createIndexBuffer (const glw::Functions& gl, const TestConfig& config)
{
	GLuint				buffer;
	vector<deUint16>	data;

	createIndexData(data, config);

	gl.genBuffers(1, &buffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers()");
	gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer()");
	gl.bufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizei)(data.size() * sizeof(deUint16)), &(data[0]), GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData()");
	gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer()");

	return buffer;
}

void createTextureData (vector<deUint8>& data, const TestConfig& config)
{
	for (int x = 0; x < config.textureWidth; x++)
	{
		for (int y = 0; y < config.textureHeight; y++)
		{
			data.push_back((deUint8)((255*x)/255));
			data.push_back((deUint8)((255*y)/255));
			data.push_back((deUint8)((255*x*y)/(255*255)));
			data.push_back(255);
		}
	}
}

GLuint createTexture (const glw::Functions& gl, const TestConfig& config)
{
	GLuint			texture;
	vector<deUint8>	data;

	createTextureData(data, config);

	gl.genTextures(1, &texture);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures()");
	gl.bindTexture(GL_TEXTURE_2D, texture);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture()");
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri");
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri");
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri");
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri");
	gl.texImage2D(GL_TEXTURE_2D, 0, GL_RGBA, config.textureWidth, config.textureWidth, 0, GL_RGBA, GL_UNSIGNED_BYTE, &(data[0]));
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage2D()");
	gl.bindTexture(GL_TEXTURE_2D, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture()");

	return texture;
}

GLuint createProgram (const glw::Functions& gl, const TestConfig& config)
{
	GLuint	vertexShader	= gl.createShader(GL_VERTEX_SHADER);
	GLuint	fragmentShader	= gl.createShader(GL_FRAGMENT_SHADER);

	if (config.useTexture)
	{
		const char* vertexShaderSource =
		"attribute mediump vec2 a_coord;\n"
		"varying mediump vec2 v_texCoord;\n"
		"void main(void)\n"
		"{\n"
		"\tv_texCoord = 0.5 * a_coord + vec2(0.5);\n"
		"\tgl_Position = vec4(a_coord, 0.0, 1.0);\n"
		"}\n";

		const char* fragmentShaderSource =
		"uniform sampler2D u_sampler;\n"
		"varying mediump vec2 v_texCoord;\n"
		"void main(void)\n"
		"{\n"
		"\tgl_FragColor = texture2D(u_sampler, v_texCoord);\n"
		"}\n";

		gl.shaderSource(vertexShader, 1, &vertexShaderSource, DE_NULL);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource()");
		gl.shaderSource(fragmentShader, 1, &fragmentShaderSource, DE_NULL);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource()");
	}
	else
	{
		const char* vertexShaderSource =
		"attribute mediump vec2 a_coord;\n"
		"varying mediump vec4 v_color;\n"
		"void main(void)\n"
		"{\n"
		"\tv_color = vec4(0.5 * a_coord + vec2(0.5), 0.5, 1.0);\n"
		"\tgl_Position = vec4(a_coord, 0.0, 1.0);\n"
		"}\n";

		const char* fragmentShaderSource =
		"varying mediump vec4 v_color;\n"
		"void main(void)\n"
		"{\n"
		"\tgl_FragColor = v_color;\n"
		"}\n";

		gl.shaderSource(vertexShader, 1, &vertexShaderSource, DE_NULL);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource()");
		gl.shaderSource(fragmentShader, 1, &fragmentShaderSource, DE_NULL);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource()");
	}

	gl.compileShader(vertexShader);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader()");
	gl.compileShader(fragmentShader);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader()");

	{
		GLint status;

		gl.getShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv()");

		if (!status)
		{
			string	log;
			GLint length;

			gl.getShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &length);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv()");
			log.resize(length, 0);

			gl.getShaderInfoLog(vertexShader, (GLsizei)log.size(), &length, &(log[0]));
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderInfoLog()");

			throw std::runtime_error(log.c_str());
		}
	}

	{
		GLint status;

		gl.getShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv()");

		if (!status)
		{
			string	log;
			GLint length;

			gl.getShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &length);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv()");
			log.resize(length, 0);

			gl.getShaderInfoLog(fragmentShader, (GLsizei)log.size(), &length, &(log[0]));
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderInfoLog()");

			throw std::runtime_error(log.c_str());
		}
	}

	{
		GLuint program = gl.createProgram();

		gl.attachShader(program, vertexShader);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader()");
		gl.attachShader(program, fragmentShader);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader()");

		gl.linkProgram(program);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram()");

		{
			GLint status;

			gl.getProgramiv(program, GL_LINK_STATUS, &status);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv()");

			if (!status)
			{
				string	log;
				GLsizei	length;

				gl.getProgramInfoLog(program, 0, &length, DE_NULL);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramInfoLog()");
				log.resize(length, 0);

				gl.getProgramInfoLog(program, (GLsizei)log.size(), &length, &(log[0]));
				GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramInfoLog()");

				throw std::runtime_error(log.c_str());
			}
		}

		gl.deleteShader(vertexShader);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteShader()");
		gl.deleteShader(fragmentShader);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteShader()");

		return program;
	}
}

EGLContext createEGLContext (EglTestContext& testCtx, EGLDisplay eglDisplay, EGLConfig eglConfig, EGLContext share)
{
	const Library&	egl				= testCtx.getLibrary();
	const EGLint	attribList[]	=
	{
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};

	EGLU_CHECK_CALL(egl, bindAPI(EGL_OPENGL_ES_API));

	EGLContext context = egl.createContext(eglDisplay, eglConfig, share, attribList);
	EGLU_CHECK_MSG(egl, "eglCreateContext()");

	return context;
}

EGLSurface createEGLSurface (EglTestContext& testCtx, EGLDisplay display, EGLConfig eglConfig, const TestConfig& config)
{
	const Library&	egl				= testCtx.getLibrary();
	const EGLint	attribList[]	=
	{
		EGL_WIDTH,	config.surfaceWidth,
		EGL_HEIGHT, config.surfaceHeight,
		EGL_NONE
	};

	EGLSurface surface = egl.createPbufferSurface(display, eglConfig, attribList);
	EGLU_CHECK_MSG(egl, "eglCreatePbufferSurface()");

	return surface;
}

} // anonymous

TestContext::TestContext (EglTestContext& testCtx, EGLDisplay eglDisplay, EGLConfig eglConfig, const TestConfig& config, bool share, TestContext* parent)
	: m_parent				(parent)
	, m_testCtx				(testCtx)
	, m_config				(config)
	, m_eglDisplay			(eglDisplay)
	, m_eglContext			(EGL_NO_CONTEXT)
	, m_eglSurface			(EGL_NO_SURFACE)
	, m_coordBuffer			(0)
	, m_indexBuffer			(0)
	, m_texture				(0)
	, m_program				(0)
	, m_eglImage			(EGL_NO_IMAGE_KHR)
{
	const Library&	egl	= m_testCtx.getLibrary();

	if (m_config.textureType == TestConfig::TEXTURETYPE_IMAGE
		|| m_config.textureType == TestConfig::TEXTURETYPE_SHARED_IMAGE
		|| m_config.textureType == TestConfig::TEXTURETYPE_SHARED_IMAGE_TEXTURE)
	{
		const vector<string> extensions = eglu::getDisplayExtensions(egl, m_eglDisplay);

		if (!de::contains(extensions.begin(), extensions.end(), "EGL_KHR_image_base") ||
			!de::contains(extensions.begin(), extensions.end(), "EGL_KHR_gl_texture_2D_image"))
			TCU_THROW(NotSupportedError, "EGL_KHR_image_base extensions not supported");
	}

	m_eglContext = createEGLContext(m_testCtx, m_eglDisplay, eglConfig, (share && parent ? parent->getEGLContext() : EGL_NO_CONTEXT));
	m_eglSurface = createEGLSurface(m_testCtx, m_eglDisplay, eglConfig, config);

	EGLU_CHECK_CALL(egl, makeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_eglContext));

	{
		const char* reqExts[] = { "GL_OES_EGL_image" };
		m_testCtx.initGLFunctions(&m_gl, glu::ApiType::es(2,0), DE_LENGTH_OF_ARRAY(reqExts), reqExts);
	}

	if (m_config.textureType == TestConfig::TEXTURETYPE_IMAGE
		|| m_config.textureType == TestConfig::TEXTURETYPE_SHARED_IMAGE
		|| m_config.textureType == TestConfig::TEXTURETYPE_SHARED_IMAGE_TEXTURE)
	{
		vector<string> glExts = de::splitString((const char*)m_gl.getString(GL_EXTENSIONS), ' ');

		if (!de::contains(glExts.begin(), glExts.end(), "GL_OES_EGL_image"))
			TCU_THROW(NotSupportedError, "GL_OES_EGL_image extensions not supported");

		TCU_CHECK(m_gl.eglImageTargetTexture2DOES);
	}

	if (m_config.useCoordBuffer && (!m_config.sharedCoordBuffer || !parent))
		m_coordBuffer = createCoordBuffer(m_gl, m_config);
	else if (m_config.useCoordBuffer && m_config.sharedCoordBuffer)
		m_coordBuffer = parent->getCoordBuffer();
	else
		createCoordData(m_coordData, m_config);

	if (m_config.useIndexBuffer && (!m_config.sharedIndexBuffer || !parent))
		m_indexBuffer = createIndexBuffer(m_gl, m_config);
	else if (m_config.useIndexBuffer && m_config.sharedIndexBuffer)
		m_indexBuffer = parent->getIndexBuffer();
	else if (m_config.useIndices)
		createIndexData(m_indexData, m_config);

	if (m_config.useTexture)
	{
		if (m_config.textureType == TestConfig::TEXTURETYPE_TEXTURE)
			m_texture = createTexture(m_gl, m_config);
		else if (m_config.textureType == TestConfig::TEXTURETYPE_SHARED_TEXTURE)
		{
			if (parent)
				m_texture = parent->getTexture();
			else
				m_texture = createTexture(m_gl, m_config);
		}
		else if (m_config.textureType == TestConfig::TEXTURETYPE_IMAGE)
		{
			m_eglImage	= createEGLImage();
			m_texture	= createTextureFromImage(m_eglImage);
		}
		else if (m_config.textureType == TestConfig::TEXTURETYPE_SHARED_IMAGE)
		{
			if (parent)
				m_eglImage = parent->getEGLImage();
			else
				m_eglImage = createEGLImage();

			m_texture = createTextureFromImage(m_eglImage);
		}
		else if (m_config.textureType == TestConfig::TEXTURETYPE_SHARED_IMAGE_TEXTURE)
		{
			if (parent)
				m_texture = parent->getTexture();
			else
			{
				m_eglImage	= createEGLImage();
				m_texture	= createTextureFromImage(m_eglImage);
			}
		}
	}

	if (!m_config.sharedProgram || !parent)
		m_program = createProgram(m_gl, m_config);
	else if (m_config.sharedProgram)
		m_program = parent->getProgram();

	m_coordLoc = m_gl.getAttribLocation(m_program, "a_coord");

	if (m_config.useTexture)
		m_textureLoc = m_gl.getUniformLocation(m_program, "u_sampler");

	EGLU_CHECK_CALL(egl, makeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
}

EGLImageKHR TestContext::createEGLImage (void)
{
	GLuint sourceTexture = createTexture(m_gl, m_config);

	try
	{
		const Library&	egl				= m_testCtx.getLibrary();
		const EGLint	attribList[]	=
		{
			EGL_GL_TEXTURE_LEVEL_KHR, 0,
			EGL_NONE
		};

		EGLImageKHR image = egl.createImageKHR(m_eglDisplay, m_eglContext, EGL_GL_TEXTURE_2D_KHR, (EGLClientBuffer)(deUintptr)sourceTexture, attribList);
		EGLU_CHECK_MSG(egl, "eglCreateImageKHR()");

		m_gl.deleteTextures(1, &sourceTexture);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "eglCreateImageKHR()");

		return image;
	}
	catch (...)
	{
		m_gl.deleteTextures(1, &sourceTexture);
		throw;
	}
}

GLuint TestContext::createTextureFromImage (EGLImageKHR image)
{
	GLuint texture = 0;

	try
	{
		m_gl.genTextures(1, &texture);
		m_gl.bindTexture(GL_TEXTURE_2D, texture);
		m_gl.eglImageTargetTexture2DOES(GL_TEXTURE_2D, image);
		m_gl.bindTexture(GL_TEXTURE_2D, 0);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "Creating texture from image");

		return texture;
	}
	catch (...)
	{
		m_gl.deleteTextures(1, &texture);
		throw;
	}
}


TestContext::~TestContext (void)
{
	const Library&	egl	= m_testCtx.getLibrary();

	EGLU_CHECK_CALL(egl, makeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_eglContext));

	if (m_parent == DE_NULL && m_eglImage)
		EGLU_CHECK_CALL(egl, destroyImageKHR(m_eglDisplay, m_eglImage));

	EGLU_CHECK_CALL(egl, makeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
	EGLU_CHECK_CALL(egl, destroyContext(m_eglDisplay, m_eglContext));
	EGLU_CHECK_CALL(egl, destroySurface(m_eglDisplay, m_eglSurface));
}

void TestContext::render (void)
{
	const Library&	egl	= m_testCtx.getLibrary();

	egl.makeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_eglContext);

	for (int frameNdx = 0; frameNdx < m_config.frameCount; frameNdx++)
	{
		m_gl.clearColor(0.75f, 0.6f, 0.5f, 1.0f);
		m_gl.clear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		for (int callNdx = 0; callNdx < m_config.drawCallCount; callNdx++)
		{
			m_gl.useProgram(m_program);
			m_gl.enableVertexAttribArray(m_coordLoc);

			if (m_config.useCoordBuffer)
			{
				m_gl.bindBuffer(GL_ARRAY_BUFFER, m_coordBuffer);
				m_gl.vertexAttribPointer(m_coordLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);
				m_gl.bindBuffer(GL_ARRAY_BUFFER, 0);
			}
			else
				m_gl.vertexAttribPointer(m_coordLoc, 2, GL_FLOAT, GL_FALSE, 0, &(m_coordData[0]));

			if (m_config.useTexture)
			{
				m_gl.bindTexture(GL_TEXTURE_2D, m_texture);
				m_gl.uniform1i(m_textureLoc, 0);
			}

			if (m_config.useIndices)
			{
				if (m_config.useIndexBuffer)
				{
					m_gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
					m_gl.drawElements(GL_TRIANGLES, m_config.triangleCount, GL_UNSIGNED_SHORT, 0);
				}
				else
					m_gl.drawElements(GL_TRIANGLES, m_config.triangleCount, GL_UNSIGNED_SHORT, &(m_indexData[0]));
			}
			else
				m_gl.drawArrays(GL_TRIANGLES, 0, m_config.triangleCount);


			if (m_config.useTexture)
				m_gl.bindTexture(GL_TEXTURE_2D, 0);

			m_gl.disableVertexAttribArray(m_coordLoc);

			m_gl.useProgram(0);
		}


		egl.swapBuffers(m_eglDisplay, m_eglSurface);
	}

	m_gl.finish();
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glFinish()");
	EGLU_CHECK_CALL(egl, makeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
}

class TestThread : de::Thread
{
public:
					TestThread		(const vector<TestContext*> contexts, const Library& egl);
					~TestThread		(void);

	void			start			(void);
	void			join			(void);

	bool			resultOk		(void) { return m_isOk; }

private:
	vector<TestContext*>	m_contexts;
	const Library&			m_egl;
	bool					m_isOk;
	string					m_errorString;

	deUint64				m_beginTimeUs;
	deUint64				m_endTimeUs;

	deUint64				m_joinBeginUs;
	deUint64				m_joinEndUs;

	deUint64				m_startBeginUs;
	deUint64				m_startEndUs;


	virtual void	run			(void);

	TestThread&		operator=	(const TestThread&);
					TestThread	(const TestThread&);
};

TestThread::TestThread (const vector<TestContext*> contexts, const Library& egl)
	: m_contexts		(contexts)
	, m_egl				(egl)
	, m_isOk			(false)
	, m_errorString		("")
	, m_beginTimeUs		(0)
	, m_endTimeUs		(0)
	, m_joinBeginUs		(0)
	, m_joinEndUs		(0)
	, m_startBeginUs	(0)
	, m_startEndUs		(0)
{
}

TestThread::~TestThread (void)
{
	m_contexts.clear();
}

void TestThread::start (void)
{
	m_startBeginUs = deGetMicroseconds();
	de::Thread::start();
	m_startEndUs = deGetMicroseconds();
}

void TestThread::join (void)
{
	m_joinBeginUs = deGetMicroseconds();
	de::Thread::join();
	m_joinEndUs = deGetMicroseconds();
}

void TestThread::run (void)
{
	try
	{
		m_beginTimeUs = deGetMicroseconds();

		for (int contextNdx = 0; contextNdx < (int)m_contexts.size(); contextNdx++)
			m_contexts[contextNdx]->render();

		m_isOk		= true;
		m_endTimeUs = deGetMicroseconds();
	}
	catch (const std::runtime_error& error)
	{
		m_isOk			= false;
		m_errorString	= error.what();
	}
	catch (...)
	{
		m_isOk			= false;
		m_errorString	= "Got unknown exception";
	}

	m_egl.releaseThread();
}

class SharedRenderingPerfCase : public TestCase
{
public:
								SharedRenderingPerfCase		(EglTestContext& eglTestCtx, const TestConfig& config, const char* name, const char* description);
								~SharedRenderingPerfCase	(void);

	void						init						(void);
	void						deinit						(void);
	IterateResult				iterate						(void);

private:
	TestConfig					m_config;
	const int					m_iterationCount;

	EGLDisplay					m_display;
	vector<TestContext*>		m_contexts;
	vector<deUint64>			m_results;

	SharedRenderingPerfCase&	operator=					(const SharedRenderingPerfCase&);
								SharedRenderingPerfCase		(const SharedRenderingPerfCase&);
};

SharedRenderingPerfCase::SharedRenderingPerfCase (EglTestContext& eglTestCtx, const TestConfig& config, const char* name, const char* description)
	: TestCase			(eglTestCtx, tcu::NODETYPE_PERFORMANCE, name, description)
	, m_config			(config)
	, m_iterationCount	(30)
	, m_display			(EGL_NO_DISPLAY)
{
}

SharedRenderingPerfCase::~SharedRenderingPerfCase (void)
{
	deinit();
}

void SharedRenderingPerfCase::init (void)
{
	m_display = eglu::getAndInitDisplay(m_eglTestCtx.getNativeDisplay());

	{
		const Library&	egl				= m_eglTestCtx.getLibrary();
		const EGLint	attribList[]	=
		{
			EGL_SURFACE_TYPE,		EGL_PBUFFER_BIT,
			EGL_RENDERABLE_TYPE,	EGL_OPENGL_ES2_BIT,
			EGL_NONE
		};
		EGLConfig		eglConfig		= eglu::chooseSingleConfig(egl, m_display, attribList);

		// Create contexts and resources
		for (int threadNdx = 0; threadNdx < m_config.threadCount * m_config.perThreadContextCount; threadNdx++)
			m_contexts.push_back(new TestContext(m_eglTestCtx, m_display, eglConfig, m_config, m_config.sharedContexts, (threadNdx == 0 ? DE_NULL : m_contexts[threadNdx-1])));
	}
}

void SharedRenderingPerfCase::deinit (void)
{
	// Destroy resources and contexts
	for (int threadNdx = 0; threadNdx < (int)m_contexts.size(); threadNdx++)
	{
		delete m_contexts[threadNdx];
		m_contexts[threadNdx] = DE_NULL;
	}

	m_contexts.clear();
	m_results.clear();

	if (m_display != EGL_NO_DISPLAY)
	{
		m_eglTestCtx.getLibrary().terminate(m_display);
		m_display = EGL_NO_DISPLAY;
	}
}

namespace
{

void createThreads (vector<TestThread*>& threads, int threadCount, int perThreadContextCount, vector<TestContext*>& contexts, const Library& egl)
{
	DE_ASSERT(threadCount * perThreadContextCount == (int)contexts.size());
	DE_ASSERT(threads.empty());

	vector<TestContext*> threadContexts;

	for (int threadNdx = 0; threadNdx < threadCount; threadNdx++)
	{
		for (int contextNdx = 0; contextNdx < perThreadContextCount; contextNdx++)
			threadContexts.push_back(contexts[threadNdx * perThreadContextCount + contextNdx]);

		threads.push_back(new TestThread(threadContexts, egl));

		threadContexts.clear();
	}
}

void destroyThreads (vector<TestThread*>& threads)
{
	for (int threadNdx = 0; threadNdx < (int)threads.size(); threadNdx++)
	{
		delete threads[threadNdx];
		threads[threadNdx] = DE_NULL;
	}

	threads.clear();
}

void startThreads (vector<TestThread*>& threads)
{
	for (int threadNdx = 0; threadNdx < (int)threads.size(); threadNdx++)
		threads[threadNdx]->start();
}

void joinThreads (vector<TestThread*>& threads)
{
	for (int threadNdx = 0; threadNdx < (int)threads.size(); threadNdx++)
		threads[threadNdx]->join();
}

bool threadResultsOk (const vector<TestThread*>& threads)
{
	for (int threadNdx = 0; threadNdx < (int)threads.size(); threadNdx++)
	{
		if (!threads[threadNdx]->resultOk())
			return false;
	}

	return true;
}

void logAndSetResults (tcu::TestContext& testCtx, const vector<deUint64>& r)
{
	TestLog& log		= testCtx.getLog();
	vector<deUint64>	resultsUs = r;
	deUint64			sum = 0;
	deUint64			average;
	deUint64			median;
	double				deviation;

	log << TestLog::SampleList("Result", "Result")
		<< TestLog::SampleInfo << TestLog::ValueInfo("Time", "Time", "us", QP_SAMPLE_VALUE_TAG_RESPONSE)
		<< TestLog::EndSampleInfo;

	for (int resultNdx = 0; resultNdx < (int)resultsUs.size(); resultNdx++)
		log << TestLog::Sample << deInt64(resultsUs[resultNdx]) << TestLog::EndSample;

	log << TestLog::EndSampleList;

	std::sort(resultsUs.begin(), resultsUs.end());

	for (int resultNdx = 0; resultNdx < (int)resultsUs.size(); resultNdx++)
		sum += resultsUs[resultNdx];

	average	= sum / resultsUs.size();
	median	= resultsUs[resultsUs.size() / 2];

	deviation = 0.0;
	for (int resultNdx = 0; resultNdx < (int)resultsUs.size(); resultNdx++)
		deviation += (double)((resultsUs[resultNdx] - average) * (resultsUs[resultNdx] - average));

	deviation = std::sqrt(deviation/(double)resultsUs.size());

	{
		tcu::ScopedLogSection	section(log, "Statistics from results", "Statistics from results");

		log << TestLog::Message
		<< "Average: "					<< ((double)average/1000.0)											<< "ms\n"
		<< "Standard deviation: "		<< ((double)deviation/1000.0)										<< "ms\n"
		<< "Standard error of mean: "	<< (((double)deviation/std::sqrt((double)resultsUs.size()))/1000.0)	<< "ms\n"
		<< "Median: "					<< ((double)median/1000.0)											<< "ms\n"
		<< TestLog::EndMessage;
	}

	testCtx.setTestResult(QP_TEST_RESULT_PASS, de::floatToString((float)((double)average/1000.0), 2).c_str());
}

void logTestConfig (TestLog& log, const TestConfig& config)
{
	tcu::ScopedLogSection threadSection(log, "Test info", "Test information");

	log << TestLog::Message << "Total triangles rendered: : "							<< config.triangleCount * config.drawCallCount * config.frameCount * config.perThreadContextCount * config.threadCount << TestLog::EndMessage;
	log << TestLog::Message << "Number of threads: "									<< config.threadCount << TestLog::EndMessage;
	log << TestLog::Message << "Number of contexts used to render with each thread: "	<< config.perThreadContextCount << TestLog::EndMessage;
	log << TestLog::Message << "Number of frames rendered with each context: "			<< config.frameCount << TestLog::EndMessage;
	log << TestLog::Message << "Number of draw calls performed by each frame: "			<< config.drawCallCount << TestLog::EndMessage;
	log << TestLog::Message << "Number of triangles rendered by each draw call: "		<< config.triangleCount << TestLog::EndMessage;

	if (config.sharedContexts)
		log << TestLog::Message << "Shared contexts." << TestLog::EndMessage;
	else
		log << TestLog::Message << "No shared contexts." << TestLog::EndMessage;

	if (config.useCoordBuffer)
		log << TestLog::Message << (config.sharedCoordBuffer ? "Shared " : "") << "Coordinate buffer" << TestLog::EndMessage;
	else
		log << TestLog::Message << "Coordinates from pointer" << TestLog::EndMessage;

	if (config.useIndices)
		log << TestLog::Message << "Using glDrawElements with indices from " << (config.sharedIndexBuffer ? "shared " : "") << (config.useIndexBuffer ? "buffer." : "pointer.") << TestLog::EndMessage;

	if (config.useTexture)
	{
		if (config.textureType == TestConfig::TEXTURETYPE_TEXTURE)
			log << TestLog::Message << "Use texture." << TestLog::EndMessage;
		else if (config.textureType == TestConfig::TEXTURETYPE_SHARED_TEXTURE)
			log << TestLog::Message << "Use shared texture." << TestLog::EndMessage;
		else if (config.textureType == TestConfig::TEXTURETYPE_IMAGE)
			log << TestLog::Message << "Use texture created from EGLImage." << TestLog::EndMessage;
		else if (config.textureType == TestConfig::TEXTURETYPE_SHARED_IMAGE)
			log << TestLog::Message << "Use texture created from shared EGLImage." << TestLog::EndMessage;
		else if (config.textureType == TestConfig::TEXTURETYPE_SHARED_IMAGE_TEXTURE)
			log << TestLog::Message << "Use shared texture created from EGLImage." << TestLog::EndMessage;
		else
			DE_ASSERT(false);

		log << TestLog::Message << "Texture size: " << config.textureWidth << "x" << config.textureHeight << TestLog::EndMessage;
	}

	if (config.sharedProgram)
		log << TestLog::Message << "Shared program." << TestLog::EndMessage;

	log << TestLog::Message << "Surface size: " << config.surfaceWidth << "x" << config.surfaceHeight << TestLog::EndMessage;
}

} // anonymous

TestCase::IterateResult SharedRenderingPerfCase::iterate (void)
{
	deUint64			beginTimeUs;
	deUint64			endTimeUs;
	vector<TestThread*>	threads;

	if (m_results.empty())
		logTestConfig(m_testCtx.getLog(), m_config);

	createThreads(threads, m_config.threadCount, m_config.perThreadContextCount, m_contexts, m_eglTestCtx.getLibrary());

	beginTimeUs = deGetMicroseconds();

	startThreads(threads);
	joinThreads(threads);

	endTimeUs = deGetMicroseconds();

	if (!threadResultsOk(threads))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		return STOP;
	}

	destroyThreads(threads);

	m_results.push_back(endTimeUs - beginTimeUs);

	if ((int)m_results.size() == m_iterationCount)
	{
		logAndSetResults(m_testCtx, m_results);
		return STOP;
	}
	else
		return CONTINUE;
}

string createTestName(int threads, int perThreadContextCount)
{
	std::ostringstream stream;

	stream << threads << (threads == 1 ? "_thread_" : "_threads_") << perThreadContextCount << (perThreadContextCount == 1 ? "_context" : "_contexts");

	return stream.str();
}

} // anonymous

GLES2SharedRenderingPerfTests::GLES2SharedRenderingPerfTests (EglTestContext& eglTestCtx)
	: TestCaseGroup(eglTestCtx, "gles2_shared_render", "")
{
}

void GLES2SharedRenderingPerfTests::init (void)
{
	TestConfig basicConfig;

	basicConfig.threadCount					= 1;
	basicConfig.perThreadContextCount		= 1;

	basicConfig.sharedContexts				= true;
	basicConfig.frameCount					= 10;
	basicConfig.drawCallCount				= 10;
	basicConfig.triangleCount				= 100;

	basicConfig.useCoordBuffer				= true;
	basicConfig.sharedCoordBuffer			= false;

	basicConfig.useIndices					= true;
	basicConfig.useIndexBuffer				= true;
	basicConfig.sharedIndexBuffer			= false;

	basicConfig.useTexture					= true;
	basicConfig.textureType					= TestConfig::TEXTURETYPE_TEXTURE;

	basicConfig.sharedProgram				= false;

	basicConfig.textureWidth				= 128;
	basicConfig.textureHeight				= 128;

	basicConfig.surfaceWidth				= 256;
	basicConfig.surfaceHeight				= 256;

	const int	threadCounts[]				= { 1, 2, 4 };
	const int	perThreadContextCounts[]	= { 1, 2, 4 };

	// Add no sharing tests
	{
		TestCaseGroup* sharedNoneGroup = new TestCaseGroup(m_eglTestCtx, "no_shared_context", "Tests without sharing contexts.");

		for (int threadCountNdx = 0; threadCountNdx < DE_LENGTH_OF_ARRAY(threadCounts); threadCountNdx++)
		{
			int threadCount = threadCounts[threadCountNdx];

			for (int contextCountNdx = 0; contextCountNdx < DE_LENGTH_OF_ARRAY(perThreadContextCounts); contextCountNdx++)
			{
				int contextCount = perThreadContextCounts[contextCountNdx];

				if (threadCount * contextCount != 4 && threadCount * contextCount != 1)
					continue;

				TestConfig config				= basicConfig;
				config.threadCount				= threadCount;
				config.perThreadContextCount	= contextCount;
				config.sharedContexts			= false;

				{
					TestConfig smallConfig		= config;
					smallConfig.triangleCount	= 1;
					smallConfig.drawCallCount	= 1000;
					smallConfig.frameCount		= 10;

					if (threadCount * contextCount == 1)
						smallConfig.frameCount *= 4;

					sharedNoneGroup->addChild(new SharedRenderingPerfCase(m_eglTestCtx, smallConfig, (createTestName(threadCount, contextCount) + "_small_call").c_str(), ""));
				}

				{
					TestConfig bigConfig	= config;
					bigConfig.triangleCount	= 1000;
					bigConfig.drawCallCount	= 1;
					bigConfig.frameCount	= 10;

					if (threadCount * contextCount == 1)
						bigConfig.frameCount *= 4;

					sharedNoneGroup->addChild(new SharedRenderingPerfCase(m_eglTestCtx, bigConfig, (createTestName(threadCount, contextCount) + "_big_call").c_str(), ""));
				}
			}
		}

		addChild(sharedNoneGroup);
	}

	// Add no resource sharing tests
	{
		TestCaseGroup* sharedNoneGroup = new TestCaseGroup(m_eglTestCtx, "no_shared_resource", "Tests without shared resources.");

		for (int threadCountNdx = 0; threadCountNdx < DE_LENGTH_OF_ARRAY(threadCounts); threadCountNdx++)
		{
			int threadCount = threadCounts[threadCountNdx];

			for (int contextCountNdx = 0; contextCountNdx < DE_LENGTH_OF_ARRAY(perThreadContextCounts); contextCountNdx++)
			{
				int contextCount = perThreadContextCounts[contextCountNdx];

				if (threadCount * contextCount != 4 && threadCount * contextCount != 1)
					continue;

				TestConfig config				= basicConfig;
				config.threadCount				= threadCount;
				config.perThreadContextCount	= contextCount;

				{
					TestConfig smallConfig		= config;
					smallConfig.triangleCount	= 1;
					smallConfig.drawCallCount	= 1000;
					smallConfig.frameCount		= 10;

					if (threadCount * contextCount == 1)
						smallConfig.frameCount *= 4;

					sharedNoneGroup->addChild(new SharedRenderingPerfCase(m_eglTestCtx, smallConfig, (createTestName(threadCount, contextCount) + "_small_call").c_str(), ""));
				}

				{
					TestConfig bigConfig	= config;
					bigConfig.triangleCount	= 1000;
					bigConfig.drawCallCount	= 1;
					bigConfig.frameCount	= 10;

					if (threadCount * contextCount == 1)
						bigConfig.frameCount *= 4;

					sharedNoneGroup->addChild(new SharedRenderingPerfCase(m_eglTestCtx, bigConfig, (createTestName(threadCount, contextCount) + "_big_call").c_str(), ""));
				}
			}
		}

		addChild(sharedNoneGroup);
	}

	// Add shared coord buffer tests
	{
		TestCaseGroup* sharedCoordBufferGroup = new TestCaseGroup(m_eglTestCtx, "shared_coord_buffer", "Shared coordinate bufffer");

		for (int threadCountNdx = 0; threadCountNdx < DE_LENGTH_OF_ARRAY(threadCounts); threadCountNdx++)
		{
			int threadCount = threadCounts[threadCountNdx];

			for (int contextCountNdx = 0; contextCountNdx < DE_LENGTH_OF_ARRAY(perThreadContextCounts); contextCountNdx++)
			{
				int contextCount = perThreadContextCounts[contextCountNdx];

				if (threadCount * contextCount != 4 && threadCount * contextCount != 1)
					continue;

				TestConfig config				= basicConfig;
				config.sharedCoordBuffer		= true;
				config.threadCount				= threadCount;
				config.perThreadContextCount	= contextCount;

				{
					TestConfig smallConfig		= config;
					smallConfig.triangleCount	= 1;
					smallConfig.drawCallCount	= 1000;
					smallConfig.frameCount		= 10;

					if (threadCount * contextCount == 1)
						smallConfig.frameCount *= 4;

					sharedCoordBufferGroup->addChild(new SharedRenderingPerfCase(m_eglTestCtx, smallConfig, (createTestName(threadCount, contextCount) + "_small_call").c_str(), ""));
				}

				{
					TestConfig bigConfig	= config;
					bigConfig.triangleCount	= 1000;
					bigConfig.drawCallCount	= 1;
					bigConfig.frameCount	= 10;

					if (threadCount * contextCount == 1)
						bigConfig.frameCount *= 4;

					sharedCoordBufferGroup->addChild(new SharedRenderingPerfCase(m_eglTestCtx, bigConfig, (createTestName(threadCount, contextCount) + "_big_call").c_str(), ""));
				}
			}
		}

		addChild(sharedCoordBufferGroup);
	}

	// Add shared index buffer tests
	{
		TestCaseGroup* sharedIndexBufferGroup = new TestCaseGroup(m_eglTestCtx, "shared_index_buffer", "Shared index bufffer");

		for (int threadCountNdx = 0; threadCountNdx < DE_LENGTH_OF_ARRAY(threadCounts); threadCountNdx++)
		{
			int threadCount = threadCounts[threadCountNdx];

			for (int contextCountNdx = 0; contextCountNdx < DE_LENGTH_OF_ARRAY(perThreadContextCounts); contextCountNdx++)
			{
				int contextCount = perThreadContextCounts[contextCountNdx];

				if (threadCount * contextCount != 4 && threadCount * contextCount != 1)
					continue;

				TestConfig config				= basicConfig;
				config.sharedIndexBuffer		= true;
				config.threadCount				= threadCount;
				config.perThreadContextCount	= contextCount;

				{
					TestConfig smallConfig		= config;
					smallConfig.triangleCount	= 1;
					smallConfig.drawCallCount	= 1000;
					smallConfig.frameCount		= 10;

					if (threadCount * contextCount == 1)
						smallConfig.frameCount *= 4;

					sharedIndexBufferGroup->addChild(new SharedRenderingPerfCase(m_eglTestCtx, smallConfig, (createTestName(threadCount, contextCount) + "_small_call").c_str(), ""));
				}

				{
					TestConfig bigConfig	= config;
					bigConfig.triangleCount	= 1000;
					bigConfig.drawCallCount	= 1;
					bigConfig.frameCount	= 10;

					if (threadCount * contextCount == 1)
						bigConfig.frameCount *= 4;

					sharedIndexBufferGroup->addChild(new SharedRenderingPerfCase(m_eglTestCtx, bigConfig, (createTestName(threadCount, contextCount) + "_big_call").c_str(), ""));
				}
			}
		}

		addChild(sharedIndexBufferGroup);
	}

	// Add shared texture tests
	{
		TestCaseGroup* sharedTextureGroup = new TestCaseGroup(m_eglTestCtx, "shared_texture", "Shared texture tests.");

		for (int threadCountNdx = 0; threadCountNdx < DE_LENGTH_OF_ARRAY(threadCounts); threadCountNdx++)
		{
			int threadCount = threadCounts[threadCountNdx];

			for (int contextCountNdx = 0; contextCountNdx < DE_LENGTH_OF_ARRAY(perThreadContextCounts); contextCountNdx++)
			{
				int contextCount = perThreadContextCounts[contextCountNdx];

				if (threadCount * contextCount != 4 && threadCount * contextCount != 1)
					continue;

				TestConfig config				= basicConfig;
				config.textureType				= TestConfig::TEXTURETYPE_SHARED_TEXTURE;
				config.threadCount				= threadCount;
				config.perThreadContextCount	= contextCount;

				{
					TestConfig smallConfig		= config;
					smallConfig.triangleCount	= 1;
					smallConfig.drawCallCount	= 1000;
					smallConfig.frameCount		= 10;

					if (threadCount * contextCount == 1)
						smallConfig.frameCount *= 4;

					sharedTextureGroup->addChild(new SharedRenderingPerfCase(m_eglTestCtx, smallConfig, (createTestName(threadCount, contextCount) + "_small_call").c_str(), ""));
				}

				{
					TestConfig bigConfig	= config;
					bigConfig.triangleCount	= 1000;
					bigConfig.drawCallCount	= 1;
					bigConfig.frameCount	= 10;

					if (threadCount * contextCount == 1)
						bigConfig.frameCount *= 4;

					sharedTextureGroup->addChild(new SharedRenderingPerfCase(m_eglTestCtx, bigConfig, (createTestName(threadCount, contextCount) + "_big_call").c_str(), ""));
				}
			}
		}

		addChild(sharedTextureGroup);
	}

	// Add shared program tests
	{
		TestCaseGroup* sharedProgramGroup = new TestCaseGroup(m_eglTestCtx, "shared_program", "Shared program tests.");

		for (int threadCountNdx = 0; threadCountNdx < DE_LENGTH_OF_ARRAY(threadCounts); threadCountNdx++)
		{
			int threadCount = threadCounts[threadCountNdx];

			for (int contextCountNdx = 0; contextCountNdx < DE_LENGTH_OF_ARRAY(perThreadContextCounts); contextCountNdx++)
			{
				int contextCount = perThreadContextCounts[contextCountNdx];

				if (threadCount * contextCount != 4 && threadCount * contextCount != 1)
					continue;

				TestConfig config				= basicConfig;
				config.sharedProgram			= true;
				config.threadCount				= threadCount;
				config.perThreadContextCount	= contextCount;

				{
					TestConfig smallConfig		= config;
					smallConfig.triangleCount	= 1;
					smallConfig.drawCallCount	= 1000;
					smallConfig.frameCount		= 10;

					if (threadCount * contextCount == 1)
						smallConfig.frameCount *= 4;

					sharedProgramGroup->addChild(new SharedRenderingPerfCase(m_eglTestCtx, smallConfig, (createTestName(threadCount, contextCount) + "_small_call").c_str(), ""));
				}

				{
					TestConfig bigConfig	= config;
					bigConfig.triangleCount	= 1000;
					bigConfig.drawCallCount	= 1;
					bigConfig.frameCount	= 10;

					if (threadCount * contextCount == 1)
						bigConfig.frameCount *= 4;

					sharedProgramGroup->addChild(new SharedRenderingPerfCase(m_eglTestCtx, bigConfig, (createTestName(threadCount, contextCount) + "_big_call").c_str(), ""));
				}
			}
		}

		addChild(sharedProgramGroup);
	}

	// Add shared all tests
	{
		TestCaseGroup* sharedallGroup = new TestCaseGroup(m_eglTestCtx, "shared_all", "Share all possible resources.");

		for (int threadCountNdx = 0; threadCountNdx < DE_LENGTH_OF_ARRAY(threadCounts); threadCountNdx++)
		{
			int threadCount = threadCounts[threadCountNdx];

			for (int contextCountNdx = 0; contextCountNdx < DE_LENGTH_OF_ARRAY(perThreadContextCounts); contextCountNdx++)
			{
				int contextCount = perThreadContextCounts[contextCountNdx];

				if (threadCount * contextCount != 4 && threadCount * contextCount != 1)
					continue;

				TestConfig config				= basicConfig;
				config.sharedCoordBuffer		= true;
				config.sharedIndexBuffer		= true;
				config.sharedProgram			= true;
				config.textureType				= TestConfig::TEXTURETYPE_SHARED_TEXTURE;
				config.threadCount				= threadCount;
				config.perThreadContextCount	= contextCount;

				{
					TestConfig smallConfig		= config;
					smallConfig.triangleCount	= 1;
					smallConfig.drawCallCount	= 1000;
					smallConfig.frameCount		= 10;

					if (threadCount * contextCount == 1)
						smallConfig.frameCount *= 4;

					sharedallGroup->addChild(new SharedRenderingPerfCase(m_eglTestCtx, smallConfig, (createTestName(threadCount, contextCount) + "_small_call").c_str(), ""));
				}

				{
					TestConfig bigConfig	= config;
					bigConfig.triangleCount	= 1000;
					bigConfig.drawCallCount	= 1;
					bigConfig.frameCount	= 10;

					if (threadCount * contextCount == 1)
						bigConfig.frameCount *= 4;

					sharedallGroup->addChild(new SharedRenderingPerfCase(m_eglTestCtx, bigConfig, (createTestName(threadCount, contextCount) + "_big_call").c_str(), ""));
				}
			}
		}

		addChild(sharedallGroup);
	}

	// Add EGLImage tests
	{
		TestCaseGroup* sharedTextureGroup = new TestCaseGroup(m_eglTestCtx, "egl_image", "EGL image tests.");

		for (int threadCountNdx = 0; threadCountNdx < DE_LENGTH_OF_ARRAY(threadCounts); threadCountNdx++)
		{
			int threadCount = threadCounts[threadCountNdx];

			for (int contextCountNdx = 0; contextCountNdx < DE_LENGTH_OF_ARRAY(perThreadContextCounts); contextCountNdx++)
			{
				int contextCount = perThreadContextCounts[contextCountNdx];

				if (threadCount * contextCount != 4 && threadCount * contextCount != 1)
					continue;

				TestConfig config = basicConfig;

				config.textureType				= TestConfig::TEXTURETYPE_IMAGE;
				config.threadCount				= threadCount;
				config.perThreadContextCount	= contextCount;
				config.sharedContexts			= false;

				{
					TestConfig smallConfig		= config;
					smallConfig.triangleCount	= 1;
					smallConfig.drawCallCount	= 1000;
					smallConfig.frameCount		= 10;

					if (threadCount * contextCount == 1)
						smallConfig.frameCount *= 4;

					sharedTextureGroup->addChild(new SharedRenderingPerfCase(m_eglTestCtx, smallConfig, (createTestName(threadCount, contextCount) + "_small_call").c_str(), ""));
				}

				{
					TestConfig bigConfig	= config;
					bigConfig.triangleCount	= 1000;
					bigConfig.drawCallCount	= 1;
					bigConfig.frameCount	= 10;

					if (threadCount * contextCount == 1)
						bigConfig.frameCount *= 4;

					sharedTextureGroup->addChild(new SharedRenderingPerfCase(m_eglTestCtx, bigConfig, (createTestName(threadCount, contextCount) + "_big_call").c_str(), ""));
				}
			}
		}

		addChild(sharedTextureGroup);
	}

	// Add shared EGLImage tests
	{
		TestCaseGroup* sharedTextureGroup = new TestCaseGroup(m_eglTestCtx, "shared_egl_image", "Shared EGLImage tests.");

		for (int threadCountNdx = 0; threadCountNdx < DE_LENGTH_OF_ARRAY(threadCounts); threadCountNdx++)
		{
			int threadCount = threadCounts[threadCountNdx];

			for (int contextCountNdx = 0; contextCountNdx < DE_LENGTH_OF_ARRAY(perThreadContextCounts); contextCountNdx++)
			{
				int contextCount = perThreadContextCounts[contextCountNdx];

				if (threadCount * contextCount != 4 && threadCount * contextCount != 1)
					continue;

				TestConfig config				= basicConfig;

				config.textureType				= TestConfig::TEXTURETYPE_SHARED_IMAGE;
				config.threadCount				= threadCount;
				config.perThreadContextCount	= contextCount;
				config.sharedContexts			= false;

				{
					TestConfig smallConfig		= config;
					smallConfig.triangleCount	= 1;
					smallConfig.drawCallCount	= 1000;
					smallConfig.frameCount		= 10;

					if (threadCount * contextCount == 1)
						smallConfig.frameCount *= 4;

					sharedTextureGroup->addChild(new SharedRenderingPerfCase(m_eglTestCtx, smallConfig, (createTestName(threadCount, contextCount) + "_small_call").c_str(), ""));
				}

				{
					TestConfig bigConfig	= config;
					bigConfig.triangleCount	= 1000;
					bigConfig.drawCallCount	= 1;
					bigConfig.frameCount	= 10;

					if (threadCount * contextCount == 1)
						bigConfig.frameCount *= 4;

					sharedTextureGroup->addChild(new SharedRenderingPerfCase(m_eglTestCtx, bigConfig, (createTestName(threadCount, contextCount) + "_big_call").c_str(), ""));
				}
			}
		}

		addChild(sharedTextureGroup);
	}

	// Shared EGLImage texture test
	{
		TestCaseGroup* sharedTextureGroup = new TestCaseGroup(m_eglTestCtx, "shared_egl_image_texture", "Shared EGLImage texture tests.");

		for (int threadCountNdx = 0; threadCountNdx < DE_LENGTH_OF_ARRAY(threadCounts); threadCountNdx++)
		{
			int threadCount = threadCounts[threadCountNdx];

			for (int contextCountNdx = 0; contextCountNdx < DE_LENGTH_OF_ARRAY(perThreadContextCounts); contextCountNdx++)
			{
				int contextCount = perThreadContextCounts[contextCountNdx];

				if (threadCount * contextCount != 4 && threadCount * contextCount != 1)
					continue;

				TestConfig config				= basicConfig;
				config.textureType				= TestConfig::TEXTURETYPE_SHARED_IMAGE_TEXTURE;
				config.threadCount				= threadCount;
				config.perThreadContextCount	= contextCount;

				{
					TestConfig smallConfig		= config;
					smallConfig.triangleCount	= 1;
					smallConfig.drawCallCount	= 1000;
					smallConfig.frameCount		= 10;

					if (threadCount * contextCount == 1)
						smallConfig.frameCount *= 4;

					sharedTextureGroup->addChild(new SharedRenderingPerfCase(m_eglTestCtx, smallConfig, (createTestName(threadCount, contextCount) + "_small_call").c_str(), ""));
				}

				{
					TestConfig bigConfig	= config;
					bigConfig.triangleCount	= 1000;
					bigConfig.drawCallCount	= 1;
					bigConfig.frameCount	= 10;

					if (threadCount * contextCount == 1)
						bigConfig.frameCount *= 4;

					sharedTextureGroup->addChild(new SharedRenderingPerfCase(m_eglTestCtx, bigConfig, (createTestName(threadCount, contextCount) + "_big_call").c_str(), ""));
				}
			}
		}

		addChild(sharedTextureGroup);
	}

}

} // egl
} // deqp
