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

#include "teglImageTests.hpp"

#include "teglImageUtil.hpp"
#include "teglAndroidUtil.hpp"
#include "teglImageFormatTests.hpp"

#include "egluNativeDisplay.hpp"
#include "egluNativeWindow.hpp"
#include "egluNativePixmap.hpp"
#include "egluStrUtil.hpp"
#include "egluUnique.hpp"
#include "egluUtil.hpp"
#include "egluGLUtil.hpp"

#include "eglwLibrary.hpp"
#include "eglwEnums.hpp"

#include "gluDefs.hpp"
#include "gluCallLogWrapper.hpp"
#include "gluObjectWrapper.hpp"
#include "gluStrUtil.hpp"

#include "glwDefs.hpp"
#include "glwEnums.hpp"

#include "tcuTestLog.hpp"
#include "tcuCommandLine.hpp"

#include "deUniquePtr.hpp"

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>
#include <set>

using tcu::TestLog;

using std::string;
using std::vector;
using std::set;
using std::ostringstream;

using de::MovePtr;
using de::UniquePtr;
using glu::ApiType;
using glu::ContextType;
using glu::Texture;
using eglu::AttribMap;
using eglu::NativeWindow;
using eglu::NativePixmap;
using eglu::UniqueImage;
using eglu::UniqueSurface;
using eglu::ScopedCurrentContext;

using namespace glw;
using namespace eglw;

namespace deqp
{
namespace egl
{

namespace Image
{

#define CHECK_EXTENSION(DPY, EXTNAME) \
	TCU_CHECK_AND_THROW(NotSupportedError, eglu::hasExtension(m_eglTestCtx.getLibrary(), DPY, EXTNAME), (string("Unsupported extension: ") + (EXTNAME)).c_str())

template <typename RetVal>
RetVal checkCallError (EglTestContext& eglTestCtx, const char* call, RetVal returnValue, EGLint expectError)
{
	tcu::TestContext&	testCtx		= eglTestCtx.getTestContext();
	TestLog&			log			= testCtx.getLog();
	EGLint				error;

	log << TestLog::Message << call << TestLog::EndMessage;

	error = eglTestCtx.getLibrary().getError();

	if (error != expectError)
	{
		log << TestLog::Message << "  Fail: Error code mismatch! Expected " << eglu::getErrorStr(expectError) << ", got " << eglu::getErrorStr(error) << TestLog::EndMessage;
		log << TestLog::Message << "  " << returnValue << " was returned" << TestLog::EndMessage;

		if (testCtx.getTestResult() == QP_TEST_RESULT_PASS)
			testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid error code");
	}

	return returnValue;
}

template <typename RetVal>
void checkCallReturn (EglTestContext& eglTestCtx, const char* call, RetVal returnValue, RetVal expectReturnValue, EGLint expectError)
{
	tcu::TestContext&	testCtx		= eglTestCtx.getTestContext();
	TestLog&			log			= testCtx.getLog();
	EGLint				error;

	log << TestLog::Message << call << TestLog::EndMessage;

	error = eglTestCtx.getLibrary().getError();

	if (returnValue != expectReturnValue)
	{
		log << TestLog::Message << "  Fail: Return value mismatch! Expected " << expectReturnValue << ", got " << returnValue << TestLog::EndMessage;

		if (testCtx.getTestResult() == QP_TEST_RESULT_PASS)
			testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid return value");
	}

	if (error != expectError)
	{
		log << TestLog::Message << "  Fail: Error code mismatch! Expected " << eglu::getErrorStr(expectError) << ", got " << eglu::getErrorStr(error) << TestLog::EndMessage;

		if (testCtx.getTestResult() == QP_TEST_RESULT_PASS)
			testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid error code");
	}
}

// \note These macros expect "EglTestContext m_eglTestCtx" to be defined.
#define CHECK_EXT_CALL_RET(CALL, EXPECT_RETURN_VALUE, EXPECT_ERROR)	checkCallReturn(m_eglTestCtx, #CALL, CALL, (EXPECT_RETURN_VALUE), (EXPECT_ERROR))
#define CHECK_EXT_CALL_ERR(CALL, EXPECT_ERROR)						checkCallError(m_eglTestCtx, #CALL, CALL, (EXPECT_ERROR))

class ImageTestCase : public TestCase, public glu::CallLogWrapper
{
public:
				ImageTestCase		(EglTestContext& eglTestCtx, ApiType api, const string& name, const string& desc)
					: TestCase				(eglTestCtx, name.c_str(), desc.c_str())
					, glu::CallLogWrapper	(m_gl, m_testCtx.getLog())
					, m_api					(api)
					, m_display				(EGL_NO_DISPLAY)
	{
	}

