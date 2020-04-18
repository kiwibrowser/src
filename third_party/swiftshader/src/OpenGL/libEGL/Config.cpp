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

// Config.cpp: Implements the egl::Config class, describing the format, type
// and size for an egl::Surface. Implements EGLConfig and related functionality.
// [EGL 1.4] section 3.4 page 15.

#include "Config.h"

#include "common/debug.h"

#include <EGL/eglext.h>
#ifdef __ANDROID__
#include <system/graphics.h>
#endif

#include <string.h>
#include <algorithm>
#include <cstring>
#include <vector>

using namespace std;

namespace egl
{

Config::Config(sw::Format displayFormat, EGLint minInterval, EGLint maxInterval, sw::Format renderTargetFormat, sw::Format depthStencilFormat, EGLint multiSample)
	: mRenderTargetFormat(renderTargetFormat), mDepthStencilFormat(depthStencilFormat), mMultiSample(multiSample)
{
	mBindToTextureRGB = EGL_FALSE;
	mBindToTextureRGBA = EGL_FALSE;

	// Initialize to a high value to lower the preference of formats for which there's no native support
	mNativeVisualID = 0x7FFFFFFF;

	switch(renderTargetFormat)
	{
	case sw::FORMAT_A1R5G5B5:
		mRedSize = 5;
		mGreenSize = 5;
		mBlueSize = 5;
		mAlphaSize = 1;
		break;
	case sw::FORMAT_A2R10G10B10:
		mRedSize = 10;
		mGreenSize = 10;
		mBlueSize = 10;
		mAlphaSize = 2;
		break;
	case sw::FORMAT_A8R8G8B8:
		mRedSize = 8;
		mGreenSize = 8;
		mBlueSize = 8;
		mAlphaSize = 8;
		mBindToTextureRGBA = EGL_TRUE;
		#ifdef __ANDROID__
			mNativeVisualID = HAL_PIXEL_FORMAT_BGRA_8888;
		#else
			mNativeVisualID = 2;   // Arbitrary; prefer over ABGR
		#endif
		break;
	case sw::FORMAT_A8B8G8R8:
		mRedSize = 8;
		mGreenSize = 8;
		mBlueSize = 8;
		mAlphaSize = 8;
		mBindToTextureRGBA = EGL_TRUE;
		#ifdef __ANDROID__
			mNativeVisualID = HAL_PIXEL_FORMAT_RGBA_8888;
		#endif
		break;
	case sw::FORMAT_R5G6B5:
		mRedSize = 5;
		mGreenSize = 6;
		mBlueSize = 5;
		mAlphaSize = 0;
		#ifdef __ANDROID__
			mNativeVisualID = HAL_PIXEL_FORMAT_RGB_565;
		#endif
		break;
	case sw::FORMAT_X8R8G8B8:
		mRedSize = 8;
		mGreenSize = 8;
		mBlueSize = 8;
		mAlphaSize = 0;
		mBindToTextureRGB = EGL_TRUE;
		#ifdef __ANDROID__
			mNativeVisualID = 0x1FF;   // HAL_PIXEL_FORMAT_BGRX_8888
		#else
			mNativeVisualID = 1;   // Arbitrary; prefer over XBGR
		#endif
		break;
	case sw::FORMAT_X8B8G8R8:
		mRedSize = 8;
		mGreenSize = 8;
		mBlueSize = 8;
		mAlphaSize = 0;
		mBindToTextureRGB = EGL_TRUE;
		#ifdef __ANDROID__
			mNativeVisualID = HAL_PIXEL_FORMAT_RGBX_8888;
		#endif
		break;
	default:
		UNREACHABLE(renderTargetFormat);
	}

	mLuminanceSize = 0;
	mBufferSize = mRedSize + mGreenSize + mBlueSize + mLuminanceSize + mAlphaSize;
	mAlphaMaskSize = 0;
	mColorBufferType = EGL_RGB_BUFFER;
	mConfigCaveat = EGL_NONE;
	mConfigID = 0;
	mConformant = EGL_OPENGL_ES_BIT | EGL_OPENGL_ES2_BIT | EGL_OPENGL_ES3_BIT;

	switch(depthStencilFormat)
	{
	case sw::FORMAT_NULL:
		mDepthSize = 0;
		mStencilSize = 0;
		break;
//	case sw::FORMAT_D16_LOCKABLE:
//		mDepthSize = 16;
//		mStencilSize = 0;
//		break;
	case sw::FORMAT_D32:
		mDepthSize = 32;
		mStencilSize = 0;
		break;
//	case sw::FORMAT_D15S1:
//		mDepthSize = 15;
//		mStencilSize = 1;
//		break;
	case sw::FORMAT_D24S8:
		mDepthSize = 24;
		mStencilSize = 8;
		break;
	case sw::FORMAT_D24X8:
		mDepthSize = 24;
		mStencilSize = 0;
		break;
//	case sw::FORMAT_D24X4S4:
//		mDepthSize = 24;
//		mStencilSize = 4;
//		break;
	case sw::FORMAT_D16:
		mDepthSize = 16;
		mStencilSize = 0;
		break;
//	case sw::FORMAT_D32F_LOCKABLE:
//		mDepthSize = 32;
//		mStencilSize = 0;
//		break;
//	case sw::FORMAT_D24FS8:
//		mDepthSize = 24;
//		mStencilSize = 8;
//		break;
	default:
		UNREACHABLE(depthStencilFormat);
	}

	mLevel = 0;
	mMatchNativePixmap = EGL_NONE;
	mMaxPBufferWidth = 4096;
	mMaxPBufferHeight = 4096;
	mMaxPBufferPixels = mMaxPBufferWidth * mMaxPBufferHeight;
	mMaxSwapInterval = maxInterval;
	mMinSwapInterval = minInterval;
	mNativeRenderable = EGL_FALSE;
	mNativeVisualType = 0;
	mRenderableType = EGL_OPENGL_ES_BIT | EGL_OPENGL_ES2_BIT | EGL_OPENGL_ES3_BIT;
	mSampleBuffers = (multiSample > 0) ? 1 : 0;
	mSamples = multiSample;
	mSurfaceType = EGL_PBUFFER_BIT | EGL_WINDOW_BIT | EGL_SWAP_BEHAVIOR_PRESERVED_BIT | EGL_MULTISAMPLE_RESOLVE_BOX_BIT;
	mTransparentType = EGL_NONE;
	mTransparentRedValue = 0;
	mTransparentGreenValue = 0;
	mTransparentBlueValue = 0;

	// Although we could support any format as an Android HWComposer compatible config by converting when necessary,
	// the intent of EGL_ANDROID_framebuffer_target is to prevent any copies or conversions.
	mFramebufferTargetAndroid = (displayFormat == renderTargetFormat) ? EGL_TRUE : EGL_FALSE;
	mRecordableAndroid = EGL_TRUE;
}

EGLConfig Config::getHandle() const
{
	return (EGLConfig)(size_t)mConfigID;
}

// This ordering determines the config ID
bool CompareConfig::operator()(const Config &x, const Config &y) const
{
	#define SORT_SMALLER(attribute)                \
		if(x.attribute != y.attribute)             \
		{                                          \
			return x.attribute < y.attribute;      \
		}

	static_assert(EGL_NONE < EGL_SLOW_CONFIG && EGL_SLOW_CONFIG < EGL_NON_CONFORMANT_CONFIG, "");
	SORT_SMALLER(mConfigCaveat);

	static_assert(EGL_RGB_BUFFER < EGL_LUMINANCE_BUFFER, "");
	SORT_SMALLER(mColorBufferType);

	SORT_SMALLER(mRedSize);
	SORT_SMALLER(mGreenSize);
	SORT_SMALLER(mBlueSize);
	SORT_SMALLER(mAlphaSize);

	SORT_SMALLER(mBufferSize);
	SORT_SMALLER(mSampleBuffers);
	SORT_SMALLER(mSamples);
	SORT_SMALLER(mDepthSize);
	SORT_SMALLER(mStencilSize);
	SORT_SMALLER(mAlphaMaskSize);
	SORT_SMALLER(mNativeVisualType);
	SORT_SMALLER(mNativeVisualID);

	#undef SORT_SMALLER

	// Strict ordering requires sorting all non-equal fields above
	assert(memcmp(&x, &y, sizeof(Config)) == 0);

	return false;
}

// Function object used by STL sorting routines for ordering Configs according to [EGL] section 3.4.1 page 24.
class SortConfig
{
public:
	explicit SortConfig(const EGLint *attribList);

