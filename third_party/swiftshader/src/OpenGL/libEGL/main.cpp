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

// main.cpp: DLL entry point and management of thread-local data.

#include "main.h"

#include "libEGL.hpp"
#include "Context.hpp"
#include "Surface.hpp"

#include "resource.h"
#include "Common/Thread.hpp"
#include "Common/SharedLibrary.hpp"
#include "common/debug.h"

#include <EGL/eglext.h>

static sw::Thread::LocalStorageKey currentTLS = TLS_OUT_OF_INDEXES;

#if !defined(_MSC_VER)
#define CONSTRUCTOR __attribute__((constructor))
#define DESTRUCTOR __attribute__((destructor))
#else
#define CONSTRUCTOR
#define DESTRUCTOR
#endif

namespace egl
{
void releaseCurrent(void *storage)
{
	// This pthread destructor is called after the TLS is already reset to NULL,
	// so we can't call EGL functions here to do the cleanup.

	Current *current = (Current*)storage;

	if(current)
	{
		if(current->drawSurface)
		{
			current->drawSurface->release();
		}

		if(current->readSurface)
		{
			current->readSurface->release();
		}

		if(current->context)
		{
			current->context->release();
		}

		free(current);
	}
}

Current *attachThread()
{
	TRACE("()");

	if(currentTLS == TLS_OUT_OF_INDEXES)
	{
		currentTLS = sw::Thread::allocateLocalStorageKey(releaseCurrent);
	}

	Current *current = (Current*)sw::Thread::allocateLocalStorage(currentTLS, sizeof(Current));

	current->error = EGL_SUCCESS;
	current->API = EGL_OPENGL_ES_API;
	current->context = nullptr;
	current->drawSurface = nullptr;
	current->readSurface = nullptr;

	return current;
}

void detachThread()
{
	TRACE("()");

	eglMakeCurrent(EGL_NO_DISPLAY, EGL_NO_CONTEXT, EGL_NO_SURFACE, EGL_NO_SURFACE);

	sw::Thread::freeLocalStorage(currentTLS);
}

CONSTRUCTOR void attachProcess()
{
	TRACE("()");

	#if !defined(ANGLE_DISABLE_TRACE) && defined(TRACE_OUTPUT_FILE)
		FILE *debug = fopen(TRACE_OUTPUT_FILE, "rt");

		if(debug)
		{
			fclose(debug);
			debug = fopen(TRACE_OUTPUT_FILE, "wt");   // Erase
			fclose(debug);
		}
	#endif

	attachThread();
}

DESTRUCTOR void detachProcess()
{
	TRACE("()");

	detachThread();
	sw::Thread::freeLocalStorageKey(currentTLS);
}
}

#if defined(_WIN32)
#ifdef DEBUGGER_WAIT_DIALOG
static INT_PTR CALLBACK DebuggerWaitDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	RECT rect;

	switch(uMsg)
	{
	case WM_INITDIALOG:
		GetWindowRect(GetDesktopWindow(), &rect);
		SetWindowPos(hwnd, HWND_TOP, rect.right / 2, rect.bottom / 2, 0, 0, SWP_NOSIZE);
		SetTimer(hwnd, 1, 100, NULL);
		return TRUE;
	case WM_COMMAND:
		if(LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hwnd, 0);
		}
		break;
	case WM_TIMER:
		if(IsDebuggerPresent())
		{
			EndDialog(hwnd, 0);
		}
	}

	return FALSE;
}

static void WaitForDebugger(HINSTANCE instance)
{
	if(!IsDebuggerPresent())
	{
		HRSRC dialog = FindResource(instance, MAKEINTRESOURCE(IDD_DIALOG1), RT_DIALOG);
		DLGTEMPLATE *dialogTemplate = (DLGTEMPLATE*)LoadResource(instance, dialog);
		DialogBoxIndirect(instance, dialogTemplate, NULL, DebuggerWaitDialogProc);
	}
}
#endif

extern "C" BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
	switch(reason)
	{
	case DLL_PROCESS_ATTACH:
		#ifdef DEBUGGER_WAIT_DIALOG
			WaitForDebugger(instance);
		#endif
		egl::attachProcess();
		break;
	case DLL_THREAD_ATTACH:
		egl::attachThread();
		break;
	case DLL_THREAD_DETACH:
		egl::detachThread();
		break;
	case DLL_PROCESS_DETACH:
		egl::detachProcess();
		break;
	default:
		break;
	}

	return TRUE;
}
#endif

