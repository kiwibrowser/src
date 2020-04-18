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

// Display.h: Defines the egl::Display class, representing the abstract
// display on which graphics are drawn. Implements EGLDisplay.
// [EGL 1.4] section 2.1.2 page 3.

#ifndef INCLUDE_DISPLAY_H_
#define INCLUDE_DISPLAY_H_

#include "Config.h"
#include "Common/MutexLock.hpp"
#include "Sync.hpp"
#include "common/NameSpace.hpp"

#include <set>

#ifndef EGL_ANGLE_iosurface_client_buffer
#define EGL_ANGLE_iosurface_client_buffer 1
#define EGL_IOSURFACE_ANGLE 0x3454
#define EGL_IOSURFACE_PLANE_ANGLE 0x345A
#define EGL_TEXTURE_RECTANGLE_ANGLE 0x345B
#define EGL_TEXTURE_TYPE_ANGLE 0x345C
#define EGL_TEXTURE_INTERNAL_FORMAT_ANGLE 0x345D
#endif // EGL_ANGLE_iosurface_client_buffer

namespace egl
{
	class Surface;
	class Context;
	class Image;

	const EGLDisplay PRIMARY_DISPLAY  = reinterpret_cast<EGLDisplay>((intptr_t)1);
	const EGLDisplay HEADLESS_DISPLAY = reinterpret_cast<EGLDisplay>((intptr_t)0xFACE1E55);

	class [[clang::lto_visibility_public]] Display
	{
	protected:
		explicit Display(EGLDisplay eglDisplay, void *nativeDisplay);
		virtual ~Display() = 0;

	public:
		static Display *get(EGLDisplay dpy);

		bool initialize();
		void terminate();

		bool getConfigs(EGLConfig *configs, const EGLint *attribList, EGLint configSize, EGLint *numConfig);
		bool getConfigAttrib(EGLConfig config, EGLint attribute, EGLint *value);

		EGLSurface createWindowSurface(EGLNativeWindowType window, EGLConfig config, const EGLint *attribList);
		EGLSurface createPBufferSurface(EGLConfig config, const EGLint *attribList, EGLClientBuffer clientBuffer = nullptr);
		EGLContext createContext(EGLConfig configHandle, const Context *shareContext, EGLint clientVersion);
		EGLSyncKHR createSync(Context *context);

		void destroySurface(Surface *surface);
		void destroyContext(Context *context);
		void destroySync(FenceSync *sync);

		bool isInitialized() const;
		bool isValidConfig(EGLConfig config);
		bool isValidContext(Context *context);
		bool isValidSurface(Surface *surface);
		bool isValidWindow(EGLNativeWindowType window);
		bool hasExistingWindowSurface(EGLNativeWindowType window);
		bool isValidSync(FenceSync *sync);

		EGLint getMinSwapInterval() const;
		EGLint getMaxSwapInterval() const;

		EGLDisplay getEGLDisplay() const;
		void *getNativeDisplay() const;

		EGLImageKHR createSharedImage(Image *image);
		bool destroySharedImage(EGLImageKHR);
		virtual Image *getSharedImage(EGLImageKHR name) = 0;

	private:
		sw::Format getDisplayFormat() const;

		const EGLDisplay eglDisplay;
		void *const nativeDisplay;

		EGLint mMaxSwapInterval;
		EGLint mMinSwapInterval;

		typedef std::set<Surface*> SurfaceSet;
		SurfaceSet mSurfaceSet;

		ConfigSet mConfigSet;

		typedef std::set<Context*> ContextSet;
		ContextSet mContextSet;

		typedef std::set<FenceSync*> SyncSet;
		sw::MutexLock mSyncSetMutex;
		SyncSet mSyncSet;

		gl::NameSpace<Image> mSharedImageNameSpace;
	};
}

#endif   // INCLUDE_DISPLAY_H_
