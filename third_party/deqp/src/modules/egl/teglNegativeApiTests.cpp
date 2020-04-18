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
 * \brief Negative API Tests.
 *//*--------------------------------------------------------------------*/

#include "teglNegativeApiTests.hpp"
#include "teglApiCase.hpp"

#include "egluNativeDisplay.hpp"
#include "egluNativeWindow.hpp"
#include "egluUtil.hpp"
#include "egluUtil.hpp"
#include "egluUnique.hpp"

#include "eglwLibrary.hpp"

#include <memory>

using tcu::TestLog;

namespace deqp
{
namespace egl
{

using namespace eglw;

template <deUint32 Type>
static bool renderable (const eglu::CandidateConfig& c)
{
	return (c.renderableType() & Type) == Type;
}

template <deUint32 Type>
static bool notRenderable (const eglu::CandidateConfig& c)
{
	return (c.renderableType() & Type) == 0;
}

template <deUint32 Bits>
static bool surfaceBits (const eglu::CandidateConfig& c)
{
	return (c.surfaceType() & Bits) == Bits;
}

template <deUint32 Bits>
static bool notSurfaceBits (const eglu::CandidateConfig& c)
{
	return (c.surfaceType() & Bits) == 0;
}

NegativeApiTests::NegativeApiTests (EglTestContext& eglTestCtx)
	: TestCaseGroup(eglTestCtx, "negative_api", "Negative API Tests")
{
}

NegativeApiTests::~NegativeApiTests (void)
{
}

void NegativeApiTests::init (void)
{
	// \todo [2012-10-02 pyry] Add tests for EGL_NOT_INITIALIZED to all functions taking in EGLDisplay
	// \todo [2012-10-02 pyry] Implement negative cases for following non-trivial cases:
	//  * eglBindTexImage()
	//    - EGL_BAD_ACCESS is generated if buffer is already bound to a texture
	//    - EGL_BAD_MATCH is generated if the surface attribute EGL_TEXTURE_FORMAT is set to EGL_NO_TEXTURE
	//    - EGL_BAD_MATCH is generated if buffer is not a valid buffer (currently only EGL_BACK_BUFFER may be specified)
	//    - EGL_BAD_SURFACE is generated if surface is not a pbuffer surface supporting texture binding
	//  * eglCopyBuffers()
	//    - EGL_BAD_NATIVE_PIXMAP is generated if the implementation does not support native pixmaps
	//    - EGL_BAD_NATIVE_PIXMAP may be generated if native_pixmap is not a valid native pixmap
	//    - EGL_BAD_MATCH is generated if the format of native_pixmap is not compatible with the color buffer of surface
	//  * eglCreateContext()
	//    - EGL_BAD_MATCH is generated if the current rendering API is EGL_NONE
	//	  - EGL_BAD_MATCH is generated if the server context state for share_context exists in an address space which cannot be shared with the newly created context
	//	  - EGL_BAD_CONTEXT is generated if share_context is not an EGL rendering context of the same client API type as the newly created context and is not EGL_NO_CONTEXT
	//  * eglCreatePbufferFromClientBuffer()
	//    - various BAD_MATCH, BAD_ACCESS etc. conditions
	//  * eglCreatePbufferSurface()
	//    - EGL_BAD_MATCH is generated if the EGL_TEXTURE_FORMAT attribute is not EGL_NO_TEXTURE, and EGL_WIDTH and/or EGL_HEIGHT specify an invalid size
	//  * eglCreatePixmapSurface()
	//    - EGL_BAD_ATTRIBUTE is generated if attrib_list contains an invalid pixmap attribute
	//    - EGL_BAD_MATCH is generated if the attributes of native_pixmap do not correspond to config or if config does not support rendering to pixmaps
	//    - EGL_BAD_MATCH is generated if config does not support the specified OpenVG alpha format attribute or colorspace attribute
	//  * eglCreateWindowSurface()
	//    - EGL_BAD_ATTRIBUTE is generated if attrib_list contains an invalid window attribute
	//    - EGL_BAD_MATCH is generated if the attributes of native_window do not correspond to config or if config does not support rendering to windows
	//    - EGL_BAD_MATCH is generated if config does not support the specified OpenVG alpha format attribute or colorspace attribute
	//  * eglMakeCurrent()
	//    - EGL_BAD_MATCH is generated if draw or read are not compatible with context
	//    - EGL_BAD_MATCH is generated if context is set to EGL_NO_CONTEXT and draw or read are not set to EGL_NO_SURFACE
	//    - EGL_BAD_MATCH is generated if draw or read are set to EGL_NO_SURFACE and context is not set to EGL_NO_CONTEXT
	//    - EGL_BAD_ACCESS is generated if context is current to some other thread
	//    - EGL_BAD_NATIVE_PIXMAP may be generated if a native pixmap underlying either draw or read is no longer valid
	//    - EGL_BAD_NATIVE_WINDOW may be generated if a native window underlying either draw or read is no longer valid
	//  * eglReleaseTexImage()
	//    - EGL_BAD_MATCH is generated if buffer is not a valid buffer (currently only EGL_BACK_BUFFER may be specified)
	//  * eglSwapInterval()
	//    - EGL_BAD_SURFACE is generated if there is no surface bound to the current context
	//  * eglWaitNative()
	//    - EGL_BAD_CURRENT_SURFACE is generated if the surface associated with the current context has a native window or pixmap, and that window or pixmap is no longer valid

	using namespace eglw;
	using namespace eglu;

	static const EGLint				s_emptyAttribList[]			= { EGL_NONE };
	static const EGLint				s_es1ContextAttribList[]	= { EGL_CONTEXT_CLIENT_VERSION, 1, EGL_NONE };
	static const EGLint				s_es2ContextAttribList[]	= { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };

	static const EGLenum			s_renderAPIs[]				= { EGL_OPENGL_API, EGL_OPENGL_ES_API, EGL_OPENVG_API };
	static const eglu::ConfigFilter	s_renderAPIFilters[]		= { renderable<EGL_OPENGL_BIT>, renderable<EGL_OPENGL_ES_BIT>, renderable<EGL_OPENVG_BIT> };

	TEGL_ADD_API_CASE(bind_api, "eglBindAPI() negative tests",
		{
			TestLog& log = m_testCtx.getLog();
			log << TestLog::Section("Test1", "EGL_BAD_PARAMETER is generated if api is not one of the accepted tokens");

			expectFalse(eglBindAPI(0));
			expectError(EGL_BAD_PARAMETER);

			expectFalse(eglBindAPI(0xfdfdfdfd));
			expectError(EGL_BAD_PARAMETER);

			expectFalse(eglBindAPI((EGLenum)0xffffffff));
			expectError(EGL_BAD_PARAMETER);

			log << TestLog::EndSection;

			log << TestLog::Section("Test2", "EGL_BAD_PARAMETER is generated if the specified client API is not supported by the EGL display, or no configuration is provided for the specified API.");

			for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(s_renderAPIs); ndx++)
			{
				if (!isAPISupported(s_renderAPIs[ndx]))
				{
					if (!eglBindAPI(s_renderAPIs[ndx]))
						expectError(EGL_BAD_PARAMETER);
					else
					{
						EGLConfig eglConfig;
						expectFalse(getConfig(&eglConfig, FilterList() << s_renderAPIFilters[ndx]));
					}
				}
			}

			log << TestLog::EndSection;
		});

	TEGL_ADD_API_CASE(bind_tex_image, "eglBindTexImage() negative tests",
		{
			TestLog&	log			= m_testCtx.getLog();
			EGLDisplay	display		= getDisplay();

			log << TestLog::Section("Test1", "EGL_BAD_DISPLAY is generated if display is not an EGL display connection");

			expectFalse(eglBindTexImage(EGL_NO_DISPLAY, EGL_NO_SURFACE, EGL_BACK_BUFFER));
			expectError(EGL_BAD_DISPLAY);

			expectFalse(eglBindTexImage((EGLDisplay)-1, EGL_NO_SURFACE, EGL_BACK_BUFFER));
			expectError(EGL_BAD_DISPLAY);

			log << TestLog::EndSection;

			log << TestLog::Section("Test2", "EGL_BAD_SURFACE is generated if surface is not an EGL surface");

			expectFalse(eglBindTexImage(display, EGL_NO_SURFACE, EGL_BACK_BUFFER));
			expectError(EGL_BAD_SURFACE);

			expectFalse(eglBindTexImage(display, (EGLSurface)-1, EGL_BACK_BUFFER));
			expectError(EGL_BAD_SURFACE);

			log << TestLog::EndSection;
		});

