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
 * \brief EGL image tests.
 *//*--------------------------------------------------------------------*/

#include "teglImageFormatTests.hpp"

#include "deStringUtil.hpp"
#include "deSTLUtil.hpp"

#include "tcuTestLog.hpp"
#include "tcuSurface.hpp"
#include "tcuTexture.hpp"
#include "tcuTextureUtil.hpp"
#include "tcuImageCompare.hpp"
#include "tcuCommandLine.hpp"

#include "egluNativeDisplay.hpp"
#include "egluNativeWindow.hpp"
#include "egluNativePixmap.hpp"
#include "egluConfigFilter.hpp"
#include "egluUnique.hpp"
#include "egluUtil.hpp"

#include "eglwLibrary.hpp"
#include "eglwEnums.hpp"

#include "gluCallLogWrapper.hpp"
#include "gluShaderProgram.hpp"
#include "gluStrUtil.hpp"
#include "gluTexture.hpp"
#include "gluPixelTransfer.hpp"
#include "gluObjectWrapper.hpp"
#include "gluTextureUtil.hpp"

#include "glwEnums.hpp"
#include "glwFunctions.hpp"

#include "teglImageUtil.hpp"
#include "teglAndroidUtil.hpp"

#include <vector>
#include <string>
#include <set>

using std::vector;
using std::set;
using std::string;

using de::MovePtr;
using de::UniquePtr;

using glu::Framebuffer;
using glu::Renderbuffer;
using glu::Texture;

using eglu::UniqueImage;

using tcu::ConstPixelBufferAccess;

using namespace glw;
using namespace eglw;

namespace deqp
{
namespace egl
{

namespace
{

glu::ProgramSources programSources (const string& vertexSource, const string& fragmentSource)
{
	glu::ProgramSources sources;

	sources << glu::VertexSource(vertexSource) << glu::FragmentSource(fragmentSource);

	return sources;
}

class Program : public glu::ShaderProgram
{
public:
	Program (const glw::Functions& gl, const char* vertexSource, const char* fragmentSource)
		: glu::ShaderProgram(gl, programSources(vertexSource, fragmentSource)) {}
};

} // anonymous

namespace Image
{

class ImageApi;

class IllegalRendererException : public std::exception
{
};

class Action
{
public:
	virtual			~Action					(void) {}
	virtual bool	invoke					(ImageApi& api, MovePtr<UniqueImage>& image, tcu::Texture2D& refImg) const = 0;
	virtual string	getRequiredExtension	(void) const = 0;
};

struct TestSpec
{
	std::string name;
	std::string desc;

	enum ApiContext
	{
		API_GLES2 = 0,
		//API_VG
		//API_GLES1

		API_LAST
	};

	struct Operation
	{
		Operation (int apiIndex_, const Action& action_) : apiIndex(apiIndex_), action(&action_) {}
		int				apiIndex;
		const Action*	action;
	};

	vector<ApiContext>	contexts;
	vector<Operation>	operations;

};

class ImageApi
{
public:
					ImageApi		(const Library& egl, int contextId, EGLDisplay display, EGLSurface surface);
	virtual			~ImageApi		(void) {}

protected:
	const Library&	m_egl;
	int				m_contextId;
	EGLDisplay		m_display;
	EGLSurface		m_surface;
};

ImageApi::ImageApi (const Library& egl, int contextId, EGLDisplay display, EGLSurface surface)
	: m_egl				(egl)
	, m_contextId		(contextId)
	, m_display			(display)
	, m_surface			(surface)
{
}

class GLES2ImageApi : public ImageApi, private glu::CallLogWrapper
{
public:
	class GLES2Action : public Action
	{
	public:
		bool				invoke					(ImageApi& api, MovePtr<UniqueImage>& image, tcu::Texture2D& ref) const;
		virtual bool		invokeGLES2				(GLES2ImageApi& api, MovePtr<UniqueImage>& image, tcu::Texture2D& ref) const = 0;
	};

	class Create : public GLES2Action
	{
	public:
								Create					(MovePtr<ImageSource> imgSource) : m_imgSource(imgSource) {}
		string					getRequiredExtension	(void) const { return m_imgSource->getRequiredExtension(); }
		bool					invokeGLES2				(GLES2ImageApi& api, MovePtr<UniqueImage>& image, tcu::Texture2D& ref) const;
		glw::GLenum				getEffectiveFormat		(void) const { return m_imgSource->getEffectiveFormat(); }

	private:
		UniquePtr<ImageSource>	m_imgSource;
	};

	class Render : public GLES2Action
	{
	public:
		string				getRequiredExtension	(void) const { return "GL_OES_EGL_image"; }
	};

	class RenderTexture2D				: public Render { public: bool invokeGLES2 (GLES2ImageApi& api, MovePtr<UniqueImage>& image, tcu::Texture2D& ref) const; };
	class RenderTextureCubemap			: public Render { public: bool invokeGLES2 (GLES2ImageApi& api, MovePtr<UniqueImage>& image, tcu::Texture2D& ref) const; };
	class RenderReadPixelsRenderbuffer	: public Render { public: bool invokeGLES2 (GLES2ImageApi& api, MovePtr<UniqueImage>& image, tcu::Texture2D& ref) const; };
	class RenderDepthbuffer				: public Render { public: bool invokeGLES2 (GLES2ImageApi& api, MovePtr<UniqueImage>& image, tcu::Texture2D& ref) const; };
	class RenderStencilbuffer			: public Render { public: bool invokeGLES2 (GLES2ImageApi& api, MovePtr<UniqueImage>& image, tcu::Texture2D& ref) const; };
	class RenderTryAll					: public Render { public: bool invokeGLES2 (GLES2ImageApi& api, MovePtr<UniqueImage>& image, tcu::Texture2D& ref) const; };

	class Modify : public GLES2Action
	{
	public:
		string				getRequiredExtension	(void) const { return "GL_OES_EGL_image"; }
	};

	class ModifyTexSubImage : public Modify
	{
	public:
							ModifyTexSubImage		(GLenum format, GLenum type) : m_format(format), m_type(type) {}
		bool				invokeGLES2				(GLES2ImageApi& api, MovePtr<UniqueImage>& image, tcu::Texture2D& ref) const;
		GLenum				getFormat				(void) const { return m_format; }
		GLenum				getType					(void) const { return m_type; }

	private:
		GLenum				m_format;
		GLenum				m_type;
	};

	class ModifyRenderbuffer : public Modify
	{
	public:
		bool				invokeGLES2				(GLES2ImageApi& api, MovePtr<UniqueImage>& image, tcu::Texture2D& ref) const;

	protected:
		virtual void		initializeRbo			(GLES2ImageApi& api, GLuint rbo, tcu::Texture2D& ref) const = 0;
	};

	class ModifyRenderbufferClearColor : public ModifyRenderbuffer
	{
	public:
					ModifyRenderbufferClearColor	(tcu::Vec4 color) : m_color(color) {}

	protected:
		void		initializeRbo					(GLES2ImageApi& api, GLuint rbo, tcu::Texture2D& ref) const;

		tcu::Vec4	m_color;
	};

	class ModifyRenderbufferClearDepth : public ModifyRenderbuffer
	{
	public:
					ModifyRenderbufferClearDepth	(GLfloat depth) : m_depth(depth) {}

	protected:
		void		initializeRbo					(GLES2ImageApi& api, GLuint rbo, tcu::Texture2D& ref) const;

		GLfloat		m_depth;
	};

	class ModifyRenderbufferClearStencil : public ModifyRenderbuffer
	{
	public:
					ModifyRenderbufferClearStencil	(GLint stencil) : m_stencil(stencil) {}

	protected:
		void		initializeRbo					(GLES2ImageApi& api, GLuint rbo, tcu::Texture2D& ref) const;

		GLint		m_stencil;
	};

					GLES2ImageApi					(const Library& egl, const glw::Functions& gl, int contextId, tcu::TestLog& log, EGLDisplay display, EGLSurface surface, EGLConfig config);
					~GLES2ImageApi					(void);

private:
	EGLContext					m_context;
	const glw::Functions&		m_gl;