namespace egl
{
static Current *getCurrent(void)
{
	Current *current = (Current*)sw::Thread::getLocalStorage(currentTLS);

	if(!current)
	{
		current = attachThread();
	}

	return current;
}

void setCurrentError(EGLint error)
{
	Current *current = getCurrent();

	current->error = error;
}

EGLint getCurrentError()
{
	Current *current = getCurrent();

	return current->error;
}

void setCurrentAPI(EGLenum API)
{
	Current *current = getCurrent();

	current->API = API;
}

EGLenum getCurrentAPI()
{
	Current *current = getCurrent();

	return current->API;
}

void setCurrentContext(egl::Context *ctx)
{
	Current *current = getCurrent();

	if(ctx)
	{
		ctx->addRef();
	}

	if(current->context)
	{
		current->context->release();
	}

	current->context = ctx;
}

NO_SANITIZE_FUNCTION egl::Context *getCurrentContext()
{
	Current *current = getCurrent();

	return current->context;
}

void setCurrentDrawSurface(egl::Surface *surface)
{
	Current *current = getCurrent();

	if(surface)
	{
		surface->addRef();
	}

	if(current->drawSurface)
	{
		current->drawSurface->release();
	}

	current->drawSurface = surface;
}

egl::Surface *getCurrentDrawSurface()
{
	Current *current = getCurrent();

	return current->drawSurface;
}

void setCurrentReadSurface(egl::Surface *surface)
{
	Current *current = getCurrent();

	if(surface)
	{
		surface->addRef();
	}

	if(current->readSurface)
	{
		current->readSurface->release();
	}

	current->readSurface = surface;
}

egl::Surface *getCurrentReadSurface()
{
	Current *current = getCurrent();

	return current->readSurface;
}

void error(EGLint errorCode)
{
	egl::setCurrentError(errorCode);

	if(errorCode != EGL_SUCCESS)
	{
		switch(errorCode)
		{
		case EGL_NOT_INITIALIZED:     TRACE("\t! Error generated: not initialized\n");     break;
		case EGL_BAD_ACCESS:          TRACE("\t! Error generated: bad access\n");          break;
		case EGL_BAD_ALLOC:           TRACE("\t! Error generated: bad alloc\n");           break;
		case EGL_BAD_ATTRIBUTE:       TRACE("\t! Error generated: bad attribute\n");       break;
		case EGL_BAD_CONFIG:          TRACE("\t! Error generated: bad config\n");          break;
		case EGL_BAD_CONTEXT:         TRACE("\t! Error generated: bad context\n");         break;
		case EGL_BAD_CURRENT_SURFACE: TRACE("\t! Error generated: bad current surface\n"); break;
		case EGL_BAD_DISPLAY:         TRACE("\t! Error generated: bad display\n");         break;
		case EGL_BAD_MATCH:           TRACE("\t! Error generated: bad match\n");           break;
		case EGL_BAD_NATIVE_PIXMAP:   TRACE("\t! Error generated: bad native pixmap\n");   break;
		case EGL_BAD_NATIVE_WINDOW:   TRACE("\t! Error generated: bad native window\n");   break;
		case EGL_BAD_PARAMETER:       TRACE("\t! Error generated: bad parameter\n");       break;
		case EGL_BAD_SURFACE:         TRACE("\t! Error generated: bad surface\n");         break;
		case EGL_CONTEXT_LOST:        TRACE("\t! Error generated: context lost\n");        break;
		default:                      TRACE("\t! Error generated: <0x%X>\n", errorCode);   break;
		}
	}
}
}

