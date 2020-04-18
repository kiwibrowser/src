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

// Display.cpp: Implements the egl::Display class, representing the abstract
// display on which graphics are drawn. Implements EGLDisplay.
// [EGL 1.4] section 2.1.2 page 3.

#include "Display.h"

#include "main.h"
#include "libEGL/Surface.hpp"
#include "libEGL/Context.hpp"
#include "common/Image.hpp"
#include "common/debug.h"
#include "Common/MutexLock.hpp"

#ifdef __ANDROID__
#include <system/window.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <fcntl.h>
#elif defined(__linux__)
#include "Main/libX11.hpp"
#elif defined(__APPLE__)
#include "OSXUtils.hpp"
#include <CoreFoundation/CoreFoundation.h>
#include <IOSurface/IOSurface.h>
#endif

#include <algorithm>
#include <vector>
#include <map>

namespace egl
{

class DisplayImplementation : public Display
{
public:
	DisplayImplementation(EGLDisplay dpy, void *nativeDisplay) : Display(dpy, nativeDisplay) {}
	~DisplayImplementation() override {}

	Image *getSharedImage(EGLImageKHR name) override
	{
		return Display::getSharedImage(name);
	}
};

Display *Display::get(EGLDisplay dpy)
{
	if(dpy != PRIMARY_DISPLAY && dpy != HEADLESS_DISPLAY)   // We only support the default display
	{
		return nullptr;
	}

	static void *nativeDisplay = nullptr;

	#if defined(__linux__) && !defined(__ANDROID__)
		// Even if the application provides a native display handle, we open (and close) our own connection
		if(!nativeDisplay && dpy != HEADLESS_DISPLAY && libX11 && libX11->XOpenDisplay)
		{
			nativeDisplay = libX11->XOpenDisplay(NULL);
		}
	#endif

	static DisplayImplementation display(dpy, nativeDisplay);

	return &display;
}

Display::Display(EGLDisplay eglDisplay, void *nativeDisplay) : eglDisplay(eglDisplay), nativeDisplay(nativeDisplay)
{
	mMinSwapInterval = 1;
	mMaxSwapInterval = 1;
}

Display::~Display()
{
	terminate();

	#if defined(__linux__) && !defined(__ANDROID__)
		if(nativeDisplay && libX11->XCloseDisplay)
		{
			libX11->XCloseDisplay((::Display*)nativeDisplay);
		}
	#endif
}

#if !defined(__i386__) && defined(_M_IX86)
	#define __i386__ 1
#endif

#if !defined(__x86_64__) && (defined(_M_AMD64) || defined (_M_X64))
	#define __x86_64__ 1
#endif

static void cpuid(int registers[4], int info)
{
	#if defined(__i386__) || defined(__x86_64__)
		#if defined(_WIN32)
			__cpuid(registers, info);
		#else
			__asm volatile("cpuid": "=a" (registers[0]), "=b" (registers[1]), "=c" (registers[2]), "=d" (registers[3]): "a" (info));
		#endif
	#else
		registers[0] = 0;
		registers[1] = 0;
		registers[2] = 0;
		registers[3] = 0;
	#endif
}

static bool detectSSE()
{
	int registers[4];
	cpuid(registers, 1);
	return (registers[3] & 0x02000000) != 0;
}

bool Display::initialize()
{
	if(isInitialized())
	{
		return true;
	}

	#if defined(__i386__) || defined(__x86_64__)
		if(!detectSSE())
		{
			return false;
		}
	#endif

	mMinSwapInterval = 0;
	mMaxSwapInterval = 4;

	const int samples[] =
	{
		0,
		2,
		4
	};

	const sw::Format renderTargetFormats[] =
	{
	//	sw::FORMAT_A1R5G5B5,
	//  sw::FORMAT_A2R10G10B10,   // The color_ramp conformance test uses ReadPixels with UNSIGNED_BYTE causing it to think that rendering skipped a colour value.
		sw::FORMAT_A8R8G8B8,
		sw::FORMAT_A8B8G8R8,
		sw::FORMAT_R5G6B5,
	//  sw::FORMAT_X1R5G5B5,      // Has no compatible OpenGL ES renderbuffer format
		sw::FORMAT_X8R8G8B8,
		sw::FORMAT_X8B8G8R8
	};

	const sw::Format depthStencilFormats[] =
	{
		sw::FORMAT_NULL,
	//  sw::FORMAT_D16_LOCKABLE,
		sw::FORMAT_D32,
	//  sw::FORMAT_D15S1,
		sw::FORMAT_D24S8,
		sw::FORMAT_D24X8,
	//  sw::FORMAT_D24X4S4,
		sw::FORMAT_D16,
	//  sw::FORMAT_D32F_LOCKABLE,
	//  sw::FORMAT_D24FS8
	};

	sw::Format currentDisplayFormat = getDisplayFormat();
	ConfigSet configSet;

	for(unsigned int samplesIndex = 0; samplesIndex < sizeof(samples) / sizeof(int); samplesIndex++)
	{
		for(sw::Format renderTargetFormat : renderTargetFormats)
		{
			for(sw::Format depthStencilFormat : depthStencilFormats)
			{
				configSet.add(currentDisplayFormat, mMinSwapInterval, mMaxSwapInterval, renderTargetFormat, depthStencilFormat, samples[samplesIndex]);
			}
		}
	}

	// Give the sorted configs a unique ID and store them internally
	EGLint index = 1;
	for(ConfigSet::Iterator config = configSet.mSet.begin(); config != configSet.mSet.end(); config++)
	{
		Config configuration = *config;
		configuration.mConfigID = index;
		index++;

		mConfigSet.mSet.insert(configuration);
	}

	if(!isInitialized())
	{
		terminate();

		return false;
	}

	return true;
}

void Display::terminate()
{
	while(!mSurfaceSet.empty())
	{
		destroySurface(*mSurfaceSet.begin());
	}

	while(!mContextSet.empty())
	{
		destroyContext(*mContextSet.begin());
	}

	while(!mSharedImageNameSpace.empty())
	{
		destroySharedImage(reinterpret_cast<EGLImageKHR>((intptr_t)mSharedImageNameSpace.firstName()));
	}
}

bool Display::getConfigs(EGLConfig *configs, const EGLint *attribList, EGLint configSize, EGLint *numConfig)
{
	return mConfigSet.getConfigs(configs, attribList, configSize, numConfig);
}

bool Display::getConfigAttrib(EGLConfig config, EGLint attribute, EGLint *value)
{
	const egl::Config *configuration = mConfigSet.get(config);

	switch(attribute)
	{
	case EGL_BUFFER_SIZE:                *value = configuration->mBufferSize;               break;
	case EGL_ALPHA_SIZE:                 *value = configuration->mAlphaSize;                break;
	case EGL_BLUE_SIZE:                  *value = configuration->mBlueSize;                 break;
	case EGL_GREEN_SIZE:                 *value = configuration->mGreenSize;                break;
	case EGL_RED_SIZE:                   *value = configuration->mRedSize;                  break;
	case EGL_DEPTH_SIZE:                 *value = configuration->mDepthSize;                break;
	case EGL_STENCIL_SIZE:               *value = configuration->mStencilSize;              break;
	case EGL_CONFIG_CAVEAT:              *value = configuration->mConfigCaveat;             break;
	case EGL_CONFIG_ID:                  *value = configuration->mConfigID;                 break;
	case EGL_LEVEL:                      *value = configuration->mLevel;                    break;
	case EGL_NATIVE_RENDERABLE:          *value = configuration->mNativeRenderable;         break;
	case EGL_NATIVE_VISUAL_ID:           *value = configuration->mNativeVisualID;           break;
	case EGL_NATIVE_VISUAL_TYPE:         *value = configuration->mNativeVisualType;         break;
	case EGL_SAMPLES:                    *value = configuration->mSamples;                  break;
	case EGL_SAMPLE_BUFFERS:             *value = configuration->mSampleBuffers;            break;
	case EGL_SURFACE_TYPE:               *value = configuration->mSurfaceType;              break;
	case EGL_TRANSPARENT_TYPE:           *value = configuration->mTransparentType;          break;
	case EGL_TRANSPARENT_BLUE_VALUE:     *value = configuration->mTransparentBlueValue;     break;
	case EGL_TRANSPARENT_GREEN_VALUE:    *value = configuration->mTransparentGreenValue;    break;
	case EGL_TRANSPARENT_RED_VALUE:      *value = configuration->mTransparentRedValue;      break;
	case EGL_BIND_TO_TEXTURE_RGB:        *value = configuration->mBindToTextureRGB;         break;
	case EGL_BIND_TO_TEXTURE_RGBA:       *value = configuration->mBindToTextureRGBA;        break;
	case EGL_MIN_SWAP_INTERVAL:          *value = configuration->mMinSwapInterval;          break;
	case EGL_MAX_SWAP_INTERVAL:          *value = configuration->mMaxSwapInterval;          break;
	case EGL_LUMINANCE_SIZE:             *value = configuration->mLuminanceSize;            break;
	case EGL_ALPHA_MASK_SIZE:            *value = configuration->mAlphaMaskSize;            break;
	case EGL_COLOR_BUFFER_TYPE:          *value = configuration->mColorBufferType;          break;
	case EGL_RENDERABLE_TYPE:            *value = configuration->mRenderableType;           break;
	case EGL_MATCH_NATIVE_PIXMAP:        *value = EGL_FALSE; UNIMPLEMENTED();               break;
	case EGL_CONFORMANT:                 *value = configuration->mConformant;               break;
	case EGL_MAX_PBUFFER_WIDTH:          *value = configuration->mMaxPBufferWidth;          break;
	case EGL_MAX_PBUFFER_HEIGHT:         *value = configuration->mMaxPBufferHeight;         break;
	case EGL_MAX_PBUFFER_PIXELS:         *value = configuration->mMaxPBufferPixels;         break;
	case EGL_RECORDABLE_ANDROID:         *value = configuration->mRecordableAndroid;        break;
	case EGL_FRAMEBUFFER_TARGET_ANDROID: *value = configuration->mFramebufferTargetAndroid; break;
	default:
		return false;
	}

	return true;
}

EGLSurface Display::createWindowSurface(EGLNativeWindowType window, EGLConfig config, const EGLint *attribList)
{
	const Config *configuration = mConfigSet.get(config);

	if(attribList)
	{
		while(*attribList != EGL_NONE)
		{
			switch(attribList[0])
			{
			case EGL_RENDER_BUFFER:
				switch(attribList[1])
				{
				case EGL_BACK_BUFFER:
					break;
				case EGL_SINGLE_BUFFER:
					return error(EGL_BAD_MATCH, EGL_NO_SURFACE);   // Rendering directly to front buffer not supported
				default:
					return error(EGL_BAD_ATTRIBUTE, EGL_NO_SURFACE);
				}
				break;
			case EGL_VG_COLORSPACE:
				return error(EGL_BAD_MATCH, EGL_NO_SURFACE);
			case EGL_VG_ALPHA_FORMAT:
				return error(EGL_BAD_MATCH, EGL_NO_SURFACE);
			default:
				return error(EGL_BAD_ATTRIBUTE, EGL_NO_SURFACE);
			}

			attribList += 2;
		}
	}

	if(hasExistingWindowSurface(window))
	{
		return error(EGL_BAD_ALLOC, EGL_NO_SURFACE);
	}

	Surface *surface = new WindowSurface(this, configuration, window);

	if(!surface->initialize())
	{
		surface->release();
		return EGL_NO_SURFACE;
	}

	surface->addRef();
	mSurfaceSet.insert(surface);

	return success(surface);
}

EGLSurface Display::createPBufferSurface(EGLConfig config, const EGLint *attribList, EGLClientBuffer clientBuffer)
{
	EGLint width = -1, height = -1, ioSurfacePlane = -1;
	EGLenum textureFormat = EGL_NO_TEXTURE;
	EGLenum textureTarget = EGL_NO_TEXTURE;
	EGLenum clientBufferFormat = EGL_NO_TEXTURE;
	EGLenum clientBufferType = EGL_NO_TEXTURE;
	EGLBoolean largestPBuffer = EGL_FALSE;
	const Config *configuration = mConfigSet.get(config);

	if(attribList)
	{
		while(*attribList != EGL_NONE)
		{
			switch(attribList[0])
			{
			case EGL_WIDTH:
				width = attribList[1];
				break;
			case EGL_HEIGHT:
				height = attribList[1];
				break;
			case EGL_LARGEST_PBUFFER:
				largestPBuffer = attribList[1];
				break;
			case EGL_TEXTURE_FORMAT:
				switch(attribList[1])
				{
				case EGL_NO_TEXTURE:
				case EGL_TEXTURE_RGB:
				case EGL_TEXTURE_RGBA:
					textureFormat = attribList[1];
					break;
				default:
					return error(EGL_BAD_ATTRIBUTE, EGL_NO_SURFACE);
				}
				break;
			case EGL_TEXTURE_INTERNAL_FORMAT_ANGLE:
				switch(attribList[1])
				{
				case GL_RED:
				case GL_R16UI:
				case GL_RG:
				case GL_BGRA_EXT:
				case GL_RGBA:
					clientBufferFormat = attribList[1];
					break;
				default:
					return error(EGL_BAD_ATTRIBUTE, EGL_NO_SURFACE);
				}
				break;
			case EGL_TEXTURE_TYPE_ANGLE:
				switch(attribList[1])
				{
				case GL_UNSIGNED_BYTE:
				case GL_UNSIGNED_SHORT:
				case GL_HALF_FLOAT_OES:
				case GL_HALF_FLOAT:
					clientBufferType = attribList[1];
					break;
				default:
					return error(EGL_BAD_ATTRIBUTE, EGL_NO_SURFACE);
				}
				break;
			case EGL_IOSURFACE_PLANE_ANGLE:
				if(attribList[1] < 0)
				{
					return error(EGL_BAD_ATTRIBUTE, EGL_NO_SURFACE);
				}
				ioSurfacePlane = attribList[1];
				break;
			case EGL_TEXTURE_TARGET:
				switch(attribList[1])
				{
				case EGL_NO_TEXTURE:
				case EGL_TEXTURE_2D:
				case EGL_TEXTURE_RECTANGLE_ANGLE:
					textureTarget = attribList[1];
					break;
				default:
					return error(EGL_BAD_ATTRIBUTE, EGL_NO_SURFACE);
				}
				break;
			case EGL_MIPMAP_TEXTURE:
				if(attribList[1] != EGL_FALSE)
				{
					UNIMPLEMENTED();
					return error(EGL_BAD_MATCH, EGL_NO_SURFACE);
				}
				break;
			case EGL_VG_COLORSPACE:
				return error(EGL_BAD_MATCH, EGL_NO_SURFACE);
			case EGL_VG_ALPHA_FORMAT:
				return error(EGL_BAD_MATCH, EGL_NO_SURFACE);
			default:
				return error(EGL_BAD_ATTRIBUTE, EGL_NO_SURFACE);
			}

			attribList += 2;
		}
	}

	if(width < 0 || height < 0)
	{
		return error(EGL_BAD_PARAMETER, EGL_NO_SURFACE);
	}

	if(width == 0 || height == 0)
	{
		return error(EGL_BAD_ATTRIBUTE, EGL_NO_SURFACE);
	}

	if((textureFormat != EGL_NO_TEXTURE && textureTarget == EGL_NO_TEXTURE) ||
	   (textureFormat == EGL_NO_TEXTURE && textureTarget != EGL_NO_TEXTURE))
	{
		return error(EGL_BAD_MATCH, EGL_NO_SURFACE);
	}

	if(!(configuration->mSurfaceType & EGL_PBUFFER_BIT))
	{
		return error(EGL_BAD_MATCH, EGL_NO_SURFACE);
	}

	if(clientBuffer)
	{
		switch(clientBufferType)
		{
		case GL_UNSIGNED_BYTE:
			switch(clientBufferFormat)
			{
			case GL_RED:
			case GL_RG:
			case GL_BGRA_EXT:
				break;
			case GL_R16UI:
			case GL_RGBA:
				return error(EGL_BAD_ATTRIBUTE, EGL_NO_SURFACE);
			default:
				return error(EGL_BAD_PARAMETER, EGL_NO_SURFACE);
			}
			break;
		case GL_UNSIGNED_SHORT:
			switch(clientBufferFormat)
			{
			case GL_R16UI:
				break;
			case GL_RED:
			case GL_RG:
			case GL_BGRA_EXT:
			case GL_RGBA:
				return error(EGL_BAD_ATTRIBUTE, EGL_NO_SURFACE);
			default:
				return error(EGL_BAD_PARAMETER, EGL_NO_SURFACE);
			}
			break;
		case GL_HALF_FLOAT_OES:
		case GL_HALF_FLOAT:
			switch(clientBufferFormat)
			{
			case GL_RGBA:
				break;
			case GL_RED:
			case GL_R16UI:
			case GL_RG:
			case GL_BGRA_EXT:
				return error(EGL_BAD_ATTRIBUTE, EGL_NO_SURFACE);
			default:
				return error(EGL_BAD_PARAMETER, EGL_NO_SURFACE);
			}
			break;
		default:
			return error(EGL_BAD_PARAMETER, EGL_NO_SURFACE);
		}

		if(ioSurfacePlane < 0)
		{
			return error(EGL_BAD_PARAMETER, EGL_NO_SURFACE);
		}

		if(textureFormat != EGL_TEXTURE_RGBA)
		{
			return error(EGL_BAD_ATTRIBUTE, EGL_NO_SURFACE);
		}

		if(textureTarget != EGL_TEXTURE_RECTANGLE_ANGLE)
		{
			return error(EGL_BAD_ATTRIBUTE, EGL_NO_SURFACE);
		}

#if defined(__APPLE__)
		IOSurfaceRef ioSurface = reinterpret_cast<IOSurfaceRef>(clientBuffer);
		size_t planeCount = IOSurfaceGetPlaneCount(ioSurface);
		if((static_cast<size_t>(width) > IOSurfaceGetWidthOfPlane(ioSurface, ioSurfacePlane)) ||
		   (static_cast<size_t>(height) > IOSurfaceGetHeightOfPlane(ioSurface, ioSurfacePlane)) ||
		   ((planeCount != 0) && static_cast<size_t>(ioSurfacePlane) >= planeCount))
		{
			return error(EGL_BAD_ATTRIBUTE, EGL_NO_SURFACE);
		}
#endif
	}
	else
	{
		if((textureFormat == EGL_TEXTURE_RGB && configuration->mBindToTextureRGB != EGL_TRUE) ||
		   ((textureFormat == EGL_TEXTURE_RGBA && configuration->mBindToTextureRGBA != EGL_TRUE)))
		{
			return error(EGL_BAD_ATTRIBUTE, EGL_NO_SURFACE);
		}
	}

	Surface *surface = new PBufferSurface(this, configuration, width, height, textureFormat, textureTarget, clientBufferFormat, clientBufferType, largestPBuffer, clientBuffer, ioSurfacePlane);

	if(!surface->initialize())
	{
		surface->release();
		return EGL_NO_SURFACE;
	}

	surface->addRef();
	mSurfaceSet.insert(surface);

	return success(surface);
}

EGLContext Display::createContext(EGLConfig configHandle, const egl::Context *shareContext, EGLint clientVersion)
{
	const egl::Config *config = mConfigSet.get(configHandle);
	egl::Context *context = nullptr;

	if(clientVersion == 1 && config->mRenderableType & EGL_OPENGL_ES_BIT)
	{
		if(libGLES_CM)
		{
			context = libGLES_CM->es1CreateContext(this, shareContext, config);
		}
	}
	else if((clientVersion == 2 && config->mRenderableType & EGL_OPENGL_ES2_BIT) ||
	        (clientVersion == 3 && config->mRenderableType & EGL_OPENGL_ES3_BIT))
	{
		if(libGLESv2)
		{
			context = libGLESv2->es2CreateContext(this, shareContext, clientVersion, config);
		}
	}
	else
	{
		return error(EGL_BAD_CONFIG, EGL_NO_CONTEXT);
	}

	if(!context)
	{
		return error(EGL_BAD_ALLOC, EGL_NO_CONTEXT);
	}

	context->addRef();
	mContextSet.insert(context);

	return success(context);
}

EGLSyncKHR Display::createSync(Context *context)
{
	FenceSync *fenceSync = new egl::FenceSync(context);
	LockGuard lock(mSyncSetMutex);
	mSyncSet.insert(fenceSync);
	return fenceSync;
}

void Display::destroySurface(egl::Surface *surface)
{
	surface->release();
	mSurfaceSet.erase(surface);

	if(surface == getCurrentDrawSurface())
	{
		setCurrentDrawSurface(nullptr);
	}

	if(surface == getCurrentReadSurface())
	{
		setCurrentReadSurface(nullptr);
	}
}

void Display::destroyContext(egl::Context *context)
{
	context->release();
	mContextSet.erase(context);

	if(context == getCurrentContext())
	{
		setCurrentContext(nullptr);
		setCurrentDrawSurface(nullptr);
		setCurrentReadSurface(nullptr);
	}
}

void Display::destroySync(FenceSync *sync)
{
	{
		LockGuard lock(mSyncSetMutex);
		mSyncSet.erase(sync);
	}
	delete sync;
}

bool Display::isInitialized() const
{
	return mConfigSet.size() > 0;
}

bool Display::isValidConfig(EGLConfig config)
{
	return mConfigSet.get(config) != nullptr;
}

bool Display::isValidContext(egl::Context *context)
{
	return mContextSet.find(context) != mContextSet.end();
}

bool Display::isValidSurface(egl::Surface *surface)
{
	return mSurfaceSet.find(surface) != mSurfaceSet.end();
}

bool Display::isValidWindow(EGLNativeWindowType window)
{
	#if defined(_WIN32)
		return IsWindow(window) == TRUE;
	#elif defined(__ANDROID__)
		if(!window)
		{
			ALOGE("%s called with window==NULL %s:%d", __FUNCTION__, __FILE__, __LINE__);
			return false;
		}
		if(static_cast<ANativeWindow*>(window)->common.magic != ANDROID_NATIVE_WINDOW_MAGIC)
		{
			ALOGE("%s called with window==%p bad magic %s:%d", __FUNCTION__, window, __FILE__, __LINE__);
			return false;
		}
		return true;
	#elif defined(__linux__)
		if(nativeDisplay)
		{
			XWindowAttributes windowAttributes;
			Status status = libX11->XGetWindowAttributes((::Display*)nativeDisplay, window, &windowAttributes);

			return status != 0;
		}
		return false;
	#elif defined(__APPLE__)
		return sw::OSX::IsValidWindow(window);
	#elif defined(__Fuchsia__)
		// TODO(crbug.com/800951): Integrate with Mozart.
		return true;
	#else
		#error "Display::isValidWindow unimplemented for this platform"
		return false;
	#endif
}

bool Display::hasExistingWindowSurface(EGLNativeWindowType window)
{
	for(const auto &surface : mSurfaceSet)
	{
		if(surface->isWindowSurface())
		{
			if(surface->getWindowHandle() == window)
			{
				return true;
			}
		}
	}

	return false;
}

bool Display::isValidSync(FenceSync *sync)
{
	LockGuard lock(mSyncSetMutex);
	return mSyncSet.find(sync) != mSyncSet.end();
}

EGLint Display::getMinSwapInterval() const
{
	return mMinSwapInterval;
}

EGLint Display::getMaxSwapInterval() const
{
	return mMaxSwapInterval;
}

EGLDisplay Display::getEGLDisplay() const
{
	return eglDisplay;
}

void *Display::getNativeDisplay() const
{
	return nativeDisplay;
}

EGLImageKHR Display::createSharedImage(Image *image)
{
	return reinterpret_cast<EGLImageKHR>((intptr_t)mSharedImageNameSpace.allocate(image));
}

bool Display::destroySharedImage(EGLImageKHR image)
{
	GLuint name = (GLuint)reinterpret_cast<intptr_t>(image);
	Image *eglImage = mSharedImageNameSpace.find(name);

	if(!eglImage)
	{
		return false;
	}

	eglImage->destroyShared();
	mSharedImageNameSpace.remove(name);

	return true;
}

Image *Display::getSharedImage(EGLImageKHR image)
{
	GLuint name = (GLuint)reinterpret_cast<intptr_t>(image);
	return mSharedImageNameSpace.find(name);
}

sw::Format Display::getDisplayFormat() const
{
	#if defined(_WIN32)
		HDC deviceContext = GetDC(0);
		unsigned int bpp = ::GetDeviceCaps(deviceContext, BITSPIXEL);
		ReleaseDC(0, deviceContext);

		switch(bpp)
		{
		case 32: return sw::FORMAT_X8R8G8B8;
		case 24: return sw::FORMAT_R8G8B8;
		case 16: return sw::FORMAT_R5G6B5;
		default: UNREACHABLE(bpp);   // Unexpected display mode color depth
		}
	#elif defined(__ANDROID__)
		static const char *const framebuffer[] =
		{
			"/dev/graphics/fb0",
			"/dev/fb0",
			0
		};

		for(int i = 0; framebuffer[i]; i++)
		{
			int fd = open(framebuffer[i], O_RDONLY, 0);

			if(fd != -1)
			{
				struct fb_var_screeninfo info;
				int io = ioctl(fd, FBIOGET_VSCREENINFO, &info);
				close(fd);

				if(io >= 0)
				{
					switch(info.bits_per_pixel)
					{
					case 16:
						return sw::FORMAT_R5G6B5;
					case 32:
						if(info.red.length    == 8 && info.red.offset    == 16 &&
						   info.green.length  == 8 && info.green.offset  == 8  &&
						   info.blue.length   == 8 && info.blue.offset   == 0  &&
						   info.transp.length == 0)
						{
							return sw::FORMAT_X8R8G8B8;
						}
						if(info.red.length    == 8 && info.red.offset    == 0  &&
						   info.green.length  == 8 && info.green.offset  == 8  &&
						   info.blue.length   == 8 && info.blue.offset   == 16 &&
						   info.transp.length == 0)
						{
							return sw::FORMAT_X8B8G8R8;
						}
						if(info.red.length    == 8 && info.red.offset    == 16 &&
						   info.green.length  == 8 && info.green.offset  == 8  &&
						   info.blue.length   == 8 && info.blue.offset   == 0  &&
						   info.transp.length == 8 && info.transp.offset == 24)
						{
							return sw::FORMAT_A8R8G8B8;
						}
						if(info.red.length    == 8 && info.red.offset    == 0  &&
						   info.green.length  == 8 && info.green.offset  == 8  &&
						   info.blue.length   == 8 && info.blue.offset   == 16 &&
						   info.transp.length == 8 && info.transp.offset == 24)
						{
							return sw::FORMAT_A8B8G8R8;
						}
						else UNIMPLEMENTED();
					default:
						UNIMPLEMENTED();
					}
				}
			}
		}

		// No framebuffer device found, or we're in user space
		return sw::FORMAT_X8B8G8R8;
	#elif defined(__linux__)
		if(nativeDisplay)
		{
			Screen *screen = libX11->XDefaultScreenOfDisplay((::Display*)nativeDisplay);
			unsigned int bpp = libX11->XPlanesOfScreen(screen);

			switch(bpp)
			{
			case 32: return sw::FORMAT_X8R8G8B8;
			case 24: return sw::FORMAT_R8G8B8;
			case 16: return sw::FORMAT_R5G6B5;
			default: UNREACHABLE(bpp);   // Unexpected display mode color depth
			}
		}
		else
		{
			return sw::FORMAT_X8R8G8B8;
		}
	#elif defined(__APPLE__)
		return sw::FORMAT_A8B8G8R8;
	#elif defined(__Fuchsia__)
		return sw::FORMAT_A8B8G8R8;
	#else
		#error "Display::isValidWindow unimplemented for this platform"
	#endif

	return sw::FORMAT_X8R8G8B8;
}

}
