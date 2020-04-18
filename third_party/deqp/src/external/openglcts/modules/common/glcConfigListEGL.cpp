/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2016 Google Inc.
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
 * \brief CTS rendering configuration list utility.
 */ /*-------------------------------------------------------------------*/

#include "glcConfigListEGL.hpp"

#include "deUniquePtr.hpp"
#include "glcConfigList.hpp"

#include <typeinfo>

#include "deUniquePtr.hpp"
#include "egluNativeDisplay.hpp"
#include "egluPlatform.hpp"
#include "egluUtil.hpp"
#include "eglwDefs.hpp"
#include "eglwEnums.hpp"
#include "tcuPlatform.hpp"

#if !defined(EGL_OPENGL_ES3_BIT_KHR)
#define EGL_OPENGL_ES3_BIT_KHR 0x0040
#endif

#if !defined(EGL_COLOR_COMPONENT_TYPE_EXT)
#define EGL_COLOR_COMPONENT_TYPE_EXT 0x3339
#endif

#if !defined(EGL_COLOR_COMPONENT_TYPE_FLOAT_EXT)
#define EGL_COLOR_COMPONENT_TYPE_FLOAT_EXT 0x333B
#endif

#if !defined(EGL_YUV_BUFFER_EXT)
#define EGL_YUV_BUFFER_EXT 0x3300
#endif

namespace glcts
{

static void getDefaultEglConfigList(tcu::Platform& eglPlatform, glu::ApiType type, ConfigList& configList)
{
	deUint32 renderableMask = 0;
	deUint32 conformantMask = 0;

	if (type == glu::ApiType::es(2, 0))
	{
		renderableMask = EGL_OPENGL_ES2_BIT;
		conformantMask = EGL_OPENGL_ES2_BIT;
	}
	else if (type == glu::ApiType::es(3, 0))
	{
		renderableMask = EGL_OPENGL_ES3_BIT_KHR;
		conformantMask = EGL_OPENGL_ES3_BIT_KHR;
	}
	else if (type == glu::ApiType::es(3, 1))
	{
		renderableMask = EGL_OPENGL_ES3_BIT_KHR;
		conformantMask = EGL_OPENGL_ES3_BIT_KHR;
	}
	else if (type == glu::ApiType::es(3, 2))
	{
		renderableMask = EGL_OPENGL_ES3_BIT_KHR;
		conformantMask = EGL_OPENGL_ES3_BIT_KHR;
	}
	else if (type.getProfile() == glu::PROFILE_CORE)
	{
		renderableMask = EGL_OPENGL_BIT;
		conformantMask = EGL_OPENGL_BIT;
	}
	else
	{
		throw tcu::Exception("Unsupported context type");
	}

	de::UniquePtr<eglu::NativeDisplay> nativeDisplay(
		eglPlatform.getEGLPlatform().getNativeDisplayFactoryRegistry().getDefaultFactory()->createDisplay());
	const eglw::Library&		 library = nativeDisplay->getLibrary();
	eglw::EGLDisplay			 display = eglu::getAndInitDisplay(*nativeDisplay);
	std::vector<eglw::EGLConfig> configs = eglu::getConfigs(library, display);
	bool supportFloatConfigs			 = eglu::hasExtension(library, display, "EGL_EXT_pixel_format_float");
	bool supportYUVConfigs				 = eglu::hasExtension(library, display, "EGL_EXT_yuv_surface");

	for (std::vector<eglw::EGLConfig>::iterator cfgIter = configs.begin(); cfgIter != configs.end(); cfgIter++)
	{
		int		 id				= eglu::getConfigAttribInt(library, display, *cfgIter, EGL_CONFIG_ID);
		deUint32 renderableBits = eglu::getConfigAttribInt(library, display, *cfgIter, EGL_RENDERABLE_TYPE);
		deUint32 conformantBits = eglu::getConfigAttribInt(library, display, *cfgIter, EGL_CONFORMANT);
		deInt32  redSize		= eglu::getConfigAttribInt(library, display, *cfgIter, EGL_RED_SIZE);
		deInt32  greenSize		= eglu::getConfigAttribInt(library, display, *cfgIter, EGL_GREEN_SIZE);
		deInt32  blueSize		= eglu::getConfigAttribInt(library, display, *cfgIter, EGL_BLUE_SIZE);
		deInt32  alphaSize		= eglu::getConfigAttribInt(library, display, *cfgIter, EGL_ALPHA_SIZE);
		deInt32  depthSize		= eglu::getConfigAttribInt(library, display, *cfgIter, EGL_DEPTH_SIZE);
		deInt32  stencilSize	= eglu::getConfigAttribInt(library, display, *cfgIter, EGL_STENCIL_SIZE);
		deInt32  numSamples		= eglu::getConfigAttribInt(library, display, *cfgIter, EGL_SAMPLES);

		bool isRenderable = (renderableBits & renderableMask) == renderableMask;
		bool isConformant = (conformantBits & conformantMask) == conformantMask;
		bool isAOSPOk	 = isRenderable && isConformant;
		bool isFloatType  = supportFloatConfigs ?
							   (eglu::getConfigAttribInt(library, display, *cfgIter, EGL_COLOR_COMPONENT_TYPE_EXT) ==
								EGL_COLOR_COMPONENT_TYPE_FLOAT_EXT) :
							   false;
		bool isYUV =
			supportYUVConfigs ?
				(eglu::getConfigAttribInt(library, display, *cfgIter, EGL_COLOR_BUFFER_TYPE) == EGL_YUV_BUFFER_EXT) :
				false;
		bool isOk = isRenderable && isConformant && (numSamples == 0) && !isFloatType && !isYUV;

		deUint32 surfaceBits  = eglu::getConfigAttribInt(library, display, *cfgIter, EGL_SURFACE_TYPE);
		deUint32 surfaceTypes = ((surfaceBits & EGL_WINDOW_BIT) ? SURFACETYPE_WINDOW : 0) |
								((surfaceBits & EGL_PIXMAP_BIT) ? SURFACETYPE_PIXMAP : 0) |
								((surfaceBits & EGL_PBUFFER_BIT) ? SURFACETYPE_PBUFFER : 0);

		if (isAOSPOk)
		{
			configList.aospConfigs.push_back(AOSPConfig(CONFIGTYPE_EGL, id, surfaceTypes, redSize, greenSize, blueSize,
														alphaSize, depthSize, stencilSize, numSamples));
		}

		if (isOk)
		{
			configList.configs.push_back(Config(CONFIGTYPE_EGL, id, surfaceTypes));
		}
		else
		{
			DE_ASSERT(!isRenderable || !isConformant || (numSamples != 0) || isFloatType || isYUV);
			configList.excludedConfigs.push_back(
				ExcludedConfig(CONFIGTYPE_EGL, id,
							   !isRenderable ? EXCLUDEREASON_NOT_COMPATIBLE :
											   !isConformant ? EXCLUDEREASON_NOT_CONFORMANT :
															   (numSamples != 0) ?
															   EXCLUDEREASON_MSAA :
															   isFloatType ? EXCLUDEREASON_FLOAT : EXCLUDEREASON_YUV));
		}
	}
}

void getConfigListEGL(tcu::Platform& platform, glu::ApiType type, ConfigList& configList)
{
	try
	{
		getDefaultEglConfigList(platform, type, configList);
	}
	catch (const std::bad_cast&)
	{
		throw tcu::Exception("Platform is not tcu::EglPlatform");
	}
}

} // glcts