namespace egl
{
EGLint GetError(void);
EGLDisplay GetDisplay(EGLNativeDisplayType display_id);
EGLBoolean Initialize(EGLDisplay dpy, EGLint *major, EGLint *minor);
EGLBoolean Terminate(EGLDisplay dpy);
const char *QueryString(EGLDisplay dpy, EGLint name);
EGLBoolean GetConfigs(EGLDisplay dpy, EGLConfig *configs, EGLint config_size, EGLint *num_config);
EGLBoolean ChooseConfig(EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config);
EGLBoolean GetConfigAttrib(EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value);
EGLSurface CreateWindowSurface(EGLDisplay dpy, EGLConfig config, EGLNativeWindowType window, const EGLint *attrib_list);
EGLSurface CreatePbufferSurface(EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list);
EGLSurface CreatePixmapSurface(EGLDisplay dpy, EGLConfig config, EGLNativePixmapType pixmap, const EGLint *attrib_list);
EGLBoolean DestroySurface(EGLDisplay dpy, EGLSurface surface);
EGLBoolean QuerySurface(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint *value);
EGLBoolean BindAPI(EGLenum api);
EGLenum QueryAPI(void);
EGLBoolean WaitClient(void);
EGLBoolean ReleaseThread(void);
EGLSurface CreatePbufferFromClientBuffer(EGLDisplay dpy, EGLenum buftype, EGLClientBuffer buffer, EGLConfig config, const EGLint *attrib_list);
EGLBoolean SurfaceAttrib(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value);
EGLBoolean BindTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer);
EGLBoolean ReleaseTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer);
EGLBoolean SwapInterval(EGLDisplay dpy, EGLint interval);
EGLContext CreateContext(EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint *attrib_list);
EGLBoolean DestroyContext(EGLDisplay dpy, EGLContext ctx);
EGLBoolean MakeCurrent(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx);
EGLContext GetCurrentContext(void);
EGLSurface GetCurrentSurface(EGLint readdraw);
EGLDisplay GetCurrentDisplay(void);
EGLBoolean QueryContext(EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint *value);
EGLBoolean WaitGL(void);
EGLBoolean WaitNative(EGLint engine);
EGLBoolean SwapBuffers(EGLDisplay dpy, EGLSurface surface);
EGLBoolean CopyBuffers(EGLDisplay dpy, EGLSurface surface, EGLNativePixmapType target);
EGLImageKHR CreateImageKHR(EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list);
EGLBoolean DestroyImageKHR(EGLDisplay dpy, EGLImageKHR image);
EGLDisplay GetPlatformDisplayEXT(EGLenum platform, void *native_display, const EGLint *attrib_list);
EGLSurface CreatePlatformWindowSurfaceEXT(EGLDisplay dpy, EGLConfig config, void *native_window, const EGLint *attrib_list);
EGLSurface CreatePlatformPixmapSurfaceEXT(EGLDisplay dpy, EGLConfig config, void *native_pixmap, const EGLint *attrib_list);
EGLSyncKHR CreateSyncKHR(EGLDisplay dpy, EGLenum type, const EGLint *attrib_list);
EGLBoolean DestroySyncKHR(EGLDisplay dpy, EGLSyncKHR sync);
EGLint ClientWaitSyncKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLint flags, EGLTimeKHR timeout);
EGLBoolean GetSyncAttribKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLint attribute, EGLint *value);
__eglMustCastToProperFunctionPointerType GetProcAddress(const char *procname);
}

extern "C"
{
EGLAPI EGLint EGLAPIENTRY eglGetError(void)
{
	return egl::GetError();
}

EGLAPI EGLDisplay EGLAPIENTRY eglGetDisplay(EGLNativeDisplayType display_id)
{
	return egl::GetDisplay(display_id);
}

EGLAPI EGLBoolean EGLAPIENTRY eglInitialize(EGLDisplay dpy, EGLint *major, EGLint *minor)
{
	return egl::Initialize(dpy, major, minor);
}

EGLAPI EGLBoolean EGLAPIENTRY eglTerminate(EGLDisplay dpy)
{
	return egl::Terminate(dpy);
}

EGLAPI const char *EGLAPIENTRY eglQueryString(EGLDisplay dpy, EGLint name)
{
	return egl::QueryString(dpy, name);
}

EGLAPI EGLBoolean EGLAPIENTRY eglGetConfigs(EGLDisplay dpy, EGLConfig *configs, EGLint config_size, EGLint *num_config)
{
	return egl::GetConfigs(dpy, configs, config_size, num_config);
}

EGLAPI EGLBoolean EGLAPIENTRY eglChooseConfig(EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config)
{
	return egl::ChooseConfig(dpy, attrib_list, configs, config_size, num_config);
}

EGLAPI EGLBoolean EGLAPIENTRY eglGetConfigAttrib(EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value)
{
	return egl::GetConfigAttrib(dpy, config, attribute, value);
}

EGLAPI EGLSurface EGLAPIENTRY eglCreateWindowSurface(EGLDisplay dpy, EGLConfig config, EGLNativeWindowType window, const EGLint *attrib_list)
{
	return egl::CreateWindowSurface(dpy, config, window, attrib_list);
}

EGLAPI EGLSurface EGLAPIENTRY eglCreatePbufferSurface(EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list)
{
	return egl::CreatePbufferSurface(dpy, config, attrib_list);
}

EGLAPI EGLSurface EGLAPIENTRY eglCreatePixmapSurface(EGLDisplay dpy, EGLConfig config, EGLNativePixmapType pixmap, const EGLint *attrib_list)
{
	return egl::CreatePixmapSurface(dpy, config, pixmap, attrib_list);
}

EGLAPI EGLBoolean EGLAPIENTRY eglDestroySurface(EGLDisplay dpy, EGLSurface surface)
{
	return egl::DestroySurface(dpy, surface);
}

EGLAPI EGLBoolean EGLAPIENTRY eglQuerySurface(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint *value)
{
	return egl::QuerySurface(dpy, surface, attribute, value);
}

EGLAPI EGLBoolean EGLAPIENTRY eglBindAPI(EGLenum api)
{
	return egl::BindAPI(api);
}

EGLAPI EGLenum EGLAPIENTRY eglQueryAPI(void)
{
	return egl::QueryAPI();
}

EGLAPI EGLBoolean EGLAPIENTRY eglWaitClient(void)
{
	return egl::WaitClient();
}

EGLAPI EGLBoolean EGLAPIENTRY eglReleaseThread(void)
{
	return egl::ReleaseThread();
}

EGLAPI EGLSurface EGLAPIENTRY eglCreatePbufferFromClientBuffer(EGLDisplay dpy, EGLenum buftype, EGLClientBuffer buffer, EGLConfig config, const EGLint *attrib_list)
{
	return egl::CreatePbufferFromClientBuffer(dpy, buftype, buffer, config, attrib_list);
}

EGLAPI EGLBoolean EGLAPIENTRY eglSurfaceAttrib(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value)
{
	return egl::SurfaceAttrib(dpy, surface, attribute, value);
}

EGLAPI EGLBoolean EGLAPIENTRY eglBindTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
	return egl::BindTexImage(dpy, surface, buffer);
}