	static const EGLint s_validGenericPbufferAttrib[] = { EGL_WIDTH, 64, EGL_HEIGHT, 64, EGL_NONE };

	TEGL_ADD_API_CASE(copy_buffers, "eglCopyBuffers() negative tests",
		{
			TestLog&							log				= m_testCtx.getLog();
			const eglw::Library&				egl				= m_eglTestCtx.getLibrary();
			EGLDisplay							display			= getDisplay();
			const eglu::NativePixmapFactory&	factory			= eglu::selectNativePixmapFactory(m_eglTestCtx.getNativeDisplayFactory(), m_testCtx.getCommandLine());
			de::UniquePtr<eglu::NativePixmap>	pixmap			(factory.createPixmap(&m_eglTestCtx.getNativeDisplay(), 64, 64));
			EGLConfig							config;

			{
				if (getConfig(&config, FilterList() << surfaceBits<EGL_PBUFFER_BIT>))
				{
					eglu::UniqueSurface	surface	(egl, display, egl.createPbufferSurface(display, config, s_validGenericPbufferAttrib));

					log << TestLog::Section("Test1", "EGL_BAD_DISPLAY is generated if display is not an EGL display connection");

					expectFalse(eglCopyBuffers(EGL_NO_DISPLAY, EGL_NO_SURFACE, pixmap->getLegacyNative()));
					expectError(EGL_BAD_DISPLAY);

					expectFalse(eglCopyBuffers((EGLDisplay)-1, EGL_NO_SURFACE, pixmap->getLegacyNative()));
					expectError(EGL_BAD_DISPLAY);

					log << TestLog::EndSection;
				}
			}

			log << TestLog::Section("Test2", "EGL_BAD_SURFACE is generated if surface is not an EGL surface");

			expectFalse(eglCopyBuffers(display, EGL_NO_SURFACE, pixmap->getLegacyNative()));
			expectError(EGL_BAD_SURFACE);

			expectFalse(eglCopyBuffers(display, (EGLSurface)-1, pixmap->getLegacyNative()));
			expectError(EGL_BAD_SURFACE);

			log << TestLog::EndSection;
		});

	static const EGLint s_invalidChooseConfigAttribList0[]	= { 0, EGL_NONE };
	static const EGLint s_invalidChooseConfigAttribList1[]	= { (EGLint)0xffffffff };
	static const EGLint s_invalidChooseConfigAttribList2[]	= { EGL_BIND_TO_TEXTURE_RGB, 4, EGL_NONE };
	static const EGLint s_invalidChooseConfigAttribList3[]	= { EGL_BIND_TO_TEXTURE_RGBA, 5, EGL_NONE };
	static const EGLint s_invalidChooseConfigAttribList4[]	= { EGL_COLOR_BUFFER_TYPE, 0, EGL_NONE };
	static const EGLint s_invalidChooseConfigAttribList5[]	= { EGL_NATIVE_RENDERABLE, 6, EGL_NONE };
	static const EGLint s_invalidChooseConfigAttribList6[]	= { EGL_TRANSPARENT_TYPE, 6, EGL_NONE };
	static const EGLint* s_invalidChooseConfigAttribLists[] =
	{
		&s_invalidChooseConfigAttribList0[0],
		&s_invalidChooseConfigAttribList1[0],
		&s_invalidChooseConfigAttribList2[0],
		&s_invalidChooseConfigAttribList3[0],
		&s_invalidChooseConfigAttribList4[0],
		&s_invalidChooseConfigAttribList5[0],
		&s_invalidChooseConfigAttribList6[0]
	};

	TEGL_ADD_API_CASE(choose_config, "eglChooseConfig() negative tests",
		{
			TestLog&	log			= m_testCtx.getLog();
			EGLDisplay	display		= getDisplay();
			EGLConfig	configs[1];
			EGLint		numConfigs;

			log << TestLog::Section("Test1", "EGL_BAD_DISPLAY is generated if display is not an EGL display connection");

			expectFalse(eglChooseConfig(EGL_NO_DISPLAY, s_emptyAttribList, &configs[0], DE_LENGTH_OF_ARRAY(configs), &numConfigs));
			expectError(EGL_BAD_DISPLAY);

			expectFalse(eglChooseConfig((EGLDisplay)-1, s_emptyAttribList, &configs[0], DE_LENGTH_OF_ARRAY(configs), &numConfigs));
			expectError(EGL_BAD_DISPLAY);

			log << TestLog::EndSection;

			log << TestLog::Section("Test2", "EGL_BAD_ATTRIBUTE is generated if attribute_list contains an invalid frame buffer configuration attribute");

			for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(s_invalidChooseConfigAttribLists); ndx++)
			{
				expectFalse(eglChooseConfig(display, s_invalidChooseConfigAttribLists[ndx], &configs[0], DE_LENGTH_OF_ARRAY(configs), &numConfigs));
				expectError(EGL_BAD_ATTRIBUTE);
			}

			log << TestLog::EndSection;

			log << TestLog::Section("Test3", "EGL_BAD_PARAMETER is generated if num_config is NULL");

			expectFalse(eglChooseConfig(display, s_emptyAttribList, &configs[0], DE_LENGTH_OF_ARRAY(configs), DE_NULL));
			expectError(EGL_BAD_PARAMETER);

			log << TestLog::EndSection;
		});

	static const EGLint s_invalidCreateContextAttribList0[] = { 0, EGL_NONE };
	static const EGLint s_invalidCreateContextAttribList1[] = { (EGLint)0xffffffff };

