// Copyright 2016 The SwiftShader Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Config.h: Defines the egl::Config class, describing the format, type
// and size for an egl::Surface. Implements EGLConfig and related functionality.
// [EGL 1.4] section 3.4 page 15.

#ifndef INCLUDE_CONFIG_H_
#define INCLUDE_CONFIG_H_

#include "Renderer/Surface.hpp"

#include <EGL/egl.h>

#include <set>

namespace egl
{
class Display;

class Config
{
public:
	Config(sw::Format displayFormat, EGLint minSwapInterval, EGLint maxSwapInterval, sw::Format renderTargetFormat, sw::Format depthStencilFormat, EGLint multiSample);

	EGLConfig getHandle() const;

	const sw::Format mRenderTargetFormat;
	const sw::Format mDepthStencilFormat;
	const EGLint mMultiSample;

	EGLint mBufferSize;              // Depth of the color buffer
	EGLint mRedSize;                 // Bits of Red in the color buffer
	EGLint mGreenSize;               // Bits of Green in the color buffer
	EGLint mBlueSize;                // Bits of Blue in the color buffer
	EGLint mLuminanceSize;           // Bits of Luminance in the color buffer
	EGLint mAlphaSize;               // Bits of Alpha in the color buffer
	EGLint mAlphaMaskSize;           // Bits of Alpha Mask in the mask buffer
	EGLBoolean mBindToTextureRGB;    // True if bindable to RGB textures.
	EGLBoolean mBindToTextureRGBA;   // True if bindable to RGBA textures.
	EGLenum mColorBufferType;        // Color buffer type
	EGLenum mConfigCaveat;           // Any caveats for the configuration
	EGLint mConfigID;                // Unique EGLConfig identifier
	EGLint mConformant;              // Whether contexts created with this config are conformant
	EGLint mDepthSize;               // Bits of Z in the depth buffer
	EGLint mLevel;                   // Frame buffer level
	EGLBoolean mMatchNativePixmap;   // Match the native pixmap format
	EGLint mMaxPBufferWidth;         // Maximum width of pbuffer
	EGLint mMaxPBufferHeight;        // Maximum height of pbuffer
	EGLint mMaxPBufferPixels;        // Maximum size of pbuffer
	EGLint mMaxSwapInterval;         // Maximum swap interval
	EGLint mMinSwapInterval;         // Minimum swap interval
	EGLBoolean mNativeRenderable;    // EGL_TRUE if native rendering APIs can render to surface
	EGLint mNativeVisualID;          // Handle of corresponding native visual
	EGLint mNativeVisualType;        // Native visual type of the associated visual
	EGLint mRenderableType;          // Which client rendering APIs are supported.
	EGLint mSampleBuffers;           // Number of multisample buffers
	EGLint mSamples;                 // Number of samples per pixel
	EGLint mStencilSize;             // Bits of Stencil in the stencil buffer
	EGLint mSurfaceType;             // Which types of EGL surfaces are supported.
	EGLenum mTransparentType;        // Type of transparency supported
	EGLint mTransparentRedValue;     // Transparent red value
	EGLint mTransparentGreenValue;   // Transparent green value
	EGLint mTransparentBlueValue;    // Transparent blue value

	EGLBoolean mRecordableAndroid;          // EGL_ANDROID_recordable
	EGLBoolean mFramebufferTargetAndroid;   // EGL_ANDROID_framebuffer_target
};

struct CompareConfig
{
	bool operator()(const Config &x, const Config &y) const;
};

class ConfigSet
{
	friend class Display;

public:
	ConfigSet();

	void add(sw::Format displayFormat, EGLint minSwapInterval, EGLint maxSwapInterval, sw::Format renderTargetFormat, sw::Format depthStencilFormat, EGLint multiSample);
	size_t size() const;
	bool getConfigs(EGLConfig *configs, const EGLint *attribList, EGLint configSize, EGLint *numConfig);
	const egl::Config *get(EGLConfig configHandle);

private:
	typedef std::set<Config, CompareConfig> Set;
	typedef Set::iterator Iterator;
	Set mSet;
};
}

#endif   // INCLUDE_CONFIG_H_