EGLAPI EGLBoolean EGLAPIENTRY eglReleaseTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
	return egl::ReleaseTexImage(dpy, surface, buffer);
}

EGLAPI EGLBoolean EGLAPIENTRY eglSwapInterval(EGLDisplay dpy, EGLint interval)
{
	return egl::SwapInterval(dpy, interval);
}

EGLAPI EGLContext EGLAPIENTRY eglCreateContext(EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint *attrib_list)
{
	return egl::CreateContext(dpy, config, share_context, attrib_list);
}

EGLAPI EGLBoolean EGLAPIENTRY eglDestroyContext(EGLDisplay dpy, EGLContext ctx)
{
	return egl::DestroyContext(dpy, ctx);
}

EGLAPI EGLBoolean EGLAPIENTRY eglMakeCurrent(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx)
{
	return egl::MakeCurrent(dpy, draw, read, ctx);
}

EGLAPI EGLContext EGLAPIENTRY eglGetCurrentContext(void)
{
	return egl::GetCurrentContext();
}

EGLAPI EGLSurface EGLAPIENTRY eglGetCurrentSurface(EGLint readdraw)
{
	return egl::GetCurrentSurface(readdraw);
}

EGLAPI EGLDisplay EGLAPIENTRY eglGetCurrentDisplay(void)
{
	return egl::GetCurrentDisplay();
}

EGLAPI EGLBoolean EGLAPIENTRY eglQueryContext(EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint *value)
{
	return egl::QueryContext(dpy, ctx, attribute, value);
}

EGLAPI EGLBoolean EGLAPIENTRY eglWaitGL(void)
{
	return egl::WaitClient();
}

EGLAPI EGLBoolean EGLAPIENTRY eglWaitNative(EGLint engine)
{
	return egl::WaitNative(engine);
}

EGLAPI EGLBoolean EGLAPIENTRY eglSwapBuffers(EGLDisplay dpy, EGLSurface surface)
{
	return egl::SwapBuffers(dpy, surface);
}

EGLAPI EGLBoolean EGLAPIENTRY eglCopyBuffers(EGLDisplay dpy, EGLSurface surface, EGLNativePixmapType target)
{
	return egl::CopyBuffers(dpy, surface, target);
}

EGLAPI EGLImageKHR EGLAPIENTRY eglCreateImageKHR(EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list)
{
	return egl::CreateImageKHR(dpy, ctx, target, buffer, attrib_list);
}

EGLAPI EGLBoolean EGLAPIENTRY eglDestroyImageKHR(EGLDisplay dpy, EGLImageKHR image)
{
	return egl::DestroyImageKHR(dpy, image);
}

EGLAPI EGLDisplay EGLAPIENTRY eglGetPlatformDisplayEXT(EGLenum platform, void *native_display, const EGLint *attrib_list)
{
	return egl::GetPlatformDisplayEXT(platform, native_display, attrib_list);
}

EGLAPI EGLSurface EGLAPIENTRY eglCreatePlatformWindowSurfaceEXT(EGLDisplay dpy, EGLConfig config, void *native_window, const EGLint *attrib_list)
{
	return egl::CreatePlatformWindowSurfaceEXT(dpy, config, native_window, attrib_list);
}