	void		init				(void)
	{
		DE_ASSERT(m_display == EGL_NO_DISPLAY);
		m_display = eglu::getAndInitDisplay(m_eglTestCtx.getNativeDisplay());

		const char* extensions[] = { "GL_OES_EGL_image" };
		m_eglTestCtx.initGLFunctions(&m_gl, m_api, DE_LENGTH_OF_ARRAY(extensions), &extensions[0]);
	}

	void		deinit				(void)
	{
		m_eglTestCtx.getLibrary().terminate(m_display);
		m_display = EGL_NO_DISPLAY;
	}

	bool		isGLRedSupported	(void)
	{
		return m_api.getMajorVersion() >= 3 || glu::hasExtension(m_gl, m_api, "GL_EXT_texture_rg");
	}

protected:
	glw::Functions	m_gl;
	ApiType			m_api;
	EGLDisplay		m_display;
};

class InvalidCreateImage : public ImageTestCase
{
public:
	InvalidCreateImage (EglTestContext& eglTestCtx)
		: ImageTestCase(eglTestCtx, ApiType::es(2, 0), "invalid_create_image", "eglCreateImageKHR() with invalid arguments")
	{
	}

	void checkCreate (const char* desc, EGLDisplay dpy, const char* dpyStr, EGLContext context, const char* ctxStr, EGLenum source, const char* srcStr, EGLint expectError);

	IterateResult iterate (void)
	{
		const Library& egl = m_eglTestCtx.getLibrary();

		if (eglu::getVersion(egl, m_display) < eglu::Version(1, 5) &&
			!eglu::hasExtension(egl, m_display, "EGL_KHR_image") &&
			!eglu::hasExtension(egl, m_display, "EGL_KHR_image_base"))
		{
			TCU_THROW(NotSupportedError, "EGLimages not supported");
		}

#define CHECK_CREATE(MSG, DPY, CONTEXT, SOURCE, ERR) checkCreate(MSG, DPY, #DPY, CONTEXT, #CONTEXT, SOURCE, #SOURCE, ERR)
		CHECK_CREATE("Testing bad display (-1)...", (EGLDisplay)-1, EGL_NO_CONTEXT, EGL_NONE, EGL_BAD_DISPLAY);
		CHECK_CREATE("Testing bad context (-1)...", m_display, (EGLContext)-1, EGL_NONE, EGL_BAD_CONTEXT);
		CHECK_CREATE("Testing bad source (-1)...", m_display, EGL_NO_CONTEXT, (EGLenum)-1, EGL_BAD_PARAMETER);
#undef CHECK_CREATE

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		return STOP;
	}

};

void InvalidCreateImage::checkCreate (const char* msg, EGLDisplay dpy, const char* dpyStr, EGLContext context, const char* ctxStr, EGLenum source, const char* srcStr, EGLint expectError)
{
	m_testCtx.getLog() << TestLog::Message << msg << TestLog::EndMessage;
	{
		const Library&		egl		= m_eglTestCtx.getLibrary();
		const EGLImageKHR	image	= egl.createImageKHR(dpy, context, source, 0, DE_NULL);
		ostringstream		call;

		call << "eglCreateImage(" << dpyStr << ", " << ctxStr << ", " << srcStr << ", 0, DE_NULL)";
		checkCallReturn(m_eglTestCtx, call.str().c_str(), image, EGL_NO_IMAGE_KHR, expectError);
	}
}

EGLConfig chooseConfig (const Library& egl, EGLDisplay display, ApiType apiType)
{
	AttribMap				attribs;
	vector<EGLConfig>		configs;
	// Prefer configs in order: pbuffer, window, pixmap
	static const EGLenum	s_surfaceTypes[] = { EGL_PBUFFER_BIT, EGL_WINDOW_BIT, EGL_PIXMAP_BIT };

	attribs[EGL_RENDERABLE_TYPE] = eglu::apiRenderableType(apiType);

	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(s_surfaceTypes); ++ndx)
	{
		attribs[EGL_SURFACE_TYPE] = s_surfaceTypes[ndx];
		configs = eglu::chooseConfigs(egl, display, attribs);

		if (!configs.empty())
			return configs.front();
	}