	MovePtr<UniqueImage>		createImage			(const ImageSource& source, const ClientBuffer& buffer) const;
};

GLES2ImageApi::GLES2ImageApi (const Library& egl, const glw::Functions& gl, int contextId, tcu::TestLog& log, EGLDisplay display, EGLSurface surface, EGLConfig config)
	: ImageApi				(egl, contextId, display, surface)
	, glu::CallLogWrapper	(gl, log)
	, m_context				(DE_NULL)
	, m_gl					(gl)
{
	const EGLint attriblist[] =
	{
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};

	EGLint configId = -1;
	EGLU_CHECK_CALL(m_egl, getConfigAttrib(m_display, config, EGL_CONFIG_ID, &configId));
	getLog() << tcu::TestLog::Message << "Creating gles2 context with config id: " << configId << " context: " << m_contextId << tcu::TestLog::EndMessage;
	egl.bindAPI(EGL_OPENGL_ES_API);
	m_context = m_egl.createContext(m_display, config, EGL_NO_CONTEXT, attriblist);
	EGLU_CHECK_MSG(m_egl, "Failed to create GLES2 context");

	egl.makeCurrent(display, m_surface, m_surface, m_context);
	EGLU_CHECK_MSG(m_egl, "Failed to make context current");
}

GLES2ImageApi::~GLES2ImageApi (void)
{
	m_egl.makeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	m_egl.destroyContext(m_display, m_context);
}

bool GLES2ImageApi::GLES2Action::invoke (ImageApi& api, MovePtr<UniqueImage>& image, tcu::Texture2D& ref) const
{
	GLES2ImageApi& gles2Api = dynamic_cast<GLES2ImageApi&>(api);

	gles2Api.m_egl.makeCurrent(gles2Api.m_display, gles2Api.m_surface, gles2Api.m_surface, gles2Api.m_context);
	return invokeGLES2(gles2Api, image, ref);
}


bool GLES2ImageApi::Create::invokeGLES2 (GLES2ImageApi& api, MovePtr<UniqueImage>& image, tcu::Texture2D& ref) const
{
	de::UniquePtr<ClientBuffer>	buffer	(m_imgSource->createBuffer(api.m_gl, &ref));
	image = api.createImage(*m_imgSource, *buffer);
	return true;
}

MovePtr<UniqueImage> GLES2ImageApi::createImage (const ImageSource& source, const ClientBuffer& buffer) const
{
	const EGLImageKHR image = source.createImage(m_egl, m_display, m_context, buffer.get());
	return MovePtr<UniqueImage>(new UniqueImage(m_egl, m_display, image));
}

static void imageTargetTexture2D (const Library& egl, const glw::Functions& gl, GLeglImageOES img)
{
	gl.eglImageTargetTexture2DOES(GL_TEXTURE_2D, img);
	{
		const GLenum error = gl.getError();

		if (error == GL_INVALID_OPERATION)
			TCU_THROW(NotSupportedError, "Creating texture2D from EGLImage type not supported");

		GLU_EXPECT_NO_ERROR(error, "glEGLImageTargetTexture2DOES()");
		EGLU_CHECK_MSG(egl, "glEGLImageTargetTexture2DOES()");
	}
}

static void imageTargetRenderbuffer (const Library& egl, const glw::Functions& gl, GLeglImageOES img)
{
	gl.eglImageTargetRenderbufferStorageOES(GL_RENDERBUFFER, img);
	{
		const GLenum error = gl.getError();

		if (error == GL_INVALID_OPERATION)
			TCU_THROW(NotSupportedError, "Creating renderbuffer from EGLImage type not supported");

		GLU_EXPECT_NO_ERROR(error, "glEGLImageTargetRenderbufferStorageOES()");
		EGLU_CHECK_MSG(egl, "glEGLImageTargetRenderbufferStorageOES()");
	}
}

static void framebufferRenderbuffer (const glw::Functions& gl, GLenum attachment, GLuint rbo)
{
	GLU_CHECK_GLW_CALL(gl, framebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, rbo));
	TCU_CHECK_AND_THROW(NotSupportedError,
						gl.checkFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
						("EGLImage as " + string(glu::getFramebufferAttachmentName(attachment)) + " not supported").c_str());
}

static const float squareTriangleCoords[] =
{
	-1.0, -1.0,
	1.0, -1.0,
	1.0,  1.0,

	1.0,  1.0,
	-1.0,  1.0,
	-1.0, -1.0
};

bool GLES2ImageApi::RenderTexture2D::invokeGLES2 (GLES2ImageApi& api, MovePtr<UniqueImage>& img, tcu::Texture2D& reference) const
{
	const glw::Functions&	gl		= api.m_gl;
	tcu::TestLog&			log		= api.getLog();
	Texture					srcTex	(gl);

	// Branch only taken in TryAll case
	if (reference.getFormat().order == tcu::TextureFormat::DS || reference.getFormat().order == tcu::TextureFormat::D)
		throw IllegalRendererException(); // Skip, GLES2 does not support sampling depth textures
	if (reference.getFormat().order == tcu::TextureFormat::S)
		throw IllegalRendererException(); // Skip, GLES2 does not support sampling stencil textures

	gl.clearColor(0.0, 0.0, 0.0, 0.0);
	gl.viewport(0, 0, reference.getWidth(), reference.getHeight());
	gl.clear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	gl.disable(GL_DEPTH_TEST);

	log << tcu::TestLog::Message << "Rendering EGLImage as GL_TEXTURE_2D in context: " << api.m_contextId << tcu::TestLog::EndMessage;
	TCU_CHECK(**img != EGL_NO_IMAGE_KHR);

	GLU_CHECK_GLW_CALL(gl, bindTexture(GL_TEXTURE_2D, *srcTex));
	imageTargetTexture2D(api.m_egl, gl, **img);

	GLU_CHECK_GLW_CALL(gl, texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
	GLU_CHECK_GLW_CALL(gl, texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
	GLU_CHECK_GLW_CALL(gl, texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	GLU_CHECK_GLW_CALL(gl, texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));

	const char* const vertexShader =
		"attribute highp vec2 a_coord;\n"
		"varying mediump vec2 v_texCoord;\n"
		"void main(void) {\n"
		"\tv_texCoord = vec2((a_coord.x + 1.0) * 0.5, (a_coord.y + 1.0) * 0.5);\n"
		"\tgl_Position = vec4(a_coord, -0.1, 1.0);\n"
		"}\n";

	const char* const fragmentShader =
		"varying mediump vec2 v_texCoord;\n"
		"uniform sampler2D u_sampler;\n"
		"void main(void) {\n"
		"\tmediump vec4 texColor = texture2D(u_sampler, v_texCoord);\n"
		"\tgl_FragColor = vec4(texColor);\n"
		"}";

	Program program(gl, vertexShader, fragmentShader);
	TCU_CHECK(program.isOk());

	GLuint glProgram = program.getProgram();
	GLU_CHECK_GLW_CALL(gl, useProgram(glProgram));

	GLuint coordLoc = gl.getAttribLocation(glProgram, "a_coord");
	TCU_CHECK_MSG((int)coordLoc != -1, "Couldn't find attribute a_coord");

	GLuint samplerLoc = gl.getUniformLocation(glProgram, "u_sampler");
	TCU_CHECK_MSG((int)samplerLoc != (int)-1, "Couldn't find uniform u_sampler");

	GLU_CHECK_GLW_CALL(gl, bindTexture(GL_TEXTURE_2D, *srcTex));
	GLU_CHECK_GLW_CALL(gl, uniform1i(samplerLoc, 0));
	GLU_CHECK_GLW_CALL(gl, enableVertexAttribArray(coordLoc));
	GLU_CHECK_GLW_CALL(gl, vertexAttribPointer(coordLoc, 2, GL_FLOAT, GL_FALSE, 0, squareTriangleCoords));

	GLU_CHECK_GLW_CALL(gl, drawArrays(GL_TRIANGLES, 0, 6));
	GLU_CHECK_GLW_CALL(gl, disableVertexAttribArray(coordLoc));
	GLU_CHECK_GLW_CALL(gl, bindTexture(GL_TEXTURE_2D, 0));

	tcu::Surface refSurface	(reference.getWidth(), reference.getHeight());
	tcu::Surface screen		(reference.getWidth(), reference.getHeight());
	GLU_CHECK_GLW_CALL(gl, readPixels(0, 0, screen.getWidth(), screen.getHeight(), GL_RGBA, GL_UNSIGNED_BYTE, screen.getAccess().getDataPtr()));

	tcu::copy(refSurface.getAccess(), reference.getLevel(0));

	float	threshold	= 0.05f;
	bool	match		= tcu::fuzzyCompare(log, "ComparisonResult", "Image comparison result", refSurface, screen, threshold, tcu::COMPARE_LOG_RESULT);

	return match;
}

bool GLES2ImageApi::RenderDepthbuffer::invokeGLES2 (GLES2ImageApi& api, MovePtr<UniqueImage>& img, tcu::Texture2D& reference) const
{
	const glw::Functions&	gl					= api.m_gl;
	tcu::TestLog&			log					= api.getLog();
	Framebuffer				framebuffer			(gl);
	Renderbuffer			renderbufferColor	(gl);
	Renderbuffer			renderbufferDepth	(gl);
	const tcu::RGBA			compareThreshold	(32, 32, 32, 32); // layer colors are far apart, large thresholds are ok

	// Branch only taken in TryAll case
	if (reference.getFormat().order != tcu::TextureFormat::DS && reference.getFormat().order != tcu::TextureFormat::D)
		throw IllegalRendererException(); // Skip, interpreting non-depth data as depth data is not meaningful

	log << tcu::TestLog::Message << "Rendering with depth buffer" << tcu::TestLog::EndMessage;

	GLU_CHECK_GLW_CALL(gl, bindFramebuffer(GL_FRAMEBUFFER, *framebuffer));

	GLU_CHECK_GLW_CALL(gl, bindRenderbuffer(GL_RENDERBUFFER, *renderbufferColor));
	GLU_CHECK_GLW_CALL(gl, renderbufferStorage(GL_RENDERBUFFER, GL_RGBA4, reference.getWidth(), reference.getHeight()));
	framebufferRenderbuffer(gl, GL_COLOR_ATTACHMENT0, *renderbufferColor);

	GLU_CHECK_GLW_CALL(gl, bindRenderbuffer(GL_RENDERBUFFER, *renderbufferDepth));
	imageTargetRenderbuffer(api.m_egl, gl, **img);
	framebufferRenderbuffer(gl, GL_DEPTH_ATTACHMENT, *renderbufferDepth);
	GLU_CHECK_GLW_CALL(gl, bindRenderbuffer(GL_RENDERBUFFER, 0));

	GLU_CHECK_GLW_CALL(gl, viewport(0, 0, reference.getWidth(), reference.getHeight()));

	// Render
	const char* vertexShader =
		"attribute highp vec2 a_coord;\n"
		"uniform highp float u_depth;\n"
		"void main(void) {\n"
		"\tgl_Position = vec4(a_coord, u_depth, 1.0);\n"
		"}\n";

	const char* fragmentShader =
		"uniform mediump vec4 u_color;\n"
		"void main(void) {\n"
		"\tgl_FragColor = u_color;\n"
		"}";

	Program program(gl, vertexShader, fragmentShader);
	TCU_CHECK(program.isOk());

	GLuint glProgram = program.getProgram();
	GLU_CHECK_GLW_CALL(gl, useProgram(glProgram));

	GLuint coordLoc = gl.getAttribLocation(glProgram, "a_coord");
	TCU_CHECK_MSG((int)coordLoc != -1, "Couldn't find attribute a_coord");

	GLuint colorLoc = gl.getUniformLocation(glProgram, "u_color");
	TCU_CHECK_MSG((int)colorLoc != (int)-1, "Couldn't find uniform u_color");

	GLuint depthLoc = gl.getUniformLocation(glProgram, "u_depth");
	TCU_CHECK_MSG((int)depthLoc != (int)-1, "Couldn't find uniform u_depth");

	GLU_CHECK_GLW_CALL(gl, clearColor(0.5f, 1.0f, 0.5f, 1.0f));
	GLU_CHECK_GLW_CALL(gl, clear(GL_COLOR_BUFFER_BIT));

	tcu::Vec4 depthLevelColors[] = {
		tcu::Vec4(1.0f, 0.0f, 0.0f, 1.0f),
		tcu::Vec4(0.0f, 1.0f, 0.0f, 1.0f),
		tcu::Vec4(1.0f, 1.0f, 0.0f, 1.0f),
		tcu::Vec4(0.0f, 0.0f, 1.0f, 1.0f),
		tcu::Vec4(1.0f, 0.0f, 1.0f, 1.0f),

		tcu::Vec4(1.0f, 1.0f, 0.0f, 1.0f),
		tcu::Vec4(1.0f, 1.0f, 1.0f, 1.0f),
		tcu::Vec4(0.5f, 0.0f, 0.0f, 1.0f),
		tcu::Vec4(0.0f, 0.5f, 0.0f, 1.0f),
		tcu::Vec4(0.5f, 0.5f, 0.0f, 1.0f)
	};

	GLU_CHECK_GLW_CALL(gl, enableVertexAttribArray(coordLoc));
	GLU_CHECK_GLW_CALL(gl, vertexAttribPointer(coordLoc, 2, GL_FLOAT, GL_FALSE, 0, squareTriangleCoords));

	GLU_CHECK_GLW_CALL(gl, enable(GL_DEPTH_TEST));
	GLU_CHECK_GLW_CALL(gl, depthFunc(GL_LESS));
	GLU_CHECK_GLW_CALL(gl, depthMask(GL_FALSE));

	for (int level = 0; level < DE_LENGTH_OF_ARRAY(depthLevelColors); level++)
	{
		const tcu::Vec4	color		= depthLevelColors[level];
		const float		clipDepth	= ((float)(level + 1) * 0.1f) * 2.0f - 1.0f; // depth in clip coords

		GLU_CHECK_GLW_CALL(gl, uniform4f(colorLoc, color.x(), color.y(), color.z(), color.w()));
		GLU_CHECK_GLW_CALL(gl, uniform1f(depthLoc, clipDepth));
		GLU_CHECK_GLW_CALL(gl, drawArrays(GL_TRIANGLES, 0, 6));
	}

	GLU_CHECK_GLW_CALL(gl, depthMask(GL_TRUE));
	GLU_CHECK_GLW_CALL(gl, disable(GL_DEPTH_TEST));
	GLU_CHECK_GLW_CALL(gl, disableVertexAttribArray(coordLoc));

	const ConstPixelBufferAccess&	refAccess		= reference.getLevel(0);
	tcu::Surface					screen			(reference.getWidth(), reference.getHeight());
	tcu::Surface					referenceScreen	(reference.getWidth(), reference.getHeight());

	gl.readPixels(0, 0, screen.getWidth(), screen.getHeight(), GL_RGBA, GL_UNSIGNED_BYTE, screen.getAccess().getDataPtr());

	for (int y = 0; y < reference.getHeight(); y++)
	{
		for (int x = 0; x < reference.getWidth(); x++)
		{
			tcu::Vec4 result = tcu::Vec4(0.5f, 1.0f, 0.5f, 1.0f);

			for (int level = 0; level < DE_LENGTH_OF_ARRAY(depthLevelColors); level++)
			{
				if ((float)(level + 1) * 0.1f < refAccess.getPixDepth(x, y))
					result = depthLevelColors[level];
			}

			referenceScreen.getAccess().setPixel(result, x, y);
		}
	}

	GLU_CHECK_GLW_CALL(gl, bindFramebuffer(GL_FRAMEBUFFER, 0));
	GLU_CHECK_GLW_CALL(gl, finish());

	return tcu::pixelThresholdCompare(log, "Depth buffer rendering result", "Result from rendering with depth buffer", referenceScreen, screen, compareThreshold, tcu::COMPARE_LOG_RESULT);
}

bool GLES2ImageApi::RenderStencilbuffer::invokeGLES2 (GLES2ImageApi& api, MovePtr<UniqueImage>& img, tcu::Texture2D& reference) const
{
	// Branch only taken in TryAll case
	if (reference.getFormat().order != tcu::TextureFormat::DS && reference.getFormat().order != tcu::TextureFormat::S)
		throw IllegalRendererException(); // Skip, interpreting non-stencil data as stencil data is not meaningful

	const glw::Functions&	gl					= api.m_gl;
	tcu::TestLog&			log					= api.getLog();
	Framebuffer				framebuffer			(gl);
	Renderbuffer			renderbufferColor	(gl);
	Renderbuffer			renderbufferStencil (gl);
	const tcu::RGBA			compareThreshold	(32, 32, 32, 32); // layer colors are far apart, large thresholds are ok
	const deUint32			numStencilBits		= tcu::getTextureFormatBitDepth(tcu::getEffectiveDepthStencilTextureFormat(reference.getLevel(0).getFormat(), tcu::Sampler::MODE_STENCIL)).x();
	const deUint32			maxStencil			= deBitMask32(0, numStencilBits);

	log << tcu::TestLog::Message << "Rendering with stencil buffer" << tcu::TestLog::EndMessage;

	GLU_CHECK_GLW_CALL(gl, bindFramebuffer(GL_FRAMEBUFFER, *framebuffer));

	GLU_CHECK_GLW_CALL(gl, bindRenderbuffer(GL_RENDERBUFFER, *renderbufferColor));
	GLU_CHECK_GLW_CALL(gl, renderbufferStorage(GL_RENDERBUFFER, GL_RGBA4, reference.getWidth(), reference.getHeight()));
	framebufferRenderbuffer(gl, GL_COLOR_ATTACHMENT0, *renderbufferColor);

	GLU_CHECK_GLW_CALL(gl, bindRenderbuffer(GL_RENDERBUFFER, *renderbufferStencil));
	imageTargetRenderbuffer(api.m_egl, gl, **img);
	framebufferRenderbuffer(gl, GL_STENCIL_ATTACHMENT, *renderbufferStencil);
	GLU_CHECK_GLW_CALL(gl, bindRenderbuffer(GL_RENDERBUFFER, 0));

	GLU_CHECK_GLW_CALL(gl, viewport(0, 0, reference.getWidth(), reference.getHeight()));

	// Render
	const char* vertexShader =
		"attribute highp vec2 a_coord;\n"
		"void main(void) {\n"
		"\tgl_Position = vec4(a_coord, 0.0, 1.0);\n"
		"}\n";

	const char* fragmentShader =
		"uniform mediump vec4 u_color;\n"
		"void main(void) {\n"
		"\tgl_FragColor = u_color;\n"
		"}";

	Program program(gl, vertexShader, fragmentShader);
	TCU_CHECK(program.isOk());

	GLuint glProgram = program.getProgram();
	GLU_CHECK_GLW_CALL(gl, useProgram(glProgram));

	GLuint coordLoc = gl.getAttribLocation(glProgram, "a_coord");
	TCU_CHECK_MSG((int)coordLoc != -1, "Couldn't find attribute a_coord");

	GLuint colorLoc = gl.getUniformLocation(glProgram, "u_color");
	TCU_CHECK_MSG((int)colorLoc != (int)-1, "Couldn't find uniform u_color");

	GLU_CHECK_GLW_CALL(gl, clearColor(0.5f, 1.0f, 0.5f, 1.0f));
	GLU_CHECK_GLW_CALL(gl, clear(GL_COLOR_BUFFER_BIT));

	tcu::Vec4 stencilLevelColors[] = {
		tcu::Vec4(1.0f, 0.0f, 0.0f, 1.0f),
		tcu::Vec4(0.0f, 1.0f, 0.0f, 1.0f),
		tcu::Vec4(1.0f, 1.0f, 0.0f, 1.0f),
		tcu::Vec4(0.0f, 0.0f, 1.0f, 1.0f),
		tcu::Vec4(1.0f, 0.0f, 1.0f, 1.0f),

		tcu::Vec4(1.0f, 1.0f, 0.0f, 1.0f),
		tcu::Vec4(1.0f, 1.0f, 1.0f, 1.0f),
		tcu::Vec4(0.5f, 0.0f, 0.0f, 1.0f),
		tcu::Vec4(0.0f, 0.5f, 0.0f, 1.0f),
		tcu::Vec4(0.5f, 0.5f, 0.0f, 1.0f)
	};

	GLU_CHECK_GLW_CALL(gl, enableVertexAttribArray(coordLoc));
	GLU_CHECK_GLW_CALL(gl, vertexAttribPointer(coordLoc, 2, GL_FLOAT, GL_FALSE, 0, squareTriangleCoords));

	GLU_CHECK_GLW_CALL(gl, enable(GL_STENCIL_TEST));
	GLU_CHECK_GLW_CALL(gl, stencilOp(GL_KEEP, GL_KEEP, GL_KEEP));

	for (int level = 0; level < DE_LENGTH_OF_ARRAY(stencilLevelColors); level++)
	{
		const tcu::Vec4	color	= stencilLevelColors[level];
		const int		stencil	= (int)(((float)(level + 1) * 0.1f) * (float)maxStencil);

		GLU_CHECK_GLW_CALL(gl, stencilFunc(GL_LESS, stencil, 0xFFFFFFFFu));
		GLU_CHECK_GLW_CALL(gl, uniform4f(colorLoc, color.x(), color.y(), color.z(), color.w()));
		GLU_CHECK_GLW_CALL(gl, drawArrays(GL_TRIANGLES, 0, 6));
	}

	GLU_CHECK_GLW_CALL(gl, disable(GL_STENCIL_TEST));
	GLU_CHECK_GLW_CALL(gl, disableVertexAttribArray(coordLoc));

	const ConstPixelBufferAccess&	refAccess		= reference.getLevel(0);
	tcu::Surface					screen			(reference.getWidth(), reference.getHeight());
	tcu::Surface					referenceScreen	(reference.getWidth(), reference.getHeight());

	gl.readPixels(0, 0, screen.getWidth(), screen.getHeight(), GL_RGBA, GL_UNSIGNED_BYTE, screen.getAccess().getDataPtr());

	for (int y = 0; y < reference.getHeight(); y++)
	for (int x = 0; x < reference.getWidth(); x++)
	{
		tcu::Vec4 result = tcu::Vec4(0.5f, 1.0f, 0.5f, 1.0f);

		for (int level = 0; level < DE_LENGTH_OF_ARRAY(stencilLevelColors); level++)
		{
			const int levelStencil = (int)(((float)(level + 1) * 0.1f) * (float)maxStencil);
			if (levelStencil < refAccess.getPixStencil(x, y))
				result = stencilLevelColors[level];
		}

		referenceScreen.getAccess().setPixel(result, x, y);
	}

	GLU_CHECK_GLW_CALL(gl, bindFramebuffer(GL_FRAMEBUFFER, 0));
	GLU_CHECK_GLW_CALL(gl, finish());

	return tcu::pixelThresholdCompare(log, "StencilResult", "Result from rendering with stencil buffer", referenceScreen, screen, compareThreshold, tcu::COMPARE_LOG_RESULT);
}

bool GLES2ImageApi::RenderReadPixelsRenderbuffer::invokeGLES2 (GLES2ImageApi& api, MovePtr<UniqueImage>& img, tcu::Texture2D& reference) const
{
	const glw::Functions&	gl				= api.m_gl;
	const tcu::IVec4		bitDepth		= tcu::getTextureFormatMantissaBitDepth(reference.getFormat());
	const tcu::IVec4		threshold		(2 * (tcu::IVec4(1) << (tcu::IVec4(8) - bitDepth)));
	const tcu::RGBA			threshold8		((deUint8)(de::clamp(threshold[0], 0, 255)), (deUint8)(de::clamp(threshold[1], 0, 255)), (deUint8)(de::clamp(threshold[2], 0, 255)), (deUint8)(de::clamp(threshold[3], 0, 255)));
	tcu::TestLog&			log				= api.getLog();
	Framebuffer				framebuffer		(gl);
	Renderbuffer			renderbuffer	(gl);
	tcu::Surface			screen			(reference.getWidth(), reference.getHeight());
	tcu::Surface			refSurface		(reference.getWidth(), reference.getHeight());

	// Branch only taken in TryAll case
	if (reference.getFormat().order == tcu::TextureFormat::DS || reference.getFormat().order == tcu::TextureFormat::D)
		throw IllegalRendererException(); // Skip, GLES2 does not support ReadPixels for depth attachments
	if (reference.getFormat().order == tcu::TextureFormat::S)
		throw IllegalRendererException(); // Skip, GLES2 does not support ReadPixels for stencil attachments

	log << tcu::TestLog::Message << "Reading with ReadPixels from renderbuffer" << tcu::TestLog::EndMessage;

	GLU_CHECK_GLW_CALL(gl, bindFramebuffer(GL_FRAMEBUFFER, *framebuffer));
	GLU_CHECK_GLW_CALL(gl, bindRenderbuffer(GL_RENDERBUFFER, *renderbuffer));
	imageTargetRenderbuffer(api.m_egl, gl, **img);
	framebufferRenderbuffer(gl, GL_COLOR_ATTACHMENT0, *renderbuffer);

	GLU_CHECK_GLW_CALL(gl, viewport(0, 0, reference.getWidth(), reference.getHeight()));

	gl.readPixels(0, 0, screen.getWidth(), screen.getHeight(), GL_RGBA, GL_UNSIGNED_BYTE, screen.getAccess().getDataPtr());

	GLU_CHECK_GLW_CALL(gl, bindFramebuffer(GL_FRAMEBUFFER, 0));
	GLU_CHECK_GLW_CALL(gl, bindRenderbuffer(GL_RENDERBUFFER, 0));
	GLU_CHECK_GLW_CALL(gl, finish());

	tcu::copy(refSurface.getAccess(), reference.getLevel(0));

	return tcu::pixelThresholdCompare(log, "Renderbuffer read", "Result from reading renderbuffer", refSurface, screen, threshold8, tcu::COMPARE_LOG_RESULT);

}

bool GLES2ImageApi::RenderTryAll::invokeGLES2 (GLES2ImageApi& api, MovePtr<UniqueImage>& img, tcu::Texture2D& reference) const
{
	bool										foundSupported			= false;
	tcu::TestLog&								log						= api.getLog();
	GLES2ImageApi::RenderTexture2D				renderTex2D;
	GLES2ImageApi::RenderReadPixelsRenderbuffer	renderReadPixels;
	GLES2ImageApi::RenderDepthbuffer			renderDepth;
	GLES2ImageApi::RenderStencilbuffer			renderStencil;
	Action*										actions[]				= { &renderTex2D, &renderReadPixels, &renderDepth, &renderStencil };

	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(actions); ++ndx)
	{
		try
		{
			if (!actions[ndx]->invoke(api, img, reference))
				return false;

			foundSupported = true;
		}
		catch (const tcu::NotSupportedError& error)
		{
			log << tcu::TestLog::Message << error.what() << tcu::TestLog::EndMessage;
		}
		catch (const IllegalRendererException&)
		{
			// not valid renderer
		}
	}

	if (!foundSupported)
		throw tcu::NotSupportedError("Rendering not supported", "", __FILE__, __LINE__);

	return true;
}

bool GLES2ImageApi::ModifyTexSubImage::invokeGLES2 (GLES2ImageApi& api, MovePtr<UniqueImage>& img, tcu::Texture2D& reference) const
{
	const glw::Functions&	gl		= api.m_gl;
	tcu::TestLog&			log		= api.getLog();
	glu::Texture			srcTex	(gl);
	const int				xOffset	= 8;
	const int				yOffset	= 16;
	const int				xSize	= de::clamp(16, 0, reference.getWidth() - xOffset);
	const int				ySize	= de::clamp(16, 0, reference.getHeight() - yOffset);
	tcu::Texture2D			src		(glu::mapGLTransferFormat(m_format, m_type), xSize, ySize);

	log << tcu::TestLog::Message << "Modifying EGLImage with gl.texSubImage2D" << tcu::TestLog::EndMessage;

	src.allocLevel(0);
	tcu::fillWithComponentGradients(src.getLevel(0), tcu::Vec4(0.0f, 0.0f, 0.0f, 0.0f), tcu::Vec4(1.0f, 1.0f, 1.0f, 1.0f));

	GLU_CHECK_GLW_CALL(gl, bindTexture(GL_TEXTURE_2D, *srcTex));
	imageTargetTexture2D(api.m_egl, gl, **img);
	GLU_CHECK_GLW_CALL(gl, texSubImage2D(GL_TEXTURE_2D, 0, xOffset, yOffset, src.getWidth(), src.getHeight(), m_format, m_type, src.getLevel(0).getDataPtr()));
	GLU_CHECK_GLW_CALL(gl, bindTexture(GL_TEXTURE_2D, 0));
	GLU_CHECK_GLW_CALL(gl, finish());

	tcu::copy(tcu::getSubregion(reference.getLevel(0), xOffset, yOffset, 0, xSize, ySize, 1), src.getLevel(0));

	return true;
}

bool GLES2ImageApi::ModifyRenderbuffer::invokeGLES2 (GLES2ImageApi& api, MovePtr<UniqueImage>& img, tcu::Texture2D& reference) const
{
	const glw::Functions&	gl				= api.m_gl;
	tcu::TestLog&			log				= api.getLog();
	glu::Framebuffer		framebuffer		(gl);
	glu::Renderbuffer		renderbuffer	(gl);

	log << tcu::TestLog::Message << "Modifying EGLImage with glClear to renderbuffer" << tcu::TestLog::EndMessage;

	GLU_CHECK_GLW_CALL(gl, bindFramebuffer(GL_FRAMEBUFFER, *framebuffer));
	GLU_CHECK_GLW_CALL(gl, bindRenderbuffer(GL_RENDERBUFFER, *renderbuffer));

	imageTargetRenderbuffer(api.m_egl, gl, **img);

	initializeRbo(api, *renderbuffer, reference);

	GLU_CHECK_GLW_CALL(gl, bindFramebuffer(GL_FRAMEBUFFER, 0));
	GLU_CHECK_GLW_CALL(gl, bindRenderbuffer(GL_RENDERBUFFER, 0));

	GLU_CHECK_GLW_CALL(gl, finish());

	return true;
}

void GLES2ImageApi::ModifyRenderbufferClearColor::initializeRbo (GLES2ImageApi& api, GLuint renderbuffer, tcu::Texture2D& reference) const
{
	const glw::Functions&	gl		= api.m_gl;

	framebufferRenderbuffer(gl, GL_COLOR_ATTACHMENT0, renderbuffer);

	GLU_CHECK_GLW_CALL(gl, viewport(0, 0, reference.getWidth(), reference.getHeight()));
	GLU_CHECK_GLW_CALL(gl, clearColor(m_color.x(), m_color.y(), m_color.z(), m_color.w()));
	GLU_CHECK_GLW_CALL(gl, clear(GL_COLOR_BUFFER_BIT));

	tcu::clear(reference.getLevel(0), m_color);
}

void GLES2ImageApi::ModifyRenderbufferClearDepth::initializeRbo (GLES2ImageApi& api, GLuint renderbuffer, tcu::Texture2D& reference) const
{
	const glw::Functions&	gl		= api.m_gl;

	framebufferRenderbuffer(gl, GL_DEPTH_ATTACHMENT, renderbuffer);

	GLU_CHECK_GLW_CALL(gl, viewport(0, 0, reference.getWidth(), reference.getHeight()));
	GLU_CHECK_GLW_CALL(gl, clearDepthf(m_depth));
	GLU_CHECK_GLW_CALL(gl, clear(GL_DEPTH_BUFFER_BIT));

	tcu::clearDepth(reference.getLevel(0), m_depth);
}

void GLES2ImageApi::ModifyRenderbufferClearStencil::initializeRbo (GLES2ImageApi& api, GLuint renderbuffer, tcu::Texture2D& reference) const
{
	const glw::Functions&	gl		= api.m_gl;

	framebufferRenderbuffer(gl, GL_STENCIL_ATTACHMENT, renderbuffer);

	GLU_CHECK_GLW_CALL(gl, viewport(0, 0, reference.getWidth(), reference.getHeight()));
	GLU_CHECK_GLW_CALL(gl, clearStencil(m_stencil));
	GLU_CHECK_GLW_CALL(gl, clear(GL_STENCIL_BUFFER_BIT));

	tcu::clearStencil(reference.getLevel(0), m_stencil);
}

class ImageFormatCase : public TestCase, private glu::CallLogWrapper
{
public:
						ImageFormatCase		(EglTestContext& eglTestCtx, const TestSpec& spec);
						~ImageFormatCase	(void);

	void				init				(void);
	void				deinit				(void);
	IterateResult		iterate				(void);
	void				checkExtensions		(void);

private:
	EGLConfig			getConfig			(void);

	const TestSpec		m_spec;

	vector<ImageApi*>	m_apiContexts;

	EGLDisplay			m_display;
	eglu::NativeWindow*	m_window;
	EGLSurface			m_surface;
	EGLConfig			m_config;
	int					m_curIter;
	MovePtr<UniqueImage>m_img;
	tcu::Texture2D		m_refImg;
	glw::Functions		m_gl;
};

EGLConfig ImageFormatCase::getConfig (void)
{
	const EGLint attribList[] =
	{
		EGL_RENDERABLE_TYPE,	EGL_OPENGL_ES2_BIT,
		EGL_SURFACE_TYPE,		EGL_WINDOW_BIT,
		EGL_RED_SIZE,			8,
		EGL_BLUE_SIZE,			8,
		EGL_GREEN_SIZE,			8,
		EGL_ALPHA_SIZE,			8,
		EGL_DEPTH_SIZE,			8,
		EGL_NONE
	};

	return eglu::chooseSingleConfig(m_eglTestCtx.getLibrary(), m_display, attribList);
}

ImageFormatCase::ImageFormatCase (EglTestContext& eglTestCtx, const TestSpec& spec)
	: TestCase				(eglTestCtx, spec.name.c_str(), spec.desc.c_str())
	, glu::CallLogWrapper	(m_gl, eglTestCtx.getTestContext().getLog())
	, m_spec				(spec)
	, m_display				(EGL_NO_DISPLAY)
	, m_window				(DE_NULL)
	, m_surface				(EGL_NO_SURFACE)
	, m_config				(0)
	, m_curIter				(0)
	, m_refImg				(tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNORM_INT8), 1, 1)
{
}

ImageFormatCase::~ImageFormatCase (void)
{
	deinit();
}

void ImageFormatCase::checkExtensions (void)
{
	const Library&			egl		= m_eglTestCtx.getLibrary();
	const EGLDisplay		dpy		= m_display;
	set<string>				exts;
	const vector<string>	glExts	= de::splitString((const char*) m_gl.getString(GL_EXTENSIONS));
	const vector<string>	eglExts	= eglu::getDisplayExtensions(egl, dpy);

	exts.insert(glExts.begin(), glExts.end());
	exts.insert(eglExts.begin(), eglExts.end());

	if (eglu::getVersion(egl, dpy) >= eglu::Version(1, 5))
	{
		// EGL 1.5 has built-in support for EGLImage and GL sources
		exts.insert("EGL_KHR_image_base");
		exts.insert("EGL_KHR_gl_texture_2D_image");
		exts.insert("EGL_KHR_gl_texture_cubemap_image");
		exts.insert("EGL_KHR_gl_renderbuffer_image");
	}

	if (!de::contains(exts, "EGL_KHR_image_base") && !de::contains(exts, "EGL_KHR_image"))
	{
		getLog() << tcu::TestLog::Message
				 << "EGL version is under 1.5 and neither EGL_KHR_image nor EGL_KHR_image_base is supported."
				 << "One should be supported."
				 << tcu::TestLog::EndMessage;
		TCU_THROW(NotSupportedError, "Extension not supported: EGL_KHR_image_base");
	}

	for (int operationNdx = 0; operationNdx < (int)m_spec.operations.size(); operationNdx++)
	{
		const TestSpec::Operation&	op	= m_spec.operations[operationNdx];
		const string				ext	= op.action->getRequiredExtension();

		if (!de::contains(exts, ext))
			TCU_THROW_EXPR(NotSupportedError, "Extension not supported", ext.c_str());
	}
}

void ImageFormatCase::init (void)
{
	const Library&						egl				= m_eglTestCtx.getLibrary();
	const eglu::NativeWindowFactory&	windowFactory	= eglu::selectNativeWindowFactory(m_eglTestCtx.getNativeDisplayFactory(), m_testCtx.getCommandLine());

	try
	{
		m_display	= eglu::getAndInitDisplay(m_eglTestCtx.getNativeDisplay());
		m_config	= getConfig();
		m_window	= windowFactory.createWindow(&m_eglTestCtx.getNativeDisplay(), m_display, m_config, DE_NULL, eglu::WindowParams(480, 480, eglu::parseWindowVisibility(m_testCtx.getCommandLine())));
		m_surface	= eglu::createWindowSurface(m_eglTestCtx.getNativeDisplay(), *m_window, m_display, m_config, DE_NULL);

		{
			const char* extensions[] = { "GL_OES_EGL_image" };
			m_eglTestCtx.initGLFunctions(&m_gl, glu::ApiType::es(2, 0), DE_LENGTH_OF_ARRAY(extensions), &extensions[0]);
		}

		for (int contextNdx = 0; contextNdx < (int)m_spec.contexts.size(); contextNdx++)
		{
			ImageApi* api = DE_NULL;
			switch (m_spec.contexts[contextNdx])
			{
				case TestSpec::API_GLES2:
				{
					api = new GLES2ImageApi(egl, m_gl, contextNdx, getLog(), m_display, m_surface, m_config);
					break;
				}

				default:
					DE_ASSERT(false);
					break;
			}
			m_apiContexts.push_back(api);
		}
		checkExtensions();
	}
	catch (...)
	{
		deinit();
		throw;
	}
}

void ImageFormatCase::deinit (void)
{
	const Library& egl = m_eglTestCtx.getLibrary();

	for (int contexNdx = 0 ; contexNdx < (int)m_apiContexts.size(); contexNdx++)
		delete m_apiContexts[contexNdx];

	m_apiContexts.clear();

	if (m_surface != EGL_NO_SURFACE)
	{
		egl.destroySurface(m_display, m_surface);
		m_surface = EGL_NO_SURFACE;
	}

	delete m_window;
	m_window = DE_NULL;

	if (m_display != EGL_NO_DISPLAY)
	{
		egl.terminate(m_display);
		m_display = EGL_NO_DISPLAY;
	}
}

TestCase::IterateResult ImageFormatCase::iterate (void)
{
	const TestSpec::Operation&	op		= m_spec.operations[m_curIter++];
	ImageApi&					api		= *m_apiContexts[op.apiIndex];
	const bool					isOk	= op.action->invoke(api, m_img, m_refImg);

	if (isOk && m_curIter < (int)m_spec.operations.size())
		return CONTINUE;
	else if (isOk)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");

	return STOP;
}

struct LabeledAction
{
	string			label;
	MovePtr<Action>	action;
};

// A simple vector mockup that we need because MovePtr isn't copy-constructible.
struct LabeledActions
{
					LabeledActions	(void) : m_numActions(0){}
	LabeledAction&	operator[]		(int ndx)				{ DE_ASSERT(0 <= ndx && ndx < m_numActions); return m_actions[ndx]; }
	void			add				(const string& label, MovePtr<Action> action);
	int				size			(void) const			{ return m_numActions; }
private:
	LabeledAction	m_actions[32];
	int				m_numActions;
};

void LabeledActions::add (const string& label, MovePtr<Action> action)
{
	DE_ASSERT(m_numActions < DE_LENGTH_OF_ARRAY(m_actions));
	m_actions[m_numActions].label = label;
	m_actions[m_numActions].action = action;
	++m_numActions;
}

class ImageTests : public TestCaseGroup
{
protected:
					ImageTests						(EglTestContext& eglTestCtx, const string& name, const string& desc)
						: TestCaseGroup(eglTestCtx, name.c_str(), desc.c_str()) {}