EGLAPI EGLSurface EGLAPIENTRY eglCreatePlatformPixmapSurfaceEXT(EGLDisplay dpy, EGLConfig config, void *native_pixmap, const EGLint *attrib_list)
{
	return egl::CreatePlatformPixmapSurfaceEXT(dpy, config, native_pixmap, attrib_list);
}

EGLAPI EGLSyncKHR EGLAPIENTRY eglCreateSyncKHR(EGLDisplay dpy, EGLenum type, const EGLint *attrib_list)
{
	return egl::CreateSyncKHR(dpy, type, attrib_list);
}

EGLAPI EGLBoolean EGLAPIENTRY eglDestroySyncKHR(EGLDisplay dpy, EGLSyncKHR sync)
{
	return egl::DestroySyncKHR(dpy, sync);
}

EGLAPI EGLint EGLAPIENTRY eglClientWaitSyncKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLint flags, EGLTimeKHR timeout)
{
	return egl::ClientWaitSyncKHR(dpy, sync, flags, timeout);
}

EGLAPI EGLBoolean EGLAPIENTRY eglGetSyncAttribKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLint attribute, EGLint *value)
{
	return egl::GetSyncAttribKHR(dpy, sync, attribute, value);
}

EGLAPI __eglMustCastToProperFunctionPointerType EGLAPIENTRY eglGetProcAddress(const char *procname)
{
	return egl::GetProcAddress(procname);
}
}

LibEGLexports::LibEGLexports()
{
	this->eglGetError = egl::GetError;
	this->eglGetDisplay = egl::GetDisplay;
	this->eglInitialize = egl::Initialize;
	this->eglTerminate = egl::Terminate;
	this->eglQueryString = egl::QueryString;
	this->eglGetConfigs = egl::GetConfigs;
	this->eglChooseConfig = egl::ChooseConfig;
	this->eglGetConfigAttrib = egl::GetConfigAttrib;
	this->eglCreateWindowSurface = egl::CreateWindowSurface;
	this->eglCreatePbufferSurface = egl::CreatePbufferSurface;
	this->eglCreatePixmapSurface = egl::CreatePixmapSurface;
	this->eglDestroySurface = egl::DestroySurface;
	this->eglQuerySurface = egl::QuerySurface;
	this->eglBindAPI = egl::BindAPI;
	this->eglQueryAPI = egl::QueryAPI;
	this->eglWaitClient = egl::WaitClient;
	this->eglReleaseThread = egl::ReleaseThread;
	this->eglCreatePbufferFromClientBuffer = egl::CreatePbufferFromClientBuffer;
	this->eglSurfaceAttrib = egl::SurfaceAttrib;
	this->eglBindTexImage = egl::BindTexImage;
	this->eglReleaseTexImage = egl::ReleaseTexImage;
	this->eglSwapInterval = egl::SwapInterval;
	this->eglCreateContext = egl::CreateContext;
	this->eglDestroyContext = egl::DestroyContext;
	this->eglMakeCurrent = egl::MakeCurrent;
	this->eglGetCurrentContext = egl::GetCurrentContext;
	this->eglGetCurrentSurface = egl::GetCurrentSurface;
	this->eglGetCurrentDisplay = egl::GetCurrentDisplay;
	this->eglQueryContext = egl::QueryContext;
	this->eglWaitGL = egl::WaitGL;
	this->eglWaitNative = egl::WaitNative;
	this->eglSwapBuffers = egl::SwapBuffers;
	this->eglCopyBuffers = egl::CopyBuffers;
	this->eglCreateImageKHR = egl::CreateImageKHR;
	this->eglDestroyImageKHR = egl::DestroyImageKHR;
	this->eglGetProcAddress = egl::GetProcAddress;
	this->eglCreateSyncKHR = egl::CreateSyncKHR;
	this->eglDestroySyncKHR = egl::DestroySyncKHR;
	this->eglClientWaitSyncKHR = egl::ClientWaitSyncKHR;
	this->eglGetSyncAttribKHR = egl::GetSyncAttribKHR;

	this->clientGetCurrentContext = egl::getCurrentContext;
}

extern "C" EGLAPI LibEGLexports *libEGL_swiftshader()
{
	static LibEGLexports libEGL;
	return &libEGL;
}

LibGLES_CM libGLES_CM(getLibraryDirectoryFromSymbol((void*)libEGL_swiftshader));
LibGLESv2 libGLESv2(getLibraryDirectoryFromSymbol((void*)libEGL_swiftshader));