	TCU_THROW(NotSupportedError, "No compatible EGL configs found");
	return (EGLConfig)0;
}

class Context
{
public:
								Context			(EglTestContext& eglTestCtx, EGLDisplay display, ContextType ctxType, int width, int height)
									: m_eglTestCtx	(eglTestCtx)
									, m_display		(display)
									, m_config		(chooseConfig(eglTestCtx.getLibrary(), display, ctxType.getAPI()))
									, m_context		(m_eglTestCtx.getLibrary(), m_display, eglu::createGLContext(eglTestCtx.getLibrary(), m_display, m_config, ctxType))
									, m_surface		(createSurface(eglTestCtx, m_display, m_config, width, height))
									, m_current		(eglTestCtx.getLibrary(), m_display, m_surface->get(), m_surface->get(), *m_context)
	{
		m_eglTestCtx.initGLFunctions(&m_gl, ctxType.getAPI());
	}

	EGLConfig					getConfig		(void) const { return m_config; }
	EGLDisplay					getEglDisplay	(void) const { return m_display; }
	EGLContext					getEglContext	(void) const { return *m_context; }
	const glw::Functions&		gl				(void) const { return m_gl; }

private:
	EglTestContext&				m_eglTestCtx;
	EGLDisplay					m_display;
	EGLConfig					m_config;
	eglu::UniqueContext			m_context;
	UniquePtr<ManagedSurface>	m_surface;
	ScopedCurrentContext		m_current;
	glw::Functions				m_gl;

								Context			(const Context&);
	Context&					operator=		(const Context&);
};

class CreateImageGLES2 : public ImageTestCase
{
public:
	static const char* getTargetName (EGLint target)
	{
		switch (target)
		{
			case EGL_GL_TEXTURE_2D_KHR:						return "tex2d";
			case EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_X_KHR:	return "cubemap_pos_x";
			case EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_X_KHR:	return "cubemap_neg_x";
			case EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Y_KHR:	return "cubemap_pos_y";
			case EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_KHR:	return "cubemap_neg_y";
			case EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Z_KHR:	return "cubemap_pos_z";
			case EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_KHR:	return "cubemap_neg_z";
			case EGL_GL_RENDERBUFFER_KHR:					return "renderbuffer";
			case EGL_NATIVE_BUFFER_ANDROID:					return "android_native";
			default:		DE_ASSERT(DE_FALSE);			return "";
		}
	}

	static const char* getStorageName (GLenum storage)
	{
		switch (storage)
		{
			case GL_RED:				return "red";
			case GL_RG:					return "rg";
			case GL_LUMINANCE:			return "luminance";
			case GL_LUMINANCE_ALPHA:	return "luminance_alpha";
			case GL_RGB:				return "rgb";
			case GL_RGBA:				return "rgba";
			case GL_DEPTH_COMPONENT16:	return "depth_component_16";
			case GL_RGBA4:				return "rgba4";
			case GL_RGB5_A1:			return "rgb5_a1";
			case GL_RGB565:				return "rgb565";
			case GL_RGB8:				return "rgb8";
			case GL_RGBA8:				return "rgba8";
			case GL_STENCIL_INDEX8:		return "stencil_index8";
			default:
				DE_ASSERT(DE_FALSE);
				return "";
		}
	}