	TEGL_ADD_API_CASE(create_context, "eglCreateContext() negative tests",
		{
			TestLog&	log			= m_testCtx.getLog();
			EGLDisplay	display		= getDisplay();

			log << TestLog::Section("Test1", "EGL_BAD_DISPLAY is generated if display is not an EGL display connection");

			expectNoContext(eglCreateContext(EGL_NO_DISPLAY, DE_NULL, EGL_NO_CONTEXT, s_emptyAttribList));
			expectError(EGL_BAD_DISPLAY);

			expectNoContext(eglCreateContext((EGLDisplay)-1, DE_NULL, EGL_NO_CONTEXT, s_emptyAttribList));
			expectError(EGL_BAD_DISPLAY);

			log << TestLog::EndSection;

			log << TestLog::Section("Test2", "EGL_BAD_CONFIG is generated if config is not an EGL frame buffer configuration");

			expectNoContext(eglCreateContext(display, (EGLConfig)-1, EGL_NO_CONTEXT, s_emptyAttribList));
			expectError(EGL_BAD_CONFIG);

			log << TestLog::EndSection;

			log << TestLog::Section("Test3", "EGL_BAD_CONFIG is generated if config does not support the current rendering API");

			if (isAPISupported(EGL_OPENGL_API))
			{
				EGLConfig es1OnlyConfig;
				if (getConfig(&es1OnlyConfig, FilterList() << renderable<EGL_OPENGL_ES_BIT> << notRenderable<EGL_OPENGL_BIT>))
				{
					expectTrue(eglBindAPI(EGL_OPENGL_API));
					expectNoContext(eglCreateContext(display, es1OnlyConfig, EGL_NO_CONTEXT, s_es1ContextAttribList));
					expectError(EGL_BAD_CONFIG);
				}

				EGLConfig es2OnlyConfig;
				if (getConfig(&es2OnlyConfig, FilterList() << renderable<EGL_OPENGL_ES2_BIT> << notRenderable<EGL_OPENGL_BIT>))
				{
					expectTrue(eglBindAPI(EGL_OPENGL_API));
					expectNoContext(eglCreateContext(display, es2OnlyConfig, EGL_NO_CONTEXT, s_es2ContextAttribList));
					expectError(EGL_BAD_CONFIG);
				}

				EGLConfig vgOnlyConfig;
				if (getConfig(&vgOnlyConfig, FilterList() << renderable<EGL_OPENVG_BIT> << notRenderable<EGL_OPENGL_BIT>))
				{
					expectTrue(eglBindAPI(EGL_OPENGL_API));
					expectNoContext(eglCreateContext(display, vgOnlyConfig, EGL_NO_CONTEXT, s_emptyAttribList));
					expectError(EGL_BAD_CONFIG);
				}
			}

			if (isAPISupported(EGL_OPENGL_ES_API))
			{
				EGLConfig glOnlyConfig;
				if (getConfig(&glOnlyConfig, FilterList() << renderable<EGL_OPENGL_BIT> << notRenderable<EGL_OPENGL_ES_BIT|EGL_OPENGL_ES2_BIT>))
				{
					expectTrue(eglBindAPI(EGL_OPENGL_ES_API));
					expectNoContext(eglCreateContext(display, glOnlyConfig, EGL_NO_CONTEXT, s_emptyAttribList));
					expectError(EGL_BAD_CONFIG);
				}

				EGLConfig vgOnlyConfig;
				if (getConfig(&vgOnlyConfig, FilterList() << renderable<EGL_OPENVG_BIT> << notRenderable<EGL_OPENGL_ES_BIT|EGL_OPENGL_ES2_BIT>))
				{
					expectTrue(eglBindAPI(EGL_OPENGL_ES_API));
					expectNoContext(eglCreateContext(display, vgOnlyConfig, EGL_NO_CONTEXT, s_emptyAttribList));
					expectError(EGL_BAD_CONFIG);
				}
			}

			if (isAPISupported(EGL_OPENVG_API))
			{
				EGLConfig glOnlyConfig;
				if (getConfig(&glOnlyConfig, FilterList() << renderable<EGL_OPENGL_BIT> << notRenderable<EGL_OPENVG_BIT>))
				{
					expectTrue(eglBindAPI(EGL_OPENVG_API));
					expectNoContext(eglCreateContext(display, glOnlyConfig, EGL_NO_CONTEXT, s_emptyAttribList));
					expectError(EGL_BAD_CONFIG);
				}

				EGLConfig es1OnlyConfig;
				if (getConfig(&es1OnlyConfig, FilterList() << renderable<EGL_OPENGL_ES_BIT> << notRenderable<EGL_OPENVG_BIT>))
				{
					expectTrue(eglBindAPI(EGL_OPENVG_API));
					expectNoContext(eglCreateContext(display, es1OnlyConfig, EGL_NO_CONTEXT, s_es1ContextAttribList));
					expectError(EGL_BAD_CONFIG);
				}

				EGLConfig es2OnlyConfig;
				if (getConfig(&es2OnlyConfig, FilterList() << renderable<EGL_OPENGL_ES2_BIT> << notRenderable<EGL_OPENVG_BIT>))
				{
					expectTrue(eglBindAPI(EGL_OPENVG_API));
					expectNoContext(eglCreateContext(display, es2OnlyConfig, EGL_NO_CONTEXT, s_es2ContextAttribList));
					expectError(EGL_BAD_CONFIG);
				}
			}

			log << TestLog::EndSection;

			log << TestLog::Section("Test4", "EGL_BAD_CONFIG or EGL_BAD_MATCH is generated if OpenGL ES 1.x context is requested and EGL_RENDERABLE_TYPE attribute of config does not contain EGL_OPENGL_ES_BIT");

			if (isAPISupported(EGL_OPENGL_ES_API))
			{
				EGLConfig notES1Config;
				if (getConfig(&notES1Config, FilterList() << notRenderable<EGL_OPENGL_ES_BIT>))
				{
					// EGL 1.4, EGL 1.5, and EGL_KHR_create_context contain contradictory language about the expected error.
					Version version = eglu::getVersion(m_eglTestCtx.getLibrary(), display);
					bool hasKhrCreateContext = eglu::hasExtension(m_eglTestCtx.getLibrary(), display, "EGL_KHR_create_context");

					expectTrue(eglBindAPI(EGL_OPENGL_ES_API));
					expectNoContext(eglCreateContext(display, notES1Config, EGL_NO_CONTEXT, s_es1ContextAttribList));
					if (hasKhrCreateContext)
						expectEitherError(EGL_BAD_CONFIG, EGL_BAD_MATCH);
					else
					{
						if (version >= eglu::Version(1, 5))
							expectError(EGL_BAD_MATCH);
						else
							expectError(EGL_BAD_CONFIG);
					}
				}
			}

			log << TestLog::EndSection;

			log << TestLog::Section("Test5", "EGL_BAD_CONFIG or EGL_BAD_MATCH is generated if OpenGL ES 2.x context is requested and EGL_RENDERABLE_TYPE attribute of config does not contain EGL_OPENGL_ES2_BIT");

			if (isAPISupported(EGL_OPENGL_ES_API))
			{
				EGLConfig notES2Config;
				if (getConfig(&notES2Config, FilterList() << notRenderable<EGL_OPENGL_ES2_BIT>))
				{
					// EGL 1.4, EGL 1.5, and EGL_KHR_create_context contain contradictory language about the expected error.
					Version version = eglu::getVersion(m_eglTestCtx.getLibrary(), display);
					bool hasKhrCreateContext = eglu::hasExtension(m_eglTestCtx.getLibrary(), display, "EGL_KHR_create_context");

					expectTrue(eglBindAPI(EGL_OPENGL_ES_API));
					expectNoContext(eglCreateContext(display, notES2Config, EGL_NO_CONTEXT, s_es2ContextAttribList));
					if (hasKhrCreateContext)
						expectEitherError(EGL_BAD_CONFIG, EGL_BAD_MATCH);
					else
					{
						if (version >= eglu::Version(1, 5))
							expectError(EGL_BAD_MATCH);
						else
							expectError(EGL_BAD_CONFIG);
					}
				}
			}

			log << TestLog::EndSection;

			log << TestLog::Section("Test6", "EGL_BAD_ATTRIBUTE is generated if attrib_list contains an invalid context attribute");

			if (isAPISupported(EGL_OPENGL_API) && !eglu::hasExtension(m_eglTestCtx.getLibrary(), display, "EGL_KHR_create_context"))
			{
				EGLConfig glConfig;
				if (getConfig(&glConfig, FilterList() << renderable<EGL_OPENGL_BIT>))
				{
					expectTrue(eglBindAPI(EGL_OPENGL_API));
					expectNoContext(eglCreateContext(display, glConfig, EGL_NO_CONTEXT, s_es1ContextAttribList));
					expectError(EGL_BAD_ATTRIBUTE);
				}
			}

			if (isAPISupported(EGL_OPENVG_API))
			{
				EGLConfig vgConfig;
				if (getConfig(&vgConfig, FilterList() << renderable<EGL_OPENVG_BIT>))
				{
					expectTrue(eglBindAPI(EGL_OPENVG_API));
					expectNoContext(eglCreateContext(display, vgConfig, EGL_NO_CONTEXT, s_es1ContextAttribList));
					expectError(EGL_BAD_ATTRIBUTE);
				}
			}

			if (isAPISupported(EGL_OPENGL_ES_API))
			{
				bool		gotConfig	= false;
				EGLConfig	esConfig;

				gotConfig = getConfig(&esConfig, FilterList() << renderable<EGL_OPENGL_ES_BIT>) ||
							getConfig(&esConfig, FilterList() << renderable<EGL_OPENGL_ES2_BIT>);

				if (gotConfig)
				{
					expectTrue(eglBindAPI(EGL_OPENGL_ES_API));
					expectNoContext(eglCreateContext(display, esConfig, EGL_NO_CONTEXT, s_invalidCreateContextAttribList0));
					expectError(EGL_BAD_ATTRIBUTE);
					expectNoContext(eglCreateContext(display, esConfig, EGL_NO_CONTEXT, s_invalidCreateContextAttribList1));
					expectError(EGL_BAD_ATTRIBUTE);
				}
			}

			log << TestLog::EndSection;
		});

