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

#include "resource.h"
#include "Framebuffer.h"
#include "Surface.h"
#include "Common/Thread.hpp"
#include "common/debug.h"

static sw::Thread::LocalStorageKey currentTLS = TLS_OUT_OF_INDEXES;

#if !defined(_MSC_VER)
#define CONSTRUCTOR __attribute__((constructor))
#define DESTRUCTOR __attribute__((destructor))
#else
#define CONSTRUCTOR
#define DESTRUCTOR
#endif

static void glAttachThread()
{
	TRACE("()");

	gl::Current *current = (gl::Current*)sw::Thread::allocateLocalStorage(currentTLS, sizeof(gl::Current));

	if(current)
	{
		current->context = nullptr;
		current->display = nullptr;
		current->drawSurface = nullptr;
		current->readSurface = nullptr;
	}
}

static void glDetachThread()
{
	TRACE("()");

	wglMakeCurrent(NULL, NULL);

	sw::Thread::freeLocalStorage(currentTLS);
}

CONSTRUCTOR static bool glAttachProcess()
{
	TRACE("()");

	#if !(ANGLE_DISABLE_TRACE)
		FILE *debug = fopen(TRACE_OUTPUT_FILE, "rt");

		if(debug)
		{
			fclose(debug);
			debug = fopen(TRACE_OUTPUT_FILE, "wt");   // Erase
			fclose(debug);
		}
	#endif

	currentTLS = sw::Thread::allocateLocalStorageKey();

	if(currentTLS == TLS_OUT_OF_INDEXES)
	{
		return false;
	}

	glAttachThread();

	return true;
}

DESTRUCTOR static void glDetachProcess()
{
	TRACE("()");

	glDetachThread();

	sw::Thread::freeLocalStorageKey(currentTLS);
}

#if defined(_WIN32)
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

extern "C" BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
	switch(reason)
	{
	case DLL_PROCESS_ATTACH:
		#ifndef NDEBUG
			WaitForDebugger(instance);
		#endif
		return glAttachProcess();
		break;
	case DLL_THREAD_ATTACH:
		glAttachThread();
		break;
	case DLL_THREAD_DETACH:
		glDetachThread();
		break;
	case DLL_PROCESS_DETACH:
		glDetachProcess();
		break;
	default:
		break;
	}

	return TRUE;
}
#endif

namespace gl
{
static gl::Current *getCurrent(void)
{
	Current *current = (Current*)sw::Thread::getLocalStorage(currentTLS);

	if(!current)
	{
		glAttachThread();
	}

	return (Current*)sw::Thread::getLocalStorage(currentTLS);
}

void makeCurrent(Context *context, Display *display, Surface *surface)
{
	Current *current = getCurrent();

	current->context = context;
	current->display = display;

	if(context && display && surface)
	{
		context->makeCurrent(surface);
	}
}

Context *getContext()
{
	Current *current = getCurrent();

	return current->context;
}

Display *getDisplay()
{
	Current *current = getCurrent();

	return current->display;
}

Device *getDevice()
{
	Context *context = getContext();

	return context ? context->getDevice() : nullptr;
}

void setCurrentDisplay(Display *dpy)
{
	Current *current = getCurrent();

	current->display = dpy;
}

void setCurrentContext(gl::Context *ctx)
{
	Current *current = getCurrent();

	current->context = ctx;
}

void setCurrentDrawSurface(Surface *surface)
{
	Current *current = getCurrent();

	current->drawSurface = surface;
}

Surface *getCurrentDrawSurface()
{
	Current *current = getCurrent();

	return current->drawSurface;
}

void setCurrentReadSurface(Surface *surface)
{
	Current *current = getCurrent();

	current->readSurface = surface;
}

Surface *getCurrentReadSurface()
{
	Current *current = getCurrent();

	return current->readSurface;
}
}

// Records an error code
void error(GLenum errorCode)
{
	gl::Context *context = gl::getContext();

	if(context)
	{
		switch(errorCode)
		{
		case GL_INVALID_ENUM:
			context->recordInvalidEnum();
			TRACE("\t! Error generated: invalid enum\n");
			break;
		case GL_INVALID_VALUE:
			context->recordInvalidValue();
			TRACE("\t! Error generated: invalid value\n");
			break;
		case GL_INVALID_OPERATION:
			context->recordInvalidOperation();
			TRACE("\t! Error generated: invalid operation\n");
			break;
		case GL_OUT_OF_MEMORY:
			context->recordOutOfMemory();
			TRACE("\t! Error generated: out of memory\n");
			break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			context->recordInvalidFramebufferOperation();
			TRACE("\t! Error generated: invalid framebuffer operation\n");
			break;
		default: UNREACHABLE(errorCode);
		}
	}
}