	void			addCreateTexture				(const string& name, EGLenum source, GLenum internalFormat, GLenum format, GLenum type);
	void			addCreateRenderbuffer			(const string& name, GLenum format);
	void			addCreateAndroidNative			(const string& name, GLenum format);
	void			addCreateTexture2DActions		(const string& prefix);
	void			addCreateTextureCubemapActions	(const string& suffix, GLenum internalFormat, GLenum format, GLenum type);
	void			addCreateRenderbufferActions	(void);
	void			addCreateAndroidNativeActions	(void);

	LabeledActions	m_createActions;
};

void ImageTests::addCreateTexture (const string& name, EGLenum source, GLenum internalFormat, GLenum format, GLenum type)
{
	m_createActions.add(name, MovePtr<Action>(new GLES2ImageApi::Create(createTextureImageSource(source, internalFormat, format, type))));
}

void ImageTests::addCreateRenderbuffer (const string& name, GLenum format)
{
	m_createActions.add(name, MovePtr<Action>(new GLES2ImageApi::Create(createRenderbufferImageSource(format))));
}

void ImageTests::addCreateAndroidNative (const string& name, GLenum format)
{
	m_createActions.add(name, MovePtr<Action>(new GLES2ImageApi::Create(createAndroidNativeImageSource(format))));
}

void ImageTests::addCreateTexture2DActions (const string& prefix)
{
	addCreateTexture(prefix + "rgb8",		EGL_GL_TEXTURE_2D_KHR,	GL_RGB,		GL_RGB,		GL_UNSIGNED_BYTE);
	addCreateTexture(prefix + "rgb565",		EGL_GL_TEXTURE_2D_KHR,	GL_RGB,		GL_RGB,		GL_UNSIGNED_SHORT_5_6_5);
	addCreateTexture(prefix + "rgba8",		EGL_GL_TEXTURE_2D_KHR,	GL_RGBA,	GL_RGBA,	GL_UNSIGNED_BYTE);
	addCreateTexture(prefix + "rgb5_a1",	EGL_GL_TEXTURE_2D_KHR,	GL_RGBA,	GL_RGBA,	GL_UNSIGNED_SHORT_5_5_5_1);
	addCreateTexture(prefix + "rgba4",		EGL_GL_TEXTURE_2D_KHR,	GL_RGBA,	GL_RGBA,	GL_UNSIGNED_SHORT_4_4_4_4);
}

void ImageTests::addCreateTextureCubemapActions (const string& suffix, GLenum internalFormat, GLenum format, GLenum type)
{
	addCreateTexture("cubemap_positive_x" + suffix,	EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_X_KHR, internalFormat,	format,	type);
	addCreateTexture("cubemap_positive_y" + suffix,	EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Y_KHR, internalFormat,	format,	type);
	addCreateTexture("cubemap_positive_z" + suffix,	EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Z_KHR, internalFormat,	format,	type);
	addCreateTexture("cubemap_negative_x" + suffix,	EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_X_KHR, internalFormat,	format,	type);
	addCreateTexture("cubemap_negative_y" + suffix,	EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_KHR, internalFormat,	format,	type);
	addCreateTexture("cubemap_negative_z" + suffix,	EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_KHR, internalFormat,	format,	type);
}

void ImageTests::addCreateRenderbufferActions (void)
{
	addCreateRenderbuffer("renderbuffer_rgba4",		GL_RGBA4);
	addCreateRenderbuffer("renderbuffer_rgb5_a1",	GL_RGB5_A1);
	addCreateRenderbuffer("renderbuffer_rgb565",	GL_RGB565);
	addCreateRenderbuffer("renderbuffer_depth16",	GL_DEPTH_COMPONENT16);
	addCreateRenderbuffer("renderbuffer_stencil",	GL_STENCIL_INDEX8);
}

void ImageTests::addCreateAndroidNativeActions (void)
{
	addCreateAndroidNative("android_native_rgb565",		GL_RGB565);
	addCreateAndroidNative("android_native_rgb8",		GL_RGB8);
	addCreateAndroidNative("android_native_rgba4",		GL_RGBA4);
	addCreateAndroidNative("android_native_rgb5_a1",	GL_RGB5_A1);
	addCreateAndroidNative("android_native_rgba8",		GL_RGBA8);
}

class RenderTests : public ImageTests
{
protected:
											RenderTests				(EglTestContext& eglTestCtx, const string& name, const string& desc)
												: ImageTests			(eglTestCtx, name, desc) {}