	TEGL_ADD_API_CASE(create_pbuffer_from_client_buffer, "eglCreatePbufferFromClientBuffer() negative tests",
		{
			TestLog&	log			= m_testCtx.getLog();
			EGLDisplay	display		= getDisplay();
			EGLConfig	anyConfig;
			EGLint		unused		= 0;

			log << TestLog::Section("Test1", "EGL_BAD_DISPLAY is generated if display is not an EGL display connection");

			expectNoSurface(eglCreatePbufferFromClientBuffer(EGL_NO_DISPLAY, EGL_OPENVG_IMAGE, 0, (EGLConfig)0, DE_NULL));
			expectError(EGL_BAD_DISPLAY);

			expectNoSurface(eglCreatePbufferFromClientBuffer((EGLDisplay)-1, EGL_OPENVG_IMAGE, 0, (EGLConfig)0, DE_NULL));
			expectError(EGL_BAD_DISPLAY);

			log << TestLog::EndSection;

			if (isAPISupported(EGL_OPENVG_API))
			{
				log << TestLog::Section("Test2", "EGL_BAD_CONFIG or EGL_BAD_PARAMETER is generated if config is not an EGL frame buffer configuration and if buffer is not valid OpenVG image");

				expectNoSurface(eglCreatePbufferFromClientBuffer(display, EGL_OPENVG_IMAGE, (EGLClientBuffer)-1, (EGLConfig)-1, DE_NULL));
				expectEitherError(EGL_BAD_CONFIG, EGL_BAD_PARAMETER);

				log << TestLog::EndSection;

				log << TestLog::Section("Test3", "EGL_BAD_PARAMETER is generated if buftype is not EGL_OPENVG_IMAGE");

				expectTrue(eglGetConfigs(display, &anyConfig, 1, &unused));

				log << TestLog::EndSection;

				log << TestLog::Section("Test4", "EGL_BAD_PARAMETER is generated if buffer is not valid OpenVG image");
				expectNoSurface(eglCreatePbufferFromClientBuffer(display, EGL_OPENVG_IMAGE, (EGLClientBuffer)-1, anyConfig, DE_NULL));
				expectError(EGL_BAD_PARAMETER);

				log << TestLog::EndSection;
			}
		});

	static const EGLint s_invalidGenericPbufferAttrib0[] = { 0, EGL_NONE };
	static const EGLint s_invalidGenericPbufferAttrib1[] = { (EGLint)0xffffffff };
	static const EGLint s_negativeWidthPbufferAttrib[] = { EGL_WIDTH, -1, EGL_HEIGHT, 64, EGL_NONE };
	static const EGLint s_negativeHeightPbufferAttrib[] = { EGL_WIDTH, 64, EGL_HEIGHT, -1, EGL_NONE };
	static const EGLint s_negativeWidthAndHeightPbufferAttrib[] = { EGL_WIDTH, -1, EGL_HEIGHT, -1, EGL_NONE };
	static const EGLint* s_invalidGenericPbufferAttribs[] =
	{
		s_invalidGenericPbufferAttrib0,
		s_invalidGenericPbufferAttrib1,
	};

	static const EGLint s_invalidNoEsPbufferAttrib0[] = { EGL_MIPMAP_TEXTURE, EGL_TRUE, EGL_WIDTH, 64, EGL_HEIGHT, 64, EGL_NONE };
	static const EGLint s_invalidNoEsPbufferAttrib1[] = { EGL_TEXTURE_FORMAT, EGL_TEXTURE_RGBA, EGL_WIDTH, 64, EGL_HEIGHT, 64, EGL_NONE };
	static const EGLint s_invalidNoEsPbufferAttrib2[] = { EGL_TEXTURE_TARGET, EGL_TEXTURE_2D, EGL_WIDTH, 64, EGL_HEIGHT, 64, EGL_NONE };
	static const EGLint* s_invalidNoEsPbufferAttribs[] =
	{
		s_invalidNoEsPbufferAttrib0,
		s_invalidNoEsPbufferAttrib1,
		s_invalidNoEsPbufferAttrib2
	};

	static const EGLint s_invalidEsPbufferAttrib0[] = { EGL_TEXTURE_FORMAT, EGL_NO_TEXTURE, EGL_TEXTURE_TARGET, EGL_TEXTURE_2D, EGL_WIDTH, 64, EGL_HEIGHT, 64, EGL_NONE };
	static const EGLint s_invalidEsPbufferAttrib1[] = { EGL_TEXTURE_FORMAT, EGL_TEXTURE_RGBA, EGL_TEXTURE_TARGET, EGL_NO_TEXTURE, EGL_WIDTH, 64, EGL_HEIGHT, 64, EGL_NONE };
	static const EGLint* s_invalidEsPbufferAttribs[] =
	{
		s_invalidEsPbufferAttrib0,
		s_invalidEsPbufferAttrib1
	};

	static const EGLint s_vgPreMultAlphaPbufferAttrib[] = { EGL_ALPHA_FORMAT, EGL_ALPHA_FORMAT_PRE, EGL_WIDTH, 64, EGL_HEIGHT, 64, EGL_NONE };
	static const EGLint s_vgLinearColorspacePbufferAttrib[] = { EGL_COLORSPACE, EGL_VG_COLORSPACE_LINEAR, EGL_WIDTH, 64, EGL_HEIGHT, 64, EGL_NONE };

	TEGL_ADD_API_CASE(create_pbuffer_surface, "eglCreatePbufferSurface() negative tests",
		{
			TestLog&	log			= m_testCtx.getLog();
			EGLDisplay	display		= getDisplay();

			log << TestLog::Section("Test1", "EGL_BAD_DISPLAY is generated if display is not an EGL display connection");

			expectNoSurface(eglCreatePbufferSurface(EGL_NO_DISPLAY, DE_NULL, s_emptyAttribList));
			expectError(EGL_BAD_DISPLAY);

			expectNoSurface(eglCreatePbufferSurface((EGLDisplay)-1, DE_NULL, s_emptyAttribList));
			expectError(EGL_BAD_DISPLAY);

			log << TestLog::EndSection;

			log << TestLog::Section("Test2", "EGL_BAD_CONFIG is generated if config is not an EGL frame buffer configuration");

			expectNoSurface(eglCreatePbufferSurface(display, (EGLConfig)-1, s_emptyAttribList));
			expectError(EGL_BAD_CONFIG);

			log << TestLog::EndSection;

			log << TestLog::Section("Test3", "EGL_BAD_ATTRIBUTE is generated if attrib_list contains an invalid pixel buffer attribute");

			// Generic pbuffer-capable config
			EGLConfig genericConfig;
			if (getConfig(&genericConfig, FilterList() << surfaceBits<EGL_PBUFFER_BIT>))
			{
				for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(s_invalidGenericPbufferAttribs); ndx++)
				{
					expectNoSurface(eglCreatePbufferSurface(display, genericConfig, s_invalidGenericPbufferAttribs[ndx]));
					expectError(EGL_BAD_ATTRIBUTE);
				}
			}

			log << TestLog::EndSection;

			log << TestLog::Section("Test4", "EGL_BAD_MATCH is generated if config does not support rendering to pixel buffers");

			EGLConfig noPbufferConfig;
			if (getConfig(&noPbufferConfig, FilterList() << notSurfaceBits<EGL_PBUFFER_BIT>))
			{
				expectNoSurface(eglCreatePbufferSurface(display, noPbufferConfig, s_validGenericPbufferAttrib));
				expectError(EGL_BAD_MATCH);
			}

			log << TestLog::EndSection;

			log << TestLog::Section("Test5", "EGL_BAD_ATTRIBUTE is generated if attrib_list contains any of the attributes EGL_MIPMAP_TEXTURE, EGL_TEXTURE_FORMAT, or EGL_TEXTURE_TARGET, and config does not support OpenGL ES rendering");

			EGLConfig noEsConfig;
			if (getConfig(&noEsConfig, FilterList() << notRenderable<EGL_OPENGL_ES_BIT|EGL_OPENGL_ES2_BIT>))
			{
				for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(s_invalidNoEsPbufferAttribs); ndx++)
				{
					expectNoSurface(eglCreatePbufferSurface(display, noEsConfig, s_invalidNoEsPbufferAttribs[ndx]));
					expectError(EGL_BAD_MATCH);
				}
			}

			log << TestLog::EndSection;

			log << TestLog::Section("Test6", "EGL_BAD_MATCH is generated if the EGL_TEXTURE_FORMAT attribute is EGL_NO_TEXTURE, and EGL_TEXTURE_TARGET is something other than EGL_NO_TEXTURE; or, EGL_TEXTURE_FORMAT is something other than EGL_NO_TEXTURE, and EGL_TEXTURE_TARGET is EGL_NO_TEXTURE");

