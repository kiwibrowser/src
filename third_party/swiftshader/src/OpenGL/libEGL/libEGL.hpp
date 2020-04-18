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

#ifndef libEGL_hpp
#define libEGL_hpp

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "Common/SharedLibrary.hpp"

class LibEGLexports
{
public:
	LibEGLexports();

	EGLint (*eglGetError)(void);
	EGLDisplay (*eglGetDisplay)(EGLNativeDisplayType display_id);
	EGLBoolean (*eglInitialize)(EGLDisplay dpy, EGLint *major, EGLint *minor);
	EGLBoolean (*eglTerminate)(EGLDisplay dpy);
	const char *(*eglQueryString)(EGLDisplay dpy, EGLint name);
	EGLBoolean (*eglGetConfigs)(EGLDisplay dpy, EGLConfig *configs, EGLint config_size, EGLint *num_config);
	EGLBoolean (*eglChooseConfig)(EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config);
	EGLBoolean (*eglGetConfigAttrib)(EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value);
	EGLSurface (*eglCreateWindowSurface)(EGLDisplay dpy, EGLConfig config, EGLNativeWindowType window, const EGLint *attrib_list);
	EGLSurface (*eglCreatePbufferSurface)(EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list);
	EGLSurface (*eglCreatePixmapSurface)(EGLDisplay dpy, EGLConfig config, EGLNativePixmapType pixmap, const EGLint *attrib_list);
	EGLBoolean (*eglDestroySurface)(EGLDisplay dpy, EGLSurface surface);
	EGLBoolean (*eglQuerySurface)(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint *value);
	EGLBoolean (*eglBindAPI)(EGLenum api);
	EGLenum (*eglQueryAPI)(void);
	EGLBoolean (*eglWaitClient)(void);
	EGLBoolean (*eglReleaseThread)(void);
	EGLSurface (*eglCreatePbufferFromClientBuffer)(EGLDisplay dpy, EGLenum buftype, EGLClientBuffer buffer, EGLConfig config, const EGLint *attrib_list);
	EGLBoolean (*eglSurfaceAttrib)(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value);
	EGLBoolean (*eglBindTexImage)(EGLDisplay dpy, EGLSurface surface, EGLint buffer);
	EGLBoolean (*eglReleaseTexImage)(EGLDisplay dpy, EGLSurface surface, EGLint buffer);
	EGLBoolean (*eglSwapInterval)(EGLDisplay dpy, EGLint interval);
	EGLContext (*eglCreateContext)(EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint *attrib_list);
	EGLBoolean (*eglDestroyContext)(EGLDisplay dpy, EGLContext ctx);
	EGLBoolean (*eglMakeCurrent)(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx);
	EGLContext (*eglGetCurrentContext)(void);
	EGLSurface (*eglGetCurrentSurface)(EGLint readdraw);
	EGLDisplay (*eglGetCurrentDisplay)(void);
	EGLBoolean (*eglQueryContext)(EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint *value);
	EGLBoolean (*eglWaitGL)(void);
	EGLBoolean (*eglWaitNative)(EGLint engine);
	EGLBoolean (*eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
	EGLBoolean (*eglCopyBuffers)(EGLDisplay dpy, EGLSurface surface, EGLNativePixmapType target);
	EGLImageKHR (*eglCreateImageKHR)(EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list);
	EGLBoolean (*eglDestroyImageKHR)(EGLDisplay dpy, EGLImageKHR image);
	__eglMustCastToProperFunctionPointerType (*eglGetProcAddress)(const char*);
	EGLSyncKHR (*eglCreateSyncKHR)(EGLDisplay dpy, EGLenum type, const EGLint *attrib_list);
	EGLBoolean (*eglDestroySyncKHR)(EGLDisplay dpy, EGLSyncKHR sync);
	EGLint (*eglClientWaitSyncKHR)(EGLDisplay dpy, EGLSyncKHR sync, EGLint flags, EGLTimeKHR timeout);
	EGLBoolean (*eglGetSyncAttribKHR)(EGLDisplay dpy, EGLSyncKHR sync, EGLint attribute, EGLint *value);

	// Functions that don't change the error code, for use by client APIs
	egl::Context *(*clientGetCurrentContext)();
};

class LibEGL
{
public:
	LibEGL(const std::string libraryDirectory) : libraryDirectory(libraryDirectory)
	{
	}

	~LibEGL()
	{
		freeLibrary(libEGL);
	}

	LibEGLexports *operator->()
	{
		return loadExports();
	}

private:
	LibEGLexports *loadExports()
	{
		if(!libEGL)
		{
			#if defined(_WIN32)
				#if defined(__LP64__)
					const char *libEGL_lib[] = {"libEGL.dll", "lib64EGL_translator.dll"};
				#else
					const char *libEGL_lib[] = {"libEGL.dll", "libEGL_translator.dll"};
				#endif
			#elif defined(__ANDROID__)
				#if defined(__LP64__)
					const char *libEGL_lib[] = {"/vendor/lib64/egl/libEGL_swiftshader.so", "/system/lib64/egl/libEGL_swiftshader.so"};
				#else
					const char *libEGL_lib[] = {"/vendor/lib/egl/libEGL_swiftshader.so", "/system/lib/egl/libEGL_swiftshader.so"};
				#endif
			#elif defined(__linux__)
				#if defined(__LP64__)
					const char *libEGL_lib[] = {"lib64EGL_translator.so", "libEGL.so.1", "libEGL.so"};
				#else
					const char *libEGL_lib[] = {"libEGL_translator.so", "libEGL.so.1", "libEGL.so"};
				#endif
			#elif defined(__APPLE__)
				#if defined(__LP64__)
					const char *libEGL_lib[] = {"libswiftshader_libEGL.dylib", "lib64EGL_translator.dylib", "libEGL.so", "libEGL.dylib"};
				#else
					const char *libEGL_lib[] = {"libswiftshader_libEGL.dylib", "libEGL_translator.dylib", "libEGL.so", "libEGL.dylib"};
				#endif
			#elif defined(__Fuchsia__)
				const char *libEGL_lib[] = {"libEGL.so"};
			#else
				#error "libEGL::loadExports unimplemented for this platform"
			#endif

			libEGL = loadLibrary(libraryDirectory, libEGL_lib, "libEGL_swiftshader");

			if(libEGL)
			{
				auto libEGL_swiftshader = (LibEGLexports *(*)())getProcAddress(libEGL, "libEGL_swiftshader");
				libEGLexports = libEGL_swiftshader();
			}
		}

		return libEGLexports;
	}

	void *libEGL = nullptr;
	LibEGLexports *libEGLexports = nullptr;
	const std::string libraryDirectory;
};

#endif   // libEGL_hpp