	bool operator()(const Config *x, const Config *y) const;

private:
	EGLint wantedComponentsSize(const Config *config) const;

	bool mWantRed;
	bool mWantGreen;
	bool mWantBlue;
	bool mWantAlpha;
	bool mWantLuminance;
};

SortConfig::SortConfig(const EGLint *attribList)
	: mWantRed(false), mWantGreen(false), mWantBlue(false), mWantAlpha(false), mWantLuminance(false)
{
	// [EGL] section 3.4.1 page 24
	// Sorting rule #3: by larger total number of color bits,
	// not considering components that are 0 or don't-care.
	for(const EGLint *attr = attribList; attr[0] != EGL_NONE; attr += 2)
	{
		if(attr[1] != 0 && attr[1] != EGL_DONT_CARE)
		{
			switch(attr[0])
			{
			case EGL_RED_SIZE:       mWantRed = true;       break;
			case EGL_GREEN_SIZE:     mWantGreen = true;     break;
			case EGL_BLUE_SIZE:      mWantBlue = true;      break;
			case EGL_ALPHA_SIZE:     mWantAlpha = true;     break;
			case EGL_LUMINANCE_SIZE: mWantLuminance = true; break;
			}
		}
	}
}

EGLint SortConfig::wantedComponentsSize(const Config *config) const
{
	EGLint total = 0;

	if(mWantRed)       total += config->mRedSize;
	if(mWantGreen)     total += config->mGreenSize;
	if(mWantBlue)      total += config->mBlueSize;
	if(mWantAlpha)     total += config->mAlphaSize;
	if(mWantLuminance) total += config->mLuminanceSize;

	return total;
}

bool SortConfig::operator()(const Config *x, const Config *y) const
{
	#define SORT_SMALLER(attribute)                \
		if(x->attribute != y->attribute)           \
		{                                          \
			return x->attribute < y->attribute;    \
		}

	static_assert(EGL_NONE < EGL_SLOW_CONFIG && EGL_SLOW_CONFIG < EGL_NON_CONFORMANT_CONFIG, "");
	SORT_SMALLER(mConfigCaveat);

	static_assert(EGL_RGB_BUFFER < EGL_LUMINANCE_BUFFER, "");
	SORT_SMALLER(mColorBufferType);

	// By larger total number of color bits, only considering those that are requested to be > 0.
	EGLint xComponentsSize = wantedComponentsSize(x);
	EGLint yComponentsSize = wantedComponentsSize(y);
	if(xComponentsSize != yComponentsSize)
	{
		return xComponentsSize > yComponentsSize;
	}

	SORT_SMALLER(mBufferSize);
	SORT_SMALLER(mSampleBuffers);
	SORT_SMALLER(mSamples);
	SORT_SMALLER(mDepthSize);
	SORT_SMALLER(mStencilSize);
	SORT_SMALLER(mAlphaMaskSize);
	SORT_SMALLER(mNativeVisualType);
	SORT_SMALLER(mConfigID);

	#undef SORT_SMALLER

	return false;
}

ConfigSet::ConfigSet()
{
}

void ConfigSet::add(sw::Format displayFormat, EGLint minSwapInterval, EGLint maxSwapInterval, sw::Format renderTargetFormat, sw::Format depthStencilFormat, EGLint multiSample)
{
	Config conformantConfig(displayFormat, minSwapInterval, maxSwapInterval, renderTargetFormat, depthStencilFormat, multiSample);
	mSet.insert(conformantConfig);
}

size_t ConfigSet::size() const
{
	return mSet.size();
}

bool ConfigSet::getConfigs(EGLConfig *configs, const EGLint *attribList, EGLint configSize, EGLint *numConfig)
{
	vector<const Config*> passed;
	passed.reserve(mSet.size());

	for(Iterator config = mSet.begin(); config != mSet.end(); config++)
	{
		bool match = true;
		bool caveatMatch = (config->mConfigCaveat == EGL_NONE);
		const EGLint *attribute = attribList;

		while(attribute[0] != EGL_NONE)
		{
			if(attribute[1] != EGL_DONT_CARE)
			{
				switch(attribute[0])
				{
				case EGL_BUFFER_SIZE:                match = config->mBufferSize >= attribute[1];                           break;
				case EGL_ALPHA_SIZE:                 match = config->mAlphaSize >= attribute[1];                            break;
				case EGL_BLUE_SIZE:                  match = config->mBlueSize >= attribute[1];                             break;
				case EGL_GREEN_SIZE:                 match = config->mGreenSize >= attribute[1];                            break;
				case EGL_RED_SIZE:                   match = config->mRedSize >= attribute[1];                              break;
				case EGL_DEPTH_SIZE:                 match = config->mDepthSize >= attribute[1];                            break;
				case EGL_STENCIL_SIZE:               match = config->mStencilSize >= attribute[1];                          break;
				case EGL_CONFIG_CAVEAT:              match = config->mConfigCaveat == (EGLenum)attribute[1];                break;
				case EGL_CONFIG_ID:                  match = config->mConfigID == attribute[1];                             break;
				case EGL_LEVEL:                      match = config->mLevel >= attribute[1];                                break;
				case EGL_NATIVE_RENDERABLE:          match = config->mNativeRenderable == (EGLBoolean)attribute[1];         break;
				case EGL_NATIVE_VISUAL_TYPE:         match = config->mNativeVisualType == attribute[1];                     break;
				case EGL_SAMPLES:                    match = config->mSamples >= attribute[1];                              break;
				case EGL_SAMPLE_BUFFERS:             match = config->mSampleBuffers >= attribute[1];                        break;
				case EGL_SURFACE_TYPE:               match = (config->mSurfaceType & attribute[1]) == attribute[1];         break;
				case EGL_TRANSPARENT_TYPE:           match = config->mTransparentType == (EGLenum)attribute[1];             break;
				case EGL_TRANSPARENT_BLUE_VALUE:     match = config->mTransparentBlueValue == attribute[1];                 break;
				case EGL_TRANSPARENT_GREEN_VALUE:    match = config->mTransparentGreenValue == attribute[1];                break;
				case EGL_TRANSPARENT_RED_VALUE:      match = config->mTransparentRedValue == attribute[1];                  break;
				case EGL_BIND_TO_TEXTURE_RGB:        match = config->mBindToTextureRGB == (EGLBoolean)attribute[1];         break;
				case EGL_BIND_TO_TEXTURE_RGBA:       match = config->mBindToTextureRGBA == (EGLBoolean)attribute[1];        break;
				case EGL_MIN_SWAP_INTERVAL:          match = config->mMinSwapInterval == attribute[1];                      break;
				case EGL_MAX_SWAP_INTERVAL:          match = config->mMaxSwapInterval == attribute[1];                      break;
				case EGL_LUMINANCE_SIZE:             match = config->mLuminanceSize >= attribute[1];                        break;
				case EGL_ALPHA_MASK_SIZE:            match = config->mAlphaMaskSize >= attribute[1];                        break;
				case EGL_COLOR_BUFFER_TYPE:          match = config->mColorBufferType == (EGLenum)attribute[1];             break;
				case EGL_RENDERABLE_TYPE:            match = (config->mRenderableType & attribute[1]) == attribute[1];      break;
				case EGL_MATCH_NATIVE_PIXMAP:        match = false; UNIMPLEMENTED();                                        break;
				case EGL_CONFORMANT:                 match = (config->mConformant & attribute[1]) == attribute[1];          break;
				case EGL_RECORDABLE_ANDROID:         match = config->mRecordableAndroid == (EGLBoolean)attribute[1];        break;
				case EGL_FRAMEBUFFER_TARGET_ANDROID: match = config->mFramebufferTargetAndroid == (EGLBoolean)attribute[1]; break;

				// Ignored attributes
				case EGL_MAX_PBUFFER_WIDTH:
				case EGL_MAX_PBUFFER_HEIGHT:
				case EGL_MAX_PBUFFER_PIXELS:
				case EGL_NATIVE_VISUAL_ID:
					break;

				default:
					*numConfig = 0;
					return false;
				}

				if(!match)
				{
					break;
				}
			}

			if(attribute[0] == EGL_CONFIG_CAVEAT)
			{
				caveatMatch = match;
			}

			attribute += 2;
		}

		if(match && caveatMatch)   // We require the caveats to be NONE or the requested flags
		{
			passed.push_back(&*config);
		}
	}

	if(configs)
	{
		sort(passed.begin(), passed.end(), SortConfig(attribList));

		EGLint index;
		for(index = 0; index < configSize && index < static_cast<EGLint>(passed.size()); index++)
		{
			configs[index] = passed[index]->getHandle();
		}

		*numConfig = index;
	}
	else
	{
		*numConfig = static_cast<EGLint>(passed.size());
	}

	return true;
}

const egl::Config *ConfigSet::get(EGLConfig configHandle)
{
	for(Iterator config = mSet.begin(); config != mSet.end(); config++)
	{
		if(config->getHandle() == configHandle)
		{
			return &(*config);
		}
	}

	return nullptr;
}
}