			// ES1 or ES2 config.
			EGLConfig	esConfig;
			bool		gotEsConfig	= getConfig(&esConfig, FilterList() << renderable<EGL_OPENGL_ES_BIT>) ||
									  getConfig(&esConfig, FilterList() << renderable<EGL_OPENGL_ES2_BIT>);
			if (gotEsConfig)
			{
				for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(s_invalidEsPbufferAttribs); ndx++)
				{
					expectNoSurface(eglCreatePbufferSurface(display, esConfig, s_invalidEsPbufferAttribs[ndx]));
					expectError(EGL_BAD_MATCH);
				}
			}

			log << TestLog::EndSection;

			log << TestLog::Section("Test7", "EGL_BAD_MATCH is generated if config does not support the specified OpenVG alpha format attribute or colorspace attribute");

			EGLConfig vgNoPreConfig;
			if (getConfig(&vgNoPreConfig, FilterList() << renderable<EGL_OPENVG_BIT> << notSurfaceBits<EGL_VG_ALPHA_FORMAT_PRE_BIT>))
			{
				expectNoSurface(eglCreatePbufferSurface(display, vgNoPreConfig, s_vgPreMultAlphaPbufferAttrib));
				expectError(EGL_BAD_MATCH);
			}

			EGLConfig vgNoLinearConfig;
			if (getConfig(&vgNoLinearConfig, FilterList() << renderable<EGL_OPENVG_BIT> << notSurfaceBits<EGL_VG_COLORSPACE_LINEAR_BIT>))
			{
				expectNoSurface(eglCreatePbufferSurface(display, vgNoLinearConfig, s_vgLinearColorspacePbufferAttrib));
				expectError(EGL_BAD_MATCH);
			}

			log << TestLog::EndSection;

			log << TestLog::Section("Test8", "EGL_BAD_PARAMETER is generated if EGL_WIDTH or EGL_HEIGHT is negative");

			if (getConfig(&genericConfig, FilterList() << surfaceBits<EGL_PBUFFER_BIT>))
			{
				expectNoSurface(eglCreatePbufferSurface(display, genericConfig, s_negativeWidthPbufferAttrib));
				expectError(EGL_BAD_PARAMETER);

				expectNoSurface(eglCreatePbufferSurface(display, genericConfig, s_negativeHeightPbufferAttrib));
				expectError(EGL_BAD_PARAMETER);

				expectNoSurface(eglCreatePbufferSurface(display, genericConfig, s_negativeWidthAndHeightPbufferAttrib));
				expectError(EGL_BAD_PARAMETER);
			}

			log << TestLog::EndSection;

		});

	TEGL_ADD_API_CASE(create_pixmap_surface, "eglCreatePixmapSurface() negative tests",
		{
			TestLog&	log			= m_testCtx.getLog();
			EGLDisplay	display		= getDisplay();

			log << TestLog::Section("Test1", "EGL_BAD_DISPLAY is generated if display is not an EGL display connection");

			expectNoSurface(eglCreatePixmapSurface(EGL_NO_DISPLAY, DE_NULL, DE_NULL, s_emptyAttribList));
			expectError(EGL_BAD_DISPLAY);

			expectNoSurface(eglCreatePixmapSurface((EGLDisplay)-1, DE_NULL, DE_NULL, s_emptyAttribList));
			expectError(EGL_BAD_DISPLAY);

			log << TestLog::EndSection;

			log << TestLog::Section("Test2", "EGL_BAD_CONFIG or EGL_BAD_PARAMETER is generated if config is not an EGL frame buffer configuration or if the PixmapSurface call is not supported");

			expectNoSurface(eglCreatePixmapSurface(display, (EGLConfig)-1, DE_NULL, s_emptyAttribList));
			expectEitherError(EGL_BAD_CONFIG, EGL_BAD_PARAMETER);

			log << TestLog::EndSection;
		});

	TEGL_ADD_API_CASE(create_window_surface, "eglCreateWindowSurface() negative tests",
		{
			EGLConfig				config			= DE_NULL;
			bool					gotConfig		= getConfig(&config, FilterList() << renderable<EGL_OPENGL_ES2_BIT> << surfaceBits<EGL_WINDOW_BIT>);

			if (gotConfig)
			{
				TestLog&							log				= m_testCtx.getLog();
				EGLDisplay							display			= getDisplay();
				const eglu::NativeWindowFactory&	factory			= eglu::selectNativeWindowFactory(m_eglTestCtx.getNativeDisplayFactory(), m_testCtx.getCommandLine());
				de::UniquePtr<eglu::NativeWindow>	window			(factory.createWindow(&m_eglTestCtx.getNativeDisplay(), display, config, DE_NULL, eglu::WindowParams(256, 256, eglu::parseWindowVisibility(m_testCtx.getCommandLine()))));

				log << TestLog::Section("Test1", "EGL_BAD_DISPLAY is generated if display is not an EGL display connection");

				expectNoSurface(eglCreateWindowSurface(EGL_NO_DISPLAY, config, window->getLegacyNative(), s_emptyAttribList));
				expectError(EGL_BAD_DISPLAY);

				expectNoSurface(eglCreateWindowSurface((EGLDisplay)-1, config, window->getLegacyNative(), s_emptyAttribList));
				expectError(EGL_BAD_DISPLAY);

				log << TestLog::EndSection;
			}
		});

	TEGL_ADD_API_CASE(destroy_context, "eglDestroyContext() negative tests",
		{
			TestLog&	log			= m_testCtx.getLog();
			EGLDisplay	display		= getDisplay();

			log << TestLog::Section("Test1", "EGL_BAD_DISPLAY is generated if display is not an EGL display connection");

			expectFalse(eglDestroyContext(EGL_NO_DISPLAY, DE_NULL));
			expectError(EGL_BAD_DISPLAY);

			expectFalse(eglDestroyContext((EGLDisplay)-1, DE_NULL));
			expectError(EGL_BAD_DISPLAY);

			log << TestLog::EndSection;

			log << TestLog::Section("Test2", "EGL_BAD_CONTEXT is generated if context is not an EGL rendering context");

			expectFalse(eglDestroyContext(display, DE_NULL));
			expectError(EGL_BAD_CONTEXT);

			expectFalse(eglDestroyContext(display, (EGLContext)-1));
			expectError(EGL_BAD_CONTEXT);

			log << TestLog::EndSection;
		});

	TEGL_ADD_API_CASE(destroy_surface, "eglDestroySurface() negative tests",
		{
			TestLog&	log			= m_testCtx.getLog();
			EGLDisplay	display		= getDisplay();

			log << TestLog::Section("Test1", "EGL_BAD_DISPLAY is generated if display is not an EGL display connection");

			expectFalse(eglDestroySurface(EGL_NO_DISPLAY, DE_NULL));
			expectError(EGL_BAD_DISPLAY);

			expectFalse(eglDestroySurface((EGLDisplay)-1, DE_NULL));
			expectError(EGL_BAD_DISPLAY);

			log << TestLog::EndSection;

			log << TestLog::Section("Test2", "EGL_BAD_SURFACE is generated if surface is not an EGL surface");

			expectFalse(eglDestroySurface(display, DE_NULL));
			expectError(EGL_BAD_SURFACE);

			expectFalse(eglDestroySurface(display, (EGLSurface)-1));
			expectError(EGL_BAD_SURFACE);

			log << TestLog::EndSection;
		});

	TEGL_ADD_API_CASE(get_config_attrib, "eglGetConfigAttrib() negative tests",
		{
			TestLog&	log			= m_testCtx.getLog();
			EGLDisplay	display		= getDisplay();
			EGLint		value		= 0;

			log << TestLog::Section("Test1", "EGL_BAD_DISPLAY is generated if display is not an EGL display connection");

			expectFalse(eglGetConfigAttrib(EGL_NO_DISPLAY, DE_NULL, EGL_RED_SIZE, &value));
			expectError(EGL_BAD_DISPLAY);

			expectFalse(eglGetConfigAttrib((EGLDisplay)-1, DE_NULL, EGL_RED_SIZE, &value));
			expectError(EGL_BAD_DISPLAY);

			log << TestLog::EndSection;

			log << TestLog::Section("Test2", "EGL_BAD_CONFIG is generated if config is not an EGL frame buffer configuration");

			expectFalse(eglGetConfigAttrib(display, (EGLConfig)-1, EGL_RED_SIZE, &value));
			expectError(EGL_BAD_CONFIG);

			log << TestLog::EndSection;

			// Any config.
			EGLConfig	config		= DE_NULL;
			bool		hasConfig	= getConfig(&config, FilterList());

			log << TestLog::Section("Test3", "EGL_BAD_ATTRIBUTE is generated if attribute is not a valid frame buffer configuration attribute");

			if (hasConfig)
			{
				expectFalse(eglGetConfigAttrib(display, config, 0, &value));
				expectError(EGL_BAD_ATTRIBUTE);

				expectFalse(eglGetConfigAttrib(display, config, -1, &value));
				expectError(EGL_BAD_ATTRIBUTE);
			}

			log << TestLog::EndSection;
		});

	TEGL_ADD_API_CASE(get_configs, "eglGetConfigs() negative tests",
		{
			TestLog&	log			= m_testCtx.getLog();
			EGLDisplay	display		= getDisplay();
			EGLConfig	cfgs[1];
			EGLint		numCfgs		= 0;

			log << TestLog::Section("Test1", "EGL_BAD_DISPLAY is generated if display is not an EGL display connection");

			expectFalse(eglGetConfigs(EGL_NO_DISPLAY, &cfgs[0], DE_LENGTH_OF_ARRAY(cfgs), &numCfgs));
			expectError(EGL_BAD_DISPLAY);

			expectFalse(eglGetConfigs((EGLDisplay)-1, &cfgs[0], DE_LENGTH_OF_ARRAY(cfgs), &numCfgs));
			expectError(EGL_BAD_DISPLAY);

			log << TestLog::EndSection;

			log << TestLog::Section("Test2", "EGL_BAD_PARAMETER is generated if num_config is NULL");

			expectFalse(eglGetConfigs(display, &cfgs[0], DE_LENGTH_OF_ARRAY(cfgs), DE_NULL));
			expectError(EGL_BAD_PARAMETER);

			log << TestLog::EndSection;
		});

	TEGL_ADD_API_CASE(initialize, "eglInitialize() negative tests",
		{
			TestLog&	log			= m_testCtx.getLog();
			EGLint		major		= 0;
			EGLint		minor		= 0;

			log << TestLog::Section("Test1", "EGL_BAD_DISPLAY is generated if display is not an EGL display connection");

			expectFalse(eglInitialize(EGL_NO_DISPLAY, &major, &minor));
			expectError(EGL_BAD_DISPLAY);

			expectFalse(eglInitialize((EGLDisplay)-1, &major, &minor));
			expectError(EGL_BAD_DISPLAY);

			log << TestLog::EndSection;
		});

	TEGL_ADD_API_CASE(make_current, "eglMakeCurrent() negative tests",
		{
			TestLog&	log			= m_testCtx.getLog();
			EGLDisplay	display		= getDisplay();

			log << TestLog::Section("Test1", "EGL_BAD_DISPLAY is generated if display is not an EGL display connection");

			expectFalse(eglMakeCurrent(EGL_NO_DISPLAY, DE_NULL, DE_NULL, DE_NULL));
			expectError(EGL_BAD_DISPLAY);

			expectFalse(eglMakeCurrent((EGLDisplay)-1, DE_NULL, DE_NULL, DE_NULL));
			expectError(EGL_BAD_DISPLAY);

			log << TestLog::EndSection;

			// Create simple pbuffer surface.
			EGLSurface surface = EGL_NO_SURFACE;
			{
				EGLConfig config;
				if (getConfig(&config, FilterList() << surfaceBits<EGL_PBUFFER_BIT>))
				{
					surface = eglCreatePbufferSurface(display, config, s_validGenericPbufferAttrib);
					expectError(EGL_SUCCESS);
				}
			}

			// Create simple ES2 context
			EGLContext context = EGL_NO_CONTEXT;
			{
				EGLConfig config;
				if (getConfig(&config, FilterList() << renderable<EGL_OPENGL_ES2_BIT>))
				{
					context = eglCreateContext(display, config, EGL_NO_CONTEXT, s_es2ContextAttribList);
					expectError(EGL_SUCCESS);
				}
			}

			if (surface != EGL_NO_SURFACE && context != EGL_NO_CONTEXT)
			{
				log << TestLog::Section("Test2", "EGL_BAD_SURFACE is generated if surface is not an EGL surface");

				expectFalse(eglMakeCurrent(display, (EGLSurface)-1, (EGLSurface)-1, context));
				expectError(EGL_BAD_SURFACE);

				expectFalse(eglMakeCurrent(display, surface, (EGLSurface)-1, context));
				expectError(EGL_BAD_SURFACE);

				expectFalse(eglMakeCurrent(display, (EGLSurface)-1, surface, context));
				expectError(EGL_BAD_SURFACE);

				log << TestLog::EndSection;
			}

			if (surface)
			{
				log << TestLog::Section("Test3", "EGL_BAD_CONTEXT is generated if context is not an EGL rendering context");

				expectFalse(eglMakeCurrent(display, surface, surface, (EGLContext)-1));
				expectError(EGL_BAD_CONTEXT);

				log << TestLog::EndSection;
			}

			if (surface != EGL_NO_SURFACE)
			{
				log << TestLog::Section("Test4", "EGL_BAD_MATCH is generated if read or draw surface is not EGL_NO_SURFACE and context is EGL_NO_CONTEXT");

				expectFalse(eglMakeCurrent(display, surface, EGL_NO_SURFACE, EGL_NO_CONTEXT));
				expectError(EGL_BAD_MATCH);

				expectFalse(eglMakeCurrent(display, EGL_NO_SURFACE, surface, EGL_NO_CONTEXT));
				expectError(EGL_BAD_MATCH);

				expectFalse(eglMakeCurrent(display, surface, surface, EGL_NO_CONTEXT));
				expectError(EGL_BAD_MATCH);

				log << TestLog::EndSection;
			}

			if (context)
			{
				eglDestroyContext(display, context);
				expectError(EGL_SUCCESS);
			}

			if (surface)
			{
				eglDestroySurface(display, surface);
				expectError(EGL_SUCCESS);
			}
		});

	TEGL_ADD_API_CASE(get_current_context, "eglGetCurrentContext() negative tests",
		{
			expectNoContext(eglGetCurrentContext());

			if (isAPISupported(EGL_OPENGL_ES_API))
			{
				expectTrue(eglBindAPI(EGL_OPENGL_ES_API));
				expectError(EGL_SUCCESS);

				expectNoContext(eglGetCurrentContext());
				expectError(EGL_SUCCESS);
			}
		});

	TEGL_ADD_API_CASE(get_current_surface, "eglGetCurrentSurface() negative tests",
		{
			TestLog&	log			= m_testCtx.getLog();
			EGLDisplay	display		= getDisplay();
			EGLConfig	config		= DE_NULL;
			EGLContext	context		= EGL_NO_CONTEXT;
			EGLSurface	surface		= EGL_NO_SURFACE;
			bool		gotConfig	= getConfig(&config, FilterList() << renderable<EGL_OPENGL_ES2_BIT> << surfaceBits<EGL_PBUFFER_BIT>);

			if (gotConfig)
			{
				expectTrue(eglBindAPI(EGL_OPENGL_ES_API));
				expectError(EGL_SUCCESS);

				context = eglCreateContext(display, config, EGL_NO_CONTEXT, s_es2ContextAttribList);
				expectError(EGL_SUCCESS);

				// Create simple pbuffer surface.
				surface = eglCreatePbufferSurface(display, config, s_validGenericPbufferAttrib);
				expectError(EGL_SUCCESS);

				expectTrue(eglMakeCurrent(display, surface, surface, context));
				expectError(EGL_SUCCESS);

				log << TestLog::Section("Test1", "EGL_BAD_PARAMETER is generated if readdraw is neither EGL_READ nor EGL_DRAW");

				expectNoSurface(eglGetCurrentSurface(EGL_NONE));
				expectError(EGL_BAD_PARAMETER);

				log << TestLog::EndSection;

				expectTrue(eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
				expectError(EGL_SUCCESS);

				if (surface != EGL_NO_SURFACE)
				{
					expectTrue(eglDestroySurface(display, surface));
					expectError(EGL_SUCCESS);
				}

				if (context != EGL_NO_CONTEXT)
				{
					expectTrue(eglDestroyContext(display, context));
					expectError(EGL_SUCCESS);
				}
			}
		});

	TEGL_ADD_API_CASE(query_context, "eglQueryContext() negative tests",
		{
			TestLog&	log			= m_testCtx.getLog();
			EGLDisplay	display		= getDisplay();
			EGLint		value		= 0;

			log << TestLog::Section("Test1", "EGL_BAD_DISPLAY is generated if display is not an EGL display connection");

			expectFalse(eglQueryContext(EGL_NO_DISPLAY, DE_NULL, EGL_CONFIG_ID, &value));
			expectError(EGL_BAD_DISPLAY);

			expectFalse(eglQueryContext((EGLDisplay)-1, DE_NULL, EGL_CONFIG_ID, &value));
			expectError(EGL_BAD_DISPLAY);

			log << TestLog::EndSection;

			log << TestLog::Section("Test2", "EGL_BAD_CONTEXT is generated if context is not an EGL rendering context");

			expectFalse(eglQueryContext(display, DE_NULL, EGL_CONFIG_ID, &value));
			expectError(EGL_BAD_CONTEXT);

			expectFalse(eglQueryContext(display, DE_NULL, EGL_CONFIG_ID, &value));
			expectError(EGL_BAD_CONTEXT);

			log << TestLog::EndSection;

			// Create ES2 context.
			EGLConfig	config		= DE_NULL;
			EGLContext	context		= DE_NULL;
			bool		gotConfig	= getConfig(&config, FilterList() << renderable<EGL_OPENGL_ES2_BIT>);

			if (gotConfig)
			{
				expectTrue(eglBindAPI(EGL_OPENGL_ES_API));
				expectError(EGL_SUCCESS);

				context = eglCreateContext(display, config, EGL_NO_CONTEXT, s_es2ContextAttribList);
				expectError(EGL_SUCCESS);
			}

			log << TestLog::Section("Test3", "EGL_BAD_ATTRIBUTE is generated if attribute is not a valid context attribute");

			if (context)
			{
				expectFalse(eglQueryContext(display, context, 0, &value));
				expectError(EGL_BAD_ATTRIBUTE);
				expectFalse(eglQueryContext(display, context, -1, &value));
				expectError(EGL_BAD_ATTRIBUTE);
				expectFalse(eglQueryContext(display, context, EGL_RED_SIZE, &value));
				expectError(EGL_BAD_ATTRIBUTE);
			}

			log << TestLog::EndSection;

			if (context)
			{
				expectTrue(eglDestroyContext(display, context));
				expectError(EGL_SUCCESS);
			}
		});

	TEGL_ADD_API_CASE(query_string, "eglQueryString() negative tests",
		{
			TestLog&	log			= m_testCtx.getLog();
			EGLDisplay	display		= getDisplay();

			log << TestLog::Section("Test1", "EGL_BAD_DISPLAY is generated if display is not an EGL display connection");

			expectNull(eglQueryString(EGL_NO_DISPLAY, EGL_VENDOR));
			expectError(EGL_BAD_DISPLAY);

			expectNull(eglQueryString((EGLDisplay)-1, EGL_VENDOR));
			expectError(EGL_BAD_DISPLAY);

			log << TestLog::EndSection;

			log << TestLog::Section("Test2", "EGL_BAD_PARAMETER is generated if name is not an accepted value");

			expectNull(eglQueryString(display, 0));
			expectError(EGL_BAD_PARAMETER);
			expectNull(eglQueryString(display, -1));
			expectError(EGL_BAD_PARAMETER);

			log << TestLog::EndSection;
		});

	TEGL_ADD_API_CASE(query_surface, "eglQuerySurface() negative tests",
		{
			TestLog&	log			= m_testCtx.getLog();
			EGLDisplay	display		= getDisplay();
			EGLint		value		= 0;

			log << TestLog::Section("Test1", "EGL_BAD_DISPLAY is generated if display is not an EGL display connection");

			expectFalse(eglQuerySurface(EGL_NO_DISPLAY, DE_NULL, EGL_CONFIG_ID, &value));
			expectError(EGL_BAD_DISPLAY);

			expectFalse(eglQuerySurface((EGLDisplay)-1, DE_NULL, EGL_CONFIG_ID, &value));
			expectError(EGL_BAD_DISPLAY);

			log << TestLog::EndSection;

			log << TestLog::Section("Test2", "EGL_BAD_SURFACE is generated if surface is not an EGL surface");

			expectFalse(eglQuerySurface(display, DE_NULL, EGL_CONFIG_ID, &value));
			expectError(EGL_BAD_SURFACE);

			expectFalse(eglQuerySurface(display, (EGLSurface)-1, EGL_CONFIG_ID, &value));
			expectError(EGL_BAD_SURFACE);

			log << TestLog::EndSection;

			// Create pbuffer surface.
			EGLSurface surface = EGL_NO_SURFACE;
			{
				EGLConfig config;
				if (getConfig(&config, FilterList() << surfaceBits<EGL_PBUFFER_BIT>))
				{
					surface = eglCreatePbufferSurface(display, config, s_validGenericPbufferAttrib);
					expectError(EGL_SUCCESS);
				}
				else
					log << TestLog::Message << "// WARNING: No suitable config found, testing will be incomplete" << TestLog::EndMessage;
			}

			log << TestLog::Section("Test3", "EGL_BAD_ATTRIBUTE is generated if attribute is not a valid surface attribute");

			if (surface)
			{
				expectFalse(eglQuerySurface(display, surface, 0, &value));
				expectError(EGL_BAD_ATTRIBUTE);

				expectFalse(eglQuerySurface(display, surface, -1, &value));
				expectError(EGL_BAD_ATTRIBUTE);
			}

			log << TestLog::EndSection;

			if (surface)
			{
				eglDestroySurface(display, surface);
				expectError(EGL_SUCCESS);
			}
		});

	TEGL_ADD_API_CASE(release_tex_image, "eglReleaseTexImage() negative tests",
		{
			TestLog&	log			= m_testCtx.getLog();
			EGLDisplay	display		= getDisplay();

			log << TestLog::Section("Test1", "EGL_BAD_DISPLAY is generated if display is not an EGL display connection");

			expectFalse(eglReleaseTexImage(EGL_NO_DISPLAY, EGL_NO_SURFACE, EGL_BACK_BUFFER));
			expectError(EGL_BAD_DISPLAY);

			expectFalse(eglReleaseTexImage((EGLDisplay)-1, EGL_NO_SURFACE, EGL_BACK_BUFFER));
			expectError(EGL_BAD_DISPLAY);

			log << TestLog::EndSection;

			log << TestLog::Section("Test2", "EGL_BAD_SURFACE is generated if surface is not an EGL surface");

			expectFalse(eglReleaseTexImage(display, EGL_NO_SURFACE, EGL_BACK_BUFFER));
			expectError(EGL_BAD_SURFACE);

			expectFalse(eglReleaseTexImage(display, (EGLSurface)-1, EGL_BACK_BUFFER));
			expectError(EGL_BAD_SURFACE);

			log << TestLog::EndSection;
		});

	TEGL_ADD_API_CASE(surface_attrib, "eglSurfaceAttrib() negative tests",
		{
			TestLog&	log			= m_testCtx.getLog();
			EGLDisplay	display		= getDisplay();

			log << TestLog::Section("Test1", "EGL_BAD_DISPLAY is generated if display is not an EGL display connection");

			expectFalse(eglSurfaceAttrib(EGL_NO_DISPLAY, DE_NULL, EGL_SWAP_BEHAVIOR, EGL_BUFFER_DESTROYED));
			expectError(EGL_BAD_DISPLAY);

			expectFalse(eglSurfaceAttrib((EGLDisplay)-1, DE_NULL, EGL_SWAP_BEHAVIOR, EGL_BUFFER_DESTROYED));
			expectError(EGL_BAD_DISPLAY);

			log << TestLog::EndSection;

			log << TestLog::Section("Test2", "EGL_BAD_SURFACE is generated if surface is not an EGL surface");

			expectFalse(eglSurfaceAttrib(display, DE_NULL, EGL_SWAP_BEHAVIOR, EGL_BUFFER_DESTROYED));
			expectError(EGL_BAD_SURFACE);

			expectFalse(eglSurfaceAttrib(display, (EGLSurface)-1, EGL_SWAP_BEHAVIOR, EGL_BUFFER_DESTROYED));
			expectError(EGL_BAD_SURFACE);

			log << TestLog::EndSection;

			{
				// Create pbuffer surface.
				EGLSurface surface = EGL_NO_SURFACE;
				{
					EGLConfig config;
					if (getConfig(&config, FilterList() << surfaceBits<EGL_PBUFFER_BIT>))
					{
						surface = eglCreatePbufferSurface(display, config, s_validGenericPbufferAttrib);
						expectError(EGL_SUCCESS);
					}
					else
						log << TestLog::Message << "// WARNING: No suitable config found, testing will be incomplete" << TestLog::EndMessage;
				}

				log << TestLog::Section("Test3", "EGL_BAD_ATTRIBUTE is generated if attribute is not a valid surface attribute");

				if (surface)
				{
					expectFalse(eglSurfaceAttrib(display, surface, 0, 0));
					expectError(EGL_BAD_ATTRIBUTE);

					expectFalse(eglSurfaceAttrib(display, surface, -1, 0));
					expectError(EGL_BAD_ATTRIBUTE);
				}

				log << TestLog::EndSection;

				if (surface)
				{
					eglDestroySurface(display, surface);
					expectError(EGL_SUCCESS);
				}
			}

			{
				// Create pbuffer surface without EGL_MULTISAMPLE_RESOLVE_BOX_BIT.
				EGLSurface surface = EGL_NO_SURFACE;
				{
					EGLConfig config;
					if (getConfig(&config, FilterList() << surfaceBits<EGL_PBUFFER_BIT> << notSurfaceBits<EGL_MULTISAMPLE_RESOLVE_BOX_BIT>))
					{
						surface = eglCreatePbufferSurface(display, config, s_validGenericPbufferAttrib);
						expectError(EGL_SUCCESS);
					}
					else
						log << TestLog::Message << "// WARNING: No suitable config found, testing will be incomplete" << TestLog::EndMessage;
				}

				log << TestLog::Section("Test4", "EGL_BAD_MATCH is generated if attribute is EGL_MULTISAMPLE_RESOLVE, value is EGL_MULTISAMPLE_RESOLVE_BOX, and the EGL_SURFACE_TYPE attribute of the EGLConfig used to create surface does not contain EGL_MULTISAMPLE_RESOLVE_BOX_BIT");

				if (surface)
				{
					expectFalse(eglSurfaceAttrib(display, surface, EGL_MULTISAMPLE_RESOLVE, EGL_MULTISAMPLE_RESOLVE_BOX));
					expectError(EGL_BAD_MATCH);
				}

				log << TestLog::EndSection;

				if (surface)
				{
					eglDestroySurface(display, surface);
					expectError(EGL_SUCCESS);
				}
			}

			{
				// Create pbuffer surface without EGL_SWAP_BEHAVIOR_PRESERVED_BIT.
				EGLSurface surface = EGL_NO_SURFACE;
				{
					EGLConfig config;
					if (getConfig(&config, FilterList() << surfaceBits<EGL_PBUFFER_BIT> << notSurfaceBits<EGL_SWAP_BEHAVIOR_PRESERVED_BIT>))
					{
						surface = eglCreatePbufferSurface(display, config, s_validGenericPbufferAttrib);
						expectError(EGL_SUCCESS);
					}
					else
						log << TestLog::Message << "// WARNING: No suitable config found, testing will be incomplete" << TestLog::EndMessage;
				}

				log << TestLog::Section("Test5", "EGL_BAD_MATCH is generated if attribute is EGL_SWAP_BEHAVIOR, value is EGL_BUFFER_PRESERVED, and the EGL_SURFACE_TYPE attribute of the EGLConfig used to create surface does not contain EGL_SWAP_BEHAVIOR_PRESERVED_BIT");

				if (surface)
				{
					expectFalse(eglSurfaceAttrib(display, surface, EGL_SWAP_BEHAVIOR, EGL_BUFFER_PRESERVED));
					expectError(EGL_BAD_MATCH);
				}

				log << TestLog::EndSection;

				if (surface)
				{
					eglDestroySurface(display, surface);
					expectError(EGL_SUCCESS);
				}
			}
		});

	TEGL_ADD_API_CASE(swap_buffers, "eglSwapBuffers() negative tests",
		{
			TestLog&	log			= m_testCtx.getLog();
			EGLDisplay	display		= getDisplay();

			log << TestLog::Section("Test1", "EGL_BAD_DISPLAY is generated if display is not an EGL display connection");

			expectFalse(eglSwapBuffers(EGL_NO_DISPLAY, DE_NULL));
			expectError(EGL_BAD_DISPLAY);

			expectFalse(eglSwapBuffers((EGLDisplay)-1, DE_NULL));
			expectError(EGL_BAD_DISPLAY);

			log << TestLog::EndSection;

			log << TestLog::Section("Test2", "EGL_BAD_SURFACE is generated if surface is not an EGL surface");

			expectFalse(eglSwapBuffers(display, DE_NULL));
			expectError(EGL_BAD_SURFACE);

			expectFalse(eglSwapBuffers(display, (EGLSurface)-1));
			expectError(EGL_BAD_SURFACE);

			log << TestLog::EndSection;
		});

	TEGL_ADD_API_CASE(swap_interval, "eglSwapInterval() negative tests",
		{
			TestLog&	log			= m_testCtx.getLog();
			EGLDisplay	display		= getDisplay();

			log << TestLog::Section("Test1", "EGL_BAD_DISPLAY is generated if display is not an EGL display connection");

			expectFalse(eglSwapInterval(EGL_NO_DISPLAY, 0));
			expectError(EGL_BAD_DISPLAY);

			expectFalse(eglSwapInterval((EGLDisplay)-1, 0));
			expectError(EGL_BAD_DISPLAY);

			log << TestLog::EndSection;

			log << TestLog::Section("Test2", "EGL_BAD_CONTEXT is generated if there is no current context on the calling thread");

			expectFalse(eglSwapInterval(display, 0));
			expectError(EGL_BAD_CONTEXT);

			log << TestLog::EndSection;
		});

	TEGL_ADD_API_CASE(terminate, "eglTerminate() negative tests",
		{
			TestLog&	log			= m_testCtx.getLog();

			log << TestLog::Section("Test1", "EGL_BAD_DISPLAY is generated if display is not an EGL display connection");

			expectFalse(eglTerminate(EGL_NO_DISPLAY));
			expectError(EGL_BAD_DISPLAY);

			expectFalse(eglTerminate((EGLDisplay)-1));
			expectError(EGL_BAD_DISPLAY);

			log << TestLog::EndSection;
		});

	TEGL_ADD_API_CASE(wait_native, "eglWaitNative() negative tests",
		{
			EGLConfig				config			= DE_NULL;
			bool					gotConfig		= getConfig(&config, FilterList() << renderable<EGL_OPENGL_ES2_BIT> << surfaceBits<EGL_WINDOW_BIT>);

			if (gotConfig)
			{
				TestLog&							log				= m_testCtx.getLog();
				const Library&						egl				= m_eglTestCtx.getLibrary();
				EGLDisplay							display			= getDisplay();
				const eglu::NativeWindowFactory&	factory			= eglu::selectNativeWindowFactory(m_eglTestCtx.getNativeDisplayFactory(), m_testCtx.getCommandLine());
				de::UniquePtr<eglu::NativeWindow>	window			(factory.createWindow(&m_eglTestCtx.getNativeDisplay(), display, config, DE_NULL, eglu::WindowParams(256, 256, eglu::parseWindowVisibility(m_testCtx.getCommandLine()))));
				eglu::UniqueSurface					surface			(egl, display, eglu::createWindowSurface(m_eglTestCtx.getNativeDisplay(), *window, display, config, DE_NULL));
				EGLContext							context			= EGL_NO_CONTEXT;

				expectTrue(eglBindAPI(EGL_OPENGL_ES_API));
				expectError(EGL_SUCCESS);

				context = eglCreateContext(display, config, EGL_NO_CONTEXT, s_es2ContextAttribList);
				expectError(EGL_SUCCESS);

				expectTrue(eglMakeCurrent(display, *surface, *surface, context));
				expectError(EGL_SUCCESS);

				log << TestLog::Section("Test1", "EGL_BAD_PARAMETER is generated if engine is not a recognized marking engine and native rendering is supported by current surface");

				eglWaitNative(-1);
				expectEitherError(EGL_BAD_PARAMETER, EGL_SUCCESS);

				log << TestLog::EndSection;

				expectTrue(eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
				expectError(EGL_SUCCESS);

				if (context != EGL_NO_CONTEXT)
				{
					expectTrue(eglDestroyContext(display, context));
					expectError(EGL_SUCCESS);
				}
			}
		});
}

} // egl
} // deqp