	MovePtr<ImageSource> getImageSource (EGLint target, GLenum internalFormat, GLenum format, GLenum type, bool useTexLevel0)
	{
		switch (target)
		{
			case EGL_GL_TEXTURE_2D_KHR:
			case EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_X_KHR:
			case EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_X_KHR:
			case EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Y_KHR:
			case EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_KHR:
			case EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Z_KHR:
			case EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_KHR:
				DE_ASSERT(format != 0u && type != 0u);
				return createTextureImageSource(target, internalFormat, format, type, useTexLevel0);

			case EGL_GL_RENDERBUFFER_KHR:
				DE_ASSERT(format == 0u && type == 0u);
				return createRenderbufferImageSource(internalFormat);

			case EGL_NATIVE_BUFFER_ANDROID:
				DE_ASSERT(format == 0u && type == 0u);
				return createAndroidNativeImageSource(internalFormat);

			default:
				DE_FATAL("Impossible");
				return MovePtr<ImageSource>();
		}
	}

	CreateImageGLES2 (EglTestContext& eglTestCtx, EGLint target, GLenum internalFormat, GLenum format, GLenum type, bool useTexLevel0 = false)
		: ImageTestCase		(eglTestCtx, ApiType::es(2, 0), string("create_image_gles2_") + getTargetName(target) + "_" + getStorageName(internalFormat) + (useTexLevel0 ? "_level0_only" : ""), "Create EGLImage from GLES2 object")
		, m_source			(getImageSource(target, internalFormat, format, type, useTexLevel0))
		, m_internalFormat	(internalFormat)
	{
	}

	IterateResult iterate (void)
	{
		const Library&			egl				= m_eglTestCtx.getLibrary();
		const EGLDisplay		dpy				= m_display;

		if (eglu::getVersion(egl, dpy) < eglu::Version(1, 5))
			CHECK_EXTENSION(dpy, m_source->getRequiredExtension());

		// Initialize result.
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

		// Create GLES2 context
		TestLog&				log				= m_testCtx.getLog();
		const ContextType		contextType		(ApiType::es(2, 0));
		Context					context			(m_eglTestCtx, dpy, contextType, 64, 64);
		const EGLContext		eglContext		= context.getEglContext();

		if ((m_internalFormat == GL_RED || m_internalFormat == GL_RG) && !isGLRedSupported())
			TCU_THROW(NotSupportedError, "Unsupported extension: GL_EXT_texture_rg");

		log << TestLog::Message << "Using EGL config " << eglu::getConfigID(egl, dpy, context.getConfig()) << TestLog::EndMessage;

		UniquePtr<ClientBuffer>	clientBuffer	(m_source->createBuffer(context.gl()));
		const EGLImageKHR		image			= m_source->createImage(egl, dpy, eglContext, clientBuffer->get());

		if (image == EGL_NO_IMAGE_KHR)
		{
			log << TestLog::Message << "  Fail: Got EGL_NO_IMAGE_KHR!" << TestLog::EndMessage;

			if (m_testCtx.getTestResult() == QP_TEST_RESULT_PASS)
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Got EGL_NO_IMAGE_KHR");
		}

		// Destroy image
		CHECK_EXT_CALL_RET(egl.destroyImageKHR(context.getEglDisplay(), image), (EGLBoolean)EGL_TRUE, EGL_SUCCESS);

		return STOP;
	}

private:
	const UniquePtr<ImageSource>	m_source;
	const GLenum					m_internalFormat;
};

class ImageTargetGLES2 : public ImageTestCase
{
public:
	static const char* getTargetName (GLenum target)
	{
		switch (target)
		{
			case GL_TEXTURE_2D:		return "tex2d";
			case GL_RENDERBUFFER:	return "renderbuffer";
			default:
				DE_ASSERT(DE_FALSE);
				return "";
		}
	}

	ImageTargetGLES2 (EglTestContext& eglTestCtx, GLenum target)
		: ImageTestCase	(eglTestCtx, ApiType::es(2, 0), string("image_target_gles2_") + getTargetName(target), "Use EGLImage as GLES2 object")
		, m_target		(target)
	{
	}

