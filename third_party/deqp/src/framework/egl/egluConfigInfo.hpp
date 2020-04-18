#ifndef _EGLUCONFIGINFO_HPP
#define _EGLUCONFIGINFO_HPP
/*-------------------------------------------------------------------------
 * drawElements Quality Program Tester Core
 * ----------------------------------------
 *
 * Copyright 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *//*!
 * \file
 * \brief EGL config info.
 *//*--------------------------------------------------------------------*/

#include "tcuDefs.hpp"
#include "eglwDefs.hpp"
#include "eglwEnums.hpp"

namespace eglw
{
class Library;
}

namespace eglu
{

class ConfigInfo
{
public:
	// Core attributes
	deInt32			bufferSize;
	deInt32			redSize;
	deInt32			greenSize;
	deInt32			blueSize;
	deInt32			luminanceSize;
	deInt32			alphaSize;
	deInt32			alphaMaskSize;
	deUint32		bindToTextureRGB;
	deUint32		bindToTextureRGBA;
	deUint32		colorBufferType;
	deUint32		configCaveat;
	deInt32			configId;
	deInt32			conformant;
	deInt32			depthSize;
	deInt32			level;
	deInt32			maxPbufferWidth;
	deInt32			maxPbufferHeight;
	deInt32			maxSwapInterval;
	deInt32			minSwapInterval;
	deUint32		nativeRenderable;
	deInt32			nativeVisualId;
	deInt32			nativeVisualType;
	deInt32			renderableType;
	deInt32			sampleBuffers;
	deInt32			samples;
	deInt32			stencilSize;
	deInt32			surfaceType;
	deUint32		transparentType;
	deInt32			transparentRedValue;
	deInt32			transparentGreenValue;
	deInt32			transparentBlueValue;

	// Extension attributes - set by queryExtConfigInfo()

	// EGL_EXT_yuv_surface
	deUint32		yuvOrder;
	deInt32			yuvNumberOfPlanes;
	deUint32		yuvSubsample;
	deUint32		yuvDepthRange;
	deUint32		yuvCscStandard;
	deInt32			yuvPlaneBpp;

	// EGL_EXT_pixel_format_float
	deUint32		colorComponentType;

	ConfigInfo (void)
		: bufferSize			(0)
		, redSize				(0)
		, greenSize				(0)
		, blueSize				(0)
		, luminanceSize			(0)
		, alphaSize				(0)
		, alphaMaskSize			(0)
		, bindToTextureRGB		(0)
		, bindToTextureRGBA		(0)
		, colorBufferType		(0)
		, configCaveat			(0)
		, configId				(0)
		, conformant			(0)
		, depthSize				(0)
		, level					(0)
		, maxPbufferWidth		(0)
		, maxPbufferHeight		(0)
		, maxSwapInterval		(0)
		, minSwapInterval		(0)
		, nativeRenderable		(0)
		, nativeVisualId		(0)
		, nativeVisualType		(0)
		, renderableType		(0)
		, sampleBuffers			(0)
		, samples				(0)
		, stencilSize			(0)
		, surfaceType			(0)
		, transparentType		(0)
		, transparentRedValue	(0)
		, transparentGreenValue	(0)
		, transparentBlueValue	(0)
		, yuvOrder				(EGL_NONE)
		, yuvNumberOfPlanes		(0)
		, yuvSubsample			(EGL_NONE)
		, yuvDepthRange			(EGL_NONE)
		, yuvCscStandard		(EGL_NONE)
		, yuvPlaneBpp			(EGL_YUV_PLANE_BPP_0_EXT)
		, colorComponentType	(EGL_NONE)
	{
	}

	deInt32 getAttribute (deUint32 attribute) const;
};

void	queryCoreConfigInfo	(const eglw::Library& egl, eglw::EGLDisplay display, eglw::EGLConfig config, ConfigInfo* dst);
void	queryExtConfigInfo	(const eglw::Library& egl, eglw::EGLDisplay display, eglw::EGLConfig config, ConfigInfo* dst);

} // eglu

#endif // _EGLUCONFIGINFO_HPP