	void									addRenderActions		(void);
	LabeledActions							m_renderActions;
};

void RenderTests::addRenderActions (void)
{
	m_renderActions.add("texture",			MovePtr<Action>(new GLES2ImageApi::RenderTexture2D()));
	m_renderActions.add("read_pixels",		MovePtr<Action>(new GLES2ImageApi::RenderReadPixelsRenderbuffer()));
	m_renderActions.add("depth_buffer",		MovePtr<Action>(new GLES2ImageApi::RenderDepthbuffer()));
	m_renderActions.add("stencil_buffer",	MovePtr<Action>(new GLES2ImageApi::RenderStencilbuffer()));
}

class SimpleCreationTests : public RenderTests
{
public:
			SimpleCreationTests		(EglTestContext& eglTestCtx, const string& name, const string& desc) : RenderTests(eglTestCtx, name, desc) {}
	void	init					(void);
};

bool isDepthFormat (GLenum format)
{
	switch (format)
	{
		case GL_RGB:
		case GL_RGB8:
		case GL_RGB565:
		case GL_RGBA:
		case GL_RGBA4:
		case GL_RGBA8:
		case GL_RGB5_A1:
			return false;

		case GL_DEPTH_COMPONENT16:
			return true;

		case GL_STENCIL_INDEX8:
			return false;

		default:
			DE_ASSERT(false);
			return false;
	}
}

bool isStencilFormat (GLenum format)
{
	switch (format)
	{
		case GL_RGB:
		case GL_RGB8:
		case GL_RGB565:
		case GL_RGBA:
		case GL_RGBA4:
		case GL_RGBA8:
		case GL_RGB5_A1:
			return false;

		case GL_DEPTH_COMPONENT16:
			return false;

		case GL_STENCIL_INDEX8:
			return true;

		default:
			DE_ASSERT(false);
			return false;
	}
}

bool isCompatibleCreateAndRenderActions (const Action& create, const Action& render)
{
	if (const GLES2ImageApi::Create* gles2Create = dynamic_cast<const GLES2ImageApi::Create*>(&create))
	{
		const GLenum createFormat = gles2Create->getEffectiveFormat();

		if (dynamic_cast<const GLES2ImageApi::RenderTexture2D*>(&render))
		{
			// GLES2 does not have depth or stencil textures
			if (isDepthFormat(createFormat) || isStencilFormat(createFormat))
				return false;
		}

		if (dynamic_cast<const GLES2ImageApi::RenderReadPixelsRenderbuffer*>(&render))
		{
			// GLES2 does not support readPixels for depth or stencil
			if (isDepthFormat(createFormat) || isStencilFormat(createFormat))
				return false;
		}

		if (dynamic_cast<const GLES2ImageApi::RenderDepthbuffer*>(&render))
		{
			// Copying non-depth data to depth renderbuffer and expecting meaningful
			// results just doesn't make any sense.
			if (!isDepthFormat(createFormat))
				return false;
		}

		if (dynamic_cast<const GLES2ImageApi::RenderStencilbuffer*>(&render))
		{
			// Copying non-stencil data to stencil renderbuffer and expecting meaningful
			// results just doesn't make any sense.
			if (!isStencilFormat(createFormat))
				return false;
		}

		return true;
	}
	else
		DE_ASSERT(false);

	return false;
}

void SimpleCreationTests::init (void)
{
	addCreateTexture2DActions("texture_");
	addCreateTextureCubemapActions("_rgba", GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);
	addCreateTextureCubemapActions("_rgb", GL_RGB, GL_RGB, GL_UNSIGNED_BYTE);
	addCreateRenderbufferActions();
	addCreateAndroidNativeActions();
	addRenderActions();

	for (int createNdx = 0; createNdx < m_createActions.size(); createNdx++)
	{
		const LabeledAction& createAction = m_createActions[createNdx];

		for (int renderNdx = 0; renderNdx < m_renderActions.size(); renderNdx++)
		{
			const LabeledAction&	renderAction	= m_renderActions[renderNdx];
			TestSpec				spec;

			if (!isCompatibleCreateAndRenderActions(*createAction.action, *renderAction.action))
				continue;

			spec.name = std::string("gles2_") + createAction.label + "_" + renderAction.label;
			spec.desc = spec.name;
			spec.contexts.push_back(TestSpec::API_GLES2);
			spec.operations.push_back(TestSpec::Operation(0, *createAction.action));
			spec.operations.push_back(TestSpec::Operation(0, *renderAction.action));

			addChild(new ImageFormatCase(m_eglTestCtx, spec));
		}
	}
}

TestCaseGroup* createSimpleCreationTests (EglTestContext& eglTestCtx, const string& name, const string& desc)
{
	return new SimpleCreationTests(eglTestCtx, name, desc);
}

bool isCompatibleFormats (GLenum createFormat, GLenum modifyFormat, GLenum modifyType)
{
	switch (modifyFormat)
	{
		case GL_RGB:
			switch (modifyType)
			{
				case GL_UNSIGNED_BYTE:
					return createFormat == GL_RGB
							|| createFormat == GL_RGB8
							|| createFormat == GL_RGB565
							|| createFormat == GL_SRGB8;

				case GL_BYTE:
					return createFormat == GL_RGB8_SNORM;

				case GL_UNSIGNED_SHORT_5_6_5:
					return createFormat == GL_RGB
							|| createFormat == GL_RGB565;

				case GL_UNSIGNED_INT_10F_11F_11F_REV:
					return createFormat == GL_R11F_G11F_B10F;

				case GL_UNSIGNED_INT_5_9_9_9_REV:
					return createFormat == GL_RGB9_E5;

				case GL_HALF_FLOAT:
					return createFormat == GL_RGB16F
							|| createFormat == GL_R11F_G11F_B10F
							|| createFormat == GL_RGB9_E5;

				case GL_FLOAT:
					return createFormat == GL_RGB16F
							|| createFormat == GL_RGB32F
							|| createFormat == GL_R11F_G11F_B10F
							|| createFormat == GL_RGB9_E5;

				default:
					DE_FATAL("Unknown modify type");
					return false;
			}

		case GL_RGBA:
			switch (modifyType)
			{
				case GL_UNSIGNED_BYTE:
					return createFormat == GL_RGBA8
						|| createFormat == GL_RGB5_A1
						|| createFormat == GL_RGBA4
						|| createFormat == GL_SRGB8_ALPHA8
						|| createFormat == GL_RGBA;

				case GL_UNSIGNED_SHORT_4_4_4_4:
					return createFormat == GL_RGBA4
						|| createFormat == GL_RGBA;

				case GL_UNSIGNED_SHORT_5_5_5_1:
					return createFormat == GL_RGB5_A1
						|| createFormat == GL_RGBA;

				case GL_UNSIGNED_INT_2_10_10_10_REV:
					return createFormat == GL_RGB10_A2
						|| createFormat == GL_RGB5_A1;

				case GL_HALF_FLOAT:
					return createFormat == GL_RGBA16F;

				case GL_FLOAT:
					return createFormat == GL_RGBA16F
						|| createFormat == GL_RGBA32F;

				default:
					DE_FATAL("Unknown modify type");
					return false;
			};

		default:
			DE_FATAL("Unknown modify format");
			return false;
	}
}

bool isCompatibleCreateAndModifyActions (const Action& create, const Action& modify)
{
	if (const GLES2ImageApi::Create* gles2Create = dynamic_cast<const GLES2ImageApi::Create*>(&create))
	{
		const GLenum createFormat = gles2Create->getEffectiveFormat();

		if (const GLES2ImageApi::ModifyTexSubImage* gles2TexSubImageModify = dynamic_cast<const GLES2ImageApi::ModifyTexSubImage*>(&modify))
		{
			const GLenum modifyFormat	= gles2TexSubImageModify->getFormat();
			const GLenum modifyType		= gles2TexSubImageModify->getType();

			return isCompatibleFormats(createFormat, modifyFormat, modifyType);
		}

		if (dynamic_cast<const GLES2ImageApi::ModifyRenderbufferClearColor*>(&modify))
		{
			// reintepreting color as non-color is not meaningful
			if (isDepthFormat(createFormat) || isStencilFormat(createFormat))
				return false;
		}

		if (dynamic_cast<const GLES2ImageApi::ModifyRenderbufferClearDepth*>(&modify))
		{
			// reintepreting depth as non-depth is not meaningful
			if (!isDepthFormat(createFormat))
				return false;
		}

		if (dynamic_cast<const GLES2ImageApi::ModifyRenderbufferClearStencil*>(&modify))
		{
			// reintepreting stencil as non-stencil is not meaningful
			if (!isStencilFormat(createFormat))
				return false;
		}

		return true;
	}
	else
		DE_ASSERT(false);

	return false;
}

class MultiContextRenderTests : public RenderTests
{
public:
					MultiContextRenderTests		(EglTestContext& eglTestCtx, const string& name, const string& desc);
	void			init						(void);
	void			addClearActions				(void);
private:
	LabeledActions	m_clearActions;
};

MultiContextRenderTests::MultiContextRenderTests (EglTestContext& eglTestCtx, const string& name, const string& desc)
	: RenderTests	(eglTestCtx, name, desc)
{
}

void MultiContextRenderTests::addClearActions (void)
{
	m_clearActions.add("renderbuffer_clear_color",		MovePtr<Action>(new GLES2ImageApi::ModifyRenderbufferClearColor(tcu::Vec4(0.8f, 0.2f, 0.9f, 1.0f))));
	m_clearActions.add("renderbuffer_clear_depth",		MovePtr<Action>(new GLES2ImageApi::ModifyRenderbufferClearDepth(0.75f)));
	m_clearActions.add("renderbuffer_clear_stencil",	MovePtr<Action>(new GLES2ImageApi::ModifyRenderbufferClearStencil(97)));
}

void MultiContextRenderTests::init (void)
{
	addCreateTexture2DActions("texture_");
	addCreateTextureCubemapActions("_rgba8", GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);
	addCreateTextureCubemapActions("_rgb8", GL_RGB, GL_RGB, GL_UNSIGNED_BYTE);
	addCreateRenderbufferActions();
	addCreateAndroidNativeActions();
	addRenderActions();
	addClearActions();

	for (int createNdx = 0; createNdx < m_createActions.size(); createNdx++)
	for (int renderNdx = 0; renderNdx < m_renderActions.size(); renderNdx++)
	for (int clearNdx = 0; clearNdx < m_clearActions.size(); clearNdx++)
	{
		const LabeledAction&	createAction	= m_createActions[createNdx];
		const LabeledAction&	renderAction	= m_renderActions[renderNdx];
		const LabeledAction&	clearAction		= m_clearActions[clearNdx];
		TestSpec				spec;

		if (!isCompatibleCreateAndRenderActions(*createAction.action, *renderAction.action))
			continue;
		if (!isCompatibleCreateAndModifyActions(*createAction.action, *clearAction.action))
			continue;

		spec.name = std::string("gles2_") + createAction.label + "_" + renderAction.label;
		spec.desc = spec.name;

		spec.contexts.push_back(TestSpec::API_GLES2);
		spec.contexts.push_back(TestSpec::API_GLES2);

		spec.operations.push_back(TestSpec::Operation(0, *createAction.action));
		spec.operations.push_back(TestSpec::Operation(0, *renderAction.action));
		spec.operations.push_back(TestSpec::Operation(0, *clearAction.action));
		spec.operations.push_back(TestSpec::Operation(1, *createAction.action));
		spec.operations.push_back(TestSpec::Operation(0, *renderAction.action));
		spec.operations.push_back(TestSpec::Operation(1, *renderAction.action));

		addChild(new ImageFormatCase(m_eglTestCtx, spec));
	}
}

TestCaseGroup* createMultiContextRenderTests (EglTestContext& eglTestCtx, const string& name, const string& desc)
{
	return new MultiContextRenderTests(eglTestCtx, name, desc);
}

class ModifyTests : public ImageTests
{
public:
								ModifyTests		(EglTestContext& eglTestCtx, const string& name, const string& desc)
									: ImageTests(eglTestCtx, name, desc) {}