	IterateResult iterate (void)
	{
		const Library&	egl	= m_eglTestCtx.getLibrary();
		TestLog&		log	= m_testCtx.getLog();

		// \todo [2011-07-21 pyry] Try all possible EGLImage sources
		CHECK_EXTENSION(m_display, "EGL_KHR_gl_texture_2D_image");

		// Initialize result.
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

		// Create GLES2 context

		Context context(m_eglTestCtx, m_display, ContextType(ApiType::es(2, 0)), 64, 64);
		log << TestLog::Message << "Using EGL config " << eglu::getConfigID(m_eglTestCtx.getLibrary(), context.getEglDisplay(), context.getConfig()) << TestLog::EndMessage;

		// Check for OES_EGL_image
		{
			const char* glExt = (const char*)glGetString(GL_EXTENSIONS);

			if (string(glExt).find("GL_OES_EGL_image") == string::npos)
				throw tcu::NotSupportedError("Extension not supported", "GL_OES_EGL_image", __FILE__, __LINE__);

			TCU_CHECK(m_gl.eglImageTargetTexture2DOES);
			TCU_CHECK(m_gl.eglImageTargetRenderbufferStorageOES);
		}

		// Create GL_TEXTURE_2D and EGLImage from it.
		log << TestLog::Message << "Creating EGLImage using GL_TEXTURE_2D with GL_RGBA storage" << TestLog::EndMessage;

		deUint32 srcTex = 1;
		GLU_CHECK_CALL(glBindTexture(GL_TEXTURE_2D, srcTex));
		GLU_CHECK_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, DE_NULL));
		GLU_CHECK_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));

		// Create EGL image
		EGLint		attribs[]	= { EGL_GL_TEXTURE_LEVEL_KHR, 0, EGL_NONE };
		EGLImageKHR	image		= CHECK_EXT_CALL_ERR(egl.createImageKHR(context.getEglDisplay(), context.getEglContext(), EGL_GL_TEXTURE_2D_KHR, (EGLClientBuffer)(deUintptr)srcTex, attribs), EGL_SUCCESS);
		if (image == EGL_NO_IMAGE_KHR)
		{
			log << TestLog::Message << "  Fail: Got EGL_NO_IMAGE_KHR!" << TestLog::EndMessage;

			if (m_testCtx.getTestResult() == QP_TEST_RESULT_PASS)
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Got EGL_NO_IMAGE_KHR");
		}

		// Create texture or renderbuffer
		if (m_target == GL_TEXTURE_2D)
		{
			log << TestLog::Message << "Creating GL_TEXTURE_2D from EGLimage" << TestLog::EndMessage;

			deUint32 dstTex = 2;
			GLU_CHECK_CALL(glBindTexture(GL_TEXTURE_2D, dstTex));
			GLU_CHECK_CALL(glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, (GLeglImageOES)image));
			GLU_CHECK_CALL(glDeleteTextures(1, &dstTex));
		}
		else
		{
			DE_ASSERT(m_target == GL_RENDERBUFFER);

			log << TestLog::Message << "Creating GL_RENDERBUFFER from EGLimage" << TestLog::EndMessage;

			deUint32 dstRbo = 2;
			GLU_CHECK_CALL(glBindRenderbuffer(GL_RENDERBUFFER, dstRbo));
			GLU_CHECK_CALL(glEGLImageTargetRenderbufferStorageOES(GL_RENDERBUFFER, (GLeglImageOES)image));
			GLU_CHECK_CALL(glDeleteRenderbuffers(1, &dstRbo));
		}

		// Destroy image
		CHECK_EXT_CALL_RET(egl.destroyImageKHR(context.getEglDisplay(), image), (EGLBoolean)EGL_TRUE, EGL_SUCCESS);

		// Destroy source texture object
		GLU_CHECK_CALL(glDeleteTextures(1, &srcTex));

		return STOP;
	}

private:
	GLenum	m_target;
};

class ApiTests : public TestCaseGroup
{
public:
	ApiTests (EglTestContext& eglTestCtx, const string& name, const string& desc) : TestCaseGroup(eglTestCtx, name.c_str(), desc.c_str()) {}

	void init (void)
	{
		addChild(new Image::InvalidCreateImage(m_eglTestCtx));

		addChild(new Image::CreateImageGLES2(m_eglTestCtx, EGL_GL_TEXTURE_2D_KHR, GL_RED, GL_RED, GL_UNSIGNED_BYTE, false));
		addChild(new Image::CreateImageGLES2(m_eglTestCtx, EGL_GL_TEXTURE_2D_KHR, GL_RG, GL_RG, GL_UNSIGNED_BYTE, false));

		addChild(new Image::CreateImageGLES2(m_eglTestCtx, EGL_GL_TEXTURE_2D_KHR, GL_LUMINANCE, GL_LUMINANCE, GL_UNSIGNED_BYTE));
		addChild(new Image::CreateImageGLES2(m_eglTestCtx, EGL_GL_TEXTURE_2D_KHR, GL_LUMINANCE_ALPHA, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE));

		addChild(new Image::CreateImageGLES2(m_eglTestCtx, EGL_GL_TEXTURE_2D_KHR, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE));
		addChild(new Image::CreateImageGLES2(m_eglTestCtx, EGL_GL_TEXTURE_2D_KHR, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE));
		addChild(new Image::CreateImageGLES2(m_eglTestCtx, EGL_GL_TEXTURE_2D_KHR, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, true));

		addChild(new Image::CreateImageGLES2(m_eglTestCtx, EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_X_KHR, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE));
		addChild(new Image::CreateImageGLES2(m_eglTestCtx, EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_X_KHR, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE));
		addChild(new Image::CreateImageGLES2(m_eglTestCtx, EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_X_KHR, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, true));

		addChild(new Image::CreateImageGLES2(m_eglTestCtx, EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_X_KHR, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE));
		addChild(new Image::CreateImageGLES2(m_eglTestCtx, EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Y_KHR, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE));
		addChild(new Image::CreateImageGLES2(m_eglTestCtx, EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_KHR, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE));
		addChild(new Image::CreateImageGLES2(m_eglTestCtx, EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Z_KHR, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE));
		addChild(new Image::CreateImageGLES2(m_eglTestCtx, EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_KHR, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE));

		static const GLenum rboStorages[] =
		{
			GL_DEPTH_COMPONENT16,
			GL_RGBA4,
			GL_RGB5_A1,
			GL_RGB565,
			GL_STENCIL_INDEX8
		};
		for (int storageNdx = 0; storageNdx < DE_LENGTH_OF_ARRAY(rboStorages); storageNdx++)
			addChild(new Image::CreateImageGLES2(m_eglTestCtx, EGL_GL_RENDERBUFFER_KHR, rboStorages[storageNdx], (GLenum)0, (GLenum)0));

		static const GLenum androidFormats[] =
		{
			GL_RGB565,
			GL_RGB8,
			GL_RGBA4,
			GL_RGB5_A1,
			GL_RGBA8,
		};

		for (int formatNdx = 0; formatNdx < DE_LENGTH_OF_ARRAY(androidFormats); ++formatNdx)
			addChild(new Image::CreateImageGLES2(m_eglTestCtx, EGL_NATIVE_BUFFER_ANDROID, androidFormats[formatNdx], (GLenum)0, (GLenum)0));

		addChild(new Image::ImageTargetGLES2(m_eglTestCtx, GL_TEXTURE_2D));
		addChild(new Image::ImageTargetGLES2(m_eglTestCtx, GL_RENDERBUFFER));
	}
};

} // Image

ImageTests::ImageTests (EglTestContext& eglTestCtx)
	: TestCaseGroup(eglTestCtx, "image", "EGLImage Tests")
{
}

ImageTests::~ImageTests (void)
{
}

void ImageTests::init (void)
{
	addChild(new Image::ApiTests(m_eglTestCtx, "api", "EGLImage API tests"));
	addChild(Image::createSimpleCreationTests(m_eglTestCtx, "create", "EGLImage creation tests"));
	addChild(Image::createModifyTests(m_eglTestCtx, "modify", "EGLImage modifying tests"));
	addChild(Image::createMultiContextRenderTests(m_eglTestCtx, "render_multiple_contexts", "EGLImage render tests on multiple contexts"));
}

} // egl
} // deqp