	void						init			(void);

protected:
	void						addModifyActions(void);

	LabeledActions				m_modifyActions;
	GLES2ImageApi::RenderTryAll	m_renderAction;
};

void ModifyTests::addModifyActions (void)
{
	m_modifyActions.add("tex_subimage_rgb8",			MovePtr<Action>(new GLES2ImageApi::ModifyTexSubImage(GL_RGB,	GL_UNSIGNED_BYTE)));
	m_modifyActions.add("tex_subimage_rgb565",			MovePtr<Action>(new GLES2ImageApi::ModifyTexSubImage(GL_RGB,	GL_UNSIGNED_SHORT_5_6_5)));
	m_modifyActions.add("tex_subimage_rgba8",			MovePtr<Action>(new GLES2ImageApi::ModifyTexSubImage(GL_RGBA,	GL_UNSIGNED_BYTE)));
	m_modifyActions.add("tex_subimage_rgb5_a1",			MovePtr<Action>(new GLES2ImageApi::ModifyTexSubImage(GL_RGBA,	GL_UNSIGNED_SHORT_5_5_5_1)));
	m_modifyActions.add("tex_subimage_rgba4",			MovePtr<Action>(new GLES2ImageApi::ModifyTexSubImage(GL_RGBA,	GL_UNSIGNED_SHORT_4_4_4_4)));

	m_modifyActions.add("renderbuffer_clear_color",		MovePtr<Action>(new GLES2ImageApi::ModifyRenderbufferClearColor(tcu::Vec4(0.3f, 0.5f, 0.3f, 1.0f))));
	m_modifyActions.add("renderbuffer_clear_depth",		MovePtr<Action>(new GLES2ImageApi::ModifyRenderbufferClearDepth(0.7f)));
	m_modifyActions.add("renderbuffer_clear_stencil",	MovePtr<Action>(new GLES2ImageApi::ModifyRenderbufferClearStencil(78)));
}

void ModifyTests::init (void)
{
	addCreateTexture2DActions("tex_");
	addCreateRenderbufferActions();
	addCreateAndroidNativeActions();
	addModifyActions();

	for (int createNdx = 0; createNdx < m_createActions.size(); createNdx++)
	{
		LabeledAction& createAction = m_createActions[createNdx];

		for (int modifyNdx = 0; modifyNdx < m_modifyActions.size(); modifyNdx++)
		{
			LabeledAction& modifyAction = m_modifyActions[modifyNdx];

			if (!isCompatibleCreateAndModifyActions(*createAction.action, *modifyAction.action))
				continue;

			TestSpec spec;
			spec.name = createAction.label + "_" + modifyAction.label;
			spec.desc = "gles2_tex_sub_image";

			spec.contexts.push_back(TestSpec::API_GLES2);

			spec.operations.push_back(TestSpec::Operation(0, *createAction.action));
			spec.operations.push_back(TestSpec::Operation(0, m_renderAction));
			spec.operations.push_back(TestSpec::Operation(0, *modifyAction.action));
			spec.operations.push_back(TestSpec::Operation(0, m_renderAction));

			addChild(new ImageFormatCase(m_eglTestCtx, spec));
		}
	}
}

TestCaseGroup* createModifyTests (EglTestContext& eglTestCtx, const string& name, const string& desc)
{
	return new ModifyTests(eglTestCtx, name, desc);
}

} // Image
} // egl
} // deqp
